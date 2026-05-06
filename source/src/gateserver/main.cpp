// main.cpp : Defines the entry point for the console application.
//

#include "gateserver.h"
#include "log.h"
#include <signal.h>
#include "LicenseValidator.h"
#include "ErrorHandler.h"
_DBC_USING;

HANDLE hConsole = nullptr;
bool bExit = false;

// #pragma init_seg( lib )
pi_LeakReporter pi_leakReporter("gatememleak.log");
CResourceBundleManage g_ResourceBundleManage("Locale.loc"); // Add by lark.li 20080130

// Helper function to log security events to terminal and log file
void LogSecurityEvent(const char* ip, const char* action, const char* reason) {
	time_t now = time(nullptr);
	char timeStr[64];
	strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
	
	// Print to terminal
	printf("[Security] [%s] IP: %s | Action: %s | %s\n", timeStr, ip, action, reason);
	
	// Log to file
	LG("Security", "[%s] IP: %s | Action: %s | %s", timeStr, ip, action, reason);
}

void __cdecl Ctrlc_Dispatch(int sig) {
	if (!bExit) {
		C_PRINT("Shutting down GateServer...\n");
		g_exit = 1;
		bExit = TRUE;
	}
}

int main(int argc, char* argv[]) {
	// =============================================
	// LICENSE VALIDATION - Must be first!
	// =============================================
	if (!License::ValidateOrExit("license.lic", "GateServer")) {
		return 1;
	}
	// =============================================

	// Initialize crash dump generation
	ErrorHandler::Initialize();

	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	C_TITLE("GateServer.exe");
	C_PRINT("Loading GateServer.cfg...\n");

	T_B

	signal(SIGINT, Ctrlc_Dispatch);
#ifndef PKO_PLATFORM_WINDOWS
	signal(SIGPIPE, SIG_IGN);  // Prevent crash on write to closed socket
#endif

	GateServerApp app;
	app.ServiceStart();

	// Run server in main thread (blocking)
	g_gtsvr->RunLoop();
	app.ServiceStop();

	T_FINAL
	return 0;
}
