// windows.h - Linux compatibility shim for Windows.h
// This file is only used on Linux builds. Redirects to platform_compat.h.
#ifndef _WINDOWS_H_LINUX_COMPAT
#define _WINDOWS_H_LINUX_COMPAT

#if defined(_WIN32)
#error "This shim should not be used on Windows - use the real windows.h"
#endif

// platform_compat.h provides all Windows type/function equivalents for Linux
#include "serversdk/platform_compat.h"

#endif // _WINDOWS_H_LINUX_COMPAT
