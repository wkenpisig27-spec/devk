/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */

/**
 *  @file ErrorHandler.h
 *  Application-wide crash handling via BugTrap on Windows
 *  (replaces basic SEH/minidump-only path); Linux keeps signal handlers.
 */

#pragma once

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include "serversdk/platform_compat.h"
#endif

/// This class contains static functions used for system-wide error handling.
class ErrorHandler {
	typedef void (*SignalHandlerPointer)(int);

public:
	/// Install crash reporting.
	/// @param appName Display name in BugTrap reports (e.g. "GameClient", "GameServer").
	/// @param interactive If true, show BugTrap UI (client). If false, auto-save report (servers).
	///                    Ignored on Linux.
	static void Initialize(const char* appName = "Game", bool interactive = true);

	/// Quiet CRT assert/abort dialogs (does not disable BugTrap UI on unhandled crashes).
	static void DisableErrorDialogs();

private:
	/// This is for use by the unhandled exception filter--if false, the filter
	/// allows the crash dialog to go up after printing an error message.
	static bool s_nonInteractive;

#if defined(_WIN32) || defined(_WIN64)
	static void CALLBACK BugTrapPreErrHandler(INT_PTR nParam);

	// Legacy helpers kept for optional diagnostics / CRT terminate paths
	static LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionPointers);
	static void CreateMiniDump(EXCEPTION_POINTERS* pExceptionPointers, const char* dumpFilePath);
#else
	// Linux signal handler for crashes
	static void SignalHandler(int signo, siginfo_t* info, void* context);

	// Write a backtrace to the exception log
	static void WriteBacktrace(const char* signalName);
#endif

	// The function that replaces terminate().
	static void TerminateFunction();

	// The function that is called when abort() is called.
	static void AbortFunction(int /* signal */);

	// Called by the other error handlers to display the error message.
	static void DisplayError(const wchar_t* errorMessage, const wchar_t* details = L"");

private:
	~ErrorHandler(void);
};
