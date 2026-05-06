// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define PKO_PLATFORM_WINDOWS 1
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include "serversdk/platform_compat.h"
#endif

#include <span>
#include <iostream>
#if defined(_WIN32) || defined(_WIN64)
#include <tchar.h>
#endif
#include <math.h>
#include "stdio.h"
#include "stdlib.h"

#include "dbccommon.h"
#include "util.h"
#include "tryutil.h"

#include "i18n.h"

extern "C" {
#include "luajit.h"
}

// Add by lark.li 20080730 begin
#include "pi_Alloc.h"
#include "pi_Memory.h"
// End
