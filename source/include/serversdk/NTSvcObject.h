//================================================================
// It must be permitted by Dabo.Zhang that this program is used for
// any purpose in any situation.
// Copyright (C) Dabo.Zhang 2000-2003
// All rights reserved by ZhangDabo.
// This program is written(created) by Zhang.Dabo in 2000.3
// This program is modified recently by Zhang.Dabo in 2003.7
//=================================================================
#ifndef NTSVCOBJECT_H
#define NTSVCOBJECT_H

#include "DBCCommon.h"
#include "dstring.h"
#include "EventMsgRC.h"

#ifdef PKO_PLATFORM_WINDOWS
// NTSvcObject is entirely Windows-specific (NT Service APIs)
// On Linux, servers run as daemons via systemd/init.d instead

_DBC_BEGIN
#pragma pack(push)
#pragma pack(4)

#define SERVICE_CONTROL_INIT 188
#define SERVICE_CONTROL_PROCESS 189
#define SERVICE_CONTROL_EXIT 190

#define TE_SUCC char(0)
#define TE_INFO char(1)
#define TE_WARN char(2)
#define TE_ERR char(3)
//=======NTSvcObject========================================================================
typedef class NTSvcObject {
	NTSvcObject();
	virtual ~NTSvcObject() { delete m_nextservice; };

	virtual void ServiceStart() = 0;
	virtual void ServiceStop() = 0;
	virtual void ServicePause() = 0;
	virtual void ServiceContinue() = 0;
	virtual int ServiceProcess() = 0;

	virtual cChar* SetSvcName() const = 0;
	virtual cChar* SetDispName() const = 0;
	virtual cChar* SetDependencies() const = 0;
	virtual cChar* SetRCFile() const = 0;
	virtual DWORD SetNumOfEvtCat() const = 0;

	virtual bool CanPaused() const = 0;
	virtual bool RelyMainThread() const = 0;
	virtual bool HasAppEvent() const = 0;
	virtual bool HasSecEvent() const = 0;
	virtual bool HasSysEvent() const = 0;

	inline void ServiceExit(DWORD, DWORD);
	void Trace_Event(uChar Severity, cChar* msg) const; // Severity�����׵�SVRT�궨��

	friend class NTServiceSet;
	friend class NTService;
	template <class T>
	friend class GenNTService;

	BOOL NotifyState(DWORD CurrentState, DWORD WaitHint = 5000, DWORD Win32ExitCode = NO_ERROR, DWORD ServiceSpecificExitCode = 0);
	// ���ݳ�Ա:
	int m_delay; // ServiceProcess���ص��ӳ�
	Sema m_semSignal1, m_semSignal2;
	HANDLE m_hAppEvtSrc, m_hSecEvtSrc, m_hSysEvtSrc;
	volatile bool m_EndSvcFlag, m_EndMainFlag, m_ServiceExitFlag;

	LPSERVICE_MAIN_FUNCTION m_ServiceMain;
	SERVICE_STATUS_HANDLE m_hServiceStatus;
	volatile DWORD m_dwCurrentState;
	DWORD m_dwCheckPoint;
	DWORD m_SvcArgc;
	LPTSTR* m_SvcArgv;
	NTSvcObject* m_nextservice;
}* PNTSVC;
//=======NTServiceSet====================================================================
class NTServiceSet {
public:
	NTServiceSet() : m_svclen(0), m_svchead(0), m_svctail(0), m_argc(0) {
		if (!m_svcHandle.Create(false))
			throw excpSync("����ͬ���������");
	};
	~NTServiceSet() {
		delete m_svchead;
	};
	int Run();

private:
	void AddService(PNTSVC svc) {
		m_svcHandle.lock();
		try {
			svc->m_nextservice = 0;
			if (m_svctail) {
				m_svctail->m_nextservice = svc;
				m_svctail = svc;
			} else {
				m_svchead = m_svctail = svc;
			}
			m_svclen++;
		} catch (...) {
		}
		m_svcHandle.unlock();
	};
	void DelService(PNTSVC svc) {
		PNTSVC l_last = 0, l_svc;
		m_svcHandle.lock();
		try {
			l_svc = m_svchead;
			for (; l_svc && l_svc != svc; l_last = l_svc, l_svc = l_svc->m_nextservice)
				;
			if (!l_svc)
				return;
			if (l_last)
				l_last->m_nextservice = l_svc->m_nextservice;
			else
				m_svchead = l_svc->m_nextservice;
			if (m_svctail == l_svc)
				m_svctail = l_last;
			l_svc->m_nextservice = 0;
			m_svclen--;
		} catch (...) {
		}
		m_svcHandle.unlock();
	}
	int RegSvc() const;
	int UnRegSvc() const;
	int RegEventLog(NTSvcObject* svc) const;
	void UnRegEventLog(NTSvcObject* svc) const;

	friend class NTSvcObject;
	friend class NTService;
	template <class T>
	friend class GenNTService;

	Mutex m_svcHandle;
	int m_svclen;
	PNTSVC m_svchead, m_svctail;

	int m_argc;	   // argc����
	char** m_argv; // argv����
};
extern NTServiceSet g_svcset;
//=======GenNTService=======================================================================
template <class T>
class GenNTService {
public:
	GenNTService() {
		m_svc = new T;
		m_svc->m_ServiceMain = GenNTService<T>::SvcMain;
		g_svcset.AddService(m_svc);
	};
	~GenNTService() {
		g_svcset.DelService(m_svc);
		delete m_svc;
	};

private:
	static VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR* lpszArgv);
	static VOID WINAPI Handler(DWORD fdwControl);
	static PNTSVC m_svc;
};
//=====������������=========================================================================
inline void NTSvcObject::ServiceExit(DWORD Win32ExitCode, DWORD ServiceSpecificExitCode) {
	m_ServiceExitFlag = true;
	NotifyState(SERVICE_STOP_PENDING, 20 * 1000, Win32ExitCode, ServiceSpecificExitCode);
};
//==============================================================================
template <class T>
VOID WINAPI GenNTService<T>::SvcMain(DWORD dwArgc, LPTSTR* lpszArgv) {
	g_svcset.AddService(m_svc);

	m_svc->m_SvcArgc = dwArgc;
	m_svc->m_SvcArgv = lpszArgv;

	m_svc->m_semSignal1.trylock(); // ���źŸ�ˢ��
	m_svc->m_delay = 0;
	m_svc->m_EndSvcFlag = m_svc->m_EndMainFlag = m_svc->m_ServiceExitFlag = false;
	char l_buf[256];
	if (m_svc->HasAppEvent()) {
		strcpy(l_buf, m_svc->SetDispName());
		m_svc->m_hAppEvtSrc = RegisterEventSource(0, l_buf);
	}
	if (m_svc->HasSecEvent()) {
		strcpy(l_buf, m_svc->SetDispName());
		strcat(l_buf, ".SEC");
		m_svc->m_hSecEvtSrc = RegisterEventSource(0, l_buf);
	}
	if (m_svc->HasSysEvent()) {
		strcpy(l_buf, m_svc->SetDispName());
		strcat(l_buf, ".SYS");
		m_svc->m_hSysEvtSrc = RegisterEventSource(0, l_buf);
	}
	m_svc->m_hServiceStatus = RegisterServiceCtrlHandler(
		m_svc->SetSvcName(), GenNTService<T>::Handler);

	SERVICE_STATUS l_svcstat;
	SC_HANDLE l_sc = 0, l_sv = 0;
	if (m_svc->RelyMainThread()) { // �������߳�
		l_sc = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
		l_sv = OpenService(l_sc, m_svc->SetSvcName(), SERVICE_USER_DEFINED_CONTROL);
	}
	m_svc->NotifyState(SERVICE_START_PENDING, 20 * 1000);

	DWORD l_CurrentState;
	while (true) {
		l_CurrentState = m_svc->m_dwCurrentState;
		switch (l_CurrentState) {
		case SERVICE_START_PENDING:
			if (m_svc->RelyMainThread()) { // �������߳�
				ControlService(l_sv, SERVICE_CONTROL_INIT, &l_svcstat);
				//				m_svc->m_semSignal1.lock();//�ȴ����̳߳�ʼ���������
			} else {
				try {
					m_svc->ServiceStart();
					m_svc->Trace_Event(TE_SUCC, dstring("����:") << m_svc->SetDispName() << " �Ѿ�����");
					m_svc->NotifyState(SERVICE_RUNNING);
				} catch (std::exception const& e) {
					m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " ���������з��������쳣,����ֻ���˳�,�쳣����������Ϣ:" << e.what());
					m_svc->NotifyState(SERVICE_STOP_PENDING, 20 * 1000); // ���˴���,��ʼֹͣ
					m_svc->m_EndMainFlag = true;						 // �����Ͳ��õȴ����߳���
				} catch (...) {
					m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " ���������з���δ֪�����쳣,����ֻ���˳�.");
					m_svc->NotifyState(SERVICE_STOP_PENDING, 20 * 1000); // ���˴���,��ʼֹͣ
					m_svc->m_EndMainFlag = true;						 // �����Ͳ��õȴ����߳���
				}
			}
			break;
		case SERVICE_STOP_PENDING:
			if (m_svc->RelyMainThread()) { // �������߳�
				if (m_svc->m_ServiceExitFlag) {
					ControlService(l_sv, SERVICE_CONTROL_EXIT, &l_svcstat);
					m_svc->m_ServiceExitFlag = false;
				} else {
					m_svc->m_semSignal2.unlock(); // ֪ͨ���߳����Ѿ���
					m_svc->m_semSignal1.lock();	  // �ȴ����߳�ֹͣ�������
				}
			} else {
				try {
					m_svc->ServiceStop();
					m_svc->Trace_Event(TE_INFO, dstring("����:") << m_svc->SetDispName() << " �Ѿ�ֹͣ");
				} catch (std::exception const& e) {
					m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " ֹͣ�����з��������쳣,����ֻ���˳�,�쳣����������Ϣ:" << e.what());
				} catch (...) {
					m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " ֹͣ�����з���δ֪�����쳣,����ֻ���˳�.");
				}
				m_svc->NotifyState(SERVICE_STOP_PENDING, 5 * 1000);
				if (m_svc->m_ServiceExitFlag) {
					m_svc->m_EndMainFlag = true; // �����Ͳ��õȴ����߳���
				}
			}
			break; // ����ֹͣ,SvcMain����
		case SERVICE_PAUSE_PENDING:
			if (m_svc->RelyMainThread()) {	  // �������߳�
				m_svc->m_semSignal2.unlock(); // ֪ͨ���߳����Ѿ���
				m_svc->m_semSignal1.lock();	  // �ȴ����߳���ͣ�������
			} else {
				try {
					m_svc->ServicePause();
					m_svc->Trace_Event(TE_SUCC, dstring("����:") << m_svc->SetDispName() << " �Ѿ���ͣ");
					m_svc->NotifyState(SERVICE_PAUSED);
				} catch (std::exception const& e) {
					m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " ��ͣ�����з��������쳣,����ֻ���˳�,�쳣����������Ϣ:" << e.what());
					m_svc->ServiceExit(ERROR, -1);
				} catch (...) {
					m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " ��ͣ�����з���δ֪�����쳣,����ֻ���˳�.");
					m_svc->ServiceExit(ERROR, -2);
				}
			}
			break;
		case SERVICE_CONTINUE_PENDING:
			if (m_svc->RelyMainThread()) {	  // �������߳�
				m_svc->m_semSignal2.unlock(); // ֪ͨ���߳����Ѿ���
				m_svc->m_semSignal1.lock();	  // �ȴ����߳���ͣ�������
			} else {
				try {
					m_svc->ServiceContinue();
					m_svc->Trace_Event(TE_SUCC, dstring("����:") << m_svc->SetDispName() << " �Ѿ��ָ�");
					m_svc->NotifyState(SERVICE_RUNNING);
				} catch (std::exception const& e) {
					m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " �ָ������з��������쳣,����ֻ���˳�,�쳣����������Ϣ:" << e.what());
					m_svc->ServiceExit(ERROR, -1);
				} catch (...) {
					m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " �ָ������з���δ֪�����쳣,����ֻ���˳�.");
					m_svc->ServiceExit(ERROR, -2);
				}
			}
			break;
		case SERVICE_RUNNING:
			if (m_svc->m_semSignal1.timelock(m_svc->m_delay) == WAIT_TIMEOUT) { // WAIT_OBJECT_0,���û��¼�����
				if (m_svc->RelyMainThread()) {									// �������߳�
					ControlService(l_sv, SERVICE_CONTROL_PROCESS, &l_svcstat);
					m_svc->m_semSignal1.lock(); // �ȴ����̴߳����������
				} else {
					try {
						m_svc->m_delay = m_svc->ServiceProcess();
					} catch (std::exception const& e) {
						m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " ���й����з��������쳣,����ֻ���˳�,�쳣����������Ϣ:" << e.what());
						m_svc->ServiceExit(ERROR, -1);
					} catch (...) {
						m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " ���й����з���δ֪�����쳣,����ֻ���˳�.");
						m_svc->ServiceExit(ERROR, -2);
					}
					m_svc->m_semSignal1.trylock(); // ������û��¼�����,���źŸ�ˢ��
				}
			}
			break;
		default:
			m_svc->m_semSignal1.lock(); // �ȴ��û��¼�
			break;
		}
		if (l_CurrentState == SERVICE_STOP_PENDING)
			break;
	}
	if (m_svc->RelyMainThread()) { // �������߳�
		CloseServiceHandle(l_sv);
		CloseServiceHandle(l_sc);
	}
	if (m_svc->m_hAppEvtSrc) {
		DeregisterEventSource(m_svc->m_hAppEvtSrc);
		m_svc->m_hAppEvtSrc = 0;
	}
	if (m_svc->m_hSecEvtSrc) {
		DeregisterEventSource(m_svc->m_hSecEvtSrc);
		m_svc->m_hSecEvtSrc = 0;
	}
	if (m_svc->m_hSysEvtSrc) {
		DeregisterEventSource(m_svc->m_hSysEvtSrc);
		m_svc->m_hSysEvtSrc = 0;
	}
	g_svcset.DelService(m_svc);

	m_svc->m_EndSvcFlag = true;
	while (!m_svc->m_EndMainFlag) // �ȴ����߳̽���
		Sleep(50);
	m_svc->NotifyState(SERVICE_STOPPED);
};
//==============================================================================
template <class T>
VOID WINAPI GenNTService<T>::Handler(DWORD fdwControl) {
	m_svc->m_semSignal2.trylock(); // ���źŸ�ˢ��
	switch (fdwControl) {
	case SERVICE_CONTROL_INIT:
		try {
			m_svc->ServiceStart();
			//			m_svc->m_semSignal1.unlock();		//֪ͨSvcMain��ʼ������
			m_svc->Trace_Event(TE_SUCC, dstring("����:") << m_svc->SetDispName() << " �Ѿ�����");
			m_svc->NotifyState(SERVICE_RUNNING);
			break;
		} catch (std::exception const& e) {
			m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " ���������з��������쳣,����ֻ���˳�,�쳣����������Ϣ:" << e.what());
			m_svc->NotifyState(SERVICE_STOP_PENDING, 20 * 1000); // ���˴���,��ʼֹͣ
		} catch (...) {
			m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " ���������з���δ֪�����쳣,����ֻ���˳�.");
			m_svc->NotifyState(SERVICE_STOP_PENDING, 20 * 1000); // ���˴���,��ʼֹͣ
		}														 // ��ʼ��ʧ��ֱ��ת�����ֹͣ����,����ֱ���˳�
	case SERVICE_CONTROL_EXIT:
		m_svc->m_ServiceExitFlag = false; // �ָ�ͨ��״̬
		m_svc->m_semSignal2.unlock();	  // �Լ�Ҳ���õȴ�SvcMain������
		m_svc->m_EndSvcFlag = true;		  // �Լ�Ҳ���õȴ�SvcMain������
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		m_svc->NotifyState(SERVICE_STOP_PENDING, 20 * 1000);
		m_svc->m_semSignal1.unlock();	// ֪ͨSvcMain���û��¼�����
		if (m_svc->RelyMainThread()) {	// �������߳�
			m_svc->m_semSignal2.lock(); // �ȴ�SvcMain����
			try {
				m_svc->ServiceStop();
				m_svc->Trace_Event(TE_INFO, dstring("����:") << m_svc->SetDispName() << " �Ѿ�ֹͣ");
			} catch (std::exception const& e) {
				m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " ֹͣ�����з��������쳣,�쳣����������Ϣ:" << e.what());
			} catch (...) {
				m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " ֹͣ�����з���δ֪�����쳣.");
			}
			m_svc->NotifyState(SERVICE_STOP_PENDING, 5 * 1000);
			m_svc->m_semSignal1.unlock(); // ֪ͨSvcMainֹͣ����ִ�����
		}
		m_svc->m_EndMainFlag = true;
		break;
	case SERVICE_CONTROL_PAUSE:
		m_svc->NotifyState(SERVICE_PAUSE_PENDING, 20 * 1000);
		m_svc->m_semSignal1.unlock();	// ֪ͨSvcMain���û��¼�����
		if (m_svc->RelyMainThread()) {	// �������߳�
			m_svc->m_semSignal2.lock(); // �ȴ�SvcMain����
			try {
				m_svc->ServicePause();
				m_svc->Trace_Event(TE_SUCC, dstring("����:") << m_svc->SetDispName() << " �Ѿ���ͣ");
				m_svc->NotifyState(SERVICE_PAUSED);
			} catch (std::exception const& e) {
				m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " ��ͣ�����з��������쳣,����ֻ���˳�,�쳣����������Ϣ:" << e.what());
				m_svc->ServiceExit(ERROR, -1);
			} catch (...) {
				m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " ��ͣ�����з���δ֪�����쳣,����ֻ���˳�.");
				m_svc->ServiceExit(ERROR, -2);
			}
			m_svc->m_semSignal1.unlock(); // ֪ͨSvcMain��ͣ����ִ�����
		}
		break;
	case SERVICE_CONTROL_CONTINUE:
		m_svc->NotifyState(SERVICE_CONTINUE_PENDING, 20 * 1000);
		m_svc->m_semSignal1.unlock();	// ֪ͨSvcMain���û��¼�����
		if (m_svc->RelyMainThread()) {	// �������߳�
			m_svc->m_semSignal2.lock(); // �ȴ�SvcMain����
			try {
				m_svc->ServiceContinue();
				m_svc->Trace_Event(TE_SUCC, dstring("����:") << m_svc->SetDispName() << " �Ѿ��ָ�");
				m_svc->NotifyState(SERVICE_RUNNING);
			} catch (std::exception const& e) {
				m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " �ָ������з��������쳣,����ֻ���˳�,�쳣����������Ϣ:" << e.what());
				m_svc->ServiceExit(ERROR, -1);
			} catch (...) {
				m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " �ָ������з���δ֪�����쳣,����ֻ���˳�.");
				m_svc->ServiceExit(ERROR, -2);
			}
			m_svc->m_semSignal1.unlock(); // ֪ͨSvcMain��������ִ�����
		}
		break;
	case SERVICE_CONTROL_PROCESS:
		try {
			m_svc->m_delay = m_svc->ServiceProcess();
		} catch (std::exception const& e) {
			m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " ���й����з��������쳣,�쳣����������Ϣ:" << e.what());
			m_svc->ServiceExit(ERROR, -1);
		} catch (...) {
			m_svc->Trace_Event(TE_ERR, dstring("����:") << m_svc->SetDispName() << " ���й����з���δ֪�����쳣.");
			m_svc->ServiceExit(ERROR, -2);
		}
		m_svc->m_semSignal1.unlock(); // ֪ͨSvcMain��������ִ�����
		break;
	case SERVICE_CONTROL_INTERROGATE:
		m_svc->NotifyState(m_svc->m_dwCurrentState);
		break;
	}
	if (m_svc->m_EndMainFlag) // �ȴ�SvcMain����
		while (!m_svc->m_EndSvcFlag)
			Sleep(50);
};

#pragma pack(pop)
_DBC_END

#endif // PKO_PLATFORM_WINDOWS

#endif // NTSVCOBJECT_H
