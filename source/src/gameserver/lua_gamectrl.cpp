#include "stdafx.h"
#include "lua_gamectrl.h"
#include "Birthplace.h"

using namespace std;

std::list<CCharacter*> g_HelpNPCList;

// 添加出生地与出生点
int lua_AddBirthPoint(lua_State* L) {
	T_B
		// 参数合法性判别
		BOOL bValid = (lua_gettop(L) == 4 && lua_isstring(L, 1) && lua_isstring(L, 2) && lua_isnumber(L, 3) && lua_isnumber(L, 4));
	if (!bValid) {
		PARAM_ERROR;
		return 0;
	}

	const char* pszLocation = lua_tostring(L, 1);
	const char* pszMapName = lua_tostring(L, 2);
	int x = (int)lua_tonumber(L, 3);
	int y = (int)lua_tonumber(L, 4);

	g_BirthMgr.AddBirthPoint(pszLocation, pszMapName, x, y);
	// LG("birth", "添加出生点[%s] [%s] %d %d\n", pszLocation, pszMapName, x, y);
	return 0;
	T_E
}

// 清除所有出生地与出生点
int lua_ClearAllBirthPoint(lua_State* L) {
	T_B
		g_BirthMgr.ClearAll();
	// LG("birth", "清除了所有出生点\n");
	return 0;
	T_E
}

extern const char* GetResPath(const char* pszRes);
void ReloadAISdk() {
	luaL_dofile(g_pLuaState, GetResPath("script/ai/ai.lua"));
}

// char g_TradeName[][32] =
//{
//	"人人",
//	"扔",
//	"捡",
//	"店买",
//	"卖",
//	"任务给",
//	"任务收",
//	"买货",
//	"卖货",
//	"进游戏",
//	"出游戏",
//	"人摊",
//	"消耗",
//	"删除",
//	"银行",
//	"装备"
// };
const char* g_TradeName[] =
	{
		RES_STRING(GM_LUA_GAMECTRL_CPP_00001),
		RES_STRING(GM_LUA_GAMECTRL_CPP_00002),
		RES_STRING(GM_LUA_GAMECTRL_CPP_00003),
		RES_STRING(GM_LUA_GAMECTRL_CPP_00004),
		RES_STRING(GM_LUA_GAMECTRL_CPP_00005),
		RES_STRING(GM_LUA_GAMECTRL_CPP_00006),
		RES_STRING(GM_LUA_GAMECTRL_CPP_00007),
		RES_STRING(GM_LUA_GAMECTRL_CPP_00008),
		RES_STRING(GM_LUA_GAMECTRL_CPP_00009),
		RES_STRING(GM_LUA_GAMECTRL_CPP_00010),
		RES_STRING(GM_LUA_GAMECTRL_CPP_00011),
		RES_STRING(GM_LUA_GAMECTRL_CPP_00012),
		RES_STRING(GM_LUA_GAMECTRL_CPP_00013),
		RES_STRING(GM_LUA_GAMECTRL_CPP_00014),
		RES_STRING(GM_LUA_GAMECTRL_CPP_00015),
		RES_STRING(GM_LUA_GAMECTRL_CPP_00016),
};

#define TL_TIME_ONE_HOUR 6 * 60 * 60 * 1000
void TL(int nType, const char* pszCha1, const char* pszCha2, const char* pszTrade) {
	if (!g_Config.m_bLogDB) {
		static short sInit = 1;
		static std::string strName;
		static DWORD dwLastTime = -1;
		static DWORD dwCount = 10000;
		if (dwCount++ > 1000) {
			DWORD dwTime = GetTickCount();
			if (dwTime - dwLastTime >= TL_TIME_ONE_HOUR || sInit == 1) {
				dwCount = 0;
				strName = "trade";
				dwLastTime = dwTime;
				SYSTEMTIME st;
				GetLocalTime(&st);
				char szData[128];
				sprintf(szData, "%d-%d-%d-%d", st.wYear, st.wMonth, st.wDay, st.wHour);
				strName += szData;
			}
		}

		LG(strName.c_str(), "%7s [%17s] [%17s] [%s]\n", g_TradeName[nType], pszCha1, pszCha2, pszTrade);
		g_pGameApp->Log(g_TradeName[nType], pszCha1, "", pszCha2, "", pszTrade);
		sInit = 0;
	} else {
		// Add by lark.li 20080324 begin
		// static CThrdLock lock;
		// lock.Lock();
		// g_pGameApp->TradeLog(g_TradeName[nType], pszCha1, pszCha2,pszTrade);
		// lock.Unlock();
		// End
	}
}

map<string, string> g_HelpList;

// 添加帮助信息, 接受2个参数: 关键字 帮助文字
int lua_AddHelpInfo(lua_State* L) {
	BOOL bValid = (lua_gettop(L) == 2 && lua_isstring(L, 1) && lua_isstring(L, 2));
	if (!bValid) {
		return 0;
	}

	const char* pszKey = (const char*)lua_tostring(L, 1);
	const char* pszText = (const char*)lua_tostring(L, 2);

	g_HelpList[pszKey] = pszText;

	return 0;
}

const char* FindHelpInfo(const char* pszKey) {
	map<string, string>::iterator it = g_HelpList.find(pszKey);
	if (it == g_HelpList.end()) {
		return nullptr;
	}
	return (*it).second.c_str();
}

void AddHelpInfo(const char* pszKey, const char* pszInfo) {
	if (strlen(pszKey) == 0)
		return;
	if (strlen(pszInfo) == 0)
		return;

	g_HelpList[pszKey] = pszInfo;

	// LG("help", "目前帮助条目数 = %d\n", g_HelpList.size());
	LG("help", "now helplist amount = %d\n", g_HelpList.size());
}

void AddMonsterHelp(int nScriptID, int x, int y) {
	CChaRecord* pCChaRecord = GetChaRecordInfo(nScriptID);
	if (pCChaRecord == nullptr)
		return;

	// char szHelp[255]; sprintf(szHelp, "听说在本海域的%d, %d附近有你打听的生物出没!", x/100, y/100);
	char szHelp[255];
	sprintf(szHelp, RES_STRING(GM_LUA_GAMECTRL_CPP_00019), x / 100, y / 100);

	AddHelpInfo(pCChaRecord->szDataName, szHelp);
}

void AddHelpNPC(CCharacter* pNPC) {
	// LG("init", "成功添加帮助NPC[%s]\n", pNPC->GetName());
	LG("init", "Succeed add HelpNPC[%s]\n", pNPC->GetName());
	g_HelpNPCList.push_back(pNPC);
}

// 通过脚本添加帮助NPC
int lua_AddHelpNPC(lua_State* L) {
	BOOL bValid = (lua_gettop(L) == 1 && lua_isstring(L, 1));
	if (!bValid) {
		return 0;
	}

	const char* pszName = (const char*)lua_tostring(L, 1); // 获得帮助NPC的名字

	// 按名字查找NPC对象
	g_pGameApp->BeginGetTNpc();
	mission::CTalkNpc* pCTNpc;
	while (pCTNpc = g_pGameApp->GetNextTNpc()) {
		if (!strcmp(pCTNpc->GetName(), pszName))
			AddHelpNPC(pCTNpc);
	}

	return 0;
}

int lua_ClearHelpNPC(lua_State* L) {
	g_HelpNPCList.clear();
	return 0;
}

// 测试DBLog
int lua_TestDBLog(lua_State* L) {
	// 参数合法性判别
	BOOL bValid = (lua_gettop(L) == 1 && lua_isnumber(L, 1));
	if (!bValid) {
		PARAM_ERROR
		return 0;
	}

	int nCnt = (int)lua_tonumber(L, 1);

	MPTimer t;
	t.Begin();
	for (int i = 0; i < nCnt; i++) {
		g_pGameApp->Log("newtest", "abcdefg", "1234567", "000000", "qqqppp", "abcdefghijklmnopqrstuvwxyz");
	}
	LG("dblog", "Add Time = %d\n", t.End());

	return 0;
}

int lua_GetMapDataByName(lua_State* L) {
	T_B
		BOOL bValid = (lua_gettop(L) == 1 && lua_isstring(L, 1));
	if (!bValid) {
		PARAM_ERROR
		return 0;
	}
	const char* mpName = lua_tostring(L, 1);
	if (mpName == nullptr) {
		PARAM_ERROR;
		return 0;
	}
	CMapRes* pMap = g_pGameApp->FindMapByName(mpName);
	if (pMap) {
		lua_pushlightuserdata(L, (CMapRes*)pMap);
		return 1;
	}
	return 0;

	T_E
}

void RegisterLuaAI(lua_State* L) {
	T_B

		// Register CCharacter as a LuaBridge class (enables OOP in Lua scripts)
		RegisterCCharacterClass(L);

	// 通用
	lua_register(L, "view", lua_view);
	lua_register(L, "EXLG", lua_EXLG);
	lua_register(L, "PRINT", lua_PRINT);
	lua_register(L, "GetResPath", lua_GetResPath);

	// For AI

	lua_register(L, "SetCurMap", lua_SetCurMap);
	lua_register(L, "GetChaID", lua_GetChaID);
	lua_register(L, "CreateChaNearPlayer", lua_CreateChaNearPlayer);
	lua_register(L, "CreateCha", lua_CreateCha);
	lua_register(L, "CreateChaX", lua_CreateChaX);
	lua_register(L, "CreateChaEx", lua_CreateChaEx);
	lua_register(L, "QueryChaAttr", lua_QueryChaAttr);
	lua_register(L, "GetChaAIType", lua_GetChaAIType);
	lua_register(L, "SetChaAIType", lua_SetChaAIType);
	lua_register(L, "GetChaTypeID", lua_GetChaTypeID);
	lua_register(L, "GetChaVision", lua_GetChaVision);
	lua_register(L, "GetChaTarget", lua_GetChaTarget);
	lua_register(L, "SetChaTarget", lua_SetChaTarget);
	lua_register(L, "GetChaHost", lua_GetChaHost);
	lua_register(L, "SetChaHost", lua_SetChaHost);
	lua_register(L, "GetPetNum", lua_GetPetNum);
	lua_register(L, "GetChaFirstTarget", lua_GetChaFirstTarget);
	lua_register(L, "GetChaPos", lua_GetChaPos);
	lua_register(L, "GetChaBlockCnt", lua_GetChaBlockCnt);
	lua_register(L, "SetChaBlockCnt", lua_SetChaBlockCnt);
	lua_register(L, "ChaMove", lua_ChaMove);
	lua_register(L, "ChaMoveToSleep", lua_ChaMoveToSleep);
	lua_register(L, "GetChaSpawnPos", lua_GetChaSpawnPos);
	lua_register(L, "SetChaPatrolState", lua_SetChaPatrolState);
	lua_register(L, "GetChaPatrolState", lua_GetChaPatrolState);
	lua_register(L, "GetChaPatrolPos", lua_GetChaPatrolPos);
	lua_register(L, "SetChaPatrolPos", lua_SetChaPatrolPos);
	lua_register(L, "SetChaFaceAngle", lua_SetChaFaceAngle);
	lua_register(L, "GetChaChaseRange", lua_GetChaChaseRange);
	lua_register(L, "SetChaChaseRange", lua_SetChaChaseRange);
	lua_register(L, "ChaUseSkill", lua_ChaUseSkill);
	lua_register(L, "ChaUseSkill2", lua_ChaUseSkill2);
	lua_register(L, "GetChaByRange", lua_GetChaByRange);
	lua_register(L, "GetChaSetByRange", lua_GetChaSetByRange);
	lua_register(L, "ClearHideChaByRange", lua_ClearHideChaByRange);
	lua_register(L, "IsChaFighting", lua_IsChaFighting);
	lua_register(L, "IsPosValid", lua_IsPosValid);
	lua_register(L, "IsMapBlock", lua_IsMapBlock);
	lua_register(L, "IsChaSleeping", lua_IsChaSleeping);
	lua_register(L, "ChaActEyeshot", lua_ChaActEyeshot);
	lua_register(L, "GetChaFacePos", lua_GetChaFacePos);
	lua_register(L, "SetChaEmotion", lua_SetChaEmotion);
	lua_register(L, "FindItem", lua_FindItem);
	lua_register(L, "PickItem", lua_PickItem);
	lua_register(L, "GetItemPos", lua_GetItemPos);
	lua_register(L, "EnableAI", lua_EnableAI);
	lua_register(L, "GetChaSkillNum", lua_GetChaSkillNum);
	lua_register(L, "GetChaSkillInfo", lua_GetChaSkillInfo);
	lua_register(L, "GetChaHarmByNo", lua_GetChaHarmByNo);
	lua_register(L, "GetFirstAtker", lua_GetFirstAtker);
	lua_register(L, "AddHate", lua_AddHate);
	lua_register(L, "GetChaHateByNo", lua_GetChaHateByNo);
	lua_register(L, "HarmLog", lua_HarmLog);
	lua_register(L, "SummonCha", lua_SummonCha);
	lua_register(L, "DelCha", lua_DelCha);
	lua_register(L, "SetChaLifeTime", lua_SetChaLifeTime);

	// 数值计算
	lua_register(L, "SetChaAttrMax", lua_SetChaAttrMax);
	lua_register(L, "GetChaDefaultName", lua_GetChaDefaultName);
	lua_register(L, "SetChaAttrI", lua_SetChaAttrI);
	lua_register(L, "GetChaAttrI", lua_GetChaAttrI);
	lua_register(L, "IsPlayer", lua_IsPlayer);
	lua_register(L, "IsChaInRegion", lua_IsChaInRegion);

	// 组队
	lua_register(L, "IsChaInTeam", lua_IsChaInTeam);
	lua_register(L, "GetTeamCha", lua_GetTeamCha);

	// 出生地与出生点
	lua_register(L, "AddBirthPoint", lua_AddBirthPoint);
	lua_register(L, "ClearAllBirthPoint", lua_ClearAllBirthPoint);

	// 天气区域
	lua_register(L, "AddWeatherRegion", lua_AddWeatherRegion);
	lua_register(L, "ClearMapWeather", lua_ClearMapWeather);

	// 帮助NPC
	lua_register(L, "AddHelpInfo", lua_AddHelpInfo);
	lua_register(L, "AddHelpNPC", lua_AddHelpNPC);
	lua_register(L, "ClearHelpNPC", lua_ClearHelpNPC);

	// 船只计时
	lua_register(L, "SetBoatCtrlTick", lua_SetBoatCtrlTick);
	lua_register(L, "GetBoatCtrlTick", lua_GetBoatCtrlTick);

	lua_register(L, "GetRoleID", lua_GetRoleID);
	lua_register(L, "UnlockItem", lua_UnlockItem);
	lua_register(L, "SetMonsterAttr", lua_SetMonsterAttr);
	// 测试脚本
	lua_register(L, "TestDBLog", lua_TestDBLog);

	T_E
}

/*
				卡片召唤宠物实现流程

一:	使用卡片道具, 执行summon怪物

二: 为summon出来的怪物设置定时消失的技能状态, 状态持续时间就是怪物的生命时间

三: summon出来的怪物，如果技能状态时间到后消失, 则怪物被清除

四: 主人经过跳转点时, summon出来的怪物被自动清除

五:	宠物AI

	function() 没有目标

		if(检测附近是否有合适的目标)
		{
			设置目标对象
		}
		else
		{
			如果距离主人太远, 则靠近
		}

		取出攻击主人的对象列表的第一个
		if(不为空)
		{
			设置目标对象
		}

	end


	function() 有目标

		if(目标距离主人太远 || 目标已死 || 目标下线已不存在)
		{
			清除目标
		}
		else
		{
			对目标使用技能
		}
	end


六: 卡片的属性设置

	属性1: 怪物编号
	属性2: 标识怪物等级的数值
	属性3: 可以使用的次数

	属性1当获得卡片时产生, 属性2和属性3可以动态改变



*/
