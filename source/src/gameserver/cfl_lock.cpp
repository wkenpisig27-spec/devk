#include "stdafx.h"
#ifdef PKO_PLATFORM_WINDOWS
#include <windows.h>
#include <winbase.h>
#endif
#include "cfl_lock.h"
#include <iostream>
using namespace std;

#ifdef _MSC_VER
#pragma warning(disable : 4800)
#endif

// cfl_spinlock
cfl_spinlock::cfl_spinlock() {
	::InitializeCriticalSectionAndSpinCount(&_cs, 4000 | 0x80000000);
}

cfl_spinlock::~cfl_spinlock() { ::DeleteCriticalSection(&_cs); }

void cfl_spinlock::lock() { ::EnterCriticalSection(&_cs); }

bool cfl_spinlock::trylock() { return (bool)::TryEnterCriticalSection(&_cs); }

void cfl_spinlock::unlock() { ::LeaveCriticalSection(&_cs); }
