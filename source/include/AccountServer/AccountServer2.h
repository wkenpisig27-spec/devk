#pragma once
#include "time.h"
#include "commrpc.h"
#include "threadpool.h"
#include "packetqueue.h"
#include "tlsindex.h"
#include "util.h"
#include "db.h"
#include <mutex>
#include <memory>

#include "MyThread.h"
#include <string>
#include <unordered_map>

using namespace dbc;

struct GroupServer2 : PreAllocStru {
	void Reconnt();
	GroupServer2(uLong size) : PreAllocStru(size), m_datasock(nullptr), m_next(nullptr) {}

	WPacket GetWPacket() { return m_datasock->GetWPacket(); }
	int SendData(WPacket wpkt) { return m_datasock->SendData(wpkt); }
	bool IsSame(std::string const& strGroup) { return (m_strName == strGroup); }
	char const* const GetName() const { return m_strName.c_str(); }
	char const* const GetAddr() const { return m_strAddr.c_str(); }

	std::string m_strName;
	std::string m_strAddr;
	DataSocket* m_datasock;
	GroupServer2* m_next;
};

class AccountServer2 : public TcpServerApp, public RPCMGR {
public:
	AccountServer2(ThreadPool* proc = 0, ThreadPool* comm = 0);
	virtual ~AccountServer2();

	GroupServer2* FindGroup(char const* szGroup);
	void DisplayGroup();

	void IncreaseMembers(int nCount = 1);
	void DecreaseMembers(int nCount = 1);
	void ResetMembersCount();
	LONG GetMembersCount();

protected:
	virtual bool OnConnect(DataSocket* datasock);
	virtual void OnProcessData(DataSocket* datasock, RPacket& rpkt);
	virtual WPacket OnServeCall(DataSocket* datasock, RPacket& rpkt);
	virtual void OnDisconnect(DataSocket* datasock, int reason);

private:
	static volatile LONG m_nMembersCount;

	// GroupServer ��ش������̺�����
	void Gs_Init();
	GroupServer2* Gs_Find(char const* szGroupName);
	bool Gs_Auth(char const* szGroupName, char const* szGroupPwd);
	void Gs_ListAdd(GroupServer2* Gs);
	void Gs_ListDel(GroupServer2* Gs);
	void Gs_Exit();

	PreAllocHeap<GroupServer2> m_GsHeap;
	GroupServer2* m_GsList;
	Mutex m_GsListLock;

	// NOTE: Old rate limiting code (AttemptInfo, mutex_attempt, attempt_info) 
	// has been removed. Rate limiting is now handled by auth::IRateLimiter.
	// See InitializeAuthSystem() in AccountServer2.cpp

private:
	short m_GsNumber;

	WPacket Gs_Login(DataSocket* datasock, RPacket& rpkt);
	bool AddGroup(GroupServer2* pGs);
	bool DelGroup(DataSocket* datasock);
	void Gs_Logout(DataSocket* datasock);

	WPacket ProcessUnknownCmd(RPacket rpkt);
};

// ��֤

class AuthQueue : public PKQueue {
public:
	AuthQueue();
	virtual ~AuthQueue();

protected:
	virtual void ProcessData(DataSocket* datasock, RPacket& rpkt);
	virtual WPacket ServeCall(DataSocket* datasock, RPacket& rpkt);
};

//  ��½��ʱ����        by Jampe
typedef std::vector<std::string> TmpNameList;
class LoginTmpList {
public:
	LoginTmpList();
	virtual ~LoginTmpList();

	bool Insert(const std::string& name);
	bool Remove(const std::string& name);
	bool Query(const std::string& name, bool lock = true);

	void Lock();
	void UnLock();

private:
	CRITICAL_SECTION m_cs;
	TmpNameList m_list;
};

class AuthThread : public MyThread {
public:
	AuthThread(int nIndex);
	virtual ~AuthThread();

	void Reconnt();

	void QueryAccount(DataSocket* datasock, RPacket rpkt);
	bool IsValidName(char const* szName, unsigned short usNameLen);

	WPacket AccountLogin(DataSocket* datasock);
	void AccountLogout(RPacket rpkt);
	WPacket TomAccountLogin(DataSocket* datasock, RPacket& rpkt);
	void TomAccountLogout(RPacket& rpkt);

	void LogoutGroup(DataSocket* datasock, RPacket rpkt);

	static void LoadConfig();

	CSQLDatabase* GetSQLDatabase();

protected:
	void Init();
	void Exit();
	bool Connect();
	void Disconn();

	virtual int Run();
	void SetRunLabel(int nRunLabel);
	void LogUserLogin(int nUserID, std::string strUserName, std::string strIP);
	void LogUserLogout(int nUserID);

protected:
	enum {
		ACCOUNT_OFFLINE,
		ACCOUNT_ONLINE,
		ACCOUNT_SAVING,
	};

	struct AccountInfo {
		bool bExist;
		int nId;
		bool bAllowLogin;
		std::string strName;
		std::string strBcryptHash;
		int nSid;
		int nStatus;
		int nEnableLoginTick;
		std::string strGroup;
		std::string strMAC;
		std::string strIP;
		std::string plaintext_password;
		int nBan;
		int nProtectTime;
		bool bRateLimited = false;  // True if blocked by rate limiter
	};

	int GenSid(char const* szName);
	void ResetAccount();
	void KickAccount(std::string& strGroup, int nId);

private:
	enum { INVALID_SID = 0 };
	enum { RELOGIN_TIME = 15 };
	enum { SAVING_TIME = 15 };
	CSQLDatabase* m_pAuth{nullptr};
	
	// Modern database connection (stored procedures)
	std::unique_ptr<cfl_rs> db_mutator;
	cfl_db db_connection;
	
	AccountInfo m_AcctInfo;
	int m_nIndex;

	static Sema m_Sema;
	static std::string m_strSrvip;
	static std::string m_strSrvdb;
	static std::string m_strUserId;
	static std::string m_strUserPwd;
	static std::string m_strAccountTableName;
};

class AuthThreadPool {
public:
	AuthThreadPool();
	~AuthThreadPool();

	void Launch();
	void NotifyToExit();
	void WaitForExit();

	enum { AT_MAXNUM = 10 };
	static int volatile RunLabel[AT_MAXNUM];
	static DWORD volatile RunLast[AT_MAXNUM];
	static DWORD volatile RunConsume[AT_MAXNUM];
	static unsigned int volatile uiAuthCount;
	static void IncAuthCount();
	static unsigned int GetAuthCount();

protected:
	AuthThread* m_Pool[AT_MAXNUM];
};

// ȫ�ֱ���
extern AccountServer2* g_As2;
extern AuthQueue g_Auth;

// Initialize the authentication rate limiter
extern void InitializeAuthSystem();
