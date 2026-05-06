// crtdbg.h - Linux compatibility shim for MSVC <crtdbg.h>
// MSVC's CRT debug header. On Linux, provide no-op stubs.
#ifndef _CRTDBG_H_LINUX_COMPAT
#define _CRTDBG_H_LINUX_COMPAT

#if defined(_WIN32)
#error "This shim should not be used on Windows - use the real crtdbg.h"
#endif

// Debug memory leak detection flags (no-op on Linux)
#define _CRTDBG_ALLOC_MEM_DF        0x01
#define _CRTDBG_DELAY_FREE_MEM_DF   0x02
#define _CRTDBG_CHECK_ALWAYS_DF      0x04
#define _CRTDBG_CHECK_CRT_DF        0x10
#define _CRTDBG_LEAK_CHECK_DF       0x20

// _CrtSetDbgFlag - No-op on Linux
#ifndef _CrtSetDbgFlag
#define _CrtSetDbgFlag(flag) (0)
#endif

// _CrtDumpMemoryLeaks - No-op on Linux
#ifndef _CrtDumpMemoryLeaks
#define _CrtDumpMemoryLeaks() (0)
#endif

// _CrtSetReportMode / _CrtSetReportFile - No-op on Linux
#define _CRT_WARN   0
#define _CRT_ERROR  1
#define _CRT_ASSERT 2
#define _CRTDBG_MODE_FILE 0x01
#define _CRTDBG_FILE_STDERR ((__FILE*)2)

#ifndef _CrtSetReportMode
#define _CrtSetReportMode(type, mode) (0)
#endif
#ifndef _CrtSetReportFile
#define _CrtSetReportFile(type, file) (0)
#endif

// _ASSERTE macro
#include <cassert>
#ifndef _ASSERTE
#define _ASSERTE(expr) assert(expr)
#endif

#endif // _CRTDBG_H_LINUX_COMPAT
