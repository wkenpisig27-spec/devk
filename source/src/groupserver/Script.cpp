// Script.cpp Created by knight-gongjian 2004.12.1.
//---------------------------------------------------------
#include "stdafx.h"
#include "Script.h"
#include "Parser.h"
#include "LuaFunc.h"
#include <string>
#include <list>
using namespace std;

//---------------------------------------------------------

const char* GetResPath(const char* pszRes) {
	static char g_szTableName[255];
	string str = "resource";
	if (str.size() > 0) {
		str += "/";
	}
	str += pszRes;
	strncpy_s(g_szTableName, sizeof(g_szTableName), str.c_str(), _TRUNCATE);
	return g_szTableName;
}

// CCharacter* g_pNoticeChar = nullptr;
lua_State* g_pLuaState = nullptr;

BOOL InitLuaScript() {
	g_pLuaState = luaL_newstate();
	luaL_openlibs(g_pLuaState);

	if (!RegisterScript())
		return FALSE;

	if (!LoadScript())
		return FALSE;

	return TRUE;
}

BOOL CloseLuaScript() {
	if (g_pLuaState)
		lua_close(g_pLuaState);
	g_pLuaState = nullptr;
	return TRUE;
}

extern list<string> g_luaFNList;
BOOL RegisterScript() {
	lua_State* L = g_pLuaState;

	// 注册Lua函数
	if (!RegisterLuaFunc())
		return FALSE;

	return TRUE;
}

void ReloadLuaInit() {
}

void ReloadLuaSdk() {
}

void ReloadScript() {
	// 装载NPC任务数据信息
	luaL_dofile(g_pLuaState, GetResPath("script/script01.lua"));
}

void ReloadEntity(const char szFileName[]) {
	luaL_dofile(g_pLuaState, szFileName);
}

BOOL LoadScript() {
	ReloadLuaInit();
	ReloadLuaSdk();
	ReloadScript();

	// 执行Lua脚本

	return TRUE;
}
