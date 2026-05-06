#pragma once

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include "serversdk/platform_compat.h"
#include <pthread.h>
#endif

class Thread {
public:
	Thread();
	virtual ~Thread();

	bool Begin(int flag = 0);
	bool Resume();
	bool Suspend();
	bool Terminate();
	int Wait(int time = INFINITE);
	bool SetPriority(int priority);
	int GetPriority();

	virtual unsigned Run() = 0;

private:
#if defined(_WIN32) || defined(_WIN64)
	HANDLE m_thread;
	unsigned m_threadid;
	friend unsigned __stdcall ThreadFunc(void* param);
#else
	pthread_t m_thread;
	bool m_started;
	friend void* ThreadFunc_Linux(void* param);
#endif
};
