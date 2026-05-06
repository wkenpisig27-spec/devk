#define _CRTDBG_MAP_ALLOC
#include "atltime.h"
#include <stdlib.h>
#include <crtdbg.h>

#include "AccountServer2.h"
#include "PlayerMgr.h"
// #include "MyThread.h"
#include "GlobalVariable.h"

struct sPlayerData {
	CTime ctLoginTime;
};

CPlayerMgr::CPlayerMgr(void) {
}

CPlayerMgr::~CPlayerMgr(void) {
}

void CPlayerMgr::Initial() {
	m_mapPlayers.clear();
}

void CPlayerMgr::PlayerLogin(std::string strPlayerName) {
	if (!strPlayerName.c_str()) {
		printf("User name is <NULL> where login!\r\n");
		return;
	}
	sPlayerData sData;
	sData.ctLoginTime = CTime::GetCurrentTime();
	m_mapPlayers[strPlayerName.c_str()] = sData;
}

void CPlayerMgr::PlayerLogout(std::string strPlayerName) {
	if (!strPlayerName.c_str()) {
		printf("User name is <NULL> where logout!\r\n");
		return;
	}
	StringMap::const_iterator iter = m_mapPlayers.find(strPlayerName.c_str());
	if (iter == m_mapPlayers.end()) {
		printf("Unable update the user live time when logout! User=[%s]\r\n", strPlayerName.c_str());
		return;
	}
	AuthThread* pThread = (AuthThread*)(g_TlsIndex.GetPointer());
	if (!pThread) {
		printf("Get AuthThread Error!\r\n");
		return;
	}
	CSQLDatabase* pDB = pThread->GetSQLDatabase();

	sPlayerData sData = iter->second;
	CTimeSpan ctSpan = CTime::GetCurrentTime() - sData.ctLoginTime;
	if (ctSpan > CTimeSpan(5) && ctSpan < CTimeSpan(30, 0, 0, 0)) // 记录有效时间5秒到30天
	{
		__int64 i64Span = ctSpan.GetTotalSeconds();
		if (false) {
			// Use stored procedure to prevent SQL injection
			// Note: This code path is disabled, but kept for reference
			// If enabled, use: CALL dbo.UpdateTomAccountLiveTime(@timeSpan, @userName)
			// pDB->StoredProcedure("UpdateTomAccountLiveTime", i64Span, strPlayerName.c_str());
		} else {
			// 原计时方式在登陆时已经修改 (Original timing method modified at login)
		}
	}
	m_mapPlayers.erase(strPlayerName.c_str());
}