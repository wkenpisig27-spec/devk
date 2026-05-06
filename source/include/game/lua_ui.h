// lua_ui.h - Lua UI helper functions

#pragma once

#include "PacketCmd.h"

inline int lua_uiGetForm(lua_State* L) {
	return 1;
}

inline int lua_uiHideAll(lua_State* L) {
	CFormMgr::s_Mgr.SetEnabled(FALSE);
	return 0;
}

// Boss Timer UI Functions
// Request boss timer data from server
inline int lua_uiRequestBossTimerData(lua_State* L) {
	CS_BossTimerRequest();
	return 0;
}
