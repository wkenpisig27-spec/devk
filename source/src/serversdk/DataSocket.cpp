
#include "DataSocket.h"
#include "Sender.h"
#include "Receiver.h"
#include "Comm.h"
#include "RPCInfo.h"
#include <string>

_DBC_USING

DataSocket::DataSocket(uLong size)
	: PreAllocStru(size), m_isServer(false), __tca(0), m_socket(INVALID_SOCKET), m_sender(*new Sender(this)), m_receiver(*new Receiver(this)) {
	Initially();
}

DataSocket::~DataSocket() {
	delete &m_receiver;
	delete &m_sender;
};

void DataSocket::Initially() {
	RunBiDirectItem<DataSocket>::Initially();

	m_rpcinfo.store(nullptr, std::memory_order_relaxed);
	m_appinfo.store(nullptr, std::memory_order_relaxed);
	m_sbts = m_spks = m_rbts = m_rpks = 0;
	m_sendbytes.store(0, std::memory_order_relaxed);
	m_recvbytes.store(0, std::memory_order_relaxed);
	m_sendpkts.store(0, std::memory_order_relaxed);
	m_recvpkts.store(0, std::memory_order_relaxed);
	m_sendbyteps.store(0, std::memory_order_relaxed);
	m_recvbyteps.store(0, std::memory_order_relaxed);
	m_sendpktps.store(0, std::memory_order_relaxed);
	m_recvpktps.store(0, std::memory_order_relaxed);

	m_sendflag.store(0, std::memory_order_relaxed);
	m_recvflag.store(0, std::memory_order_relaxed);
	m_procflag.store(0, std::memory_order_relaxed);
	m_deltime.store(0, std::memory_order_relaxed);
	m_delflag.store(0, std::memory_order_relaxed);
	m_delremain.store(0, std::memory_order_relaxed);
	m_delreason.store(0, std::memory_order_relaxed);
	m_isProcess.store(1, std::memory_order_relaxed);
	m_gsCheck.store(0, std::memory_order_relaxed);

	m_sender.Initially();
	m_receiver.Initially();
}
void DataSocket::Finally() {
	m_receiver.Finally();
	m_sender.Finally();

	if (m_socket != INVALID_SOCKET) {
		closesocket(m_socket);
		*const_cast<SOCKET*>(&m_socket) = INVALID_SOCKET;
	}
	RPCInfo* rpc = m_rpcinfo.exchange(nullptr, std::memory_order_relaxed);
	if (rpc) {
		delete rpc;
	}

	RunBiDirectItem<DataSocket>::Finally();
	m_appinfo.store(nullptr, std::memory_order_relaxed);
}

void DataSocket::Init(SOCKET socket, cChar* peerip, uShort peerport, TcpCommApp* tca, bool IsServer) {
	m_socket = socket;
	m_isServer = IsServer;
	__tca = tca;
	if (tca->__rpc) {
		RPCInfo* rpc = new RPCInfo;
		if (rpc == nullptr) {
			printf("rpc failed!\n");
		}
		m_rpcinfo.store(rpc, std::memory_order_relaxed);
	}

	m_peerport = peerport;
	strncpy_s(m_peerip, sizeof(m_peerip), peerip, _TRUNCATE);

#ifdef PKO_PLATFORM_WINDOWS
	int l_len = sizeof(int);
#else
	socklen_t l_len = sizeof(int);
#endif
	int l_buflen = 0;
	getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)&l_buflen, &l_len);
	m_sendbuf.store(uLong(l_buflen), std::memory_order_relaxed);
	getsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char*)&l_buflen, &l_len);
	m_recvbuf.store(uLong(l_buflen), std::memory_order_relaxed);

	sockaddr_in l_sa;
	l_len = sizeof(l_sa);
	MemSet((char*)&l_sa, 0, l_len);
	getsockname(m_socket, (sockaddr*)&l_sa, &l_len);
	strncpy_s(m_localip, sizeof(m_localip), inet_ntoa(l_sa.sin_addr), _TRUNCATE);
	m_localport = ntohs(l_sa.sin_port);

	m_receiver.Init();
	m_sender.Init();
};

WPacket DataSocket::GetWPacket() {
	return __tca->GetWPacket();
}
int DataSocket::SendData(WPacket sendbuf) {
	return __tca ? __tca->SendData(this, sendbuf) : -100;
}

void* DataSocket::GetPointer() const {
	return m_appinfo.load(std::memory_order_relaxed);
}
bool DataSocket::SetPointer(void* appinfo) {
	bool l_retval = false;
	if (!m_appinfo.load(std::memory_order_relaxed) || !appinfo) {
		m_appinfo.store(appinfo, std::memory_order_relaxed);
		l_retval = true;
	}
	return l_retval;
}

extern PreAllocHeap<rbuf> __bufheap;
int DataSocket::SetSendBuf(uLong bytes) {
	bytes = ((bytes + __bufheap.GetUnitSize() - 1) / __bufheap.GetUnitSize()) * __bufheap.GetUnitSize();
	const uLong sendBuf = max(bytes, 4 * 1024); //>=4K
	m_sendbuf.store(sendBuf, std::memory_order_relaxed);
	int l_sendbuf = static_cast<int>(sendBuf);
	return setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)&l_sendbuf, sizeof(int));
}
int DataSocket::SetRecvBuf(uLong bytes) {
	bytes = ((bytes + __bufheap.GetUnitSize() - 1) / __bufheap.GetUnitSize()) * __bufheap.GetUnitSize();
	const uLong recvBuf = max(bytes, 4 * 1024); //>=4K
	m_recvbuf.store(recvBuf, std::memory_order_relaxed);
	int l_recvbuf = static_cast<int>(recvBuf);
	return setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char*)&l_recvbuf, sizeof(int));
}
