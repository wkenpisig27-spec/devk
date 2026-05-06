// ������,����Ŀ�ṹ������������,�жദ�ط�Ƿȱ�߳�ͬ������,������д - by Arcol

// main.cpp : Defines the entry point for the console application.
//

#ifdef PKO_PLATFORM_WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include "stdafx.h"
#ifdef PKO_PLATFORM_WINDOWS
#include "resource.h"
#endif
#include "AccountServer2.h"
#include <signal.h>
#ifdef PKO_PLATFORM_WINDOWS
#include <CommCtrl.h>
#endif
#include "GlobalVariable.h"

#include "inifile.h"
#include "LicenseValidator.h"
#include "ErrorHandler.h"

#define AUTHUPDATE_TIMER 1
#define GROUPUPDATE_TIMER 2

ThreadPool* comm = nullptr;

#ifndef PKO_PLATFORM_WINDOWS
// On Linux, use g_exit flag instead of Windows message queue for shutdown
static volatile int g_exit = 0;
#endif
ThreadPool* proc = nullptr;
AuthThreadPool atp;
bool bExit = false;

// #pragma init_seg( lib )
pi_LeakReporter pi_leakReporter("accountmemleak.log");
CResourceBundleManage g_ResourceBundleManage("Locale.loc"); // Add by lark.li 20080330


void __cdecl Ctrlc_Dispatch(int sig) {
	if (!bExit) {
		C_PRINT("Notify Logout to AccountServer...\n");
#ifdef PKO_PLATFORM_WINDOWS
		PostMessage(g_hMainWnd, WM_CLOSE, 0, 0);
#else
		g_exit = 1;
#endif
		bExit = TRUE;
	}
}
#ifdef PKO_PLATFORM_WINDOWS
LRESULT OnInitDialog(HWND hwnd) {
	// SetWindowText(hwnd, "AccountServer");
	SetWindowText(hwnd, "AccountServer");

	// ��֤
	SetDlgItemText(hwnd, IDC_TOP, "Verification");
	// ���а��� 0
	SetDlgItemText(hwnd, IDC_QUEUECAP, "Party packet: 0");
	// �������� 0
	SetDlgItemText(hwnd, IDC_TASKCNT, "Concurrent number: 0");
	// GroupServer
	SetDlgItemText(hwnd, IDC_MID, "GroupServer");
	// Exit
	SetDlgItemText(hwnd, IDOK, "Exit");
	// Auth thread report list
	{
		HWND hAuthList = GetDlgItem(hwnd, IDC_AUTHLIST);

		DWORD dwStyle = ListView_GetExtendedListViewStyle(hAuthList);
		ListView_SetExtendedListViewStyle(hAuthList,
										  dwStyle | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

		LVCOLUMN lv;
		lv.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;

		lv.cx = 50;
		lv.pszText = const_cast<LPSTR>("Thread");
		ListView_InsertColumn(hAuthList, 0, &lv);

		lv.cx = 100;
		lv.pszText = const_cast<LPSTR>("Run Label");
		lv.fmt = LVCFMT_CENTER;
		ListView_InsertColumn(hAuthList, 1, &lv);

		lv.cx = 150;
		lv.pszText = const_cast<LPSTR>("Last/Max Consume (ms)");
		lv.fmt = LVCFMT_CENTER;
		ListView_InsertColumn(hAuthList, 2, &lv);

		LVITEM item;
		item.mask = LVIF_TEXT;
		item.iSubItem = 0;
		char buf[80] = {0};
		for (char i = 0; i < AuthThreadPool::AT_MAXNUM; ++i) {
			item.iItem = i;
			sprintf(buf, "#%02d", i + 1);
			item.pszText = (LPSTR)buf;
			ListView_InsertItem(hAuthList, &item);
		}
	}

	// Group report list
	{
		HWND hGroupList = GetDlgItem(hwnd, IDC_GROUPLIST);

		DWORD dwStyle = ListView_GetExtendedListViewStyle(hGroupList);
		ListView_SetExtendedListViewStyle(hGroupList,
										  dwStyle | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

		LVCOLUMN lv;
		lv.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;

		lv.cx = 100;
		lv.pszText = const_cast<LPSTR>("Name");
		ListView_InsertColumn(hGroupList, 0, &lv);

		lv.cx = 100;
		lv.pszText = const_cast<LPSTR>("Group IP");
		lv.fmt = LVCFMT_CENTER;
		ListView_InsertColumn(hGroupList, 1, &lv);

		lv.cx = 100;
		lv.pszText = const_cast<LPSTR>("Status");
		lv.fmt = LVCFMT_CENTER;
		ListView_InsertColumn(hGroupList, 2, &lv);
	}

	// Update Timer
	SetTimer(hwnd, AUTHUPDATE_TIMER, 1000, nullptr);
	SetTimer(hwnd, GROUPUPDATE_TIMER, 3000, nullptr);

	return 0;
}
void ClearGroupList() {
	HWND hGroupList = GetDlgItem(g_hMainWnd, IDC_GROUPLIST);
	ListView_DeleteAllItems(hGroupList);
}
BOOL AddGroupToList(char const* strName, char const* strAddr, char const* strStatus) {
	HWND hGroupList = GetDlgItem(g_hMainWnd, IDC_GROUPLIST);

	LVITEM item;
	item.mask = LVIF_TEXT;
	item.iItem = 0;
	item.iSubItem = 0;
	item.pszText = (LPSTR)strName;
	int index = ListView_InsertItem(hGroupList, &item);
	ListView_SetItemText(hGroupList, index, 1, (LPSTR)strAddr);
	ListView_SetItemText(hGroupList, index, 2, (LPSTR)strStatus);

	return TRUE;
}
LRESULT OnTimer(HWND hwnd, UINT idEvent) {
	if (idEvent == AUTHUPDATE_TIMER) {
		LVITEM item;
		item.mask = LVIF_TEXT;
		char buf[80] = {0};
		HWND hAuthList = GetDlgItem(hwnd, IDC_AUTHLIST);
		for (char i = 0; i < AuthThreadPool::AT_MAXNUM; ++i) {
			item.iItem = i;

			item.iSubItem = 1;
			sprintf(buf, "%02d", AuthThreadPool::RunLabel[i]);
			item.pszText = (LPSTR)buf;
			ListView_SetItem(hAuthList, &item);

			item.iSubItem = 2;
			sprintf(buf, "%04d/%04d", AuthThreadPool::RunLast[i], AuthThreadPool::RunConsume[i]);
			item.pszText = (LPSTR)buf;
			ListView_SetItem(hAuthList, &item);
		}

		HWND hQueueCap = GetDlgItem(hwnd, IDC_QUEUECAP);
		// sprintf(buf, "���а��� %d", g_Auth.GetPkTotal());
		sprintf(buf, RES_STRING(AS_MAIN_CPP_00024), g_Auth.GetPkTotal());
		SetWindowText(hQueueCap, (LPCTSTR)buf);

		HWND hTaskCnt = GetDlgItem(hwnd, IDC_TASKCNT);
		// sprintf(buf, "�������� %d", proc->GetTaskCount());
		sprintf(buf, RES_STRING(AS_MAIN_CPP_00025), proc->GetTaskCount());
		SetWindowText(hTaskCnt, (LPCTSTR)buf);
	} else if (idEvent == GROUPUPDATE_TIMER) {
		g_As2->DisplayGroup();
	}

	return 0;
}
INT_PTR CALLBACK MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	WORD wNotify = HIWORD(wParam);
	WORD wID = LOWORD(wParam);

	switch (uMsg) {
	case WM_INITDIALOG:
		OnInitDialog(hwndDlg);
		break;

	case WM_KEYDOWN:
		break;

	case WM_TIMER:
		OnTimer(hwndDlg, (UINT)wParam);
		break;

	case WM_COMMAND:
		if (wNotify == BN_CLICKED) {
			if (wID == IDOK) {
				PostMessage(hwndDlg, WM_CLOSE, 0, 0);
			}
		}
		break;

	case WM_CLOSE:
		KillTimer(hwndDlg, GROUPUPDATE_TIMER);
		KillTimer(hwndDlg, AUTHUPDATE_TIMER);
		PostQuitMessage(0);
		break;

	case WM_USER_LOG: {
		sUserLog* pUserLog = (sUserLog*)lParam;
		if (pUserLog->bLogin) {
			g_MainDBHandle.UserLogin(pUserLog->nUserID, pUserLog->strUserName, pUserLog->strLoginIP);
		} else {
			g_MainDBHandle.UserLogout(pUserLog->nUserID);
		}
		delete pUserLog;
		break;
	}
	case WM_USER_LOG_MAP: {
		sUserLog* pUserLog = (sUserLog*)lParam;
		if (pUserLog->bLogin) {
			g_MainDBHandle.UserLoginMap(pUserLog->strUserName, "Enter_Map");
		} else {
			g_MainDBHandle.UserLogoutMap(pUserLog->strUserName);
		}
		delete pUserLog;
		break;
	}
	}
	return FALSE;
}
HWND CreateMainDialog() {
	HINSTANCE hInst = GetModuleHandle(nullptr);
	HWND hwnd = ::CreateDialog(hInst, MAKEINTRESOURCE(IDD_MAINDIALOG), nullptr, MainDlgProc);
	::ShowWindow(hwnd, SW_SHOW);
	return hwnd;
}
#endif // PKO_PLATFORM_WINDOWS

// #include <string.h>

HANDLE hConsole = nullptr;

int main(int argc, char* argv[]) {
	// =============================================
	// LICENSE VALIDATION - Must be first!
	// =============================================
	if (!License::ValidateOrExit("license.lic", "AccountServer")) {
		return 1;
	}
	// =============================================

	// Initialize crash dump generation
	ErrorHandler::Initialize();

	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	C_TITLE("AccountServer.exe");
	C_PRINT("Loading AccountServer.cfg...\n");

#ifdef PKO_PLATFORM_WINDOWS
	SEHTranslator translator;
#endif

	T_B

		// ����������
#ifdef PKO_PLATFORM_WINDOWS
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); // Add by Arcol (2005-12-2)
#endif

#ifdef PKO_PLATFORM_WINDOWS
	g_hMainWnd = CreateMainDialog();
#endif
	signal(SIGINT, Ctrlc_Dispatch);
#ifndef PKO_PLATFORM_WINDOWS
	signal(SIGPIPE, SIG_IGN);  // Prevent crash on write to closed socket
#endif
	g_MainThreadID = GetCurrentThreadId();

	if (!g_MainDBHandle.CreateObject()) {
		C_PRINT("failed\n");
		printf("Connection to database failed, the process terminated! \n");
		system("pause");
		return -1;
	}
	atp.Launch();

	// Initialize new authentication components
	InitializeAuthSystem();

	// ��ʼ������
	TcpCommApp::WSAStartup();
	comm = ThreadPool::CreatePool(10, 10, 256);

	int nProcCnt = 2 * AuthThreadPool::AT_MAXNUM;
	proc = ThreadPool::CreatePool(nProcCnt, nProcCnt, 2048);
	try {
		g_As2 = new AccountServer2(proc, comm);
	} catch (std::exception&e) {
		printf("%s\n", e.what());
		comm->DestroyPool();
		TcpCommApp::WSACleanup();
		Sleep(10 * 1000);
		return -1;
	} catch (...) {
		// printf("AccountServer��ʼ���ڼ䷢��δ֪������֪ͨ������\n");
		printf(RES_STRING(AS_MAIN_CPP_00006));
		comm->DestroyPool();
		TcpCommApp::WSACleanup();
		Sleep(10 * 1000);
		return -2;
	}

	// ��Ϣѭ��
#ifdef PKO_PLATFORM_WINDOWS
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#else
	// Linux: simple blocking loop until exit signal
	C_PRINT("AccountServer running. Type 'exit' or press Ctrl+C to stop.\n");
	while (!bExit && !g_exit) {
		Sleep(100);
	}
#endif

	// ж������
	delete g_As2;
	if (comm != nullptr)
		comm->DestroyPool();
	if (proc != nullptr)
		proc->DestroyPool();
	TcpCommApp::WSACleanup();

	// Auth thread exit
	atp.NotifyToExit();
	atp.WaitForExit();

	// Log run status
	LG("RunLabel", "\n");
	for (char i = 0; i < AuthThreadPool::AT_MAXNUM; ++i) {
		LG("RunLabel", "%02d %04d\n", AuthThreadPool::RunLabel[i],
		   AuthThreadPool::RunConsume[i]);
	}
	LG("RunLabel", "\n");

	g_MainDBHandle.ReleaseObject();

	T_FINAL

	return 0;
}
