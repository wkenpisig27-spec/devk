#pragma once

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include "serversdk/platform_compat.h"
#endif

class Lock {
public:
	Lock();
	~Lock();
	void lock();
	void unlock();

private:
	CRITICAL_SECTION m_cs;
};
