
#include "gateserver.h"
#include "log.h"
#include <stdexcept>
#include <condition_variable>
using namespace std;
using namespace dbc;

ToGameServer::ToGameServer(char const* fname, ThreadPool* proc, ThreadPool* comm)
	: TcpServerApp(this, proc, comm), RPCMGR(this), _mut_game(),
	  _game_heap(1, 20), _game_list(nullptr), _map_game() {
	_mut_game.Create(false);
	_game_num = 0;

	// ¿ªÊ¼¼àÌý
	IniFile inf(fname);
	IniSection& is = inf["ToGameServer"];
	const std::string ip = is["IP"];
	uShort port = std::stoi(is["Port"]);

	// Æô¶¯ PING Ïß³Ì

	SetPKParse(0, 2, 64 * 1024, 400);
	BeginWork(std::stoi(is["EnablePing"]));
	if (OpenListenSocket(port, ip.c_str()) != 0) {
		throw std::runtime_error("ToGameServer listen failed");
	}
}

ToGameServer::~ToGameServer() { ShutDown(12 * 1000); }

void ToGameServer::_add_game(GameServer* game) {
	game->next = _game_list;
	_game_list = game;
	++_game_num;
}

bool ToGameServer::_exist_game(char const* game) {
	GameServer* curr = _game_list;
	bool exist = false;

	while (curr) {
		if (curr->gamename == game) {
			exist = true;
			break;
		}
		curr = curr->next;
	}

	return exist;
}

void ToGameServer::_del_game(GameServer* game) {
	GameServer* curr = _game_list;
	GameServer* prev = 0;
	while (curr) {
		if (curr == game)
			break;
		prev = curr;
		curr = curr->next;
	}

	if (curr) {
		if (prev)
			prev->next = curr->next;
		else
			_game_list = curr->next;
		--_game_num;
	}
}

void ToGameServer::TaskDispatcher(Task* task) {
	extern std::mutex global_gate_ready_mutex;
	extern std::condition_variable global_gate_ready_cv;
	extern bool is_global_gate_ready;
	std::unique_lock<std::mutex> lk(global_gate_ready_mutex);
	global_gate_ready_cv.wait(lk, [] { return is_global_gate_ready; });

	lk.unlock();

	TcpServerApp::TaskDispatcher(task);
}

bool ToGameServer::OnConnect(DataSocket* datasock) // ·µ»ØÖµ:true-ÔÊÐíÁ¬½Ó,false-²»ÔÊÐíÁ¬½Ó
{
	datasock->SetPointer(0);
	datasock->SetRecvBuf(64 * 1024);
	datasock->SetSendBuf(64 * 1024);
	// l_line<<newln<<"GameServer= ["<<datasock->GetPeerIP()<<"] À´ÁË,SocketÊýÄ¿= "<<GetSockTotal()+1;
	LG("GateServer", "GameServer= [%s] come,Socket num= %d\n", datasock->GetPeerIP(), GetSockTotal() + 1);
	return true;
}

void ToGameServer::OnDisconnect(DataSocket* datasock, int reason) // reasonÖµ:0-±¾µØ³ÌÐòÕý³£ÍË³ö£»-3-ÍøÂç±»¶Ô·½¹Ø±Õ£»-1-Socket´íÎó;-5-°ü³¤¶È³¬¹ýÏÞÖÆ¡£
{
	LG("GateServer", "GameServer= [%s] gone,Socket num= %d,reason= %s\n", datasock->GetPeerIP(), GetSockTotal() + 1, GetDisconnectErrText(reason).c_str());

	if (reason == DS_SHUTDOWN || reason == DS_DISCONN) {
		return;
	}

	auto l_game = static_cast<GameServer*>(datasock->GetPointer());
	if (!l_game) {
		return;
	}

	bool already_delete = false;
	// ´ÓÁ´±íÖÐÉ¾³ý´Ë GameServer
	_mut_game.lock();
	try {
		// NOTE(Ogge): Is it checking twice because of multithreading?
		//  Once before locking, then as safety after locking?
		l_game = static_cast<GameServer*>(datasock->GetPointer());
		if (l_game) {
			LG("GateServer", " delete [%s]\n", l_game->gamename.c_str());
			_del_game(l_game);
			for (int i = 0; i < l_game->mapcnt; ++i) {
				LG("GateServer", "delete map [%s]\n", l_game->maplist[i].c_str());
				_map_game.erase(l_game->maplist[i]);
			}
			l_game->mapcnt = 0;
			l_game->Free();
			datasock->SetPointer(nullptr);

			portalcache.Clear(reinterpret_cast<uintptr_t>(l_game));
		} else {
			already_delete = true;
		}
	} catch (...) {
		// l_line<<newln<<"Exception raised from OnDisconnect{´ÓÁ´±íÖÐÉ¾³ý´Ë GameServer}"<<endln;
		LG("GateServer", "Exception raised from OnDisconnect{delete GameServer from list}\n");
	}
	_mut_game.unlock();

	if (already_delete)
		return;

	// Í¨ÖªÍ¨¹ý´ËGateServerÁ¬ÉÏ´ËGameServerµÄËùÓÐÓÃ»§£ºµØÍ¼·þÎñÆ÷¹ÊÕÏ
	RPacket retpk = g_gtsvr->gp_conn->get_playerlist();
	uShort ply_cnt = retpk.ReverseReadShort(); // ´ËGateServerÉÏËùÓÐÍæ¼Ò¸öÊý

	Player* ply_addr{};
	uLong db_id{};
	auto ply_array = std::make_unique<Player*[]>(ply_cnt);
	uShort l_notcount = 0;
	for (uShort i = 0; i < ply_cnt; ++i) {
		ply_addr = (Player*)MakePointer(retpk.ReadLongLong());
		db_id = (uLong)retpk.ReadLong();
		
		// Validate player pointer before accessing any members
		if (!g_gtsvr->ValidatePlayerPointer(ply_addr)) {
			l_notcount++;
			continue;
		}
		
		if (l_game == ply_addr->game) {
			ply_array[i - l_notcount] = ply_addr;
		} else {
			l_notcount++;
			continue;
		}

		// Verify dbid matches
		if (ply_addr->m_dbid != db_id) {
			// Character already offline
			continue;
		}

		try {
			g_gtsvr->cli_conn->post_mapcrash_msg(ply_addr); // Notify user of map server failure
		} catch (...) {
			continue;
		}

		continue;
	}

	ply_cnt -= l_notcount;
	LG("GateServer", "because GameServer trouble, notice %d user offline\n", ply_cnt);
	for (int i = 0; i < ply_cnt; ++i) {
		try // Á¢¼´¶ÏµôÕâÌõÁ¬½Ó
		{
			LG("GateServer", "because GameServer trouble, disconnect [%s]\n", ply_array[i]->m_datasock->GetPeerIP());
			g_gtsvr->cli_conn->Disconnect(ply_array[i]->m_datasock, 100, -29);
		} catch (...) {
		}
	}
}

WPacket ToGameServer::OnServeCall(DataSocket* datasock, RPacket& in_para) {
	/*
	GameServer* l_game = (GameServer *)(datasock->GetPointer());
	WPacket l_retpk = GetWPacket();
	uShort l_cmd = in_para.ReadCmd();

	return l_retpk;
	*/

	return nullptr;
}

void ToGameServer::OnProcessData(DataSocket* datasock, RPacket& recvbuf) {
	GameServer* l_game = (GameServer*)(datasock->GetPointer());

	uShort l_cmd = recvbuf.ReadCmd();
	// LG("ToGameServer", "-->l_cmd = %d\n", l_cmd);

	// printf("Incoming from GameServer Packet CMD ID: %d\n", l_cmd);
	// printf("Packet data size: %d bytes\n", recvbuf.GetDataLen());
	// printf("Packet total size: %d bytes\n", recvbuf.GetPktLen());

	try {
		switch (l_cmd) {
		case CMD_MP_GM1SAY: {
			g_gtsvr->gp_conn->SendData(g_gtsvr->gp_conn->get_datasock(), WPacket(recvbuf));
			break;
		}
		case CMD_MT_LOGIN:
			MT_LOGIN(datasock, recvbuf);
			break;
		case CMD_MT_SWITCHMAP: {
			MT_SWITCHMAP(datasock, recvbuf);
			break;
		}
		case CMD_MC_ENTERMAP: {
			MC_ENTERMAP(datasock, recvbuf);
			break;
		}
		case CMD_MT_KICKUSER: {
			MT_KICKUSER(datasock, recvbuf);
			break;
		}
		case CMD_MT_MAPENTRY: {
			MT_MAPENTRY(datasock, recvbuf);
		} break;
		default:
		{
			if (l_cmd / 500 == CMD_MC_BASE / 500) {
				RPacket l_rpk = recvbuf;
				uShort l_aimnum = l_rpk.ReverseReadShort();
				// Discard trailer: count (2) + per player (dbid=4 + gateaddr=8)
				recvbuf.DiscardLast((sizeof(uLong) + sizeof(LLong)) * l_aimnum + sizeof(uShort));
				for (uShort i = 0; i < l_aimnum; i++) {
					Player* l_ply = (Player*)MakePointer(l_rpk.ReverseReadLongLong());
					uLong l_dbid = l_rpk.ReverseReadLong();
					// Validate player is in registry AND dbid matches
					if (!g_gtsvr->ValidatePlayerPointer(l_ply) || l_ply->m_dbid != l_dbid) {
						continue;
					}
					g_gtsvr->cli_conn->SendData(l_ply->m_datasock, recvbuf);
				}
			} else if (l_cmd / 500 == CMD_MP_BASE / 500) {
				RPacket l_rpk = recvbuf;
				uShort l_aimnum = l_rpk.ReverseReadShort();
				// Discard trailer: count (2) + per player (dbid=4 + gateaddr=8)
				recvbuf.DiscardLast((sizeof(uLong) + sizeof(LLong)) * l_aimnum + sizeof(uShort));
				if (l_aimnum > 0) {
					WPacket l_wpk, l_wpk0 = WPacket(recvbuf).Duplicate();
					for (uShort i = 0; i < l_aimnum; i++) {
						Player* l_ply = (Player*)MakePointer(l_rpk.ReverseReadLongLong());
						uLong l_dbid = l_rpk.ReverseReadLong();
						// Validate player is in registry AND dbid matches
						if (!g_gtsvr->ValidatePlayerPointer(l_ply) || l_ply->m_dbid != l_dbid) {
							continue;
						}
						l_wpk = l_wpk0;
						l_wpk.WriteLongLong(MakeULong(l_ply));
						l_wpk.WriteLongLong(l_ply->gp_addr);
						g_gtsvr->gp_conn->SendData(g_gtsvr->gp_conn->get_datasock(), l_wpk);
					}
				} else {
					WPacket l_wpk = WPacket(recvbuf).Duplicate();
					g_gtsvr->gp_conn->SendData(g_gtsvr->gp_conn->get_datasock(), l_wpk);
				}
			} else if (l_cmd / 500 == CMD_MM_BASE / 500) {
				for (GameServer* l_game = _game_list; l_game; l_game = l_game->next) {
					g_gtsvr->gm_conn->SendData(l_game->m_datasock, recvbuf);
				}
			}
			break;
		}
		}
	} catch (...) {
		LG("ToGameServerError", "l_cmd = %d\n", l_cmd);
	}
	// LG("ToGameServer", "<--l_cmd = %d\n", l_cmd);
}

void ToGameServer::MT_LOGIN(DataSocket* datasock, RPacket& rpk) {
	cChar* gms_name = rpk.ReadString();
	cChar* map_list = rpk.ReadString();
	GameServer* gms = _game_heap.Get();
	WPacket retpk = GetWPacket();
	bool valid = true;
	int i;

	retpk.WriteCmd(CMD_TM_LOGIN_ACK);
	int cnt = Util_ResolveTextLine(map_list, gms->maplist, MAX_MAP, ';', 0);
	// l_line<<newln<<"ÊÕµ½GameServer ["<<gms_name<<"] µØÍ¼´®["<<map_list<<"] ¹²["<<cnt<<"]¸ö"<<endln;
	LG("GateServer", "recieve GameServer [%s] map string [%s] total [%d]\n", gms_name, map_list, cnt);
	if (cnt <= 0) { // MAP´®Óï·¨ÓÐ´í
		// l_line<<newln<<"µØÍ¼´® ["<<map_list<<"] ´æÔÚÓï·¨´íÎó£¬ÇëÒÔ';'·Ö¸ô"<<endln;
		LG("GateServer", "map string [%s] has syntax mistake, please use ';'compart\n", map_list);
		retpk.WriteShort(ERR_TM_MAPERR);
		datasock->SetPointer(nullptr);
		gms->Free();
	} else {
		gms->gamename = gms_name;
		gms->mapcnt = cnt;

		_mut_game.lock();
		try {
			do { // Ê×ÏÈ¼ì²é GameServer Ãû×ÖÊÇ·ñÒÑ×¢²á
				if (_exist_game(gms_name)) {
					// l_line<<newln<<"´æÔÚÍ¬ÃûµÄGameServer: "<<gms_name<<endln;
					LG("GateServer", "the same name GameServer exsit: %s\n", gms_name);
					retpk.WriteShort(ERR_TM_OVERNAME);
					datasock->SetPointer(nullptr);
					valid = false;
					break;
				}

				// Æä´Î¼ì²éµØÍ¼ÃûÊÇ·ñ»áÓÐÖØ¸´µÄ
				for (i = 0; i < cnt; ++i) {
					if (find(gms->maplist[i].c_str()) != nullptr) {
						// l_line<<newln<<"´æÔÚÍ¬ÃûµÄMAP: "<<gms->maplist[i].c_str()<<endln;
						LG("GateServer", "the same name MAP exsit: %s\n", gms->maplist[i].c_str());
						retpk.WriteShort(ERR_TM_OVERMAP);
						datasock->SetPointer(nullptr);
						valid = false;
						break;
					}
				}
			} while (false);

			if (valid) {		// ºÏ·¨µÄ GameServer£¬ ¼ÓÈëµ½±íÖÐ
				_add_game(gms); // Ìí¼Óµ½Á´±íÖÐ
				// l_line<<newln<<"Ìí¼ÓGameServer ["<<gms_name<<"] ³É¹¦"<<endln;
				LG("GateServer", "add GameServer [%s] ok\n", gms_name);
				for (i = 0; i < cnt; ++i) // Ìí¼Óµ½ map ÖÐ
				{
					// l_line<<newln<<"Ìí¼Ó ["<<gms_name<<"] ÉÏµÄ ["<<gms->maplist[i].c_str()<<"] µØÍ¼³É¹¦"<<endln;
					LG("GateServer", "add [%s]  [%s] map ok\n", gms_name, gms->maplist[i].c_str());
					_map_game[gms->maplist[i]] = gms;
				}

				datasock->SetPointer(gms);
				gms->m_datasock = datasock;
				retpk.WriteShort(ERR_SUCCESS);
				retpk.WriteString(g_gtsvr->gp_conn->_myself.c_str());

				{ // Portal times
					const auto maps_with_potal = rpk.ReverseReadShort();
					std::vector<Portal> portals;
					for (auto i = 0; i < maps_with_potal; ++i)
					{
						auto& portal = portals.emplace_back();
						portal.map_id = rpk.ReadChar();
						portal.entryFirst = rpk.ReadLongLong();
						portal.entryInterval = rpk.ReadLongLong();
						portal.entryClose = rpk.ReadLongLong();
						portal.destinationClose = rpk.ReadLongLong();
					}
					portalcache.Clear(reinterpret_cast<uintptr_t>(gms));
					if (!portals.empty())
					{
						portalcache.Add(reinterpret_cast<uintptr_t>(gms), std::move(portals));
					}
				}
			} else { // ·Ç·¨µÄ GateServer
				gms->Free();
			}
		} catch (...) {
			// l_line<<newln<<"Exception raised from MT_LOGIN{Ìí¼ÓµØÍ¼}"<<endln;
			LG("GateServer", "Exception raised from MT_LOGIN{add map}\n");
		}
		_mut_game.unlock();

		// if (!valid) Disconnect(datasock);
	}

	SendData(datasock, retpk);
}

void ToGameServer::MT_SWITCHMAP(DataSocket* datasock, RPacket& recvbuf) {
	RPacket l_rpk = recvbuf;
	uShort l_aimnum = l_rpk.ReverseReadShort(); // l_aimnum永远等于1

	Player* l_ply = (Player*)MakePointer(l_rpk.ReverseReadLongLong());
	uLong l_dbid = l_rpk.ReverseReadLong();
	
	// Validate player pointer and dbid
	if (!g_gtsvr->ValidatePlayerPointer(l_ply) || l_ply->m_dbid != l_dbid) {
		LG("GateServer", "MT_SWITCHMAP: Invalid player pointer %p or dbid mismatch\n", l_ply);
		return;
	}
	
	uChar l_return = l_rpk.ReverseReadChar();
	// Trailer: count(2) + per player (dbid=4 + gateaddr=8) + chSwitchType(1)
	recvbuf.DiscardLast(sizeof(uShort) + (sizeof(uLong) + sizeof(LLong)) * l_aimnum + sizeof(uChar));

	cChar* l_srcmap = l_rpk.ReadString();
	Long lSrcMapCopyNO = l_rpk.ReadLong();
	//...
	uLong l_srcx = l_rpk.ReadLong(); // 坐标
	uLong l_srcy = l_rpk.ReadLong(); // 坐标

	cChar* l_map = l_rpk.ReadString();
	Long lMapCopyNO = l_rpk.ReadLong();
	//...
	uLong l_x = l_rpk.ReadLong(); // 坐标
	uLong l_y = l_rpk.ReadLong(); // 坐标
	GameServer* l_game = g_gtsvr->gm_conn->find(l_map);

	if (l_game) {
		l_ply->game->m_plynum--;
		l_ply->game = 0;
		l_ply->gm_addr = 0;
		// l_line<<newln<<"客户端: "<<l_ply->m_datasock->GetPeerIP()<<":"<<l_ply->m_datasock->GetPeerPort()
		//<<"	Switch到地图,Gate向["<<l_game->m_datasock->GetPeerIP()<<"]发送了EnterMap命令,dbid:"<<l_ply->m_dbid
		//<<uppercase<<hex<<",附带Gate地址:"<<MakeULong(l_ply)<<dec<<nouppercase<<endln;
		LG("GateServer", "clinet: %s:%d\tSwitch to map,to Gate[%s]send EnterMap command,dbid:%u,Gate address:%llX\n",
			l_ply->m_datasock->GetPeerIP(), l_ply->m_datasock->GetPeerPort(),
			l_game->m_datasock->GetPeerIP(), l_ply->m_dbid, MakeULong(l_ply));
		l_game->EnterMap(l_ply, l_ply->m_loginID, l_ply->m_dbid, l_ply->m_worldid, l_map, lMapCopyNO, l_x, l_y, 1, l_ply->m_sGarnerWiner); // 根据地图查找GameServer，然后请求GameServer以进入这个地图。
		l_game->m_plynum++;
	} else if (!l_return) // 目标地图不可达，重新进入源地图
	{
		WPacket l_wpk = datasock->GetWPacket();
		l_wpk.WriteCmd(CMD_MC_SYSINFO);
		// l_wpk.WriteString(dstring("[")<<l_map<<"]当前不可到达，请稍后再试！");
		l_wpk.WriteString((std::string("[") + l_map + std::string("] can't reach, pealse retry later!")).c_str());
		l_ply->m_datasock->SendData(l_wpk);

		// l_line<<newln<<"客户端: "<<l_ply->m_datasock->GetPeerIP()<<":"<<l_ply->m_datasock->GetPeerPort()
		//<<"	Switch回地图,Gate向["<<l_ply->game->m_datasock->GetPeerIP()<<"]发送了EnterMap命令,dbid:"<<l_ply->m_dbid
		//<<uppercase<<hex<<",附带Gate地址:"<<MakeULong(l_ply)<<dec<<nouppercase<<endln;
		LG("GateServer", "clinet: %s:%d\tSwitch back map,to Gate[%s]send EnterMap command,dbid:%u,Gate address:%llX\n",
			l_ply->m_datasock->GetPeerIP(), l_ply->m_datasock->GetPeerPort(),
			l_ply->game->m_datasock->GetPeerIP(), l_ply->m_dbid, MakeULong(l_ply));
		l_ply->game->EnterMap(l_ply, l_ply->m_loginID, l_ply->m_dbid, l_ply->m_worldid, l_srcmap, lSrcMapCopyNO, l_srcx, l_srcy, 1, l_ply->m_sGarnerWiner); // 根据地图查找GameServer，然后请求GameServer以进入这个地图。
	} else {
		g_gtsvr->cli_conn->Disconnect(l_ply->m_datasock, 0, -24);
	}
}

void ToGameServer::MT_MAPENTRY(dbc::DataSocket* datasock, dbc::RPacket& recvbuf) {
	WPacket l_wpk0 = WPacket(recvbuf).Duplicate();
	WPacket l_wpk = l_wpk0;

	RPacket l_rpk = recvbuf;
	cChar* l_map = l_rpk.ReadString();
	GameServer* l_game = g_gtsvr->gm_conn->find(l_map);
	if (l_game) {
		l_wpk.WriteCmd(CMD_TM_MAPENTRY);
		g_gtsvr->gm_conn->SendData(l_game->m_datasock, l_wpk);
	} else {
		l_wpk.WriteCmd(CMD_TM_MAPENTRY_NOMAP);
		g_gtsvr->gm_conn->SendData(datasock, l_wpk);
	}
}

void ToGameServer::MC_ENTERMAP(dbc::DataSocket* datasock, dbc::RPacket& recvbuf) {
	RPacket l_rpk = recvbuf;
	GameServer* l_game = (GameServer*)(datasock->GetPointer());

	// Read the target number
	uShort l_aimnum = l_rpk.ReverseReadShort(); // l_aimnum will always be 1

	// Read the player pointer and DBID
	Player* l_ply = (Player*)MakePointer(l_rpk.ReverseReadLongLong());
	uLong l_dbid = l_rpk.ReverseReadLong();

	// Validate player pointer and DBID
	if (!g_gtsvr->ValidatePlayerPointer(l_ply) || l_ply->m_dbid != l_dbid) {
		LG("GateServer", "MC_ENTERMAP: Invalid player %p or DBID mismatch (local=%u, remote=%u)\n",
			l_ply, l_ply ? l_ply->m_dbid : 0, l_dbid);
		return;
	}

	// Read the return code
	uShort l_retcode = l_rpk.ReadShort();

	if (l_retcode == ERR_SUCCESS) {
		l_ply->game = l_game;
		l_ply->gm_addr = l_rpk.ReverseReadLongLong();
		l_game->m_plynum = l_rpk.ReverseReadLong();
		char l_isSwitch = l_rpk.ReverseReadChar();

		LG("GateServer", "Client: %s:%d receive Gate from [%s] success EnterMap command, Game address:%llX, Gate address:%llX\n",
			l_ply->m_datasock->GetPeerIP(), l_ply->m_datasock->GetPeerPort(),
			datasock->GetPeerIP(), l_ply->gm_addr, MakeULong(l_ply));

		// Discard gate-only trailer before forwarding to client:
		// Per player: dbid (uLong=4) + gateaddr (LLong=8) = 12 bytes each
		// Fixed: count (uShort=2) + gm_addr (LLong=8) + playercount (uLong=4) + chLogin (uChar=1) = 15 bytes
		recvbuf.DiscardLast(sizeof(uShort) + (sizeof(uLong) + sizeof(LLong)) * l_aimnum + sizeof(LLong) + sizeof(uLong) + sizeof(uChar));
		g_gtsvr->cli_conn->SendData(l_ply->m_datasock, recvbuf);

		{ // Send portal times to client
			auto wpk = g_gtsvr->cli_conn->GetWPacket();
			wpk.WriteCmd(CMD_TC_PORTALTIMES);
			portalcache.Fetch(wpk);
			g_gtsvr->cli_conn->SendData(l_ply->m_datasock, wpk);
		}
		{
			WPacket l_wpk = GetWPacket();
			l_wpk.WriteCmd(CMD_MP_ENTERMAP);
			l_wpk.WriteChar(l_isSwitch);
			l_wpk.WriteLongLong(MakeULong(l_ply));
			l_wpk.WriteLongLong(l_ply->gp_addr);

			g_gtsvr->gp_conn->SendData(g_gtsvr->gp_conn->get_datasock(), l_wpk);
		}
	} else {
		l_ply->m_status = 1;
		l_game->m_plynum--;

		LG("GateServer", "Client: %s:%d receive from [%s] failed EnterMap command, Error:%d\n",
			l_ply->m_datasock->GetPeerIP(), l_ply->m_datasock->GetPeerPort(),
			datasock->GetPeerIP(), l_retcode);

		// Discard gate-only trailer: count (2) + per player (dbid=4 + gateaddr=8)
		recvbuf.DiscardLast(sizeof(uShort) + (sizeof(uLong) + sizeof(LLong)) * l_aimnum);

		g_gtsvr->cli_conn->Disconnect(l_ply->m_datasock, 10, -33);
	}
}

void ToGameServer::MT_KICKUSER(dbc::DataSocket* datasock, dbc::RPacket& recvbuf) {
	uShort l_aimnum = recvbuf.ReverseReadShort();
	Player* l_ply = (Player*)MakePointer(recvbuf.ReverseReadLongLong());
	uLong l_dbid = recvbuf.ReverseReadLong();
	
	// Validate player pointer and DBID
	if (g_gtsvr->ValidatePlayerPointer(l_ply) && l_ply->m_dbid == l_dbid) {
		g_gtsvr->cli_conn->Disconnect(l_ply->m_datasock, 0, -23);
	} else if (l_ply) {
		LG("GateServer", "MT_KICKUSER: Invalid player %p or dbid mismatch\n", l_ply);
	}
}

GameServer* ToGameServer::find(cChar* mapname) {
	map<string, GameServer*>::iterator it = _map_game.find(mapname);
	if (it == _map_game.end()) {
		// l_line<<newln<<"Î´ÕÒµ½ ["<<mapname<<"] µØÍ¼£¡£¡£¡";
		LG("GateServer", "not found [%s] map!!!\n", mapname);
		return nullptr;
	} else
		return (*it).second;
}

void GameServer::Initially() {
	m_plynum = 0;
	gamename = "";
	ip = "";
	port = 0;
	m_datasock = nullptr;
	next = nullptr;
	mapcnt = 0;
}

void GameServer::Finally() {
	m_plynum = 0;
	gamename = "";
	ip = "";
	port = 0;
	m_datasock = nullptr;
	next = nullptr;
	mapcnt = 0;
}

void GameServer::EnterMap(Player* ply, uLong actid, uLong dbid, uLong worldid, cChar* map, Long lMapCpyNO, uLong x, uLong y, char entertype, short swiner) {
	WPacket l_wpk = m_datasock->GetWPacket();
	l_wpk.WriteCmd(CMD_TM_ENTERMAP);
	l_wpk.WriteLong(actid);
	l_wpk.WriteString(ply->m_password);
	l_wpk.WriteLong(dbid);
	l_wpk.WriteLong(worldid);
	l_wpk.WriteString(map);
	l_wpk.WriteLong(lMapCpyNO);
	l_wpk.WriteLong(x);
	l_wpk.WriteLong(y);
	l_wpk.WriteChar(entertype);
	l_wpk.WriteLongLong(MakeULong(ply)); // Gate address (reverse-read by GameServer)
	l_wpk.WriteShort(swiner);
	g_gtsvr->gm_conn->SendData(m_datasock, l_wpk);
	ply->SetMapName(map); // Chaos Blind
}
