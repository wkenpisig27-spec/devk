//================================================================
// It must be permitted by Dabo.Zhang that this program is used for
// any purpose in any situation.
// Copyright (C) Dabo.Zhang 2000-2003
// All rights reserved by ZhangDabo.
// This program is written(created) by Zhang.Dabo in 2000.3
// This program is modified recently by Zhang.Dabo in 2003.7
//=================================================================
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "platform_compat.h"

#include "DBCCommon.h"
#include "excp.h"
#include "PreAlloc.h"

_DBC_BEGIN
#pragma pack(push)
#pragma pack(4)

#ifdef USING_IOCP
// Delete by lark.li 20081103

#else // NOT_USE_IOCP************************************************************************
/*
	�����ǲ���ʹ��IOCPƽ̨���̳߳�ʵ��
*/
class Task;
class TaskQue;
class Thread;
class ThrdQue;
class TaskWait;

#define NOTWAITSTATE (0x3FFFFFFF)

/**
 * @file ThreadPool.H
 * @class ThreadPool
 * @author ZhangDabo
 * @brief �̳߳ض���,ʹ��CreatePool���гصĴ�������ֱ�ӵ��ù��캯��
 * @bug
 */
class ThreadPool // �̳߳ض���
{
	friend class Thread;

public:
	/**
	 * @brif ����һ���̳߳�
	 * @param[in]  idle  ���е��߳���
	 * @param[in]  max  �����߳���
	 * @param[in]  taskmaxnum  ���������
	 * @param[in]  nPriority  �߳����ȼ�
	 *
	 * @return ThreadPool �����µ��߳�
	 *
	 * #exception EXCP �̳߳س�ʼ���̶߳��������ڴ�ʧ��
	 */
	static ThreadPool* CreatePool(int idle = 0, int max = 0x10, int taskmaxnum = 0x100, int nPriority = THREAD_PRIORITY_NORMAL);

	/**
	 * @brif ɾ��һ���̳߳�
	 */
	void DestroyPool();

	/**
	 * @brif ׷��һ������
	 * @param[in]  task  ׷�ӵ�����
	 */
	void AddTask(Task* task);

	/**
	 * @brif ׷��һ������
	 * @param[in]  task  �ȴ�������
	 */
	int WaitForTask(Task* task);

	/**
	 * @brif �õ��̳߳�������
	 */
	int GetTaskCount();

	/**
	 * @brif
	 */
	int getCurr() { return m_curr; }		  // ���ܼ��Ҫ��
	int getCurrFree() { return m_currfree; } // ���ܼ��Ҫ��

	InterLockedLong m_taskexitflag;

private:
	ThreadPool(int idle, int max, int taskmaxnum, int nPriority);
	virtual ~ThreadPool();
	void ThreadExcute(Thread*);
	inline Task* PoolProcess(Thread* l_myself);

private:
	InterLockedLong m_exitflag;
	const int m_nPriority;
	const int m_max, m_idle;
	volatile int m_curr, m_currfree;

	TaskQue* volatile m_taskQue;
	ThrdQue* volatile m_thrdQue;
};

/**
 * @file ThreadPool.H
 * @class Task
 * @author ZhangDabo
 * @brief ��������࣬���̳߳�ʹ��
 * @bug
 */
class Task { // ���������
	friend class ThreadPool;
	friend class TaskQue;

public:
	/**
	 * @brif ���캯��
	 */
	Task() : __canwait(false), __ThisThread(0), __TaskQue(0), __nexttask(0), __taskwait(0) {
		__mtxtaskwait.Create(false);
		if (!__mtxtaskwait) {
			// THROW_EXCP(excpSync,"�������������ϵͳͬ���������");
			throw excpSync("Create sync mutex failed!");
		}
	};
	/**
	 * @brif ȡ���˳���־
	 */
	bool GetExitFlag();

protected:
	/**
	 * @brif ���캯��
	 */
	virtual ~Task() {}

	/**
	 * @brif �ͷ���Դ
	 */
	virtual Task* Lastly() {
		Free();
		return 0;
	}

	/**
	 * @brif ȡ���߳̾��
	 *
	 * @return HANDLE �߳̾��
	 */
	HANDLE GetHandle(); // ��ȡ�����̵߳ľ��

	/**
	 * @brif ��ȡ�����̵߳ı�־ID
	 *
	 * @return DWORD �̵߳ı�־ID
	 */
	DWORD GetThreadID(); // ��ȡ�����̵߳ı�־ID
private:
	virtual void Free() { delete this; }
	virtual int Process() = 0;
	inline Task* TaskExec(Thread* ThisThread);
	int WaitMe();

	Thread* volatile __ThisThread;
	TaskQue* volatile __TaskQue;

	bool volatile __canwait;
	Mutex __mtxtaskwait;
	TaskWait* volatile __taskwait;
	static PreAllocHeapPtr<TaskWait> __freetaskwait;

	Task* volatile __nexttask;
};

/**
 * @file ThreadPool.H
 * @class PreAllocTask
 * @author ZhangDabo
 * @brief �ڴ�Ԥ�����������
 * @bug
 */
class PreAllocTask : public PreAllocStru, public Task {
public:
	/**
	 * @brif ���캯��
	 * @param[in]  size  Ԥ����Ĵ�С
	 */
	PreAllocTask(uLong size) : PreAllocStru(size){};

	/**
	 * @brif �ͷ���Դ
	 */
	virtual void Free() { PreAllocStru::Free(); }
};
#endif // USING_IOCP
#pragma pack(pop)
_DBC_END
#endif