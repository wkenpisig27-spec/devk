#include "stdafx.h"

#ifdef USE_IOCP

#include "gmsvr.h"
#include "gtplayer.h"
#include "gameapp.h"
#ifdef _MSC_VER
#pragma warning(disable : 4311)
#endif

myiocpclt::myiocpclt()
	: cfl_iocpclt(), _mqlock(), rpktpool(1000), wpktpool(1000), msgpool(1000), gtnum(0) {
	pkthdr_size = Packet::hdr_sz();
	_msgqueue.clear();
}

bool myiocpclt::init_gtconn(CGameConfig* gmcfg) {
	gmsvr_name = gmcfg->m_szName;

	int num = min(MAX_GATE, gmcfg->m_nGateCnt);

	for (int i = 0; i < num; ++i) {
		gtarray[i].GetIP() = gmcfg->m_szGateIP[i];
		gtarray[i].GetPort() = gmcfg->m_nGatePort[i];
		gtarray[i].Invalid();
	}

	gtnum = num;

	DWORD dwThreadId;
	CreateThread(nullptr, 0, conn_thrd, this, 0, &dwThreadId);
	return true;
}

DWORD WINAPI myiocpclt::conn_thrd(LPVOID conn_thrd_ctx) {
	myiocpclt* that = (myiocpclt*)conn_thrd_ctx;
	PER_SOCKET_CONTEXT* sk_ctx;
	int i;

	while (!that->get_exit_flag()) {
		for (i = 0; i < that->gtnum; ++i) {
			if (that->gtarray[i].IsValid()) {
			} else {
				sk_ctx = that->connect(that->gtarray[i].GetIP().c_str(), that->gtarray[i].GetPort());
				if (sk_ctx == nullptr) {
					// LG("iocp_conn", "连接 Gate %s:%d 失败！\n", that->gtarray[i].GetIP().c_str(),
					//   that->gtarray[i].GetPort());
					LG("iocp_conn", "connect Gate %s:%d failed！\n", that->gtarray[i].GetIP().c_str(),
					   that->gtarray[i].GetPort());
				} else {
					that->gtarray[i].SetDataSock(sk_ctx, that);
					that->_postconnmsg(sk_ctx); // notify walker thread to update the socket
				}
			}
		}

		Sleep(1000);
	}

	LG("iocp_conn", "conn_thrd thread exit\n");
	return 0;
}

void myiocpclt::postmsg(DWORD msg_type, GateServer* gt, Packet* pkt) {
	if (gt == nullptr)
		return;

	mymsg* msg = msgpool.get();
	if (msg == nullptr) {
		cfl_printf("message pool is empty! drop a packet\n");
		pkt->free();
		return;
	}

	msg->type = msg_type;
	msg->gt = gt;
	msg->pkt = pkt;

	_mqlock.lock();
	try {
		_msgqueue.push_back(msg);
	} catch (...) {
		cfl_printf("Exception raised from postmsg\n");
	}
	_mqlock.unlock();
}

int myiocpclt::on_connect(PER_SOCKET_CONTEXT* sk_ctx) {
	GateServer* gt = (GateServer*)sk_ctx->getusrdat();
	if (gt == nullptr) {
		cfl_printf("gtsvr pool is empty!\n");
		return 0;
	}

	// Clear the player list
	gt->GetPlayerList() = nullptr;

	// Post the connect message to app
	postmsg(NETMSG_GATE_CONNECTED, gt, nullptr);

	// Post the first asynchronized read quest on this socket
	_recv(sk_ctx);

	return 0;
}

int myiocpclt::on_disconn(PER_SOCKET_CONTEXT* sk_ctx) {
	GateServer* gt = (GateServer*)sk_ctx->getusrdat();

	// Post the disconnect message to app
	Packet* pkt = (Packet*)rpktpool.get();
	if (pkt != nullptr) {
		pkt->WriteLongLong(MakeULong(gt->GetPlayerList())); // x64: use 64-bit for pointer
		pkt->ResetOffset();
	}
	postmsg(NETMSG_GATE_DISCONNECT, gt, pkt);

	// Reset the user data of socket
	sk_ctx->setusrdat(nullptr);
	gt->Invalid();

	return 0;
}

int myiocpclt::on_recvdat(PER_SOCKET_CONTEXT* sk_ctx, DWORD size) {
	// cfl_printf("Receive %dB from %s:%d\n", size, sk_ctx->remote_ip.c_str(), sk_ctx->remote_port);

	char* buf = sk_ctx->io_ctx->buffer;
	int& sent_bytes = sk_ctx->io_ctx->sent_bytes; // data last received
	short len = (short)size;
	short remain_len = 0;
	short pkt_size;
	Packet* pkt;

	len += sent_bytes;
	if (len == 0)
		return 0;

	do {

		if (len < pkthdr_size)
			break;

		// receive a packet header
		pkt_size = Packet::pkt_len(buf);
		if (len < pkt_size) {
			remain_len = pkt_size - len;
			break;
		}

		// cfl_printf("packet size = %d\n", pkt_size);

		// receive a packet
		pkt = (Packet*)rpktpool.get();
		if (pkt != nullptr) {
			memcpy(pkt->pkt_buf(), buf, pkt_size);

			// post it up
			postmsg(NETMSG_PACKET, (GateServer*)sk_ctx->getusrdat(), pkt);
		}

		// find whether there is another packet or not
		buf += pkt_size;
		len -= pkt_size;

	} while (true);

	if (len > 0) {
		// cfl_printf("Remain length = %d, enter recv state again!\n", remain_len);

		// need to move data to the start position
		//  will be optimized in the future
		char* buf2 = sk_ctx->io_ctx->buffer;
		char tmpbuf[MAX_BUFF_SIZE];
		memcpy(tmpbuf, buf, len);
		memcpy(buf2, tmpbuf, len);

		// receive the remain data
		sent_bytes = len;
		_recv(sk_ctx, len);
	} else {
		// cfl_printf("Enter recv state normally!\n");
		sent_bytes = 0;
		_recv(sk_ctx);
	}

	return 0;
}

int myiocpclt::on_sendfsh(PER_SOCKET_CONTEXT* sk_ctx, DWORD size) {
	// cfl_printf("Send %dB to %s:%d\n", size, sk_ctx->remote_ip.c_str(), sk_ctx->remote_port);

	return 0;
}

mymsg* myiocpclt::recvmsg() {
	mymsg* msg;

	_mqlock.lock();
	try {
		msg = (_msgqueue.empty()) ? nullptr : _msgqueue.front();
		if (msg != nullptr)
			_msgqueue.pop_front();
	} catch (...) {
		LG("gmsvr_error", "Exception in recvmsg()\n");
	}
	_mqlock.unlock();

	return msg;
}

void myiocpclt::peekpkt(int ms /* = 0 */) {
	mymsg* msg;
	int cnt = 0;

	while (true) {
		// peek a packet
		msg = recvmsg();
		if (msg == nullptr)
			break;

		// dispatch packet
		::g_pGameApp->ProcessNetMsg(msg->type, msg->gt, msg->pkt);
		if (msg->pkt != nullptr)
			msg->pkt->free();
		msg->free();

		if (cnt >= ms) {
			Sleep(0);
			++cnt;
		}
	}
}

bool myiocpclt::addplayer(GatePlayer* gtplayer, GateServer* gt, uintptr_t gtaddr) {
	if (gt == nullptr || gtplayer == nullptr)
		return false;
	if (!gt->IsValid())
		return false;

	// 赋予 GatePlayer 某些字段值
	gtplayer->SetGate(gt);
	gtplayer->SetGateAddr(gtaddr);

	// 将 gtplayer 插入到头部
	gtplayer->Prev = nullptr;
	gtplayer->Next = gt->GetPlayerList();

	if (gtplayer->Next != nullptr)
		gtplayer->Next->Prev = gtplayer;

	// 更新头部
	gt->GetPlayerList() = gtplayer;

	return true;
}

bool myiocpclt::delplayer(GatePlayer* gtplayer) {
	if (gtplayer == nullptr)
		return false;

	GateServer* gt = gtplayer->GetGate();
	if (gt == nullptr || !gt->IsValid())
		return false;

	if (gt->m_listcurplayer == gtplayer)
		gt->m_listcurplayer = gtplayer->Next;
	// 从链表中剔除
	GatePlayer*& playerlist = gt->GetPlayerList();
	if ((gtplayer->Prev == nullptr) && (gtplayer->Next == nullptr)) {
		if (gtplayer == playerlist) { // 只有一个，清空
			playerlist = nullptr;
			return true;
		} else { // 非法的gtplayer
			return false;
		}
	} else if ((gtplayer->Prev == nullptr) && (gtplayer->Next != nullptr)) { // 头部
		playerlist = gtplayer->Next;
		playerlist->Prev = nullptr;

		gtplayer->Next = nullptr;
		return true;
	} else if ((gtplayer->Prev != nullptr) && (gtplayer->Next == nullptr)) { // 尾部
		gtplayer->Prev->Next = nullptr;

		gtplayer->Prev = nullptr;
		return true;
	} else { // 中间
		gtplayer->Prev->Next = gtplayer->Next;
		gtplayer->Next->Prev = gtplayer->Prev;

		gtplayer->Prev = nullptr;
		gtplayer->Next = nullptr;
		return true;
	}
}

bool myiocpclt::kickplayer(GatePlayer* gtplayer, int lTimeSec) {
	Packet* pkt = (Packet*)wpktpool.get();
	pkt->WriteCmd(CMD_MT_KICKUSER);
	pkt->WriteLong(lTimeSec);
	return sendtoclient(pkt, gtplayer);
}

bool myiocpclt::sendtoworld(Packet* pkt) {
	if (pkt == nullptr)
		return false;

	Packet* tmp;
	short cnt;

	GatePlayer* pPlayer;
	GateServer* pGate;

	// 准备为每一个连接的 Gate 产生通知包
	for (int i = 0; i < gtnum; ++i) {
		pGate = &gtarray[i];

		if (pGate->IsValid()) {
			cnt = 0;
			tmp = (Packet*)wpktpool.get();
			if (tmp == nullptr) {
				continue;
			}

			tmp->clone_from(pkt);

			pPlayer = pGate->GetPlayerList();
			while (pPlayer != nullptr) {
				tmp->WriteLong(pPlayer->GetDBChaId());
				tmp->WriteLongLong(pPlayer->GetGateAddr());
				cnt++;

				pPlayer = pPlayer->Next;
			}

			if (cnt > 0) {
				tmp->WriteShort(cnt);
				pGate->SendData(tmp);
			}
		}
	}

	pkt->free();
	return true;
}

bool myiocpclt::sendtoclient(Packet* pkt, GatePlayer* playerlist) {
	if (playerlist == nullptr || pkt == nullptr)
		return false;

	// 找出有效滴 Gate
	unsigned short usCount[MAX_GATE];
	Packet* CChginf[MAX_GATE];
	Packet* tmppkt;
	GateServer* pValidGate[MAX_GATE];
	short sValidGateNum = 0;
	for (int i = 0; i < gtnum; ++i) {
		if (gtarray[i].IsValid()) {
			pValidGate[sValidGateNum] = &gtarray[i];
			usCount[sValidGateNum] = 0;

			tmppkt = (Packet*)wpktpool.get();
			if (tmppkt != nullptr)
				tmppkt->clone_from(pkt);
			CChginf[sValidGateNum] = tmppkt;

			sValidGateNum++;
		}
	}

	// 遍历 playerlist ，组织发往各个 Gate 的包
	GatePlayer* tmp = playerlist;
	while (tmp != nullptr) {
		if (tmp->GetGate() == nullptr) {
#ifdef defCOMMU_LOG
			LG("SendToClient", "WARNING! pGate = nullptr, cha_id=%d, gt_addr=0x%x\n",
			   tmp->GetDBChaId(), tmp->GetGateAddr());
#endif
		} else {
			for (i = 0; i < sValidGateNum; ++i) {
				if ((tmp->GetGate() == pValidGate[i]) && (CChginf[i] != nullptr)) {
					usCount[i]++;
					CChginf[i]->WriteLong(tmp->GetDBChaId());
					CChginf[i]->WriteLongLong(tmp->GetGateAddr());
					break;
				}
			}
		}

		tmp = tmp->GetNextPlayer();
	}

	// 　添加最后一个 个数 数据，并发送出去
	for (i = 0; i < sValidGateNum; i++) {
		if (usCount[i] > 0) {
			CChginf[i]->WriteShort(usCount[i]);
			pValidGate[i]->SendData(CChginf[i]);
		}
	}

	pkt->free();
	return true;
}

bool myiocpclt::sendtoclient(Packet* pkt, int array_cnt, uplayer* uplayer_array) {
	if (uplayer_array == nullptr || pkt == nullptr)
		return false;

#ifdef defCOMMU_LOG
		// LG("SendToClient", "\nSendToClient called to notify %d players\n", array_cnt);
#endif

	// 找出有效滴 Gate
	unsigned short usCount[MAX_GATE];
	Packet* CChginf[MAX_GATE];
	Packet* tmppkt;
	GateServer* pValidGate[MAX_GATE];
	short sValidGateNum = 0;
	for (int i = 0; i < gtnum; ++i) {
		if (gtarray[i].IsValid()) {
			pValidGate[sValidGateNum] = &gtarray[i];
#ifdef defCOMMU_LOG
			// LG("SendToClient", "Valid Gate 0x%x\n", pValidGate[sValidGateNum]);
#endif
			usCount[sValidGateNum] = 0;

			tmppkt = (Packet*)wpktpool.get();
			if (tmppkt != nullptr) {
				tmppkt->clone_from(pkt);
			}
			CChginf[sValidGateNum] = tmppkt;

			sValidGateNum++;
		}
	}
#ifdef defCOMMU_LOG
	// LG("SendToClient", "Valid Gate num = %d\n", sValidGateNum);
#endif

	// 遍历 uplayer_array ，组织发往各个 Gate 的包
	int j;
	for (i = 0; i < array_cnt; ++i) {
#ifdef defCOMMU_LOG
		// LG("SendToClient", "cha_id = %d, pGate = 0x%x, gt_addr = 0x%x\n", uplayer_array[i].m_dwDBChaId, uplayer_array[i].pGate, uplayer_array[i].m_ulGateAddr);
#endif

		if (uplayer_array[i].pGate == nullptr) {
#ifdef defCOMMU_LOG
			LG("SendToClient", "WARNING! pGate = nullptr, cha_id=%d, gt_addr=0x%x\n",
			   uplayer_array[i].m_dwDBChaId, uplayer_array[i].m_ulGateAddr);
#endif
			continue;
		}

		for (j = 0; j < sValidGateNum; ++j) {
			if ((uplayer_array[i].pGate == pValidGate[j]) && (CChginf[j] != nullptr)) {
				usCount[j]++;
				CChginf[j]->WriteLong(uplayer_array[i].m_dwDBChaId);
				CChginf[j]->WriteLongLong(uplayer_array[i].m_ulGateAddr);
				break;
			}
		}
	}

	// 　添加最后一个 个数 数据，并发送出去
	for (i = 0; i < sValidGateNum; i++) {
#ifdef defCOMMU_LOG
		// LG("SendToClient", "send to Gate: %s for %d players\n", pValidGate[i]->GetIP().c_str(), usCount[i]);
#endif
		if (usCount[i] > 0) {
			CChginf[i]->WriteShort(usCount[i]);
			pValidGate[i]->SendData(CChginf[i]);
		}
	}

	pkt->free();
	return true;
}

bool myiocpclt::sendtogame(Packet* pkt, uplayer* uplyr) {
	int l = uplyr->pGate->SendData(pkt);
	pkt->free();
	return (l > 0) ? true : false;
}

myiocpclt* g_mygmsvr = nullptr;

#endif
