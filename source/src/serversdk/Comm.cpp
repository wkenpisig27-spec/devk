//================================================================
// It must be permitted by Dabo.Zhang that this program is used for
// any purpose in any situation.
// Copyright (C) Dabo.Zhang 2000-2003
// All rights reserved by ZhangDabo.
// This program is written(created) by Zhang.Dabo in 2000.3
// This program is modified recently by Zhang.Dabo in 2003.7
//=================================================================
#define FD_SETSIZE 2048

#include "Comm.h"
#include "CommRPC.h"
#include "ThreadPool.h"
#include "Sender.h"
#include "Receiver.h"
#include "AcceptConnect.h"

#include "iostream"

#ifdef PKO_PLATFORM_WINDOWS
// Windows: override FD_SET to skip the limit-check and use the fd_array directly
#ifdef FD_SET
#undef FD_SET
#endif
#define FD_SET(fd, set)                                                    \
	((fd_set FAR*)(set))->fd_array[((fd_set FAR*)(set))->fd_count] = (fd); \
	((fd_set FAR*)(set))->fd_count++;
#else
// Linux: glibc's _FORTIFY_SOURCE replaces FD_SET/FD_ISSET/FD_CLR with versions
// that call __fdelt_chk(), which aborts if fd >= __FD_SETSIZE (hardcoded to 1024
// in libc.so, ignoring our #define FD_SETSIZE 2048).
// Override __FD_ELT to bypass the glibc runtime check while keeping our larger fd_set.
#ifdef __FD_ELT
#undef __FD_ELT
#endif
#define __FD_ELT(d) ((long int)((d) / __NFDBITS))

// Safe wrappers that validate fd range before calling FD_SET/FD_ISSET
// to prevent buffer overflow on our 2048-bit fd_set
#undef FD_SET
#define FD_SET(fd, set) do { \
	int _fd = (int)(fd); \
	if (_fd >= 0 && _fd < FD_SETSIZE) { \
		((set)->fds_bits[_fd / __NFDBITS] |= (1UL << (_fd % __NFDBITS))); \
	} \
} while(0)

#undef FD_ISSET
#define FD_ISSET(fd, set) ({ \
	int _fd = (int)(fd); \
	(_fd >= 0 && _fd < FD_SETSIZE) ? \
		(((set)->fds_bits[_fd / __NFDBITS] & (1UL << (_fd % __NFDBITS))) != 0) : 0; \
})
#endif

// On Linux, select() needs nfds = max_fd + 1
// On Windows, the first param is ignored (always 0)
#ifdef PKO_PLATFORM_LINUX
// Helper to track max fd when adding to fd_set
struct FdTracker {
	int maxfd;
	FdTracker() : maxfd(-1) {}
	void track(SOCKET fd) {
		int ifd = (int)fd;
		if (ifd >= 0 && ifd < FD_SETSIZE && ifd > maxfd)
			maxfd = ifd;
	}
	int nfds() const { return maxfd + 1; }
};
#define SELECT_NFDS(tracker) (tracker).nfds()
#else
struct FdTracker {
	void track(SOCKET) {}
};
#define SELECT_NFDS(tracker) 0
#endif

_DBC_USING
InterLockedLong __bufinit = 0;
PreAllocHeap<rbuf> __bufheap(1024);
PreAllocHeapPtr<DataSocket> __sockheap(1, 100);
extern uLong NetBuffer[];

bool g_bCommAppDebug = false;
void SetCommAppDebug(bool bDebug) {
	g_bCommAppDebug = bDebug;
}
// �ϲ�Ӧ���ض���:
// static uLong	NetBuffer[]	={400,300,200,100,50,40,30,0};

//=====DispatchThread==============================================================================
class DispatchThread : public Task {
public:
	DispatchThread(TcpCommApp* tcpapp) : m_tcpapp(tcpapp){};

private:
	virtual int Process() {
		int l_stat = 0;
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
		m_tcpapp->TaskDispatcher(this);
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
		return 0;
	};

	TcpCommApp* m_tcpapp;
};
//================================================================================
int TcpCommApp::WSAStartup() {
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(2, 2);

	err = ::WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		return -1;
	}

	if (LOBYTE(wsaData.wVersion) != 2 ||
		HIBYTE(wsaData.wVersion) != 2) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		WSACleanup();
		return -1;
	}
	return 0;
}
void TcpCommApp::WSACleanup() {
	::WSACleanup();
}
//================================================================================
TcpCommApp::TcpCommApp(RPCMGR* rpc, ThreadPool* processor, ThreadPool* communicator, bool mode)
	: __maxsndque(10), m_keepalive(10 * 1000), __delay(0), __len_offset(0), __len_size(2), __pkt_maxlen(4 * 1024), __rpc(rpc), m_selexit(0), __mode(mode), __communicator1(communicator), __communicator(communicator ? communicator : ThreadPool::CreatePool(3, 3, 128, THREAD_PRIORITY_ABOVE_NORMAL)), __processor1(processor), __processor(processor), __atnotconn(0), __conntotal(0), __deltotal(0) {
	m_band.m_sendbytes = m_band.m_sendpkts = m_band.m_recvbytes = m_band.m_recvpkts = 0;
};
TcpCommApp::~TcpCommApp() {
	if (!__communicator1) {
		if (__communicator) {
			__communicator->m_taskexitflag = TRUE;
			__communicator->DestroyPool();
		}
	}
	if (!__processor1) {
		if (__processor) {
			__processor->m_taskexitflag = TRUE;
			__processor->DestroyPool();
		}
	}
};
void TcpCommApp::ShutDown(uLong ulMilliseconds) {
	__atnotconn = 1;
	while (__conntotal) {
		Sleep(1);
	}
	RunChainGetArmor<DataSocket> l(__socklist);
	for (DataSocket* l_datasock = __socklist.GetNextItem(); l_datasock; l_datasock = __socklist.GetNextItem()) {
		::shutdown(l_datasock->m_socket, SD_SEND);
		Disconnect(l_datasock, 0, DS_SHUTDOWN);
	}
	l.unlock();

	uLong l_starttime = GetTickCount();
	while (GetTickCount() - l_starttime < ulMilliseconds && GetSockTotal()) {
		Sleep(1);
	}
	while (__deltotal) {
		Sleep(1);
	}
	m_selexit = 1;
	l_starttime = GetTickCount();
	while (m_selexit && GetTickCount() - l_starttime < 1000) {
		Sleep(1);
	}
}
//--------------------------------------------------------------------
void TcpCommApp::SetPKParse(uLong len_offset, uChar len_size, uLong pkt_maxlen, int maxsndque) {
	*const_cast<Long*>(&__maxsndque) = static_cast<Long>(max(maxsndque, (int)10));
	*const_cast<uLong*>(&__len_offset) = len_offset;
	*const_cast<uChar*>(&__len_size) = len_size;
	*const_cast<uLong*>(&__pkt_maxlen) = pkt_maxlen;
	if (((__len_size != 2) && (__len_size != 4) && (__len_size != 1))) {
		std::runtime_error("packet size is wrong!");
	}
}
bool TcpCommApp::AddSocket(DataSocket* datasock) {
	if (datasock) {
		datasock->m_sendtime = datasock->m_recvtime = m_TickCount;
		return datasock->_BeginRun(&__socklist) ? true : false;
	} else {
		return false;
	}
}
inline bool TcpCommApp::DelSocket(DataSocket* datasock) {
	return datasock ? datasock->_EndRun() ? true : false : false;
}
BandwidthStat TcpCommApp::GetBandwidthStat() const {
	BandwidthStat l_band;
	RunChainGetArmor<DataSocket> l(__socklist);
	l_band = m_band;
	l.unlock();
	return l_band;
}
uLong TcpCommApp::GetPktHead() const {
	return __len_offset + __len_size + (__rpc ? (RPCMGR::__ses_size) : 0);
}
WPacket TcpCommApp::GetWPacket() const {
	WPacket l_ret = this;
	l_ret = __bufheap.Get(1024);
	return l_ret;
}
void TcpCommApp::BeginWork(uLong keepalive_seconds, uLong delay) {
	m_keepalive = ((keepalive_seconds && (keepalive_seconds < 10)) ? 10 : keepalive_seconds) * 1000;
	*const_cast<uLong*>(&__delay) = delay;
	m_TickCount = GetTickCount();
	if (++__bufinit == 1) {
		__bufheap.Init(NetBuffer);
	}
	Task* task = new DispatchThread(this);
	__communicator->AddTask(task);
}

void TcpCommApp::DisconnectAll() {
	RunChainGetArmor<DataSocket> l(__socklist);
	DataSocket* l_datasock;
	while (l_datasock = __socklist.GetNextItem()) {
		Disconnect(l_datasock);
	}
	l.unlock();
}

//--------------------------------------------------------------------
void TcpCommApp::Disconnect(DataSocket* datasock, uLong remain, int reason) {
	if (datasock) {
		datasock->m_delreason = reason;
		datasock->m_delremain = remain;
		datasock->m_deltime = datasock->__tca->m_TickCount;
	}
};
//--------------------------------------------------------------------
int TcpCommApp::SendData(DataSocket* datasock, WPacket sendbuf) {
	if (__rpc) {
		return __rpc->SendSESS(datasock, sendbuf);
	} else {
		return _SendData(datasock, sendbuf);
	}
}
int TcpCommApp::_SendData(DataSocket* datasock, WPacket& sendbuf) {
	if (!datasock || datasock->m_delflag || !bool(sendbuf)) {
		return -1;
	}
	datasock->m_sender << sendbuf;
	return 0;
};
//--------------------------------------------------------------------
_DBC_BEGIN
struct selparm {
	selparm() : m_tick(0) {}
	fd_set m_readfds, m_writefds, m_errfds;
	bool m_readflag, m_writeflag, m_errflag;
	int m_selretval;
	uLong m_tick;
	FdTracker m_fdtrack;
};
_DBC_END

void TcpCommApp::BeforeSel(selparm& p) {
	if (g_bCommAppDebug) {
		printf("CommApp: BeforeSel start!\n");
	}

	uLong l_tick = GetTickCount();
	if (l_tick > uLong(m_TickCount)) {
		m_TickCount = l_tick;
	} else if (l_tick < uLong(m_TickCount)) {
		l_tick = ++m_TickCount;
		std::cout << std::endl
				  << "system GetTickCount function wrong: later < before." << std::endl;
	}
	uLong l_sendbytes, l_recvbytes, l_sendpkts, l_recvpkts;
	uLong l_intertime;
	bool l_flag = ((l_tick - p.m_tick) >= 1000);
	RunChainGetArmor<DataSocket> l(__socklist);
	if (l_flag) {
		l_sendbytes = l_recvbytes = l_sendpkts = l_recvpkts = 0;
		l_intertime = l_tick - p.m_tick;
		p.m_tick = l_tick;
		m_band.m_tick = l_tick;
	}
	DataSocket* l_datasock;
	while (l_datasock = __socklist.GetNextItem()) {
		if (l_flag) {
			uLong l_ulong = l_datasock->m_sbts.SetZero();
			if (l_ulong) {
				l_sendbytes += l_ulong;
				l_datasock->m_sendbytes += l_ulong;
				if (l_ulong < (0xFFFFFFFF / 1000)) {
					l_datasock->m_sendbyteps = l_ulong * 1000 / l_intertime;
				} else {
					l_datasock->m_sendbyteps = (l_ulong / l_intertime) * 1000;
				}
			} else {
				l_datasock->m_sendbyteps = 0;
			}
			l_ulong = l_datasock->m_spks.SetZero();
			if (l_ulong) {
				l_sendpkts += l_ulong;
				l_datasock->m_sendpkts += l_ulong;
				if (l_ulong < (0xFFFFFFFF / 1000)) {
					l_datasock->m_sendpktps = l_ulong * 1000 / l_intertime;
				} else {
					l_datasock->m_sendpktps = (l_ulong / l_intertime) * 1000;
				}
			} else {
				l_datasock->m_sendpktps = 0;
			}
			l_ulong = l_datasock->m_rbts.SetZero();
			if (l_ulong) {
				l_recvbytes += l_ulong;
				l_datasock->m_recvbytes += l_ulong;
				if (l_ulong < (0xFFFFFFFF / 1000)) {
					l_datasock->m_recvbyteps = l_ulong * 1000 / l_intertime;
				} else {
					l_datasock->m_recvbyteps = (l_ulong / l_intertime) * 1000;
				}
			} else {
				l_datasock->m_recvbyteps = 0;
			}
			l_ulong = l_datasock->m_rpks.SetZero();
			if (l_ulong) {
				l_recvpkts += l_ulong;
				l_datasock->m_recvpkts += l_ulong;
				if (l_ulong < (0xFFFFFFFF / 1000)) {
					l_datasock->m_recvpktps = l_ulong * 1000 / l_intertime;
				} else {
					l_datasock->m_recvpktps = (l_ulong / l_intertime) * 1000;
				}
			} else {
				l_datasock->m_recvpktps = 0;
			}
		}
		if (m_keepalive) {
			if (l_tick - uLong(l_datasock->m_recvtime) > m_keepalive) {
				Disconnect(l_datasock, 0, -9);
			} else if (l_tick - 2000 > uLong(l_datasock->m_sendtime)) {
				WPacket l_wpk = GetWPacket();

				// Add by lark.li 20090309 begin
				// Ping����
				l_wpk.WriteCmd(10000);
				l_wpk.WriteSESS(1);
				// End

				l_wpk.m_wpos = __len_offset + __len_size - l_wpk.GetPktLen();
				_SendData(l_datasock, l_wpk);
				l_datasock->m_sendtime = l_tick;
			}
		}
		if (l_datasock->m_deltime) {
			if (l_datasock->m_isProcess)
				l_datasock->m_isProcess = 0;
			if ((l_tick - l_datasock->m_deltime) >= uLong(l_datasock->m_delremain)) {
				if (!l_datasock->m_delflag)
					l_datasock->m_delflag = 1;
				if (l_datasock->m_recvflag++ == 0) {
					if (l_datasock->m_procflag <= 0) {
						if (l_datasock->m_sendflag++ == 0) {
							DelSocket(l_datasock);

							++__deltotal;
							DelConnect* l_del = DelConnect::m_delHeap.Get();
							l_del->Init(l_datasock, l_datasock->m_delreason);
							(__processor ? __processor : __communicator)->AddTask(l_del);
						} else {
							l_datasock->m_sendflag--;
							l_datasock->m_recvflag--;
						}
					} else {
						l_datasock->m_recvflag--;
					}
				} else {
					l_datasock->m_recvflag--;
				}
				continue;
			}
		}

		// Skip invalid sockets (can happen during disconnect race)
		if (l_datasock->m_socket == INVALID_SOCKET || (int)l_datasock->m_socket < 0) {
			continue;
		}

		FD_SET(l_datasock->m_socket, &(p.m_errfds));
		p.m_fdtrack.track(l_datasock->m_socket);
		p.m_errflag = true;

		if (!l_datasock->m_recvflag) {
			FD_SET(l_datasock->m_socket, &(p.m_readfds));
			p.m_fdtrack.track(l_datasock->m_socket);
			p.m_readflag = true;
		}
		if (!l_datasock->m_sendflag && l_datasock->m_sender.HasData()) {
			FD_SET(l_datasock->m_socket, &(p.m_writefds));
			p.m_fdtrack.track(l_datasock->m_socket);
			p.m_writeflag = true;
		}
	} // while(l_datasock =__socklist.GetNextItem())
	if (l_flag) {
		m_band.m_sendbytes += l_sendbytes;
		m_band.m_sendpkts += l_sendpkts;
		m_band.m_recvbytes += l_recvbytes;
		m_band.m_recvpkts += l_recvpkts;
		if (l_sendbytes < (0xFFFFFFFF / 1000)) {
			m_band.m_sendbyteps = l_sendbytes * 1000 / l_intertime;
		} else {
			m_band.m_sendbyteps = (l_sendbytes / l_intertime) * 1000;
		}
		if (l_sendpkts < (0xFFFFFFFF / 1000)) {
			m_band.m_sendpktps = l_sendpkts * 1000 / l_intertime;
		} else {
			m_band.m_sendpktps = (l_sendpkts / l_intertime) * 1000;
		}
		if (l_recvbytes < (0xFFFFFFFF / 1000)) {
			m_band.m_recvbyteps = l_recvbytes * 1000 / l_intertime;
		} else {
			m_band.m_recvbyteps = (l_recvbytes / l_intertime) * 1000;
		}
		if (l_recvpkts < (0xFFFFFFFF / 1000)) {
			m_band.m_recvpktps = l_recvpkts * 1000 / l_intertime;
		} else {
			m_band.m_recvpktps = (l_recvpkts / l_intertime) * 1000;
		}
	}
	l.unlock();

	if (g_bCommAppDebug) {
		printf("CommApp: BeforeSel end!\n");
	}
}
void TcpCommApp::AfterSel(selparm& p) {
	if (g_bCommAppDebug) {
		printf("CommApp: AfterSel start!\n");
	}

	RunChainGetArmor<DataSocket> l(__socklist);
	DataSocket* l_datasock;
	while ((l_datasock = __socklist.GetNextItem()) && p.m_selretval) {
		if (l_datasock->m_delflag)
			continue;
		if (FD_ISSET(l_datasock->m_socket, &(p.m_errfds))) {
			Disconnect(l_datasock, 0, -1);
			p.m_selretval--;
		} else {
			if (!l_datasock->m_recvflag && FD_ISSET(l_datasock->m_socket, &(p.m_readfds))) {
				if (l_datasock->m_recvflag++ == 0) {
					__communicator->AddTask(&(l_datasock->m_receiver));
				} else {
					l_datasock->m_recvflag--;
				}
				p.m_selretval--;
			}
			if (!l_datasock->m_sendflag && l_datasock->m_sender.HasData() && FD_ISSET(l_datasock->m_socket, &(p.m_writefds))) {
				if (l_datasock->m_sendflag++ == 0) {
					__communicator->AddTask(&(l_datasock->m_sender));
				} else {
					l_datasock->m_sendflag--;
				}
				p.m_selretval--;
			}
		}
	}
	l.unlock();

	if (g_bCommAppDebug) {
		printf("CommApp: AfterSel end!\n");
	}
}
//==============TcpClientApp==================================================================
TcpClientApp::TcpClientApp(RPCMGR* rpc, ThreadPool* processor, ThreadPool* communicator, bool mode)
	: TcpCommApp(rpc, processor, communicator, mode) {
}
void TcpClientApp::TaskDispatcher(Task* l_dt) {
	timeval l_timeout;
	selparm l_p;

	while (!l_dt->GetExitFlag() && !m_selexit) {
		FD_ZERO(&(l_p.m_readfds));
		FD_ZERO(&(l_p.m_writefds));
		FD_ZERO(&(l_p.m_errfds));
		l_p.m_readflag = l_p.m_writeflag = l_p.m_errflag = false;
		l_p.m_fdtrack = FdTracker();
		BeforeSel(l_p);
		if (!l_p.m_readflag && !l_p.m_writeflag && !l_p.m_errflag) {
			Sleep(2);
			continue;
		}
		l_timeout.tv_sec = 0;
		l_timeout.tv_usec = 2000;
		l_p.m_selretval = select(SELECT_NFDS(l_p.m_fdtrack), l_p.m_readflag ? &(l_p.m_readfds) : 0, l_p.m_writeflag ? &(l_p.m_writefds) : 0, l_p.m_errflag ? &(l_p.m_errfds) : 0, &l_timeout);
		switch (l_p.m_selretval) {
		case 0: {
			break;
		}
		case SOCKET_ERROR: {
			int l_wsaerr = WSAGetLastError();
			if ((l_wsaerr == WSAENETDOWN) || (l_wsaerr == WSANOTINITIALISED)) {
				return;
			}
			break;
		}
		default: {
			AfterSel(l_p);
		}
		}
		if (__delay)
			Sleep(__delay);
	}
	m_selexit = 0;
};
//--------------------------------------------------------------------
DataSocket* TcpClientApp::Connect(cChar* hostname, uShort port, SOCKET* sock_out) {
	if (!__atnotconn) {
		RefArmor l_conn(__conntotal);
		sockaddr_in l_peersaddr;
		HOSTENT* l_hostent;

		MemSet((char*)&l_peersaddr, 0, sizeof(sockaddr_in));
		l_peersaddr.sin_family = AF_INET;
		l_peersaddr.sin_port = htons(port);
		if ((l_peersaddr.sin_addr.s_addr = inet_addr(hostname)) == INADDR_NONE) {
			l_hostent = gethostbyname(hostname);
			if (!l_hostent) {
				return 0;
			}
			l_peersaddr.sin_addr.s_addr = *(uLong*)(l_hostent->h_addr);
		}
		SOCKET l_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (sock_out)
			*sock_out = l_sock;

		if (connect(l_sock, (sockaddr*)&l_peersaddr, sizeof(sockaddr)) == SOCKET_ERROR) {
			closesocket(l_sock);
			return 0;
		}

		u_long l_noblock = 1;
		if (ioctlsocket(l_sock, FIONBIO, &l_noblock)) {
			return 0;
		}
		BOOL l_enable = 1;
		// if(setsockopt(l_sock,SOL_SOCKET,SO_OOBINLINE,(cChar*)&l_enable,sizeof(l_enable))){return 0;}
		if (setsockopt(l_sock, SOL_SOCKET, SO_KEEPALIVE, (cChar*)&l_enable, sizeof(l_enable))) {
			return 0;
		}

		DataSocket* l_datasock = __sockheap.Get();
		l_datasock->Init(l_sock, inet_ntoa(l_peersaddr.sin_addr), port, this, false);

		bool l_b = false;
		try {
			l_b = OnConnect(l_datasock);
		} catch (...) {
		}
		if (l_b) {
			AddSocket(l_datasock);
			return l_datasock;
		} else {
			l_datasock->Free();
			return 0;
		}
	} else {
		return 0;
	}
};
//==============TcpServerApp==================================================================
TcpServerApp::TcpServerApp(RPCMGR* rpc, ThreadPool* processor, ThreadPool* communicator, bool mode)
	: __socket(INVALID_SOCKET), __port(1973), __acceptflag(false), TcpClientApp(rpc, processor, communicator, mode) {
	in_addr l_ina;
	l_ina.s_addr = INADDR_ANY;
	// strcpy(__localaddr,inet_ntoa(l_ina));
	strncpy_s(__localaddr, sizeof(__localaddr), inet_ntoa(l_ina), _TRUNCATE);
}
TcpServerApp::~TcpServerApp() {
	if (__socket != INVALID_SOCKET) {
		closesocket(__socket);
	}
};
//--------------------------------------------------------------------
void TcpServerApp::ShutDown(uLong ulMilliseconds) {
	CloseListenSocket();
	TcpClientApp::ShutDown(ulMilliseconds);
}
//--------------------------------------------------------------------
int TcpServerApp::OpenListenSocket(uShort port, cChar* cp) {
	__port = port;
	if (cp && (strlen(cp) < 16)) {
		// strcpy(__localaddr,cp);
		strncpy_s(__localaddr, sizeof(__localaddr), cp, _TRUNCATE);
	} else {
		in_addr l_ina;
		l_ina.s_addr = INADDR_ANY;
		// strcpy(__localaddr,inet_ntoa(l_ina));
		strncpy_s(__localaddr, sizeof(__localaddr), inet_ntoa(l_ina), _TRUNCATE);
	}

	if (__socket != INVALID_SOCKET) {
		return -1;
	}
	__socket = socket(PF_INET, SOCK_STREAM, 0);
	if (__socket == INVALID_SOCKET) {
		return -1;
	}

	sockaddr_in my_addr;
	MemSet((char*)&my_addr, 0, sizeof(my_addr));

	uLong l_inaddr = __localaddr[0] ? inet_addr(__localaddr) : INADDR_ANY;
	if (l_inaddr == INADDR_NONE) {
		return -1;
	}

	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = l_inaddr;
	my_addr.sin_port = htons(__port);

	if (bind(__socket, (sockaddr FAR*)&my_addr, sizeof(my_addr))) {
		return -2;
	}
	if (listen(__socket, SOMAXCONN)) {
		return -3;
	}

	// �趨socketΪ������ģʽ
	u_long l_noblock = 1;
	if (ioctlsocket(__socket, FIONBIO, &l_noblock)) {
		return -4;
	}
	__acceptflag = false;

	return 0;
};
//--------------------------------------------------------------------
void TcpServerApp::CloseListenSocket() {
	SOCKET l_sock = __socket;
	if (l_sock != INVALID_SOCKET) {
		__socket = INVALID_SOCKET;
		closesocket(l_sock);
	}
	__acceptflag = true;
};
//--------------------------------------------------------------------
void TcpServerApp::TaskDispatcher(Task* l_dt) {
	AcceptConnect* l_newconn;
	timeval l_timeout;
	selparm l_p;

	while (!l_dt->GetExitFlag() && !m_selexit) {
		FD_ZERO(&(l_p.m_readfds));
		FD_ZERO(&(l_p.m_writefds));
		FD_ZERO(&(l_p.m_errfds));
		l_p.m_readflag = l_p.m_writeflag = l_p.m_errflag = false;
		l_p.m_fdtrack = FdTracker();
		if (__socket != INVALID_SOCKET) {
			FD_SET(__socket, &(l_p.m_errfds));
			l_p.m_fdtrack.track(__socket);
			l_p.m_errflag = true;
			if (!__acceptflag && !__atnotconn) {
				FD_SET(__socket, &(l_p.m_readfds));
				l_p.m_fdtrack.track(__socket);
				l_p.m_readflag = true;
			}
		}
		BeforeSel(l_p);
		if (!l_p.m_readflag && !l_p.m_writeflag && !l_p.m_errflag) {
			Sleep(2);
			continue;
		}
		l_timeout.tv_sec = 0;
		l_timeout.tv_usec = 2000;
		l_p.m_selretval = select(SELECT_NFDS(l_p.m_fdtrack), l_p.m_readflag ? &(l_p.m_readfds) : 0, l_p.m_writeflag ? &(l_p.m_writefds) : 0, l_p.m_errflag ? &(l_p.m_errfds) : 0, &l_timeout);
		switch (l_p.m_selretval) {
		case 0: {
			break;
		}
		case SOCKET_ERROR: {
			int l_wsaerr = WSAGetLastError();
			if ((l_wsaerr == WSAENETDOWN) || (l_wsaerr == WSANOTINITIALISED)) {
				return;
			}
			break;
		}
		default: {
			if (FD_ISSET(__socket, &(l_p.m_errfds))) {
				// NOTE(Ogge): Error has been detected on __socket; Restart listening socket.
				CloseListenSocket();
				OpenListenSocket(__port, __localaddr);
				l_p.m_selretval--;
			} else if (!__acceptflag && FD_ISSET(__socket, &(l_p.m_readfds))) {
				++__conntotal;
				if (!__atnotconn) {
					l_newconn = AcceptConnect::m_acceptheap.Get();
					l_newconn->Init(this);
					__acceptflag = true;
					__communicator->AddTask(l_newconn);
				} else {
					--__conntotal;
				}
				l_p.m_selretval--;
			}
			AfterSel(l_p);
		}
		}
		if (__delay)
			Sleep(__delay);
	}
	m_selexit = 0;
}
dstring TcpCommApp::GetDisconnectErrText(int reason) {
	switch (reason) {
	case 0:
		return "���س��������˳���";
	case -1:
		return "Socket����";
	case -3:
		return "���类�Է������رա�";
	case -5:
		return "�����ȳ������ơ�";
	case -7:
		return "Socket�ϵ���������̫���ء�";
	case -9:
		return "KeepAliveʧ�ܡ�";
	case DS_DISCONN:
		return "������õ�Ĭ�Ϸ����ӡ�";
	case DS_SHUTDOWN:
		return "������Ϊ�˳���ShutDown������ϵͳ��";
	case 10054:
		return "Զ������ǿ�ȹر���һ�����е����ӡ�";
	case 10050:
		return "�׽��ֲ���������һ�����������硣";
	case 10051:
		return "��һ���޷����ӵ����糢����һ���׽��ֲ�����";
	case 10052:
		return "���ò����ڽ����У����ڱ��ֻ�Ĳ�����⵽һ�����ϣ��������жϡ�";
	case 10053:
		return "���������е�����������һ���ѽ��������ӡ�";
	case 10055:
		return "����ϵͳ�������ռ䲻����ж�����������ִ���׽����ϵĲ�����";
	case 10056:
		return "��һ���Ѿ����ӵ��׽���������һ����������";
	case 10057:
		return "�����׽���û�����Ӳ���(��ʹ��һ�� sendto ���÷������ݱ��׽���ʱ)û���ṩ��ַ�����ͻ�������ݵ�����û�б����ܡ�";
	case 10058:
		return "������ǰ�Ĺرյ��ã��׽������Ǹ������Ѿ��رգ����ͻ�������ݵ�����û�б����ܡ�";
	case 10059:
		return "��ĳ���ں˶�������ù��ࡣ";
	case 10060:
		return "�������ӷ���һ��ʱ���û����ȷ�𸴻����ӵ�����û�з�Ӧ�����ӳ���ʧ�ܡ�";
	case 10061:
		return "����Ŀ����������ܾ����޷����ӡ�";
	case 10064:
		return "����Ŀ���������ˣ��׽��ֲ���ʧ�ܡ�";
	case 10065:
		return "�׽��ֲ�������һ���޷����ӵ�������";
	case 10067:
		return "һ�� Windows �׽��ֲ��������ڿ���ͬʱʹ�õ�Ӧ�ó�����Ŀ�������ơ�";
	case 10048:
		return "ͨ��ÿ���׽��ֵ�ַ(Э��/�����ַ/�˿�)ֻ����ʹ��һ�Ρ�";
	case 10047:
		return "ʹ�����������Э�鲻���ݵĵ�ַ��";
	// default:return dstring("δ֪Disconnect�룺")<<reason;
	default:
		dstring s = "δ֪Disconnect�룺";
		s += reason;
		return s;
	}
}
