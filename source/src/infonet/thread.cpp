

#include "thread.h"

#if defined(_WIN32) || defined(_WIN64)

#include <process.h>

unsigned __stdcall ThreadFunc(void* param) {
	try {
		Thread* pThread = (Thread*)param;
		unsigned retVal = pThread->Run();
		_endthreadex(retVal);
		return retVal;
	} catch (...) {
	}
	return -1;
}

Thread::Thread() {
}

Thread::~Thread() {
}

bool Thread::Begin(int flag) {
	m_thread = (HANDLE)_beginthreadex(nullptr, 0, ThreadFunc, this, flag, &m_threadid);
	return nullptr != m_thread;
}

bool Thread::Resume() {
	return -1 != ResumeThread((HANDLE)m_thread);
}

bool Thread::Suspend() {
	return -1 != SuspendThread((HANDLE)m_thread);
}

bool Thread::Terminate() {
	return TRUE == TerminateThread((HANDLE)m_thread, 0);
}

int Thread::Wait(int time) {
	return (int)WaitForSingleObject((HANDLE)m_thread, time);
}

bool Thread::SetPriority(int priority) {
	return TRUE == SetThreadPriority((HANDLE)m_thread, priority);
}

int Thread::GetPriority() {
	return GetThreadPriority((HANDLE)m_thread);
}

#else // Linux

#include <cstdint>

void* ThreadFunc_Linux(void* param) {
	try {
		Thread* pThread = (Thread*)param;
		unsigned retVal = pThread->Run();
		return (void*)(uintptr_t)retVal;
	} catch (...) {
	}
	return (void*)(uintptr_t)-1;
}

Thread::Thread() : m_thread(0), m_started(false) {
}

Thread::~Thread() {
}

bool Thread::Begin(int /*flag*/) {
	int ret = pthread_create(&m_thread, nullptr, ThreadFunc_Linux, this);
	m_started = (ret == 0);
	return m_started;
}

bool Thread::Resume() {
	return true; // No-op on Linux
}

bool Thread::Suspend() {
	return true; // No-op on Linux
}

bool Thread::Terminate() {
	if (m_started) {
		pthread_cancel(m_thread);
		m_started = false;
	}
	return true;
}

int Thread::Wait(int time) {
	if (!m_started) return WAIT_OBJECT_0;
	if (time == (int)INFINITE) {
		pthread_join(m_thread, nullptr);
		m_started = false;
		return WAIT_OBJECT_0;
	}
	// Timed wait
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += time / 1000;
	ts.tv_nsec += (time % 1000) * 1000000L;
	if (ts.tv_nsec >= 1000000000L) { ts.tv_sec++; ts.tv_nsec -= 1000000000L; }
	int ret = pthread_timedjoin_np(m_thread, nullptr, &ts);
	if (ret == 0) {
		m_started = false;
		return WAIT_OBJECT_0;
	}
	return WAIT_TIMEOUT;
}

bool Thread::SetPriority(int /*priority*/) {
	return true; // No-op on Linux
}

int Thread::GetPriority() {
	return 0; // Default normal priority
}

#endif
