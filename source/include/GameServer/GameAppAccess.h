#pragma once

#include "Character.h"
#include "SubMap.h"
#include "GameApp.h"
#include "Script.h"

inline CGameApp* ActiveGameApp() {
	return g_pGameApp;
}

inline CGameApp* AppFromCharacter(CCharacter* pCha) {
	if (pCha) {
		if (CGameApp* pApp = pCha->GetOwnerApp()) {
			return pApp;
		}
	}
	return g_pGameApp;
}

inline CGameApp* AppFromEntity(Entity* pEnt) {
	if (pEnt) {
		if (CGameApp* pApp = pEnt->GetOwnerApp()) {
			return pApp;
		}
	}
	return g_pGameApp;
}

inline CGameApp* AppFromSubMap(SubMap* pMap) {
	if (pMap) {
		if (CGameApp* pApp = pMap->GetOwnerApp()) {
			return pApp;
		}
	}
	return g_pGameApp;
}

// Active mission / packet / opcode script context.
inline CGameApp* AppFromNoticeChar() {
	return AppFromCharacter(g_pNoticeChar);
}
