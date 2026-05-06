
#include "gateserver.h"
#include "log.h"
#include <condition_variable>

using namespace dbc;
using namespace std;

dbc::cuShort g_version = 103;

int ConnectGroupServer::Process() {
	extern std::mutex global_gate_ready_mutex;
	extern std::condition_variable global_gate_ready_cv;
	extern bool is_global_gate_ready;
	std::unique_lock<std::mutex> lk(global_gate_ready_mutex);
	global_gate_ready_cv.wait(lk, [] { return is_global_gate_ready; });
	lk.unlock();

	T_B
		_tgps->m_calltotal++;

	DataSocket* datasock = nullptr;
	while (!GetExitFlag() && !_tgps->m_atexit) {
		if (_tgps->_connected) {
			if (_tgps->IsSync() && g_gtsvr) {
				RunChainGetArmor<Player> l(g_gtsvr->m_plylst);
				WPacket pk = _tgps->GetWPacket();
				pk.WriteCmd(CMD_TP_SYNC_PLYLST);

				int ply_cnt = g_gtsvr->m_plylst.GetTotal(); // ×ÜÊý
				pk.WriteLong(ply_cnt);
				pk.WriteString(_tgps->_myself.c_str()); // gateserverµÄÃû×Ö
				auto ply_array = std::make_unique<Player*[]>(ply_cnt);

				int i = 0;
				for (auto l_ply = g_gtsvr->m_plylst.GetNextItem(); l_ply; l_ply = g_gtsvr->m_plylst.GetNextItem()) {
					pk.WriteLongLong(reinterpret_cast<uintptr_t>(l_ply)); // 64-bit player address
					pk.WriteLong(l_ply->m_loginID);
					pk.WriteLong(l_ply->m_actid);

					ply_array[i++] = l_ply;
				}

				RPacket retpk = _tgps->SyncCall(_tgps->get_datasock(), pk);
				int err = retpk.ReadShort();

				if (!retpk.HasData() || err == ERR_PT_LOGFAIL) {
					Sleep(5000);
					_tgps->Disconnect(_tgps->get_datasock());
					// Ê§°ÜÁË
				} else {
					int num = retpk.ReadShort();

					if (num != ply_cnt) {
						Sleep(5000);
						_tgps->Disconnect(_tgps->get_datasock());
						// Ê§°ÜÁË
					} else {
						// NOTE(Ogge): What is this? Investigate
						for (int i = 0; i < num; i++) {
							if (retpk.ReadShort() == 1) {
								ply_array[i]->gp_addr = retpk.ReverseReadLongLong();
							}
						}
					}

					_tgps->SetSync(false);
				}
			}

			// ÒÑ¾­Á¬½ÓÉÏ
			Sleep(1000);
		} else {
			// Î´Á¬½Ó»òÁ¬½Ó¶Ïµô£¬ÖØÐÂÁ¬!
			LG("Connect", "%s\n", RES_STRING(GS_TOGROUPSERVER_CPP_00001));
			datasock = _tgps->Connect(_tgps->_gs.ip.c_str(), _tgps->_gs.port);
			if (datasock == nullptr) {
				LG("Connect", "%s\n", RES_STRING(GS_TOGROUPSERVER_CPP_00002));
				Sleep(5000);
				continue;
			} else {
				// µÇÂ¼µ½ GroupServer
				WPacket pk = _tgps->GetWPacket();
				pk.WriteCmd(CMD_TP_LOGIN);
				pk.WriteShort(g_version);
				pk.WriteString(_tgps->_myself.c_str());

				RPacket retpk = _tgps->SyncCall(datasock, pk);
				int err = retpk.ReadShort();
				if (!retpk.HasData() || err == ERR_PT_LOGFAIL) {
					LG("Connect", "%s\n", RES_STRING(GS_TOGROUPSERVER_CPP_00003));
					_tgps->Disconnect(datasock);
					datasock = nullptr;
					Sleep(5000);
				} else {
					LG("Connect", "%s\n", RES_STRING(GS_TOGROUPSERVER_CPP_00004));
					_tgps->_gs.datasock = datasock;
					_tgps->_connected = true;

					_tgps->SetSync();

					datasock = nullptr;
				}
			}
		}
	}

	T_FINAL

	return 0;
}
Task* ConnectGroupServer::Lastly() {
	--(_tgps->m_calltotal);
	return Task::Lastly();
}

ToGroupServer::ToGroupServer(char const* fname, ThreadPool* proc, ThreadPool* comm)
	: TcpClientApp(this, proc, comm), RPCMGR(this), _gs(), _connected(false), m_atexit(0), m_calltotal(0),
	  _myself() {
	IniFile inf(fname);
	IniSection& is = inf["GroupServer"];
	_myself = inf["Main"]["Name"];
	_gs.ip = is["IP"];
	_gs.port = std::stoi(is["Port"]);

	// Æô¶¯ PING Ïß³Ì

	SetPKParse(0, 2, 64 * 1024, 400);
	BeginWork(std::stoi(is["EnablePing"]));

	//++m_calltotal;
	// proc->AddTask(new ConnectGroupServer(this));
}

ToGroupServer::~ToGroupServer() {
	m_atexit = 1;
	while (m_calltotal) {
		Sleep(1);
	}
	ShutDown(12 * 1000);
}

void ToGroupServer::TaskDispatcher(Task* task) {
	extern std::mutex global_gate_ready_mutex;
	extern std::condition_variable global_gate_ready_cv;
	extern bool is_global_gate_ready;
	std::unique_lock<std::mutex> lk(global_gate_ready_mutex);
	global_gate_ready_cv.wait(lk, [] { return is_global_gate_ready; });

	lk.unlock();

	TcpClientApp::TaskDispatcher(task);
}

bool ToGroupServer::OnConnect(DataSocket* datasock) // ·µ»ØÖµ:true-ÔÊÐíÁ¬½Ó,false-²»ÔÊÐíÁ¬½Ó
{
	datasock->SetRecvBuf(64 * 1024);
	datasock->SetSendBuf(64 * 1024);
	// l_line<<newln<<"Á¬½ÓÉÏGroupServer: "<<datasock->GetPeerIP()<<",SocketÊýÄ¿:"<<GetSockTotal()+1;
	LG("Connect", "connect GroupServer: %s,Socket num:%d\n", datasock->GetPeerIP(), GetSockTotal() + 1);
	return true;
}

void ToGroupServer::OnDisconnect(DataSocket* datasock, int reason) // reasonÖµ:0-±¾µØ³ÌÐòÕý³£ÍË³ö£»-3-ÍøÂç±»¶Ô·½¹Ø±Õ£»-1-Socket´íÎó;-5-°ü³¤¶È³¬¹ýÏÞÖÆ¡£
{																   // ¼¤»î ConnnectGroupServer Ïß³Ì
	// l_line<<newln<<"ÓëGroupServerµÄÍøÂçÁ¬½ÓÖÐ¶Ï,SocketÊýÄ¿: "<<GetSockTotal()<<",reason ="<<GetDisconnectErrText(reason).c_str()<<"£¬Á¢¼´ÖØÁ¬..."<<endln;
	LG("Connect", "disconnection with GroupServer,Socket num: %d,reason =%s, reconnecting...\n", GetSockTotal(), GetDisconnectErrText(reason).c_str());

	if (!g_appexit) {
		_connected = false;
	}
}

WPacket ToGroupServer::OnServeCall(DataSocket* datasock, RPacket& in_para) {
	uShort l_cmd = in_para.ReadCmd();
	WPacket retpk = GetWPacket();

	switch (l_cmd) {
	case 0:
	default:
		break;
	}

	return retpk;
}

void ToGroupServer::OnProcessData(DataSocket* datasock, RPacket& recvbuf) {
	uShort l_cmd = recvbuf.ReadCmd();
	LG("ToGroupServer", "OnProcessData-->l_cmd = %d\n", l_cmd);
	try {
		switch (l_cmd) {
		// case CMD_PM_GARNER2_UPDATE:
		//	{
		//		//ºöÂÔCMD_PM_GARNER2_UPDATEÏûÏ¢
		//	}
		//	break;
		case CMD_MM_DO_STRING: {
			for (GameServer* l_game = g_gtsvr->gm_conn->_game_list; l_game; l_game = l_game->next) {
				g_gtsvr->gm_conn->SendData(l_game->m_datasock, recvbuf);
			}
			break;
		}
		case CMD_PM_TEAM: {
			for (GameServer* l_game = g_gtsvr->gm_conn->_game_list; l_game; l_game = l_game->next) {
				g_gtsvr->gm_conn->SendData(l_game->m_datasock, recvbuf);
			}
			break;
		}
		case CMD_AP_KICKUSER:
		case CMD_PT_KICKUSER: {
			uShort l_aimnum = recvbuf.ReverseReadShort();
			Player* l_ply = (Player*)MakePointer(recvbuf.ReverseReadLongLong());
			long long l_gp_addr = recvbuf.ReverseReadLongLong();
			
			// Use player registry validation instead of just gp_addr check
			if (!g_gtsvr->ValidatePlayerPointer(l_ply, l_gp_addr)) {
				if (l_ply) {
					LG("GateServer", "GroupServer kick person REJECTED: invalid player pointer %p or gp_addr mismatch\n", l_ply);
				}
				break;
			}
			
			LG("GateServer", "GroupServer kick person, l_ply->m_dbid=%u, m_status=%d\n", l_ply->m_dbid, l_ply->m_status);

			// If player is in-game (status 2), notify GameServer to remove from map
			GameServer* l_game = l_ply->game;
			if ((l_ply->m_status == 2) && l_ply->gm_addr && l_game && l_game->m_datasock) {
				LG("GateServer", "Kick: Sending CMD_TM_GOOUTMAP to GameServer for dbid=%u\n", l_ply->m_dbid);
				WPacket l_wpk = l_game->m_datasock->GetWPacket();
				l_wpk.WriteCmd(CMD_TM_GOOUTMAP);
				l_wpk.WriteChar(0);
				l_wpk.WriteLongLong(MakeULong(l_ply));
				l_wpk.WriteLongLong(l_ply->gm_addr);
				g_gtsvr->gm_conn->SendData(l_game->m_datasock, l_wpk);

				l_ply->game = 0;
				l_ply->gm_addr = 0;
			}

			// Send disconnect notification to GroupServer
			{
				WPacket l_wpk = GetWPacket();
				l_wpk.WriteCmd(CMD_TP_DISC);
				l_wpk.WriteLong(l_ply->m_actid);
				l_wpk.WriteLong(l_ply->m_datasock ? inet_addr(l_ply->m_datasock->GetPeerIP()) : 0);
				l_wpk.WriteString("Kicked due to duplicate login");
				g_gtsvr->gp_conn->SendData(datasock, l_wpk);
			}

			// Send logout notification to GroupServer
			{
				WPacket l_wpk = GetWPacket();
				l_wpk.WriteCmd(CMD_TP_USER_LOGOUT);
				l_wpk.WriteLongLong(MakeULong(l_ply));
				l_wpk.WriteLongLong(l_ply->gp_addr);
				l_ply->gp_addr = 0;
				g_gtsvr->gp_conn->SendData(datasock, l_wpk);
			}

			// Disconnect the client
			g_gtsvr->cli_conn->Disconnect(l_ply->m_datasock, 0, -21);
			break;
		}
		case CMD_PT_DEL_ESTOPUSER: {
			uShort l_aimnum = recvbuf.ReverseReadShort();
			Player* l_ply = (Player*)MakePointer(recvbuf.ReverseReadLongLong());
			long long l_gp_addr = recvbuf.ReverseReadLongLong();
			if (g_gtsvr->ValidatePlayerPointer(l_ply, l_gp_addr)) {
				LG("GateServer", "GroupServer del estop user,operator success,l_ply->m_dbid =%u\n", l_ply->m_dbid);
				l_ply->m_estop = false;
			} else if (l_ply) {
				LG("GateServer", "GroupServer del estop user REJECTED: invalid player %p\n", l_ply);
			}
		} break;
		case CMD_PT_ESTOPUSER: {
			// printf( "CMD_PT_ESTOPUSER" );
			uShort l_aimnum = recvbuf.ReverseReadShort();
			Player* l_ply = (Player*)MakePointer(recvbuf.ReverseReadLongLong());
			long long l_gp_addr = recvbuf.ReverseReadLongLong();
			if (g_gtsvr->ValidatePlayerPointer(l_ply, l_gp_addr)) {
				LG("GateServer", "GroupServer set estop user,operator success,l_ply->m_dbid =%u\n", l_ply->m_dbid);
				l_ply->m_estop = true;
			} else if (l_ply) {
				LG("GateServer", "GroupServer set estop user REJECTED: invalid player %p\n", l_ply);
			}
		} break;
		case CMD_MC_SYSINFO:
			l_cmd = CMD_PC_BASE;
		default: // È±Ê¡×ª·¢
		{
			if (l_cmd / 500 == CMD_PC_BASE / 500) {
				RPacket l_rpk = recvbuf;
				uShort l_aimnum = l_rpk.ReverseReadShort();
				// GroupServer sends: per player (gtAddr=LLong + gp_addr=LLong) + count(uShort)
				recvbuf.DiscardLast((sizeof(LLong) + sizeof(LLong)) * l_aimnum + sizeof(uShort));
				Player* l_ply{};
				for (uShort i = 0; i < l_aimnum; i++) {
					l_ply = (Player*)MakePointer(l_rpk.ReverseReadLongLong());
					long long l_gp_addr = l_rpk.ReverseReadLongLong();
					if (!g_gtsvr->ValidatePlayerPointer(l_ply, l_gp_addr)) {
						l_ply = nullptr;
						continue;
					}
					g_gtsvr->cli_conn->SendData(l_ply->m_datasock, recvbuf);
				}
				if (l_cmd == CMD_PC_CHANGE_PERSONINFO && l_ply) {
					WPacket l_wpk = recvbuf;
					l_wpk.WriteCmd(CMD_TM_CHANGE_PERSONINFO);
					l_wpk.WriteLongLong(MakeULong(l_ply));
					l_wpk.WriteLongLong(l_ply->gm_addr); // 附加上在GameServer上的内存地址
					g_gtsvr->gm_conn->SendData(l_ply->game->m_datasock, l_wpk);
					break;
				}
				if (l_cmd == CMD_PC_PING && l_ply) {
					l_ply->m_pingtime = GetTickCount();
					break;
				}
			} else if (l_cmd / 500 == CMD_PM_BASE / 500) {
				RPacket l_rpk = recvbuf;
				uShort l_aimnum = l_rpk.ReverseReadShort();
				// GroupServer sends: per player (gtAddr=LLong + gp_addr=LLong) + count(uShort)
				recvbuf.DiscardLast((sizeof(LLong) + sizeof(LLong)) * l_aimnum + sizeof(uShort));
				if (!l_aimnum) {
					WPacket l_wpk = WPacket(recvbuf).Duplicate();
					l_wpk.WriteLong(0);
					for (GameServer* l_game = g_gtsvr->gm_conn->_game_list; l_game; l_game = l_game->next) {
						g_gtsvr->gm_conn->SendData(l_game->m_datasock, l_wpk);
					}
				} else {
					WPacket l_wpk, l_wpk0 = WPacket(recvbuf).Duplicate();
					for (uShort i = 0; i < l_aimnum; i++) {
						Player* l_ply = (Player*)MakePointer(l_rpk.ReverseReadLongLong());
						long long l_gp_addr = l_rpk.ReverseReadLongLong();
						
						if (!g_gtsvr->ValidatePlayerPointer(l_ply, l_gp_addr) || !l_ply->game) {
							continue;
						}

						l_wpk = l_wpk0;
						l_wpk.WriteLongLong(MakeULong(l_ply));
						l_wpk.WriteLongLong(l_ply->gm_addr);
						g_gtsvr->gm_conn->SendData(l_ply->game->m_datasock, l_wpk);
					}
				}
			}
			break;
		}
		}
	} catch (...) {
		LG("ToGroupServerError", "l_cmd = %d\n", l_cmd);
	}
	// LG("ToGroupServer", "<--l_cmd = %d\n", l_cmd);
}

// ´Ó GroupServer ÉÏµÃµ½ËùÓÐÓÃ»§ÁÐ±í
RPacket ToGroupServer::get_playerlist() {
	WPacket pk = GetWPacket();
	pk.WriteCmd(CMD_TP_REQPLYLST);

	return SyncCall(_gs.datasock, pk);
}
