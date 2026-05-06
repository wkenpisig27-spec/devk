//================================================================
// It must be permitted by Dabo.Zhang that this program is used for
// any purpose in any situation.
// Copyright (C) Dabo.Zhang 2000-2003
// All rights reserved by ZhangDabo.
// This program is written(created) by Zhang.Dabo in 2000.3
// This program is modified recently by Zhang.Dabo in 2003.7
//=================================================================
#ifndef DBCOMM_H
#define DBCOMM_H

#include "platform_compat.h"

#include "DataSocket.h"
#include "Packet.h"

#define DS_SHUTDOWN (0xFFFF)
#define DS_DISCONN (0xFFFE)

_DBC_BEGIN
#pragma pack(push)
#pragma pack(4)

class RPCMGR;
class Task;
class ThreadPool;
struct selparm;
struct BandwidthStat {
	volatile uLong m_tick;
	volatile uLong m_sendbyteps, m_recvbyteps, m_sendpktps, m_recvpktps;
	volatile LLong m_sendbytes, m_recvbytes;
	volatile LLong m_sendpkts, m_recvpkts;
};

//=========================TcpCommApp=================================

extern void SetCommAppDebug(bool bDebug);

/**
 * @class TcpCommApp
 * @author ZhangDabo
 * @brief Tcp��װ����
 * @bug
 */
class TcpCommApp {
	friend class TcpClientApp;
	friend class TcpServerApp;
	friend class RPCMGR;
	friend class AcceptConnect;
	friend class DelConnect;
	friend class OnConnect;
	friend class OnServeCall;

public:
	/**
	 * @brif Socket��ʼ��
	 */
	static int WSAStartup();

	/**
	 * @brif Socket���
	 */
	static void WSACleanup();
	// �ⲿ����

	/**
	 * @brif ����ǰ����
	 * @param[in]  DataSocket  Socket����
	 *
	 * @return bool true-��������,false-����������
	 */

	virtual bool OnConnect(DataSocket* datasock) { return true; };

	/**
	 * @brif ���Ӻ�֪ͨ����
	 * @param[in]  DataSocket  Socket����
	 */
	virtual void OnConnected(DataSocket* datasock){};

	/**
	 * @brif ���Ӻ�֪ͨ����
	 * reasonֵ:0-���س��������˳�;-1-Socket����;-3-���类�Է��ر�;-5-�����ȳ�������;-7-����̫������;-9-KeepAliveʧ��
	 * @param[in]  DataSocket  Socket����
	 * @param[in]  reason  Socket����
	 */
	virtual void OnDisconnect(DataSocket* datasock, int reason){};

	virtual void OnProcessData(DataSocket* datasock, RPacket& recvbuf) = 0;
	virtual void OnSendAll(DataSocket* datasock) {}
	virtual bool OnSendBlock(DataSocket* datasock) { return false; }														   // ����ֵ:true-���������ȴ�,false-�Ͽ��������
	virtual void OnEncrypt(DataSocket* datasock, char* ciphertext, cChar* text, uLong& len) { MemCpy(ciphertext, text, len); } // ����
	virtual void OnDecrypt(DataSocket* datasock, char* ciphertext, uLong& len) {}											   // ����
	virtual void TaskDispatcher(Task*) = 0;

	int SendData(DataSocket* datasock, WPacket sendbuf);
	void Disconnect(DataSocket* datasock, uLong remain = 0, int reason = DS_DISCONN);

	BandwidthStat GetBandwidthStat() const;
	uLong GetSockTotal() const { return __socklist.GetTotal(); }
	ThreadPool* GetProcessor() const { return __processor; }
	ThreadPool* GetCommunicator() const { return __communicator; }
	uLong GetPktHead() const;
	WPacket GetWPacket() const;
	uLong GetCurrentTick() const { return m_TickCount; }
	uLong GetRecvTime(DataSocket* datasock) const {
		if (datasock) {
			return datasock->m_recvtime;
		} else {
			return uLong(int(-1));
		}
	}
	static dstring GetDisconnectErrText(int reason);

	// const��Ա
	cLong __maxsndque;
	cuLong __len_offset, __pkt_maxlen;
	cuChar __len_size;
	bool const __mode;
	RPCMGR* const __rpc;

protected:
	void SetPKParse(uLong len_offset, uChar len_size, uLong pkt_maxlen, int maxsndque);
	void BeginWork(uLong keepalive_seconds = 10, uLong delay = 0);

	virtual void ShutDown(uLong ulMilliseconds);

	void DisconnectAll();

private:
	TcpCommApp(RPCMGR* rpc, ThreadPool* processor, ThreadPool* communicator, bool mode);
	virtual ~TcpCommApp();

	void BeforeSel(selparm& p);
	void AfterSel(selparm& p);
	bool AddSocket(DataSocket* datasock);
	bool DelSocket(DataSocket* datasock);
	RunBiDirectChain<DataSocket> __socklist;
	BandwidthStat m_band;
	InterLockedLong m_selexit;

	ThreadPool *const __communicator, *const __communicator1, *const __processor, *const __processor1;
	InterLockedLong m_TickCount;
	uLong const __delay;
	uLong m_keepalive;

	InterLockedLong __atnotconn, __conntotal, __deltotal;
	int _SendData(DataSocket* datasock, WPacket& sendbuf);
};

/**
 * @class TcpClientApp
 * @author ZhangDabo
 * @brief Tcp��װ�ͻ���
 * @bug
 */
class TcpClientApp : public TcpCommApp {
	friend class TcpServerApp;

public:
	TcpClientApp(RPCMGR* rpc, ThreadPool* processor, ThreadPool* communicator, bool mode = true); // mode =false ǿ�ơ����ݰ��������봦���̳߳�
	~TcpClientApp(){};

	/**
	 * @brif ���ӷ�����
	 * @param[in]  hostname  ��������ַ
	 * @param[in]  port �������˿�
	 * @param[in]  sock_out ���Ӷ˿�
	 */
	DataSocket* Connect(cChar* hostname, uShort port, SOCKET* sock_out = 0);

protected:
	virtual void TaskDispatcher(Task*);
};

/**
 * @class TcpServerApp
 * @author ZhangDabo
 * @brief Tcp��װ�����
 * @bug
 */
class TcpServerApp : public TcpClientApp {
public:
	TcpServerApp(RPCMGR* rpc, ThreadPool* processor, ThreadPool* communicator, bool mode = true); // mode =false ǿ�ơ����ݡ��������봦���̳߳�
	~TcpServerApp();

	/**
	 * @brif �������˿�
	 * @param[in]  port  �������˿�
	 * @param[in]  cp ��������ַ
	 *
	 * @return bool /����ֵ0���ɹ���1��ʧ��
	 */
	int OpenListenSocket(uShort port, cChar* cp); // ����ֵ0���ɹ���1��ʧ��

	/**
	 * @brif �رռ���
	 */
	void CloseListenSocket();

	const SOCKET GetListenSocket() const { return __socket; }
	cChar* GetListenIP() const { return __localaddr; }
	uShort GetListenPort() const { return __port; }
	void ZeroAcceptFlag() { __acceptflag = false; }

protected:
	virtual void ShutDown(uLong ulMilliseconds);
	virtual void TaskDispatcher(Task*);

private:
	bool volatile __acceptflag; // NOTE(Ogge): Only one accept() at a time is allowed. (true: processing an accept)
	SOCKET volatile __socket;
	uShort __port;		  // �����������˿�(�����ֽ�˳��)
	char __localaddr[16]; // ������������ַ(IP)
};

#pragma pack(pop)
_DBC_END
#pragma warning(disable : 4355)

#endif
