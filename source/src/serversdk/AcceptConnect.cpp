
#include "Comm.h"
#include "AcceptConnect.h"
#include "DataSocket.h"

#ifdef PKO_PLATFORM_LINUX
#include <netinet/tcp.h>
#endif

_DBC_USING

extern PreAllocHeapPtr<DataSocket> __sockheap;
PreAllocHeapPtr<AcceptConnect> AcceptConnect::m_acceptheap(0, 5);
PreAllocHeapPtr<OnConnect> AcceptConnect::m_connectheap(0, 5);
PreAllocHeapPtr<DelConnect> DelConnect::m_delHeap(1, 50);
//================================================================================
inline void OnConnect::Init(DataSocket* datasock) {
	m_datasock = datasock;
	__tca = m_datasock->GetTcpApp();
}

int OnConnect::Process() {
	bool l_b = false;
	try {
		l_b = __tca->OnConnect(m_datasock);
	} catch (...) {
	}
	if (l_b) {
		__tca->AddSocket(m_datasock);
		__tca->OnConnected(m_datasock);
	} else {
		m_datasock->Free();
	}
	return 0;
}
Task* OnConnect::Lastly() {
	--(__tca->__conntotal);
	return PreAllocTask::Lastly();
}

//================================================================================
int AcceptConnect::Process() {
	SOCKET l_sock;
	sockaddr_in l_peersaddr;
#ifdef PKO_PLATFORM_WINDOWS
	int l_addrlen = sizeof sockaddr_in;
#else
	socklen_t l_addrlen = sizeof(sockaddr_in);
#endif
	l_sock = accept(__tsa->GetListenSocket(), (sockaddr*)&l_peersaddr, &l_addrlen);
	__tsa->ZeroAcceptFlag();
	if (l_sock == INVALID_SOCKET) {
		return -1;
	}
	u_long l_noblock = 1;
	if (ioctlsocket(l_sock, FIONBIO, &l_noblock)) {
		closesocket(l_sock);
		return 0;
	}
	
	// SO_LINGER with l_onoff=1, l_linger=0: Forces RST on close instead of FIN
	// This eliminates TIME_WAIT state accumulation during DDoS attacks
	// Trade-off: Less graceful disconnect, but critical for DDoS mitigation
	struct linger ling;
	ling.l_onoff = 1;   // Enable linger
	ling.l_linger = 0;  // Timeout of 0 = immediate RST, no TIME_WAIT
	if (setsockopt(l_sock, SOL_SOCKET, SO_LINGER, (cChar*)&ling, sizeof(ling))) {
		closesocket(l_sock);
		return 0;
	}
	
	BOOL l_enable = 1;
	if (setsockopt(l_sock, SOL_SOCKET, SO_KEEPALIVE, (cChar*)&l_enable, sizeof(l_enable))) {
		closesocket(l_sock);
		return 0;
	}
#ifdef _DEBUG
#ifdef PKO_PLATFORM_WINDOWS
	struct tcp_keepalive ka;
	ka.onoff = 1;
	ka.keepalivetime = 5 * 60000; // 5 Minutes
	ka.keepaliveinterval = 1;
	DWORD bytesreturned;
	if (WSAIoctl(l_sock, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), nullptr, 0, &bytesreturned, nullptr, nullptr)) {
		closesocket(l_sock);
		return 0;
	}
#else
	// Linux: use setsockopt for TCP keepalive parameters
	int keepidle = 5 * 60; // 5 minutes (seconds)
	int keepintvl = 1;     // 1 second interval
	int keepcnt = 5;       // 5 probes
	setsockopt(l_sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle));
	setsockopt(l_sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl));
	setsockopt(l_sock, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt));
#endif
#endif
	DataSocket* l_datasock = __sockheap.Get();
	l_datasock->Init(l_sock, inet_ntoa(l_peersaddr.sin_addr), ntohs(l_peersaddr.sin_port), __tsa, true);

	OnConnect* l_onconn;
	l_onconn = m_connectheap.Get();
	l_onconn->Init(l_datasock);
	if (__tsa->GetProcessor()) {
		++(__tsa->__conntotal);
		if (!__tsa->__atnotconn) {
			__tsa->GetProcessor()->AddTask(l_onconn);
		} else {
			--(__tsa->__conntotal);
		}
	} else {
		try {
			l_onconn->Process();
		} catch (...) {
		}
		try {
			l_onconn->Free();
		} catch (...) {
		}
	}
	return 0;
};
Task* AcceptConnect::Lastly() {
	--(__tsa->__conntotal);
	return PreAllocTask::Lastly();
}
//================================================================================
int DelConnect::Process() {
	try {
		__tca->OnDisconnect(m_datasock, m_reason);
	} catch (...) {
	}

	m_datasock->Free();
	return 0;
}

Task* DelConnect::Lastly() {
	--(__tca->__deltotal);
	return PreAllocTask::Lastly();
}
