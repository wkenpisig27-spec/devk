//================================================================
// It must be permitted by Dabo.Zhang that this program is used for
// any purpose in any situation.
// Copyright (C) Dabo.Zhang 2000-2003
// All rights reserved by ZhangDabo.
// This program is written(created) by Zhang.Dabo in 2000.3
// This program is modified recently by Zhang.Dabo in 2003.7
//=================================================================
#ifndef NTSERVICE_H
#define NTSERVICE_H

#include "platform_compat.h"

#include "NTSvcObject.h"

#ifdef PKO_PLATFORM_WINDOWS
// NTService is entirely Windows-specific (NT Service wrapper)

_DBC_BEGIN
#pragma pack(push)
#pragma pack(4)

#define DECLARE_SERVICE(Service) dbc::PNTSVC dbc::GenNTService<Service>::m_svc;
#define INIT_SERVICE(Service) dbc::GenNTService<Service> DBC##Service;
#define SERVICE_RUN dbc::g_svcset.Run();
//=======NTService=======================================================================
class NTService : NTSvcObject {
private:
	virtual void ServiceStart() = 0;
	virtual void ServiceStop() = 0;
	virtual void ServicePause(){};
	virtual void ServiceContinue(){};
	virtual int ServiceProcess() { return 1000; }; // ���ط�����Ҫ�ĵ��ü��ʱ��,ȱʡΪ1000����(1��),���������źŵȴ�Ӧ�÷���0,�����еȴ�����̫��(ӦС��3��),�Ա��ֶ��û�������Ӧ�ļ�ʱ��

	virtual cChar* SetSvcName() const = 0;
	virtual cChar* SetDispName() const { return SetSvcName(); }; // ��ʾ��ȱʡ���ڷ�����
	virtual cChar* SetDependencies() const { return 0; }		 // ȱʡ����û������,���ظ�ʽ��"MSSQL\0Server\0"
	virtual cChar* SetRCFile() const { return 0; };				 // ȱʡʹ�ÿ�ִ���ļ�����Դ�ļ�
	virtual DWORD SetNumOfEvtCat() const { return 0; }			 // CategoryMessageFile�ļ��е�Ŀ¼����,������Զ������ṩĿ¼.

	virtual bool CanPaused() const { return true; };	   // ȱʡ֧����ͣ�ͼ�������
	virtual bool RelyMainThread() const { return false; }; // �������������߳�,����������Ҫ��override����true;
	virtual bool HasAppEvent() const { return true; };
	virtual bool HasSecEvent() const { return false; }; // һ����˵û�а�ȫ�¼�
	virtual bool HasSysEvent() const { return false; }; // һ����˵û��ϵͳ�¼�
public:
	void GetSvcArg(DWORD& argc, LPTSTR*& argv) const {
		argc = m_SvcArgc;
		argv = m_SvcArgv;
	};
	void GetExeArg(int& argc, char**& argv) const {
		argc = g_svcset.m_argc;
		argv = g_svcset.m_argv;
	};
	char* GetExeFullPath() const { return g_svcset.m_argv[0]; };

	void Trace_Event(uChar Severity, cChar* msg) const { NTSvcObject::Trace_Event(Severity, msg); }; // Severity�����׵�SVRT�궨��
	void ServiceExit(DWORD Win32ExitCode = NO_ERROR, DWORD ServiceSpecExitCode = 0)					 // �����ο�Win32 API��SetServiceStatus()����,һ��ʹ��ȱʡֵ����
	{
		NTSvcObject::ServiceExit(Win32ExitCode, ServiceSpecExitCode);
	};
	BOOL NotifyState(DWORD WaitHint = 5000) {
		return NTSvcObject::NotifyState(m_dwCurrentState, WaitHint);
	}
};

#define CATCH_COM_ERROR                                                      \
	catch (_com_error & e) {                                                 \
		char l_buf[128];                                                     \
		l_buf[0] = 0;                                                        \
		sprintf(l_buf, "Error:");                                            \
		sprintf(l_buf, "Code = %08lx", e.Error());                           \
		sprintf(l_buf, "Code meaning = %s", (TCHAR*)e.ErrorMessage());       \
		sprintf(l_buf, "Source = %s", (TCHAR*)e.Source());                   \
		sprintf(l_buf, "Error Description = %s\n", (TCHAR*)e.Description()); \
		RollbackTrans();                                                     \
		THROW_EXCP(excpCOM, l_buf);                                          \
	}

#pragma pack(pop)
_DBC_END

#endif // PKO_PLATFORM_WINDOWS

#endif // NTSERVICE_H
