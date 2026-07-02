#pragma once

#include "Character.h"
#include "SubMap.h"
#include "GameApp.h"
#include "Script.h"

// ---------------------------------------------------------------------------
// GameApp access policy (post g_pGameApp narrowing)
//
// g_pGameApp is defined in GameSMain.cpp for bootstrap only (allocate, bind,
// teardown). Runtime code should use owner-bound access:
//   - Entity::GetOwnerApp() / CPlayer::GetOwnerApp() / SubMap::GetOwnerApp()
//   - GameServerApp::GetGameApp() after BindGameApp()
//
// Use the helpers below only when no owner exists (script notice char, dev UI,
// game-thread bootstrap). ActiveGameApp() is the single intentional read of the
// global outside GameSMain allocation/teardown.
// ---------------------------------------------------------------------------

inline CGameApp* ActiveGameApp() {
	return g_pGameApp;
}

inline CGameApp* AppFromCharacter(CCharacter* pCha) {
	if (pCha) {
		if (CGameApp* pApp = pCha->GetOwnerApp()) {
			return pApp;
		}
	}
	return ActiveGameApp();
}

inline CGameApp* AppFromEntity(Entity* pEnt) {
	if (pEnt) {
		if (CGameApp* pApp = pEnt->GetOwnerApp()) {
			return pApp;
		}
	}
	return ActiveGameApp();
}

inline CGameApp* AppFromSubMap(SubMap* pMap) {
	if (pMap) {
		if (CGameApp* pApp = pMap->GetOwnerApp()) {
			return pApp;
		}
	}
	return ActiveGameApp();
}

// Active mission / packet / opcode script context.
inline CGameApp* AppFromNoticeChar() {
	return AppFromCharacter(g_pNoticeChar);
}
