#pragma once

#ifndef _GLOBALVARIABLE_H_
#define _GLOBALVARIABLE_H_

#include "tlsindex.h"
#include "databasectrl.h"

#include "i18n.h" //Add by lark.li 20080307

// Add by lark.li 20080730 begin
#include "pi_Alloc.h"
#include "pi_Memory.h"
// End

extern std::string g_strCfgFile;

extern dbc::TLSIndex g_TlsIndex;
extern CDataBaseCtrl g_MainDBHandle;
#ifdef PKO_PLATFORM_WINDOWS
extern HWND g_hMainWnd;
#endif
extern DWORD g_MainThreadID;

#ifdef PKO_PLATFORM_WINDOWS
#define WM_USER_LOG WM_USER + 100
#define WM_USER_LOG_MAP WM_USER + 101
#else
#define WM_USER_LOG 0x0464
#define WM_USER_LOG_MAP 0x0465
#endif

struct sUserLog {
	bool bLogin; // true:login  false:logout
	int nUserID;
	std::string strUserName;
	std::string strPassport;
	std::string strLoginIP;
};

extern HANDLE hConsole;

#ifdef PKO_PLATFORM_WINDOWS
#define C_PRINT(s, ...)                    \
	SetConsoleTextAttribute(hConsole, 14); \
	printf(s, __VA_ARGS__);                \
	SetConsoleTextAttribute(hConsole, 10);

#define C_TITLE(s)                                                             \
	char szPID[32];                                                            \
	_snprintf_s(szPID, sizeof(szPID), _TRUNCATE, "%d", GetCurrentProcessId()); \
	std::string strConsoleT;                                                   \
	strConsoleT += "[PID:";                                                    \
	strConsoleT += szPID;                                                      \
	strConsoleT += "]";                                                        \
	strConsoleT += s;                                                          \
	SetConsoleTitle(strConsoleT.c_str());
#else
#define C_PRINT(s, ...) printf(s, ##__VA_ARGS__)
#define C_TITLE(s) do { printf("[PID:%d] %s\n", (int)getpid(), s); } while(0)
#endif

#endif