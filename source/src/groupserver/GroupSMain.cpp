// GroupServerApp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "GroupServerApp.h"
#include "LicenseValidator.h"
#include "ErrorHandler.h"
#include <signal.h>

HANDLE hConsole = nullptr;

// 初始化顺序
#ifdef _MSC_VER
#pragma init_seg(lib)
#endif
pi_LeakReporter pi_leakReporter("groupememleak.log");
CResourceBundleManage g_ResourceBundleManage("Locale.loc"); // Add by alfred.shi 20080130

// binary compatible
int _tmain(int argc, _TCHAR* argv[]) {
	// =============================================
	// LICENSE VALIDATION - Must be first!
	// =============================================
	if (!License::ValidateOrExit("license.lic", "GroupServer")) {
		return 1;
	}
	// =============================================

	// Initialize crash dump generation
	ErrorHandler::Initialize();

	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	C_TITLE("GroupServer.exe");
	C_PRINT("Loading GroupServer.cfg...\n");

	// SEHTranslator translator;

	T_B

#ifndef PKO_PLATFORM_WINDOWS
	signal(SIGPIPE, SIG_IGN);  // Prevent crash on write to closed socket
#endif

	TcpCommApp::WSAStartup();
	ThreadPool* l_proc = ThreadPool::CreatePool(16, 32, 4096);
	ThreadPool* l_comm = ThreadPool::CreatePool(4, 8, 512, THREAD_PRIORITY_ABOVE_NORMAL);

	try {
		g_gpsvr = new GroupServerApp(l_proc, l_comm);
	} catch (...) {
		l_comm->DestroyPool();
		l_proc->DestroyPool();
		TcpCommApp::WSACleanup();
		Sleep(10 * 1000);
		return -1;
	}
	while (!g_exit) {
		std::string str;
		str.reserve(256);

		std::cout << RES_STRING(GP_MAIN_CPP_00001);
		std::cin >> str;
		if (str == "exit" || g_exit) {
			std::cout << RES_STRING(GP_MAIN_CPP_00002) << std::endl;
			break;
		} else if (str == "logbak") {
			// LogStream removed - backup functionality no longer available
			std::cout << "Log backup not available" << std::endl;
		} else if (str == "maintenance" || str == "maint") {
			// Toggle maintenance mode
			bool newState = !g_gpsvr->IsMaintenanceMode();
			g_gpsvr->SetMaintenanceMode(newState, true);
		} else if (str == "maint_on") {
			// Enable maintenance mode and kick non-GMs
			g_gpsvr->SetMaintenanceMode(true, true);
		} else if (str == "maint_off") {
			// Disable maintenance mode
			g_gpsvr->SetMaintenanceMode(false, false);
		} else if (str == "kicknongm") {
			// Kick all non-GM players without enabling maintenance
			g_gpsvr->KickAllNonGMPlayers("Server administrator action");
		} else if (str == "status") {
			// Show server status
			std::cout << "=== Server Status ===" << std::endl;
			std::cout << "Maintenance Mode: " << (g_gpsvr->IsMaintenanceMode() ? "ENABLED" : "DISABLED") << std::endl;
			std::cout << "Online Players: " << g_gpsvr->m_plylst.GetTotal() << std::endl;
		} else if (str == "help") {
			// Show available commands
			std::cout << "=== Available Commands ===" << std::endl;
			std::cout << "  exit       - Shutdown server" << std::endl;
			std::cout << "  status     - Show server status" << std::endl;
			std::cout << "  maintenance/maint - Toggle maintenance mode" << std::endl;
			std::cout << "  maint_on   - Enable maintenance mode (kicks non-GMs)" << std::endl;
			std::cout << "  maint_off  - Disable maintenance mode" << std::endl;
			std::cout << "  kicknongm  - Kick all non-GM players" << std::endl;
			std::cout << "  help       - Show this help" << std::endl;
		} else {
			std::cout << RES_STRING(GP_MAIN_CPP_00003) << std::endl;
			std::cout << "Type 'help' for available commands" << std::endl;
		}
	}
	// if(!g_exit)
	{
		g_exit = 1;
		while (g_ref) {
			Sleep(1);
		}
		delete g_gpsvr;

		l_comm->DestroyPool();
		l_proc->DestroyPool();
		TcpCommApp::WSACleanup();
		g_exit = 2;
		Sleep(2000);
	}
	while (g_exit != 2) {
		Sleep(1);
	}

	T_FINAL
	return 0;
}
