/*
 Copyright (c) 2001
 Author: Konstantin Boukreev
 E-mail: konstantin@mail.primorye.ru
 Created: 28.08.2001 18:38:49
 Version: 1.0.0

 Permission to use, copy, modify, distribute and sell this software
 and its documentation for any purpose is hereby granted without fee,
 provided that the above copyright notice appear in all copies and
 that both that copyright notice and this permission notice appear
 in supporting documentation.  Konstantin Boukreev makes no representations
 about the suitability of this software for any purpose.
 It is provided "as is" without express or implied warranty.

 Common header file
 based on CmdHdr.h by Jeffrey Richter
*/

#ifndef _kbase_8998f07c_472c_4ff3_90b6_ffd5738d85e6
#define _kbase_8998f07c_472c_4ff3_90b6_ffd5738d85e6

#if defined(_MSC_VER) && _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#if defined(_WIN32) || defined(_WIN64)

#ifndef STRICT
#define STRICT
#endif

// Exclude rarely-used stuff from Windows headers
// #define WIN32_LEAN_AND_MEAN
// #define VC_EXTRALEAN

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif // _WIN32_WINNT
#ifndef WINVER
#define WINVER 0x0500
#endif // WINVER
#ifndef _WIN32_IE
#define _WIN32_IE 0x0500
#endif // _WIN32_IE

#ifndef _M_IX86
// #define UNICODE
#endif
#ifdef UNICODE
#define _UNICODE
#endif

#endif // _WIN32

// #pragma warning(push, 3)
#ifdef _MSC_VER
#pragma warning(push, 4)
#endif
#if defined(_WIN32) || defined(_WIN64)
#include <tchar.h>
#include <windows.h>
#if (WINVER < 0x0500)
#include <winable.h>
#endif // (WINVER < 0x0500)
#else
#include "serversdk/platform_compat.h"
#include <cstdarg>
#include <cstdio>
#endif
// #pragma warning(pop)
// #pragma warning(push, 4)

#ifndef WT_EXECUTEINPERSISTENTIOTHREAD
#if defined(_WIN32) || defined(_WIN64)
#pragma message("You are not using the latest Platform SDK header/library ")
#pragma message("files. This may prevent the project from building correctly.")
#pragma message("You may install the Platform SDK from http://msdn.microsoft.com/downloads/")
#endif
#endif

#ifdef _DEBUG
#ifdef _MSC_VER
#pragma warning(disable : 4127) // conditional expression is constant
#endif
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4786) // disable "identifier was truncated to 'number' characters in the debug information"
#pragma warning(disable : 4290) // C++ Exception Specification ignored
#pragma warning(disable : 4097) // typedef-name 'identifier1' used as synonym for class-name 'identifier2'
#pragma warning(disable : 4001) // nonstandard extension 'single line comment' was used
#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(disable : 4699) // Note: Creating precompiled header
#pragma warning(disable : 4710) // function not inlined
#pragma warning(disable : 4514) // unreferenced inline function has been removed
#pragma warning(disable : 4512) // assignment operator could not be generated
#pragma warning(disable : 4310) // cast truncates constant value
#endif

// Linux-specific compatibility
#if !defined(_WIN32) && !defined(_WIN64)
#include <csignal>
#include <unistd.h>
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#ifndef MB_ICONWARNING
#define MB_ICONWARNING 0x30
#endif
#ifndef MB_ICONSTOP
#define MB_ICONSTOP 0x10
#endif
#ifndef MB_SERVICE_NOTIFICATION
#define MB_SERVICE_NOTIFICATION 0
#endif
#ifndef _ASSERTE
#ifdef _DEBUG
#include <cassert>
#define _ASSERTE(expr) assert(expr)
#else
#define _ASSERTE(expr) ((void)0)
#endif
#endif
inline HWND GetActiveWindow() { return nullptr; }
inline DWORD GetModuleFileNameA(HMODULE, char* buf, DWORD sz) {
    ssize_t len = readlink("/proc/self/exe", buf, sz - 1);
    if (len == -1) { if (sz > 0) buf[0] = '\0'; return 0; }
    buf[len] = '\0';
    return (DWORD)len;
}
inline DWORD GetModuleFileNameW(HMODULE, wchar_t* buf, DWORD sz) { if (sz > 0) buf[0] = L'\0'; return 0; }
inline int GetWindowsDirectoryW(wchar_t*, unsigned int) { return 0; }
inline void OutputDebugStringW(const wchar_t*) {}
inline void ExitProcess(unsigned int code) { _exit(code); }
#endif // !_WIN32

#define _QUOTE(x) #x
#define QUOTE(x) _QUOTE(x)
#define __FILE__LINE__ __FILE__ "(" QUOTE(__LINE__) ") : "

// #define MSG (desc) message(__FILE__ "(" QUOTE(__LINE__) ") : " #desc)
#define TODO(desc) message(__FILE__ "(" QUOTE(__LINE__) ") : TODO " #desc)

#define IN_RANGE(low, Num, High) (((low) <= (Num)) && ((Num) <= (High)))
#define DIM_OF(Array) (sizeof(Array) / sizeof(Array[0]))

#ifdef _X86_
#define DebugBreak() _asm { int 3 }
#elif !defined(_WIN32) && !defined(_WIN64)
#ifndef DebugBreak
#define DebugBreak() raise(SIGTRAP)
#endif
#endif

#define MAKE_SOFTWARE_EXCEPTION(Severity, Facility, Exception) \
	((DWORD)(/* Severity code    */ (Severity) |               \
			 /* MS(0) or Cust(1) */ (1 << 29) |                \
			 /* Reserved(0)      */ (0 << 28) |                \
			 /* Facility code    */ (Facility << 16) |         \
			 /* Exception code   */ (Exception << 0)))

///////////////////////// Common namespace /////////////////////////

namespace kbase_2001 {

#if defined(_WIN32) || defined(_WIN64)

inline void MsgBox(char* sMsg, UINT uType = MB_OK) {
	char title[MAX_PATH];
	GetModuleFileNameA(0, title, DIM_OF(title));
	MessageBoxA(GetActiveWindow(), sMsg, title, uType);
}

inline void MsgBox(wchar_t* sMsg, UINT uType = MB_OK) {
	wchar_t title[MAX_PATH];
	GetModuleFileNameW(0, title, DIM_OF(title));
	MessageBoxW(GetActiveWindow(), sMsg, title, uType);
}

inline int MsgBox(UINT uType, char* title, char* fmt, ...) {
	char message[1024] = {0};
	va_list args;
	va_start(args, fmt);
	wvsprintfA(message, fmt, args);
	va_end(args);
	HWND hwnd = GetActiveWindow();
	return MessageBoxA(0, message, title, uType | ((hwnd == nullptr) ? MB_SERVICE_NOTIFICATION : 0));
}

inline int MsgBox(UINT uType, wchar_t* title, wchar_t* fmt, ...) {
	wchar_t message[1024] = {0};
	va_list args;
	va_start(args, fmt);
	wvsprintfW(message, fmt, args);
	va_end(args);
	HWND hwnd = GetActiveWindow();
	return MessageBoxW(hwnd, message, title, uType | ((hwnd == nullptr) ? MB_SERVICE_NOTIFICATION : 0));
}

#else  // Linux implementations

inline void MsgBox(char* sMsg, UINT /*uType*/ = MB_OK) {
	fprintf(stderr, "[MsgBox] %s\n", sMsg);
}

inline void MsgBox(wchar_t* /*sMsg*/, UINT /*uType*/ = MB_OK) {
	fprintf(stderr, "[MsgBox] (wide string)\n");
}

inline int MsgBox(UINT /*uType*/, char* title, char* fmt, ...) {
	char message[1024] = {0};
	va_list args;
	va_start(args, fmt);
	vsnprintf(message, sizeof(message), fmt, args);
	va_end(args);
	fprintf(stderr, "[%s] %s\n", title, message);
	return 0;
}

inline int MsgBox(UINT /*uType*/, wchar_t* /*title*/, wchar_t* /*fmt*/, ...) {
	fprintf(stderr, "[MsgBox] (wide string)\n");
	return 0;
}

#endif // _WIN32

/*
inline void Fail(char* msg)
{
   char title[MAX_PATH];
   GetModuleFileNameA(0, title, DIM_OF(title));
   MsgBox(MB_ICONSTOP, title, msg);
   DebugBreak();
}

inline void Fail(char* file, int line, char* expr)
{
   char title[MAX_PATH];
   GetModuleFileNameA(0, title, DIM_OF(title));
   MsgBox(MB_ICONSTOP, title,
		"Assertion Failed!\n\nFile %s, line %d : %s", file, line, expr);
   DebugBreak();
}
*/

class SystemInfo : public SYSTEM_INFO {
public:
	SystemInfo() { GetSystemInfo(this); }
};

struct OSVersionInfo : public OSVERSIONINFO {
	OSVersionInfo() {
		//		memset(this, 0, sizeof(OSVERSIONINFO));
		dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(this);
	}
};

#if defined(_WIN32) || defined(_WIN64)
inline void Windows9xNotAllowed() {
	OSVersionInfo vi;
	if (vi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
		MsgBox("This application requires features not present in Windows 9x.", MB_ICONWARNING);
		ExitProcess(0);
	}
}

inline bool IsNT() {
	OSVersionInfo vi;
	return vi.dwPlatformId == VER_PLATFORM_WIN32_NT;
}

inline void Windows2000Required() {
	OSVersionInfo vi;
	if ((vi.dwPlatformId != VER_PLATFORM_WIN32_NT) && (vi.dwMajorVersion < 5)) {
		MsgBox("This application requires features present in Windows 2000.", MB_ICONWARNING);
		ExitProcess(0);
	}
}
#else
inline void Windows9xNotAllowed() { /* no-op on Linux */ }
inline bool IsNT() { return true; /* Linux is always "NT-like" */ }
inline void Windows2000Required() { /* no-op on Linux */ }
#endif

template <class TV, class TM>
inline TV RoundDown(TV Value, TM Multiple) {
	return ((Value / Multiple) * Multiple);
}

template <class TV, class TM>
inline TV RoundUp(TV Value, TM Multiple) {
	return (RoundDown(Value, Multiple) + (((Value % Multiple) > 0) ? Multiple : 0));
}

///////////////////////////// UNICODE Check Macro /////////////////////////////

// Since Windows 98 does not support Unicode, issue an error and terminate
// the process if this is a native Unicode build running on Windows 98
// This is accomplished by creating a global C++ object. Its constructor is
// executed before WinMain.

#if defined(UNICODE) && (defined(_WIN32) || defined(_WIN64))

struct UnicodeSupported {
	UnicodeSupported() {
		if (GetWindowsDirectoryW(nullptr, 0) <= 0) {
			MsgBox("This application requires an OS that supports Unicode.", MB_ICONWARNING);
			ExitProcess(0);
		}
	}
};

// "static" stops the linker from complaining that multiple instances of the
// object exist when a single project contains multiple source files.
static UnicodeSupported g_UnicodeSupported;

#endif // UNICODE && _WIN32

#if defined(_WIN32) || defined(_WIN64)
inline void dprintf(char* pStr, ...) {
	char buffer[1024] = {0};
	va_list args;
	va_start(args, pStr);
	wvsprintfA(buffer, pStr, args);
	va_end(args);
	OutputDebugStringA(buffer);
}

inline void dprintf(wchar_t* pStr, ...) {
	wchar_t buffer[1024] = {0};
	va_list args;
	va_start(args, pStr);
	wvsprintfW(buffer, pStr, args);
	va_end(args);
	OutputDebugStringW(buffer);
}
#else
inline void dprintf(char* pStr, ...) {
	char buffer[1024] = {0};
	va_list args;
	va_start(args, pStr);
	vsnprintf(buffer, sizeof(buffer), pStr, args);
	va_end(args);
	// On Linux, OutputDebugString is a no-op
}

inline void dprintf(wchar_t* /*pStr*/, ...) {
	// Wide string debug output not supported on Linux
}
#endif

inline void dummy_func(...) {}

} // namespace kbase_2001

// declare alias
// namespace kb = kbase_2001;

/*
#ifdef _DEBUG
#define ASSERT(x) if (!(x)) kbase_2001::Fail(__FILE__, __LINE__, #x);
#else
#define ASSERT(x)
#endif
*/

#ifdef _DEBUG
#define VERIFY(x) _ASSERTE(x)
#else
#define VERIFY(x) (x)
#endif

#ifdef _DEBUG
#define TRACE kbase_2001::dprintf
#else
#define TRACE kbase_2001::dummy_func
#endif

/////////////////////////// Force Windows subsystem ///////////////////////////
// #pragma comment(linker, "/subsystem:Windows,5")
// #pragma comment(linker, "/version:5")

#endif //_kbase_8998f07c_472c_4ff3_90b6_ffd5738d85e6
