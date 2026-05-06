// Script.cpp Created by knight-gongjian 2004.12.1.
//---------------------------------------------------------
#include "stdafx.h"
#include "Script.h"
#include "NpcScript.h"
#include "CharScript.h"
#include "EntityScript.h"

//---------------------------------------------------------
extern const char* GetResPath(const char* pszRes);

CCharacter* g_pNoticeChar = nullptr;
lua_State* g_pLuaState = nullptr;

HANDLE lConsole = nullptr;

void print_error(lua_State* state) {
	// The error message is on top of the stack.
	// Fetch it, print it and then pop it off the stack.
	SetConsoleTextAttribute(lConsole, 14);
	const char* message = lua_tostring(state, -1);
	puts(message);
	SetConsoleTextAttribute(lConsole, 10);
	lua_pop(state, 1);
}

BOOL InitLuaScript() {
	g_pLuaState = luaL_newstate(); // lua_open();
	if (!g_pLuaState)
		return 1;

	luaL_openlibs(g_pLuaState);

	if (lConsole == nullptr)
		lConsole = GetStdHandle(STD_OUTPUT_HANDLE);

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

BOOL RegisterScript() {
	if (!RegisterCharScript() || !RegisterNpcScript())
		return FALSE;

	if (!RegisterEntityScript())
		return FALSE;

	return TRUE;
}

void ReloadLuaInit() {
	luaL_dofile(g_pLuaState, GetResPath("script/initial.lua"));
}

void ReloadLuaSdk() {
	luaL_dofile(g_pLuaState, GetResPath("script/MisSdk/NpcSdk.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisSdk/MissionSdk.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisSdk/scriptsdk.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/ScriptDefine.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/NpcDefine.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/templatesdk.lua"));

	// 由updateall会触发ai_sdk更新
	luaL_dofile(g_pLuaState, GetResPath("script/birth/birth_conf.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/ai/ai.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/calculate/skilleffect.lua"));
}

void ReloadNpcScript() {
	// 装载NPC任务数据信息
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/MissionScript01.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/MissionScript02.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/MissionScript03.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/MissionScript04.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/MissionScript05.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/MissionScript06.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/MissionScript07.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/MissionScript08.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/SendMission.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/EudemonScript.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/CharBornScript.lua"));

	// 装载NPC对话数据信息
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/NpcScript01.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/NpcScript02.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/NpcScript03.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/NpcScript04.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/NpcScript05.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/NpcScript06.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/NpcScript07.lua"));
	luaL_dofile(g_pLuaState, GetResPath("script/MisScript/NpcScript08.lua"));
}

void ReloadEntity(const char szFileName[]) {
	luaL_dofile(g_pLuaState, szFileName);
}

BOOL LoadScript() {
	// Reset global Lua tables before reloading to prevent memory accumulation
	// This prevents issues like NpcInfoList growing unbounded on &updateall
	lua_getglobal(g_pLuaState, "ResetAllGlobalTables");
	if (lua_isfunction(g_pLuaState, -1)) {
		int nStatus = lua_pcall(g_pLuaState, 0, 0, 0);
		if (nStatus) {
			LG("script_reload", "ResetAllGlobalTables call failed");
			lua_settop(g_pLuaState, 0);
		}
	} else {
		lua_pop(g_pLuaState, 1);
	}

	// Load all scripts in order:
	// 1) initial.lua - utility functions, hooks, globals
	// 2) SDK scripts - NpcSdk, MissionSdk, ScriptSdk (registers maps via AddMap/SetMap), 
	//    ScriptDefine (map definitions), NpcDefine, templates, birth, AI, skilleffect
	// 3) NPC/Mission scripts - MissionScript01-08, NpcScript01-08, etc.
	ReloadLuaInit();
	ReloadLuaSdk();
	ReloadNpcScript();

	// Force full Lua garbage collection after mass script reload.
	// Each dofile() creates thousands of new Lua objects (functions, tables, strings).
	// Without explicit GC, the incremental collector can't keep up with the burst,
	// causing memory to grow with each &updateall until the server overflows.
	int memBefore = lua_gc(g_pLuaState, LUA_GCCOUNT, 0);
	lua_gc(g_pLuaState, LUA_GCCOLLECT, 0);
	int memAfter = lua_gc(g_pLuaState, LUA_GCCOUNT, 0);
	LG("script_reload", "Lua GC after reload: %dKB -> %dKB (freed %dKB)", memBefore, memAfter, memBefore - memAfter);

	return TRUE;
}
