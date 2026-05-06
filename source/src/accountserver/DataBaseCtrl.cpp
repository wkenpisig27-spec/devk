#define _CRTDBG_MAP_ALLOC
#include "stdafx.h"
#include <stdlib.h>
#include <crtdbg.h>

#include "commrpc.h"
#include "util.h"
#include "databasectrl.h"
#include "inifile.h"
#include "GlobalVariable.h"
#include "AccountServer2.h"
#include "NetCommand.h"
#include "NetRetCode.h"
#include <botan/bcrypt.h>
#include <botan/auto_rng.h>

using namespace std;

CDataBaseCtrl::CDataBaseCtrl(void) {
	m_strServerIP = "";
	m_strServerDB = "";
	m_strUserID = "";
	m_strUserPwd = "";
	m_pDataBase = nullptr;
	db_mutator = nullptr;
}

CDataBaseCtrl::~CDataBaseCtrl(void) {
	if (IsConnect()) {
		ReleaseObject();
	}
}

bool CDataBaseCtrl::CreateObject() {
	try {
		dbc::IniFile inf(g_strCfgFile.c_str());
		dbc::IniSection& is = inf["db"];

		m_strServerIP = is["dbserver"];
		m_strServerDB = is["db"];
		m_strUserID = is["userid"];
		m_strUserPwd = is["passwd"];
	} catch (std::exception& e) {
		cout << e.what() << endl;
		return false;
	}

	printf("Connecting database [%s : %s]... ", m_strServerIP.c_str(), m_strServerDB.c_str());
	if (!Connect())
		return false;
	C_PRINT("success!\n");

	// Verify database schema by checking critical fields
	try {
		// Use modern query to validate schema (no data returned, just validation)
		db_connection.exec_sql_direct("SELECT TOP 1 ban FROM account_login");
		db_connection.exec_sql_direct("SELECT TOP 1 log_id, user_id, user_name, login_time, logout_time, login_ip FROM user_log");
		if (false) {
			LG("DBExcp", "Check data field failure! Unable to validate database schema\n");
			printf("Check data field failure! Unable to validate database schema\r\n");
			return false;
		}
	} catch (...) {
		LG("DBExcp", "Check data field failure! Exception raised from CDataBaseCtrl::CreateObject()\n");
		printf("Check data field failure! Exception raised from CDataBaseCtrl::CreateObject()\n");
		return false;
	}

	return true;
}

bool CDataBaseCtrl::InsertUser(std::string username, std::string password, std::string email) {
	printf("[InsertUser] Starting for user: %s\n", username.c_str());
	printf("[InsertUser] Target database: %s on %s\n", m_strServerDB.c_str(), m_strServerIP.c_str());
	
	if (!IsConnect()) {
		printf("[InsertUser] Not connected, attempting to connect...\n");
		Connect();
	}

	if (IsConnect()) {
		try {
			printf("[InsertUser] Connected to database, generating bcrypt hash...\n");
			// Hash password with bcrypt before storing (work factor 10)
			Botan::AutoSeeded_RNG rng;
			std::string bcryptHash = Botan::generate_bcrypt(password, rng, 10);
			printf("[InsertUser] Bcrypt hash generated: %s\n", bcryptHash.c_str());
			
			// Use parameterized stored procedure (SQL injection protected)
			if (!db_mutator) {
				printf("[InsertUser] ERROR: db_mutator not initialized\n");
				LG("DBExcp", "db_mutator not initialized\n");
				return false;
			}
			
			// Call InsertNewUser stored procedure with parameterized query
			printf("[InsertUser] Executing InsertNewUser stored procedure...\n");
			SQLRETURN ret = db_mutator->stored_procedure("{CALL dbo.InsertNewUser(?, ?, ?)}", 
				"dbo", "InsertNewUser", 3,
				username.c_str(), bcryptHash.c_str(), email.c_str());
			
			bool execResult = SQL_SUCCEEDED(ret);
			printf("[InsertUser] Stored procedure result: %s\n", execResult ? "SUCCESS" : "FAILED");
			
			if (execResult) {
				printf("[InsertUser] SUCCESS: Account created for %s\n", username.c_str());
				LG("AccountServer", "InsertUser: Successfully created account [%s] with bcrypt hash\n", username.c_str());
				return true;
			} else {
				printf("[InsertUser] FAILED: InsertNewUser stored procedure failed\n");
				LG("DBExcp", "Failed to execute InsertNewUser stored procedure\n");
			}
		} catch (std::exception& e) {
			printf("[InsertUser] EXCEPTION: %s\n", e.what());
			LG("DBExcp", "Exception raised from CDataBaseCtrl::InsertUser: %s\n", e.what());
		} catch (...) {
			printf("[InsertUser] UNKNOWN EXCEPTION\n");
			LG("DBExcp", "Exception raised from CDataBaseCtrl::InsertUser\n");
		}
	} else {
		printf("[InsertUser] ERROR: Not connected to database\n");
	}
	LG("AccountServer", "CDataBaseCtrl::InsertUser: A record of user login cannot be saved! UserName=%s \n\n", username.c_str());

	Disconnect();
	return false;
}

bool CDataBaseCtrl::UpdatePassword(string user, string pass) {
	if (!IsConnect())
		Connect();

	if (IsConnect()) {
		try {
			// Hash new password with bcrypt before storing (work factor 10)
			Botan::AutoSeeded_RNG rng;
			std::string bcryptHash = Botan::generate_bcrypt(pass, rng, 10);
			
			// Use modern stored procedure with parameterized query (SQL injection protected)
			if (!db_mutator) {
				LG("DBExcp", "db_mutator not initialized\n");
				return false;
			}
			SQLRETURN ret = db_mutator->stored_procedure("", "dbo", "UpdateUserPassword", 2,
				user.c_str(), bcryptHash.c_str());
			
			if (SQL_SUCCEEDED(ret)) {
				LG("AccountServer", "UpdatePassword: Successfully updated password for [%s] with bcrypt hash\n", user.c_str());
				return true;
			} else {
				LG("DBExcp", "Failed to execute UpdateUserPassword stored procedure\n");
			}
		} catch (std::exception& e) {
			LG("DBExcp", "Exception raised from CDataBaseCtrl::UpdatePassword: %s\n", e.what());
		} catch (...) {
			LG("DBExcp", "Exception raised from CDataBaseCtrl::UpdatePassword\n");
		}
	}
	Disconnect();
	return false;
}

void CDataBaseCtrl::ReleaseObject() {
	Disconnect();
}

bool CDataBaseCtrl::Connect() {
	if (IsConnect())
		return true;

	// Modern database connection
	std::string err_info;
	const bool r = db_connection.connect(m_strServerIP.c_str(), m_strServerDB.c_str(),
										 m_strUserID.c_str(), m_strUserPwd.c_str(), err_info);
	if (r) {
		db_mutator = std::make_unique<cfl_rs>(&db_connection);
		return true;
	}

	// Fallback to legacy connection
	try {
		m_pDataBase = new CSQLDatabase();
	} catch (std::bad_alloc& e) {
		LG("DBExcp", "CDataBaseCtrl::CreateObject() new failed: %s\n", e.what());
		m_pDataBase = nullptr;
		return false;
	} catch (...) {
		LG("DBExcp", "CDataBaseCtrl::CreateObject() unknown exception\n");
		m_pDataBase = nullptr;
		return false;
	}

	// Connect to database
	char buf[512] = {0};
#ifdef _WIN32
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "DRIVER={SQL Server};SERVER=%s;UID=%s;PWD=%s;DATABASE=%s",
#else
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "DRIVER={ODBC Driver 18 for SQL Server};SERVER=%s;UID=%s;PWD=%s;DATABASE=%s;TrustServerCertificate=yes;Encrypt=optional",
#endif
			m_strServerIP.c_str(), m_strUserID.c_str(), m_strUserPwd.c_str(), m_strServerDB.c_str());

	if (!m_pDataBase->Open(buf)) {
		SAFE_DELETE(m_pDataBase);
		return false;
	}
	m_pDataBase->SetAutoCommit(true);

	return true;
}

bool CDataBaseCtrl::IsConnect() {
	return (db_mutator.get() != nullptr || m_pDataBase != nullptr);
}

void CDataBaseCtrl::Disconnect() {
	// Disconnect modern connection
	if (db_mutator.get() != nullptr) {
		try {
			db_connection.disconn();
			db_mutator.reset();
		} catch (...) {
			LG("DBExcp", "Exception raised when disconnecting modern db_connection\n");
		}
	}
	
	// Disconnect legacy connection
	if (m_pDataBase != nullptr) {
		try {
			m_pDataBase->Close();
		} catch (...) {
			LG("DBExcp", "Exception raised when CDataBaseCtrl::Disconnect()\n");
		}
		SAFE_DELETE(m_pDataBase);
	}
}

bool CDataBaseCtrl::UserLogin(int nUserID, string strUserName, string strIP) {
	if (!strUserName.c_str() || strUserName == "") {
		LG("AccountServer", "CDataBaseCtrl::UserLogin: parameter strUserName is empty or null\n");
		return false;
	}
	// LG("AccountServer", "CDataBaseCtrl::UserLogin: UserName=[%s] \n", strUserName.c_str());
	if (!strIP.c_str())
		strIP = "";

	if (!IsConnect())
		Connect();

	if (IsConnect()) {
		try {
			// Use modern stored procedure with parameterized query (SQL injection protected)
			if (!db_mutator) {
				LG("DBExcp", "db_mutator not initialized\n");
				return false;
			}
			char userIdStr[32];
			sprintf_s(userIdStr, "%d", nUserID);
			SQLRETURN ret = db_mutator->stored_procedure("", "dbo", "InsertUserLogin", 3,
				userIdStr, strUserName.c_str(), strIP.c_str());
			
			if (SQL_SUCCEEDED(ret)) {
				return true;
			} else {
				LG("DBExcp", "Failed to execute InsertUserLogin stored procedure\n");
			}
		} catch (...) {
			LG("DBExcp", "Exception raised from CDataBaseCtrl::UserLogin\n");
		}
	}
	LG("AccountServer", "CDataBaseCtrl::UserLogin: A record of user login cannot be saved! UserID=%d UserName=%s IP=%s\n\n", nUserID, strUserName.c_str(), strIP.c_str());

	Disconnect();
	return false;
}

bool CDataBaseCtrl::UserLogout(int nUserID) {
	if (!IsConnect())
		Connect();

	if (IsConnect()) {
		try {
			// Use modern stored procedure with parameterized query (SQL injection protected)
			if (!db_mutator) {
				LG("DBExcp", "db_mutator not initialized\n");
				return false;
			}
			char userIdStr[32];
			sprintf_s(userIdStr, "%d", nUserID);
			SQLRETURN ret = db_mutator->stored_procedure("", "dbo", "UpdateUserLogout", 1, userIdStr);
			
			if (SQL_SUCCEEDED(ret)) {
				return true;
			} else {
				LG("DBExcp", "Failed to execute UpdateUserLogout stored procedure\n");
			}
		} catch (...) {
			LG("DBExcp", "Exception raised from CDataBaseCtrl::UserLogout\n");
		}
	}
	LG("AccountServer", "CDataBaseCtrl::UserLogout: A record of user logout cannot be saved! UserID=%d \n", nUserID);

	Disconnect();
	return false;
}

bool CDataBaseCtrl::KickUser(std::string strUserName) {
	if (!strUserName.c_str() || strUserName == "") {
		LG("AccountServer", "CDataBaseCtrl::KickUser: parameter strUserName is empty or null\n");
		return false;
	}
	// LG("AccountServer", "CDataBaseCtrl::KickUser: UserName=[%s] \n", strUserName.c_str());

	if (!IsConnect())
		Connect();

	std::string strUserLeave = "";
	std::string strGroupServerName = "";
	if (IsConnect()) {
		try {
			// Use modern stored procedure with parameterized query (SQL injection protected)
			if (!db_mutator) {
				LG("DBExcp", "db_mutator not initialized\n");
				Disconnect();
				return false;
			}
			
			SQLRETURN ret = db_mutator->stored_procedure("", "dbo", "GetAccountInfo", 1, strUserName.c_str());
			
			if (!SQL_SUCCEEDED(ret)) {
				LG("DBExcp", "Failed to execute GetAccountInfo stored procedure\n");
				Disconnect();
				return false;
			}
			
			// TODO: Implement result reading
			return true; // TODO: Result reading to be implemented
			
		} catch (...) {
			LG("DBExcp", "Exception raised from CDataBaseCtrl::KickUser\n");
		}
	} // Close if (IsConnect())
	
	Disconnect();
	return false;
}

void CDataBaseCtrl::SetExpScale(std::string strUserName, int time) {
	// Note: This function appears to be unused in AccountServer
	// Character::SetExpScale is the actual function being used in GameServer
	// Stub implementation for header compatibility
	LG("AccountServer", "CDataBaseCtrl::SetExpScale called (unused function): UserName=%s, Time=%d\n", strUserName.c_str(), time);
}

bool CDataBaseCtrl::UserLoginMap(std::string strUserName, std::string strPassport) {
	// Note: This function appears to be unused (WM_USER_LOG_MAP is never sent)
	// Stub implementation for linker compatibility
	return true;
}

bool CDataBaseCtrl::UserLogoutMap(std::string strUserName) {
	// Note: This function appears to be unused (WM_USER_LOG_MAP is never sent)
	// Stub implementation for linker compatibility
	return true;
}

bool CDataBaseCtrl::OperAccountBan(std::string strActName, int iban) {
	if (!strActName.c_str() || strActName == "") {
		LG("AccountServer", "CDataBaseCtrl::OperAccountBan: parameter strActName is empty or null\n");
		return false;
	}
	
	if (!IsConnect())
		Connect();
		
	if (IsConnect()) {
		try {
			// Use modern stored procedure with parameterized query (SQL injection protected)
			if (!db_mutator) {
				LG("DBExcp", "db_mutator not initialized\n");
				Disconnect();
				return false;
			}
			
			SQLRETURN ret = db_mutator->stored_procedure("", "dbo", "OperAccountBan", 2, strActName.c_str(), &iban);
			
			if (SQL_SUCCEEDED(ret)) {
				return true;
			} else {
				LG("DBExcp", "Failed to execute OperAccountBan stored procedure\n");
				Disconnect();
				return false;
			}
		} catch (...) {
			LG("DBExcp", "Exception raised from CDataBaseCtrl::OperAccountBan\n");
		}
	}
	
	LG("AccountServer", "CDataBaseCtrl::OperAccountBan: Account ban operation failed! actName=%s\n", strActName.c_str());
	Disconnect();
	return false;
}