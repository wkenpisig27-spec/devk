#include "DataSocket.h"
#include "PacketQueue.h"
#include "../../include/util/log.h"

_DBC_USING

// #define _NOSERVER
PreAllocHeapPtr<PKItem> PKQueue::m_heap(1, 100);

void LimitCurrentProcTest() {
	HANDLE hCurrentProcess = GetCurrentProcess();

	// Get the processor affinity mask for this process
	DWORD_PTR dwProcessAffinityMask = 0;
	DWORD_PTR dwSystemAffinityMask = 0;

	if (GetProcessAffinityMask(hCurrentProcess, &dwProcessAffinityMask, &dwSystemAffinityMask) != 0 && dwProcessAffinityMask) {
		// Find the lowest processor that our process is allows to run against
		DWORD_PTR dwAffinityMask = (dwProcessAffinityMask & ((~dwProcessAffinityMask) + 1));

		// Set this as the processor that our thread must always run against
		// This must be a subset of the process affinity mask
		HANDLE hCurrentThread = GetCurrentThread();
		if (INVALID_HANDLE_VALUE != hCurrentThread) {
			SetThreadAffinityMask(hCurrentThread, dwAffinityMask);
			CloseHandle(hCurrentThread);
		}
	}

	CloseHandle(hCurrentProcess);
}

PKQueue::PKQueue(bool mode /* =true*/) : m_head(0), m_tail(0), m_isclose(false), m_mode(mode), m_pktotal(0) {
	// LimitCurrentProcTest();
	m_mut.Create(false);
	if (!m_mode) {
		m_sem.Create(0, 0x7FFFFFFF);
	}
	PKItem* l_it = m_heap.Get();
	l_it->Free();
}

WPacket PKQueue::SyncPK(DataSocket* datasock, RPacket& in_para, uLong ulMilliseconds) {
	uLong l_tick = GetTickCount() - in_para.GetTickCount();
	if (ulMilliseconds > l_tick) {
		ulMilliseconds = ulMilliseconds - l_tick;

		PKItem* l_item;
		MutexArmor l(m_mut);
		if (m_isclose) {
			l.unlock();
			return 0;
		}
		l_item = m_heap.Get();
		if (m_tail) {
			m_tail->m_next = l_item;
			m_tail = l_item;
		} else {
			m_head = m_tail = l_item;
		}
		l_item->m_next = 0;
		l_item->m_datasock = datasock;
		l_item->m_inpk = in_para;
		l_item->m_iscall = 1;
		++m_pktotal;
		l_item->m_semWait.trylock();
		if (!m_mode)
			m_sem.unlock();
		l.unlock();

		DWORD l_sem = l_item->m_semWait.timelock(ulMilliseconds);
		MutexArmor ll(m_mut);
		if ((l_sem == WAIT_OBJECT_0) || (l_item->m_semWait.trylock() == WAIT_OBJECT_0)) //(l_item->m_iscall ==2)
		{
			if (l_item->m_iscall != 2) {
				LG("comm", "PKQueue::SyncPK: sync issue with [volatile char m_iscall] between threads\n");
			}
			ll.unlock();
			WPacket l_retpk = l_item->m_retpk;
			if (l_retpk.HasData() == 0) {
				LG("comm", "PKQueue::SyncPK return packet has no data\n");
			}
			l_item->Free();
			return l_retpk;
		} else {
			l_item->m_iscall = 2;
			ll.unlock();
			LG("comm", "PKQueue::SyncPK timeout: %u ms\n", ulMilliseconds);
			return 0;
		}
	} else {
		LG("comm", "PKQueue::SyncPK timeout (already timed out): %u ms\n", l_tick);
		return 0;
	}
}
void PKQueue::AddPK(DataSocket* datasock, RPacket& pk) {
	MutexArmor l(m_mut);
	if (m_isclose) {
		l.unlock();
		return;
	}
	PKItem* l_item = m_heap.Get();
	if (m_tail) {
		m_tail->m_next = l_item;
		m_tail = l_item;
	} else {
		m_head = m_tail = l_item;
	}
	l_item->m_next = 0;
	l_item->m_datasock = datasock;
	l_item->m_inpk = pk;
	l_item->m_iscall = 0;
	l_item->m_dwLastTime = GetTickCount();
	++m_pktotal;
	if (!m_mode)
		m_sem.unlock();
	l.unlock();
}
PKItem* PKQueue::GetPKItem(uLong end, bool isServer) {
	PKItem* l_item = 0;
	DWORD l_ret = GetTickCount();
	if (!isServer) {
		if (m_isclose || m_head == nullptr)
			return nullptr;
	}
	if (m_mode || ((end > l_ret) ? ((l_ret = m_sem.timelock(end - l_ret)) == WAIT_OBJECT_0) : ((l_ret = m_sem.trylock()) == WAIT_OBJECT_0))) {
		MutexArmor l(m_mut);
		if (!m_isclose && m_head) {
			// if( GetTickCount() - m_head->m_dwLastTime < 500 )
			//{
			//	m_sem.unlock();
			//	return nullptr;
			// }

			--m_pktotal;
			l_item = m_head;
			if (m_head == m_tail) {
				m_head = m_tail = 0;
			} else {
				m_head = m_head->m_next;
			}
		}
		l.unlock();
	}
	return l_item;
}

/*-----------------------------------------------------------------------
  �ͻ��˵ķ������ Add by Waiting 2009-06-18
  ���Խ����ÿ�ο��������ʱ����Ҫ��������ٺ��룬��ɻ���ͣ��
  �Ľ�����1.�������̴���������Ҫ��ʱ�� 2.��������̴߳����������
-----------------------------------------------------------------------*/
// #define __CLIENT_PK_TEST__
#ifdef __CLIENT_PK_TEST__
DWORD g_dwRecvTime = 0;
DWORD g_dwLoopCount = 0;
#endif
void PKQueue::PeekPacket(uLong sleep, bool isServer) {
	if (m_isclose)
		return;
#ifdef __CLIENT_PK_TEST__
	g_dwRecvTime = timeGetTime();
	g_dwLoopCount = 0;
#endif
	uLong l_tick = GetTickCount() + sleep;
	for (PKItem* l_item = GetPKItem(l_tick, isServer); l_item; l_item = GetPKItem(l_tick)) {
#ifdef __CLIENT_PK_TEST__
		g_dwLoopCount++;
#endif
		// LogLine l_line(g_dbclog);
		// l_line<<newln<<"PKQueue::PeekPacket:cmd=" <<l_item->m_inpk.ReadCmd();

		switch (l_item->m_iscall) {
		case 0: {
			if (isServer) {
				try {
					ProcessData(l_item->m_datasock, l_item->m_inpk);
				} catch (...) {
					l_item->Free();
					throw;
				}
			} else {
				ProcessData(l_item->m_datasock, l_item->m_inpk);
			}
			l_item->Free();
			break;
		}
		case 1: {
			try {
				l_item->m_retpk = ServeCall(l_item->m_datasock, l_item->m_inpk);
			} catch (...) {
				LG("comm", "PKQueue::PeekPacket exception\n");
				MutexArmor l(m_mut);
				if (l_item->m_iscall == 1) {
					l_item->m_retpk = 0;
					l_item->m_iscall = 2;
					l_item->m_semWait.unlock();
				} else {
					l_item->Free();
				}
				l.unlock();
				throw;
			}
			MutexArmor l(m_mut);
			if (l_item->m_iscall == 1) {
				l_item->m_iscall = 2;
				l_item->m_semWait.unlock();
			} else {
				l_item->Free();
			}
			l.unlock();
			break;
		}
		default: {
			l_item->Free();
			break;
		}
		}
		if (!isServer) {
			if (GetTickCount() > l_tick)
				break;
		}
	}
}
void PKQueue::CloseQueue() {
	MutexArmor l(m_mut);
	m_isclose = true;
	for (PKItem* l_item = GetPKItem(0); l_item; l_item = GetPKItem(0)) {
		if (l_item->m_iscall) {
			l_item->m_semWait.unlock();
		}
		l_item->Free();
	}
	l.unlock();
}
