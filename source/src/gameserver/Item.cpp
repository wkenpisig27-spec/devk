//=============================================================================
// FileName: Item.cpp
// Creater: ZhangXuedong
// Date: 2004.09.21
// Comment: CItem class
//=============================================================================
#include "stdafx.h"
#include "Item.h"
#include "GameCommon.h"
#include "GameAppNet.h"
#include "SubMap.h"
#include "gamedb.h"

_DBC_USING

CItem::CItem() {
	T_B
		chValid = 0;
	m_pCItemRecord = 0;
	m_SGridContent.sID = 0;
	m_lFromEntityID = 0;
	m_chSpawType = enumITEM_APPE_NATURAL;
	T_E
}

void CItem::Initially() {
	T_B
	Entity::Initially();

	chValid = 0;
	m_pCItemRecord = 0;
	m_SGridContent.sID = 0;
	m_chSpawType = enumITEM_APPE_NATURAL;
	m_ulStartTick = GetTickCount();
	m_ulOnTick = g_Config.m_lItemShowTime * 1000;
	m_ulProtOnTick = g_Config.m_lItemProtTime * 1000;
	m_ulProtID = 0;
	m_ulProtHandle = 0;
	m_chProtType = enumITEM_PROT_OWN;
	T_E
}

void CItem::Finally() {
	T_B
	// Safety check: only call GoOut if we have a valid submap
	// and we're not in a shutdown state
	extern volatile BOOL g_bGameEnd;
	if (m_submap && !g_bGameEnd) {
		try {
			m_submap->GoOut(this);
		} catch (...) {
			LG("Item exception", "Exception in CItem::Finally GoOut for item %s (ID %u)\n", 
				GetName(), GetID());
		}
	}
	m_submap = nullptr;  // Clear to prevent double-cleanup
	Entity::Finally();
	T_E
}

void CItem::OnBeginSeen(CCharacter* pCMainCha) {
	T_B
		WPACKET pk = GETWPACKET();
	WRITE_CMD(pk, CMD_MC_ITEMBEGINSEE);
	// 基本数据
	WRITE_LONG(pk, m_ID); // world ID
	WRITE_LONG(pk, m_lHandle);
	WRITE_LONG(pk, m_pCItemRecord->lID);  // ID
	WRITE_LONG(pk, GetShape().centre.x);  // 当前x位置
	WRITE_LONG(pk, GetShape().centre.y);  // 当前y位置
	WRITE_SHORT(pk, m_sAngle);			  // 方向
	WRITE_SHORT(pk, m_SGridContent.sNum); // 个数
	//
	WRITE_CHAR(pk, m_chSpawType);
	WRITE_LONG(pk, m_lFromEntityID);
	// 事件信息
	WriteEventInfo(pk);

	pCMainCha->ReflectINFof(this, pk); // 通告
	T_E
}

void CItem::OnEndSeen(CCharacter* pCMainCha) {
	T_B
		WPACKET pk = GETWPACKET();
	WRITE_CMD(pk, CMD_MC_ITEMENDSEE);
	WRITE_LONG(pk, m_ID);			   // ID
	pCMainCha->ReflectINFof(this, pk); // 通告
	T_E
}

void CItem::Run(dbc::uLong ulCurTick) {
	if (m_ulProtID != 0)
		if (m_ulProtOnTick != 0 && ulCurTick - m_ulStartTick >= m_ulProtOnTick) // 保护时间消失
			m_ulProtID = 0;

	// Note: Item expiration and deletion is now handled by GameItemRun()
	// using deferred deletion to avoid iterator invalidation crashes.
	// See ShouldExpire() method and GameApp::GameItemRun() for the implementation.
	
	// Handle boat captain certificate item cleanup when expiring
	if (ShouldExpire(ulCurTick)) {
		CItemRecord* pItem = m_pCItemRecord;
		if (pItem != nullptr) {
			if (pItem->sType == enumItemTypeBoat) {
				game_db.SaveBoatDelTag(this->GetGridContent()->GetDBParam(enumITEMDBP_INST_ID), 1);
			}
		}
		if (!m_submap) {
			LG("Item disappear error", "item %s(ID %u，HANDLE %u，position[%d %d]) when it disappear find the map is null\n", GetName(), GetID(), GetHandle(), GetPos().x, GetPos().y);
		}
		// Free() is now called by GameItemRun() after iteration completes
	}
}