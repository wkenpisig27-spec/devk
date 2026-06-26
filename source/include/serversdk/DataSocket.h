//================================================================
// It must be permitted by Dabo.Zhang that this program is used for
// any purpose in any situation.
// Copyright (C) Dabo.Zhang 2000-2003
// All rights reserved by ZhangDabo.
// This program is written(created) by Zhang.Dabo in 2000.3
// This program is modified recently by Zhang.Dabo in 2003.7
//=================================================================
#ifndef DATASOCKET_H
#define DATASOCKET_H

#include "platform_compat.h"

#include <atomic>

#include "DBCCommon.h"
#include "Packet.h"
#include "PreAlloc.h"
#include "RunBiDirectChain.h"

_DBC_BEGIN

class Receiver;
class Sender;

#pragma pack(push)
#pragma pack(4)
//==============================DataSocket===================================
//
class RPCInfo;
class DataSocket : public PreAllocStru, public RunBiDirectItem<DataSocket> {
	friend class TcpCommApp;
	friend class TcpClientApp;
	friend class TcpServerApp;
	friend class RPCMGR;
	friend class Sender;
	friend class Receiver;
	friend class OnProcessData;

public:
	uLong GetRecvBuf() const { return m_recvbuf.load(std::memory_order_relaxed); }
	uLong GetSendBuf() const { return m_sendbuf.load(std::memory_order_relaxed); }
	TcpCommApp* GetTcpApp() const { return __tca; }
	cChar* GetLocalIP() const { return m_localip; }
	uShort GetLocalPort() const { return m_localport; }
	cChar* GetPeerIP() const { return m_peerip; }
	uShort GetPeerPort() const { return m_peerport; }
	void SetPeerIP(const char* ip) { strncpy_s(m_peerip, sizeof(m_peerip), ip, _TRUNCATE); }
	void SetPeerPort(uShort port) { m_peerport = port; }
	SOCKET GetSocket() const { return m_socket; }
	int GetDisconnectReason() const { return m_delreason.load(std::memory_order_relaxed); }
	bool IsDisconnectPending() const { return m_delflag.load(std::memory_order_relaxed) != 0; }
	bool IsServer() const { return m_isServer; }

	WPacket GetWPacket();
	int SendData(WPacket sendbuf);
	void* GetPointer() const;
	bool SetPointer(void* appinfo);

	int SetSendBuf(uLong bytes);
	int SetRecvBuf(uLong bytes);

	DataSocket(uLong size);
	void Init(SOCKET socket, cChar* peerip, uShort peerport, TcpCommApp* tca, bool IsServer);
	void Free() { PreAllocStru::Free(); }
	std::atomic<LLong> m_sendbytes{0}, m_recvbytes{0};
	std::atomic<LLong> m_sendpkts{0}, m_recvpkts{0};
	std::atomic<uLong> m_sendbyteps{0}, m_recvbyteps{0};
	std::atomic<uLong> m_sendpktps{0}, m_recvpktps{0};

private:
	virtual ~DataSocket();
	void Initially();
	void Finally();
	RPCInfo* GetRPCInfo() const { return m_rpcinfo.load(std::memory_order_relaxed); }

	InterLockedLong m_sbts, m_rbts;
	InterLockedLong m_spks, m_rpks;

	std::atomic<LONG> m_sendflag{0}, m_recvflag{0}, m_procflag{0}, m_isProcess{0};
	std::atomic<LONG> m_sendtime{0}, m_recvtime{0};
	std::atomic<LONG> m_deltime{0}, m_delremain{0}, m_delflag{0};
	std::atomic<int> m_delreason{0};

	TcpCommApp* __tca;
	Sender& m_sender;
	Receiver& m_receiver;
	std::atomic<uLong> m_sendbuf{0}, m_recvbuf{0};

	bool m_isServer;
	SOCKET m_socket;
	char m_localip[16], m_peerip[16];
	uShort m_localport, m_peerport;

	std::atomic<RPCInfo*> m_rpcinfo{nullptr};
	std::atomic<void*> m_appinfo{nullptr};

public:
	std::atomic<short> m_gsCheck{0};
};

#pragma pack(pop)
_DBC_END

#endif
