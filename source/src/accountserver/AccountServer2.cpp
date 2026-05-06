#define _CRTDBG_MAP_ALLOC
#include "stdafx.h"

#include <cstdlib>
#include <crtdbg.h>
#include <string_view>

#include "AccountServer2.h"
#include "gamecommon.h"
#include "inifile.h"
#include "util.h"
#include "GlobalVariable.h"

#include <conformity.h>
#include <botan/botan.h>
#include <botan/bcrypt.h>

// New auth system integration
#include "Auth.h"
#include "PacketSanitizer.h"

using namespace std;

#pragma warning(disable : 4800)

// TLSIndex g_TlsIndex;
AuthQueue g_Auth; // ��֤�������

LoginTmpList tmpLogin; //   ��½��ʱ�б�

uLong NetBuffer[] = {100, 10, 0};
bool g_logautobak = true;

// New auth system - rate limiter with same config as original magic numbers
static std::shared_ptr<auth::IRateLimiter> g_authRateLimiter;
static std::shared_ptr<auth::IAuthService> g_authService;

void InitializeAuthSystem() {
    // Load auth config from AccountServer.cfg
    // If sections are missing, LoadAuthConfig returns safe defaults matching legacy behavior
    auth::AuthConfig authConfig = auth::LoadAuthConfig("AccountServer.cfg");
    
    // Create rate limiter with loaded config
    g_authRateLimiter = auth::CreateDefaultRateLimiter(authConfig.rateLimit);
    
    // Create auth service with rate limiter
    g_authService = auth::CreateLegacyAuthService(g_authRateLimiter, authConfig);
    
    // Log configuration for verification
    LG("AccountServer", "Auth system initialized:\n");
    LG("AccountServer", "  Rate Limiting: %s\n", authConfig.rateLimit.enabled ? "ENABLED" : "DISABLED");
    LG("AccountServer", "  Max Attempts: %d per %d seconds\n", 
       authConfig.rateLimit.maxAttemptsPerInterval, authConfig.rateLimit.intervalSeconds);
    LG("AccountServer", "  Block Duration: %d seconds\n", authConfig.rateLimit.blockDurationSeconds);
    LG("AccountServer", "  Login Timeout: %d ms\n", authConfig.timeouts.loginTimeoutMs);
    LG("AccountServer", "  Saving Protection: %d seconds\n", authConfig.timeouts.savingProtectionSeconds);
}

volatile LONG AccountServer2::m_nMembersCount = 0;

int isValidMacAddress(const char* mac);

// Tcp Server
AccountServer2::AccountServer2(ThreadPool* proc, ThreadPool* comm)
	: TcpServerApp(this, proc, comm, false),
	  RPCMGR(this), m_GsHeap(1, 100), m_GsList(nullptr), m_GsNumber(0) {
	m_GsHeap.Init();

	IniFile inf(g_strCfgFile.c_str());
	
	// Set log directory from config (optional section)
	try {
		IniSection& resSection = inf["Res"];
		const std::string logDir = resSection["log_dir"];
		if (!logDir.empty()) {
			LG_SetDirWithTimestamp(logDir.c_str());
		}
	} catch (...) {
		// [Res] section not found, use default log directory
	}
	
	IniSection& is = inf["net"];
	const auto& ip = is["listen_ip"];
	unsigned short port = std::stoi(is["listen_port"]);

	// the offset of "packet length" is 0
	// the field of "packet length" is 2 bytes
	// the max length of packet is 4K bytes
	// the max length of send queue is 100
	SetPKParse(0, 2, 4 * 1024, 100);

#ifdef _DEBUG
	BeginWork(200);
#else
	BeginWork(30);
#endif

	OpenListenSocket(port, ip.c_str());

	ResetMembersCount();
}
AccountServer2::~AccountServer2() {
	ShutDown(12 * 1000);
}

void AccountServer2::IncreaseMembers(int nCount) {
	InterlockedExchangeAdd(&m_nMembersCount, static_cast<LONG>(nCount));
}

void AccountServer2::DecreaseMembers(int nCount) {
	InterlockedExchangeAdd(&m_nMembersCount, static_cast<LONG>(-nCount));
}

void AccountServer2::ResetMembersCount() {
	InterlockedExchange(&m_nMembersCount, 0);
}

LONG AccountServer2::GetMembersCount() {
	return m_nMembersCount;
}

bool AccountServer2::OnConnect(DataSocket* datasock) {
	return true;
}
void AccountServer2::OnProcessData(DataSocket* datasock, RPacket& rpkt) {
	unsigned short usCmd = rpkt.ReadCmd();

	// Validate CMD range - AccountServer only accepts CMD_PA (3000-3050)
	if (usCmd < 3000 || usCmd > 3050) {
		LG("Security", "[AccountServer] Invalid CMD %u from peer, dropping packet\n", usCmd);
		return;
	}

	switch (usCmd) {
		// GroupServerЭ��
	case CMD_PA_CHANGEPASS: {
		string name = rpkt.ReadString();
		string pass = rpkt.ReadString();
		if (!PS::ValidateUsername(name.c_str())) {
			LG("Security", "[AccountServer] CMD_PA_CHANGEPASS: invalid username '%s'\n", name.c_str());
			break;
		}
		if (!PS::IsSafeString(pass.c_str())) {
			LG("Security", "[AccountServer] CMD_PA_CHANGEPASS: unsafe password for user '%s'\n", name.c_str());
			break;
		}
		g_MainDBHandle.UpdatePassword(name, pass);
		break;
	}
	case CMD_PA_REGISTER: {
		string name = rpkt.ReadString();
		string pass = rpkt.ReadString();
		string email = rpkt.ReadString();
		if (!PS::ValidateUsername(name.c_str())) {
			LG("Security", "[AccountServer] CMD_PA_REGISTER: invalid username '%s'\n", name.c_str());
			break;
		}
		if (!PS::IsSafeString(pass.c_str())) {
			LG("Security", "[AccountServer] CMD_PA_REGISTER: unsafe password for user '%s'\n", name.c_str());
			break;
		}
		if (!PS::IsSafeString(email.c_str())) {
			LG("Security", "[AccountServer] CMD_PA_REGISTER: unsafe email for user '%s'\n", name.c_str());
			break;
		}
		bool result = g_MainDBHandle.InsertUser(name, pass, email);
		break;
	}
	case CMD_PA_LOGOUT: {
		g_Auth.AddPK(datasock, rpkt);
		break;
	}

		// ��֤��Э��
	case CMD_PA_USER_LOGOUT: {
		g_Auth.AddPK(datasock, rpkt);
		break;
	}

	case CMD_PA_GMBANACCOUNT: {
		string actName = rpkt.ReadString();
		if (actName.empty() || actName.size() > PS::MAX_USERNAME_LENGTH) {
			LG("Security", "[AccountServer] CMD_PA_GMBANACCOUNT: invalid account name length\n");
			break;
		}
		g_MainDBHandle.OperAccountBan(actName, 3);
		break;
	}
	case CMD_PA_GMUNBANACCOUNT: {
		string actName = rpkt.ReadString();
		if (actName.empty() || actName.size() > PS::MAX_USERNAME_LENGTH) {
			LG("Security", "[AccountServer] CMD_PA_GMUNBANACCOUNT: invalid account name length\n");
			break;
		}
		g_MainDBHandle.OperAccountBan(actName, 0);
		break;
	}
		// ����δ֪Э��
	default:
		LG("As2Excp", "Unknown usCmd=[%d]\n", usCmd);
	}
}
WPacket AccountServer2::OnServeCall(DataSocket* datasock, RPacket& rpkt) {
	unsigned short usCmd = rpkt.ReadCmd();

	// Validate CMD range - AccountServer only accepts CMD_PA (3000-3050)
	if (usCmd < 3000 || usCmd > 3050) {
		LG("Security", "[AccountServer] Invalid CMD %u in ServeCall, rejecting\n", usCmd);
		return ProcessUnknownCmd(rpkt);
	}

	switch (usCmd) {
		// GroupServerЭ��
	case CMD_PA_LOGIN:
		return Gs_Login(datasock, rpkt);

		// ��֤��Э��
	case CMD_PA_USER_LOGIN:
		return g_Auth.SyncPK(datasock, rpkt, 20 * 1000);

		// ����δ֪Э��
	default:
		LG("As2Excp", "Unknown usCmd=[%d]\n", usCmd);
		return ProcessUnknownCmd(rpkt);
	}
}
WPacket AccountServer2::ProcessUnknownCmd(RPacket rpkt) {
	WPacket wpkt = GetWPacket();
	wpkt.WriteShort(ERR_AP_UNKNOWNCMD);
	return wpkt;
}
void AccountServer2::OnDisconnect(DataSocket* datasock, int reason) {
	Gs_Logout(datasock);
}

// GroupServer��ط���
void AccountServer2::Gs_Init() {
	m_GsNumber = 0;
	m_GsList = nullptr;
	m_GsHeap.Init();
	m_GsListLock.Create(false);
}
GroupServer2* AccountServer2::Gs_Find(char const* szGroupName) {
	GroupServer2* curr = m_GsList;
	GroupServer2* prev = nullptr;

	while (curr != nullptr) {
		if (curr->m_strName.compare(szGroupName) == 0)
			break;

		prev = curr;
		curr = curr->m_next;
	}

	return curr;
}
bool AccountServer2::Gs_Auth(char const* szGroupName, char const* szGroupPwd) {
	std::string_view pwd;
	IniFile inf(g_strCfgFile.c_str());

	try {
		pwd = inf["gs"][szGroupName];
	} catch (...) {
		return false;
	}
	return pwd == szGroupPwd;
}
void AccountServer2::Gs_ListAdd(GroupServer2* Gs) {
	Gs->m_next = m_GsList;
	m_GsList = Gs;
	++m_GsNumber;
}
void AccountServer2::Gs_ListDel(GroupServer2* Gs) {
	GroupServer2* curr = m_GsList;
	GroupServer2* prev = nullptr;

	while (curr) {
		if (curr == Gs)
			break;

		prev = curr;
		curr = curr->m_next;
	}

	if (curr) {
		if (prev) {
			prev->m_next = curr->m_next;
		} else {
			m_GsList = curr->m_next;
		}

		--m_GsNumber;
	}
}
void AccountServer2::Gs_Exit() {}
WPacket AccountServer2::Gs_Login(DataSocket* datasock, RPacket& rpkt) {
	/*
	2005-4-14 add by Arcol:
	���ִ˺������������̵߳���,������Ҫһ���߳�ͬ����
	���ִ˺�����Gs_Logout���ܴ��ڶ��߳�ͬ�����е����,�˺�����Ҫ��Gs_Logout����һ���߳�ͬ����
	���˺����ܸ��ٷ���(���������ݿ����),�����߳�ͬ��������ʺܵ�
	ʹ�ô�Χ�߳�ͬ�������Խ��ͬһ�����ڲ���Դ��ͻ����,���޷�������溯�����õĳ�ͻ(��Ȼ��������ּ��ʸ��ӵ�)
	����Ч�ķ����ǰ�Gs_Login��Gs_Logout֮��ĺ��������̵߳���ת���ɶ�����Ϣ����,��ṹ�Ķ��Ƚϴ�,Ŀǰδ���ô˷���
	*/

	bool bAuthSuccess = false;
	bool bAlreadyLogin = false;
	char const* szGroupName = rpkt.ReadString();
	char const* szGroupPwd = rpkt.ReadString();

	if (FindGroup(szGroupName) != nullptr) {
		bAlreadyLogin = true;
	} else {
		bAuthSuccess = Gs_Auth(szGroupName, szGroupPwd);
	}

	WPacket wpkt = GetWPacket();
	if (bAlreadyLogin) {
		wpkt.WriteShort(ERR_AP_GPSLOGGED);
	} else {
		if (bAuthSuccess) {
			GroupServer2* pGs = m_GsHeap.Get();
			pGs->m_strName = szGroupName;
			pGs->m_strAddr = datasock->GetPeerIP();
			pGs->m_datasock = datasock;

			if (AddGroup(pGs)) {
				// ���뵽List�ɹ�
				datasock->SetPointer(pGs);
				wpkt.WriteShort(ERR_SUCCESS);
				// cout << "[" << szGroupName << "] Add Successfully!" << endl;
				LG("GroupServer", "[%s] Add Successfully!\n", szGroupName);
			} else {
				// ���뵽Listʧ�ܣ�˵����ͬ��GroupServer�ոյ�¼��������쳣
				pGs->Free();
				bAlreadyLogin = true;
				wpkt.WriteShort(ERR_AP_GPSAUTHFAIL);
			}
		} else {
			wpkt.WriteShort(ERR_AP_GPSAUTHFAIL);
		}
	}

	if (bAlreadyLogin)
		Disconnect(datasock, 1000);
	return wpkt;
}
void AccountServer2::Gs_Logout(DataSocket* datasock) {
	/*
	2005-4-14 add by Arcol:
	���ִ˺������������̵߳���,������Ҫһ���߳�ͬ����
	���ִ˺�����Gs_Login���ܴ��ڶ��߳�ͬ�����е����,�˺�����Ҫ��Gs_Login����һ���߳�ͬ����
	���˺����ܸ��ٷ���(���������ݿ����),�����߳�ͬ��������ʺܵ�
	ʹ�ô�Χ�߳�ͬ�������Խ��ͬһ�����ڲ���Դ��ͻ����,���޷�������溯�����õĳ�ͻ(��Ȼ��������ּ��ʸ��ӵ�)
	����Ч�ķ����ǰ�Gs_Login��Gs_Logout֮��ĺ��������̵߳���ת���ɶ�����Ϣ����,��ṹ�Ķ��Ƚϴ�,Ŀǰδ���ô˷���
	����GroupServer�Ͽ����Ӻ����״̬,����AccountServerҲ��Ӧ������û�״̬,�ڵȴ��Զ������Ӻ���ָܻ�����,��Ҫ��GroupServer������AccountServerҲ��������
	*/
	std::string strGroupName;

	GroupServer2* pGs = (GroupServer2*)datasock->GetPointer();
	if ((pGs == nullptr) || (pGs->m_datasock != datasock))
		return;

	strGroupName = pGs->m_strName;
	LG("GroupServer", "[%s] disconnected!\n", strGroupName.c_str());

	if (DelGroup(datasock)) {
		WPacket wpkt = GetWPacket();
		wpkt.WriteCmd(CMD_PA_LOGOUT);
		wpkt.WriteString(strGroupName.c_str());
		auto rpkt = RPacket(wpkt);
		OnProcessData(datasock, rpkt);
	}
}
GroupServer2* AccountServer2::FindGroup(char const* szGroup) {
	GroupServer2* pGs = nullptr;

	m_GsListLock.lock();
	try {
		pGs = Gs_Find(szGroup);
	} catch (...) {
		LG("As2Excp", "Exception raised from KickAccount when find GroupServer: [%s]\n", szGroup);
	}
	m_GsListLock.unlock();

	return pGs;
}
void AccountServer2::DisplayGroup() {
	extern void ClearGroupList();
	extern BOOL AddGroupToList(char const* strName, char const* strAddr, char const* strStatus);

	GroupServer2* pGs = m_GsList;
	ClearGroupList();
	m_GsListLock.lock();
	try {
		while (pGs != nullptr) {
			// AddGroupToList(pGs->GetName(), pGs->GetAddr(), "������");
			AddGroupToList(pGs->GetName(), pGs->GetAddr(), "Connected");
			pGs = pGs->m_next;
		}
	} catch (...) {
	}
	m_GsListLock.unlock();
}
bool AccountServer2::AddGroup(GroupServer2* pGs) {
	bool bAlreadyLogin = false;

	m_GsListLock.lock();
	try {
		if (Gs_Find(pGs->m_strName.c_str()) != nullptr) {
			bAlreadyLogin = true;
		}

		if (!bAlreadyLogin) {
			Gs_ListAdd(pGs);
		}
	} catch (...) {
		LG("As2Excp", "Exception raised from AddGroup() when add [%s]\n", pGs->m_strName.c_str());
		bAlreadyLogin = true; // ��������GroupServer��¼
	}
	m_GsListLock.unlock();

	return !bAlreadyLogin;
}
bool AccountServer2::DelGroup(DataSocket* datasock) {
	bool bDel = false;
	GroupServer2* pGs = nullptr;

	m_GsListLock.lock();
	try {
		// ����һ�μ��!
		pGs = (GroupServer2*)datasock->GetPointer();
		if ((pGs != nullptr) && (pGs->m_datasock == datasock)) {
			Gs_ListDel(pGs);
			datasock->SetPointer(nullptr);
			bDel = true;
			pGs->Free();
		}
	} catch (...) {
		LG("As2Excp", "Exception raised from AddGroup() when add [%s]\n", pGs->m_strName.c_str());
	}
	m_GsListLock.unlock();

	return bDel;
}

// Auth
AuthQueue::AuthQueue() : PKQueue(false) {}
AuthQueue::~AuthQueue() {}
void AuthQueue::ProcessData(DataSocket* datasock, RPacket& rpkt) {
	bool bRetry = true;
	unsigned short usCmd = rpkt.ReadCmd();

	// �õ���ǰ�̶߳���
	AuthThread* pThis = (AuthThread*)(g_TlsIndex.GetPointer());
	if (pThis == nullptr)
		return;

	while (bRetry) {
		try {
			switch (usCmd) {
			case CMD_PA_LOGOUT: {
				pThis->LogoutGroup(datasock, rpkt);
			} break;

			case CMD_PA_USER_LOGOUT: {
				pThis->AccountLogout(rpkt);
			} break;

			default:
				LG("AuthProcessData", "Unknown usCmd=[%d], Skipped...\n", usCmd);
				break;
			}
			bRetry = false;
		} catch (CSQLException* se) {
			LG("AuthProcessDataExcp", "SQL Exception: %s\n", se->m_strError.c_str());
			pThis->Reconnt();
		} catch (const std::exception& e) {
			LG("AuthProcessDataExcp", "std::exception in ProcessData cmd=%d: %s\n", usCmd, e.what());
			bRetry = false;
		} catch (...) {
			LG("AuthProcessDataExcp", "unknown exception in ProcessData cmd=%d\n", usCmd);
			bRetry = false;
		}
	}
}

WPacket AuthQueue::ServeCall(DataSocket* datasock, RPacket& rpkt) {
	bool bRetry = true;
	unsigned short usCmd = rpkt.ReadCmd();
	WPacket wpkt = datasock->GetWPacket();

	// �õ���ǰ�̶߳���
	AuthThread* pThis = (AuthThread*)(g_TlsIndex.GetPointer());
	if (pThis == nullptr) {
		LG("AuthExcp", "pThis = nullptr \n");
		wpkt.WriteShort(ERR_AP_TLSWRONG);
		return wpkt;
	}

	int retryCount = 0;
	const int maxRetries = 3;
	
	while (bRetry && retryCount < maxRetries) {
		try {
			switch (usCmd) {
			case CMD_PA_USER_LOGIN: {
				pThis->QueryAccount(datasock, rpkt);
				return pThis->AccountLogin(datasock);
			}

			default: {
				LG("AuthServeCall", "Unknown usCmd=[%d], Skipped...\n", usCmd);
				wpkt.WriteShort(ERR_AP_UNKNOWNCMD);
				return wpkt;
			}
			}
		} catch (CSQLException* se) {
			LG("AuthServeCallExcp", "SQL Exception (retry %d/%d): %s\n", retryCount + 1, maxRetries, se->m_strError.c_str());
			pThis->Reconnt();
			retryCount++;
		} catch (const std::exception& e) {
			LG("AuthServeCallExcp", "std::exception in ServeCall cmd=%d: %s\n", usCmd, e.what());
			bRetry = false;
		} catch (...) {
			LG("AuthServeCallExcp", "unknown exception in ServeCall cmd=%d (retry %d)\n", usCmd, retryCount + 1);
			// Try to reconnect database on unknown exceptions too
			pThis->Reconnt();
			retryCount++;
			if (retryCount >= maxRetries) {
				LG("AuthServeCallExcp", "max retries exceeded for cmd=%d, giving up\n", usCmd);
				bRetry = false;
			}
		}
	}

	wpkt.WriteShort(ERR_AP_UNKNOWN);
	return wpkt;
}

//  LoginTmpList
LoginTmpList::LoginTmpList() {
	InitializeCriticalSection(&m_cs);
}

LoginTmpList::~LoginTmpList() {
	DeleteCriticalSection(&m_cs);
}

bool LoginTmpList::Insert(const std::string& name) {
	bool ret = false;
	Lock();
	if (!Query(name, false)) {
		m_list.push_back(name);
		ret = true;
	}
	UnLock();
	return ret;
}

bool LoginTmpList::Remove(const std::string& name) {
	bool ret = false;
	TmpNameList::iterator it;
	Lock();
	for (it = m_list.begin(); it != m_list.end(); it++) {
		if ((*it) == name) {
			ret = true;
			m_list.erase(it);
			break;
		}
	}
	UnLock();
	return ret;
}

bool LoginTmpList::Query(const std::string& name, bool lock /* = true*/) {
	bool ret = false;
	TmpNameList::iterator it;
	if (lock) {
		Lock();
	}
	for (it = m_list.begin(); it != m_list.end(); it++) {
		if ((*it) == name) {
			ret = true;
			break;
		}
	}
	if (lock) {
		UnLock();
	}
	return ret;
}

void LoginTmpList::Lock() {
	EnterCriticalSection(&m_cs);
}

void LoginTmpList::UnLock() {
	LeaveCriticalSection(&m_cs);
}

// AuthThread
Sema AuthThread::m_Sema(0, AuthThreadPool::AT_MAXNUM);
std::string AuthThread::m_strSrvip = "";
std::string AuthThread::m_strSrvdb = "";
std::string AuthThread::m_strUserId = "";
std::string AuthThread::m_strUserPwd = "";
std::string AuthThread::m_strAccountTableName = "account_login";
AuthThread::AuthThread(int nIndex) : m_pAuth(nullptr), m_nIndex(nIndex) {}
AuthThread::~AuthThread() { Exit(); }
void AuthThread::Init() {
	g_TlsIndex.SetPointer(nullptr);
	m_pAuth = nullptr;

	SetRunLabel(-1);

	while (!Connect()) {
		if (GetExitFlag())
			return;
		Sleep(5000);
	}
	g_TlsIndex.SetPointer(this);
	ResetAccount();

	SetRunLabel(0);
}
void AuthThread::Exit() {
	Disconn();
	g_TlsIndex.SetPointer(nullptr);
}
bool AuthThread::Connect() {
	// Use modern database connection with stored procedure support (like TOP-master)
	std::string err_info;
	const bool r = db_connection.connect(m_strSrvip.c_str(), m_strSrvdb.c_str(), 
										 m_strUserId.c_str(), m_strUserPwd.c_str(), err_info);
	if (r) {
		db_mutator = std::make_unique<cfl_rs>(&db_connection);
		return true;
	}
	
	if (!err_info.empty()) {
		LG("AuthDBExcp", "AuthThread::Connect() failed: %s\n", err_info.c_str());
	}
	return false;
}
void AuthThread::Disconn() {
	db_connection.disconn();
	db_mutator.reset();
}
void AuthThread::Reconnt() {
	Disconn();
	while (!Connect()) {
		LG("As2", "unable to connect to database, reconnecting in 5 seconds! \n");
		if (GetExitFlag())
			return;

		Sleep(5000);
	}
}
void AuthThread::LoadConfig() {
	try {
		IniFile inf(g_strCfgFile.c_str());
		IniSection& is = inf["db"];
		m_strSrvip = is["dbserver"];
		m_strSrvdb = is["db"];
		m_strUserId = is["userid"];
		m_strUserPwd = is["passwd"];
	} catch (std::exception& e) {
		cout << e.what() << endl;
		getchar();
		ExitProcess(-1);
	}
}

void AuthThread::LogUserLogin(int nUserID, string strUserName, string strIP) {
	sUserLog* pUserLog = new sUserLog;
	pUserLog->bLogin = true;
	pUserLog->nUserID = nUserID;
	pUserLog->strUserName = strUserName;
	pUserLog->strLoginIP = strIP;
#ifdef PKO_PLATFORM_WINDOWS
	if (!PostMessage(g_hMainWnd, WM_USER_LOG, 0, (LPARAM)pUserLog)) {
		LG("AccountServer", "AuthThread::LogUserLogin: PostMessage WM_USER_LOG failed!\n");
		delete pUserLog;
	}
#else
	// On Linux, call DB directly (no Win32 message queue)
	g_MainDBHandle.UserLogin(pUserLog->nUserID, pUserLog->strUserName, pUserLog->strLoginIP);
	delete pUserLog;
#endif
}

void AuthThread::LogUserLogout(int nUserID) {
	sUserLog* pUserLog = new sUserLog;
	pUserLog->bLogin = false;
	pUserLog->nUserID = nUserID;
#ifdef PKO_PLATFORM_WINDOWS
	if (!PostMessage(g_hMainWnd, WM_USER_LOG, 0, (LPARAM)pUserLog)) {
		LG("AccountServer", "AuthThread::LogUserLogout: PostMessage WM_USER_LOG failed!\n");
		delete pUserLog;
	}
#else
	g_MainDBHandle.UserLogout(pUserLog->nUserID);
	delete pUserLog;
#endif
}

void AuthThread::QueryAccount(DataSocket* datasock, RPacket rpkt) {
	unsigned short usNameLen;
	const char* pName = nullptr;
	char szSql[512] = {0};
	SetRunLabel(11);
	
	// Reset rate limit flag for this new attempt
	m_AcctInfo.bRateLimited = false;

	// Read this stuff but ignore it, as the packet was duplicated.
	//@to-do: add ReverseReadSequence/ReverseReadString.
	uShort cipherlen;
	uShort ivlen;
	rpkt.ReadSequence(cipherlen);
	rpkt.ReadSequence(ivlen);

	// Read passport (ignored, duplicated in packet)
	pName = rpkt.ReadString();

	// Read account name
	pName = rpkt.ReadString(&usNameLen);
	LG("PASSWD", "From GroupServer [%s] = [%d]\n", pName, strlen(pName));
	
	// DEBUG: Log the username being validated
	LG("AuthDebug", "Validating username: [%s] len=%d\n", pName ? pName : "(null)", usNameLen);
	
	// Validation using new auth service
	const bool isValidName = g_authService->IsValidUsername(pName ? pName : "");

	if ((pName == nullptr) || !isValidName) {
		LG("AuthExcp", "NULL or INVALID Name field: [%s] valid=%d\n", pName ? pName : "(null)", isValidName);
		m_AcctInfo.bExist = false;
		return;
	}

	m_AcctInfo.strName = pName;

	// Read MAC address
	m_AcctInfo.strMAC = rpkt.ReadString();
	
	// Read password
	uShort plainlen;
	cChar* plainptr = rpkt.ReadSequence(plainlen);
	m_AcctInfo.plaintext_password = std::string(plainptr, plainptr + plainlen);

	// Read ACTUAL client IP from packet (not GroupServer connection IP!)
	in_addr ipAddr;
	ipAddr.s_addr = rpkt.ReadLong();
	m_AcctInfo.strIP = inet_ntoa(ipAddr);

	// NOW do rate limiting using the REAL client IP (not datasock->GetPeerIP())
	LG("AuthDebug", "Checking rate limit for client IP: [%s]\n", m_AcctInfo.strIP.c_str());
	
	try {
		if (g_authRateLimiter) {
			if (!g_authRateLimiter->IsAllowed(m_AcctInfo.strIP)) {
				// Rate limited - different log message to distinguish from validation
				LG("AuthExcp", "RATE LIMITED - Client IP [%s] blocked due to too many attempts\n", m_AcctInfo.strIP.c_str());
				m_AcctInfo.bExist = false;
				m_AcctInfo.bRateLimited = true;  // Mark as rate limited for proper error message
				return;
			}
			// Record the attempt
			g_authRateLimiter->RecordAttempt(m_AcctInfo.strIP);
			LG("AuthDebug", "Rate limit check passed for client IP: [%s]\n", m_AcctInfo.strIP.c_str());
		}
	} catch (const std::exception& e) {
		// FAIL-CLOSED: If rate limiter fails, block the login attempt for security
		// This prevents attackers from bypassing rate limiting by causing exceptions
		LG("AuthExcp", "Rate limiter exception for IP [%s]: %s - BLOCKING login for security\n", m_AcctInfo.strIP.c_str(), e.what());
		m_AcctInfo.bExist = false;
		m_AcctInfo.bRateLimited = true;
		return;
	}

	if (isValidMacAddress(m_AcctInfo.strMAC.c_str())) {
		m_AcctInfo.bExist = false;
		return;
	}

	// Query account using stored procedure (like TOP-master)
	SetRunLabel(12);
	std::string buf[7];
	int affect_rows = 0;

	// Safety check - ensure db_mutator is valid
	if (!db_mutator) {
		LG("AuthExcp", "db_mutator is null for account [%s], attempting reconnect\n", m_AcctInfo.strName.c_str());
		Reconnt();
		if (!db_mutator) {
			LG("AuthExcp", "db_mutator still null after reconnect, failing login\n");
			m_AcctInfo.bExist = false;
			return;
		}
	}

	db_mutator->_get_row_stored_procedure(buf, std::size(buf), "{CALL dbo.QueryAccount(?)}", "dbo", "QueryAccount", &affect_rows, 1, m_AcctInfo.strName.c_str());

	SetRunLabel(13);

	if (affect_rows == 1) {
		m_AcctInfo.bExist = true;
		m_AcctInfo.nId = atoi(buf[0].c_str());
		m_AcctInfo.strBcryptHash = buf[1];
		m_AcctInfo.nSid = atoi(buf[2].c_str());
		m_AcctInfo.nStatus = atoi(buf[3].c_str());
		m_AcctInfo.strGroup = buf[4];
		m_AcctInfo.nBan = atoi(buf[5].c_str());
		m_AcctInfo.nProtectTime = atoi(buf[6].c_str());

		if (!tmpLogin.Insert(m_AcctInfo.strName)) {
			m_AcctInfo.nStatus = ACCOUNT_ONLINE;
			LG("AuthExcp", "Account %s multilogin at same times.\n", m_AcctInfo.strName.c_str());
		}
	} else {
		m_AcctInfo.bExist = false;
	}
}

bool AuthThread::IsValidName(char const* szName, unsigned short usNameLen) {
	// Tom????????"_" "-" "."3???
	if (usNameLen > 32)
		return false;
	if (usNameLen > 20)
		return false;
	const unsigned char* l_name = reinterpret_cast<const unsigned char*>(szName);
	bool l_ishan = false;
	for (unsigned short i = 0; i < usNameLen; i++) {
		if (!l_name[i]) {
			return false;
		} else if (l_ishan) {
			if (l_name[i - 1] == 0xA1 && l_name[i] == 0xA1) // ??????
			{
				return false;
			}
			if (l_name[i] > 0x3F && l_name[i] < 0xFF && l_name[i] != 0x7F) {
				l_ishan = false;
			} else {
				return false;
			}
		} else if (l_name[i] > 0x80 && l_name[i] < 0xFF) {
			l_ishan = true;
		} else if ((l_name[i] >= 'A' && l_name[i] <= 'Z') || (l_name[i] >= 'a' && l_name[i] <= 'z') || (l_name[i] >= '0' && l_name[i] <= '9')) {

		} else if (l_name[i] == '.' || l_name[i] == '_' || l_name[i] == '-') {
			// TomService removed
			// if (!g_TomService.IsEnable())
			// 	return false;
			return false;
		} else {
			return false;
		}
	}
	return !l_ishan;

	// if (usNameLen > 20) return false;
	// if (strstr(sql, "select") != nullptr) return false;
	// if (strstr(sql, "select") != nullptr) return false;
	// return true;
}

WPacket AuthThread::AccountLogin(DataSocket* datasock) {
	DWORD dwStartCount = GetTickCount();

	SetRunLabel(14);
	WPacket wpkt = datasock->GetWPacket();
	GroupServer2* pFromGroupServer = (GroupServer2*)datasock->GetPointer();
	if (pFromGroupServer == nullptr) {
		// ?GroupServer???
		LG("AuthExcp", "pFromGroupServer = nullptr \n");
		wpkt.WriteShort(ERR_AP_DISCONN);
		return wpkt;
	}
	SetRunLabel(15);

	wpkt.WriteCmd(0); // added by andor.zhang,????????GroupServer??CMD_AP_KICKUSER???

	// Check if rate limited FIRST - return specific error code
	if (m_AcctInfo.bRateLimited) {
		LG("AuthExcp", "Rate limited login attempt for account [%s]\n", m_AcctInfo.strName.c_str());
		wpkt.WriteShort(ERR_AP_TOO_MANY_ATTEMPTS);
		return wpkt;
	}

	// Modify by lark.li 20080825 begin
	if (!m_AcctInfo.bExist) {
		// ??????
		wpkt.WriteShort(ERR_AP_INVALIDUSER);
		return wpkt;
	}
	if (m_AcctInfo.nBan == 2) {
		tmpLogin.Remove(m_AcctInfo.strName);
		wpkt.WriteShort(ERR_AP_BANUSER);
		return wpkt;
	} else if (m_AcctInfo.nBan == 1) {
		tmpLogin.Remove(m_AcctInfo.strName);
		wpkt.WriteShort(ERR_AP_PBANUSER);
		return wpkt;
	}
	// Add by sunny.sun 20090827
	else if (m_AcctInfo.nBan == 3) {
		tmpLogin.Remove(m_AcctInfo.strName);
		wpkt.WriteShort(ERR_AP_BANUSER);
		return wpkt;
	}
	// End

	SetRunLabel(16);

	bool bVerify = Botan::check_bcrypt(m_AcctInfo.plaintext_password, m_AcctInfo.strBcryptHash);
	if (!bVerify) { // ????
		wpkt.WriteShort(ERR_AP_INVALIDPWD);
		LG("AccountAuth", "Thread#%d Auth [%s] (id=%d) failed: invalid password!\n", m_nIndex, m_AcctInfo.strName.c_str(), m_AcctInfo.nId);
		tmpLogin.Remove(m_AcctInfo.strName);
		return wpkt;
	}
	SetRunLabel(17);

	int nSid = GenSid(m_AcctInfo.strName.c_str());
	if (m_AcctInfo.nStatus == ACCOUNT_OFFLINE) {
		// Normal login (OFFLINE -> ONLINE) - Use stored procedure like TOP-master
		const int new_status = ACCOUNT_ONLINE;
		const auto ret = db_mutator->stored_procedure("{CALL AccountLogin(?, ?, ?, ?, ?)}", "dbo", "AccountLogin", 5, 
			&new_status, pFromGroupServer->GetName(), m_AcctInfo.strMAC.c_str(), m_AcctInfo.strIP.c_str(), &m_AcctInfo.nId);
		if (DBOK(ret)) {
			SetRunLabel(18);
			wpkt.WriteShort(ERR_SUCCESS);
			wpkt.WriteLong(m_AcctInfo.nId);
			wpkt.WriteLong(nSid);
			LogUserLogin(m_AcctInfo.nId, m_AcctInfo.strName.c_str(), m_AcctInfo.strIP.c_str());
		} else {
			wpkt.WriteShort(ERR_AP_UNKNOWN);
			LG("AccountAuth", "Thread#%d Auth [%s] (id=%d) failed: update database error where normal login!\n", m_nIndex, m_AcctInfo.strName.c_str(), m_AcctInfo.nId);
			SetRunLabel(19);
			goto login_over;
		}
	} else if (m_AcctInfo.nStatus == ACCOUNT_ONLINE) {
		// Account is already online - kick old session and allow new login
		LG("AccountAuth", "Thread#%d Auth [%s] (id=%d) duplicate login detected, kicking old session\n", 
			m_nIndex, m_AcctInfo.strName.c_str(), m_AcctInfo.nId);
		
		// Set status to SAVING and send kick command to GroupServer
		const int new_status = ACCOUNT_SAVING;
		const auto ret = db_mutator->stored_procedure("{CALL AccountLogin(?, ?, ?, ?, ?)}", "dbo", "AccountLogin", 5,
			&new_status, pFromGroupServer->GetName(), m_AcctInfo.strMAC.c_str(), m_AcctInfo.strIP.c_str(), &m_AcctInfo.nId);
		if (DBOK(ret)) {
			// Send kick command - GroupServer will kick old session then retry login
			wpkt.WriteShort(ERR_AP_ONLINE);  // Error triggers kick+retry
			wpkt.WriteCmd(CMD_AP_KICKUSER);
			wpkt.WriteLong(m_AcctInfo.nId);
			SetRunLabel(20);
			goto login_over;
		} else {
			wpkt.WriteShort(ERR_AP_UNKNOWN);
			LG("AccountAuth", "Thread#%d Auth [%s] (id=%d) failed: database error during kick\n", m_nIndex, m_AcctInfo.strName.c_str(), m_AcctInfo.nId);
			SetRunLabel(21);
			goto login_over;
		}
	} else if (m_AcctInfo.nStatus == ACCOUNT_SAVING) {
		// Account in saving state
		if (m_AcctInfo.nProtectTime >= 0 && m_AcctInfo.nProtectTime < SAVING_TIME) {
			wpkt.WriteShort(ERR_AP_SAVING);
			SetRunLabel(22);
			goto login_over;
		} else {
			// Login from saving state - Use stored procedure like TOP-master
			const int new_status = ACCOUNT_ONLINE;
			const auto ret = db_mutator->stored_procedure("{CALL AccountLogin(?, ?, ?, ?, ?)}", "dbo", "AccountLogin", 5,
				&new_status, pFromGroupServer->GetName(), m_AcctInfo.strMAC.c_str(), m_AcctInfo.strIP.c_str(), &m_AcctInfo.nId);
			if (DBOK(ret)) {
				SetRunLabel(23);
				wpkt.WriteShort(ERR_SUCCESS);
				wpkt.WriteLong(m_AcctInfo.nId);
				wpkt.WriteLong(nSid);
				LogUserLogin(m_AcctInfo.nId, m_AcctInfo.strName.c_str(), m_AcctInfo.strIP.c_str());
			} else {
				wpkt.WriteShort(ERR_AP_UNKNOWN);
				LG("AccountAuth", "Thread#%d Auth [%s] (id=%d) failed: update database error when login without locked!\n", m_nIndex, m_AcctInfo.strName.c_str(), m_AcctInfo.nId);
				SetRunLabel(24);
				goto login_over;
			}
		}
	} else {
		// ?????????
		wpkt.WriteShort(ERR_AP_UNKNOWN);
		LG("AccountAuth", "Thread#%d Auth [%s] (id=%d) failed: unknown last login status!\n", m_nIndex, m_AcctInfo.strName.c_str(), m_AcctInfo.nId);
		SetRunLabel(25);
		goto login_over;
	}
	SetRunLabel(0);

login_over:

	DWORD dwEndCount = GetTickCount() - dwStartCount;
	AuthThreadPool::RunLast[m_nIndex] = dwEndCount;
	if (dwEndCount > AuthThreadPool::RunConsume[m_nIndex]) {
		AuthThreadPool::RunConsume[m_nIndex] = dwEndCount;
	}
	tmpLogin.Remove(m_AcctInfo.strName);
	return wpkt;
}

void AuthThread::AccountLogout(RPacket rpkt) {
	SetRunLabel(99);
	const int nID = rpkt.ReadLong();
	// Use stored procedure for logout (like TOP-master)
	const int new_status = ACCOUNT_OFFLINE;
	const auto ret = db_mutator->stored_procedure("{CALL AccountLogout(?, ?)}", "dbo", "AccountLogout", 2, &new_status, &nID);
	if (DBOK(ret)) {
		SetRunLabel(0);
	}
	LogUserLogout(nID);
}



void AuthThread::LogoutGroup(DataSocket* datasock, RPacket rpkt) {
	/*
	2005-4-14 added by arcol:
	�������Ͽ���������Զ�����,��˲�����ʺ�״̬,Ҫ��GroupServer������AccountServerҲ��Ҫ����
	*/

	/*
		char szSql[512] = {0};
		std::string strName;
		char const* pszGroup = rpkt.ReadString();
		LG("As2Logout", "GroupServer: [%s] �Ͽ�����������¼�ϵ������ʺŵĵ�¼״̬\n", pszGroup);

		SetRunLabel(23);
		try {
			CSQLRecordset rs(*m_pAuth);
			sprintf(szSql, "select name from account_login where (login_group='%s' and login_status=%d)",
				pszGroup, ACCOUNT_ONLINE);
			rs << szSql;
			rs.SQLExecDirect();
			while (rs.SQLFetch()) {
				strName = rs.SQLGetData(1);
			}
		} catch (CSQLException* se) {
			LG("LogoutGroup", "Select SQL Exception: %s\n", se->m_strError.c_str());
			delete se;
		}
	*/

	/*
		SetRunLabel(24);
		try {
			CSQLUpdate s("account_login");
			sprintf(szSql, "(login_group='%s' and login_status=%d)", pszGroup, ACCOUNT_ONLINE);
			s.SetWhere(szSql);
			//s.SetColumn("login_status", ACCOUNT_SAVING);	//2006-2-24 Arcol_test
			s.SetColumn("login_status", ACCOUNT_OFFLINE);	//2006-2-24 Arcol_test
			s.SetColumn("sid", INVALID_SID);
			sprintf(szSql, "enable_login_time=dateadd(second, %d, getdate())", SAVING_TIME);
			s.SetColumn(szSql);
			sprintf(szSql, "last_logout_time=getdate()");
			s.SetColumn(szSql);
			sprintf(szSql, "total_live_time=total_live_time+datediff(second, last_login_time, getdate())");
			s.SetColumn(szSql);

			SetRunLabel(25);
			m_pAuth->ExecuteSQL(s.GetStatement());
		} catch (CSQLException* se) {
			LG("LogoutGroup", "Update SQL Exception: %s\n", se->m_strError.c_str());
			delete se;
		}
	*/

	SetRunLabel(0);
}

CSQLDatabase* AuthThread::GetSQLDatabase() {
	return m_pAuth;
}

int AuthThread::GenSid(char const* szName) {
#define SHA1_DIGEST_LEN 20
	char md[SHA1_DIGEST_LEN];

	// ������ϢԴ
	char buf[256];
	int buf_len = sprintf(buf, "%s%d", szName, GetTickCount());
	if (buf_len >= sizeof buf)
		throw std::out_of_range("buffer overflow in GenSid()\n");

	// ����ժҪ
	sha1((unsigned char*)buf, buf_len, (unsigned char*)md);

	// ȡ��ǰ4���ֽ�
	int* ptr = (int*)md;
	return ptr[0];
}
void AuthThread::ResetAccount() {
	/*
	2005-4-17 added by Arcol:
	������뿴����ͷ��,��������������д,Ŀǰ�ⲿ���߳���CDataBaseCtrl������Գ�ʼ�������ڻ����ݿ�
	ע��:�����m_nIndexΪ0ʱ������һ�����״�ִ��,���,���ܳ���һ�����,�������߳��Ѿ���չ��֤������,����ʱ0�������̲߳Ž�����ʼ���������ɱ�����Դ���ʳ�ͻ,���³�ʼ��ʧ��
	*/
	// return;	//����ʹ�ú���Ĵ��� 2005-4-17

	if (m_nIndex == 0) {
		// ֻ�е�һ���̱߳������޸�
		CSQLUpdate s("account_login");
		s.SetColumn("login_status", ACCOUNT_OFFLINE);
		s.SetColumn("sid", INVALID_SID);
		s.SetColumn("enable_login_time=getdate()");
		s.SetColumn("login_group", "");

		try {
			// Use modern db_connection instead of legacy m_pAuth
			SQLRETURN ret = db_connection.exec_sql_direct(s.GetStatement().c_str());
			if (DBOK(ret)) {
				LG("As2", "AuthThread#%d connected to Auth database successfully\n", m_nIndex);
			} else {
				LG("AuthDBExcp", "AuthThread::ResetAccount() exec_sql_direct failed (ret=%d)\n", ret);
			}
		} catch (CSQLException* pEx) {
			LG("AuthDBExcp", "AuthThread::ResetAccount() ExecuteSQL failed : %s\n",
			   pEx->m_strError.c_str());
		} catch (...) {
			LG("AuthDBExcp", "Unknown exception raised from AuthThread::ResetAccount()\n");
		}

		// ����һ���߳�
		AuthThread::m_Sema.unlock();
	} else {
		// �����̲߳��޸ģ����ȴ��޸����
		AuthThread::m_Sema.lock();
		// LG("As2", "AuthThread#%d����Auth���ݿ�ɹ�\n", m_nIndex);
		LG("As2", "AuthThread#%d connected to Auth database successfully\n", m_nIndex);

		// �ټ���һ���߳�
		AuthThread::m_Sema.unlock();
	}
}
void AuthThread::KickAccount(std::string& strGroup, int nId) {
	GroupServer2* pGs = g_As2->FindGroup(strGroup.c_str());
	if (pGs != nullptr) {
		WPacket wpkt = pGs->GetWPacket();
		wpkt.WriteCmd(CMD_AP_KICKUSER);
		wpkt.WriteShort(ERR_AP_ONLINE);
		wpkt.WriteLong(nId);
		pGs->SendData(wpkt);
	}
}
int AuthThread::Run() {
	Init();
	while (!GetExitFlag()) {
		g_Auth.PeekPacket(1000); // ����1���ʱ�����ɼ������е������
	}
	Exit();

	ExitThread();
	return 0;
}
void AuthThread::SetRunLabel(int nRunLabel) {
	AuthThreadPool::RunLabel[m_nIndex] = nRunLabel;
}

//
int volatile AuthThreadPool::RunLabel[AT_MAXNUM] = {0};
DWORD volatile AuthThreadPool::RunLast[AT_MAXNUM] = {0};
DWORD volatile AuthThreadPool::RunConsume[AT_MAXNUM] = {0};
unsigned int volatile AuthThreadPool::uiAuthCount = 0;
void AuthThreadPool::IncAuthCount() { ++uiAuthCount; }
unsigned int AuthThreadPool::GetAuthCount() { return uiAuthCount; }
AuthThreadPool::AuthThreadPool() {
	for (int i = 0; i < AT_MAXNUM; ++i) {
		m_Pool[i] = new AuthThread(i);
	}
	AuthThread::LoadConfig();
}
AuthThreadPool::~AuthThreadPool() {
	for (int i = 0; i < AT_MAXNUM; ++i) {
		if (m_Pool[i] != nullptr) {
			delete m_Pool[i];
			m_Pool[i] = nullptr;
		}
	}
}
void AuthThreadPool::Launch() {
	for (int i = 0; i < AT_MAXNUM; ++i) {
		m_Pool[i]->Launch();
	}
}
void AuthThreadPool::NotifyToExit() {
	for (int i = 0; i < AT_MAXNUM; ++i) {
		m_Pool[i]->NotifyToExit();
	}
}
void AuthThreadPool::WaitForExit() {
	// printf("���ڵȴ��̳߳��ڵ����߳��˳������Ժ�\n");
	printf("Loading thread pool thread exit function, please wait\n");
	for (int i = 0; i < AT_MAXNUM; ++i) {
		m_Pool[i]->WaitForExit(-1);
	}
}

int isValidMacAddress(const char* mac) {
	int i = 0;
	int s = 0;
	while (*mac) {
		if (isxdigit(*mac)) {
			i++;
		} else if (*mac == ':' || *mac == '-') {

			if (i == 0 || i / 2 - 1 != s)
				break;

			++s;
		} else {
			s = -1;
		}
		++mac;
	}
	return (i == 12 && (s == 5 || s == 0));
}

// �����ܶ���
AccountServer2* g_As2 = nullptr;
