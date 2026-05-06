// tchar.h - Linux compatibility shim for MSVC tchar.h
// This file is only used on Linux builds. On Windows, the real tchar.h is used.
#ifndef _TCHAR_H_LINUX_COMPAT
#define _TCHAR_H_LINUX_COMPAT

#if defined(_WIN32)
#error "This shim should not be used on Windows - use the real tchar.h"
#endif

#include <cstring>
#include <cstdlib>

#ifndef _TCHAR_DEFINED
#define _TCHAR_DEFINED
typedef char TCHAR;
typedef char _TCHAR;
#endif

// String functions
#ifndef _tcslen
#define _tcslen     strlen
#endif
#ifndef _tcscpy
#define _tcscpy     strcpy
#endif
#ifndef _tcsncpy
#define _tcsncpy    strncpy
#endif
#ifndef _tcscat
#define _tcscat     strcat
#endif
#ifndef _tcscmp
#define _tcscmp     strcmp
#endif
#ifndef _tcsicmp
#define _tcsicmp    strcasecmp
#endif
#ifndef _tcschr
#define _tcschr     strchr
#endif
#ifndef _tcsrchr
#define _tcsrchr    strrchr
#endif
#ifndef _tcsstr
#define _tcsstr     strstr
#endif
#ifndef _tcstol
#define _tcstol     strtol
#endif
#ifndef _tcstod
#define _tcstod     strtod
#endif
#ifndef _tprintf
#define _tprintf    printf
#endif
#ifndef _stprintf
#define _stprintf   sprintf
#endif
#ifndef _sntprintf
#define _sntprintf  snprintf
#endif
#ifndef _vstprintf
#define _vstprintf  vsprintf
#endif
#ifndef _vsntprintf
#define _vsntprintf vsnprintf
#endif
#ifndef _tfopen
#define _tfopen     fopen
#endif
#ifndef _ttoi
#define _ttoi       atoi
#endif
#ifndef _ttol
#define _ttol       atol
#endif

// _TEXT / _T macro
#ifndef _TEXT
#define _TEXT(x) x
#endif
#ifndef _T
#define _T(x) x
#endif
#ifndef TEXT
#define TEXT(x) x
#endif

#endif // _TCHAR_H_LINUX_COMPAT
