// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#if defined(_WIN32) || defined(_WIN64)
#define PKO_PLATFORM_WINDOWS 1
#define NOCRYPT
#include <winsock2.h>
#include <windows.h>
#include <oleauto.h>
#else
#include "serversdk/platform_compat.h"
#endif

#include <iostream>
#if defined(_WIN32) || defined(_WIN64)
#include <tchar.h>
#endif
#include <cassert>

// TODO: reference additional headers your program requires here
