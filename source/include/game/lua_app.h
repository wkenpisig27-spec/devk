
//-----------------
// 设置app当前scene
//-----------------
inline int lua_appSetCaption(lua_State* L) {
	// 参数合法性判别
	BOOL bValid = (lua_gettop(L) == 1 && lua_isstring(L, 1));
	if (!bValid) {
		PARAM_ERROR
		return 0;
	}
	g_pGameApp->SetCaption(lua_tostring(L, 1));
	return 0;
}

//-----------------
// 取得app当前scene
//-----------------
inline int lua_appGetCurScene(lua_State* L) {
	// 参数合法性判别
	BOOL bValid = (lua_gettop(L) == 0);
	if (!bValid) {
		PARAM_ERROR
		return 0;
	}
	lua_pushlightuserdata(L, g_pGameApp->GetCurScene());
	return 1;
}

//-----------------
// 设置app当前scene
//-----------------
inline int lua_appSetCurScene(lua_State* L) {
	// 参数合法性判别
	BOOL bValid = (lua_gettop(L) == 1 && lua_islightuserdata(L, 1));
	if (!bValid) {
		PARAM_ERROR
		return 0;
	}
	CGameScene* pScene = (CGameScene*)lua_touserdata(L, 1);
	g_pGameApp->GotoScene(pScene, false);
	return 1;
}

inline int lua_appPlaySound(lua_State* L) {
	// 参数合法性判别
	BOOL bValid = (lua_gettop(L) == 1 && lua_isnumber(L, 1));
	if (!bValid) {
		PARAM_ERROR
		return 0;
	}
	g_pGameApp->PlaySound((int)lua_tonumber(L, 1));
	return 0;
}

inline int lua_appCreateScene(lua_State* L) {
	// 参数合法性判别
	BOOL bValid = (lua_gettop(L) == 5 && lua_isstring(L, 1) &&
				   lua_isnumber(L, 2) && lua_isnumber(L, 3) &&
				   lua_isnumber(L, 4) && lua_isnumber(L, 5));

	if (!bValid) {
		PARAM_ERROR
		return 0;
	}
	char* pszMapName = (char*)lua_tostring(L, 1);

	int nMaxCha = (int)lua_tonumber(L, 2);
	int nMaxObj = (int)lua_tonumber(L, 3);
	int nMaxItem = (int)lua_tonumber(L, 4);
	int nMaxEff = (int)lua_tonumber(L, 5);

	// 创建Scene
	CGameScene* pScene = nullptr;

	lua_pushlightuserdata(L, pScene);
	return 1;
}

inline int lua_appUpdateRender(lua_State* L) {
	MPTerrain* pTerrain = g_pGameApp->GetCurScene()->GetTerrain();
	if (pTerrain)
		pTerrain->UpdateRender();
	return 0;
}

// Toggle melee attack restriction for caster classes (Cleric, Seal Master, Voyager)
// Lua: SetDisableMeleeForCasters(true/false)
extern bool g_bDisableMeleeForCasters;
inline int lua_SetDisableMeleeForCasters(lua_State* L) {
	BOOL bValid = (lua_gettop(L) == 1 && lua_isboolean(L, 1));
	if (!bValid) {
		PARAM_ERROR
		return 0;
	}
	g_bDisableMeleeForCasters = lua_toboolean(L, 1) != 0;
	return 0;
}

// Query current state of melee attack restriction
// Lua: local disabled = GetDisableMeleeForCasters()
inline int lua_GetDisableMeleeForCasters(lua_State* L) {
	lua_pushboolean(L, g_bDisableMeleeForCasters ? 1 : 0);
	return 1;
}

#include "autoattack.h"

// Toggle auto-attack mode on/off
// Lua: ToggleAutoAttack()
inline int lua_ToggleAutoAttack(lua_State* L) {
	CWorldScene* pScene = dynamic_cast<CWorldScene*>(g_pGameApp->GetCurScene());
	if (!pScene) {
		SCENE_NULL_ERROR
		return 0;
	}
	pScene->GetMouseDown().GetAutoAttack()->ToggleAutoAttack();
	return 0;
}

// Query current auto-attack toggle state
// Lua: local enabled = GetAutoAttackEnabled()
inline int lua_GetAutoAttackEnabled(lua_State* L) {
	CWorldScene* pScene = dynamic_cast<CWorldScene*>(g_pGameApp->GetCurScene());
	if (!pScene) {
		lua_pushboolean(L, 0);
		return 1;
	}
	bool enabled = pScene->GetMouseDown().GetAutoAttack()->IsToggleEnabled();
	lua_pushboolean(L, enabled ? 1 : 0);
	return 1;
}

// Enable/disable auto-target (auto-select nearest monster when target dies)
// Lua: SetAutoTarget(bool)
inline int lua_SetAutoTarget(lua_State* L) {
	if (lua_gettop(L) < 1) return 0;
	bool v = lua_toboolean(L, 1) != 0;
	CWorldScene* pScene = dynamic_cast<CWorldScene*>(g_pGameApp->GetCurScene());
	if (!pScene) { SCENE_NULL_ERROR return 0; }
	pScene->GetMouseDown().GetAutoAttack()->SetAutoTarget(v);
	return 0;
}

// Query auto-target state
// Lua: local enabled = GetAutoTarget()
inline int lua_GetAutoTarget(lua_State* L) {
	CWorldScene* pScene = dynamic_cast<CWorldScene*>(g_pGameApp->GetCurScene());
	if (!pScene) { lua_pushboolean(L, 0); return 1; }
	bool v = pScene->GetMouseDown().GetAutoAttack()->IsAutoTarget();
	lua_pushboolean(L, v ? 1 : 0);
	return 1;
}

// Enable/disable melee fallback in auto-attack
// Lua: SetMeleeEnabled(bool)
inline int lua_SetMeleeEnabled(lua_State* L) {
	if (lua_gettop(L) < 1) return 0;
	bool v = lua_toboolean(L, 1) != 0;
	CWorldScene* pScene = dynamic_cast<CWorldScene*>(g_pGameApp->GetCurScene());
	if (!pScene) { SCENE_NULL_ERROR return 0; }
	pScene->GetMouseDown().GetAutoAttack()->SetMeleeEnabled(v);
	return 0;
}

// Query melee fallback state
// Lua: local enabled = GetMeleeEnabled()
inline int lua_GetMeleeEnabled(lua_State* L) {
	CWorldScene* pScene = dynamic_cast<CWorldScene*>(g_pGameApp->GetCurScene());
	if (!pScene) { lua_pushboolean(L, 1); return 1; }
	bool v = pScene->GetMouseDown().GetAutoAttack()->IsMeleeEnabled();
	lua_pushboolean(L, v ? 1 : 0);
	return 1;
}
