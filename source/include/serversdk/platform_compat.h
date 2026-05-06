//================================================================
// platform_compat.h
// Cross-platform compatibility layer for PKO Server
// Provides POSIX equivalents for Windows APIs used by ServerSDK
// 
// This file replaces the never-implemented "TAOSpecial.h"
// Include via #define USING_TAO or directly on Linux builds
//
// Created: February 2026 - Linux porting initiative
//================================================================
#ifndef PLATFORM_COMPAT_H
#define PLATFORM_COMPAT_H

//================================================================
// PLATFORM DETECTION
//================================================================
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32)
    #define PKO_PLATFORM_WINDOWS 1
#elif defined(__linux__) || defined(__linux)
    #define PKO_PLATFORM_LINUX 1
#elif defined(__FreeBSD__)
    #define PKO_PLATFORM_FREEBSD 1
#else
    #error "Unsupported platform"
#endif

//================================================================
// WINDOWS: Just include native headers
//================================================================
#ifdef PKO_PLATFORM_WINDOWS

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601  // Windows 7+
#endif
#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0601
#endif

#include <winsock2.h>
#include <Mstcpip.h>
#include <windows.h>
#include <bcrypt.h>   // CNG types needed by wincrypt.h/ncrypt.h in newer Windows SDKs
#include <cstdint>    // int32_t, uint32_t for cross-platform type definitions

#endif // PKO_PLATFORM_WINDOWS

//================================================================
// LINUX/POSIX COMPATIBILITY LAYER
//================================================================
#if defined(PKO_PLATFORM_LINUX) || defined(PKO_PLATFORM_FREEBSD)

// ---- Standard POSIX headers ----
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <signal.h>

// ---- Networking ----
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <poll.h>

// ---- Threading ----
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>

// ---- System ----
#include <sys/time.h>
#include <sys/stat.h>
#include <cwchar>       // wcslen
#include <cstdarg>      // va_list, va_start, va_end
#include <cassert>      // assert
#include <cctype>       // tolower, toupper
#include <climits>      // INT_MAX, PATH_MAX
#include <algorithm>    // std::min, std::max
#include <type_traits>  // std::common_type (for mixed-type min/max)
#include <stdexcept>    // std::runtime_error (replaces MSVC std::exception(const char*))
#include <shared_mutex> // std::shared_mutex, std::unique_lock
#include <dirent.h>     // directory operations
#include <sys/resource.h>
#include <dlfcn.h>

//================================================================
// Structured Exception Handling (SEH) compatibility
// MSVC __try/__except → standard C++ try/catch(...)
//================================================================
#define __try try
#define __except(x) catch(...)
#define EXCEPTION_EXECUTE_HANDLER 1

//================================================================
// Prevent ODBC sqltypes.h from redefining Windows types (WCHAR, DWORD, etc.)
// sqltypes.h checks this guard for its bulk type block.
//================================================================
#ifndef ALREADY_HAVE_WINDOWS_TYPE
#define ALREADY_HAVE_WINDOWS_TYPE
#endif

// Also define FAR, SQL_API, CALLBACK that sqltypes.h provides inside the guard
#ifndef FAR
#define FAR
#endif
#ifndef CALLBACK
#define CALLBACK
#endif
#ifndef SQL_API
#define SQL_API
#endif

// Calling conventions - no-op on Linux/GCC
#ifndef __stdcall
#define __stdcall
#endif
#ifndef __cdecl
#define __cdecl
#endif
#ifndef WINAPI
#define WINAPI
#endif

//================================================================
// ODBC compatibility types
// sqltypes.h's second ALREADY_HAVE_WINDOWS_TYPE guard block defines
// these. Since we set ALREADY_HAVE_WINDOWS_TYPE, we must provide them.
//================================================================
typedef signed char         SCHAR;
typedef SCHAR               SQLSCHAR;
typedef signed short int    SWORD;
typedef unsigned short int  UWORD;
typedef signed int         SLONG;
typedef signed short        SSHORT;
typedef unsigned short      USHORT;
typedef double              SDOUBLE;
typedef double              LDOUBLE;
typedef float               SFLOAT;
typedef signed short        RETCODE;
typedef void*               SQLHWND;
// SDWORD/UDWORD: on LP64 (SIZEOF_LONG_INT == 8), use int (matches sqltypes.h)
typedef int                 SDWORD;
typedef unsigned int        UDWORD;

//================================================================
// Windows type definitions for Linux
// Note: On LP64 (Linux x64), int is 8 bytes. Windows types like
// DWORD/LONG are always 32-bit, so we use int/unsigned int.
// ULONG matches sqltypes.h's unsigned int (8 bytes on LP64).
//================================================================
typedef unsigned char       BYTE;
typedef unsigned char*      PBYTE;
typedef unsigned short      WORD;
typedef unsigned int        UINT;
typedef unsigned int       ULONG;       // matches sqltypes.h on LP64
typedef unsigned int        DWORD;       // 32-bit (matches sqltypes.h)
typedef DWORD*              LPDWORD;
typedef unsigned long long  ULONGLONG;
typedef int                 LONG;        // 32-bit (matches Windows)
typedef long long           LONGLONG;
typedef long long           LONG64;      // 64-bit signed
typedef unsigned long long  ULONG64;     // 64-bit unsigned
typedef long long           __int64;     // MSVC __int64 = long long (avoid LP64 int64_t=long mismatch)
typedef int                 BOOL;
typedef int                 INT;
typedef char                CHAR;
typedef unsigned char       UCHAR;
typedef unsigned short      WCHAR;       // 16-bit UTF-16 (matches sqltypes.h / Windows)
typedef BYTE*               LPBYTE;
typedef char*               LPSTR;
typedef const char*         LPCSTR;
typedef const char*         LPCTSTR;
typedef void*               LPVOID;
typedef void*               HANDLE;
typedef void*               HINSTANCE;
typedef void*               HMODULE;
typedef void*               HWND;
typedef uintptr_t           DWORD_PTR;   // pointer-sized
typedef uintptr_t           ULONG_PTR;
typedef intptr_t            LONG_PTR;
typedef size_t              SIZE_T;
typedef unsigned short      SHORT;       // 16-bit
typedef unsigned short      USHORT;      // 16-bit
typedef WCHAR*              LPWSTR;
typedef const WCHAR*        LPCWSTR;

// Additional Windows types used by GameServer
typedef int                HRESULT;     // COM-style return code
typedef uintptr_t           WPARAM;      // Message parameter (pointer-sized)
typedef intptr_t            LPARAM;      // Message parameter (pointer-sized)

// Max filename component length (MSVC <stdlib.h>)
#ifndef _MAX_FNAME
#define _MAX_FNAME 256
#endif
#ifndef _MAX_PATH
#define _MAX_PATH  260
#endif

// HIWORD / LOWORD macros (Windows API)
#ifndef HIWORD
#define HIWORD(l) ((WORD)(((DWORD_PTR)(l) >> 16) & 0xFFFF))
#endif
#ifndef LOWORD
#define LOWORD(l) ((WORD)((DWORD_PTR)(l) & 0xFFFF))
#endif
#ifndef MAKELONG
#define MAKELONG(a, b) ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xFFFF)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xFFFF))) << 16))
#endif
#ifndef MAKEWORD
#define MAKEWORD(a, b) ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#endif

// RGB macro (Windows GDI)
#ifndef RGB
#define RGB(r,g,b) ((DWORD)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
#endif

// RECT structure (used by util2.h)
typedef struct tagRECT {
    int left;
    int top;
    int right;
    int bottom;
} RECT;

// LOG_PROC callback type (used by log.h)
typedef void (*LOG_PROC)(const char* pszType, const char* pszContent);

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef INFINITE
#define INFINITE 0xFFFFFFFF
#endif
#ifndef WAIT_OBJECT_0
#define WAIT_OBJECT_0 0
#endif
#ifndef WAIT_TIMEOUT
#define WAIT_TIMEOUT  0x00000102
#endif
#ifndef WAIT_ABANDONED
#define WAIT_ABANDONED 0x00000080
#endif
#ifndef NO_ERROR
#define NO_ERROR 0
#endif
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(int)-1)
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif
#ifndef WINAPI
#define WINAPI
#endif
#ifndef CALLBACK
#define CALLBACK
#endif

// ZeroMemory / CopyMemory
#ifndef ZeroMemory
#define ZeroMemory(dest, len)   memset((dest), 0, (len))
#endif
#ifndef CopyMemory
#define CopyMemory(dest, src, len) memcpy((dest), (src), (len))
#endif
#ifndef MoveMemory
#define MoveMemory(dest, src, len) memmove((dest), (src), (len))
#endif

//================================================================
// Socket compatibility
//================================================================
typedef int SOCKET;
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR   (-1)
#endif
#ifndef SD_BOTH
#define SD_BOTH SHUT_RDWR
#endif
#ifndef SD_SEND
#define SD_SEND SHUT_WR
#endif
#ifndef SD_RECEIVE
#define SD_RECEIVE SHUT_RD
#endif

// Winsock -> POSIX socket mapping
inline int closesocket(SOCKET s) { return close(s); }

inline int ioctlsocket(SOCKET s, int cmd, u_long* argp) {
    if (cmd == 0x8004667E /*FIONBIO*/) { // FIONBIO
        int flags = fcntl(s, F_GETFL, 0);
        if (flags < 0) return -1;
        if (*argp)
            flags |= O_NONBLOCK;
        else
            flags &= ~O_NONBLOCK;
        return fcntl(s, F_SETFL, flags);
    }
    return ioctl(s, cmd, argp);
}

// FIONBIO constant used by the codebase
#ifndef FIONBIO
#define FIONBIO 0x8004667E
#endif

// WSA error mapping
inline int WSAGetLastError() { return errno; }

// WSAStartup / WSACleanup - defined later in the file (WSA stubs section)

// Map common WSA error codes to POSIX
#ifndef WSAEINPROGRESS
#define WSAEINPROGRESS  EINPROGRESS
#endif
#ifndef WSAEWOULDBLOCK
#define WSAEWOULDBLOCK  EWOULDBLOCK
#endif
#ifndef WSAECONNRESET
#define WSAECONNRESET   ECONNRESET
#endif
#ifndef WSAENOTCONN
#define WSAENOTCONN     ENOTCONN
#endif
#ifndef WSAECONNABORTED
#define WSAECONNABORTED ECONNABORTED
#endif
#ifndef WSAESHUTDOWN
#define WSAESHUTDOWN    ESHUTDOWN
#endif
#ifndef WSAETIMEDOUT
#define WSAETIMEDOUT    ETIMEDOUT
#endif
#ifndef WSAECONNREFUSED
#define WSAECONNREFUSED ECONNREFUSED
#endif
#ifndef WSAENETDOWN
#define WSAENETDOWN     ENETDOWN
#endif
#ifndef WSANOTINITIALISED
#define WSANOTINITIALISED ENETDOWN  // No POSIX equivalent; map to ENETDOWN
#endif

// FAR pointer qualifier -- meaningless on 32/64-bit flat memory (Windows compat)
#ifndef FAR
#define FAR
#endif

// HOSTENT typedef (Windows uses uppercase, Linux uses lowercase struct hostent)
#ifndef _HOSTENT_DEFINED
typedef struct hostent HOSTENT;
typedef struct hostent* LPHOSTENT;
#define _HOSTENT_DEFINED
#endif

// IN_ADDR pointer type (Windows-style)
typedef struct in_addr* LPIN_ADDR;

// _timeb / _ftime aliases for POSIX timeb/ftime
#include <sys/timeb.h>
#define _timeb  timeb
#define _ftime  ftime

// WM_USER - Windows message constant
#ifndef WM_USER
#define WM_USER 0x0400
#endif

// Windows in_addr uses S_un.S_addr, Linux uses s_addr directly
// On Windows, s_addr is already a macro for S_un.S_addr, so using .s_addr works on both.
// Any remaining S_un.S_addr references in source should be changed to .s_addr
// We remove the broken S_un macro and just provide S_addr mapping
#define S_addr s_addr

// MAKEWORD, LOBYTE, HIBYTE -- used for WSAStartup version check
#ifndef MAKEWORD
#define MAKEWORD(a, b) ((WORD)(((BYTE)((DWORD_PTR)(a) & 0xff)) | ((WORD)((BYTE)((DWORD_PTR)(b) & 0xff))) << 8))
#endif
#ifndef LOBYTE
#define LOBYTE(w) ((BYTE)((DWORD_PTR)(w) & 0xff))
#endif
#ifndef HIBYTE
#define HIBYTE(w) ((BYTE)(((DWORD_PTR)(w) >> 8) & 0xff))
#endif

// SIO_KEEPALIVE_VALS is Windows-specific; on Linux use setsockopt directly
// This struct is used in AcceptConnect.cpp
struct tcp_keepalive {
    unsigned int onoff;
    unsigned int keepalivetime;
    unsigned int keepaliveinterval;
};

//================================================================
// CRITICAL_SECTION emulation via pthread_mutex
//================================================================
typedef pthread_mutex_t CRITICAL_SECTION;
typedef CRITICAL_SECTION* LPCRITICAL_SECTION;

inline void InitializeCriticalSection(LPCRITICAL_SECTION cs) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(cs, &attr);
    pthread_mutexattr_destroy(&attr);
}

inline BOOL InitializeCriticalSectionAndSpinCount(LPCRITICAL_SECTION cs, DWORD /*spinCount*/) {
    // Linux doesn't have spin count for mutexes; just use recursive mutex
    InitializeCriticalSection(cs);
    return TRUE;
}

inline void DeleteCriticalSection(LPCRITICAL_SECTION cs) {
    pthread_mutex_destroy(cs);
}

inline void EnterCriticalSection(LPCRITICAL_SECTION cs) {
    pthread_mutex_lock(cs);
}

inline BOOL TryEnterCriticalSection(LPCRITICAL_SECTION cs) {
    return (pthread_mutex_trylock(cs) == 0) ? TRUE : FALSE;
}

inline void LeaveCriticalSection(LPCRITICAL_SECTION cs) {
    pthread_mutex_unlock(cs);
}

//================================================================
// Semaphore / Thread handle emulation
//================================================================
// On Linux, we wrap sem_t/pthread_t to provide Windows-like APIs
// Used by the Sema class in DBCCommon.h

enum PosixHandleType { POSIX_HANDLE_SEMAPHORE = 0xAA55, POSIX_HANDLE_THREAD = 0x55AA };

struct PosixHandleBase {
    PosixHandleType type;
};

struct PosixSemaphore : public PosixHandleBase {
    sem_t sem;
    int maxCount;
    PosixSemaphore() { type = POSIX_HANDLE_SEMAPHORE; }
};

// PosixThread must be defined before CloseHandle/WaitForSingleObject
struct PosixThread : public PosixHandleBase {
    pthread_t thread;
    bool joinable;
    PosixThread() { type = POSIX_HANDLE_THREAD; joinable = false; }
};

inline HANDLE CreateSemaphore(void* /*security*/, LONG initialCount, LONG maximumCount, LPCTSTR /*name*/) {
    PosixSemaphore* ps = new PosixSemaphore();
    ps->maxCount = maximumCount;
    if (sem_init(&ps->sem, 0, (unsigned int)initialCount) != 0) {
        delete ps;
        return nullptr;
    }
    return (HANDLE)ps;
}

inline BOOL CloseHandle(HANDLE h) {
    if (!h) return FALSE;
    PosixHandleBase* base = (PosixHandleBase*)h;
    if (base->type == POSIX_HANDLE_SEMAPHORE) {
        PosixSemaphore* ps = (PosixSemaphore*)h;
        sem_destroy(&ps->sem);
        delete ps;
    } else if (base->type == POSIX_HANDLE_THREAD) {
        // Thread handle: detach the thread (resources freed on exit)
        struct PosixThread* pt = (struct PosixThread*)h;
        if (pt->joinable) {
            pthread_detach(pt->thread);
            pt->joinable = false;
        }
        delete pt;
    }
    return TRUE;
}

inline DWORD WaitForSingleObject(HANDLE h, DWORD milliseconds) {
    if (!h) return WAIT_TIMEOUT;
    PosixHandleBase* base = (PosixHandleBase*)h;
    
    if (base->type == POSIX_HANDLE_THREAD) {
        // Wait for thread to finish
        struct PosixThread* pt = (struct PosixThread*)h;
        if (!pt->joinable) return WAIT_OBJECT_0;
        if (milliseconds == INFINITE) {
            pthread_join(pt->thread, nullptr);
            pt->joinable = false;
            return WAIT_OBJECT_0;
        } else {
            // Timed thread wait -- use pthread_timedjoin_np on Linux
#ifdef __linux__
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec  += milliseconds / 1000;
            ts.tv_nsec += (milliseconds % 1000) * 1000000L;
            if (ts.tv_nsec >= 1000000000L) { ts.tv_sec++; ts.tv_nsec -= 1000000000L; }
            if (pthread_timedjoin_np(pt->thread, nullptr, &ts) == 0) {
                pt->joinable = false;
                return WAIT_OBJECT_0;
            }
#endif
            return WAIT_TIMEOUT;
        }
    }
    
    // Semaphore wait
    PosixSemaphore* ps = (PosixSemaphore*)h;
    if (milliseconds == INFINITE) {
        if (sem_wait(&ps->sem) == 0)
            return WAIT_OBJECT_0;
        return WAIT_TIMEOUT;
    } else if (milliseconds == 0) {
        if (sem_trywait(&ps->sem) == 0)
            return WAIT_OBJECT_0;
        return WAIT_TIMEOUT;
    } else {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec  += milliseconds / 1000;
        ts.tv_nsec += (milliseconds % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000L;
        }
        if (sem_timedwait(&ps->sem, &ts) == 0)
            return WAIT_OBJECT_0;
        return WAIT_TIMEOUT;
    }
}

inline BOOL ReleaseSemaphore(HANDLE h, LONG releaseCount, LONG* /*previousCount*/) {
    if (!h) return FALSE;
    PosixSemaphore* ps = (PosixSemaphore*)h;
    for (LONG i = 0; i < releaseCount; i++) {
        sem_post(&ps->sem);
    }
    return TRUE;
}

//================================================================
// Interlocked operations via GCC/Clang atomic builtins
//================================================================
inline LONG InterlockedIncrement(volatile LONG* val) {
    return __sync_add_and_fetch(val, 1);
}
inline LONG InterlockedDecrement(volatile LONG* val) {
    return __sync_sub_and_fetch(val, 1);
}
inline LONG InterlockedExchangeAdd(volatile LONG* val, LONG amount) {
    return __sync_fetch_and_add(val, amount);
}
inline LONG InterlockedExchange(volatile LONG* val, LONG newval) {
    return __sync_lock_test_and_set(val, newval);
}
// Note: int→int overloads removed (LONG=int on both platforms now)
inline LONG InterlockedCompareExchange(volatile LONG* val, LONG newval, LONG comperand) {
    return __sync_val_compare_and_swap(val, comperand, newval);
}

// Acquire variants (same on x86/x64 -- already acquire semantics)
#define InterlockedIncrementAcquire      InterlockedIncrement
#define InterlockedDecrementAcquire      InterlockedDecrement
#define InterlockedCompareExchangeAcquire InterlockedCompareExchange

// 64-bit variants
inline long long InterlockedIncrement64(volatile long long* val) {
    return __sync_add_and_fetch(val, 1LL);
}
inline long long InterlockedDecrement64(volatile long long* val) {
    return __sync_sub_and_fetch(val, 1LL);
}
inline long long InterlockedExchangeAdd64(volatile long long* val, long long amount) {
    return __sync_fetch_and_add(val, amount);
}
inline long long InterlockedExchange64(volatile long long* val, long long newval) {
    return __sync_lock_test_and_set(val, newval);
}
inline long long InterlockedCompareExchange64(volatile long long* val, long long newval, long long comperand) {
    return __sync_val_compare_and_swap(val, comperand, newval);
}
#define InterlockedIncrementAcquire64      InterlockedIncrement64
#define InterlockedDecrementAcquire64      InterlockedDecrement64
#define InterlockedCompareExchangeAcquire64 InterlockedCompareExchange64

//================================================================
// Thread creation wrappers
//================================================================
// The codebase uses: DWORD WINAPI ThreadProc(LPVOID)
// On Linux we need: void* ThreadProc(void*)

// PosixThread already defined above (before CloseHandle)

typedef DWORD (WINAPI *LPTHREAD_START_ROUTINE)(LPVOID);

// Thread trampoline: converts Windows DWORD WINAPI f(LPVOID) to void* f(void*)
struct _ThreadTrampoline {
    LPTHREAD_START_ROUTINE func;
    LPVOID param;
};

inline void* _PosixThreadEntry(void* arg) {
    _ThreadTrampoline* tramp = (_ThreadTrampoline*)arg;
    LPTHREAD_START_ROUTINE func = tramp->func;
    LPVOID param = tramp->param;
    delete tramp;
    DWORD result = func(param);
    return (void*)(uintptr_t)result;
}

inline HANDLE CreateThread(
    void* /*securityAttribs*/,
    SIZE_T /*stackSize*/,
    LPTHREAD_START_ROUTINE startAddress,
    LPVOID parameter,
    DWORD /*creationFlags*/,
    DWORD* threadId)
{
    PosixThread* pt = new PosixThread();
    pt->joinable = true;
    _ThreadTrampoline* tramp = new _ThreadTrampoline();
    tramp->func = startAddress;
    tramp->param = parameter;
    
    if (pthread_create(&pt->thread, nullptr, _PosixThreadEntry, tramp) != 0) {
        delete tramp;
        delete pt;
        return nullptr;
    }
    if (threadId) {
        *threadId = (DWORD)(uintptr_t)pt->thread;
    }
    return (HANDLE)pt;
}

// Thread priority -- no-op on Linux (use nice values if needed)
#ifndef THREAD_PRIORITY_NORMAL
#define THREAD_PRIORITY_NORMAL          0
#define THREAD_PRIORITY_ABOVE_NORMAL    1
#define THREAD_PRIORITY_BELOW_NORMAL   -1
#define THREAD_PRIORITY_HIGHEST         2
#define THREAD_PRIORITY_LOWEST         -2
#define THREAD_PRIORITY_IDLE           -15
#define THREAD_PRIORITY_TIME_CRITICAL   15

#ifndef CREATE_SUSPENDED
#define CREATE_SUSPENDED 0x00000004
#endif
#endif

inline BOOL SetThreadPriority(HANDLE /*hThread*/, int /*nPriority*/) {
    // Linux uses nice values or sched_setscheduler; ignore for now
    return TRUE;
}

inline int GetThreadPriority(HANDLE /*hThread*/) {
    return THREAD_PRIORITY_NORMAL;
}

//================================================================
// Timer functions
//================================================================
inline DWORD GetTickCount() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (DWORD)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

// LARGE_INTEGER for QueryPerformanceCounter
typedef union _LARGE_INTEGER {
    struct {
        DWORD LowPart;
        LONG  HighPart;
    };
    long long QuadPart;
} LARGE_INTEGER;

inline BOOL QueryPerformanceFrequency(LARGE_INTEGER* freq) {
    freq->QuadPart = 1000000000LL; // nanoseconds
    return TRUE;
}

inline BOOL QueryPerformanceCounter(LARGE_INTEGER* counter) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    counter->QuadPart = (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
    return TRUE;
}

inline HANDLE GetCurrentThread() {
    // Returns a pseudo-handle; only used for SetThreadAffinityMask which we no-op
    return (HANDLE)(uintptr_t)pthread_self();
}

inline DWORD_PTR SetThreadAffinityMask(HANDLE /*hThread*/, DWORD_PTR /*mask*/) {
    // No-op on Linux for timer usage; could use pthread_setaffinity_np if needed
    return 1;
}

//================================================================
// Sleep
//================================================================
inline void Sleep(DWORD milliseconds) {
    if (milliseconds == 0) {
        sched_yield();  // Sleep(0) on Windows yields the timeslice
        return;
    }
    usleep(milliseconds * 1000);
}

//================================================================
// Console functions
//================================================================
#ifndef STD_OUTPUT_HANDLE
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#endif
#ifndef STD_ERROR_HANDLE
#define STD_ERROR_HANDLE  ((DWORD)-12)
#endif

inline HANDLE GetStdHandle(DWORD /*nStdHandle*/) {
    return nullptr; // Not used meaningfully on Linux
}

//================================================================
// String compatibility
//================================================================
// strncpy_s emulation
inline int strncpy_s(char* dest, size_t destSize, const char* src, size_t count) {
    if (!dest || destSize == 0) return -1;
    if (!src) { dest[0] = '\0'; return -1; }
    size_t toCopy = (count < destSize - 1) ? count : destSize - 1;
    strncpy(dest, src, toCopy);
    dest[toCopy] = '\0';
    return 0;
}

// strcpy_s emulation (template auto-detects array size, function takes explicit size)
#ifndef _PKO_STRCPY_S_DEFINED
#define _PKO_STRCPY_S_DEFINED
template <size_t N>
inline int strcpy_s(char (&dest)[N], const char* src) {
    if (!src) { dest[0] = '\0'; return -1; }
    strncpy(dest, src, N);
    dest[N - 1] = '\0';
    return 0;
}
inline int strcpy_s(char* dest, size_t destSize, const char* src) {
    if (!dest || destSize == 0) return -1;
    if (!src) { dest[0] = '\0'; return -1; }
    strncpy(dest, src, destSize);
    dest[destSize - 1] = '\0';
    return 0;
}
#endif

// _countof - element count for static arrays
#ifndef _countof
#define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

// _TRUNCATE
#ifndef _TRUNCATE
#define _TRUNCATE ((size_t)-1)
#endif

// _stricmp / _strnicmp
#ifndef _stricmp
#define _stricmp strcasecmp
#endif
#ifndef _strnicmp
#define _strnicmp strncasecmp
#endif

// strlwr - in-place lowercase string (MSVC CRT function)
inline char* strlwr(char* str) {
    if (!str) return nullptr;
    for (char* p = str; *p; ++p) {
        *p = (char)tolower((unsigned char)*p);
    }
    return str;
}
// _strlwr alias
#ifndef _strlwr
#define _strlwr strlwr
#endif

// _snprintf
#ifndef _snprintf
#define _snprintf snprintf
#endif

// sprintf_s -> snprintf (template auto-detects array size, function takes explicit size)
#ifndef _PKO_SPRINTF_S_DEFINED
#define _PKO_SPRINTF_S_DEFINED
template <size_t N>
inline int sprintf_s(char (&buf)[N], const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = vsnprintf(buf, N, fmt, args);
    va_end(args);
    return result;
}
inline int sprintf_s(char* buf, size_t size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return result;
}
#endif

// _snprintf_s -> snprintf (for MSVC secure CRT calls)
#ifndef _snprintf_s
#define _snprintf_s(buf, size, count, ...) snprintf(buf, size, __VA_ARGS__)
#endif

// strncat_s emulation
inline int strncat_s(char* dest, size_t destSize, const char* src, size_t count) {
    if (!dest || destSize == 0) return -1;
    if (!src) return -1;
    size_t destLen = strlen(dest);
    if (destLen >= destSize) return -1;
    size_t remaining = destSize - destLen - 1;
    size_t toCopy = (count == _TRUNCATE) ? remaining : ((count < remaining) ? count : remaining);
    strncat(dest, src, toCopy);
    return 0;
}

// strcat_s emulation (MSVC secure strcat)
inline int strcat_s(char* dest, size_t destSize, const char* src) {
    if (!dest || !src || destSize == 0) return -1;
    size_t destLen = strlen(dest);
    if (destLen >= destSize) return -1;
    size_t remaining = destSize - destLen - 1;
    size_t srcLen = strlen(src);
    size_t toCopy = (srcLen < remaining) ? srcLen : remaining;
    strncat(dest, src, toCopy);
    return 0;
}

// lstrcmp / lstrcpy / lstrcmpi (Win32 string functions)
#ifndef lstrcmp
#define lstrcmp strcmp
#endif
#ifndef lstrcpy
#define lstrcpy strcpy
#endif
#ifndef lstrcmpi
#define lstrcmpi strcasecmp
#endif
#ifndef lstrlen
#define lstrlen strlen
#endif

// stricmp (bare, without underscore prefix) -> strcasecmp
#ifndef stricmp
#define stricmp strcasecmp
#endif
#ifndef strnicmp
#define strnicmp strncasecmp
#endif

// _dos_getdate / dosdate_t (DOS date function used in db3.cpp)
struct dosdate_t {
    unsigned char day;
    unsigned char month;
    unsigned short year;
    unsigned char dayofweek;
};

inline void _dos_getdate(dosdate_t* date) {
    time_t t = time(nullptr);
    struct tm tm_result;
    localtime_r(&t, &tm_result);
    date->day = (unsigned char)tm_result.tm_mday;
    date->month = (unsigned char)(tm_result.tm_mon + 1);
    date->year = (unsigned short)(tm_result.tm_year + 1900);
    date->dayofweek = (unsigned char)tm_result.tm_wday;
}

// _atoi64 -> atoll (MSVC 64-bit integer parsing)
#ifndef _atoi64
#define _atoi64 atoll
#endif

// _TEXT / _T macro (TCHAR text macro - in non-Unicode build, identity)
#ifndef _TEXT
#define _TEXT(x) x
#endif
#ifndef _T
#define _T(x) x
#endif

// _strdup -> strdup (POSIX)
#ifndef _strdup
#define _strdup strdup
#endif

// _vsnprintf -> vsnprintf (POSIX)
#ifndef _vsnprintf
#define _vsnprintf vsnprintf
#endif

// _strupr (uppercase a string in-place - MSVC extension)
inline char* _strupr(char* str) {
    if (!str) return nullptr;
    for (char* p = str; *p; ++p) *p = (char)toupper((unsigned char)*p);
    return str;
}

// _chmod -> chmod, _S_IWRITE -> S_IWUSR
#ifndef _chmod
#define _chmod chmod
#endif
#ifndef _S_IWRITE
#define _S_IWRITE S_IWUSR
#endif
#ifndef _S_IREAD
#define _S_IREAD S_IRUSR
#endif

// __get_cpuid (GCC provides cpuid.h)
#include <cpuid.h>

// MessageBox constants (used in error dialogs - no-op on Linux server)
#ifndef MB_OK
#define MB_OK              0x00000000
#endif
#ifndef MB_ICONERROR
#define MB_ICONERROR       0x00000010
#endif
#ifndef MB_ICONWARNING
#define MB_ICONWARNING     0x00000030
#endif
#ifndef MB_YESNO
#define MB_YESNO           0x00000004
#endif
#ifndef IDYES
#define IDYES              6
#endif
#ifndef IDOK
#define IDOK               1
#endif

// MessageBox stub (no GUI on Linux server, just log and return)
inline int MessageBox(HWND hWnd, const char* text, const char* caption, unsigned int type) {
    (void)hWnd; (void)type;
    fprintf(stderr, "[MessageBox] %s: %s\n", caption ? caption : "", text ? text : "");
    return 0;
}
inline int MessageBoxA(HWND hWnd, const char* text, const char* caption, unsigned int type) {
    return MessageBox(hWnd, text, caption, type);
}

// CopyFile stub (Win32 file copy)
inline BOOL CopyFile(const char* src, const char* dst, BOOL failIfExists) {
    if (failIfExists) {
        struct stat st;
        if (stat(dst, &st) == 0) return FALSE; // file exists
    }
    FILE* in = fopen(src, "rb");
    if (!in) return FALSE;
    FILE* out = fopen(dst, "wb");
    if (!out) { fclose(in); return FALSE; }
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
        fwrite(buf, 1, n, out);
    fclose(in);
    fclose(out);
    return TRUE;
}
inline BOOL CopyFileA(const char* src, const char* dst, BOOL failIfExists) {
    return CopyFile(src, dst, failIfExists);
}

// DeleteFile stub (Win32 file delete)
inline BOOL DeleteFile(const char* path) {
    return (remove(path) == 0) ? TRUE : FALSE;
}
inline BOOL DeleteFileA(const char* path) {
    return DeleteFile(path);
}

// MoveFile stub
inline BOOL MoveFile(const char* src, const char* dst) {
    return (rename(src, dst) == 0) ? TRUE : FALSE;
}

// CreateEvent / SetEvent / ResetEvent stubs (emulate Win32 events with condition variables)
// Simplified implementation for porting
#include <pthread.h>
struct _LINUX_EVENT {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool signaled;
    bool manualReset;
};

inline HANDLE CreateEvent(void* secAttr, BOOL manualReset, BOOL initialState, const char* name) {
    (void)secAttr; (void)name;
    _LINUX_EVENT* evt = new _LINUX_EVENT();
    pthread_mutex_init(&evt->mutex, nullptr);
    pthread_cond_init(&evt->cond, nullptr);
    evt->signaled = (initialState != 0);
    evt->manualReset = (manualReset != 0);
    return (HANDLE)evt;
}

inline BOOL SetEvent(HANDLE hEvent) {
    _LINUX_EVENT* evt = (_LINUX_EVENT*)hEvent;
    if (!evt) return FALSE;
    pthread_mutex_lock(&evt->mutex);
    evt->signaled = true;
    if (evt->manualReset)
        pthread_cond_broadcast(&evt->cond);
    else
        pthread_cond_signal(&evt->cond);
    pthread_mutex_unlock(&evt->mutex);
    return TRUE;
}

inline BOOL ResetEvent(HANDLE hEvent) {
    _LINUX_EVENT* evt = (_LINUX_EVENT*)hEvent;
    if (!evt) return FALSE;
    pthread_mutex_lock(&evt->mutex);
    evt->signaled = false;
    pthread_mutex_unlock(&evt->mutex);
    return TRUE;
}

// PostMessage stub (no window messaging on Linux server)
inline BOOL PostMessage(HWND hWnd, UINT msg, DWORD_PTR wParam, LONG_PTR lParam) {
    (void)hWnd; (void)msg; (void)wParam; (void)lParam;
    return TRUE; // no-op
}

// GetExitCodeThread / TerminateThread / ResumeThread stubs
#ifndef STILL_ACTIVE
#define STILL_ACTIVE 0x00000103
#endif
#ifndef ERROR_ALREADY_EXISTS
#define ERROR_ALREADY_EXISTS 183
#endif

inline BOOL GetExitCodeThread(HANDLE hThread, DWORD* lpExitCode) {
    (void)hThread;
    if (lpExitCode) *lpExitCode = 0;
    return TRUE;
}

inline BOOL TerminateThread(HANDLE hThread, DWORD exitCode) {
    (void)exitCode;
    PosixThread* pt = (PosixThread*)hThread;
    if (pt) {
        pthread_cancel(pt->thread);
        return TRUE;
    }
    return FALSE;
}

inline DWORD ResumeThread(HANDLE hThread) {
    (void)hThread;
    return 0; // stub - Linux threads don't have suspend/resume
}

inline DWORD SuspendThread(HANDLE hThread) {
    (void)hThread;
    return (DWORD)-1; // stub - not supported on Linux
}

// SOCKADDR_IN / LPSOCKADDR (networking types - from POSIX headers)
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
typedef struct sockaddr_in  SOCKADDR_IN;
typedef struct sockaddr*    LPSOCKADDR;

// Windows Sockets compatibility (WSA stubs)
// Linux doesn't need WSAStartup/WSACleanup; sockets work directly.
typedef int SOCKET;
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR   (-1)
#endif
#ifndef closesocket
#define closesocket close
#endif

struct WSADATA {
    WORD wVersion;
    WORD wHighVersion;
    char szDescription[257];
    char szSystemStatus[129];
};
#ifndef MAKEWORD
#define MAKEWORD(a, b) ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#endif

inline int WSAStartup(WORD versionRequested, WSADATA* wsaData) {
    (void)versionRequested;
    if (wsaData) memset(wsaData, 0, sizeof(WSADATA));
    return 0; // success - Linux doesn't need socket initialization
}
inline int WSACleanup() { return 0; }

// WSAAsyncSelect - Windows message-based socket notifications (no Linux equivalent)
// FD_READ/WRITE/etc constants for WSA events
#ifndef FD_READ
#define FD_READ    0x01
#define FD_WRITE   0x02
#define FD_OOB     0x04
#define FD_ACCEPT  0x08
#define FD_CONNECT 0x10
#define FD_CLOSE   0x20
#endif

inline int WSAAsyncSelect(SOCKET s, HWND hWnd, unsigned int wMsg, int lEvent) {
    (void)s; (void)hWnd; (void)wMsg; (void)lEvent;
    return 0; // no-op on Linux - use poll/epoll instead
}
#define WSAGETSELECTEVENT(lParam) ((int)(lParam) & 0xFFFF)

// ZeroMemory
#ifndef ZeroMemory
#define ZeroMemory(ptr, size) memset((ptr), 0, (size))
#endif

// WC_COMPOSITECHECK / WC_NO_BEST_FIT_CHARS (Unicode conversion flags)
#ifndef WC_COMPOSITECHECK
#define WC_COMPOSITECHECK 0x00000200
#endif
#ifndef WC_NO_BEST_FIT_CHARS
#define WC_NO_BEST_FIT_CHARS 0x00000400
#endif
#ifndef WINVER
#define WINVER 0x0600
#endif

// min / max - Windows code uses min()/max() as global functions with mixed types.
// Approach: Bring std::min/max into global scope (handles same-type calls),
// plus SFINAE-guarded template overloads for mixed-type calls (int vs size_t, etc.)
// This avoids macros which break STL headers (numeric_limits::min(), etc.)
#include <algorithm>
#include <type_traits>
using std::min;
using std::max;

// Mixed-type overloads: enabled ONLY when T != U to avoid ambiguity with std::min/max
template<typename T, typename U,
         typename = typename std::enable_if<!std::is_same<typename std::decay<T>::type,
                                                           typename std::decay<U>::type>::value>::type>
inline auto min(const T& a, const U& b) -> typename std::common_type<T, U>::type {
    typedef typename std::common_type<T, U>::type CT;
    return (static_cast<CT>(a) < static_cast<CT>(b)) ? static_cast<CT>(a) : static_cast<CT>(b);
}
template<typename T, typename U,
         typename = typename std::enable_if<!std::is_same<typename std::decay<T>::type,
                                                           typename std::decay<U>::type>::value>::type>
inline auto max(const T& a, const U& b) -> typename std::common_type<T, U>::type {
    typedef typename std::common_type<T, U>::type CT;
    return (static_cast<CT>(a) > static_cast<CT>(b)) ? static_cast<CT>(a) : static_cast<CT>(b);
}

// __min / __max - Microsoft-specific, safe as macros (no STL conflict)
#ifndef __min
#define __min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef __max
#define __max(a,b) (((a) > (b)) ? (a) : (b))
#endif

// _setmaxstdio -> setrlimit
#include <sys/resource.h>
inline int _setmaxstdio(int maxFiles) {
    struct rlimit rl;
    rl.rlim_cur = maxFiles;
    rl.rlim_max = maxFiles;
    return setrlimit(RLIMIT_NOFILE, &rl);
}

// itoa is not standard; provide a simple version
inline char* _itoa(int value, char* str, int base) {
    if (base == 10) {
        sprintf(str, "%d", value);
    } else if (base == 16) {
        sprintf(str, "%x", value);
    } else if (base == 8) {
        sprintf(str, "%o", value);
    }
    return str;
}
#ifndef itoa
#define itoa _itoa
#endif

// _itoa_s (MSVC secure version)
inline int _itoa_s(int value, char* buf, size_t bufSize, int radix) {
    if (!buf || bufSize == 0) return -1;
    if (radix == 10) {
        snprintf(buf, bufSize, "%d", value);
    } else if (radix == 16) {
        snprintf(buf, bufSize, "%x", value);
    } else if (radix == 8) {
        snprintf(buf, bufSize, "%o", value);
    }
    return 0;
}

// UINT32 type (used by FindPath.cpp)
#ifndef UINT32
typedef uint32_t UINT32;
#endif

// _tmain compatibility
#ifndef _tmain
#define _tmain main
#endif
// _TCHAR and TCHAR are defined in tchar.h shim via typedef
#include "tchar.h"
#ifndef _T
#define _T(x) x
#endif

//================================================================
// Dynamic library loading
//================================================================
inline HMODULE LoadLibrary(LPCSTR path) {
    return dlopen(path, RTLD_LAZY);
}
inline BOOL FreeLibrary(HMODULE hModule) {
    return (dlclose(hModule) == 0) ? TRUE : FALSE;
}
// Note: GetProcAddress is commonly needed; define as macro
#define GetProcAddress(handle, name) dlsym(handle, name)

//================================================================
// Misc Win32 stubs
//================================================================
inline DWORD GetLastError() { return (DWORD)errno; }
inline void SetLastError(DWORD err) { errno = (int)err; }
inline void OutputDebugString(LPCSTR /*s*/) {} // No-op on Linux

// GetSystemInfo for CPU count
struct SYSTEM_INFO {
    DWORD dwNumberOfProcessors;
};
inline void GetSystemInfo(SYSTEM_INFO* si) {
    si->dwNumberOfProcessors = (DWORD)sysconf(_SC_NPROCESSORS_ONLN);
}

// SetConsoleCtrlHandler stub
typedef BOOL (WINAPI *PHANDLER_ROUTINE)(DWORD);
inline BOOL SetConsoleCtrlHandler(PHANDLER_ROUTINE handler, BOOL add) {
    // On Linux, use signal handlers instead. This is a stub.
    (void)handler; (void)add;
    return TRUE;
}

// MessageBox stub - defined earlier in the file (around line 840)

// GetModuleHandle stub
inline HMODULE GetModuleHandle(LPCSTR) { return nullptr; }

//================================================================
// Windows version check stubs (used in DBCCommon.cpp)
//================================================================
#define VER_PLATFORM_WIN32_NT 2

struct OSVERSIONINFOEX {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    char szCSDVersion[128];
};
typedef OSVERSIONINFOEX* POSVERSIONINFOEX;
typedef OSVERSIONINFOEX OSVERSIONINFO;

inline BOOL GetVersionEx(OSVERSIONINFO* osvi) {
    // On Linux, pretend we're a modern system that supports Acquire semantics
    osvi->dwPlatformId = VER_PLATFORM_WIN32_NT;
    osvi->dwMajorVersion = 10;
    osvi->dwMinorVersion = 0;
    return TRUE;
}

//================================================================
// WideCharToMultiByte stub (used in DBCCommon.h wctmbcpy)
//================================================================
inline int WideCharToMultiByte(
    unsigned int /*codePage*/, unsigned int /*flags*/,
    const wchar_t* src, int srcLen,
    char* dst, int dstLen,
    const char* /*defaultChar*/, int* /*usedDefaultChar*/)
{
    // Simple wcstombs wrapper; proper implementation would use ICU
    if (!src) return 0;
    size_t len = (srcLen < 0) ? wcslen(src) : (size_t)srcLen;
    if (!dst || dstLen == 0) {
        // Calculate required size
        return (int)(len * 4 + 1); // worst case UTF-8
    }
    size_t result = wcstombs(dst, src, (size_t)dstLen);
    if (result == (size_t)-1) return 0;
    return (int)result;
}

//================================================================
// PostQuitMessage / PeekMessage stubs (for server mains that have
// Win32 message loops -- these will be refactored)
//================================================================
struct MSG { int message; };
#define WM_QUIT 0x0012
#define WM_CLOSE 0x0010
inline void PostQuitMessage(int /*exitCode*/) {}
inline BOOL PeekMessage(MSG* msg, HWND, UINT, UINT, UINT) {
    msg->message = 0;
    return FALSE;
}
inline BOOL TranslateMessage(const MSG*) { return FALSE; }
inline LONG_PTR DispatchMessage(const MSG*) { return 0; }
inline BOOL GetMessage(MSG* msg, HWND, UINT, UINT) {
    msg->message = 0;
    // Block briefly to avoid busy spin
    usleep(10000); // 10ms
    return TRUE;
}

// WINBASEAPI stubs for the extern "C" declarations in DBCCommon.h
#ifndef WINBASEAPI
#define WINBASEAPI
#endif

//================================================================
// Thread Local Storage (TLS) -- TLSIndex.h equivalents
//================================================================
#define TLS_OUT_OF_INDEXES ((DWORD)0xFFFFFFFF)
typedef pthread_key_t TLS_KEY;

inline DWORD TlsAlloc() {
    pthread_key_t key;
    if (pthread_key_create(&key, nullptr) != 0)
        return TLS_OUT_OF_INDEXES;
    return (DWORD)key;
}
inline BOOL TlsFree(DWORD dwTlsIndex) {
    return (pthread_key_delete((pthread_key_t)dwTlsIndex) == 0) ? TRUE : FALSE;
}
inline LPVOID TlsGetValue(DWORD dwTlsIndex) {
    return pthread_getspecific((pthread_key_t)dwTlsIndex);
}
inline BOOL TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue) {
    return (pthread_setspecific((pthread_key_t)dwTlsIndex, lpTlsValue) == 0) ? TRUE : FALSE;
}

//================================================================
// Process / Thread affinity stubs
//================================================================
typedef int* HANDLE_PROCESS; // Dummy type for GetCurrentProcess
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#endif

inline HANDLE GetCurrentProcess() { return (HANDLE)(intptr_t)-1; }

inline BOOL GetProcessAffinityMask(HANDLE, DWORD_PTR* processAffinityMask, DWORD_PTR* systemAffinityMask) {
    // Return all CPUs available
    int nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    DWORD_PTR mask = 0;
    for (int i = 0; i < nprocs && i < 64; i++)
        mask |= ((DWORD_PTR)1 << i);
    if (processAffinityMask) *processAffinityMask = mask;
    if (systemAffinityMask)  *systemAffinityMask = mask;
    return TRUE;
}

// SetThreadAffinityMask already defined above (line ~595)

//================================================================
// Multimedia timer -- timeGetTime() equivalent
//================================================================
inline DWORD timeGetTime() {
    return GetTickCount(); // Same resolution for server purposes
}

//================================================================
// Secure CRT functions (MSVC-specific) - additional
//================================================================

// errno_t -- MSVC type, just int on POSIX
#ifndef _ERRNO_T_DEFINED
typedef int errno_t;
#define _ERRNO_T_DEFINED
#endif

// fopen_s -- returns errno_t, sets *pFile
inline errno_t fopen_s(FILE** pFile, const char* filename, const char* mode) {
    if (!pFile) return EINVAL;
    *pFile = fopen(filename, mode);
    if (!*pFile) return errno;
    return 0;
}

// _stricmp / _strnicmp / itoa / _TRUNCATE already defined above

// _i64toa -- MSVC-specific 64-bit itoa
inline char* _i64toa(long long value, char* buffer, int radix) {
    if (radix == 10) {
        sprintf(buffer, "%lld", value);
    } else if (radix == 16) {
        sprintf(buffer, "%llx", value);
    } else {
        buffer[0] = '\0';
    }
    return buffer;
}

//================================================================
// GetTickCount64 -- 64-bit monotonic tick count (milliseconds)
//================================================================
inline unsigned long long GetTickCount64() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long)(ts.tv_sec) * 1000ULL + ts.tv_nsec / 1000000ULL;
}

//================================================================
// SYSTEMTIME / GetLocalTime / GetSystemTime
//================================================================
typedef struct _SYSTEMTIME {
    unsigned short wYear;
    unsigned short wMonth;
    unsigned short wDayOfWeek;
    unsigned short wDay;
    unsigned short wHour;
    unsigned short wMinute;
    unsigned short wSecond;
    unsigned short wMilliseconds;
} SYSTEMTIME;

inline void GetLocalTime(SYSTEMTIME* st) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm_result;
    localtime_r(&ts.tv_sec, &tm_result);
    st->wYear         = (unsigned short)(tm_result.tm_year + 1900);
    st->wMonth        = (unsigned short)(tm_result.tm_mon + 1);
    st->wDayOfWeek    = (unsigned short)(tm_result.tm_wday);
    st->wDay          = (unsigned short)(tm_result.tm_mday);
    st->wHour         = (unsigned short)(tm_result.tm_hour);
    st->wMinute       = (unsigned short)(tm_result.tm_min);
    st->wSecond       = (unsigned short)(tm_result.tm_sec);
    st->wMilliseconds = (unsigned short)(ts.tv_nsec / 1000000);
}

inline void GetSystemTime(SYSTEMTIME* st) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm_result;
    gmtime_r(&ts.tv_sec, &tm_result);
    st->wYear         = (unsigned short)(tm_result.tm_year + 1900);
    st->wMonth        = (unsigned short)(tm_result.tm_mon + 1);
    st->wDayOfWeek    = (unsigned short)(tm_result.tm_wday);
    st->wDay          = (unsigned short)(tm_result.tm_mday);
    st->wHour         = (unsigned short)(tm_result.tm_hour);
    st->wMinute       = (unsigned short)(tm_result.tm_min);
    st->wSecond       = (unsigned short)(tm_result.tm_sec);
    st->wMilliseconds = (unsigned short)(ts.tv_nsec / 1000000);
}

//================================================================
// FILETIME / GetSystemTimeAsFileTime
//================================================================
typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME;

inline void GetSystemTimeAsFileTime(FILETIME* ft) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    // Windows FILETIME epoch: Jan 1, 1601; Unix epoch: Jan 1, 1970
    // Difference: 11644473600 seconds, in 100-nanosecond intervals
    unsigned long long ticks = (unsigned long long)ts.tv_sec * 10000000ULL
                             + ts.tv_nsec / 100ULL
                             + 116444736000000000ULL;
    ft->dwLowDateTime  = (DWORD)(ticks & 0xFFFFFFFF);
    ft->dwHighDateTime = (DWORD)(ticks >> 32);
}

//================================================================
// Process / Thread ID functions
//================================================================
#include <sys/syscall.h>

inline DWORD GetCurrentProcessId() {
    return (DWORD)getpid();
}

inline DWORD GetCurrentThreadId() {
    return (DWORD)syscall(SYS_gettid);
}

inline void ExitProcess(int exitCode) {
    _exit(exitCode);
}

//================================================================
// Console text color (ANSI escape sequences)
//================================================================
#ifndef FOREGROUND_RED
#define FOREGROUND_RED       0x0004
#define FOREGROUND_GREEN     0x0002
#define FOREGROUND_BLUE      0x0001
#define FOREGROUND_INTENSITY 0x0008
#endif

inline BOOL SetConsoleTextAttribute(HANDLE /*hConsole*/, WORD wAttributes) {
    // Map Windows color attributes to ANSI escape codes
    if (wAttributes & FOREGROUND_RED && wAttributes & FOREGROUND_INTENSITY)
        fprintf(stdout, "\033[91m"); // Bright red
    else if (wAttributes & FOREGROUND_RED)
        fprintf(stdout, "\033[31m"); // Red
    else if (wAttributes & FOREGROUND_GREEN && wAttributes & FOREGROUND_INTENSITY)
        fprintf(stdout, "\033[92m"); // Bright green
    else if (wAttributes & FOREGROUND_GREEN)
        fprintf(stdout, "\033[32m"); // Green
    else if (wAttributes & FOREGROUND_BLUE && wAttributes & FOREGROUND_INTENSITY)
        fprintf(stdout, "\033[94m"); // Bright blue
    else if (wAttributes & FOREGROUND_BLUE)
        fprintf(stdout, "\033[34m"); // Blue
    else
        fprintf(stdout, "\033[0m");  // Reset
    return TRUE;
}

inline void ResetConsoleColor() {
    fprintf(stdout, "\033[0m");
}

//================================================================
// File access check (_access → access)
//================================================================
#ifndef _access
#define _access access
#endif
// access() mode flags
#ifndef F_OK
#define F_OK 0
#endif
// R_OK, W_OK, X_OK already defined in unistd.h

//================================================================
// OutputDebugStringA alias
//================================================================
#ifndef OutputDebugStringA
#define OutputDebugStringA OutputDebugString
#endif

//================================================================
// _beginthread / _endthread stubs
//================================================================
typedef void (*_beginthread_proc)(void*);
inline uintptr_t _beginthread(_beginthread_proc proc, unsigned /*stack_size*/, void* arglist) {
    pthread_t tid;
    // Wrap the void(*)(void*) as pthread expects void*(*)(void*)
    // This is a simplification; the thread function signature differs
    // Using a trampoline for safety
    struct _BegThreadArg {
        _beginthread_proc fn;
        void* arg;
    };
    // Note: This leaks the arg struct. For production use, the thread should free it.
    auto* wrap = new _BegThreadArg{proc, arglist};
    auto trampoline = [](void* p) -> void* {
        auto* a = static_cast<_BegThreadArg*>(p);
        a->fn(a->arg);
        delete a;
        return nullptr;
    };
    if (pthread_create(&tid, nullptr, trampoline, wrap) != 0) {
        delete wrap;
        return (uintptr_t)-1;
    }
    pthread_detach(tid); // _beginthread threads are detached
    return (uintptr_t)tid;
}

inline void _endthread() {
    pthread_exit(nullptr);
}

//================================================================
// Registry stubs (servers may check registry; no-op on Linux)
//================================================================
typedef void* HKEY;
#ifndef HKEY_LOCAL_MACHINE
#define HKEY_LOCAL_MACHINE ((HKEY)(uintptr_t)0x80000002)
#endif
#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS 0L
#endif
#ifndef KEY_READ
#define KEY_READ 0x20019
#endif
#ifndef REG_SZ
#define REG_SZ 1
#endif

inline LONG RegOpenKeyExA(HKEY, LPCSTR, DWORD, DWORD, HKEY*) { return 2L; /* ERROR_FILE_NOT_FOUND */ }
inline LONG RegQueryValueExA(HKEY, LPCSTR, DWORD*, DWORD*, BYTE*, DWORD*) { return 2L; }
inline LONG RegCloseKey(HKEY) { return ERROR_SUCCESS; }

#ifndef RegOpenKeyEx
#define RegOpenKeyEx RegOpenKeyExA
#endif
#ifndef RegQueryValueEx
#define RegQueryValueEx RegQueryValueExA
#endif

// Interlocked 64-bit operations already defined above (line ~475)

//================================================================
// File attributes (used by TerrainAttrib.cpp, etc.)
//================================================================
#ifndef FILE_ATTRIBUTE_READONLY
#define FILE_ATTRIBUTE_READONLY  0x00000001
#endif
#ifndef FILE_ATTRIBUTE_ARCHIVE
#define FILE_ATTRIBUTE_ARCHIVE   0x00000020
#endif
#ifndef FILE_ATTRIBUTE_NORMAL
#define FILE_ATTRIBUTE_NORMAL    0x00000080
#endif
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES  ((DWORD)-1)
#endif

inline DWORD GetFileAttributes(LPCSTR lpFileName) {
    struct stat st;
    if (stat(lpFileName, &st) != 0) return INVALID_FILE_ATTRIBUTES;
    DWORD attrs = FILE_ATTRIBUTE_NORMAL;
    if (!(st.st_mode & S_IWUSR)) attrs |= FILE_ATTRIBUTE_READONLY;
    return attrs;
}
#define GetFileAttributesA GetFileAttributes

inline BOOL SetFileAttributes(LPCSTR lpFileName, DWORD dwFileAttributes) {
    struct stat st;
    if (stat(lpFileName, &st) != 0) return FALSE;
    mode_t mode = st.st_mode;
    if (dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
        mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
    } else {
        mode |= S_IWUSR;
    }
    return (chmod(lpFileName, mode) == 0) ? TRUE : FALSE;
}
#define SetFileAttributesA SetFileAttributes

//================================================================
// Console stubs (used by util2.cpp CreateConsole)
//================================================================
typedef struct { short X; short Y; } COORD;

inline BOOL AllocConsole() { return TRUE; /* no-op on Linux */ }
inline BOOL SetConsoleTitle(LPCSTR) { return TRUE; }
#define SetConsoleTitleA SetConsoleTitle
inline BOOL SetConsoleScreenBufferSize(HANDLE, COORD) { return TRUE; }

//================================================================
// INI file reading (GetPrivateProfileString/Int)
// Simple implementation for Linux - parses standard INI format
//================================================================
inline DWORD GetPrivateProfileStringA(LPCSTR section, LPCSTR key, LPCSTR defaultVal,
                                       char* buffer, DWORD bufSize, LPCSTR filename) {
    if (!buffer || bufSize == 0) return 0;
    buffer[0] = 0;
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        if (defaultVal) { strncpy(buffer, defaultVal, bufSize); buffer[bufSize - 1] = 0; }
        return (DWORD)strlen(buffer);
    }
    char line[1024];
    bool inSection = false;
    while (fgets(line, sizeof(line), fp)) {
        char* nl = strchr(line, '\n'); if (nl) *nl = 0;
        nl = strchr(line, '\r'); if (nl) *nl = 0;
        if (line[0] == '[') {
            char* end = strchr(line, ']');
            if (end) { *end = 0; inSection = (strcasecmp(line + 1, section) == 0); }
        } else if (inSection) {
            char* eq = strchr(line, '=');
            if (eq) {
                *eq = 0;
                char* k = line; while (*k == ' ' || *k == '\t') k++;
                char* ke = eq - 1; while (ke > k && (*ke == ' ' || *ke == '\t')) { *ke = 0; ke--; }
                if (strcasecmp(k, key) == 0) {
                    char* v = eq + 1; while (*v == ' ' || *v == '\t') v++;
                    strncpy(buffer, v, bufSize); buffer[bufSize - 1] = 0;
                    fclose(fp); return (DWORD)strlen(buffer);
                }
            }
        }
    }
    fclose(fp);
    if (defaultVal) { strncpy(buffer, defaultVal, bufSize); buffer[bufSize - 1] = 0; }
    return (DWORD)strlen(buffer);
}
inline int GetPrivateProfileInt(LPCSTR section, LPCSTR key, int defaultVal, LPCSTR filename) {
    char buf[64], defStr[32];
    snprintf(defStr, sizeof(defStr), "%d", defaultVal);
    GetPrivateProfileStringA(section, key, defStr, buf, sizeof(buf), filename);
    return atoi(buf);
}
#define GetPrivateProfileString GetPrivateProfileStringA

//================================================================
// GetModuleFileName - returns path of current executable
//================================================================
inline DWORD GetModuleFileName(HMODULE, char* lpFilename, DWORD nSize) {
    ssize_t len = readlink("/proc/self/exe", lpFilename, nSize - 1);
    if (len == -1) return 0;
    lpFilename[len] = 0;
    return (DWORD)len;
}
#define GetModuleFileNameA GetModuleFileName

//================================================================
// MBCS (Multi-Byte Character Set) stubs
// Provided by include/mbstring.h shim - do not duplicate here
//================================================================

//================================================================
// pragma warning -- no-op on GCC/Clang
//================================================================
// #pragma warning(disable: ...) is MSVC-only; GCC/Clang use
// #pragma GCC diagnostic. We handle this per-file with #ifdef _MSC_VER.

#endif // PKO_PLATFORM_LINUX || PKO_PLATFORM_FREEBSD

#endif // PLATFORM_COMPAT_H
