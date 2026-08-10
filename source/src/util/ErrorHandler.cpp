/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
   Cross-platform port: Linux signal handlers + backtrace
 * -------------------------------------------------------------------------- */

/**
 *  @file ErrorHandler.cpp
 *  Crash handling via BugTrap on Windows (https://github.com/bchavez/BugTrap);
 *  Linux keeps signal handlers + backtrace.
 */

#include "ErrorHandler.h"

#include <signal.h>	 // for signal()
#include <stdio.h>
#include <stdlib.h>
#include <sstream>

#include "log.h"

#if defined(_WIN32) || defined(_WIN64)

#include "ReturnValues.h"
#include "Stacktrace.h"

#include <windows.h>
#include <eh.h>
#include <crtdbg.h>
#include <DbgHelp.h>
#include <cstdint>

#include "BugTrap/BugTrap.h"

#pragma comment(lib, "DbgHelp.lib")
#ifdef _DEBUG
#pragma comment(lib, "BugTrapD-x64.lib")
#else
#pragma comment(lib, "BugTrap-x64.lib")
#endif

using namespace std;

bool ErrorHandler::s_nonInteractive = false;

void CALLBACK ErrorHandler::BugTrapPreErrHandler(INT_PTR /*nParam*/) {
	// Keep a lightweight breadcrumb next to existing game logs.
	std::string strfile;
	LG_GetDir(strfile);
	strfile += "\\exception.txt";

	FILE* fp = fopen(strfile.c_str(), "a+");
	if (!fp)
		return;

	SYSTEMTIME st;
	GetLocalTime(&st);
	fprintf(fp, "%02d-%02d %02d:%02d:%02d BugTrap crash handler invoked\n",
			st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	fclose(fp);
}

void ErrorHandler::Initialize(const char* appName, bool interactive) {
	if (!appName || !appName[0])
		appName = "Game";

	std::string reportDir;
	LG_GetDir(reportDir);
	if (reportDir.empty())
		reportDir = ".";
	reportDir += "\\crashes";
	CreateDirectoryA(reportDir.c_str(), nullptr);

	BT_SetAppName(appName);
	BT_SetFlags(BTF_DETAILEDMODE | BTF_LISTPROCESSES |
				(interactive ? (BTF_SCREENCAPTURE | BTF_SHOWADVANCEDUI) : BTF_NONE));
	BT_SetReportFilePath(reportDir.c_str());
	BT_SetActivityType(interactive ? BTA_SHOWUI : BTA_SAVEREPORT);
	BT_SetPreErrHandler(BugTrapPreErrHandler, 0);

	// Attach common log crumbs when present.
	std::string exceptionLog;
	LG_GetDir(exceptionLog);
	exceptionLog += "\\exception.txt";
	BT_AddLogFile(exceptionLog.c_str());

	BT_InstallSehFilter();
	BT_SetTerminate();
}

void ErrorHandler::DisableErrorDialogs() {
	s_nonInteractive = true;

	::SetErrorMode(SEM_NOGPFAULTERRORBOX);

	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);

	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);

	// Do not override set_terminate/abort — BugTrap installs those in Initialize().
	::_set_error_mode(_OUT_TO_STDERR);
}

LONG WINAPI ErrorHandler::UnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionPointers) {
	// Retained as a fallback if BugTrap is unavailable; prefer BT_InstallSehFilter path.
	RuntimeStack statck(pExceptionPointers);

	DWORD exceptionCode = 0;
	PVOID exceptionAddress = nullptr;
	if (pExceptionPointers && pExceptionPointers->ExceptionRecord) {
		exceptionCode = pExceptionPointers->ExceptionRecord->ExceptionCode;
		exceptionAddress = pExceptionPointers->ExceptionRecord->ExceptionAddress;
	}

	std::string exceptionName = SEHTranslator::name(exceptionCode);
	std::string exceptionDesc = SEHTranslator::description(exceptionCode);

	char moduleNameBuf[MAX_PATH] = {0};
	HMODULE hModule = nullptr;
	if (exceptionAddress &&
		GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
						   (LPCSTR)exceptionAddress, &hModule)) {
		GetModuleFileNameA(hModule, moduleNameBuf, MAX_PATH);
	}

	std::stringstream text;
	text << "UnhandledException" << std::endl;
	text << "  Exception: " << exceptionName << " (0x" << std::hex << exceptionCode << ")" << std::endl;
	text << "  Description: " << exceptionDesc << std::endl;
	text << "  Address: 0x" << std::hex << (uintptr_t)exceptionAddress << std::dec << std::endl;
	if (moduleNameBuf[0]) {
		text << "  Module: " << moduleNameBuf << std::endl;
	}
	text << "  Stack Trace:" << std::endl;
	text << statck << std::endl;
	std::string mText = text.str();

	std::string strfile;
	LG_GetDir(strfile);
	strfile += "\\exception.txt";
	FILE* fp = fopen(strfile.c_str(), "a+");
	if (fp) {
		SYSTEMTIME st;
		char tim[100] = {0};
		GetLocalTime(&st);
		sprintf(tim, "%02d-%02d %02d:%02d:%02d", st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		fwrite(tim, strlen(tim), 1, fp);
		fwrite(mText.c_str(), strlen(mText.c_str()) - 1, 1, fp);
		fclose(fp);
	}

	std::string dumpfile;
	LG_GetDir(dumpfile);
	SYSTEMTIME st;
	GetLocalTime(&st);
	char dumpname[256] = {0};
	sprintf(dumpname, "\\crash_%04d%02d%02d_%02d%02d%02d.dmp", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute,
			st.wSecond);
	dumpfile += dumpname;
	CreateMiniDump(pExceptionPointers, dumpfile.c_str());

	return EXCEPTION_CONTINUE_SEARCH;
}

void ErrorHandler::CreateMiniDump(EXCEPTION_POINTERS* pExceptionPointers, const char* dumpFilePath) {
	HANDLE hFile = CreateFileA(dumpFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	MINIDUMP_EXCEPTION_INFORMATION mdei;
	mdei.ThreadId = GetCurrentThreadId();
	mdei.ExceptionPointers = pExceptionPointers;
	mdei.ClientPointers = FALSE;

	MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithHandleData | MiniDumpWithThreadInfo |
														MiniDumpWithUnloadedModules);

	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, dumpType, pExceptionPointers ? &mdei : NULL,
					  NULL, NULL);
	CloseHandle(hFile);
}

void ErrorHandler::TerminateFunction() {
	DisplayError(L"Premature shutdown.  terminate() was called.");
	::ExitProcess(WINUNIT_EXIT_UNHANDLED_EXCEPTION);
}

void ErrorHandler::AbortFunction(int /* signal */) {
	DisplayError(L"Premature shutdown.  abort() was called.");
	::ExitProcess(WINUNIT_EXIT_UNHANDLED_EXCEPTION);
}

void ErrorHandler::DisplayError(const wchar_t* /*errorMessage*/, const wchar_t* /*details*/) {}

#else // Linux implementation

#include <execinfo.h>   // backtrace(), backtrace_symbols()
#include <cxxabi.h>     // __cxa_demangle
#include <unistd.h>
#include <cstring>
#include <ctime>
#include <sys/resource.h>

using namespace std;

bool ErrorHandler::s_nonInteractive = false;

/// Install signal handlers for crash signals
void ErrorHandler::Initialize(const char* /*appName*/, bool /*interactive*/) {
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = SignalHandler;
	sa.sa_flags = SA_SIGINFO | SA_RESETHAND; // Reset after first signal to allow core dump

	sigaction(SIGSEGV, &sa, nullptr);   // Segmentation fault
	sigaction(SIGBUS,  &sa, nullptr);   // Bus error
	sigaction(SIGFPE,  &sa, nullptr);   // Floating point exception
	sigaction(SIGILL,  &sa, nullptr);   // Illegal instruction
	sigaction(SIGABRT, &sa, nullptr);   // Abort

	// Enable core dumps for post-mortem debugging
	struct rlimit core_limit;
	core_limit.rlim_cur = RLIM_INFINITY;
	core_limit.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &core_limit);
}

void ErrorHandler::DisableErrorDialogs() {
	s_nonInteractive = true;
	// On Linux, there are no error dialogs to disable.
	// set_terminate and signal handlers are set via Initialize().
	std::set_terminate(TerminateFunction);
	signal(SIGABRT, AbortFunction);
}

/// Signal handler — write backtrace then re-raise to generate core dump
void ErrorHandler::SignalHandler(int signo, siginfo_t* info, void* /*context*/) {
	const char* signame = "UNKNOWN";
	switch (signo) {
		case SIGSEGV: signame = "SIGSEGV (Segmentation fault)"; break;
		case SIGBUS:  signame = "SIGBUS (Bus error)"; break;
		case SIGFPE:  signame = "SIGFPE (Floating point exception)"; break;
		case SIGILL:  signame = "SIGILL (Illegal instruction)"; break;
		case SIGABRT: signame = "SIGABRT (Abort)"; break;
	}

	// Write backtrace to file
	WriteBacktrace(signame);

	// Log fault address if available
	if (info && (signo == SIGSEGV || signo == SIGBUS)) {
		fprintf(stderr, "Signal %s at address %p\n", signame, info->si_addr);
	} else {
		fprintf(stderr, "Signal %s received\n", signame);
	}

	// Re-raise signal to generate core dump (SA_RESETHAND ensures default handler)
	raise(signo);
}

/// Write a backtrace to the exception log file
void ErrorHandler::WriteBacktrace(const char* signalName) {
	// Get backtrace
	void* frames[128];
	int nframes = backtrace(frames, 128);
	char** symbols = backtrace_symbols(frames, nframes);

	// Build log path
	std::string strfile;
	LG_GetDir(strfile);
	strfile += "/exception.txt";

	FILE* fp = fopen(strfile.c_str(), "a+");
	if (fp) {
		// Timestamp
		time_t now = time(nullptr);
		struct tm tm_result;
		localtime_r(&now, &tm_result);
		char tim[100] = {0};
		snprintf(tim, sizeof(tim), "%02d-%02d %02d:%02d:%02d",
				 tm_result.tm_mon + 1, tm_result.tm_mday,
				 tm_result.tm_hour, tm_result.tm_min, tm_result.tm_sec);

		fprintf(fp, "\n%s Crash: %s (PID: %d)\n", tim, signalName, (int)getpid());
		fprintf(fp, "Backtrace (%d frames):\n", nframes);

		for (int i = 0; i < nframes; ++i) {
			// Try to demangle C++ names
			if (symbols && symbols[i]) {
				// Symbol format: "module(function+offset) [address]"
				// Try to extract and demangle the function name
				char* mangled = nullptr;
				char* offset_begin = nullptr;
				char* offset_end = nullptr;

				for (char* p = symbols[i]; *p; ++p) {
					if (*p == '(') mangled = p + 1;
					else if (*p == '+') offset_begin = p;
					else if (*p == ')') { offset_end = p; break; }
				}

				if (mangled && offset_begin && offset_end && mangled < offset_begin) {
					*offset_begin = '\0';
					int status;
					char* demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
					if (status == 0 && demangled) {
						fprintf(fp, "  #%d  %s\n", i, demangled);
						free(demangled);
					} else {
						*offset_begin = '+'; // Restore
						fprintf(fp, "  #%d  %s\n", i, symbols[i]);
					}
				} else {
					fprintf(fp, "  #%d  %s\n", i, symbols[i]);
				}
			} else {
				fprintf(fp, "  #%d  %p\n", i, frames[i]);
			}
		}

		fprintf(fp, "--- End of backtrace ---\n\n");
		fclose(fp);
	}

	if (symbols) free(symbols);

	// Also dump to stderr for immediate visibility
	fprintf(stderr, "\n=== CRASH: %s (PID: %d) ===\n", signalName, (int)getpid());
	backtrace_symbols_fd(frames, nframes, STDERR_FILENO);
	fprintf(stderr, "=== End backtrace ===\n");
}

void ErrorHandler::TerminateFunction() {
	DisplayError(L"Premature shutdown.  terminate() was called.");
	_exit(1);
}

void ErrorHandler::AbortFunction(int /* signal */) {
	DisplayError(L"Premature shutdown.  abort() was called.");
	_exit(1);
}

void ErrorHandler::DisplayError(const wchar_t* errorMessage,
								const wchar_t* details /* L"" */) {
	if (errorMessage) {
		fprintf(stderr, "[ErrorHandler] %ls %ls\n", errorMessage, details ? details : L"");
	}
}

#endif // _WIN32
