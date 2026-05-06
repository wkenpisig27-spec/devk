#pragma once

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include "serversdk/platform_compat.h"
// WPARAM/LPARAM stubs for Linux (not used on Linux server)
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
#endif

class CUdpSocket {
public:
	CUdpSocket();
	virtual ~CUdpSocket();
	virtual void Close();

	bool IsCreate(); // 是否已经创建

	bool RegeditReadEvent(HWND hWnd, DWORD dwMessage);
	bool OnNetworkEvent(WPARAM wParam, LPARAM lParam);

	SOCKET GetSocket() { return m_Socket; }

protected:
	SOCKET m_Socket;
};

class CUdpServer;
typedef void (*ReceiveEvent)(CUdpServer* pUdpServer, const char* szClientIP, unsigned int nClientPort, const char* pData, int len, void* pParam);

class CUdpServer : public CUdpSocket {
public:
	CUdpServer();

	bool Init(int nPort);

	bool Read(ReceiveEvent pEvent, void* pParam = nullptr);
	bool Send(const char* szIP, unsigned int nPort, const char* pSendData, unsigned int nLength);

protected:
	unsigned int m_nPort{};
};

class CUdpClient : public CUdpSocket {
public:
	CUdpClient();

	bool Init(const char* szIP, unsigned int nPort, int nReadTimeOut);

	bool Send(const char* pSendData, unsigned int nLength);
	bool Read(char* pReadBuf, unsigned int nReadLen);

protected:
	sockaddr_in m_ServerAddr;
	socklen_t m_nAddrLen;
};
