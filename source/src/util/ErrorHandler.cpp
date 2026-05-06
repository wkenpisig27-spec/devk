/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
   Cross-platform port: Linux signal handlers + backtrace
 * -------------------------------------------------------------------------- */

/**
 *  @file ErrorHandler.cpp
 *  The implementation file for the application-wide error handling functions
 *  used by WinUnit.exe.  These have been put in a separate class to avoid
 *  cluttering up main (and so the dependencies can be more easily removed
 *  if desired).
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

#include <windows.h> // for SetUnhandledExceptionFilter
#include <eh.h>		 // for set_terminate
#include <crtdbg.h>	 // for _CrtSetReport*
#include <DbgHelp.h> // for MiniDumpWriteDump

#pragma comment(lib, "DbgHelp.lib")

using namespace std;

/// This function sets up a process-wide unhandled exception handler.
void ErrorHandler::Initialize() {
	::SetUnhandledExceptionFilter(UnhandledExceptionFilter);
}

void ErrorHandler::DisableErrorDialogs() {
	s_nonInteractive = true;

	/* DWORD oldProcessErrorMode = */ ::SetErrorMode(SEM_NOGPFAULTERRORBOX);

	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);

	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);

	::set_terminate(TerminateFunction);
	signal(SIGABRT, AbortFunction);
	::_set_error_mode(_OUT_TO_STDERR);
}

LONG WINAPI ErrorHandler::UnhandledExceptionFilter(
	EXCEPTION_POINTERS* pExceptionPointers)
{
	RuntimeStack statck(pExceptionPointers);

	std::stringstream text;
	text << "UnhandledException" << std::endl
		 << statck << std::endl;
	std::string mText = text.str();

	std::string strfile;
	LG_GetDir(strfile);
	strfile += "\\exception.txt";
	FILE* fp = fopen(strfile.c_str(), "a+");

	if (fp) {
		SYSTEMTIME st;
		char tim[100] = {0};
		GetLocalTime(&st);
		sprintf(tim, "%02d-%02d %02d:%02d:%02d", st.wMonth, st.wDay, st.wHour,
				st.wMinute, st.wSecond);

		fwrite(tim, strlen(tim), 1, fp);
		fwrite(mText.c_str(), strlen(mText.c_str()) - 1, 1, fp);
		fclose(fp);
	}

	// Create minidump file for detailed crash analysis
	std::string dumpfile;
	LG_GetDir(dumpfile);
	SYSTEMTIME st;
	GetLocalTime(&st);
	char dumpname[256] = {0};
	sprintf(dumpname, "\\crash_%04d%02d%02d_%02d%02d%02d.dmp", 
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	dumpfile += dumpname;
	
	CreateMiniDump(pExceptionPointers, dumpfile.c_str());

	return EXCEPTION_CONTINUE_SEARCH;
}

void ErrorHandler::CreateMiniDump(EXCEPTION_POINTERS* pExceptionPointers, const char* dumpFilePath) {
	HANDLE hFile = CreateFileA(
		dumpFilePath,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (hFile == INVALID_HANDLE_VALUE) {
		return;
	}

	MINIDUMP_EXCEPTION_INFORMATION mdei;
	mdei.ThreadId = GetCurrentThreadId();
	mdei.ExceptionPointers = pExceptionPointers;
	mdei.ClientPointers = FALSE;

	MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(
		MiniDumpNormal |
		MiniDumpWithHandleData |
		MiniDumpWithThreadInfo |
		MiniDumpWithUnloadedModules
	);

	BOOL success = MiniDumpWriteDump(
		GetCurrentProcess(),
		GetCurrentProcessId(),
		hFile,
		dumpType,
		pExceptionPointers ? &mdei : NULL,
		NULL,
		NULL
	);

	CloseHandle(hFile);

	if (success) {
		FILE* fp = fopen("minidump_status.log", "a+");
		if (fp) {
			fprintf(fp, "Minidump created: %s\n", dumpFilePath);
			fclose(fp);
		}
	}
}

void ErrorHandler::TerminateFunction() {
	DisplayError(L"Premature shutdown.  terminate() was called.");
	::ExitProcess(WINUNIT_EXIT_UNHANDLED_EXCEPTION);
}

void ErrorHandler::AbortFunction(int /* signal */) {
	DisplayError(L"Premature shutdown.  abort() was called.");
	::ExitProcess(WINUNIT_EXIT_UNHANDLED_EXCEPTION);
}

void ErrorHandler::DisplayError(const wchar_t* errorMessage,
								const wchar_t* details /* L"" */) {
	// Logging removed — was originally logger-based
	(void)errorMessage;
	(void)details;
}

#else // Linux implementation

#include <execinfo.h>   // backtrace(), backtrace_symbols()
#include <cxxabi.h>     // __cxa_demangle
#include <unistd.h>
#include <cstring>
#include <ctime>
#include <sys/resource.h>

using namespace std;

/// Install signal handlers for crash signals
void ErrorHandler::Initialize() {
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

bool ErrorHandler::s_nonInteractive = false;
