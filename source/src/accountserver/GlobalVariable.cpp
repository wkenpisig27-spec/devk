#define _CRTDBG_MAP_ALLOC
#include "stdafx.h"
#include <stdlib.h>
#include <crtdbg.h>

// #include "AccountServer2.h"
#include <string>
#include "MyThread.h"
#include "GlobalVariable.h"

// 锟斤拷锟斤拷锟斤拷锟饺讹拷锟斤拷锟斤拷锟斤拷锟侥硷拷锟街凤拷锟斤拷
std::string g_strCfgFile = "AccountServer.cfg";

dbc::TLSIndex g_TlsIndex;
CDataBaseCtrl g_MainDBHandle;
DWORD g_MainThreadID;
HWND g_hMainWnd = nullptr;
