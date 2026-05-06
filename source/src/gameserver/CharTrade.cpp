// CharTrade.cpp Created by knight-gongjian 2004.12.7.
//---------------------------------------------------------
#include "stdafx.h"
#include "CharTrade.h"
#include "GameApp.h"
#include "GameAppNet.h"
#include "SubMap.h"
#include "Player.h"
#include "GameDB.h"
#include "lua_gamectrl.h"
//---------------------------------------------------------
using namespace std;

mission::CTradeSystem g_TradeSystem;

mission::CStoreSystem g_StoreSystem;

namespace mission {
//----------------------------------------------------
// CTradeData implemented

CTradeData::CTradeData(dbc::uLong lSize)
	: PreAllocStru(1){T_B

						  T_E}

	  CTradeData::~CTradeData(){T_B

									T_E}

	  //----------------------------------------------------
	  // CTradeSystem implemented

	  CTradeSystem::CTradeSystem(){T_B

									   T_E}

	  CTradeSystem::~CTradeSystem(){T_B

										T_E}

	  // äº¤æ˜“æ“ä½œ
	  BOOL CTradeSystem::Request(BYTE byType, CCharacter & character, DWORD dwAcceptID) {
	T_B if (character.GetPlyMainCha()->IsStoreEnable()) {
		// character.SystemNotice("æ— æ³•äº¤æ˜“!");
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00001));
		return FALSE;
	}

	if (character.GetBoat()) {
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00002));
		return FALSE;
	}

	if (character.GetStallData()) {
		// character.SystemNotice( "æ­£åœ¨æ‘†æ‘Šï¼Œä¸å¯ä»¥äº¤æ˜“" );
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00003));
		return FALSE;
	}

	// add by ALLEN 2007-10-16
	if (character.IsReadBook()) {
		// character.SystemNotice("æ­£åœ¨è¯»ä¹¦ï¼Œä¸å¯ä»¥äº¤æ˜“");
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00004));
		return FALSE;
	}

	if (character.m_CKitbag.IsLock() || !character.GetActControl(enumACTCONTROL_ITEM_OPT)) {
		// character.SystemNotice( "èƒŒåŒ…å·²è¢«é”å®šï¼Œä¸å¯ä»¥äº¤æ˜“ï¼" );
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00005));
		return FALSE;
	}

	if (character.GetPlyMainCha() && character.GetPlyMainCha()->m_CKitbag.IsLock()) {
		// character.SystemNotice( "èƒŒåŒ…å·²è¢«é”å®šï¼Œä¸å¯ä»¥äº¤æ˜“ï¼" );
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00005));
		return FALSE;
	}

	if (character.GetPlyMainCha() && character.GetPlyMainCha()->m_CKitbag.IsPwdLocked()) {
		// character.SystemNotice( "èƒŒåŒ…å·²è¢«å¯†ç é”å®šï¼Œä¸å¯ä»¥äº¤æ˜“ï¼" );
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00006));
		return FALSE;
	}

	CCharacter* pMain = &character;
	CCharacter* pChar = pMain->GetSubMap()->FindCharacter(dwAcceptID, pMain->GetShape().centre);
	if (pChar == nullptr || !pChar->IsPlayerCha()) {
		// pMain->SystemNotice( "è¢«é‚€è¯·çŽ©å®¶å·²ç»ç¦»å¼€!" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00007));
		return FALSE;
	}

	// Check if target player is in offline mode (ghost character)
	if (pChar->_dwStallTick > 0) {
		pMain->SystemNotice("That player is currently offline.");
		return FALSE;
	}

	if (pChar->GetPlayer()->GetBankNpc()) {
		// pMain->SystemNotice( "å¯¹æ–¹æ­£åœ¨ä½¿ç”¨é“¶è¡Œï¼Œè¯·ç¨å€™å†è¯•ï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00008));
		return FALSE;
	}

	if (pChar->GetPlyMainCha()->IsStoreEnable()) {
		// character.SystemNotice("æ— æ³•äº¤æ˜“!");
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00001));
		return FALSE;
	}

	if (!pMain->GetPlyMainCha() || !pChar->GetPlyMainCha()) {
		/*pMain->SystemNotice( "äº¤æ˜“è§’è‰²ä¸å­˜åœ¨ï¼" );
		pChar->SystemNotice( "äº¤æ˜“è§’è‰²ä¸å­˜åœ¨ï¼" );*/
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00010));
		pChar->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00010));
	}

	if (pMain->GetPlyMainCha()->GetLevel() < 6) {
		// pMain->SystemNotice("æ‚¨çš„ç­‰çº§ä¸å¤Ÿ,æ— æ³•äº¤æ˜“!");
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00011));
		return FALSE;
	}

	if (pChar->GetBoat()) {
		// character.SystemNotice( "è§’è‰²%sæ­£åœ¨é€ èˆ¹ï¼Œä¸å¯ä»¥äº¤æ˜“", pChar->GetName() );
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00012), pChar->GetName());
		return FALSE;
	}

	if (pChar->GetStallData()) {
		// character.SystemNotice( "è§’è‰²%sæ­£åœ¨æ‘†æ‘Šï¼Œä¸å¯ä»¥äº¤æ˜“", pChar->GetName() );
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00013), pChar->GetName());
		return FALSE;
	}

	// add by ALLEN 2007-10-16
	if (pChar->IsReadBook()) {
		// character.SystemNotice( "è§’è‰²%sæ­£åœ¨è¯»ä¹¦ï¼Œä¸å¯ä»¥äº¤æ˜“", pChar->GetName() );
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00014), pChar->GetName());
		return FALSE;
	}

	if (pChar->m_CKitbag.IsLock() || !pChar->GetActControl(enumACTCONTROL_ITEM_OPT)) {
		// character.SystemNotice( "è§’è‰²%sèƒŒåŒ…å·²è¢«é”å®šï¼Œä¸å¯ä»¥äº¤æ˜“ï¼", pChar->GetName() );
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00015), pChar->GetName());
		return FALSE;
	}

	if (pChar->GetPlyMainCha()->m_CKitbag.IsPwdLocked()) {
		// character.SystemNotice( "è§’è‰²%sèƒŒåŒ…å·²è¢«å¯†ç é”å®šï¼Œä¸å¯ä»¥äº¤æ˜“ï¼", pChar->GetName() );
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00016), pChar->GetName());
		return FALSE;
	}

	if (byType == mission::TRADE_CHAR) {
		pMain = pMain->GetPlyMainCha();
		pChar = pChar->GetPlyMainCha();
	} else {
		if (pChar == pChar->GetPlyMainCha() || pMain == pMain->GetPlyMainCha()) {
			/*pMain->SystemNotice( "äº¤æ˜“è§’è‰²ç±»åž‹ä¸åŒ¹é…ï¼" );
			pChar->SystemNotice( "äº¤æ˜“è§’è‰²ç±»åž‹ä¸åŒ¹é…ï¼" );*/
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00017));
			pChar->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00017));
			return FALSE;
		}
	}

	if (pMain->GetPlayer()->IsLuanchOut() || pChar->GetPlayer()->IsLuanchOut()) {
		// pMain->SystemNotice( "æµ·ä¸Šç¦æ­¢äº¤æ˜“ï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00018));
		return FALSE;
	}
	/*else if( pMain->GetPlayer()->IsLuanchOut() && !pChar->GetPlayer()->IsLuanchOut() )
	{
		pMain->SystemNotice( "ä½ å·²ç»å‡ºæµ·ï¼ŒçŽ°åœ¨ä¸å¯ä»¥è¯·æ±‚ä¸Žä»–äº¤æ˜“ï¼" );
		return FALSE;
	}*/
	else if (pMain->GetPlayer()->IsInForge()) {
		// pMain->SystemNotice( "ä½ çŽ°åœ¨ä¸å¯ä»¥è¯·æ±‚ä¸Žå…¶ä»–äººäº¤æ˜“ï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00019));
		return FALSE;
	}

	CTradeData* pTradeData1 = pChar->GetTradeData();
	if (pTradeData1) {
		// pMain->SystemNotice( "%sæ­£åœ¨äº¤æ˜“ä¸­ï¼", pChar->GetName() );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00020), pChar->GetName());
		return FALSE;
	}

	CTradeData* pTradeData2 = pMain->GetTradeData();
	if (pTradeData2) {
		return FALSE;
	}

	// å‘é€äº¤æ˜“é‚€è¯·
	WPACKET packet = GETWPACKET();
	WRITE_CMD(packet, CMD_MC_CHARTRADE);
	WRITE_SHORT(packet, CMD_MC_CHARTRADE_REQUEST);
	WRITE_CHAR(packet, byType);
	WRITE_LONG(packet, character.GetID());

	pChar->ReflectINFof(pChar, packet);
	return TRUE;
	T_E
}

BOOL CTradeSystem::IsTradeDist(CCharacter& Char1, CCharacter& Char2, DWORD dwDist) {
	T_B
		DWORD dwxDist = (Char1.GetShape().centre.x - Char2.GetShape().centre.x) *
						(Char1.GetShape().centre.x - Char2.GetShape().centre.x);
	DWORD dwyDist = (Char1.GetShape().centre.y - Char2.GetShape().centre.y) *
					(Char1.GetShape().centre.y - Char2.GetShape().centre.y);
	return (dwxDist + dwyDist < dwDist * 100);
	T_E
}

BOOL CTradeSystem::Accept(BYTE byType, CCharacter& character, DWORD dwRequestID) {
	T_B if (character.GetBoat()) {
		// character.SystemNotice( "æ­£åœ¨å»ºé€ èˆ¹åªï¼Œä¸å¯ä»¥äº¤æ˜“ï¼" );
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00002));
		return FALSE;
	}

	if (character.GetStallData()) {
		// character.SystemNotice( "æ­£åœ¨æ‘†æ‘Šï¼Œä¸å¯ä»¥äº¤æ˜“" );
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00003));
		return FALSE;
	}

	// add by ALLEN 2007-10-16
	if (character.IsReadBook()) {
		// character.SystemNotice("æ­£åœ¨è¯»ä¹¦ï¼Œä¸å¯ä»¥äº¤æ˜“");
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00004));
		return FALSE;
	}

	if (character.m_CKitbag.IsLock() || !character.GetActControl(enumACTCONTROL_ITEM_OPT)) {
		// character.SystemNotice( "èƒŒåŒ…å·²è¢«é”å®šï¼Œä¸å¯ä»¥äº¤æ˜“ï¼" );
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00005));
		return FALSE;
	}

	if (character.GetPlyMainCha() && character.GetPlyMainCha()->m_CKitbag.IsLock()) {
		// character.SystemNotice( "èƒŒåŒ…å·²è¢«é”å®šï¼Œä¸å¯ä»¥äº¤æ˜“ï¼" );
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00005));
		return FALSE;
	}

	if (character.GetPlyMainCha() && character.GetPlyMainCha()->m_CKitbag.IsPwdLocked()) {
		// character.SystemNotice( "èƒŒåŒ…å·²è¢«å¯†ç é”å®šï¼Œä¸å¯ä»¥äº¤æ˜“ï¼" );
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00006));
		return FALSE;
	}

	if (!character.IsLiveing()) {
		character.SystemNotice("Dead pirates are unable to trade.");
		return FALSE;
	}
	CCharacter* pMain = &character;
	if (pMain->GetID() == dwRequestID) {
		// pMain->SystemNotice( "ä¸å¯ä»¥è¯·æ±‚å’Œè‡ªå·±äº¤æ˜“ï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00021));
		return FALSE;
	}

	CCharacter* pChar = pMain->GetSubMap()->FindCharacter(dwRequestID, pMain->GetShape().centre);
	if (pChar == nullptr) {
		// pMain->SystemNotice( "å‘é€é€šçŸ¥è¯¥è§’è‰²å·²ç»ç¦»å¼€!" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00022));
		return FALSE;
	}

	// Check if target player is in offline mode (ghost character)
	if (pChar->_dwStallTick > 0) {
		pMain->SystemNotice("That player is currently offline.");
		return FALSE;
	}

	if (!pChar->IsLiveing()) {
		pChar->SystemNotice("Dead pirates are unable to trade.");
		return FALSE;
	}

	if (character.GetPlyMainCha()->GetPlayer()->GetBankNpc()) {
		// character.SystemNotice("ä½ æ­£åœ¨ä½¿ç”¨é“¶è¡Œï¼Œä¸å¯ä»¥äº¤æ˜“");
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00023));
		// pChar->SystemNotice( "å¯¹æ–¹æ­£åœ¨ä½¿ç”¨é“¶è¡Œï¼Œè¯·ç¨å€™å†è¯•ï¼" );
		pChar->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00008));
		return FALSE;
	}

	if (character.GetPlyMainCha()->IsStoreEnable() || pChar->GetPlyMainCha()->IsStoreEnable()) {
		/*character.SystemNotice("æ— æ³•äº¤æ˜“!");
		pChar->SystemNotice("æ— æ³•äº¤æ˜“!");*/
		character.SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00001));
		pChar->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00001));
		return FALSE;
	}

	if (!pChar->IsLiveing()) {
		// pMain->SystemNotice( "è¯·æ±‚äº¤æ˜“æ–¹å·²æ­»äº¡ä¸å¯äº¤æ˜“ï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00025));
		return FALSE;
	}

	if (!pMain->IsLiveing()) {
		// pMain->SystemNotice( "ä½ å·²æ­»äº¡ä¸å¯äº¤æ˜“ï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00026));
		return FALSE;
	}

	if (pChar->GetBoat()) {
		// pChar->SystemNotice( "æ­£åœ¨å»ºé€ èˆ¹åªï¼Œä¸å¯ä»¥äº¤æ˜“ï¼" );
		pChar->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00002));
		return FALSE;
	}

	if (pChar->GetStallData()) {
		// pChar->SystemNotice( "æ­£åœ¨æ‘†æ‘Šï¼Œä¸å¯ä»¥äº¤æ˜“ï¼" );
		pChar->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00003));
		return FALSE;
	}

	// add by ALLEN 2007-10-16
	if (pChar->IsReadBook()) {
		// pChar->SystemNotice( "æ­£åœ¨è¯»ä¹¦ï¼Œä¸å¯ä»¥äº¤æ˜“ï¼" );
		pChar->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00004));
		return FALSE;
	}

	if (pChar->m_CKitbag.IsLock() || !pChar->GetActControl(enumACTCONTROL_ITEM_OPT)) {
		// pChar->SystemNotice( "èƒŒåŒ…å·²è¢«é”å®šï¼Œä¸å¯ä»¥äº¤æ˜“ï¼" );
		pChar->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00005));
		return FALSE;
	}

	if (pChar->GetPlyMainCha()->m_CKitbag.IsPwdLocked()) {
		// pChar->SystemNotice( "èƒŒåŒ…å·²è¢«å¯†ç é”å®šï¼Œä¸å¯ä»¥äº¤æ˜“ï¼" );
		pChar->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00006));
		return FALSE;
	}

	if (pChar->GetPlayer()->GetBankNpc()) {
		// pChar->SystemNotice("é“¶è¡Œæ‰“å¼€æ—¶ï¼Œä¸å…è®¸äº¤æ˜“ï¼");
		pChar->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00027));
		return FALSE;
	}

	if (byType == mission::TRADE_CHAR) {
		pMain = pMain->GetPlyMainCha();
		pChar = pChar->GetPlyMainCha();
	} else {
		if (pChar == pChar->GetPlyMainCha() || pMain == pMain->GetPlyMainCha()) {
			/*pMain->SystemNotice( "äº¤æ˜“è§’è‰²ç±»åž‹ä¸åŒ¹é…ï¼" );
			pChar->SystemNotice( "äº¤æ˜“è§’è‰²ç±»åž‹ä¸åŒ¹é…ï¼" );*/
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00017));
			pChar->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00017));
			return FALSE;
		}
	}

	if (!pMain->GetPlayer()->IsLuanchOut() && pChar->GetPlayer()->IsLuanchOut()) {
		// pMain->SystemNotice( "å¯¹æ–¹å·²ç»å‡ºæµ·ï¼Œä½ çŽ°åœ¨ä¸å¯ä»¥æŽ¥å—è¯·æ±‚ä¸Žä»–äº¤æ˜“ï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00029));
		return FALSE;
	} else if (pMain->GetPlayer()->IsLuanchOut() && !pChar->GetPlayer()->IsLuanchOut()) {
		// pMain->SystemNotice( "ä½ å·²ç»å‡ºæµ·ï¼ŒçŽ°åœ¨ä¸å¯ä»¥æŽ¥å—è¯·æ±‚ä¸Žä»–äº¤æ˜“ï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00024));
		return FALSE;
	} else if (pMain->GetPlayer()->IsInForge()) {
		// pMain->SystemNotice( "ä½ çŽ°åœ¨ä¸å¯ä»¥è¯·æ±‚ä¸Žå…¶ä»–ä»–äººäº¤æ˜“ï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00030));
		return FALSE;
	}

	// if( !IsTradeDist( *pMain, *pChar, ROLE_MAXSIZE_TRADEDIST - 400 ) )
	//{
	//	// è¶…å‡ºè§’è‰²äº¤æ˜“è·ç¦»ï¼Œå‘é€è§’è‰²å·²ç¦»å¼€ä¿¡æ¯ï¼
	//	return FALSE;
	// }

	CTradeData* pTradeData1 = pChar->GetTradeData();
	if (pTradeData1) {
		// pMain->SystemNotice( "%sæ­£åœ¨äº¤æ˜“ä¸­ï¼", pChar->GetName() );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00020), pChar->GetName());
		return FALSE;
	}

	CTradeData* pTradeData2 = pMain->GetTradeData();
	if (pTradeData2) {
		// è‡ªå·±æ­£åœ¨ä¸Žå…¶ä»–çŽ‹å®¶è¿›è¡Œäº¤æ˜“ä¸­
		return FALSE;
	}

	// åˆ†é…çš„èµ„æºç”±äº¤æ˜“é‚€è¯·è€…é‡Šæ”¾
	CTradeData* pData = g_pGameApp->m_TradeDataHeap.Get();
	if (pData == nullptr) {
		// pMain->SystemNotice( "åˆ†é…äº¤æ˜“æ•°æ®ç¼“å†²å¤±è´¥ï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00031));
		return FALSE;
	}
	pData->Clear();
	pData->pRequest = pChar;
	pData->pAccept = pMain;
	pData->dwTradeTime = GetTickCount();
	pData->bTradeStart = ROLE_TRADE_START;

	//// äº¤æ˜“éªŒè¯åœ°ç‚¹
	// pData->sxPos = (USHORT)pMain->GetShape().centre.x;
	// pData->syPos = (USHORT)pMain->GetShape().centre.y;

	// è®¾ç½®äº¤æ˜“äº¤æ˜“ä¿¡æ¯æ•°æ®
	pMain->SetTradeData(pData);
	pChar->SetTradeData(pData);

	// é”å®šäº¤æ˜“è§’è‰²çŠ¶æ€
	pMain->TradeAction(TRUE);
	pChar->TradeAction(TRUE);
	CKitbag& ReqBag = pData->pRequest->m_CKitbag;
	CKitbag& AcpBag = pData->pAccept->m_CKitbag;
	ReqBag.Lock();
	AcpBag.Lock();

	// å‘é€è§’è‰²äº¤æ˜“é¡µå‘½ä»¤
	WPACKET packet = GETWPACKET();
	WRITE_CMD(packet, CMD_MC_CHARTRADE);
	WRITE_SHORT(packet, CMD_MC_CHARTRADE_PAGE);
	WRITE_CHAR(packet, byType);
	WRITE_LONG(packet, pMain->GetID());
	WRITE_LONG(packet, pChar->GetID());
	pChar->ReflectINFof(pMain, packet);
	pMain->ReflectINFof(pMain, packet);
	return TRUE;
	T_E
}

BOOL CTradeSystem::Cancel(BYTE byType, CCharacter& character, DWORD dwCharID) {
	T_B
		CCharacter* pMain = &character;
	if (!pMain->GetPlyMainCha()) {
		// pMain->SystemNotice( "äº¤æ˜“è§’è‰²ä¸å­˜åœ¨ï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00010));
	}

	if (byType == mission::TRADE_CHAR) {
		pMain = pMain->GetPlyMainCha();
	} else {
		if (pMain == pMain->GetPlyMainCha()) {
			// pMain->SystemNotice( "äº¤æ˜“è§’è‰²ç±»åž‹ä¸åŒ¹é…ï¼" );
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00017));
			return FALSE;
		}
	}

	CTradeData* pTradeData2 = pMain->GetTradeData();
	if (!pTradeData2) {
		char szData[128];
		// sprintf( szData, "Cancel:è¯¥è§’è‰²%så¹¶ä¸äº¤æ˜“ä¸­!\n", pMain->GetName() );
		sprintf(szData, RES_STRING(GM_CHARTRADE_CPP_00032), pMain->GetName());
		LG("trade_error", szData);
		return FALSE;
	}

	CCharacter* pChar;
	if (pMain->GetID() == dwCharID) {
		// printf( "æŠ¥æ–‡ä¿¡æ¯é”™è¯¯ï¼Œä¸èƒ½å–æ¶ˆå’Œè‡ªå·±IDç›¸åŒçš„äº¤æ˜“æ“ä½œï¼" );
		printf(RES_STRING(GM_CHARTRADE_CPP_00033));
		return FALSE;
	} else if (pTradeData2->pRequest->GetID() == dwCharID) {
		pChar = pTradeData2->pRequest;
	} else if (pTradeData2->pAccept->GetID() == dwCharID) {
		pChar = pTradeData2->pAccept;
	} else {
		// pMain->SystemNotice( "å®¢æˆ·ç«¯è¯·æ±‚çš„äº¤æ˜“å¯¹è±¡ä¿¡æ¯é”™è¯¯ï¼ID = 0x%x", dwCharID );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00034), dwCharID);
		return FALSE;
	}

	CTradeData* pTradeData1 = pChar->GetTradeData();
	if (pTradeData1 == nullptr || pTradeData2 != pTradeData1) {
		// pMain->SystemNotice( "é”™è¯¯:çŽ©å®¶%sæœªå’Œä½ è¿›è¡Œäº¤æ˜“ï¼", pChar->GetName() );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00009), pChar->GetName());
		return FALSE;
	}

	// æ¸…é™¤é“å…·æ ä½é”å®šçŠ¶æ€
	pTradeData1->pAccept->m_CKitbag.UnLock();
	pTradeData1->pRequest->m_CKitbag.UnLock();

	ResetItemState(*pTradeData1->pAccept, *pTradeData1);
	ResetItemState(*pTradeData1->pRequest, *pTradeData1);

	pTradeData1->pAccept->SetTradeData(nullptr);
	pTradeData1->pRequest->SetTradeData(nullptr);

	// å–æ¶ˆè§’è‰²äº¤æ˜“
	WPACKET packet = GETWPACKET();
	WRITE_CMD(packet, CMD_MC_CHARTRADE);
	WRITE_SHORT(packet, CMD_MC_CHARTRADE_CANCEL);
	WRITE_LONG(packet, pMain->GetID());

	pTradeData1->pAccept->ReflectINFof(pMain, packet);
	pTradeData1->pRequest->ReflectINFof(pMain, packet);

	// å–æ¶ˆè§’è‰²é”å®šçŠ¶æ€
	pTradeData1->pAccept->TradeAction(FALSE);
	pTradeData1->pRequest->TradeAction(FALSE);

	pTradeData1->Free();

	return TRUE;
	T_E
}

BOOL CTradeSystem::Clear(BYTE byType, CCharacter& character) {
	T_B
		CCharacter* pMain = &character;
	if (!pMain->GetPlyMainCha()) {
		// pMain->SystemNotice( "äº¤æ˜“è§’è‰²ä¸å­˜åœ¨ï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00010));
	}

	if (byType == mission::TRADE_CHAR) {
		pMain = pMain->GetPlyMainCha();
	} else {
		if (pMain == pMain->GetPlyMainCha()) {
			// pMain->SystemNotice( "äº¤æ˜“è§’è‰²ç±»åž‹ä¸åŒ¹é…ï¼" );
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00017));
			return FALSE;
		}
	}

	CTradeData* pTradeData = pMain->GetTradeData();
	if (!pTradeData) {
		// è¯¥è§’è‰²å¹¶ä¸äº¤æ˜“ä¸­!
		return FALSE;
	}

	// SECURITY FIX: Validate pointers before accessing
	if (!pTradeData->pRequest || !pTradeData->pAccept) {
		LG("trade_error", "Clear: Invalid trade participant pointers");
		pTradeData->Free();
		return FALSE;
	}

	if (pTradeData->pRequest == pMain) {
		// å–æ¶ˆè§’è‰²äº¤æ˜“
		WPACKET packet = GETWPACKET();
		WRITE_CMD(packet, CMD_MC_CHARTRADE);
		WRITE_SHORT(packet, CMD_MC_CHARTRADE_CANCEL);
		WRITE_LONG(packet, pMain->GetID());
		pTradeData->pAccept->ReflectINFof(pMain, packet);
		pTradeData->pAccept->SetTradeData(nullptr);

		// æ¸…é™¤é“å…·æ ä½é”å®šçŠ¶æ€
		pTradeData->pAccept->m_CKitbag.UnLock();
		pTradeData->pAccept->TradeAction(FALSE);
		ResetItemState(*pTradeData->pAccept, *pTradeData);
	} else if (pTradeData->pAccept == pMain) {
		// å–æ¶ˆè§’è‰²äº¤æ˜“
		WPACKET packet = GETWPACKET();
		WRITE_CMD(packet, CMD_MC_CHARTRADE);
		WRITE_SHORT(packet, CMD_MC_CHARTRADE_CANCEL);
		WRITE_LONG(packet, pMain->GetID());
		pTradeData->pRequest->ReflectINFof(pMain, packet);
		pTradeData->pRequest->SetTradeData(nullptr);

		// æ¸…é™¤é“å…·æ ä½é”å®šçŠ¶æ€
		pTradeData->pRequest->m_CKitbag.UnLock();
		pTradeData->pRequest->TradeAction(FALSE);
		ResetItemState(*pTradeData->pRequest, *pTradeData);
	} else {
		// LG( "Trade", "åˆ é™¤è§’è‰²æ—¶ï¼Œæ¸…é™¤å…¶äº¤æ˜“ä¿¡æ¯å‘çŽ°é”™è¯¯(ä¸åŒ¹é…çš„è§’è‰²æŒ‡é’ˆ)ï¼"  );
		LG("Trade", "when delete characterï¼Œit find error while clear trade information,the error is:(unsuited charcter pointer)ï¼");
		return FALSE;
	}

	pTradeData->Free();
	return TRUE;
	T_E
}

BOOL CTradeSystem::AddIMP(BYTE byType, CCharacter& character, DWORD dwCharID, BYTE byOpType, DWORD dwMoney) {
	T_B
		CCharacter* pMain = &character;
	if (!pMain->GetPlyMainCha()) {
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00010));
	}

	if (byType == mission::TRADE_CHAR) {
		pMain = pMain->GetPlyMainCha();
	} else {
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00028), byType);
		return FALSE;
	}

	CTradeData* pTradeData = pMain->GetTradeData();
	if (!pTradeData) {
		char szData[128];
		sprintf(szData, RES_STRING(GM_CHARTRADE_CPP_00035), pMain->GetName());
		LG("trade_error", szData);
		return FALSE;
	}

	if (pMain->GetID() == dwCharID) {
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00033));
		return FALSE;
	} else if (pTradeData->pRequest->GetID() != dwCharID && pTradeData->pAccept->GetID() != dwCharID) {
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00036));
		return FALSE;
	}

	TRADE_DATA* pItemData = nullptr;
	if (pMain == pTradeData->pRequest) {
		if (pTradeData->bReqTrade == 1) {
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00037));
			return FALSE;
		}
		pItemData = &pTradeData->ReqTradeData;
	} else if (pMain == pTradeData->pAccept) {
		if (pTradeData->bAcpTrade == 1) {
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00037));
			return FALSE;
		}
		pItemData = &pTradeData->AcpTradeData;
	} else {
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00038));
		return FALSE;
	}

	if (byOpType == TRADE_DRAGMONEY_ITEM) {
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00039));
		return FALSE;
	} else if (byOpType == TRADE_DRAGMONEY_TRADE) {
		DWORD dwCharIMP = pMain->GetIMP();
		pItemData->dwIMP = dwMoney;
		if (pItemData->dwIMP > 2000000) {
			pItemData->dwIMP = 2000000;
		}
		if (pItemData->dwIMP > dwCharIMP) {
			pItemData->dwIMP = dwCharIMP;
		}
	} else {
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00039));
		return FALSE;
	}

	WPACKET packet = GETWPACKET();
	WRITE_CMD(packet, CMD_MC_CHARTRADE);
	WRITE_SHORT(packet, CMD_MC_CHARTRADE_MONEY);
	WRITE_LONG(packet, pMain->GetID());
	WRITE_LONG(packet, pItemData->dwIMP);
	WRITE_CHAR(packet, 1);
	pTradeData->pAccept->ReflectINFof(pMain, packet);
	pTradeData->pRequest->ReflectINFof(pMain, packet);
	return TRUE;
	T_E
}

BOOL CTradeSystem::AddMoney(BYTE byType, CCharacter& character, DWORD dwCharID, BYTE byOpType, __int64 llMoney) {
	T_B
		CCharacter* pMain = &character;
	if (!pMain->GetPlyMainCha()) {
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00010));
	}

	if (byType == mission::TRADE_CHAR) {
		pMain = pMain->GetPlyMainCha();
	} else {
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00028), byType);
		return FALSE;
	}

	CTradeData* pTradeData = pMain->GetTradeData();
	if (!pTradeData) {
		char szData[128];
		sprintf(szData, RES_STRING(GM_CHARTRADE_CPP_00035), pMain->GetName());
		LG("trade_error", szData);
		return FALSE;
	}

	if (pMain->GetID() == dwCharID) {
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00033));
		return FALSE;
	} else if (pTradeData->pRequest->GetID() != dwCharID && pTradeData->pAccept->GetID() != dwCharID) {
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00036));
		return FALSE;
	}

	TRADE_DATA* pItemData = nullptr;
	if (pMain == pTradeData->pRequest) {
		if (pTradeData->bReqTrade == 1) {
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00037));
			return FALSE;
		}
		pItemData = &pTradeData->ReqTradeData;
	} else if (pMain == pTradeData->pAccept) {
		if (pTradeData->bAcpTrade == 1) {
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00037));
			return FALSE;
		}
		pItemData = &pTradeData->AcpTradeData;
	} else {
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00038));
		return FALSE;
	}

	if (byOpType == TRADE_DRAGMONEY_ITEM) {
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00039));
		return FALSE;
	} else if (byOpType == TRADE_DRAGMONEY_TRADE) {
		__int64 llCharMoney = pMain->m_CChaAttr.GetAttr(ATTR_GD);
		__int64 llMaxTrade = 100000000000LL;  // 100 billion cap
		pItemData->llMoney = llMoney;
		if (pItemData->llMoney > llCharMoney) {
			pItemData->llMoney = llCharMoney;
		}
		if (pItemData->llMoney > llMaxTrade) {
			pItemData->llMoney = llMaxTrade;
			pMain->SystemNotice("Trade gold capped at 100 billion.");
		}
	} else {
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00039));
		return FALSE;
	}

	WPACKET packet = GETWPACKET();
	WRITE_CMD(packet, CMD_MC_CHARTRADE);
	WRITE_SHORT(packet, CMD_MC_CHARTRADE_MONEY);
	WRITE_LONG(packet, pMain->GetID());
	WRITE_LONGLONG(packet, pItemData->llMoney);
	WRITE_CHAR(packet, 0);
	pTradeData->pAccept->ReflectINFof(pMain, packet);
	pTradeData->pRequest->ReflectINFof(pMain, packet);
	return TRUE;
	T_E
}

// æ”¾ç½®æˆ–è€…å–èµ°ç‰©å“åˆ°äº¤æ˜“æ 
BOOL CTradeSystem::AddItem(BYTE byType, CCharacter& character, DWORD dwCharID, BYTE byOpType, BYTE byIndex, BYTE byItemIndex, BYTE byCount) {
	T_B
		CCharacter* pMain = &character;
	if (pMain->GetPlayer() == nullptr) {
		return FALSE;
	}

	if (!pMain->GetPlyMainCha()) {
		// pMain->SystemNotice( "äº¤æ˜“è§’è‰²ä¸å­˜åœ¨ï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00010));
	}

	if (byType == mission::TRADE_CHAR) {
		pMain = pMain->GetPlyMainCha();
	} else {
		if (pMain == pMain->GetPlyMainCha()) {
			// pMain->SystemNotice( "äº¤æ˜“è§’è‰²ç±»åž‹ä¸åŒ¹é…ï¼" );
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00017));
			return FALSE;
		}
	}

	CKitbag& Bag = pMain->m_CKitbag;
	SItemGrid* pGridCont = Bag.GetGridContByID(byItemIndex);

	CTradeData* pTradeData = pMain->GetTradeData();
	if (!pTradeData) {
		char szData[128];
		// sprintf( szData, "AddItem:è¯¥è§’è‰²%så¹¶ä¸äº¤æ˜“ä¸­!", pMain->GetName() );
		sprintf(szData, RES_STRING(GM_CHARTRADE_CPP_00040), pMain->GetName());
		LG("trade_error", szData);
		return FALSE;
	}

	if (pMain->GetID() == dwCharID) {
		// pMain->SystemNotice( "æŠ¥æ–‡ä¿¡æ¯é”™è¯¯ï¼Œä¸èƒ½æ·»åŠ ç‰©å“å’Œè‡ªå·±IDç›¸åŒçš„äº¤æ˜“æ“ä½œï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00041));
		return FALSE;
	} else if (pTradeData->pRequest->GetID() != dwCharID && pTradeData->pAccept->GetID() != dwCharID) {
		// pMain->SystemNotice( "äº¤æ˜“å¯¹è±¡ä¿¡æ¯é”™è¯¯ï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00036));
		return FALSE;
	}

	CCharacter* pChar = nullptr;
	TRADE_DATA* pItemData = nullptr;
	// éªŒè¯æ˜¯å¦å¯ä»¥æ·»åŠ ç‰©å“æˆ–å–èµ°ç‰©å“æ“ä½œ
	if (pMain == pTradeData->pRequest) {
		// åˆ¤æ–­æ˜¯å¦å¯ä»¥æ“ä½œç‰©å“
		if (pTradeData->bReqTrade == 1) {

			// pMain->SystemNotice( "è¯¥è§’è‰²äº¤æ˜“ç‰©å“éªŒè¯æŒ‰é’®å·²ç»æŒ‰ä¸‹ï¼Œä¸å¯ä»¥å†ä½œç‰©å“æ‹–åŠ¨æ“ä½œï¼" );
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00042));
			return FALSE;
		}
		pItemData = &pTradeData->ReqTradeData;
		pChar = pTradeData->pAccept;
	} else if (pMain == pTradeData->pAccept) {
		if (pTradeData->bAcpTrade == 1) {
			// pMain->SystemNotice( "è¯¥è§’è‰²äº¤æ˜“ç‰©å“éªŒè¯æŒ‰é’®å·²ç»æŒ‰ä¸‹ï¼Œä¸å¯ä»¥å†ä½œç‰©å“æ‹–åŠ¨æ“ä½œï¼" );
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00042));
			return FALSE;
		}
		pItemData = &pTradeData->AcpTradeData;
		pChar = pTradeData->pRequest;
	} else {
		// pMain->SystemNotice( "è¯¥è§’è‰²æœªåœ¨äº¤æ˜“ï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00038));
		return FALSE;
	}

	// è®¾ç½®äº¤æ˜“æ ç‰©å“ï¼Œå¹¶ä¸”å‘é€é€šçŸ¥ä¿¡æ¯åˆ°å®¢æˆ·ç«¯
	if (byOpType == TRADE_DRAGTO_ITEM) {
		if (byIndex >= ROLE_MAXNUM_TRADEDATA) {
			// pMain->SystemNotice( "æœªçŸ¥çš„äº¤æ˜“æ ä½ç´¢å¼•ä¿¡æ¯ï¼" );
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00043));
			return FALSE;
		}
		int nCapacity = pMain->m_CKitbag.GetCapacity();
		if (byItemIndex >= nCapacity) {
			// pMain->SystemNotice( "æœªçŸ¥çš„é“å…·æ ä½ç´¢å¼•ä¿¡æ¯ï¼" );
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00044));
			return FALSE;
		}
		if (pItemData->ItemArray[byIndex].sItemID == 0) {
			// pMain->SystemNotice( "è¯¥è§’è‰²æ‹–åŠ¨çš„äº¤æ˜“æ ä½ç‰©å“ä¸å­˜åœ¨ï¼" );
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00045));
			return FALSE;
		}
		if (Bag.GetNum(pItemData->ItemArray[byIndex].byIndex) > 0 &&
			Bag.GetID(pItemData->ItemArray[byIndex].byIndex) != pItemData->ItemArray[byIndex].sItemID) {
			// pMain->SystemNotice( "ç³»ç»Ÿç‰©å“æ äº¤æ˜“è®°å½•é”™è¯¯ï¼Œè¯·é€šçŸ¥å¼€å‘äººå‘˜ï¼Œè°¢è°¢ï¼ID1= %d, ID2 = %d",
			//	Bag.GetID( pItemData->ItemArray[byIndex].byIndex ), pItemData->ItemArray[byIndex].sItemID );
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00046),
								Bag.GetID(pItemData->ItemArray[byIndex].byIndex), pItemData->ItemArray[byIndex].sItemID);
			return FALSE;
		}

		WPACKET packet = GETWPACKET();
		WRITE_CMD(packet, CMD_MC_CHARTRADE);
		WRITE_SHORT(packet, CMD_MC_CHARTRADE_ITEM);
		WRITE_LONG(packet, pMain->GetID());
		WRITE_CHAR(packet, TRADE_DRAGTO_ITEM);
		WRITE_CHAR(packet, pItemData->ItemArray[byIndex].byIndex);
		WRITE_CHAR(packet, byIndex);
		WRITE_CHAR(packet, byCount);

		// å¼€å¯é“å…·æ ç‰©ç‰©å“æ´»åŠ¨çŠ¶æ€
		Bag.Enable(pItemData->ItemArray[byIndex].byIndex);
		pItemData->ItemArray[byIndex].sItemID = 0;
		pItemData->ItemArray[byIndex].byCount = 0;
		pItemData->ItemArray[byIndex].byType = 0;
		pItemData->ItemArray[byIndex].byIndex = 0;
		pItemData->byItemCount--;

		pTradeData->pRequest->ReflectINFof(pMain, packet);
		pTradeData->pAccept->ReflectINFof(pMain, packet);
	} else if (byOpType == TRADE_DRAGTO_TRADE) {
		if (byIndex >= ROLE_MAXNUM_TRADEDATA) {
			// pMain->SystemNotice( "æœªçŸ¥çš„äº¤æ˜“æ ä½ç´¢å¼•ä¿¡æ¯ï¼" );
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00043));
			return FALSE;
		}
		int nCapacity = pMain->m_CKitbag.GetCapacity();
		if (byItemIndex >= nCapacity) {
			// pMain->SystemNotice( "æœªçŸ¥çš„é“å…·æ ä½ç´¢å¼•ä¿¡æ¯ï¼" );
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00044));
			return FALSE;
		}

		if (!Bag.HasItem(byItemIndex) || !Bag.IsEnable(byItemIndex)) {
			// pMain->SystemNotice( "è¯¥ç‰©å“æ ä½å·²è¢«ç¦æ­¢æ‹–åŠ¨ï¼" );
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00047));
			return FALSE;
		}
		if (pItemData->ItemArray[byIndex].sItemID != 0) {
			// pMain->SystemNotice( "è¯¥äº¤æ˜“ç‰©å“æ ä½å·²å­˜åœ¨ç‰©å“ï¼Œè¯·å¦é€‰ä½ç½®æ‘†æ”¾ï¼" );
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00048));
			return FALSE;
		}

		CItemRecord* pItem = (CItemRecord*)GetItemRecordInfo(Bag.GetID(byItemIndex));
		if (pItem == nullptr) {
			// pMain->SystemNotice( "ç‰©å“IDé”™è¯¯ï¼Œæ— æ³•æ‰¾åˆ°è¯¥ç‰©å“ä¿¡æ¯ï¼ID = %d", Bag.GetID( byItemIndex ) );
			pMain->SystemNotice(RES_STRING(GM_CHARSTALL_CPP_00041), Bag.GetID(byItemIndex));
			return FALSE;
		}

		if (!pItem->chIsTrade || !Bag.GetGridContByID(byItemIndex)->GetInstAttr(ITEMATTR_TRADABLE)) {
			// pMain->SystemNotice( "ç‰©å“ã€Š%sã€‹ä¸å¯äº¤æ˜“ï¼", pItem->szName );
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00049), pItem->szName);
			return FALSE;
		}

		if (pGridCont->dwDBID) {
			pMain->SystemNotice("Item is bind, cannot be traded!");
			return FALSE;
		};

		// if( pItem->sType == enumItemTypeMission )
		//{
		//	pMain->SystemNotice( "ä»»åŠ¡é“å…·ã€Š%sã€‹ä¸å¯ä»¥äº¤æ˜“ï¼", pItem->szName );
		//	return FALSE;
		// }
		// else
		if (pItem->sType == enumItemTypeBoat) {
			if (pMain->GetPlayer()->IsLuanchOut()) {
				if (Bag.GetDBParam(enumITEMDBP_INST_ID, byItemIndex) == pMain->GetPlayer()->GetLuanchID()) {
					// pMain->SystemNotice( "ä½ æ­£åœ¨ä½¿ç”¨è¯¥èˆ¹åªï¼Œä¸å¯ä»¥äº¤æ˜“è¯¥èˆ¹åªèˆ¹é•¿è¯æ˜Žï¼" );
					pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00050));
					return FALSE;
				}
			}

			if (!pChar->GetPlayer()->IsBoatFull()) {
				USHORT sID = Bag.GetID(byItemIndex);
				USHORT sNum = pChar->GetPlayer()->GetNumBoat();

				for (int i = 0; i < ROLE_MAXNUM_TRADEDATA; i++) {
					if (sID == pItemData->ItemArray[i].sItemID) {
						sNum++;
						if (sNum >= MAX_CHAR_BOAT) {
							// pMain->SystemNotice( "å¯¹æ–¹å·²ç»æ‹¥æœ‰äº†è¶³å¤Ÿæ•°é‡çš„èˆ¹åªï¼Œä¸å¯ä»¥å†æ‹¥æœ‰æ–°èˆ¹åªï¼" );
							pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00051));
							return FALSE;
						}
					}
				}
			} else {
				// pMain->SystemNotice( "å¯¹æ–¹å·²ç»æ‹¥æœ‰äº†è¶³å¤Ÿæ•°é‡çš„èˆ¹åªï¼Œä¸å¯ä»¥å†æ‹¥æœ‰æ–°èˆ¹åªï¼" );
				pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00051));
				return FALSE;
			}

			CCharacter* pBoat = pMain->GetPlayer()->GetBoat((DWORD)Bag.GetDBParam(enumITEMDBP_INST_ID, byItemIndex));
			if (!pBoat) {
				/*pMain->SystemNotice( "è¯¥èˆ¹æ•°æ®é”™è¯¯ï¼Œä¸å¯äº¤æ˜“ï¼ID[0x%X]",
					Bag.GetDBParam( enumITEMDBP_INST_ID, byItemIndex ) );
				LG( "trade_error", "è¯¥èˆ¹æ•°æ®é”™è¯¯ï¼Œä¸å¯äº¤æ˜“ï¼ID[0x%X]",
					Bag.GetDBParam( enumITEMDBP_INST_ID, byItemIndex ) );*/
				pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00052),
									Bag.GetDBParam(enumITEMDBP_INST_ID, byItemIndex));
				LG("trade_error", "The data error of this boatï¼Œcannot tradeï¼ID[0x%X]",
				   Bag.GetDBParam(enumITEMDBP_INST_ID, byItemIndex));
				return FALSE;
			}
			if (!game_db.SaveBoat(*pBoat, enumSAVE_TYPE_OFFLINE)) {
				/*pMain->SystemNotice( "AddItem:ä¿å­˜èˆ¹åªæ•°æ®å¤±è´¥ï¼èˆ¹åªã€Š%sã€‹ID[0x%X]ã€‚", pBoat->GetName(),
					Bag.GetDBParam( enumITEMDBP_INST_ID, byItemIndex ) );
				LG( "trade_error", "AddItem:ä¿å­˜èˆ¹åªæ•°æ®å¤±è´¥ï¼èˆ¹åªã€Š%sã€‹ID[0x%X]ã€‚", pBoat->GetName(),
					Bag.GetDBParam( enumITEMDBP_INST_ID, byItemIndex ) );*/
				pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00053), pBoat->GetName(),
									Bag.GetDBParam(enumITEMDBP_INST_ID, byItemIndex));
				LG("trade_error", "AddItem:it failed to save boat dataï¼boatã€Š%sã€‹ID[0x%X]ã€‚", pBoat->GetName(),
				   Bag.GetDBParam(enumITEMDBP_INST_ID, byItemIndex));
				return FALSE;
			}
		}

		if (byCount == 0) {
			byCount = 1;
		}

		if (byCount > ROLE_MAXNUM_ITEMTRADE) {
			byCount = ROLE_MAXNUM_ITEMTRADE;
		}

		if (byCount > Bag.GetNum(byItemIndex)) {
			byCount = (BYTE)Bag.GetNum(byItemIndex);
		}

		pItemData->ItemArray[byIndex].sItemID = Bag.GetID(byItemIndex);
		pItemData->ItemArray[byIndex].byCount = byCount;
		pItemData->ItemArray[byIndex].byIndex = byItemIndex;
		pItemData->byItemCount++;

		// ç¦æ­¢ç‰©å“æ ä½æ´»åŠ¨çŠ¶æ€
		Bag.Disable(byItemIndex);

		WPACKET packet = GETWPACKET();
		WRITE_CMD(packet, CMD_MC_CHARTRADE);
		WRITE_SHORT(packet, CMD_MC_CHARTRADE_ITEM);
		WRITE_LONG(packet, pMain->GetID());
		WRITE_CHAR(packet, TRADE_DRAGTO_TRADE);
		WRITE_SHORT(packet, pItemData->ItemArray[byIndex].sItemID);
		WRITE_CHAR(packet, pItemData->ItemArray[byIndex].byIndex);
		WRITE_CHAR(packet, byIndex);
		WRITE_CHAR(packet, byCount);
		WRITE_SHORT(packet, pItem->sType);

		if (pItem->sType == enumItemTypeBoat) {
			CCharacter* pBoat = pMain->GetPlayer()->GetBoat((DWORD)Bag.GetDBParam(enumITEMDBP_INST_ID, byItemIndex));
			if (pBoat) {
				WRITE_CHAR(packet, 1);
				WRITE_STRING(packet, pBoat->GetName());
				WRITE_SHORT(packet, (USHORT)pBoat->getAttr(ATTR_BOAT_SHIP));
				WRITE_SHORT(packet, (USHORT)pBoat->getAttr(ATTR_LV));

				WRITE_LONG(packet, (int)pBoat->getAttr(ATTR_CEXP));
				WRITE_LONG(packet, (int)pBoat->getAttr(ATTR_HP));

				WRITE_LONG(packet, (int)pBoat->getAttr(ATTR_BMXHP));
				WRITE_LONG(packet, (int)pBoat->getAttr(ATTR_SP));
				WRITE_LONG(packet, (int)pBoat->getAttr(ATTR_BMXSP));
				WRITE_LONG(packet, (int)pBoat->getAttr(ATTR_BMNATK));
				WRITE_LONG(packet, (int)pBoat->getAttr(ATTR_BMXATK));
				WRITE_LONG(packet, (int)pBoat->getAttr(ATTR_BDEF));
				WRITE_LONG(packet, (int)pBoat->getAttr(ATTR_BMSPD));
				WRITE_LONG(packet, (int)pBoat->getAttr(ATTR_BASPD));
				WRITE_CHAR(packet, (BYTE)pBoat->m_CKitbag.GetUseGridNum());
				WRITE_CHAR(packet, (BYTE)pBoat->m_CKitbag.GetCapacity());
				WRITE_LONG(packet, (int)pBoat->getAttr(ATTR_BOAT_PRICE));
			} else {
				WRITE_CHAR(packet, 0);
			}
		} else {
			// è¯¥é“å…·çš„å®žä¾‹å±žæ€§
			SItemGrid* pGridCont = Bag.GetGridContByID(byItemIndex);
			if (!pGridCont) {
				// pMain->SystemNotice( "æŒ‡å®šçš„ç‰©å“æ ä½ç‰©å“å®žä¾‹ä¿¡æ¯ä¸ºç©ºï¼ID[%d]", byItemIndex );
				pMain->SystemNotice(RES_STRING(GM_CHARSTALL_CPP_00057), byItemIndex);
				return FALSE;
			}

			WRITE_SHORT(packet, pGridCont->sEndure[0]);
			WRITE_SHORT(packet, pGridCont->sEndure[1]);
			WRITE_SHORT(packet, pGridCont->sEnergy[0]);
			WRITE_SHORT(packet, pGridCont->sEnergy[1]);
			WRITE_CHAR(packet, pGridCont->chForgeLv);
			WRITE_CHAR(packet, pGridCont->IsValid() ? 1 : 0);
			WRITE_CHAR(packet, pGridCont->bItemTradable);
			WRITE_LONG(packet, pGridCont->expiration);

			WRITE_LONG(packet, pGridCont->GetDBParam(enumITEMDBP_FORGE));
			WRITE_LONG(packet, pGridCont->GetDBParam(enumITEMDBP_INST_ID));
			if (pGridCont->IsInstAttrValid()) // å­˜åœ¨å®žä¾‹å±žæ€§
			{
				WRITE_CHAR(packet, 1);
				for (int j = 0; j < defITEM_INSTANCE_ATTR_NUM; j++) {
					WRITE_SHORT(packet, pGridCont->sInstAttr[j][0]);
					WRITE_SHORT(packet, pGridCont->sInstAttr[j][1]);
				}
			} else {
				WRITE_CHAR(packet, 0); // ä¸å­˜åœ¨å®žä¾‹å±žæ€§
			}
		}

		pTradeData->pRequest->ReflectINFof(pMain, packet);
		pTradeData->pAccept->ReflectINFof(pMain, packet);
	} else {
		// pMain->SystemNotice( "æœªçŸ¥çš„ç‰©å“æ‹–åŠ¨ç±»åž‹æŒ‡ä»¤ï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00054));
		return FALSE;
	}

	return TRUE;
	T_E
}

BOOL CTradeSystem::ValidateItemData(BYTE byType, CCharacter& character, DWORD dwCharID) {
	T_B
		CCharacter* pMain = &character;
	if (!pMain->GetPlyMainCha()) {
		// pMain->SystemNotice( "äº¤æ˜“è§’è‰²ä¸å­˜åœ¨ï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00010));
	}

	if (byType == mission::TRADE_CHAR) {
		pMain = pMain->GetPlyMainCha();
	} else {
		if (pMain == pMain->GetPlyMainCha()) {
			// pMain->SystemNotice( "äº¤æ˜“è§’è‰²ç±»åž‹ä¸åŒ¹é…ï¼" );
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00017));
			return FALSE;
		}
	}

	CTradeData* pTradeData = pMain->GetTradeData();
	if (!pTradeData) {
		char szData[128];
		// sprintf( szData, "ValidateItemData:è¯¥è§’è‰²%så¹¶ä¸äº¤æ˜“ä¸­!", pMain->GetName() );
		sprintf(szData, RES_STRING(GM_CHARTRADE_CPP_00055), pMain->GetName());
		LG("trade_error", szData);
		return FALSE;
	}

	if (!pTradeData->pRequest->IsLiveing() || !pTradeData->pAccept->IsLiveing()) {
		pTradeData->pAccept->SystemNotice("Dead pirates are unable to trade.");
		pTradeData->pRequest->SystemNotice("Dead pirates are unable to trade.");
		return FALSE;
	}

	if (pMain->GetID() == dwCharID) {
		// pMain->SystemNotice( "æŠ¥æ–‡ä¿¡æ¯é”™è¯¯ï¼Œä¸èƒ½å–æ¶ˆå’Œè‡ªå·±IDç›¸åŒçš„äº¤æ˜“æ“ä½œï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00033));
		return FALSE;
	} else if (pTradeData->pRequest->GetID() != dwCharID && pTradeData->pAccept->GetID() != dwCharID) {
		// pMain->SystemNotice( "äº¤æ˜“å¯¹è±¡ä¿¡æ¯é”™è¯¯ï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00036));
		return FALSE;
	}

	// è®¾ç½®ç¡®å®šç‰©å“ä¿¡æ¯çŠ¶æ€
	if (pMain == pTradeData->pRequest) {
		pTradeData->bReqTrade = 1;
	} else if (pMain == pTradeData->pAccept) {
		pTradeData->bAcpTrade = 1;
	} else {
		/*pMain->SystemNotice( "äº¤æ˜“å¯¹è±¡ä¿¡æ¯å†…éƒ¨é”™è¯¯ï¼" );
		LG( "trade_error", "äº¤æ˜“å¯¹è±¡ä¿¡æ¯å†…éƒ¨é”™è¯¯ï¼" );*/
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00056));
		LG("trade_error", "information of trade object  inside error");
		return FALSE;
	}

	WPACKET packet = GETWPACKET();
	WRITE_CMD(packet, CMD_MC_CHARTRADE);
	WRITE_SHORT(packet, CMD_MC_CHARTRADE_VALIDATEDATA);
	WRITE_LONG(packet, pMain->GetID());

	if (pMain == pTradeData->pRequest) {
		pTradeData->pAccept->ReflectINFof(pMain, packet);
	} else {
		pTradeData->pRequest->ReflectINFof(pMain, packet);
	}
	return TRUE;
	T_E
}

BOOL CTradeSystem::ValidateTrade(BYTE byType, CCharacter& character, DWORD dwCharID) {
	T_B
		CCharacter* pMain = &character;
	if (!pMain->GetPlyMainCha()) {
		// pMain->SystemNotice( "äº¤æ˜“è§’è‰²ä¸å­˜åœ¨ï¼" );
		pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00010));
	}

	if (byType == mission::TRADE_CHAR) {
		pMain = pMain->GetPlyMainCha();
	} else {
		if (pMain == pMain->GetPlyMainCha()) {
			// pMain->SystemNotice( "äº¤æ˜“è§’è‰²ç±»åž‹ä¸åŒ¹é…ï¼" );
			pMain->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00017));
			return FALSE;
		}
	}

	CTradeData* pTradeData = pMain->GetTradeData();
	if (!pTradeData) {
		char szData[128];
		// sprintf( szData, "ValidateTrade:è¯¥è§’è‰²%så¹¶ä¸äº¤æ˜“ä¸­!", pMain->GetName() );
		sprintf(szData, RES_STRING(GM_CHARTRADE_CPP_00057), pMain->GetName());
		LG("trade_error", szData);
		return FALSE;
	}

	// SECURITY FIX: Validate both trade participants are still connected
	if (!pTradeData->ValidateParticipants()) {
		LG("trade_error", "ValidateTrade: One or both trade participants are no longer valid");
		pTradeData->Clear();
		return FALSE;
	}

	if (!pTradeData->pRequest->IsLiveing() || !pTradeData->pAccept->IsLiveing()) {
		pTradeData->pAccept->SystemNotice("Dead pirates are unable to trade.");
		pTradeData->pRequest->SystemNotice("Dead pirates are unable to trade.");
		return FALSE;
	}

	if (pMain->GetID() == dwCharID) {
		// printf( "æŠ¥æ–‡ä¿¡æ¯é”™è¯¯ï¼Œä¸èƒ½å–æ¶ˆå’Œè‡ªå·±IDç›¸åŒçš„äº¤æ˜“æ“ä½œï¼" );
		printf(RES_STRING(GM_CHARTRADE_CPP_00033));
		return FALSE;
	} else if (pTradeData->pRequest->GetID() != dwCharID && pTradeData->pAccept->GetID() != dwCharID) {
		// printf( "äº¤æ˜“å¯¹è±¡ä¿¡æ¯é”™è¯¯ï¼" );
		printf(RES_STRING(GM_CHARTRADE_CPP_00036));
		return FALSE;
	}

	// è®¾ç½®äº¤æ˜“çŠ¶æ€ï¼Œå¹¶æ£€æµ‹æ˜¯å¦åŒæ–¹éƒ½è¯·æ±‚äº¤æ˜“
	if (pMain == pTradeData->pRequest) {
		if (pTradeData->bReqTrade != 1 || pTradeData->bAcpTrade != 1) {
			return FALSE;
		}
		pTradeData->bReqOk = 1;
	} else if (pMain == pTradeData->pAccept) {
		if (pTradeData->bReqTrade != 1 || pTradeData->bAcpTrade != 1) {
			return FALSE;
		}
		pTradeData->bAcpOk = 1;
	}

	if (pTradeData->bAcpTrade == 1 && pTradeData->bReqTrade == 1 &&
		pTradeData->bAcpOk == 1 && pTradeData->bReqOk == 1) {

		// SECURITY FIX: Acquire execution lock to prevent race conditions
		// This ensures only one thread can execute the trade finalization
		if (!pTradeData->TryLockExecution()) {
			// Trade is already being executed by another thread/packet
			LG("trade_security", "Trade execution already in progress for %s <-> %s, rejecting duplicate request",
			   pTradeData->pRequest->GetName(), pTradeData->pAccept->GetName());
			return FALSE;
		}

		CCharacter* pRequest = pTradeData->pRequest;
		CCharacter* pAccept = pTradeData->pAccept;
		CKitbag& ReqBag = pRequest->m_CKitbag;
		CKitbag& AcpBag = pAccept->m_CKitbag;
		long long llReqMoney = pRequest->getAttr(ATTR_GD);
		long long llAcpMoney = pAccept->getAttr(ATTR_GD);

		int dwReqIMP = pRequest->GetIMP();
		int dwAcpIMP = pAccept->GetIMP();

		if (pTradeData->ReqTradeData.dwIMP > dwReqIMP) {
			pAccept->SystemNotice("Character (%s] IMP in trading mode is incorrect, trading cannot be continued!", pRequest->GetName());
			pRequest->SystemNotice("Character (%s] IMP in trading mode is incorrect, trading cannot be continued!", pRequest->GetName());
			pTradeData->UnlockExecution();
			return FALSE;
		}

		if (pTradeData->AcpTradeData.dwIMP > dwAcpIMP) {
			pAccept->SystemNotice("Character (%s] IMP in trading mode is incorrect, trading cannot be continued!", pAccept->GetName());
			pRequest->SystemNotice("Character (%s] IMP in trading mode is incorrect, trading cannot be continued!", pAccept->GetName());
			pTradeData->UnlockExecution();
			return FALSE;
		}

		if (dwAcpIMP + pTradeData->ReqTradeData.dwIMP > 2000000) {
			pAccept->SystemNotice("Character (%s] IMP would exceed 2b, trading cannot be continued!", pAccept->GetName());
			pRequest->SystemNotice("Character (%s] IMP would exceed 2b, trading cannot be continued!", pAccept->GetName());
			pTradeData->UnlockExecution();
			return FALSE;
		}

		if (dwReqIMP + pTradeData->AcpTradeData.dwIMP > 2000000) {
			pAccept->SystemNotice("Character (%s] IMP would exceed 2b, trading cannot be continued!", pRequest->GetName());
			pRequest->SystemNotice("Character (%s] IMP would exceed 2b, trading cannot be continued!", pRequest->GetName());
			pTradeData->UnlockExecution();
			return FALSE;
		}

		// å†æ¬¡æ ¡éªŒäº¤æ˜“é‡‘é’±æ•°æ®ä¿¡æ¯
		if (pTradeData->ReqTradeData.llMoney > llReqMoney) {
			/*pAccept->SystemNotice( "è§’è‰²ã€Š%sã€‹äº¤æ˜“é‡‘é’±æ•°æ®ä¸æ­£ç¡®ï¼Œä¸å¯ä»¥ç»§ç»­äº¤æ˜“ï¼", pRequest->GetName() );
			pRequest->SystemNotice( "è§’è‰²ã€Š%sã€‹äº¤æ˜“é‡‘é’±æ•°æ®ä¸æ­£ç¡®ï¼Œä¸å¯ä»¥ç»§ç»­äº¤æ˜“ï¼", pRequest->GetName() );*/
			pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00058), pRequest->GetName());
			pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00058), pRequest->GetName());
			pTradeData->UnlockExecution();
			return FALSE;
		}

		if (pTradeData->AcpTradeData.llMoney > llAcpMoney) {
			/*pAccept->SystemNotice( "è§’è‰²ã€Š%sã€‹äº¤æ˜“é‡‘é’±æ•°æ®ä¸æ­£ç¡®ï¼Œä¸å¯ä»¥ç»§ç»­äº¤æ˜“ï¼", pAccept->GetName() );
			pRequest->SystemNotice( "è§’è‰²ã€Š%sã€‹äº¤æ˜“é‡‘é’±æ•°æ®ä¸æ­£ç¡®ï¼Œä¸å¯ä»¥ç»§ç»­äº¤æ˜“ï¼", pAccept->GetName() );*/
			pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00058), pRequest->GetName());
			pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00058), pRequest->GetName());
			pTradeData->UnlockExecution();
			return FALSE;
		}

		// Unlock bags so KbPopItem can extract items
		// The execution lock (TryLockExecution) already protects against race conditions
		ReqBag.UnLock();
		AcpBag.UnLock();
		ResetItemState(*pAccept, *pTradeData);
		ResetItemState(*pRequest, *pTradeData);

		// å¤‡ä»½äº¤æ˜“åŒæ–¹èƒŒåŒ…å’Œé‡‘é’±æ•°æ®ä¿¡æ¯
		CKitbag ReqBagData, AcpBagData;
		ReqBagData = ReqBag;
		AcpBagData = AcpBag;

		//
		ReqBag.SetChangeFlag(false);
		AcpBag.SetChangeFlag(false);
		pRequest->m_CChaAttr.ResetChangeFlag();
		pRequest->SetBoatAttrChangeFlag(false);
		pAccept->m_CChaAttr.ResetChangeFlag();
		pAccept->SetBoatAttrChangeFlag(false);

		// å®Œæˆäº¤æ˜“ä¿¡æ¯æ“ä½œ
		int nAcpCapacity = pAccept->m_CKitbag.GetCapacity();
		int nReqCapacity = pRequest->m_CKitbag.GetCapacity();
		SItemGrid AcpGrid[ROLE_MAXNUM_TRADEDATA];
		SItemGrid ReqGrid[ROLE_MAXNUM_TRADEDATA];

		// æ£€éªŒé“å…·äº¤æ˜“æ˜¯å¦å¯ä»¥è¿›è¡Œ
		char szTemp[128] = "";
		char szTrade[2046] = "";
		// sprintf( szTrade, "æŽ¥å—è€…%säº¤æ˜“æ•°æ®ï¼š{", pAccept->GetName() );
		sprintf(szTrade, RES_STRING(GM_CHARTRADE_CPP_00059), pAccept->GetName());

		// åˆ¤æ–­åŒæ–¹èƒŒåŒ…
		BOOL bBagSucc = true;
		if (!pTradeData->pAccept->HasLeaveBagGrid(pTradeData->ReqTradeData.byItemCount)) {
			/*pTradeData->pRequest->SystemNotice("å¯¹æ–¹èƒŒåŒ…ç©ºé—´ä¸å¤Ÿ,äº¤æ˜“å¤±è´¥!");
			pTradeData->pAccept->SystemNotice("èƒŒåŒ…ç©ºé—´ä¸å¤Ÿ,äº¤æ˜“å¤±è´¥!");*/
			pTradeData->pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00060));
			pTradeData->pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00061));
			bBagSucc = false;
		} else if (!pTradeData->pRequest->HasLeaveBagGrid(pTradeData->AcpTradeData.byItemCount)) {
			/*pTradeData->pAccept->SystemNotice("å¯¹æ–¹èƒŒåŒ…ç©ºé—´ä¸å¤Ÿ,äº¤æ˜“å¤±è´¥!");
			pTradeData->pRequest->SystemNotice("èƒŒåŒ…ç©ºé—´ä¸å¤Ÿ,äº¤æ˜“å¤±è´¥!");*/
			pTradeData->pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00060));
			pTradeData->pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00061));
			bBagSucc = false;
		}
		if (!bBagSucc) {
			pAccept->SetTradeData(nullptr);
			pRequest->SetTradeData(nullptr);

			// å–æ¶ˆè§’è‰²é”å®šçŠ¶æ€
			pAccept->TradeAction(FALSE);
			pRequest->TradeAction(FALSE);

			// Send failure packet BEFORE freeing trade data
			WPACKET packet = GETWPACKET();
			WRITE_CMD(packet, CMD_MC_CHARTRADE);
			WRITE_SHORT(packet, CMD_MC_CHARTRADE_RESULT);
			WRITE_CHAR(packet, TRADE_FAILER);

			pAccept->ReflectINFof(pMain, packet);
			pRequest->ReflectINFof(pMain, packet);

			// SECURITY FIX: Unlock execution before freeing
			pTradeData->UnlockExecution();
			pTradeData->Free();

			return FALSE;
		}

		// é“å…·äº¤æ˜“æ“ä½œ
		for (int i = 0; i < ROLE_MAXNUM_TRADEDATA; i++) {
			//
			if (pTradeData->AcpTradeData.ItemArray[i].sItemID != 0) {
				CItemRecord* pItem = GetItemRecordInfo(pTradeData->AcpTradeData.ItemArray[i].sItemID);
				if (pItem == nullptr) {
					/*pMain->SystemNotice( "ç‰©å“IDé”™è¯¯ï¼Œæ— æ³•æ‰¾åˆ°è¯¥ç‰©å“ä¿¡æ¯ï¼ID = %d", pTradeData->AcpTradeData.ItemArray[i].sItemID );
					LG( "trade_error", "ç‰©å“IDé”™è¯¯ï¼Œæ— æ³•æ‰¾åˆ°è¯¥ç‰©å“ä¿¡æ¯ï¼ID = %d", pTradeData->AcpTradeData.ItemArray[i].sItemID );*/
					pMain->SystemNotice(RES_STRING(GM_CHARSTALL_CPP_00041), pTradeData->AcpTradeData.ItemArray[i].sItemID);
					LG("trade_error", "res ID errorï¼Œit cannot find this res informationï¼ID = %d", pTradeData->AcpTradeData.ItemArray[i].sItemID);
					pTradeData->UnlockExecution();
					return FALSE;
				} else {
					AcpGrid[i].sNum = pTradeData->AcpTradeData.ItemArray[i].byCount;
					Short sPopResult = pAccept->KbPopItem(true, false, AcpGrid + i, pTradeData->AcpTradeData.ItemArray[i].byIndex);
					if (sPopResult != enumKBACT_SUCCESS) {
						/*pAccept->SystemNotice( "ä»Žäº¤æ˜“æŽ¥å—è€…ã€Š%sã€‹äº¤æ˜“æå–ç‰©å“ã€Š%dã€‹ç‰©å“å¤±è´¥ï¼ID = %d",
							pAccept->GetName(), pTradeData->AcpTradeData.ItemArray[i].sItemID );
						pRequest->SystemNotice( "ä»Žäº¤æ˜“æŽ¥å—è€…ã€Š%sã€‹äº¤æ˜“æå–ç‰©å“ã€Š%dã€‹ç‰©å“å¤±è´¥ï¼ID = %d",
							pAccept->GetName(), pTradeData->AcpTradeData.ItemArray[i].sItemID );
						LG( "trade_error", "ä»Žäº¤æ˜“è¯·æ±‚è€…ã€Š%sã€‹äº¤æ˜“æå–ç‰©å“ã€Š%dã€‹ç‰©å“å¤±è´¥ï¼ID = %d",
							pAccept->GetName(), pTradeData->AcpTradeData.ItemArray[i].sItemID );*/
						// FIX: Pass error code as third argument to match format string
						pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00062),
											  pAccept->GetName(), pTradeData->AcpTradeData.ItemArray[i].sItemID, sPopResult);
						pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00062),
											   pAccept->GetName(), pTradeData->AcpTradeData.ItemArray[i].sItemID, sPopResult);
						LG("trade_error", "Failed to extract item [%d] from trade receiver [%s]! Error = %d, Index = %d",
						   pTradeData->AcpTradeData.ItemArray[i].sItemID, pAccept->GetName(), sPopResult, 
						   pTradeData->AcpTradeData.ItemArray[i].byIndex);
						pTradeData->UnlockExecution();
						return FALSE;
					}

					if (pItem->sType == enumItemTypeBoat) {
						CCharacter* pBoat = pAccept->GetPlayer()->GetBoat((DWORD)AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID));
						if (pBoat) {
							// sprintf( szTemp, "%dè‰˜èˆ¹åªã€Š%sã€‹ID[0x%X]ï¼Œ", AcpGrid[i].sNum, pBoat->GetName(),
							//	AcpGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );
							sprintf(szTemp, RES_STRING(GM_CHARTRADE_CPP_00063), AcpGrid[i].sNum, pBoat->GetName(),
									AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID));
							strcat(szTrade, szTemp);
						} else {
							// sprintf( szTemp, "%dè‰˜èˆ¹åªï¼šæœªçŸ¥èˆ¹åªæ•°æ®ID[0x%X]ï¼Œ", AcpGrid[i].sNum,
							sprintf(szTemp, RES_STRING(GM_CHARTRADE_CPP_00064), AcpGrid[i].sNum,
									AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID));
							strcat(szTrade, szTemp);
						}

						if (!pAccept->BoatClear(AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID))) {
							/*pAccept->SystemNotice( "åˆ é™¤%sæ‹¥æœ‰çš„èˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼ID[0x%X]",
								pAccept->GetName(), AcpGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );
							pRequest->SystemNotice( "åˆ é™¤%sæ‹¥æœ‰çš„èˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼ID[0x%X]",
								pAccept->GetName(), AcpGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );
							LG( "trade_error", "åˆ é™¤%sçš„èˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼DBID[0x%X]",
								pAccept->GetName(), AcpGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );*/
							pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00065),
												  pAccept->GetName(), AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID));
							pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00065),
												   pAccept->GetName(), AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID));
							LG("trade_error", "it failed to delete captain confirm boat that %s have ï¼DBID[0x%X]",
							   pAccept->GetName(), AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID));
						}
					} else {
						sprintf(szTemp, RES_STRING(GM_CHARTRADE_CPP_00096), AcpGrid[i].sNum, pItem->szName);
						strcat(szTrade, szTemp);
					}
				}
			}
		}

		// sprintf( szTemp, "}ï¼Œè¯·æ±‚è€…%säº¤æ˜“æ•°æ®ï¼š{", pRequest->GetName() );
		sprintf(szTemp, RES_STRING(GM_CHARTRADE_CPP_00066), pRequest->GetName());
		strcat(szTrade, szTemp);
		for (int i = 0; i < ROLE_MAXNUM_TRADEDATA; i++) {
			if (pTradeData->ReqTradeData.ItemArray[i].sItemID != 0) {
				CItemRecord* pItem = GetItemRecordInfo(pTradeData->ReqTradeData.ItemArray[i].sItemID);
				if (pItem == nullptr) {
					/*pMain->SystemNotice( "ç‰©å“IDé”™è¯¯ï¼Œæ— æ³•æ‰¾åˆ°è¯¥ç‰©å“ä¿¡æ¯ï¼ID = %d", pTradeData->ReqTradeData.ItemArray[i].sItemID );
					LG( "trade_error", "ç‰©å“IDé”™è¯¯ï¼Œæ— æ³•æ‰¾åˆ°è¯¥ç‰©å“ä¿¡æ¯ï¼ID = %d", pTradeData->ReqTradeData.ItemArray[i].sItemID );*/
					pMain->SystemNotice(RES_STRING(GM_CHARSTALL_CPP_00041), pTradeData->ReqTradeData.ItemArray[i].sItemID);
					LG("trade_error", "res ID errorï¼Œit cannot find this res informationï¼ID = %d", pTradeData->ReqTradeData.ItemArray[i].sItemID);
					pTradeData->UnlockExecution();
					return FALSE;
				} else {
					ReqGrid[i].sNum = pTradeData->ReqTradeData.ItemArray[i].byCount;
					Short sPopResult = pRequest->KbPopItem(true, false, ReqGrid + i, pTradeData->ReqTradeData.ItemArray[i].byIndex);
					if (sPopResult != enumKBACT_SUCCESS) {
						/*pAccept->SystemNotice( "ä»Žäº¤æ˜“è¯·æ±‚è€…ã€Š%sã€‹äº¤æ˜“æå–ç‰©å“ã€Š%dã€‹ç‰©å“å¤±è´¥ï¼ID = %d",
							pRequest->GetName(), pTradeData->ReqTradeData.ItemArray[i].sItemID );
						pRequest->SystemNotice( "ä»Žäº¤æ˜“è¯·æ±‚è€…ã€Š%sã€‹äº¤æ˜“æå–ç‰©å“ã€Š%dã€‹ç‰©å“å¤±è´¥ï¼ID = %d",
							pRequest->GetName(), pTradeData->ReqTradeData.ItemArray[i].sItemID );
						LG( "trade_error", "ä»Žäº¤æ˜“è¯·æ±‚è€…ã€Š%sã€‹äº¤æ˜“æå–ç‰©å“ã€Š%dã€‹ç‰©å“å¤±è´¥ï¼ID = %d",
							pRequest->GetName(), pTradeData->ReqTradeData.ItemArray[i].sItemID );*/
						// FIX: Pass error code as third argument to match format string
						pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00067),
											  pRequest->GetName(), pTradeData->ReqTradeData.ItemArray[i].sItemID, sPopResult);
						pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00067),
											   pRequest->GetName(), pTradeData->ReqTradeData.ItemArray[i].sItemID, sPopResult);
						LG("trade_error", "Failed to extract item [%d] from trade requester [%s]! Error = %d, Index = %d",
						   pTradeData->ReqTradeData.ItemArray[i].sItemID, pRequest->GetName(), sPopResult,
						   pTradeData->ReqTradeData.ItemArray[i].byIndex);
						pTradeData->UnlockExecution();
						return FALSE;
					}

					if (pItem->sType == enumItemTypeBoat) {
						CCharacter* pBoat = pRequest->GetPlayer()->GetBoat((DWORD)ReqGrid[i].GetDBParam(enumITEMDBP_INST_ID));
						if (pBoat) {
							/*sprintf( szTemp, "%dè‰˜èˆ¹åªã€Š%sã€‹ID[0x%X]ï¼Œ", ReqGrid[i].sNum, pBoat->GetName(),
								ReqGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );*/
							sprintf(szTemp, RES_STRING(GM_CHARTRADE_CPP_00063), ReqGrid[i].sNum, pBoat->GetName(),
									ReqGrid[i].GetDBParam(enumITEMDBP_INST_ID));
							strcat(szTrade, szTemp);
						} else {
							/*sprintf( szTemp, "%dè‰˜èˆ¹åªï¼šæœªçŸ¥èˆ¹åªæ•°æ®ID[0x%X]ï¼Œ", ReqGrid[i].sNum,
								ReqGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );*/
							sprintf(szTemp, RES_STRING(GM_CHARTRADE_CPP_00063), ReqGrid[i].sNum,
									ReqGrid[i].GetDBParam(enumITEMDBP_INST_ID));
							strcat(szTrade, szTemp);
						}

						if (!pRequest->BoatClear(ReqGrid[i].GetDBParam(enumITEMDBP_INST_ID))) {
							/*pAccept->SystemNotice( "åˆ é™¤%sçš„èˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼ID[0x%X]",
								pRequest->GetName(), ReqGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );
							pRequest->SystemNotice( "åˆ é™¤%sçš„èˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼ID[0x%X]",
								pRequest->GetName(), ReqGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );
							LG( "trade_error", "åˆ é™¤%sçš„èˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼DBID[0x%X]",
								pRequest->GetName(), ReqGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );*/
							pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00068),
												  pRequest->GetName(), ReqGrid[i].GetDBParam(enumITEMDBP_INST_ID));
							pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00068),
												   pRequest->GetName(), ReqGrid[i].GetDBParam(enumITEMDBP_INST_ID));
							LG("trade_error", "it failed to delete boat that captain confirm of %s haveï¼DBID[0x%X]",
							   pRequest->GetName(), ReqGrid[i].GetDBParam(enumITEMDBP_INST_ID));
						}
					} else {
						sprintf(szTemp, "%dä¸ª%sï¼Œ", ReqGrid[i].sNum, pItem->szName);
						strcat(szTrade, szTemp);
					}
				}
			}
		}
		strcat(szTrade, "}");

		for (int i = 0; i < ROLE_MAXNUM_TRADEDATA; i++) {
			if (pTradeData->AcpTradeData.ItemArray[i].sItemID != 0) {
				CItemRecord* pItem = GetItemRecordInfo(pTradeData->AcpTradeData.ItemArray[i].sItemID);
				if (pItem == nullptr) {
					/*pRequest->SystemNotice( "ç‰©å“IDé”™è¯¯ï¼Œæ— æ³•æ‰¾åˆ°è¯¥ç‰©å“ä¿¡æ¯ï¼ä¸èƒ½ç»™äºˆä½ è¯¥ç‰©å“ï¼ŒID = %d",
						pTradeData->AcpTradeData.ItemArray[i].sItemID );
					pAccept->SystemNotice( "ç‰©å“IDé”™è¯¯ï¼Œæ— æ³•æ‰¾åˆ°è¯¥ç‰©å“ä¿¡æ¯ï¼ä¸èƒ½ç»™äºˆä½ è¯¥ç‰©å“ï¼ŒID = %d",
						pTradeData->AcpTradeData.ItemArray[i].sItemID );
					LG( "trade_error", "ç‰©å“IDé”™è¯¯ï¼Œæ— æ³•æ‰¾åˆ°è¯¥ç‰©å“ä¿¡æ¯ï¼ä¸èƒ½ç»™äºˆä½ è¯¥ç‰©å“ï¼ŒID = %d",
						pTradeData->AcpTradeData.ItemArray[i].sItemID );*/
					pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00069),
										   pTradeData->AcpTradeData.ItemArray[i].sItemID);
					pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00069),
										  pTradeData->AcpTradeData.ItemArray[i].sItemID);
					LG("trade_error", "res ID errorï¼Œit cannot find res informationï¼it cannot give you this resï¼ŒID = %d",
					   pTradeData->AcpTradeData.ItemArray[i].sItemID);
					continue;
				}

				// å°†æŽ¥å—è€…ç»™çš„ä¸œè¥¿èµ‹äºˆè¯·æ±‚è€…
				USHORT sCount = AcpGrid[i].sNum;
				Short sPushPos = defKITBAG_DEFPUSH_POS;
				Short sPushRet = pRequest->KbPushItem(true, false, AcpGrid + i, sPushPos);

				if (sPushRet == enumKBACT_ERROR_FULL) // é“å…·æ å·²æ»¡ï¼Œä¸¢åˆ°åœ°é¢
				{
					// èŽ·å¾—ç‰©å“è§¦å‘äº‹ä»¶
					USHORT sNum = sCount - AcpGrid[i].sNum;

					CCharacter *pCCtrlCha = pRequest->GetPlyCtrlCha(), *pCMainCha = pRequest->GetPlyMainCha();
					Long lPosX, lPosY;
					pCCtrlCha->GetTrowItemPos(&lPosX, &lPosY);
					if (pCCtrlCha->GetSubMap()->ItemSpawn(AcpGrid + i, lPosX, lPosY, enumITEM_APPE_THROW, pCCtrlCha->GetID(), pCMainCha->GetID(), pCMainCha->GetHandle()) == nullptr) {
						/*pAccept->SystemNotice( "äº¤æ˜“æ—¶å°†%sèƒŒåŒ…è£…ä¸ä¸‹çš„ç‰©å“ã€Š%sã€‹æ”¾åˆ°åœ°é¢å¤±è´¥ï¼äº¤æ˜“ç‰©å“ä¸¢å¤±ï¼ID[%d], Num[%d]",
							pRequest->GetName(), pItem->szName, AcpGrid[i].sID, AcpGrid[i].sNum );
						pRequest->SystemNotice( "äº¤æ˜“æ—¶å°†%sèƒŒåŒ…è£…ä¸ä¸‹çš„ç‰©å“ã€Š%sã€‹æ”¾åˆ°åœ°é¢å¤±è´¥ï¼äº¤æ˜“ç‰©å“ä¸¢å¤±ï¼ID[%d], Num[%d]",
							pRequest->GetName(), pItem->szName, AcpGrid[i].sID, AcpGrid[i].sNum );
						LG( "trade_error", "Error code[%d],äº¤æ˜“æ—¶å°†%sèƒŒåŒ…è£…ä¸ä¸‹çš„ç‰©å“ã€Š%sã€‹æ”¾åˆ°åœ°é¢å¤±è´¥ï¼äº¤æ˜“ç‰©å“ä¸¢å¤±ï¼ID[%d], Num[%d]",
							sPushRet, pRequest->GetName(), pItem->szName, AcpGrid[i].sID, AcpGrid[i].sNum );*/
						pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00070),
											  pRequest->GetName(), pItem->szName, AcpGrid[i].sID, AcpGrid[i].sNum);
						pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00070),
											   pRequest->GetName(), pItem->szName, AcpGrid[i].sID, AcpGrid[i].sNum);
						LG("trade_error", "Error code[%d],when trading,%s bag is full,ã€Š%sã€‹failed to put on floorï¼trade res failedï¼ID[%d], Num[%d]",
						   sPushRet, pRequest->GetName(), pItem->szName, AcpGrid[i].sID, AcpGrid[i].sNum);
					}
				} else if (sPushRet != enumKBACT_SUCCESS) {
					/*pAccept->SystemNotice( "äº¤æ˜“æ—¶å°†ç‰©å“ã€Š%sã€‹æ”¾å…¥%sèƒŒåŒ…å¤±è´¥ï¼äº¤æ˜“ç‰©å“ä¸¢å¤±ï¼ID[%d], Num[%d]", pItem->szName, pRequest->GetName(),
						AcpGrid[i].sID, ReqGrid[i].sNum );
					pRequest->SystemNotice( "äº¤æ˜“æ—¶å°†ç‰©å“ã€Š%sã€‹æ”¾å…¥%sèƒŒåŒ…å¤±è´¥ï¼äº¤æ˜“ç‰©å“ä¸¢å¤±ï¼ID[%d], Num[%d]", pItem->szName, pRequest->GetName(),
						AcpGrid[i].sID, ReqGrid[i].sNum );
					LG( "trade_error", "Error code[%d],äº¤æ˜“æ—¶å°†ç‰©å“ã€Š%sã€‹æ”¾å…¥%sèƒŒåŒ…å¤±è´¥ï¼äº¤æ˜“ç‰©å“ä¸¢å¤±ï¼ID[%d], Num[%d]", sPushRet, pItem->szName, pRequest->GetName(),
						AcpGrid[i].sID, ReqGrid[i].sNum );*/
					pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00071), pItem->szName, pRequest->GetName(),
										  AcpGrid[i].sID, ReqGrid[i].sNum);
					pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00071), pItem->szName, pRequest->GetName(),
										   AcpGrid[i].sID, ReqGrid[i].sNum);
					LG("trade_error", "Error code[%d],it failed to put res in %s bag when trading res ã€Š%sã€‹ï¼trade res failedï¼ID[%d], Num[%d]", sPushRet, pItem->szName, pRequest->GetName(),
					   AcpGrid[i].sID, ReqGrid[i].sNum);
				} else {
					AcpGrid[i].sNum = 0;
				}

				if (sPushRet != enumKBACT_ERROR_FULL && pItem->sType == enumItemTypeBoat) {
					if (!pRequest->BoatAdd(AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID))) {
						/*pAccept->SystemNotice( "æ·»åŠ ç»™%sèˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼ID[0x%X]",
							pRequest->GetName(), AcpGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );
						pRequest->SystemNotice( "æ·»åŠ ç»™%sèˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼ID[0x%X]",
							pRequest->GetName(), AcpGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );
						LG( "trade_error", "æ·»åŠ ç»™%sèˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼DBID[0x%X]",
							pRequest->GetName(), AcpGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );*/
						pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00072),
											  pRequest->GetName(), AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID));
						pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00072),
											   pRequest->GetName(), AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID));
						LG("trade_error", "add to %scaptain confirm it hold boat failedï¼DBID[0x%X]",
						   pRequest->GetName(), AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID));
					}
				}
			}

			//
			if (pTradeData->ReqTradeData.ItemArray[i].sItemID != 0) {
				CItemRecord* pItem = GetItemRecordInfo(pTradeData->ReqTradeData.ItemArray[i].sItemID);
				if (pItem == nullptr) {
					/*pRequest->SystemNotice( "ç‰©å“IDé”™è¯¯ï¼Œæ— æ³•æ‰¾åˆ°è¯¥ç‰©å“ä¿¡æ¯ï¼ä¸èƒ½ç»™äºˆä½ è¯¥ç‰©å“ï¼ŒID = %d",
						pTradeData->ReqTradeData.ItemArray[i].sItemID );
					pAccept->SystemNotice( "ç‰©å“IDé”™è¯¯ï¼Œæ— æ³•æ‰¾åˆ°è¯¥ç‰©å“ä¿¡æ¯ï¼ä¸èƒ½ç»™äºˆä½ è¯¥ç‰©å“ï¼ŒID = %d",
						pTradeData->ReqTradeData.ItemArray[i].sItemID );
					LG( "trade_error", "ç‰©å“IDé”™è¯¯ï¼Œæ— æ³•æ‰¾åˆ°è¯¥ç‰©å“ä¿¡æ¯ï¼ä¸èƒ½ç»™äºˆä½ è¯¥ç‰©å“ï¼ŒID = %d",
						pTradeData->ReqTradeData.ItemArray[i].sItemID );*/
					pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00069),
										   pTradeData->ReqTradeData.ItemArray[i].sItemID);
					pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00069),
										  pTradeData->ReqTradeData.ItemArray[i].sItemID);
					LG("trade_error", "res ID errorï¼Œcannot find this res informationï¼this res cannot give youï¼ŒID = %d",
					   pTradeData->ReqTradeData.ItemArray[i].sItemID);
					continue;
				}

				// å°†è¯·æ±‚è€…ç»™çš„ä¸œè¥¿èµ‹äºˆæŽ¥å—è€…
				USHORT sCount = ReqGrid[i].sNum;
				Short sPushPos = defKITBAG_DEFPUSH_POS;
				Short sPushRet = pAccept->KbPushItem(true, false, ReqGrid + i, sPushPos);

				if (sPushRet == enumKBACT_ERROR_FULL) // é“å…·æ å·²æ»¡ï¼Œä¸¢åˆ°åœ°é¢
				{
					// èŽ·å¾—ç‰©å“è§¦å‘äº‹ä»¶
					USHORT sNum = sCount - ReqGrid[i].sNum;

					CCharacter *pCCtrlCha = pAccept->GetPlyCtrlCha(), *pCMainCha = pAccept->GetPlyMainCha();
					Long lPosX, lPosY;
					pCCtrlCha->GetTrowItemPos(&lPosX, &lPosY);
					if (pCCtrlCha->GetSubMap()->ItemSpawn(ReqGrid + i, lPosX, lPosY, enumITEM_APPE_THROW, pCCtrlCha->GetID(), pCMainCha->GetID(), pCMainCha->GetHandle()) == nullptr) {
						/*pAccept->SystemNotice( "äº¤æ˜“æ—¶å°†%sèƒŒåŒ…è£…ä¸ä¸‹çš„ç‰©å“ã€Š%sã€‹æ”¾åˆ°åœ°é¢å¤±è´¥ï¼äº¤æ˜“ç‰©å“ä¸¢å¤±ï¼ID[%d], Num[%d]",
							pAccept->GetName(), pItem->szName, ReqGrid[i].sID, ReqGrid[i].sNum );
						pRequest->SystemNotice( "äº¤æ˜“æ—¶å°†%sèƒŒåŒ…è£…ä¸ä¸‹çš„ç‰©å“ã€Š%sã€‹æ”¾åˆ°åœ°é¢å¤±è´¥ï¼äº¤æ˜“ç‰©å“ä¸¢å¤±ï¼ID[%d], Num[%d]",
							pAccept->GetName(), pItem->szName, ReqGrid[i].sID, ReqGrid[i].sNum );
						LG( "trade_error", "Error code[%d],äº¤æ˜“æ—¶å°†%sèƒŒåŒ…è£…ä¸ä¸‹çš„ç‰©å“ã€Š%sã€‹æ”¾åˆ°åœ°é¢å¤±è´¥ï¼äº¤æ˜“ç‰©å“ä¸¢å¤±ï¼ID[%d], Num[%d]",
							sPushRet, pAccept->GetName(), pItem->szName, ReqGrid[i].sID, ReqGrid[i].sNum );*/
						pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00070),
											  pAccept->GetName(), pItem->szName, ReqGrid[i].sID, ReqGrid[i].sNum);
						pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00070),
											   pAccept->GetName(), pItem->szName, ReqGrid[i].sID, ReqGrid[i].sNum);
						LG("trade_error", "Error code[%d],when trading,%s bag is full,ã€Š%sã€‹failed to put on floorï¼trade res failedï¼ID[%d], Num[%d]",
						   sPushRet, pRequest->GetName(), pItem->szName, AcpGrid[i].sID, AcpGrid[i].sNum);
					}
				} else if (sPushRet != enumKBACT_SUCCESS) {
					/*pAccept->SystemNotice( "äº¤æ˜“æ—¶å°†ç‰©å“ã€Š%sã€‹æ”¾å…¥%sèƒŒåŒ…å¤±è´¥ï¼äº¤æ˜“ç‰©å“ä¸¢å¤±ï¼ID[%d], Num[%d]", pItem->szName, pAccept->GetName(),
						ReqGrid[i].sID, ReqGrid[i].sNum );
					pRequest->SystemNotice( "äº¤æ˜“æ—¶å°†ç‰©å“ã€Š%sã€‹æ”¾å…¥%sèƒŒåŒ…å¤±è´¥ï¼äº¤æ˜“ç‰©å“ä¸¢å¤±ï¼ID[%d], Num[%d]", pItem->szName, pAccept->GetName(),
						ReqGrid[i].sID, ReqGrid[i].sNum );
					LG( "trade_error", "Error code[%d],äº¤æ˜“æ—¶å°†ç‰©å“ã€Š%sã€‹æ”¾å…¥%sèƒŒåŒ…å¤±è´¥ï¼äº¤æ˜“ç‰©å“ä¸¢å¤±ï¼ID[%d], Num[%d]", sPushRet, pItem->szName, pAccept->GetName(),
						ReqGrid[i].sID, ReqGrid[i].sNum );*/
					pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00071), pItem->szName, pAccept->GetName(),
										  ReqGrid[i].sID, ReqGrid[i].sNum);
					pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00071), pItem->szName, pAccept->GetName(),
										   ReqGrid[i].sID, ReqGrid[i].sNum);
					LG("trade_error", "Error code[%d],it failed to put res in %s bag when trading res ã€Š%sã€‹ï¼trade res failedï¼ID[%d], Num[%d]", sPushRet, pItem->szName, pRequest->GetName(),
					   AcpGrid[i].sID, ReqGrid[i].sNum);
				} else {
					ReqGrid[i].sNum = 0;
				}

				if (sPushRet != enumKBACT_ERROR_FULL && pItem->sType == enumItemTypeBoat) {
					if (!pAccept->BoatAdd(ReqGrid[i].GetDBParam(enumITEMDBP_INST_ID))) {
						/*pAccept->SystemNotice( "æ·»åŠ ç»™%sèˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼ID[0x%X]",
							pRequest->GetName(), AcpGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );
						pRequest->SystemNotice( "æ·»åŠ ç»™%sèˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼ID[0x%X]",
							pRequest->GetName(), AcpGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );
						LG( "trade_error", "æ·»åŠ ç»™%sèˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼DBID[0x%X]",
							pRequest->GetName(), AcpGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );*/
						pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00072),
											  pRequest->GetName(), AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID));
						pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00072),
											   pRequest->GetName(), AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID));
						LG("trade_error", "add to %scaptain confirm it hold boat failedï¼DBID[0x%X]",
						   pRequest->GetName(), AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID));
					}
				}
			}
		}

		// æ‰£é’±
		// Requester gives gold to Accepter
                if (pTradeData->ReqTradeData.llMoney > 0) {
                        __int64 maxGold = 100000000000LL;
                        __int64 acceptGold = pAccept->getAttr(ATTR_GD);
                        __int64 reqMoney = pTradeData->ReqTradeData.llMoney;
                        __int64 actualTransfer = reqMoney;
                        
                        if (acceptGold >= maxGold) {
                                pAccept->SystemNotice("Trade complete but your gold is at the maximum limit (100 billion)!");
                                actualTransfer = 0;
                        } else if (acceptGold + reqMoney > maxGold) {
                                actualTransfer = maxGold - acceptGold;
                                pAccept->SystemNotice("Received %lld gold from trade (capped at max 100 billion).", actualTransfer);
                        }
                        
                        pRequest->setAttr(ATTR_GD, pRequest->getAttr(ATTR_GD) - actualTransfer);
                        pAccept->setAttr(ATTR_GD, acceptGold + actualTransfer);
                }

                // Accepter gives gold to Requester
                if (pTradeData->AcpTradeData.llMoney > 0) {
                        __int64 maxGold = 100000000000LL;
                        __int64 reqGold = pRequest->getAttr(ATTR_GD);
                        __int64 acpMoney = pTradeData->AcpTradeData.llMoney;
                        __int64 actualTransfer = acpMoney;
                        
                        if (reqGold >= maxGold) {
                                pRequest->SystemNotice("Trade complete but your gold is at the maximum limit (100 billion)!");
                                actualTransfer = 0;
                        } else if (reqGold + acpMoney > maxGold) {
                                actualTransfer = maxGold - reqGold;
                                pRequest->SystemNotice("Received %lld gold from trade (capped at max 100 billion).", actualTransfer);
                        }
                        
                        pAccept->setAttr(ATTR_GD, pAccept->getAttr(ATTR_GD) - actualTransfer);
                        pRequest->setAttr(ATTR_GD, reqGold + actualTransfer);
                }

		// IMP
		if (pTradeData->ReqTradeData.dwIMP > 0) {
			pRequest->SetIMP(pRequest->GetIMP() - pTradeData->ReqTradeData.dwIMP);
			pAccept->SetIMP(pAccept->GetIMP() + pTradeData->ReqTradeData.dwIMP);
		}

		if (pTradeData->AcpTradeData.dwIMP > 0) {
			pAccept->SetIMP(pAccept->GetIMP() - pTradeData->AcpTradeData.dwIMP);
			pRequest->SetIMP(pRequest->GetIMP() + pTradeData->AcpTradeData.dwIMP);
		}

		// sprintf( szTemp, "æŽ¥å—è€…äº¤æ˜“é‡‘é’±ï¼š%dï¼Œè¯·æ±‚è€…äº¤æ˜“é‡‘é’±ï¼š%d", pTradeData->AcpTradeData.llMoney,
		// pTradeData->ReqTradeData.llMoney );
		sprintf(szTemp, RES_STRING(GM_CHARTRADE_CPP_00073), pTradeData->AcpTradeData.llMoney,
				pTradeData->ReqTradeData.llMoney);
		strcat(szTrade, szTemp);

		pAccept->SetTradeData(nullptr);
		pRequest->SetTradeData(nullptr);
		// NOTE: pTradeData->Free() moved to end of trade completion to fix use-after-free bug

		// æ•°æ®åº“å­˜å‚¨
		game_db.BeginTran();
		if (!pRequest->SaveAssets() || !pAccept->SaveAssets()) {
			game_db.RollBack();

			// äº¤æ˜“æ•°æ®åº“å­˜å‚¨å¤±è´¥ï¼Œæ•°æ®æ¢å¤
			ReqBag = ReqBagData;
			AcpBag = AcpBagData;
			pRequest->setAttr(ATTR_GD, llReqMoney);
			pAccept->setAttr(ATTR_GD, llAcpMoney);

			// æ¢å¤èˆ¹åªæ•°æ®ä¿¡æ¯
			for (int i = 0; i < ROLE_MAXNUM_TRADEDATA; i++) {
				if (pTradeData->AcpTradeData.ItemArray[i].sItemID != 0) {
					CItemRecord* pItem = GetItemRecordInfo(pTradeData->AcpTradeData.ItemArray[i].sItemID);
					if (pItem == nullptr) {
						/*pRequest->SystemNotice( "ç‰©å“IDé”™è¯¯ï¼Œæ— æ³•æ‰¾åˆ°è¯¥ç‰©å“ä¿¡æ¯ï¼ä¸èƒ½ç»™äºˆä½ è¯¥ç‰©å“ï¼ŒID = %d",
						pTradeData->AcpTradeData.ItemArray[i].sItemID );
					pAccept->SystemNotice( "ç‰©å“IDé”™è¯¯ï¼Œæ— æ³•æ‰¾åˆ°è¯¥ç‰©å“ä¿¡æ¯ï¼ä¸èƒ½ç»™äºˆä½ è¯¥ç‰©å“ï¼ŒID = %d",
						pTradeData->AcpTradeData.ItemArray[i].sItemID );
					LG( "trade_error", "ç‰©å“IDé”™è¯¯ï¼Œæ— æ³•æ‰¾åˆ°è¯¥ç‰©å“ä¿¡æ¯ï¼ä¸èƒ½ç»™äºˆä½ è¯¥ç‰©å“ï¼ŒID = %d",
						pTradeData->AcpTradeData.ItemArray[i].sItemID );*/
						pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00069),
											   pTradeData->AcpTradeData.ItemArray[i].sItemID);
						pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00069),
											  pTradeData->AcpTradeData.ItemArray[i].sItemID);
						LG("trade_error", "res ID errorï¼Œit cannot find res informationï¼it cannot give you this resï¼ŒID = %d",
						   pTradeData->AcpTradeData.ItemArray[i].sItemID);
						continue;
					}

					// å°†æŽ¥å—è€…ç»™çš„ä¸œè¥¿èµ‹äºˆè¯·æ±‚è€…
					if (pItem->sType == enumItemTypeBoat) {
						if (!pRequest->BoatClear(AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID))) {

							/*pAccept->SystemNotice( "åˆ é™¤%sçš„èˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼ID[0x%X]",
								pRequest->GetName(), ReqGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );
							pRequest->SystemNotice( "åˆ é™¤%sçš„èˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼ID[0x%X]",
								pRequest->GetName(), ReqGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );
							LG( "trade_error", "åˆ é™¤%sçš„èˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼DBID[0x%X]",
								pRequest->GetName(), ReqGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );*/
							pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00068),
												  pRequest->GetName(), ReqGrid[i].GetDBParam(enumITEMDBP_INST_ID));
							pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00068),
												   pRequest->GetName(), ReqGrid[i].GetDBParam(enumITEMDBP_INST_ID));
							LG("trade_error", "it failed to delete boat that captain confirm of %s haveï¼DBID[0x%X]",
							   pRequest->GetName(), ReqGrid[i].GetDBParam(enumITEMDBP_INST_ID));
						}

						if (!pAccept->BoatAdd(AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID))) {
							/*pAccept->SystemNotice( "æ·»åŠ ç»™%sèˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼ID[0x%X]",
							pRequest->GetName(), AcpGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );
						pRequest->SystemNotice( "æ·»åŠ ç»™%sèˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼ID[0x%X]",
							pRequest->GetName(), AcpGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );
						LG( "trade_error", "æ·»åŠ ç»™%sèˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼DBID[0x%X]",
							pRequest->GetName(), AcpGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );*/
							pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00072),
												  pRequest->GetName(), AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID));
							pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00072),
												   pRequest->GetName(), AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID));
							LG("trade_error", "add to %scaptain confirm it hold boat failedï¼DBID[0x%X]",
							   pRequest->GetName(), AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID));
						}
					}
				}

				//
				if (pTradeData->ReqTradeData.ItemArray[i].sItemID != 0) {
					CItemRecord* pItem = GetItemRecordInfo(pTradeData->ReqTradeData.ItemArray[i].sItemID);
					if (pItem == nullptr) {
						/*pRequest->SystemNotice( "ç‰©å“IDé”™è¯¯ï¼Œæ— æ³•æ‰¾åˆ°è¯¥ç‰©å“ä¿¡æ¯ï¼ä¸èƒ½ç»™äºˆä½ è¯¥ç‰©å“ï¼ŒID = %d",
						pTradeData->AcpTradeData.ItemArray[i].sItemID );
					pAccept->SystemNotice( "ç‰©å“IDé”™è¯¯ï¼Œæ— æ³•æ‰¾åˆ°è¯¥ç‰©å“ä¿¡æ¯ï¼ä¸èƒ½ç»™äºˆä½ è¯¥ç‰©å“ï¼ŒID = %d",
						pTradeData->AcpTradeData.ItemArray[i].sItemID );
					LG( "trade_error", "ç‰©å“IDé”™è¯¯ï¼Œæ— æ³•æ‰¾åˆ°è¯¥ç‰©å“ä¿¡æ¯ï¼ä¸èƒ½ç»™äºˆä½ è¯¥ç‰©å“ï¼ŒID = %d",
						pTradeData->AcpTradeData.ItemArray[i].sItemID );*/
						pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00069),
											   pTradeData->AcpTradeData.ItemArray[i].sItemID);
						pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00069),
											  pTradeData->AcpTradeData.ItemArray[i].sItemID);
						LG("trade_error", "res ID errorï¼Œit cannot find res informationï¼it cannot give you this resï¼ŒID = %d",
						   pTradeData->AcpTradeData.ItemArray[i].sItemID);
						continue;
					}

					// å°†è¯·æ±‚è€…ç»™çš„ä¸œè¥¿èµ‹äºˆæŽ¥å—è€…
					if (pItem->sType == enumItemTypeBoat) {
						if (!pAccept->BoatClear(ReqGrid[i].GetDBParam(enumITEMDBP_INST_ID))) {

							/*pAccept->SystemNotice( "åˆ é™¤%sçš„èˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼ID[0x%X]",
								pRequest->GetName(), ReqGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );
							pRequest->SystemNotice( "åˆ é™¤%sçš„èˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼ID[0x%X]",
								pRequest->GetName(), ReqGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );
							LG( "trade_error", "åˆ é™¤%sçš„èˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼DBID[0x%X]",
								pRequest->GetName(), ReqGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );*/
							pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00068),
												  pRequest->GetName(), ReqGrid[i].GetDBParam(enumITEMDBP_INST_ID));
							pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00068),
												   pRequest->GetName(), ReqGrid[i].GetDBParam(enumITEMDBP_INST_ID));
							LG("trade_error", "it failed to delete boat that captain confirm of %s haveï¼DBID[0x%X]",
							   pRequest->GetName(), ReqGrid[i].GetDBParam(enumITEMDBP_INST_ID));
						}

						if (!pRequest->BoatAdd(ReqGrid[i].GetDBParam(enumITEMDBP_INST_ID))) {
							/*pAccept->SystemNotice( "æ·»åŠ ç»™%sèˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼ID[0x%X]",
							pRequest->GetName(), AcpGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );
						pRequest->SystemNotice( "æ·»åŠ ç»™%sèˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼ID[0x%X]",
							pRequest->GetName(), AcpGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );
						LG( "trade_error", "æ·»åŠ ç»™%sèˆ¹é•¿è¯æ˜Žæ‹¥æœ‰çš„èˆ¹åªå¤±è´¥ï¼DBID[0x%X]",
							pRequest->GetName(), AcpGrid[i].GetDBParam( enumITEMDBP_INST_ID ) );*/
							pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00072),
												  pRequest->GetName(), AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID));
							pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00072),
												   pRequest->GetName(), AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID));
							LG("trade_error", "add to %scaptain confirm it hold boat failedï¼DBID[0x%X]",
							   pRequest->GetName(), AcpGrid[i].GetDBParam(enumITEMDBP_INST_ID));
						}
					}
				}
			}

			// é€šçŸ¥å®¢æˆ·ç«¯å¹¶ä¸”è®°å½•æ—¥å¿—
			/*pRequest->SystemNotice( "äº¤æ˜“å¤±è´¥ï¼Œæ•°æ®å­˜å‚¨é”™è¯¯ï¼" );
			pAccept->SystemNotice( "äº¤æ˜“å¤±è´¥ï¼Œæ•°æ®å­˜å‚¨é”™è¯¯ï¼" );
			LG( "trade_error", "äº¤æ˜“æ•°æ®å­˜å‚¨æ•°æ®åº“å¤±è´¥ï¼Œäº¤æ˜“æ•°æ®æ¢å¤å®Œæˆï¼Œäº¤æ˜“ï¼šè¯·æ±‚æ–¹ã€Š%sã€‹ID[0x%X]ï¼ŒæŽ¥å—æ–¹ã€Š%sã€‹ID[0x%X]ã€‚",
				pRequest->GetName(), pRequest->GetPlayer()->GetDBChaId(), pAccept->GetName(), pAccept->GetPlayer()->GetDBChaId() );*/
			pRequest->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00074));
			pAccept->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00074));
			LG("trade_error", "the trade data failed to memory in DBï¼Œtrade data resume completeï¼Œtradeï¼šrequest oneã€Š%sã€‹ID[0x%X]ï¼Œaccept oneã€Š%sã€‹ID[0x%X]ã€‚",
			   pRequest->GetName(), pRequest->GetPlayer()->GetDBChaId(), pAccept->GetName(), pAccept->GetPlayer()->GetDBChaId());

			// å–æ¶ˆè§’è‰²é”å®šçŠ¶æ€
			pAccept->TradeAction(FALSE);
			pRequest->TradeAction(FALSE);

			// è§’è‰²äº¤æ˜“æˆåŠŸ
			WPACKET packet = GETWPACKET();
			WRITE_CMD(packet, CMD_MC_CHARTRADE);
			WRITE_SHORT(packet, CMD_MC_CHARTRADE_RESULT);
			WRITE_CHAR(packet, TRADE_FAILER);

			pAccept->ReflectINFof(pMain, packet);
			pRequest->ReflectINFof(pMain, packet);

			// Free trade data after all uses complete (fixes use-after-free bug)
			pTradeData->Free();

			return FALSE;
		} else {
			// ä¸¤æ¬¡æ•°æ®å­˜å‚¨æˆåŠŸ
			game_db.CommitTran();

			if (pRequest->IsBoat()) {
				char szBoat1[64] = "";
				char szBoat2[64] = "";
				// sprintf( szBoat1, "%sèˆ¹ã€Š%sã€‹", pAccept->GetPlyMainCha()->GetName(), pAccept->GetName() );
				// sprintf( szBoat2, "%sèˆ¹ã€Š%sã€‹", pRequest->GetPlyMainCha()->GetName(), pRequest->GetName() );
				sprintf(szBoat1, RES_STRING(GM_CHARTRADE_CPP_00075), pAccept->GetPlyMainCha()->GetName(), pAccept->GetName());
				sprintf(szBoat2, RES_STRING(GM_CHARTRADE_CPP_00075), pRequest->GetPlyMainCha()->GetName(), pRequest->GetName());
				TL(CHA_CHA, szBoat1, szBoat2, szTrade);
			} else {
				TL(CHA_CHA, pAccept->GetName(), pRequest->GetName(), szTrade);
			}

			pRequest->LogAssets(enumLASSETS_TRADE);
			pAccept->LogAssets(enumLASSETS_TRADE);
		}

		// äº¤æ˜“ç‰©å“æˆåŠŸåŽèŽ·å–è§¦å‘
		for (int i = 0; i < ROLE_MAXNUM_TRADEDATA; i++) {
			if (pTradeData->AcpTradeData.ItemArray[i].sItemID != 0) {
				pAccept->RefreshNeedItem(pTradeData->AcpTradeData.ItemArray[i].sItemID);
				BYTE byNum = pTradeData->AcpTradeData.ItemArray[i].byCount - AcpGrid[i].sNum;
				if (byNum) {
					pRequest->AfterPeekItem(pTradeData->AcpTradeData.ItemArray[i].sItemID, byNum);
				}
			}

			if (pTradeData->ReqTradeData.ItemArray[i].sItemID != 0) {
				pRequest->RefreshNeedItem(pTradeData->ReqTradeData.ItemArray[i].sItemID);
				BYTE byNum = pTradeData->ReqTradeData.ItemArray[i].byCount - ReqGrid[i].sNum;
				if (byNum) {
					pAccept->AfterPeekItem(pTradeData->ReqTradeData.ItemArray[i].sItemID, byNum);
				}
			}
		}

		// é€šçŸ¥äº¤æ˜“é‡‘é’±æ•°æ®ä¿¡æ¯
		char szNotice[255];

		if (pTradeData->ReqTradeData.llMoney) {
			// pAccept->SystemNotice( "ä½ ä»Ž(%s)å¤„å¾—åˆ°äº†%dé‡‘é’±ï¼", pRequest->GetName(), pTradeData->ReqTradeData.llMoney );
			CFormatParameter param(2);
			param.setString(0, pRequest->GetName());
			param.setLong(1, pTradeData->ReqTradeData.llMoney);

			RES_FORMAT_STRING(GM_CHARTRADE_CPP_00076, param, szNotice);

			pAccept->SystemNotice(szNotice);
		}

		if (pTradeData->AcpTradeData.llMoney) {
			CFormatParameter param(2);
			param.setString(0, pAccept->GetName());
			param.setLong(1, pTradeData->AcpTradeData.llMoney);

			RES_FORMAT_STRING(GM_CHARTRADE_CPP_00076, param, szNotice);

			pRequest->SystemNotice(szNotice);
		}

		if (pTradeData->AcpTradeData.dwIMP) {
			sprintf(szNotice, "You have received [%d] IMPs from (%s).", pTradeData->AcpTradeData.dwIMP, pAccept->GetName());
			pRequest->SystemNotice(szNotice);
		}

		if (pTradeData->ReqTradeData.dwIMP) {
			sprintf(szNotice, "You have received [%d] IMPs from (%s).", pTradeData->ReqTradeData.dwIMP, pRequest->GetName());
			pAccept->SystemNotice(szNotice);
		}

		// åŒæ­¥é‡‘é’±æ•°æ®å’ŒèƒŒåŒ…æ•°æ®
		pAccept->SynAttr(enumATTRSYN_TRADE);
		pAccept->SyncBoatAttr(enumATTRSYN_TRADE);
		pRequest->SynAttr(enumATTRSYN_TRADE);
		pRequest->SyncBoatAttr(enumATTRSYN_TRADE);

		pRequest->SynKitbagNew(enumSYN_KITBAG_TRADE);
		pAccept->SynKitbagNew(enumSYN_KITBAG_TRADE);

		if (pTradeData->AcpTradeData.dwIMP > 0 || pTradeData->ReqTradeData.dwIMP > 0) {
			WPACKET packet = GETWPACKET();
			WRITE_CMD(packet, CMD_MC_UPDATEIMP);
			WRITE_LONG(packet, pAccept->GetIMP());
			pAccept->ReflectINFof(pMain, packet);

			WPACKET packet2 = GETWPACKET();
			WRITE_CMD(packet2, CMD_MC_UPDATEIMP);
			WRITE_LONG(packet2, pRequest->GetIMP());
			pRequest->ReflectINFof(pMain, packet2);
		}

		// å–æ¶ˆè§’è‰²é”å®šçŠ¶æ€
		pAccept->TradeAction(FALSE);
		pRequest->TradeAction(FALSE);

		// SECURITY: Log successful trade completion
		LG("trade_security", "TRADE COMPLETE: %s (ID:%d) <-> %s (ID:%d) | Money: %d <-> %d | IMP: %d <-> %d | Items: %d <-> %d",
		   pRequest->GetName(), pRequest->GetPlayer()->GetDBChaId(),
		   pAccept->GetName(), pAccept->GetPlayer()->GetDBChaId(),
		   pTradeData->ReqTradeData.llMoney, pTradeData->AcpTradeData.llMoney,
		   pTradeData->ReqTradeData.dwIMP, pTradeData->AcpTradeData.dwIMP,
		   pTradeData->ReqTradeData.byItemCount, pTradeData->AcpTradeData.byItemCount);

		// Free trade data now that all processing is complete (fixes use-after-free bug)
		pTradeData->Free();

		// è§’è‰²äº¤æ˜“æˆåŠŸ
		WPACKET packet = GETWPACKET();
		WRITE_CMD(packet, CMD_MC_CHARTRADE);
		WRITE_SHORT(packet, CMD_MC_CHARTRADE_RESULT);
		WRITE_CHAR(packet, TRADE_SUCCESS);

		pAccept->ReflectINFof(pMain, packet);
		pRequest->ReflectINFof(pMain, packet);

		// Save both players immediately after successful trade to prevent rollback/duplication
		game_db.SaveChaAssets(pAccept);
		game_db.SaveChaAssets(pRequest);
	} else {
		WPACKET packet = GETWPACKET();
		WRITE_CMD(packet, CMD_MC_CHARTRADE);
		WRITE_SHORT(packet, CMD_MC_CHARTRADE_VALIDATE);
		WRITE_LONG(packet, pMain->GetID());
		if (pMain == pTradeData->pRequest) {
			pTradeData->pAccept->ReflectINFof(pMain, packet);
		} else {
			pTradeData->pRequest->ReflectINFof(pMain, packet);
		}
	}

	return TRUE;
	T_E
}

void CTradeSystem::ResetItemState(CCharacter& character, CTradeData& TradeData) {
	T_B int nCapacity = character.m_CKitbag.GetCapacity();
	CKitbag& Bag = character.m_CKitbag;
	TRADE_DATA* pItemData;
	if (&character == TradeData.pAccept) {
		pItemData = &TradeData.AcpTradeData;
	} else {
		pItemData = &TradeData.ReqTradeData;
	}

	for (int i = 0; i < ROLE_MAXNUM_TRADEDATA; i++) {
		if (pItemData->ItemArray[i].byIndex < nCapacity) {
			Bag.Enable(pItemData->ItemArray[i].byIndex);
		}
	}
	T_E
}

CStoreSystem::CStoreSystem() : m_bValid(false){T_B

												   T_E}

							   CStoreSystem::~CStoreSystem(){T_B

																 T_E}

							   BOOL CStoreSystem::PushOrder(CCharacter * pCha, long long lOrderID, int lComID, int lNum) {
	T_B
	// Memory protection: Limit order list size to prevent unbounded growth
	if (m_OrderList.size() >= 10000) {
		LG("Store_error", "Order list exceeded limit (10000), rejecting new order for ChaID:%d\n", pCha->GetPlayer()->GetDBChaId());
		return FALSE;
	}
	
		SOrderData OrderInfo;
	OrderInfo.lOrderID = lOrderID;
	OrderInfo.ChaID = pCha->GetPlayer()->GetDBChaId();
	strcpy(OrderInfo.ChaName, pCha->GetName());
	OrderInfo.lComID = lComID;
	OrderInfo.lNum = lNum;
	OrderInfo.lRecDBID = pCha->GetKitbagTmpRecDBID();
	OrderInfo.dwTickCount = GetTickCount();

	m_OrderList.push_back(OrderInfo);
	LG("Store_order", "PushOrder:[OrderID:%lld][ChaID:%d][ChaName:%s][ComID:%d][Num:%d][RecDBID:%d][TickCount:%d]\n", OrderInfo.lOrderID, OrderInfo.ChaID, OrderInfo.ChaName, OrderInfo.lComID, OrderInfo.lNum, OrderInfo.lRecDBID, OrderInfo.dwTickCount);
	return true;
	T_E
}

SOrderData CStoreSystem::PopOrder(long long lOrderID) {
	T_B
		SOrderData OrderInfo;
	BOOL bFound = false;

	vector<SOrderData>::iterator vec_it;
	for (vec_it = m_OrderList.begin(); vec_it != m_OrderList.end(); vec_it++) {
		if ((*vec_it).lOrderID == lOrderID) {
			OrderInfo.ChaID = (*vec_it).ChaID;
			strcpy(OrderInfo.ChaName, (*vec_it).ChaName);
			OrderInfo.lComID = (*vec_it).lComID;
			OrderInfo.lNum = (*vec_it).lNum;
			OrderInfo.lOrderID = (*vec_it).lOrderID;
			OrderInfo.lRecDBID = (*vec_it).lRecDBID;
			OrderInfo.dwTickCount = (*vec_it).dwTickCount;
			m_OrderList.erase(vec_it);
			bFound = TRUE;
			LG("Store_order", "PopOrder:[OrderID:%lld][ChaID:%d][ChaName:%s][ComID:%d][Num:%d][RecDBID:%d][TickCount:%d]\n", OrderInfo.lOrderID, OrderInfo.ChaID, OrderInfo.ChaName, OrderInfo.lComID, OrderInfo.lNum, OrderInfo.lRecDBID, OrderInfo.dwTickCount);
			break;
		}
	}
	if (!bFound) {
		OrderInfo.ChaID = 0;
		OrderInfo.lComID = 0;
		OrderInfo.lNum = 0;
		OrderInfo.lOrderID = 0;
		OrderInfo.lRecDBID = 0;
	}
	return OrderInfo;
	T_E
}

BOOL CStoreSystem::HasOrder(long long lOrderID) {
	T_B
		vector<SOrderData>::iterator vec_it;
	for (vec_it = m_OrderList.begin(); vec_it != m_OrderList.end(); vec_it++) {
		if ((*vec_it).lOrderID == lOrderID) {
			return true;
		}
	}
	return false;
	T_E
}

int CStoreSystem::GetClassId(int lComID) {
	T_B
		map<int, int>::iterator it = m_ItemSearchList.find(lComID);
	if (it != m_ItemSearchList.end()) {
		return m_ItemSearchList[lComID];
	} else
		return 0;
	T_E
}

cChar* CStoreSystem::GetClassName(int lClsID) {
	T_B
		vector<ClassInfo>::iterator vec_it;
	for (vec_it = m_ItemClass.begin(); vec_it != m_ItemClass.end(); vec_it++) {
		if ((*vec_it).clsID == lClsID)
			return (*vec_it).clsName;
	}
	return nullptr;
	T_E
}

SItemData* CStoreSystem::GetItemData(int lComID) {
	T_B int lClsID = GetClassId(lComID);
	if (lClsID != 0) {
		vector<SItemData>::iterator it;
		for (it = m_ItemList[lClsID].begin(); it != m_ItemList[lClsID].end(); it++) {
			if ((*it).store_head.comID == lComID)
				return &(*it);
		}
	}
	return nullptr;
	T_E
}

BOOL CStoreSystem::DelItemData(int lComID) {
	T_B int lClsID = 0;

	map<int, int>::iterator cls_it = m_ItemSearchList.find(lComID);
	if (cls_it != m_ItemSearchList.end()) {
		lClsID = m_ItemSearchList[lComID];
		m_ItemSearchList.erase(cls_it);
	}

	if (lClsID != 0) {
		vector<SItemData>::iterator it;
		for (it = m_ItemList[lClsID].begin(); it != m_ItemList[lClsID].end(); it++) {
			if ((*it).store_head.comID == lComID) {
				m_ItemList[lClsID].erase(it);
				break;
			}
		}
	}

	return TRUE;
	T_E
}

BOOL CStoreSystem::Request(CCharacter* pCha, int lComID) {
	T_B if (pCha->IsStoreBuy()) {
		// pCha->SystemNotice("æ‚¨çš„ä¸Šä¸€ä¸ªè®¢å•è¿˜æœªå¤„ç†å®Œ!");
		pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00077));

		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_STORE_BUY_ASR);
		WRITE_CHAR(WtPk, 0);
		pCha->ReflectINFof(pCha, WtPk);

		return false;
	}

	SItemData* pComData = GetItemData(lComID);
	if (!pComData || pComData->store_head.comNumber == 0) {
		// pCha->SystemNotice("è¯¥å•†å“å·²æ’¤æž¶!");
		pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00078));

		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_STORE_BUY_ASR);
		WRITE_CHAR(WtPk, 0);
		pCha->ReflectINFof(pCha, WtPk);

		return false;
	}

	cChar* szClsName = GetClassName(pComData->store_head.comClass);
	if (szClsName) {
		// if(!strcmp(szClsName, "ç™½é‡‘ä¼šå‘˜"))
		if (!strcmp(szClsName, RES_STRING(GM_CHARTRADE_CPP_00079))) {
			// Modify by lark.li 20080919 begin
			// if(pCha->GetPlayer()->GetVipType() == 0 || )
			if (pCha->GetPlayer()->GetVipType() == 0 || pCha->m_SChaPart.sTypeID == 1 || pCha->m_SChaPart.sTypeID == 2)
			// End
			{
				// pCha->SystemNotice("åªæœ‰ç™½é‡‘ä¼šå‘˜æ‰èƒ½ä¹°è¿™ä¸ªå•†å“!");
				pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00080));

				WPACKET WtPk = GETWPACKET();
				WRITE_CMD(WtPk, CMD_MC_STORE_BUY_ASR);
				WRITE_CHAR(WtPk, 0);
				pCha->ReflectINFof(pCha, WtPk);

				return false;
			}
		}
	}

	short sGridNum = 0;
	ItemInfo* pItem = pComData->pItemArray;
	for (int i = 0; i < pComData->store_head.itemNum; i++) {
		CItemRecord* pItemRec = GetItemRecordInfo(pItem->itemID);
		if (pItemRec == nullptr) {
			// pCha->SystemNotice( "Request: é”™è¯¯çš„ç‰©å“æ•°æ®ç±»åž‹ï¼ID = %d", pItem->itemID );
			pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00081), pItem->itemID);

			WPACKET WtPk = GETWPACKET();
			WRITE_CMD(WtPk, CMD_MC_STORE_BUY_ASR);
			WRITE_CHAR(WtPk, 0);
			pCha->ReflectINFof(pCha, WtPk);

			return false;
		}
		if (pItemRec->GetIsPile()) {
			sGridNum += 1;
		} else {
			sGridNum += pItem->itemNum;
		}

		pItem++;
	}

	if (!pCha->HasLeaveBagTempGrid(sGridNum)) {
		// pCha->PopupNotice("æ‚¨çš„ä¸´æ—¶èƒŒåŒ…ç©ºé—´ä¸å¤Ÿ!");
		pCha->PopupNotice(RES_STRING(GM_CHARTRADE_CPP_00082));
		return false;
	}

	pNetMessage pNm = new NetMessage();
	RoleInfo* pChaInfo = new RoleInfo();

	pChaInfo->actID = pCha->GetPlayer()->GetActLoginID();
	strcpy(pChaInfo->actName, pCha->GetPlayer()->GetActName());
	pChaInfo->chaID = pCha->GetPlayer()->GetDBChaId();
	strcpy(pChaInfo->chaName, pCha->GetName());
	pChaInfo->moBean = pCha->GetPlayer()->GetMoBean();
	pChaInfo->rplMoney = pCha->GetPlayer()->GetRplMoney();
	pChaInfo->vip = pCha->GetPlayer()->GetVipType();

	BuildNetMessage(pNm, INFO_STORE_BUY, 0, lComID, 0, (unsigned char*)pChaInfo, sizeof(RoleInfo));
	if (HasOrder(pNm->msgHead.msgOrder)) {
		SAFE_DELETE(pChaInfo);
		FreeNetMessage(pNm);
		// LG("Store_msg", "è®¢å•å·[ID:%lld]é‡å¤!\n", pNm->msgHead.msgOrder);
		LG("Store_msg", "order form number [ID:%lld] repeat!\n", pNm->msgHead.msgOrder);

		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_STORE_BUY_ASR);
		WRITE_CHAR(WtPk, 0);
		pCha->ReflectINFof(pCha, WtPk);
		// pCha->SystemNotice("å•†åŸŽæ“ä½œå¤±è´¥, è®¢å•å·é‡å¤!");
		pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00083));

		return false;
	}
	if (IsValid() && g_gmsvr->GetInfoServer()->SendData(pNm)) {
		pCha->SetStoreBuy(true);
		PushOrder(pCha, pNm->msgHead.msgOrder, lComID, 1);
		// LG("Store_record", "è§’è‰²[%s][ID:%d]è®¢è´­äº†å•†å“[ID:%d]!\n", pChaInfo->chaName, pChaInfo->chaID, lComID);
		LG("Store_record", "character [%s][ID:%d] order merchandise [ID:%d]!\n", pChaInfo->chaName, pChaInfo->chaID, lComID);
	} else {
		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_STORE_BUY_ASR);
		WRITE_CHAR(WtPk, 0);
		pCha->ReflectINFof(pCha, WtPk);
		// pCha->SystemNotice("å•†åŸŽäº¤æ˜“å¤±è´¥!");
		pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00084));

		// LG("Store_msg", "Request: InfoServerå‘é€å¤±è´¥!\n");

		LG("Store_msg", "Request: InfoServer send failed!\n");
	}

	SAFE_DELETE(pChaInfo);
	FreeNetMessage(pNm);

	return true;
	T_E
}

BOOL CStoreSystem::Accept(CCharacter* pCha, int lComID) {
	T_B
		pCha->SetStoreBuy(false);
	SItemData* pComData = GetItemData(lComID);
	if (pComData) {
		int lNum = pComData->store_head.itemNum;
		ItemInfo* pItem = pComData->pItemArray;

		while (lNum-- > 0) {
			pCha->AddItem2KitbagTemp((short)pItem->itemID, (short)pItem->itemNum, pItem);
			pItem++;
		}

		// LG("Store_record", "è§’è‰²[%s][ID:%d]è´­ä¹°äº†å•†å“[ID:%d], åŠ é“å…·æ“ä½œæˆåŠŸ!\n", pCha->GetName(), pCha->GetPlayer()->GetDBChaId(), lComID);
		LG("Store_record", RES_STRING(GM_CHARTRADE_CPP_00085), pCha->GetName(), pCha->GetPlayer()->GetDBChaId(), lComID);

		if (pComData->store_head.comNumber > 0) {
			pComData->store_head.comNumber--;
		}

		// åˆ é™¤å•†å“
		if (pComData->store_head.comNumber <= 0 && pComData->store_head.comNumber != -1) {
			DelItemData(lComID);
		}
	} else {
		// LG("Store_msg", "Accept2: æ‰¾ä¸åˆ°å•†å“[ID:%d]!\n", lComID);
		LG("Store_msg", "Accept2: cannot find merchandise [ID:%d]!\n", lComID);
		return false;
	}
	return true;
	T_E
}

BOOL CStoreSystem::Accept(long long lOrderID, RoleInfo* ChaInfo) {
	T_B

	BOOL bSucc = false;
	SOrderData OrderInfo = PopOrder(lOrderID);
	if (OrderInfo.lOrderID != 0) {
		int lChaID = OrderInfo.ChaID;
		int lComID = OrderInfo.lComID;
		CCharacter* pCha = nullptr;
		CPlayer* pPlayer = g_pGameApp->GetPlayerByDBID(lChaID);
		if (pPlayer) {
			pCha = pPlayer->GetMainCha();
		}

		// LG("Store_record", "è§’è‰²[%s][ID:%d]è´­ä¹°äº†å•†å“[ID:%d], å·²æ‰£æ¬¾!\n", ChaInfo->chaName, lChaID, lComID);
		LG("Store_record", "character[%s][ID:%d] has buy res[ID:%d], has buckle money!\n", ChaInfo->chaName, lChaID, lComID);
		SItemData* pCData = GetItemData(lComID);
		if (pCData->store_head.comNumber > 0) {
			pCData->store_head.comNumber--;
		}

		if (!pCha) {
			// LG("Store_msg", "è§’è‰²[%s][ID:%d]å·²ç¦»å¼€!\n", ChaInfo->chaName, lChaID);
			LG("Store_msg", "character[%s][ID:%d] has leava!\n", ChaInfo->chaName, lChaID);
		}

		// åŠ é“å…·
		if (pCha) {
			pCha->SetStoreBuy(false);
			SItemData* pComData = GetItemData(lComID);
			if (pComData) {
				int lNum = pComData->store_head.itemNum;
				ItemInfo* pItem = pComData->pItemArray;

				while (lNum-- > 0) {
					pCha->AddItem2KitbagTemp((short)pItem->itemID, (short)pItem->itemNum, pItem);
					pItem++;
				}
				bSucc = true;

				pCha->GetPlayer()->SetMoBean(ChaInfo->moBean);
				pCha->GetPlayer()->SetRplMoney(ChaInfo->rplMoney);
				pCha->GetPlayer()->SetVipType(ChaInfo->vip);
			} else {
				// LG("Store_msg", "æ‰¾ä¸åˆ°å•†å“[ID:%d]!\n", lComID);
				LG("Store_msg", "cannot finde merchandise[ID:%d]!\n", lComID);
			}
		} else {
			// LG("Store_msg", "è§’è‰²[%s][ID:%d]ä¸åœ¨æœ¬åœ°å›¾!\n", ChaInfo->chaName, lChaID);
			LG("Store_msg", "character[%s][ID:%d] don't in this map!\n", ChaInfo->chaName, lChaID);

			BOOL bOnline;
			if (!game_db.IsChaOnline(lChaID, bOnline)) {
				// LG("Store_msg", "è¯»å–è§’è‰²åœ¨çº¿çŠ¶æ€å¤±è´¥ã€‚\n");
				LG("Store_msg", "it failed to get character online stateã€‚\n");
			} else {
				if (!bOnline) {
					// LG("Store_msg", "è§’è‰²ä¸åœ¨çº¿ã€‚\n");
					LG("Store_msg", "character didn't onlineã€‚\n");

					if (!game_db.SaveStoreItemID(lChaID, lComID)) {
						// LG("Store_msg", "å­˜å…¥ç¦»çº¿è§’è‰²å•†å“ä¿¡æ¯å¤±è´¥ã€‚\n");
						LG("Store_msg", "it failed to memory merchandise information who did not online characterã€‚\n");
					}
				} else {
					// LG("Store_msg", "è§’è‰²[%s][ID:%d]åœ¨å…¶ä»–åœ°å›¾!\n", ChaInfo->chaName, lChaID);
					LG("Store_msg", "character[%s][ID:%d]is in other map!\n", ChaInfo->chaName, lChaID);

					BEGINGETGATE();
					CPlayer* pCPlayer;
					CCharacter* pChaOut = 0;
					GateServer* pGateServer;
					while (pGateServer = GETNEXTGATE()) {
						bool bFound = false;

						if (!BEGINGETPLAYER(pGateServer))
							continue;
						while (pCPlayer = (CPlayer*)GETNEXTPLAYER(pGateServer)) {
							pChaOut = pCPlayer->GetMainCha();
							if (pChaOut) {
								WPACKET WtPk = GETWPACKET();
								WRITE_CMD(WtPk, CMD_MM_STORE_BUY);
								WRITE_LONG(WtPk, pChaOut->GetID());
								WRITE_LONG(WtPk, lChaID);
								WRITE_LONG(WtPk, lComID);
								// WRITE_LONG(WtPk, ChaInfo->moBean);
								WRITE_LONG(WtPk, ChaInfo->rplMoney);
								pChaOut->ReflectINFof(pChaOut, WtPk); // é€šå‘Š

								bFound = true;
								break;
							}
						}

						if (bFound) {
							break;
						}
					}
				}
			}
		}
		// è®°å½•
		if (bSucc) {
			LG("Store_record", "è§’è‰²[%s][ID:%d]è´­ä¹°äº†å•†å“[ID:%d], åŠ é“å…·æ“ä½œæˆåŠŸ!\n", pCha->GetName(), lChaID, lComID);

			// é€šçŸ¥çŽ©å®¶äº¤æ˜“æˆåŠŸ
			WPACKET WtPk = GETWPACKET();
			WRITE_CMD(WtPk, CMD_MC_STORE_BUY_ASR);
			WRITE_CHAR(WtPk, 1);
			WRITE_LONG(WtPk, ChaInfo->rplMoney);

			pCha->ReflectINFof(pCha, WtPk);
		} else {
		}

		// åˆ é™¤å•†å“
		if (pCData->store_head.comNumber <= 0 && pCData->store_head.comNumber != -1) {
			DelItemData(lComID);
		}
	} else {
		// LG("Store_msg", "Accept:æ‰¾ä¸åˆ°è®¢å•[ID:%lld]!\n", lOrderID);
		LG("Store_msg", "Accept:not find order form[ID:%lld]!\n", lOrderID);
	}
	return true;
	T_E
}

BOOL CStoreSystem::Cancel(long long lOrderID) {
	T_B

	SOrderData OrderInfo = PopOrder(lOrderID);

	CCharacter* pCha = nullptr;
	CPlayer* pPlayer = g_pGameApp->GetPlayerByDBID(OrderInfo.ChaID);
	if (pPlayer) {
		pCha = pPlayer->GetMainCha();
	}

	if (OrderInfo.lOrderID != 0) {
		// é€šçŸ¥çŽ©å®¶äº¤æ˜“å¤±è´¥
		if (pCha) {
			pCha->SetStoreBuy(false);

			WPACKET WtPk = GETWPACKET();
			WRITE_CMD(WtPk, CMD_MC_STORE_BUY_ASR);
			WRITE_CHAR(WtPk, 0);
			pCha->ReflectINFof(pCha, WtPk);

			// pCha->SystemNotice("å•†åŸŽäº¤æ˜“å¤±è´¥!");
			pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00084));
			// LG("Store_data", "[%s]è´­ä¹°é“å…·[ComID:%d]å¤±è´¥!\n", pCha->GetName(), OrderInfo.lComID);
			LG("Store_data", "[%s]failed to buy prop [ComID:%d]!\n", pCha->GetName(), OrderInfo.lComID);
		}
	} else {
		// LG("Store_msg", "Cancel:æ‰¾ä¸åˆ°è®¢å•[ID:%lld]!\n", lOrderID);
		LG("Store_msg", "Cancel:not find order form[ID:%lld]!\n", lOrderID);
	}
	return true;
	T_E
}

void CStoreSystem::Run(DWORD dwCurTime, DWORD dwIntervalTime, DWORD dwOrderTime) {
	try {
		static DWORD dwLastTime = 0;
		if (dwCurTime - dwLastTime < dwIntervalTime) {
			return;
		} else {
			dwLastTime = dwCurTime;
		}

		vector<SOrderData>::iterator vec_it;
		for (vec_it = m_OrderList.begin(); vec_it != m_OrderList.end(); vec_it++) {
			if (dwCurTime - (*vec_it).dwTickCount > dwOrderTime) {
				DWORD dwChaID = (*vec_it).ChaID;
				LG("Store_order", "timeout:[OrderID:%lld][ChaID:%d][ChaName:%s][ComID:%d][Num:%d][RecDBID:%d][TickCount:%d]\n", (*vec_it).lOrderID, (*vec_it).ChaID, (*vec_it).ChaName, (*vec_it).lComID, (*vec_it).lNum, (*vec_it).lRecDBID, (*vec_it).dwTickCount);
				m_OrderList.erase(vec_it);

				CCharacter* pCha = nullptr;
				CPlayer* pPlayer = g_pGameApp->GetPlayerByDBID(dwChaID);
				if (pPlayer) {
					pCha = pPlayer->GetMainCha();
				}
				if (pCha) {
					// pCha->SystemNotice("å•†åŸŽæ“ä½œè¶…æ—¶,å·²è¢«å–æ¶ˆ!");
					pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00095));
				}

				break;
			}
		}
	} catch (...) {
		LG("trade_error", "Exception in CheckShopTimeout()\n");
	}
}

BOOL CStoreSystem::GetItemList() {
	T_B
		// LG("Store_msg", "è¯·æ±‚å•†åŸŽåˆ—è¡¨!\n");
		LG("Store_msg", "ask for store list!\n");
	pNetMessage pNm = new NetMessage();
	BuildNetMessage(pNm, INFO_REQUEST_STORE, 0, 0, 0, nullptr, 0);
	g_gmsvr->GetInfoServer()->SendData(pNm);
	FreeNetMessage(pNm);
	return true;
	T_E
}

BOOL CStoreSystem::RequestItemList(CCharacter* pCha, int lClsID, short sPage, short sNum) {
	T_B
		map<int, vector<SItemData>>::iterator map_it = m_ItemList.find(lClsID);
	if (map_it != m_ItemList.end()) {
		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_STORE_LIST_ASR);
		short sItemNum = 0;
		short sPageNum = static_cast<short>((m_ItemList[lClsID].size() % sNum == 0) ? (m_ItemList[lClsID].size() / sNum) : (m_ItemList[lClsID].size() / sNum + 1));
		WRITE_SHORT(WtPk, sPageNum);
		WRITE_SHORT(WtPk, sPage);
		if (sPage > sPageNum) {
			sItemNum = 0;
			// LG("Store_msg", "çŽ©å®¶è¯·æ±‚çš„é¡µé¢è¶…å‡ºäº†èŒƒå›´!\n");
			LG("Store_msg", "player open-eared page layout over range!\n");
		} else if (sPage == sPageNum) {
			sItemNum = static_cast<short>(m_ItemList[lClsID].size() - (sPage - 1) * sNum);
		} else {
			sItemNum = sNum;
		}
		WRITE_SHORT(WtPk, sItemNum);
		vector<SItemData>::iterator it = m_ItemList[lClsID].begin();
		int i;
		for (i = 0; i < (sPage - 1) * sNum; i++) {
			it++;
		}
		for (i = 0; i < sItemNum; i++) {
			int l_time = (int)((*it).store_head.comExpire);
			if (l_time <= 0) {
				l_time = -1;
			} else {
				l_time -= (int)time(0);
				l_time /= 3600;
				if (l_time < 1) {
					l_time = 1;
				}
			}

			WRITE_LONG(WtPk, (*it).store_head.comID);
			WRITE_SEQ(WtPk, (*it).store_head.comName, uShort(strlen((*it).store_head.comName) + 1));
			WRITE_LONG(WtPk, (*it).store_head.comPrice);
			WRITE_SEQ(WtPk, (*it).store_head.comRemark, USHORT(strlen((*it).store_head.comRemark) + 1));
			WRITE_CHAR(WtPk, (*it).store_head.isHot);
			WRITE_LONG(WtPk, static_cast<int>((*it).store_head.comTime));
			WRITE_LONG(WtPk, static_cast<int>((*it).store_head.comNumber));
			WRITE_LONG(WtPk, l_time);

			WRITE_SHORT(WtPk, (*it).store_head.itemNum);
			int j;
			for (j = 0; j < (*it).store_head.itemNum; j++) {
				WRITE_SHORT(WtPk, (*it).pItemArray[j].itemID);
				WRITE_SHORT(WtPk, (*it).pItemArray[j].itemNum);
				WRITE_SHORT(WtPk, (*it).pItemArray[j].itemFlute);
				int k;
				for (k = 0; k < 5; k++) {
					WRITE_SHORT(WtPk, (*it).pItemArray[j].itemAttrID[k]);
					WRITE_SHORT(WtPk, (*it).pItemArray[j].itemAttrVal[k]);
				}
			}

			it++;
		}
		pCha->ReflectINFof(pCha, WtPk);
	} else {
		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_STORE_LIST_ASR);
		WRITE_SHORT(WtPk, 0);
		WRITE_SHORT(WtPk, sPage);
		WRITE_SHORT(WtPk, 0);
		pCha->ReflectINFof(pCha, WtPk);
	}

	return true;
	T_E
}

BOOL CStoreSystem::RequestVIP(CCharacter* pCha, short sVipID, short sMonth) {
	T_B
		pNetMessage pNm = new NetMessage();
	RoleInfo* pChaInfo = new RoleInfo();

	pChaInfo->actID = pCha->GetPlayer()->GetActLoginID();
	strcpy(pChaInfo->actName, pCha->GetPlayer()->GetActName());
	pChaInfo->chaID = pCha->GetPlayer()->GetDBChaId();
	strcpy(pChaInfo->chaName, pCha->GetName());
	pChaInfo->moBean = pCha->GetPlayer()->GetMoBean();
	pChaInfo->rplMoney = pCha->GetPlayer()->GetRplMoney();
	pChaInfo->vip = pCha->GetPlayer()->GetVipType();

	DWORD dwVipParam = ((sVipID << 16) & 0xffff0000) | (sMonth & 0x0000ffff);

	BuildNetMessage(pNm, INFO_REGISTER_VIP, 0, dwVipParam, 0, (unsigned char*)pChaInfo, sizeof(RoleInfo));
	if (HasOrder(pNm->msgHead.msgOrder)) {
		SAFE_DELETE(pChaInfo);
		FreeNetMessage(pNm);
		// LG("Store_msg", "è®¢å•å·[ID:%lld]é‡å¤!\n", pNm->msgHead.msgOrder);
		LG("Store_msg", "order form number[ID:%lld]repeat!\n", pNm->msgHead.msgOrder);

		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_STORE_VIP);
		WRITE_CHAR(WtPk, 0);
		pCha->ReflectINFof(pCha, WtPk);
		// pCha->SystemNotice("å•†åŸŽæ“ä½œå¤±è´¥, è®¢å•å·é‡å¤!");
		pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00083));

		return false;
	}
	if (IsValid() && g_gmsvr->GetInfoServer()->SendData(pNm)) {
		PushOrder(pCha, pNm->msgHead.msgOrder, 0, 0);
	} else {
		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_STORE_VIP);
		WRITE_CHAR(WtPk, 0);
		pCha->ReflectINFof(pCha, WtPk);

		//	LG("Store_msg", "RequestVIP: InfoServerå‘é€å¤±è´¥!\n");
		LG("Store_msg", "RequestVIP: InfoServer send failed!\n");
	}

	SAFE_DELETE(pChaInfo);
	FreeNetMessage(pNm);

	return true;
	T_E
}

BOOL CStoreSystem::AcceptVIP(long long lOrderID, RoleInfo* ChaInfo, DWORD dwVipParam) {
	T_B

	SOrderData OrderInfo = PopOrder(lOrderID);

	if (OrderInfo.lOrderID == 0) {
		// LG("Store_msg", "AcceptVIP:æ‰¾ä¸åˆ°è®¢å•[ID:%lld]!\n", lOrderID);
		LG("Store_msg", "AcceptVIP: cannot find order form[ID:%lld]!\n", lOrderID);
		return false;
	}

	CCharacter* pCha = nullptr;
	CPlayer* pPlayer = g_pGameApp->GetPlayerByDBID(OrderInfo.ChaID);
	if (pPlayer) {
		pCha = pPlayer->GetMainCha();
	}

	if (pCha) {
		pCha->ResetStoreTime();
		pCha->GetPlayer()->SetMoBean(ChaInfo->moBean);
		pCha->GetPlayer()->SetRplMoney(ChaInfo->rplMoney);
		pCha->GetPlayer()->SetVipType(ChaInfo->vip);

		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_STORE_VIP);
		WRITE_CHAR(WtPk, 1);
		WRITE_SHORT(WtPk, HIWORD(dwVipParam));
		WRITE_SHORT(WtPk, LOWORD(dwVipParam));
		WRITE_LONG(WtPk, ChaInfo->rplMoney);
		WRITE_LONG(WtPk, ChaInfo->moBean);
		pCha->ReflectINFof(pCha, WtPk);
		// pCha->PopupNotice("è´­ä¹°ç™½é‡‘ä¼šå‘˜æˆåŠŸ!");
		pCha->PopupNotice(RES_STRING(GM_CHARTRADE_CPP_00086));
		// LG("Store_data", "[%s]è´­ä¹°VIPæˆåŠŸ!\n", pCha->GetName());
		LG("Store_data", "[%s] purchase VIP succeed!\n", pCha->GetName());
	}
	return true;
	T_E
}

BOOL CStoreSystem::CancelVIP(long long lOrderID) {
	T_B

	SOrderData OrderInfo = PopOrder(lOrderID);
	if (OrderInfo.lOrderID != 0) {
		CCharacter* pCha = nullptr;
		CPlayer* pPlayer = g_pGameApp->GetPlayerByDBID(OrderInfo.ChaID);
		if (pPlayer) {
			pCha = pPlayer->GetMainCha();
		}

		if (pCha) {
			pCha->ResetStoreTime();
			WPACKET WtPk = GETWPACKET();
			WRITE_CMD(WtPk, CMD_MC_STORE_VIP);
			WRITE_CHAR(WtPk, 0);
			pCha->ReflectINFof(pCha, WtPk);
			// pCha->PopupNotice("è´­ä¹°ç™½é‡‘ä¼šå‘˜å¤±è´¥!");
			pCha->PopupNotice(RES_STRING(GM_CHARTRADE_CPP_00087));
			// LG("Store_data", "[%s]è´­ä¹°VIPå¤±è´¥!\n", pCha->GetName());
			LG("Store_data", "[%s]perchase VIP failed!\n", pCha->GetName());
		}
	} else {
		// LG("Store_msg", "CancelVIP:æ‰¾ä¸åˆ°è®¢å•[ID:%lld]!\n", lOrderID);
		LG("Store_msg", "CancelVIP:cannot find order form[ID:%lld]!\n", lOrderID);
	}
	return true;
	T_E
}

BOOL CStoreSystem::RequestChange(CCharacter* pCha, int lNum) {
	T_B
		pNetMessage pNm = new NetMessage();
	RoleInfo* pChaInfo = new RoleInfo();

	pChaInfo->actID = pCha->GetPlayer()->GetActLoginID();
	strcpy(pChaInfo->actName, pCha->GetPlayer()->GetActName());
	pChaInfo->chaID = pCha->GetPlayer()->GetDBChaId();
	strcpy(pChaInfo->chaName, pCha->GetName());
	pChaInfo->moBean = pCha->GetPlayer()->GetMoBean();
	pChaInfo->rplMoney = pCha->GetPlayer()->GetRplMoney();
	pChaInfo->vip = pCha->GetPlayer()->GetVipType();

	BuildNetMessage(pNm, INFO_EXCHANGE_MONEY, 0, lNum, 0, (unsigned char*)pChaInfo, sizeof(RoleInfo));
	if (HasOrder(pNm->msgHead.msgOrder)) {
		SAFE_DELETE(pChaInfo);
		FreeNetMessage(pNm);
		// LG("Store_msg", "è®¢å•å·[ID:%lld]é‡å¤!\n", pNm->msgHead.msgOrder);
		LG("Store_msg", "order form number [ID:%lld] repeat!\n", pNm->msgHead.msgOrder);

		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_STORE_CHANGE_ASR);
		WRITE_CHAR(WtPk, 0);
		pCha->ReflectINFof(pCha, WtPk);
		// pCha->SystemNotice("å•†åŸŽæ“ä½œå¤±è´¥, è®¢å•å·é‡å¤!");
		pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00083));

		return false;
	}
	if (IsValid() && g_gmsvr->GetInfoServer()->SendData(pNm)) {
		PushOrder(pCha, pNm->msgHead.msgOrder, 0, 0);
	} else {
		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_STORE_CHANGE_ASR);
		WRITE_CHAR(WtPk, 0);
		pCha->ReflectINFof(pCha, WtPk);

		// LG("Store_msg", "RequestChange: InfoServerå‘é€å¤±è´¥!\n");
		LG("Store_msg", "RequestChange: InfoServer send failed!\n");
	}

	SAFE_DELETE(pChaInfo);
	FreeNetMessage(pNm);

	return true;
	T_E
}

BOOL CStoreSystem::AcceptChange(long long lOrderID, RoleInfo* ChaInfo) {
	T_B

	SOrderData OrderInfo = PopOrder(lOrderID);

	if (OrderInfo.lOrderID == 0) {
		// LG("Store_msg", "AcceptChange:æ‰¾ä¸åˆ°è®¢å•[ID:%lld]!\n", lOrderID);
		LG("Store_msg", "AcceptChange:cannot find order form [ID:%lld]!\n", lOrderID);
		return false;
	}

	CCharacter* pCha = nullptr;
	CPlayer* pPlayer = g_pGameApp->GetPlayerByDBID(OrderInfo.ChaID);
	if (pPlayer) {
		pCha = pPlayer->GetMainCha();
	}

	if (pCha) {
		pCha->ResetStoreTime();
		pCha->GetPlayer()->SetMoBean(ChaInfo->moBean);
		pCha->GetPlayer()->SetRplMoney(ChaInfo->rplMoney);
		pCha->GetPlayer()->SetVipType(ChaInfo->vip);

		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_STORE_CHANGE_ASR);
		WRITE_CHAR(WtPk, 1);
		WRITE_LONG(WtPk, ChaInfo->moBean);
		WRITE_LONG(WtPk, ChaInfo->rplMoney);
		pCha->ReflectINFof(pCha, WtPk);
		// LG("Store_data", "[%s]å…‘æ¢ä»£å¸æˆåŠŸ!\n", pCha->GetName());
		LG("Store_data", "[%s]change token succeed!\n", pCha->GetName());
	}
	return true;
	T_E
}

BOOL CStoreSystem::CancelChange(long long lOrderID) {
	T_B

	SOrderData OrderInfo = PopOrder(lOrderID);
	if (OrderInfo.lOrderID != 0) {
		// é€šçŸ¥çŽ©å®¶å…‘æ¢ä»£å¸å¤±è´¥
		CCharacter* pCha = nullptr;
		CPlayer* pPlayer = g_pGameApp->GetPlayerByDBID(OrderInfo.ChaID);
		if (pPlayer) {
			pCha = pPlayer->GetMainCha();
		}

		if (pCha) {
			pCha->ResetStoreTime();
			WPACKET WtPk = GETWPACKET();
			WRITE_CMD(WtPk, CMD_MC_STORE_CHANGE_ASR);
			WRITE_CHAR(WtPk, 0);
			pCha->ReflectINFof(pCha, WtPk);
			// LG("Store_data", "[%s]å…‘æ¢ä»£å¸å¤±è´¥!\n", pCha->GetName());
			LG("Store_data", "[%s]change token failed!\n", pCha->GetName());
		}
	} else {
		LG("Store_msg", "CancelChange:cannot find order form[ID:%lld]!\n", lOrderID);
	}
	return true;
	T_E
}

BOOL CStoreSystem::RequestRoleInfo(CCharacter* pCha) {
	T_B
		pNetMessage pNm = new NetMessage();
	RoleInfo* pChaInfo = new RoleInfo();

	pChaInfo->actID = pCha->GetPlayer()->GetActLoginID();
	strcpy(pChaInfo->actName, pCha->GetPlayer()->GetActName());
	pChaInfo->chaID = pCha->GetPlayer()->GetDBChaId();
	strcpy(pChaInfo->chaName, pCha->GetName());

	BuildNetMessage(pNm, INFO_REQUEST_ACCOUNT, 0, 0, 0, (unsigned char*)pChaInfo, sizeof(RoleInfo));
	if (HasOrder(pNm->msgHead.msgOrder)) {
		SAFE_DELETE(pChaInfo);
		FreeNetMessage(pNm);
		// LG("Store_msg", "è®¢å•å·[ID:%lld]é‡å¤!\n", pNm->msgHead.msgOrder);
		LG("Store_msg", "order form number[ID:%lld]repeat!\n", pNm->msgHead.msgOrder);
		// pCha->SystemNotice("å•†åŸŽæ“ä½œå¤±è´¥, è®¢å•å·é‡å¤!");
		pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00083));

		return false;
	}
	if (IsValid() && g_gmsvr->GetInfoServer()->SendData(pNm)) {
		PushOrder(pCha, pNm->msgHead.msgOrder, 0, 0);
	} else {
		BOOL bValid = IsValid();
		if (bValid) {
			InValid();
		}

		pCha->GetPlayer()->SetMoBean(0);
		pCha->GetPlayer()->SetRplMoney(0);
		pCha->GetPlayer()->SetVipType(0);
		g_StoreSystem.Open(pCha, 0);

		if (bValid) {
			SetValid();
		}

		// LG("Store_msg", "RequestRoleInfo: InfoServerå‘é€å¤±è´¥!\n");
		LG("Store_msg", "RequestRoleInfo: InfoServer send failed!\n");
	}

	SAFE_DELETE(pChaInfo);
	FreeNetMessage(pNm);

	return true;
	T_E
}

BOOL CStoreSystem::AcceptRoleInfo(long long lOrderID, RoleInfo* ChaInfo) {
	T_B

	SOrderData OrderInfo = PopOrder(lOrderID);

	if (OrderInfo.lOrderID == 0) {
		// LG("Store_msg", "AcceptRoleInfo:æ‰¾ä¸åˆ°è®¢å•[ID:%lld]!\n", lOrderID);
		LG("Store_msg", "AcceptRoleInfo:cannot find order form [ID:%lld]!\n", lOrderID);
		return false;
	}

	int lChaID = ChaInfo->chaID;
	int lMoBean = ChaInfo->moBean;
	int lRplMoney = ChaInfo->rplMoney;
	int lVip = ChaInfo->vip;
	CCharacter* pCha = nullptr;
	CPlayer* pPlayer = g_pGameApp->GetPlayerByDBID(lChaID);
	if (pPlayer) {
		pCha = pPlayer->GetMainCha();
	}

	if (pCha) {
		pCha->ResetStoreTime();
		pCha->GetPlayer()->SetMoBean(lMoBean);
		pCha->GetPlayer()->SetRplMoney(lRplMoney);
		pCha->GetPlayer()->SetVipType(lVip);

		g_StoreSystem.Open(pCha, lVip);
		// LG("Store_data", "[%s]èŽ·å–å¸æˆ·ä¿¡æ¯æˆåŠŸ!\n", pCha->GetName());
		LG("Store_data", "[%s]get account information succeed!\n", pCha->GetName());
	}
	return true;
	T_E
}

BOOL CStoreSystem::CancelRoleInfo(long long lOrderID) {
	T_B

	SOrderData OrderInfo = PopOrder(lOrderID);
	if (OrderInfo.lOrderID == 0) {
		// LG("Store_msg", "CancelRoleInfo:æ‰¾ä¸åˆ°è®¢å•[ID:%lld]!\n", lOrderID);
		LG("Store_msg", "CancelRoleInfo:cannot find order form[ID:%lld]!\n", lOrderID);
		return false;
	}

	int lChaID = OrderInfo.ChaID;
	CCharacter* pCha = nullptr;
	CPlayer* pPlayer = g_pGameApp->GetPlayerByDBID(lChaID);
	if (pPlayer) {
		pCha = pPlayer->GetMainCha();
	}

	if (pCha) {
		pCha->GetPlayer()->SetMoBean(0);
		pCha->GetPlayer()->SetRplMoney(0);
		pCha->GetPlayer()->SetVipType(0);
		pCha->ResetStoreTime();
		// g_StoreSystem.Open(pCha, 0);
		/*pCha->SystemNotice("æ— æ³•æŸ¥åˆ°æ‚¨çš„è´¦å·,æ‰“å¼€å•†åŸŽå¤±è´¥!");
		LG("Store_data", "[%s]èŽ·å–å¸æˆ·ä¿¡æ¯å¤±è´¥!\n", pCha->GetName());*/
		pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00088));
		LG("Store_data", "[%s]get account information failed!\n", pCha->GetName());
	}
	return true;
	T_E
}

BOOL CStoreSystem::RequestRecord(CCharacter* pCha, int lNum) {
	T_B
		pNetMessage pNm = new NetMessage();
	RoleInfo* pChaInfo = new RoleInfo();

	pChaInfo->actID = pCha->GetPlayer()->GetActLoginID();
	strcpy(pChaInfo->actName, pCha->GetPlayer()->GetActName());
	pChaInfo->chaID = pCha->GetPlayer()->GetDBChaId();
	strcpy(pChaInfo->chaName, pCha->GetName());
	pChaInfo->moBean = pCha->GetPlayer()->GetMoBean();
	pChaInfo->rplMoney = pCha->GetPlayer()->GetRplMoney();
	pChaInfo->vip = pCha->GetPlayer()->GetVipType();

	BuildNetMessage(pNm, INFO_REQUEST_HISTORY, 0, lNum, 0, (unsigned char*)pChaInfo, sizeof(RoleInfo));
	if (HasOrder(pNm->msgHead.msgOrder)) {
		SAFE_DELETE(pChaInfo);
		FreeNetMessage(pNm);
		// LG("Store_msg", "è®¢å•å·[ID:%lld]é‡å¤!\n", pNm->msgHead.msgOrder);
		LG("Store_msg", "order form number[ID:%lld]repeat!\n", pNm->msgHead.msgOrder);

		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_STORE_QUERY);
		WRITE_CHAR(WtPk, 0);
		pCha->ReflectINFof(pCha, WtPk);
		// pCha->SystemNotice("å•†åŸŽæ“ä½œå¤±è´¥, è®¢å•å·é‡å¤!");
		pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00083));

		return false;
	}
	if (IsValid() && g_gmsvr->GetInfoServer()->SendData(pNm)) {
		PushOrder(pCha, pNm->msgHead.msgOrder, 0, lNum);
	} else {
		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_STORE_QUERY);
		WRITE_LONG(WtPk, 0);
		pCha->ReflectINFof(pCha, WtPk);

		// LG("Store_msg", "RequestRecord: InfoServerå‘é€å¤±è´¥!\n");
		LG("Store_msg", "RequestRecord: InfoServer send failed!\n");
	}

	SAFE_DELETE(pChaInfo);
	FreeNetMessage(pNm);

	return true;
	T_E
}

BOOL CStoreSystem::AcceptRecord(long long lOrderID, HistoryInfo* pRecord) {
	T_B

	SOrderData OrderInfo = PopOrder(lOrderID);
	if (OrderInfo.lOrderID == 0) {
		// LG("Store_msg", "AcceptRecord:æ‰¾ä¸åˆ°è®¢å•[ID:%lld]!\n", lOrderID);
		LG("Store_msg", "AcceptRecord:cannot find order form[ID:%lld]!\n", lOrderID);
		return false;
	}

	int lNum = OrderInfo.lNum;

	CCharacter* pCha = nullptr;
	CPlayer* pPlayer = g_pGameApp->GetPlayerByDBID(OrderInfo.ChaID);
	if (pPlayer) {
		pCha = pPlayer->GetMainCha();
	}

	if (pCha) {
		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_STORE_QUERY);
		WRITE_LONG(WtPk, lNum);
		int i;
		for (i = 0; i < lNum; i++) {
			time_t tradeT = static_cast<time_t>(pRecord->tradeTime);
			WRITE_SEQ(WtPk, ctime(&tradeT), USHORT(strlen(ctime(&tradeT)) + 1));
			WRITE_LONG(WtPk, pRecord->comID);
			WRITE_SEQ(WtPk, pRecord->comName, USHORT(strlen(pRecord->comName) + 1));
			WRITE_LONG(WtPk, pRecord->tradeMoney);
			pRecord++;
		}
		pCha->ReflectINFof(pCha, WtPk);
		// LG("Store_data", "[%s]æŸ¥è¯¢äº¤æ˜“è®°å½•æˆåŠŸ!\n", pCha->GetName());
		LG("Store_data", "[%s]query trade note succeed!\n", pCha->GetName());
	}
	return true;
	T_E
}

BOOL CStoreSystem::CancelRecord(long long lOrderID) {
	T_B

	SOrderData OrderInfo = PopOrder(lOrderID);

	if (OrderInfo.lOrderID == 0) {
		// LG("Store_msg", "CancelRecord:æ‰¾ä¸åˆ°è®¢å•[ID:%lld]!\n", lOrderID);
		LG("Store_msg", "CancelRecord:cannot find order form[ID:%lld]!\n", lOrderID);
		return false;
	}

	CCharacter* pCha = nullptr;
	CPlayer* pPlayer = g_pGameApp->GetPlayerByDBID(OrderInfo.ChaID);
	if (pPlayer) {
		pCha = pPlayer->GetMainCha();
	}

	if (pCha) {
		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_STORE_QUERY);
		WRITE_LONG(WtPk, 0);
		pCha->ReflectINFof(pCha, WtPk);
		// LG("Store_data", "[%s]æŸ¥è¯¢äº¤æ˜“è®°å½•å¤±è´¥!\n", pCha->GetName());
		LG("Store_data", "[%s]query trade note failed!\n", pCha->GetName());
	}
	return true;
	T_E
}

BOOL CStoreSystem::RequestGMSend(CCharacter* pCha, cChar* szTitle, cChar* szContent) {
	T_B
		pNetMessage pNm = new NetMessage();
	pMailInfo pMi = new MailInfo();

	strcpy(pMi->title, szTitle);
	strcpy(pMi->description, szContent);
	pMi->actID = pCha->GetPlayer()->GetActLoginID();
	pMi->chaID = pCha->GetPlayer()->GetDBChaId();
	strcpy(pMi->actName, pCha->GetPlayer()->GetActName());
	strcpy(pMi->chaName, pCha->GetName());
	pMi->sendTime = time(0);

	BuildNetMessage(pNm, INFO_SND_GM_MAIL, 0, 0, 0, (unsigned char*)pMi, sizeof(MailInfo));
	if (HasOrder(pNm->msgHead.msgOrder)) {
		SAFE_DELETE(pMi);
		FreeNetMessage(pNm);
		// LG("Store_msg", "è®¢å•å·[ID:%lld]é‡å¤!\n", pNm->msgHead.msgOrder);
		LG("Store_msg", "order form number[ID:%lld]repeat!\n", pNm->msgHead.msgOrder);
		// pCha->SystemNotice("é‚®ä»¶æ“ä½œå¤±è´¥, è®¢å•å·é‡å¤!");
		pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00089));

		return false;
	}
	if (IsValid() && g_gmsvr->GetInfoServer()->SendData(pNm)) {
		PushOrder(pCha, pNm->msgHead.msgOrder, 0, 0);
	} else {
		/*pCha->SystemNotice("GMé‚®ä»¶å‘é€å¤±è´¥ï¼Œå¦‚æžœæ‚¨å·²ç»å‘é€è¿‡ä¸€æ¬¡é‚®ä»¶ï¼Œè¯·ç­‰å¾…GMå›žå¤ä¹‹åŽå†æ¬¡å‘é€!");
		LG("Store_msg", "RequestGMSend: InfoServerå‘é€å¤±è´¥!\n");*/
		pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00090));
		LG("Store_msg", "RequestGMSend: InfoServer send failed!\n");
	}

	SAFE_DELETE(pMi);
	FreeNetMessage(pNm);

	return true;
	T_E
}

BOOL CStoreSystem::AcceptGMSend(long long lOrderID, int lMailID) {
	T_B

	SOrderData OrderInfo = PopOrder(lOrderID);

	if (OrderInfo.lOrderID == 0) {
		// LG("Store_msg", "AcceptGMSend:æ‰¾ä¸åˆ°è®¢å•[ID:%lld]!\n", lOrderID);
		LG("Store_msg", "AcceptGMSend:cannot find order form[ID:%lld]!\n", lOrderID);
		return false;
	}

	CCharacter* pCha = nullptr;
	CPlayer* pPlayer = g_pGameApp->GetPlayerByDBID(OrderInfo.ChaID);
	if (pPlayer) {
		pCha = pPlayer->GetMainCha();
	}

	if (pCha) {
		/*pCha->SystemNotice("GMé‚®ä»¶å‘é€æˆåŠŸ, [é—®é¢˜ID: %d]!", lMailID);
		LG("Store_data", "[%s]å‘é€GMé‚®ä»¶æˆåŠŸ!\n", pCha->GetName());*/
		pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00091), lMailID);
		LG("Store_data", "[%s]send GM mail succeed !\n", pCha->GetName());
	}

	return true;
	T_E
}

BOOL CStoreSystem::CancelGMSend(long long lOrderID) {
	T_B

	SOrderData OrderInfo = PopOrder(lOrderID);

	if (OrderInfo.lOrderID == 0) {
		// LG("Store_msg", "CancelGMSend:æ‰¾ä¸åˆ°è®¢å•[ID:%lld]!\n", lOrderID);
		LG("Store_msg", "CancelGMSend:cannot find order form[ID:%lld]!\n", lOrderID);
		return false;
	}

	CCharacter* pCha = nullptr;
	CPlayer* pPlayer = g_pGameApp->GetPlayerByDBID(OrderInfo.ChaID);
	if (pPlayer) {
		pCha = pPlayer->GetMainCha();
	}

	if (pCha) {
		/*pCha->SystemNotice("GMé‚®ä»¶å‘é€å¤±è´¥ï¼Œå¦‚æžœæ‚¨å·²ç»å‘é€è¿‡ä¸€æ¬¡é‚®ä»¶ï¼Œè¯·ç­‰å¾…GMå›žå¤ä¹‹åŽå†æ¬¡å‘é€!");
		LG("Store_data", "[%s]å‘é€GMé‚®ä»¶å¤±è´¥!\n", pCha->GetName());*/
		pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00090));
		LG("Store_data", "[%s]send GM mail failed!\n", pCha->GetName());
	}

	return true;
	T_E
}

BOOL CStoreSystem::RequestGMRecv(CCharacter* pCha) {
	T_B
		pNetMessage pNm = new NetMessage();
	RoleInfo* pChaInfo = new RoleInfo();

	pChaInfo->actID = pCha->GetPlayer()->GetActLoginID();
	strcpy(pChaInfo->actName, pCha->GetPlayer()->GetActName());
	pChaInfo->chaID = pCha->GetPlayer()->GetDBChaId();
	strcpy(pChaInfo->chaName, pCha->GetName());
	pChaInfo->moBean = pCha->GetPlayer()->GetMoBean();
	pChaInfo->rplMoney = pCha->GetPlayer()->GetRplMoney();
	pChaInfo->vip = pCha->GetPlayer()->GetVipType();

	BuildNetMessage(pNm, INFO_RCV_GM_MAIL, 0, 0, 0, (unsigned char*)pChaInfo, sizeof(RoleInfo));
	if (HasOrder(pNm->msgHead.msgOrder)) {
		SAFE_DELETE(pChaInfo);
		FreeNetMessage(pNm);
		// LG("Store_msg", "è®¢å•å·[ID:%lld]é‡å¤!\n", pNm->msgHead.msgOrder);
		LG("Store_msg", "order form number [ID:%lld]repeat!\n", pNm->msgHead.msgOrder);
		// pCha->SystemNotice("é‚®ä»¶æ“ä½œå¤±è´¥, è®¢å•å·é‡å¤!");
		pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00089));

		return false;
	}
	if (IsValid() && g_gmsvr->GetInfoServer()->SendData(pNm)) {
		PushOrder(pCha, pNm->msgHead.msgOrder, 0, 0);
	} else {
		// pCha->SystemNotice("GMé‚®ä»¶æŽ¥æ”¶å¤±è´¥!");
		pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00092));
		// LG("Store_msg", "RequestGMRecv: InfoServerå‘é€å¤±è´¥!\n");
		LG("Store_msg", "RequestGMRecv: InfoServersend failed!\n");
	}

	SAFE_DELETE(pChaInfo);
	FreeNetMessage(pNm);

	return true;
	T_E
}

BOOL CStoreSystem::AcceptGMRecv(long long lOrderID, MailInfo* pMi) {
	T_B

	SOrderData OrderInfo = PopOrder(lOrderID);

	if (OrderInfo.lOrderID == 0) {
		// LG("Store_msg", "AcceptGMRecv:æ‰¾ä¸åˆ°è®¢å•[ID:%lld]!\n", lOrderID);
		LG("Store_msg", "AcceptGMRecv:cannot find order form[ID:%lld]!\n", lOrderID);
		return false;
	}

	CCharacter* pCha = nullptr;
	CPlayer* pPlayer = g_pGameApp->GetPlayerByDBID(OrderInfo.ChaID);
	if (pPlayer) {
		pCha = pPlayer->GetMainCha();
	}

	if (pCha) {
		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_GM_MAIL);
		WRITE_STRING(WtPk, pMi->title);
		WRITE_STRING(WtPk, pMi->description);
		WRITE_LONG(WtPk, static_cast<int>(pMi->sendTime));
		pCha->ReflectINFof(pCha, WtPk);
		/*pCha->SystemNotice("GMé‚®ä»¶å›žå¤: [é—®é¢˜ID: %d]!", pMi->id);
		LG("Store_data", "[%s]æŽ¥æ”¶GMé‚®ä»¶æˆåŠŸ!\n", pCha->GetName());*/
		pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00093), pMi->id);
		LG("Store_data", "[%s] receive GM mail succeed!\n", pCha->GetName());
	}

	return true;
	T_E
}

BOOL CStoreSystem::CancelGMRecv(long long lOrderID) {
	T_B

	SOrderData OrderInfo = PopOrder(lOrderID);

	if (OrderInfo.lOrderID == 0) {
		// LG("Store_msg", "CancelGMRecv:æ‰¾ä¸åˆ°è®¢å•[ID:%lld]!\n", lOrderID);
		LG("Store_msg", "CancelGMRecv:cannot find order form[ID:%lld]!\n", lOrderID);
		return false;
	}

	CCharacter* pCha = nullptr;
	CPlayer* pPlayer = g_pGameApp->GetPlayerByDBID(OrderInfo.ChaID);
	if (pPlayer) {
		pCha = pPlayer->GetMainCha();
	}

	if (pCha) {
		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_GM_MAIL);
		// WRITE_STRING(WtPk, "GMæ²¡æœ‰é‚®ä»¶ç»™ä½ !");
		WRITE_STRING(WtPk, "GM do not have mail send to you!");
		WRITE_STRING(WtPk, "");
		WRITE_LONG(WtPk, 0);
		pCha->ReflectINFof(pCha, WtPk);

		// LG("Store_data", "[%s]æŽ¥æ”¶GMé‚®ä»¶å¤±è´¥!\n", pCha->GetName());

		LG("Store_data", "[%s]receive GM mail failed!\n", pCha->GetName());
	}

	return true;
	T_E
}

BOOL CStoreSystem::GetAfficheList() {
	T_B
		// LG("Store_msg", "è¯·æ±‚å…¬å‘Šåˆ—è¡¨!\n");
		LG("Store_msg", "ask for affiche list!\n");
	pNetMessage pNm = new NetMessage();
	BuildNetMessage(pNm, INFO_REQUEST_AFFICHE, 0, 0, 0, nullptr, 0);
	g_gmsvr->GetInfoServer()->SendData(pNm);
	FreeNetMessage(pNm);
	return true;
	T_E
}

BOOL CStoreSystem::SetItemList(void* pItemList, int lNum) {
	try {
		// LG("Store_msg", "è®¾ç½®å•†åŸŽé“å…·åˆ—è¡¨!\n");
		LG("Store_msg", "set store item list!\n");

		ClearItemList();

		int i;
		StoreInfo* pStore = (StoreInfo*)pItemList;
		ItemInfo* pItem = (ItemInfo*)(pStore + 1);
		for (i = 0; i < lNum; i++) {
			int lComID = pStore->comID;
			int lClsID = pStore->comClass;
			int lItemNum = pStore->itemNum;
			time_t lComTime = pStore->comTime;

			// åˆ†é…å•†å“èŠ‚ç‚¹
			SItemData ItemNode;
			memcpy(&ItemNode.store_head, pStore, sizeof(StoreInfo));
			if (lItemNum > 0) {
				ItemNode.pItemArray = new ItemInfo[lItemNum];
				memcpy(ItemNode.pItemArray, pItem, lItemNum * sizeof(ItemInfo));
			} else
				ItemNode.pItemArray = nullptr;

			// æ’å…¥å•†å“åˆ—è¡¨
			map<int, vector<SItemData>>::iterator map_it = m_ItemList.find(lClsID);
			if (map_it != m_ItemList.end()) {
				(*map_it).second.push_back(ItemNode);
			} else {
				vector<SItemData> vecItem;
				vecItem.push_back(ItemNode);
				pair<int, vector<SItemData>> MapNode(lClsID, vecItem);
				m_ItemList.insert(MapNode);
			}

			pair<int, int> SearchNode(lComID, lClsID);
			m_ItemSearchList.insert(SearchNode);

			pStore = (StoreInfo*)(pItem + lItemNum);
			pItem = (ItemInfo*)(pStore + 1);
		}

		// for test
		// LG("Store_info", "å•†åŸŽå•†å“:\n");
		LG("Store_info", "store merchandise:\n");
		vector<ClassInfo>::iterator cls_it = m_ItemClass.begin();
		{
			while (cls_it != m_ItemClass.end()) {
				short sClsID = (*cls_it).clsID;
				map<int, vector<SItemData>>::iterator itemList_it = m_ItemList.find(sClsID);
				if (itemList_it != m_ItemList.end()) {
					vector<SItemData>::iterator item_it = m_ItemList[sClsID].begin();
					while (item_it != m_ItemList[sClsID].end()) {
						LG("Store_info", "\t[comID:%d]\t[comName:%s]\t[comClass:%d]\t[comPrice:%d]\t[itemNum:%d]\n", (*item_it).store_head.comID, (*item_it).store_head.comName, (*item_it).store_head.comClass, (*item_it).store_head.comPrice, (*item_it).store_head.itemNum);
						ItemInfo* pItemIt = (*item_it).pItemArray;
						int i;
						for (i = 0; i < (*item_it).store_head.itemNum; i++) {
							LG("Store_info", "\t\t[itemID:%d]\t[itemNum:%d]\n", pItemIt->itemID, pItemIt->itemNum);
							pItemIt++;
						}
						item_it++;
					}
				}
				cls_it++;
			}
		}
		LG("Store_info", "\n");
	} catch (std::exception& e) {
		LG("Store_error", "CStoreSystem::SetItemList() %s!\n", e.what());
	} catch (...) {
		// LG("Store_error", "CStoreSystem::SetItemList() æœªçŸ¥å¼‚å¸¸!\n");
		LG("Store_error", "CStoreSystem::SetItemList() unknown abnormity!\n");
	}

	return true;
}

BOOL CStoreSystem::ClearItemList() {
	T_B
		// Fixed: Memory leak - free pItemArray before clearing
		map<int, vector<SItemData>>::iterator cls_it = m_ItemList.begin();
		while (cls_it != m_ItemList.end()) {
			vector<SItemData>::iterator item_it = (*cls_it).second.begin();
			while (item_it != (*cls_it).second.end()) {
				if ((*item_it).pItemArray) {
					delete[] (*item_it).pItemArray;
					(*item_it).pItemArray = nullptr;
				}
				item_it++;
			}
			cls_it++;
		}
		m_ItemList.clear();
	m_ItemSearchList.clear();
	return true;
	T_E
}

BOOL CStoreSystem::SetItemClass(ClassInfo* pClassList, int lNum) {
	T_B
		// LG("Store_msg", "è®¾ç½®å•†åŸŽé“å…·åˆ†ç±»!\n");
		LG("Store_msg", "set store item sort!\n");
	ClearItemClass();
	while (lNum-- > 0) {
		m_ItemClass.push_back(*pClassList);
		pClassList++;
	}

	// for test
	// LG("Store_info", "å•†åŸŽåˆ†ç±»:\n");
	LG("Store_info", "store sort:\n");
	vector<ClassInfo>::iterator it = m_ItemClass.begin();
	while (it != m_ItemClass.end()) {
		LG("Store_info", "\t[clsID:%d]\t[clsName:%s]\t[parentID:%d]\n", (*it).clsID, (*it).clsName, (*it).parentID);
		it++;
	}
	LG("Store_info", "\n");

	return true;
	T_E
}

BOOL CStoreSystem::ClearItemClass() {
	T_B
		m_ItemClass.clear();
	return true;
	T_E
}

BOOL CStoreSystem::SetAfficheList(AfficheInfo* pAfficheList, int lNum) {
	T_B
		// LG("Store_msg", "è®¾ç½®å•†åŸŽå…¬å‘Šåˆ—è¡¨!\n");
		LG("Store_msg", "set stroe affiche list!\n");
	ClearAfficheList();
	while (lNum > 0) {
		m_AfficheList.push_back(*pAfficheList);
		lNum--;
		pAfficheList++;
	}

	// for test
	// LG("Store_info", "å•†åŸŽå…¬å‘Š:\n");
	LG("Store_info", "store affiche:\n");
	vector<AfficheInfo>::iterator it = m_AfficheList.begin();
	while (it != m_AfficheList.end()) {
		LG("Store_info", "\t[affiID:%d]\t[affiTitle:%s]\t[comID:%s]\n", (*it).affiID, (*it).affiTitle, (*it).comID);
		it++;
	}
	LG("Store_info", "\n");

	return true;
	T_E
}

BOOL CStoreSystem::ClearAfficheList() {
	T_B
		m_AfficheList.clear();
	return true;
	T_E
}

BOOL CStoreSystem::Open(CCharacter* pCha, int vip) {
	T_B char bValid;
	int lAfficheNum;
	int lClsNum;
	if (!IsValid()) {
		bValid = 0;
		lAfficheNum = 0;
		lClsNum = 0;
	} else {
		bValid = 1;
		lAfficheNum = (int)m_AfficheList.size();
		lClsNum = (int)m_ItemClass.size();
	}

	int i;
	if (bValid == 1) {
		pCha->ForgeAction();
		pCha->SetStoreEnable(true);
	}
	WPACKET WtPk = GETWPACKET();
	WRITE_CMD(WtPk, CMD_MC_STORE_OPEN_ASR);
	WRITE_CHAR(WtPk, bValid); // å•†åŸŽæ˜¯å¦å¼€å¯

	if (bValid == 1) {
		WRITE_LONG(WtPk, vip);
		WRITE_LONG(WtPk, pCha->GetPlayer()->GetMoBean());	// æ‘©è±†
		WRITE_LONG(WtPk, pCha->GetPlayer()->GetRplMoney()); // å…ƒå®

		WRITE_LONG(WtPk, lAfficheNum); // å…¬å‘Šæ•°é‡
		for (i = 0; i < lAfficheNum; i++) {
			WRITE_LONG(WtPk, m_AfficheList[i].affiID);
			WRITE_SEQ(WtPk, m_AfficheList[i].affiTitle, uShort(strlen(m_AfficheList[i].affiTitle) + 1));
			WRITE_SEQ(WtPk, m_AfficheList[i].comID, uShort(strlen(m_AfficheList[i].comID) + 1));
		}
		WRITE_LONG(WtPk, lClsNum); // åˆ†ç±»æ•°é‡
		for (i = 0; i < lClsNum; i++) {
			WRITE_SHORT(WtPk, m_ItemClass[i].clsID);
			WRITE_SEQ(WtPk, m_ItemClass[i].clsName, uShort(strlen(m_ItemClass[i].clsName) + 1));
			WRITE_SHORT(WtPk, m_ItemClass[i].parentID);
		}
	}
	pCha->ReflectINFof(pCha, WtPk);

	if (bValid != 1) {
		// pCha->SystemNotice("å†…ç½®å•†åŸŽè¿˜æœªå¼€å¼ ,æ­£åœ¨æ‰“å¼€å•†åŸŽç½‘é¡µ!");
		pCha->SystemNotice(RES_STRING(GM_CHARTRADE_CPP_00094));
	}

	return true;
	T_E
}

} // namespace mission
