//=============================================================================
// FileName: EntityAlloc.cpp
// Creater: ZhangXuedong
// Date: 2005.01.18
// Comment: EntityAlloc class

// modifed by knight.gong 2005.5.16. (To alloc all entities by the template of allocer)
//=============================================================================
#include "stdafx.h"
#include "EntityAlloc.h"

char g_szEntiAlloc[256] = "EntityAlloc";

namespace {

bool ValidateReturnEntityKind(int lType, Entity* pEnt) {
	if (!pEnt) {
		return true;
	}

	if (lType == defENTI_ALLOC_TYPE_CHA) {
		if (!pEnt->IsCharacter() || pEnt->IsNpc()) {
			LG("EntityAlloc", "ReturnEntity: object is not a character-pool entity id=0x%08X\n", pEnt->GetHandle());
			return false;
		}
	} else if (lType == defENTI_ALLOC_TYPE_TNPC) {
		if (!pEnt->IsNpc()) {
			LG("EntityAlloc", "ReturnEntity: object is not a talk NPC id=0x%08X\n", pEnt->GetHandle());
			return false;
		}
	}

	return true;
}

} // namespace

bool CCharacterPool::create(int lChaNum, int lTNpcNum) {
	clear();
	if (lChaNum > 0) {
		if (!m_ChaAlloc.create(lChaNum, defENTI_ALLOC_TYPE_CHA)) {
			clear();
			return false;
		}
	}
	if (lTNpcNum > 0) {
		if (!m_TalkNpcAlloc.create(lTNpcNum, defENTI_ALLOC_TYPE_TNPC)) {
			clear();
			return false;
		}
	}
	return true;
}

void CCharacterPool::clear() {
	m_ChaAlloc.clear();
	m_TalkNpcAlloc.clear();
}

CCharacter* CCharacterPool::allocCharacter() {
	CCharacter* pChar = m_ChaAlloc.alloc();
	if (!pChar) {
		LG(g_szEntiAlloc, RES_STRING(GM_GAMEAPP_CPP_00010));
	}
	return pChar;
}

mission::CTalkNpc* CCharacterPool::allocTalkNpc() {
	mission::CTalkNpc* pNpc = m_TalkNpcAlloc.alloc();
	if (!pNpc) {
		LG(g_szEntiAlloc, RES_STRING(GM_GAMEAPP_CPP_00012));
	}
	return pNpc;
}

Entity* CCharacterPool::getinfo(int lType, int lSlotId) {
	if (lType == defENTI_ALLOC_TYPE_CHA) {
		return m_ChaAlloc.getinfo(lSlotId);
	}
	if (lType == defENTI_ALLOC_TYPE_TNPC) {
		return m_TalkNpcAlloc.getinfo(lSlotId);
	}
	return nullptr;
}

void CCharacterPool::destroy(int lType, int lSlotId) {
	Entity* pEnt = getinfo(lType, lSlotId);
	if (pEnt) {
		const int handleType = pEnt->GetHandle() & 0xff000000;
		if (handleType != lType) {
			LG("EntityAlloc", "ReturnEntity: handle type mismatch id=0x%08X entityHandle=0x%08X\n",
			   lType | lSlotId, pEnt->GetHandle());
			return;
		}
		if (!ValidateReturnEntityKind(lType, pEnt)) {
			return;
		}
	}

	if (lType == defENTI_ALLOC_TYPE_CHA) {
		m_ChaAlloc.destroy(lSlotId);
	} else if (lType == defENTI_ALLOC_TYPE_TNPC) {
		m_TalkNpcAlloc.destroy(lSlotId);
	}
}

void CCharacterPool::destroy(Entity* pEnt) {
	if (!pEnt) {
		return;
	}

	const int lHandle = pEnt->GetHandle();
	const int lType = lHandle & 0xff000000;
	const int lSlotId = lHandle & 0x00ffffff;

	if ((lType != defENTI_ALLOC_TYPE_CHA && lType != defENTI_ALLOC_TYPE_TNPC) ||
		pEnt != getinfo(lType, lSlotId)) {
		LG("EntityAlloc", "ReturnEntity: entity pointer/handle mismatch handle=0x%08X\n", lHandle);
		return;
	}

	destroy(lType, lSlotId);
}

CEntityAlloc::CEntityAlloc(int lChaNum, int lItemNum, int lTNpcNum) {
	T_B
		if (!m_CharPool.create(lChaNum, lTNpcNum)) {
			LG(g_szEntiAlloc, "msgFailed to create unified character pool\n");
		}
	m_ItemAlloc.create(lItemNum, defENTI_ALLOC_TYPE_ITEM);
	m_BerthAlloc.create(1000, defENTI_ALLOC_TYPE_ENTBERTH);
	m_ResourceAlloc.create(1000, defENTI_ALLOC_TYPE_ENTRESOURCE);
	bindEntSpace();
	T_E
}

void CEntityAlloc::bindEntSpace() {
	m_CharPool.bindEntSpace(this);
	m_ItemAlloc.bindEntSpace(this);
	m_BerthAlloc.bindEntSpace(this);
	m_ResourceAlloc.bindEntSpace(this);
}

CEntityAlloc::~CEntityAlloc() {
	T_B
		m_CharPool.clear();
	m_ItemAlloc.clear();
	m_BerthAlloc.clear();
	m_ResourceAlloc.clear();
	T_E
}

CCharacter* CEntityAlloc::GetNewCha() {
	T_B return m_CharPool.allocCharacter();
	T_E
}

CItem* CEntityAlloc::GetNewItem() {
	T_B
		CItem* pItem = m_ItemAlloc.alloc();
	if (!pItem) {
		LG(g_szEntiAlloc, RES_STRING(GM_GAMEAPP_CPP_00011));
		return nullptr;
	}
	return pItem;
	T_E
}

mission::CTalkNpc* CEntityAlloc::GetNewTNpc() {
	T_B return m_CharPool.allocTalkNpc();
	T_E
}

mission::CEventEntity* CEntityAlloc::GetEventEntity(BYTE byType) {
	switch (byType) {
	case mission::BASE_ENTITY: {
	} break;

	case mission::RESOURCE_ENTITY: {
		return m_ResourceAlloc.alloc();
	} break;

	case mission::TRANSIT_ENTITY: {
	} break;

	case mission::BERTH_ENTITY: {
		return m_BerthAlloc.alloc();
	} break;
	default: {
		LG(g_szEntiAlloc, RES_STRING(GM_GAMEAPP_CPP_00013), byType);
		return nullptr;
	} break;
	}
	LG(g_szEntiAlloc, RES_STRING(GM_GAMEAPP_CPP_00014), byType);
	return nullptr;
}

Entity* CEntityAlloc::GetEntity(int lID) {
	T_B
		const int lType = lID & 0xff000000;
	const int lEntiID = lID & 0x00ffffff;

	Entity* pEnt = m_CharPool.getinfo(lType, lEntiID);
	if (pEnt) {
		return pEnt;
	}

	if (lType == defENTI_ALLOC_TYPE_ITEM) {
		return m_ItemAlloc.getinfo(lEntiID);
	}
	if (lType == defENTI_ALLOC_TYPE_ENTBERTH) {
		return m_BerthAlloc.getinfo(lEntiID);
	}
	if (lType == defENTI_ALLOC_TYPE_ENTRESOURCE) {
		return m_ResourceAlloc.getinfo(lEntiID);
	}
	return nullptr;
	T_E
}

void CEntityAlloc::ReturnEntity(int lID) {
	T_B
		const int lType = lID & 0xff000000;
	const int lEntiID = lID & 0x00ffffff;

	if (lType == defENTI_ALLOC_TYPE_CHA || lType == defENTI_ALLOC_TYPE_TNPC) {
		m_CharPool.destroy(lType, lEntiID);
		return;
	}

	Entity* pEnt = GetEntity(lID);
	if (pEnt) {
		const int handleType = pEnt->GetHandle() & 0xff000000;
		if (handleType != lType) {
			LG("EntityAlloc", "ReturnEntity: handle type mismatch id=0x%08X entityHandle=0x%08X\n", lID, pEnt->GetHandle());
			return;
		}
		if (lType == defENTI_ALLOC_TYPE_ITEM && !pEnt->IsItem()) {
			LG("EntityAlloc", "ReturnEntity: object is not an item id=0x%08X\n", lID);
			return;
		}
	}

	if (lType == defENTI_ALLOC_TYPE_ITEM) {
		m_ItemAlloc.destroy(lEntiID);
	} else if (lType == defENTI_ALLOC_TYPE_ENTBERTH) {
		m_BerthAlloc.destroy(lEntiID);
	} else if (lType == defENTI_ALLOC_TYPE_ENTRESOURCE) {
		m_ResourceAlloc.destroy(lEntiID);
	}
	T_E
}

CPlayer* CPlayerAlloc::GetNewPly() {
	T_B
		CPlayer* pCPly = m_PlyAlloc.alloc();
	if (!pCPly) {
		LG(g_szEntiAlloc, RES_STRING(GM_GAMEAPP_CPP_00015));
		return nullptr;
	}
	return pCPly;
	T_E
}

CPlayerAlloc::CPlayerAlloc(int lPlyNum) {
	if (!m_PlyAlloc.create(lPlyNum)) {
		LG(g_szEntiAlloc, "msgFailed to create player pool\n");
	}
	bindPlySpace();
}

void CPlayerAlloc::bindPlySpace() {
	m_PlyAlloc.forEachAllocSlot([this](CPlayer& ply) { ply.SetPlySpace(this); });
}
