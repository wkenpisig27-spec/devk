//---------------------------------------------------------
// EntityScript.cpp Created by knight-gong in 2005.5.12.
#include "stdafx.h" //add by alfred.shi 20080203

#include "EntityScript.h"
#include "GameAppNet.h"
#include "Character.h"
#include "SubMap.h"
#include "lua_gamectrl.h"

//---------------------------------------------------------
_DBC_USING
using namespace mission;

inline int lua_GetCurSubmap(lua_State* L) {
	if (!g_pScriptMap) {
		// LG( "entity_error", "地图指针为空！" );
		LG("entity_error", RES_STRING(GM_ENTITYSCRIPT_CPP_00001));
		// printf( "地图指针为空！" );
		printf(RES_STRING(GM_ENTITYSCRIPT_CPP_00001));
		E_LUANULL;
		return 0;
	}

	lua_pushnumber(L, LUA_TRUE);
	lua_pushlightuserdata(L, g_pScriptMap);

	return 2;
}

inline int lua_CreateEventEntity(lua_State* L) {
	BOOL bValid = lua_gettop(L) == 8 && lua_isnumber(L, 1) && lua_isuserdata(L, 2) &&
				  lua_isstring(L, 3) && lua_isnumber(L, 4) && lua_isnumber(L, 5) && lua_isnumber(L, 6) &&
				  lua_isnumber(L, 7) && lua_isnumber(L, 8);
	if (!bValid) {
		E_LUAPARAM;
		return 0;
	}

	BOOL bRet = FALSE;
	mission::CEventEntity* pEntity = nullptr;
	BYTE byType = (BYTE)lua_tonumber(L, 1);
	SubMap* pMap = (SubMap*)lua_touserdata(L, 2);
	const char* pszName = lua_tostring(L, 3);
	USHORT sID = (USHORT)lua_tonumber(L, 4);
	USHORT sInfoID = (USHORT)lua_tonumber(L, 5);
	DWORD dwxPos = (DWORD)lua_tonumber(L, 6);
	DWORD dwyPos = (DWORD)lua_tonumber(L, 7);
	USHORT sDir = (USHORT)lua_tonumber(L, 8);
	pEntity = pMap->GetOwnerApp()->CreateEntity(byType);
	if (!pMap || !pEntity) {
		E_LUANULL;
		return 0;
	}
	bRet = pEntity->Create(*pMap, pszName, sID, sInfoID, dwxPos, dwyPos, sDir);

	lua_pushnumber(L, (bRet) ? LUA_TRUE : LUA_FALSE);
	lua_pushlightuserdata(L, pEntity);

	return 2;
}

int lua_SetEntityData(lua_State* L) {
	BOOL bValid = lua_gettop(L) >= 1;
	if (!bValid) {
		E_LUAPARAM;
		return 0;
	}

	BOOL bRet = FALSE;
	mission::CEventEntity* pEntity = (mission::CEventEntity*)lua_touserdata(L, 1);
	switch (pEntity->GetType()) {
	case BASE_ENTITY: // 基本实体
	{
	} break;

	case RESOURCE_ENTITY: // 资源实体
	{
		bValid = lua_gettop(L) >= 4;
		if (!bValid) {
			E_LUAPARAM;
			return 0;
		}
		USHORT sItemID = (USHORT)lua_tonumber(L, 2);
		USHORT sCount = (USHORT)lua_tonumber(L, 3);
		USHORT sTime = (USHORT)lua_tonumber(L, 4);
		bRet = ((mission::CResourceEntity*)pEntity)->SetData(sItemID, sCount, sTime);
	} break;

	case TRANSIT_ENTITY: // 传送实体
	{
	} break;

	case BERTH_ENTITY: // 停泊实体
	{
		BOOL bValid = lua_gettop(L) >= 5;
		if (!bValid) {
			E_LUAPARAM;
			return 0;
		}
		USHORT sBerthID = (USHORT)lua_tonumber(L, 2);
		USHORT sxPos = (USHORT)lua_tonumber(L, 3);
		USHORT syPos = (USHORT)lua_tonumber(L, 4);
		USHORT sDir = (USHORT)lua_tonumber(L, 5);
		bRet = ((mission::CBerthEntity*)pEntity)->SetData(sBerthID, sxPos, syPos, sDir);
	} break;
	default: {
		E_LUAPARAM;
		return 0;
	} break;
	}

	lua_pushnumber(L, (bRet) ? LUA_TRUE : LUA_FALSE);

	return 1;
}

inline int lua_CreateDynamicPortal(lua_State* L) {
	BOOL bValid = lua_gettop(L) >= 6 && lua_isnumber(L, 2) && lua_isnumber(L, 3) && lua_isstring(L, 4) && lua_isstring(L, 5) && lua_isnumber(L, 6);
	if (!bValid) {
		E_LUAPARAM;
		return 0;
	}

	SubMap* pMap = (SubMap*)lua_touserdata(L, 1);
	if (!pMap) {
		E_LUANULL;
		return 0;
	}

	long lPosX = (long)lua_tonumber(L, 2);
	long lPosY = (long)lua_tonumber(L, 3);
	const char* szFunction = lua_tostring(L, 4);
	const char* szName = lua_tostring(L, 5);
	long lLifeTime = (long)lua_tonumber(L, 6);
	long lItemID = 193;
	if (lua_gettop(L) >= 7)
		lItemID = (long)lua_tonumber(L, 7);

	long lPortalID = pMap->CreateDynamicPortal(lPosX, lPosY, szFunction, szName, lLifeTime, lItemID);
	lua_pushnumber(L, lPortalID);
	return 1;
}

inline int lua_CreateDynamicPortalCha(lua_State* L) {
	BOOL bValid = lua_gettop(L) >= 7 && lua_isnumber(L, 3) && lua_isnumber(L, 4) && lua_isstring(L, 5) && lua_isstring(L, 6) && lua_isnumber(L, 7);
	if (!bValid) {
		E_LUAPARAM;
		return 0;
	}

	CCharacter* pOwner = (CCharacter*)lua_touserdata(L, 1);
	SubMap* pMap = (SubMap*)lua_touserdata(L, 2);
	if (!pOwner || !pMap) {
		E_LUANULL;
		return 0;
	}

	long lPosX = (long)lua_tonumber(L, 3);
	long lPosY = (long)lua_tonumber(L, 4);
	const char* szFunction = lua_tostring(L, 5);
	const char* szName = lua_tostring(L, 6);
	long lLifeTime = (long)lua_tonumber(L, 7);
	long lItemID = 193;
	if (lua_gettop(L) >= 8)
		lItemID = (long)lua_tonumber(L, 8);

	long lPortalID = pMap->CreateDynamicPortalCha(pOwner, lPosX, lPosY, szFunction, szName, lLifeTime, lItemID);
	lua_pushnumber(L, lPortalID);
	return 1;
}

inline int lua_DestroyDynamicPortal(lua_State* L) {
	BOOL bValid = lua_gettop(L) == 1 && lua_isnumber(L, 1);
	if (!bValid) {
		E_LUAPARAM;
		return 0;
	}

	long lPortalID = (long)lua_tonumber(L, 1);
	bool bResult = false;
	std::map<long, SDynamicPortal>::iterator it = g_DynamicPortalList.find(lPortalID);
	if (it != g_DynamicPortalList.end())
		bResult = it->second.pMap->DestroyDynamicPortal(lPortalID);

	lua_pushnumber(L, bResult ? LUA_TRUE : LUA_FALSE);
	return 1;
}

inline int lua_DestroyDynamicPortalCha(lua_State* L) {
	BOOL bValid = lua_gettop(L) == 1;
	if (!bValid) {
		E_LUAPARAM;
		return 0;
	}

	CCharacter* pCha = (CCharacter*)lua_touserdata(L, 1);
	if (!pCha) {
		E_LUANULL;
		return 0;
	}

	long lDestroyedCount = 0;
	for (std::map<long, SDynamicPortal>::iterator it = g_DynamicPortalList.begin(); it != g_DynamicPortalList.end();) {
		if (it->second.lOwnerChaID == pCha->GetID()) {
			long lPortalID = it->first;
			SubMap* pMap = it->second.pMap;
			++it;
			if (pMap && pMap->DestroyDynamicPortal(lPortalID))
				lDestroyedCount++;
		} else {
			++it;
		}
	}

	lua_pushnumber(L, lDestroyedCount);
	return 1;
}

BOOL RegisterEntityScript() {
	lua_State* L = g_pLuaState;

	REGFN(GetCurSubmap);
	REGFN(CreateEventEntity);
	REGFN(SetEntityData);
	REGFN(CreateDynamicPortal);
	REGFN(CreateDynamicPortalCha);
	REGFN(DestroyDynamicPortal);
	REGFN(DestroyDynamicPortalCha);

	return TRUE;
}