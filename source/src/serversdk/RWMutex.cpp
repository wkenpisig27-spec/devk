//================================================================
// It must be permitted by Dabo.Zhang that this program is used for
// any purpose in any situation.
// Copyright (C) Dabo.Zhang 2000-2003
// All rights reserved by ZhangDabo.
// This program is written(created) by Zhang.Dabo in 2000.3
// This program is modified recently by Zhang.Dabo in 2003.7
//=================================================================

#include "RWMutex.h"
#include "TLSIndex.h"

_DBC_USING

//==============================================================================
class AccThread : public PreAllocStru {
public:
	AccThread(uLong size) : PreAllocStru(size), m_rwsync(0), m_RDCount(0), m_WRCount(0), nextthread(0){};
	~AccThread() {
		if (nextthread)
			delete nextthread;
	}

public:
	void Initially() {
		m_rwsync = 0;
		m_RDCount = m_WRCount = 0;
	}
	void Finally() { nextthread = 0; }

	RWMutex* m_rwsync;
	volatile int m_RDCount;
	volatile int m_WRCount;
	AccThread* nextthread;
};
typedef AccThread* PAccThread;
static TLSIndex st_tls;
static PreAllocHeapPtr<AccThread> st_FreeAccThread(0, 20);
//==============================================================================
void RWMutex::BeginRead() {
	AccThread *l_AccThread, *l_bakAccThread;

	l_bakAccThread = l_AccThread = (PAccThread)st_tls.GetPointer();
	while (l_AccThread) {
		if (l_AccThread->m_rwsync == this) {
			break;
		}
		l_AccThread = l_AccThread->nextthread;
	}
	if (!l_AccThread) {
		l_AccThread = st_FreeAccThread.Get();
		l_AccThread->m_rwsync = this;
		l_AccThread->nextthread = l_bakAccThread;
		st_tls.SetPointer(l_AccThread);
	}

	m_mtxRWCount.lock();
	try {
		while (m_WRCount && (!l_AccThread->m_WRCount)) { // ��д�̲߳��Ҳ��ǵ�ǰ�߳���д,�������ڶ�����
			m_mtxRWCount.unlock();
			try {
				m_semRead.lock(); // �������߳�
			} catch (...) {
			}
			m_mtxRWCount.lock();
		}
		m_semRead.unlock(); // �ͷ����������̲߳�������

		l_AccThread->m_RDCount++;
		m_RDCount++;
	} catch (...) {
	}
	m_mtxRWCount.unlock();
};
//==============================================================================
void RWMutex::BeginWrite() {
	AccThread *l_AccThread, *l_bakAccThread;

	l_bakAccThread = l_AccThread = (PAccThread)st_tls.GetPointer();
	while (l_AccThread) {
		if (l_AccThread->m_rwsync == this) {
			break;
		}
		l_AccThread = l_AccThread->nextthread;
	}
	if (!l_AccThread) {
		l_AccThread = st_FreeAccThread.Get();
		l_AccThread->m_rwsync = this;
		l_AccThread->nextthread = l_bakAccThread;
		st_tls.SetPointer(l_AccThread);
	}

	m_mtxRWCount.lock();
	try {
		while ((m_WRCount && (!l_AccThread->m_WRCount)) ||			   // ��д�̲߳��Ҳ��ǵ�ǰ�߳���д����
			   (m_RDCount && (m_RDCount != l_AccThread->m_RDCount))) { // �ж��̲߳��Ҳ�ֻ��ǰ�߳��ڶ�,��������д����
			m_mtxRWCount.unlock();
			try {
				m_semWrite.lock(); // ����д�߳�
			} catch (...) {
			}
			m_mtxRWCount.lock();
		}

		l_AccThread->m_WRCount++;
		m_WRCount++;
	} catch (...) {
	}
	m_mtxRWCount.unlock();
};
//==============================================================================
void RWMutex::EndRead() {
	AccThread *l_AccThread, *l_bakAccThread;

	l_bakAccThread = 0;
	l_AccThread = (PAccThread)st_tls.GetPointer();
	while (l_AccThread) {
		if (l_AccThread->m_rwsync == this) {
			break;
		}
		l_bakAccThread = l_AccThread;
		l_AccThread = l_AccThread->nextthread;
	}

	// ����Ϊ�գ���ֱ�ӷ���
	if (l_AccThread == nullptr)
		return;

	m_mtxRWCount.lock();
	try {
		m_RDCount--;
		l_AccThread->m_RDCount--;
		m_semWrite.unlock(); // ���߳�,�ͷ�д��,��д�����

		if (!l_AccThread->m_RDCount && !l_AccThread->m_WRCount) { // ���һ�β���
			m_mtxRWCount.unlock();

			if (l_bakAccThread) {
				l_bakAccThread->nextthread = l_AccThread->nextthread;
			} else {
				st_tls.SetPointer(l_AccThread->nextthread);
			}
			l_AccThread->Free();
		} else {
			m_mtxRWCount.unlock();
		}
	} catch (...) {
		m_mtxRWCount.unlock();
	}
};
//==============================================================================
void RWMutex::EndWrite() {
	AccThread *l_AccThread, *l_bakAccThread;

	l_bakAccThread = 0;
	l_AccThread = (PAccThread)st_tls.GetPointer();
	while (l_AccThread) {
		if (l_AccThread->m_rwsync == this) {
			break;
		}
		l_bakAccThread = l_AccThread;
		l_AccThread = l_AccThread->nextthread;
	}

	// ����Ϊ�գ���ֱ�ӷ���
	if (l_AccThread == nullptr)
		return;

	m_mtxRWCount.lock();
	try {
		m_WRCount--;
		l_AccThread->m_WRCount--;
		m_semWrite.unlock();									  // �ͷ�д��
		m_semRead.unlock();										  // �ͷŶ���
		if (!l_AccThread->m_WRCount && !l_AccThread->m_RDCount) { // ���һ�β���
			m_mtxRWCount.unlock();
			if (l_bakAccThread) {
				l_bakAccThread->nextthread = l_AccThread->nextthread;
			} else {
				st_tls.SetPointer(l_AccThread->nextthread);
			}
			l_AccThread->Free();
		} else {
			m_mtxRWCount.unlock();
		}
	} catch (...) {
		m_mtxRWCount.unlock();
	}
};
