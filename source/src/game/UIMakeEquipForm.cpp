#include "StdAfx.h"

#include "UIMakeEquipForm.h"
#include "UIFormMgr.h"
#include "uiform.h"
#include "UIFastCommand.h"
#include "UIEquipForm.h"
#include "UILabel.h"
#include "UIItemCommand.h"
#include "UIGoodsGrid.h"
#include "UIMemo.h"
#include "PacketCmd.h"
#include "GameApp.h"
#include "UIProgressBar.h"
#include "WorldScene.h"
#include "Character.h"
#include "UIBoxForm.h"
#include "StringLib.h"
#include "ItemRecord.h"

using namespace std;

namespace GUI {

//-----------------------------------------------------------------------------
bool CMakeEquipMgr::Init() {
	CFormMgr& mgr = CFormMgr::s_Mgr;

	frmMakeEquip = mgr.Find("frmMakeEquip");
	if (!frmMakeEquip) {
		LG("gui", RES_STRING(CL_LANGUAGE_MATCH_685));
		return false;
	}
	frmMakeEquip->evtEntrustMouseEvent = _MainMouseEvent;
	frmMakeEquip->evtClose = _OnClose;

	cmdRouleau = dynamic_cast<COneCommand*>(frmMakeEquip->Find("cmdRouLeau"));
	if (!cmdRouleau) {
		return Error(RES_STRING(CL_LANGUAGE_MATCH_561),
					 frmMakeEquip->GetName(),
					 "cmdRouleau");
	}
	cmdRouleau->evtBeforeAccept = _DragEvtRouleau;

	cmdLastEquip = dynamic_cast<COneCommand*>(frmMakeEquip->Find("cmdLastEquip"));
	if (!cmdLastEquip) {
		return Error(RES_STRING(CL_LANGUAGE_MATCH_561),
					 frmMakeEquip->GetName(),
					 "cmdForgeItem");
	}

	char szBuf[32];
	for (int i(0); i < ITEM_NUM; i++) {
		sprintf(szBuf, "cmdItem%d", i);
		cmdItem[i] = dynamic_cast<COneCommand*>(frmMakeEquip->Find(szBuf));
		if (!cmdItem[i])
			return Error(RES_STRING(CL_LANGUAGE_MATCH_561),
						 frmMakeEquip->GetName(),
						 szBuf);
	}
	cmdItem[0]->evtBeforeAccept = _DragEvtEquipItem0;
	cmdItem[1]->evtBeforeAccept = _DragEvtEquipItem1;
	cmdItem[2]->evtBeforeAccept = _DragEvtEquipItem2;
	cmdItem[3]->evtBeforeAccept = _DragEvtEquipItem3;

	// Register double-click handlers
	cmdRouleau->evtUseCommand = _evtSlotUseEvent;
	for (int i = 0; i < ITEM_NUM; i++) {
		cmdItem[i]->evtUseCommand = _evtSlotUseEvent;
	}
	g_stUIEquip.GetGoodsGrid()->evtUseCommand = _evtInventoryUseEvent;

	labForgeGold = dynamic_cast<CLabel*>(frmMakeEquip->Find("labForgeGold"));
	if (!labForgeGold) {
		return Error(RES_STRING(CL_LANGUAGE_MATCH_561),
					 frmMakeEquip->GetName(),
					 "labForgeGold");
	}

	memForgeItemState = dynamic_cast<CMemo*>(frmMakeEquip->Find("memForgeItemState"));
	if (!memForgeItemState)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_561), frmMakeEquip->GetName(), "memForgeItemState");

	btnYes = dynamic_cast<CTextButton*>(frmMakeEquip->Find("btnForgeYes"));
	if (!btnYes)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_561), frmMakeEquip->GetName(), "btnForgeYes");

	return true;
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::_evtInventoryUseEvent(CCommandObj* pSender, bool& isUse) {
	if (!g_stUIMakeEquip.frmMakeEquip || !g_stUIMakeEquip.frmMakeEquip->GetIsShow()) {
		return;
	}

	CItemCommand* pItemCommand = dynamic_cast<CItemCommand*>(pSender);
	if (!pItemCommand || !pItemCommand->GetIsValid()) {
		return;
	}

	int itemIndex = pItemCommand->GetIndex();
	if (itemIndex < 0) return;

	CItemRecord* pItemInfo = pItemCommand->GetItemInfo();
	if (!pItemInfo) return;

	// Check if it's the correct rouleau/recipe for the current form type
	bool isCorrectRouleau = false;
	int currentFormType = g_stUIMakeEquip.GetType();
	
	switch (currentFormType) {
	case MAKE_EQUIP_TYPE:
		isCorrectRouleau = g_stUIMakeEquip.IsEquipMakeRouleau(*pItemCommand);
		break;
	case EQUIP_FUSION_TYPE:
		isCorrectRouleau = g_stUIMakeEquip.IsEquipFusionRouleau(*pItemCommand);
		break;
	case EQUIP_UPGRADE_TYPE:
		isCorrectRouleau = g_stUIMakeEquip.IsEquipUpgradeRouleau(*pItemCommand);
		break;
	case ELF_SHIFT_TYPE:
		isCorrectRouleau = g_stUIMakeEquip.IsElfShiftStone(*pItemCommand);
		break;
	}
	
	if (isCorrectRouleau) {
		if (!g_stUIMakeEquip.cmdRouleau->GetCommand()) {
			g_stUIEquip.GetGoodsGrid()->SetDragIndex(itemIndex);
			g_stUIMakeEquip.PushRouleau(*pItemCommand);
			isUse = false;
		}
		return;
	}

	// Must have rouleau first before placing other items
	if (!g_stUIMakeEquip.cmdRouleau->GetCommand()) {
		return; // No rouleau yet, can't place items
	}

	// Determine target slot and send validation request to server
	char chFormType = (char)g_stUIMakeEquip.GetType();
	char chSlotIndex = -1;

	switch (chFormType) {
	case MAKE_EQUIP_TYPE:
		if (g_stUIMakeEquip.IsMakeGem()) {
			// Gem combining - find first empty slot
			for (int i = 0; i < CMakeEquipMgr::ITEM_NUM; i++) {
				if (!g_stUIMakeEquip.cmdItem[i]->GetCommand()) {
					chSlotIndex = i;
					break;
				}
			}
		}
		break;

	case EQUIP_FUSION_TYPE:
		// Fusion has specific slots: 0=Appearance, 1=Equipment, 2=Catalyzer
		// Let server decide which slot based on item type
		if (g_stUIMakeEquip.IsEquipFusionCatalyzer(*pItemCommand)) {
			if (!g_stUIMakeEquip.cmdItem[2]->GetCommand()) {
				chSlotIndex = 2;
			}
		} else {
			// Try slot 0 first, then slot 1
			if (!g_stUIMakeEquip.cmdItem[0]->GetCommand()) {
				chSlotIndex = 0;
			} else if (!g_stUIMakeEquip.cmdItem[1]->GetCommand()) {
				chSlotIndex = 1;
			}
		}
		break;

	case EQUIP_UPGRADE_TYPE:
		// Upgrade has specific slots: 0=Fusion Equipment, 1=Upgrade Spar
		if (g_stUIMakeEquip.IsEquipUpgradeSpar(*pItemCommand)) {
			if (!g_stUIMakeEquip.cmdItem[1]->GetCommand()) {
				chSlotIndex = 1;
			}
		} else {
			if (!g_stUIMakeEquip.cmdItem[0]->GetCommand()) {
				chSlotIndex = 0;
			}
		}
		break;

	case ELF_SHIFT_TYPE:
		// Elf shift - find first empty slot
		for (int i = 0; i < CMakeEquipMgr::ITEM_NUM; i++) {
			if (!g_stUIMakeEquip.cmdItem[i]->GetCommand()) {
				chSlotIndex = i;
				break;
			}
		}
		break;
	}

	// If we found a valid slot, send validation request
	if (chSlotIndex >= 0) {
		CS_ValidateSlotItem(chFormType, chSlotIndex, (short)itemIndex);
		isUse = false; // Prevent normal use while waiting for validation
	}
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::_evtSlotUseEvent(CCommandObj* pSender, bool& isUse) {
	isUse = false;

	if (g_stUIMakeEquip.cmdRouleau->GetCommand() == pSender) {
		g_stUIMakeEquip.PopRouleau();
		return;
	}

	for (int i = 0; i < CMakeEquipMgr::ITEM_NUM; i++) {
		if (g_stUIMakeEquip.cmdItem[i]->GetCommand() == pSender) {
			if (g_stUIMakeEquip.IsMakeGem()) {
				g_stUIMakeEquip.PopGemItem(i);
			} else {
				g_stUIMakeEquip.ClearEquipList(i);
			}
			return;
		}
	}
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::CloseForm() {
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::SwitchMap() {
	this->Clear();
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::ShowMakeEquipForm(bool bShow) {
	this->Clear();

	if (bShow) {
		// Register double-click handler for inventory
		g_stUIEquip.GetGoodsGrid()->evtUseCommand = _evtInventoryUseEvent;

		frmMakeEquip->SetPos(150, 150);
		frmMakeEquip->Reset();
		frmMakeEquip->Refresh();
		frmMakeEquip->Show();

		// ??????????
		int x = frmMakeEquip->GetX() + frmMakeEquip->GetWidth();
		int y = frmMakeEquip->GetY();
		g_stUIEquip.GetItemForm()->SetPos(x, y);
		g_stUIEquip.GetItemForm()->Refresh();

		if (!(m_isOldEquipFormShow = g_stUIEquip.GetItemForm()->GetIsShow())) {
			g_stUIEquip.GetItemForm()->Show();
		}
	} else {
		frmMakeEquip->Close();
		g_stUIEquip.GetItemForm()->SetIsShow(m_isOldEquipFormShow);
	}

	return;
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::ShowConfirmDialog(long lMoney) {
	char szBuf[255] = {0};
	sprintf(szBuf, RES_STRING(CL_LANGUAGE_MATCH_568), lMoney);
	GUI::stSelectBox* pBox = g_stUIBox.ShowSelectBox(_evtConfirmEvent, szBuf, true);
	pBox->frmDialog->evtEscClose = _evtConfirmCancelEvent;
}

//-----------------------------------------------------------------------------
bool CMakeEquipMgr::IsRouleauCommand(COneCommand* oneCommand) {
	return (oneCommand == cmdRouleau);
}

//-----------------------------------------------------------------------------
bool CMakeEquipMgr::IsAllCommand(COneCommand* oneCommand) {
	if (oneCommand == cmdRouleau)
		return true;
	else if (oneCommand == cmdRouleau)
		return true;
	else
		for (int i(0); i < ITEM_NUM; i++)
			if (cmdItem[i] == oneCommand)
				return true;

	return false;
}

int CMakeEquipMgr::GetItemComIndex(COneCommand* oneCommand) {
	for (int i(0); i < ITEM_NUM; i++)
		if (cmdItem[i] == oneCommand)
			return i;
	return -1;
}

void CMakeEquipMgr::PopGemItem(int iIndex) {
	PopItem(iIndex);
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::DragRouleauToEquipGrid() {
	this->PopRouleau();
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::DragItemToEquipGrid(int iIndex) {
	switch (m_iType) {
	case MAKE_EQUIP_TYPE:
		if (this->IsMakeGem()) {
			PopGemItem(iIndex);
		} else {
			PopEquipItem(iIndex);
		}
		break;
	case EQUIP_FUSION_TYPE:
		if (iIndex == 0) { // ????,??????
			PopItem(iIndex);

			CItemCommand* pEquipItemCommand =
				dynamic_cast<CItemCommand*>(cmdItem[1]->GetCommand());
			if (pEquipItemCommand) {
				PopItem(1);
			}
		} else if (iIndex == 1) { // ????
			PopItem(iIndex);
		} else if (iIndex == 2) { // ?????
			PopItem(iIndex);
		}
		break;
	case EQUIP_UPGRADE_TYPE:
		PopItem(iIndex);
		break;
	}
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::MakeEquipSuccess(long lChaID) {
	if (!CGameApp::GetCurScene())
		return;

	CCharacter* pCha = CGameApp::GetCurScene()->SearchByID(lChaID);
	if (!pCha)
		return;

	pCha->SelfEffect(FORGE_SUCCESS_EFF_ID);

	if (pCha->IsMainCha())
		this->ShowMakeEquipForm(false);
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::MakeEquipFailed(long lChaID) {
	if (!CGameApp::GetCurScene())
		return;

	CCharacter* pCha = CGameApp::GetCurScene()->SearchByID(lChaID);
	if (!pCha)
		return;

	pCha->SelfEffect(FORGE_FAILED_EFF_ID);

	if (pCha->IsMainCha())
		this->ShowMakeEquipForm(false);
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::MakeEquipOther(long lChaID) {
	this->ShowMakeEquipForm(false);
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::OnValidateSlotSuccess(char chFormType, char chSlotIndex, short sGridID) {
	// Get the item from inventory and place it in the validated slot
	CItemCommand* pItemCommand = dynamic_cast<CItemCommand*>(g_stUIEquip.GetGoodsGrid()->GetItem(sGridID));
	if (!pItemCommand || !pItemCommand->GetIsValid()) {
		return;
	}

	// Set the drag index for PushItem to use
	g_stUIEquip.GetGoodsGrid()->SetDragIndex(sGridID);

	// Place the item based on form type and slot
	switch (chFormType) {
	case MAKE_EQUIP_TYPE:
		if (IsMakeGem()) {
			if (CanPushStone(chSlotIndex, *pItemCommand)) {
				PushGemItem(chSlotIndex, *pItemCommand);
				SetMakeEquipUI();
			}
		}
		break;
		
	case EQUIP_FUSION_TYPE:
		PushEquipFusionItem(chSlotIndex, *pItemCommand);
		SetMakeEquipUI();
		break;
		
	case EQUIP_UPGRADE_TYPE:
		PushEquipUpgradeItem(chSlotIndex, *pItemCommand);
		SetMakeEquipUI();
		break;
		
	case ELF_SHIFT_TYPE:
		PushElfShiftItem(chSlotIndex, *pItemCommand);
		SetMakeEquipUI();
		break;
	}
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::PushEquipItem(int iIndex, CItemCommand& rItem) {
	CItemCommand* pOldItemCommand =
		dynamic_cast<CItemCommand*>(cmdItem[iIndex]->GetCommand());
	if (!pOldItemCommand)
		return;

	int iNum(0), iPos(-1);
	// ?????COneCommand???
	if (pOldItemCommand->GetIsPile()) {
	} else {
		iNum = 1;
	}

	// int iPos = g_stUIEquip.GetGoodsGrid()->GetDragIndex();
	//// ?????EquipList
	// EquipInfo* pEquipInfo = new EquipInfo();
	// pEquipInfo->iPos = iPos;
	// pEquipInfo->iNum = iNum;
	// equipItems[iIndex].push_back(pEquipInfo);

	return;
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::PopEquipItem(int iIndex) {
	// ????COneCommand???

	// ?????EquipList
	ClearEquipList(iIndex);
}
//-----------------------------------------------------------------------------
void CMakeEquipMgr::PushEquipFusionItem(int iIndex, CItemCommand& rItem) {
	if (iIndex == 0) {
		if (IsAppearanceEquip(rItem)) {
			PushItem(iIndex, rItem, 1);
		} else {
			g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_686));
			return;
		}
	} else if (iIndex == 1) {
		CItemCommand* pItemCommand =
			dynamic_cast<CItemCommand*>(cmdItem[0]->GetCommand());
		if (!pItemCommand) {
			g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_687));
			return;
		}

		// modify by Philip.Wu  2006-06-11
		// ???,??????? 27,????????? 22 ?,??????
		if ((pItemCommand->GetItemInfo()->sType == 27 && rItem.GetItemInfo()->sType == 22) ||
			(IsSameAppearEquip(rItem, *pItemCommand))) {
			PushItem(iIndex, rItem, 1);
		} else {
			g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_688));
			return;
		}
	} else if (iIndex == 2) {
		if (IsEquipFusionCatalyzer(rItem)) {
			PushItem(iIndex, rItem, 1);
		} else {
			g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_689));
			return;
		}
	}
}
//-----------------------------------------------------------------------------
void CMakeEquipMgr::PopEquipFusionItem(int iIndex) {
	PopItem(iIndex);
}
//-----------------------------------------------------------------------------
void CMakeEquipMgr::PushEquipUpgradeItem(int iIndex, CItemCommand& rItem) {
	if (iIndex == 0) {
		if (IsFusionEquip(rItem)) {
			PushItem(iIndex, rItem, 1);
		} else {
			g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_690));
			return;
		}
	} else if (iIndex == 1) {
		if (IsEquipUpgradeSpar(rItem)) {
			PushItem(iIndex, rItem, 1);
		} else {
			g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_691));
			return;
		}
	}
}
//-----------------------------------------------------------------------------
void CMakeEquipMgr::PopEquipUpgradeItem(int iIndex) {
	PopItem(iIndex);
}
//-----------------------------------------------------------------------------
void CMakeEquipMgr::PushRouleau(CItemCommand& rItem) {
	// ??????????,?????????
	CItemCommand* pItemCommand =
		dynamic_cast<CItemCommand*>(cmdRouleau->GetCommand());
	if (pItemCommand) {
		if (pItemCommand->GetItemInfo()->lID == rItem.GetItemInfo()->lID) {
			return;
		} else {
			PopRouleau();
		}
	}

	// ????????????
	m_iRouleauPos = g_stUIEquip.GetGoodsGrid()->GetDragIndex();

	// ???????????
	rItem.SetIsValid(false);
	// ????Command??
	CItemCommand* pItemCmd = new CItemCommand(rItem);
	cmdRouleau->AddCommand(pItemCmd);
	pItemCmd->SetIsValid(true);

	// ???????COneCommand???,????????,?????????
	if (rItem.GetItemInfo()->sType == GEM_ROULEAU_TYPE) { // ????
		PushNewGems();
	} else { // ????
		PushNewEquips(*(rItem.GetItemInfo()));
	}

	this->SetMakeEquipUI();

	return;
}

void CMakeEquipMgr::PushNewGems() {
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::PopRouleau() {
	// ????COneCommand???
	if (m_iRouleauPos == -1)
		return;

	CCommandObj* pCmdObj = g_stUIEquip.GetGoodsGrid()->GetItem(m_iRouleauPos);
	if (pCmdObj)
		pCmdObj->SetIsValid(true);

	// ?????Command (DelCommand()???delete??)
	cmdRouleau->DelCommand();

	// ??????COneCommand?????
	for (int i(0); i < ITEM_NUM; ++i)
		PopItem(i);

	// ?????????
	PopLastEquip();

	this->SetMakeEquipUI();
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::PushLastEquip(CItemCommand& rItem) {
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::PopLastEquip() {
	CCommandObj* pCom = cmdLastEquip->GetCommand();
	if (pCom) {
		cmdLastEquip->DelCommand();
	}
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::ClearEquipList(int iIndex) {
	EquipListIter iter = equipItems[iIndex].begin();
	EquipListIter end = equipItems[iIndex].end();
	for (; iter != end; ++iter) {
		// delete (*iter);
		SAFE_DELETE(*iter); // UI????
	}
	equipItems[iIndex].clear();
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::Clear() {
	// ??UI????
	labForgeGold->SetCaption("");
	btnYes->SetIsEnabled(false);

	// ????(???????Item)
	PopRouleau();
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::ClearEquips() {
	// ????????????
	EquipListIter iter, end;
	for (int i(0); i < ITEM_NUM; ++i) {
		ClearEquipList(i);
	}

	return;
}

bool CMakeEquipMgr::CanPushEquip(int iIndex, CItemCommand& rItem) {
	return true;
}

//-----------------------------------------------------------------------------
bool CMakeEquipMgr::CanPushStone(int iIndex, CItemCommand& rItem) {
	CItemRecord* pItemRecord = rItem.GetItemInfo();
	if (!pItemRecord)
		return false;

	//  ????????????false
	if (pItemRecord->sType != GEN_STONE_TYPE && pItemRecord->sType != FORGE_STONE_TYPE) {
		g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_692));
		return false;
	}

	// ????????????
	int iOtherIndex = iIndex == 0 ? 1 : 0;
	CItemCommand* pOtherItem = dynamic_cast<CItemCommand*>(cmdItem[iOtherIndex]->GetCommand());
	if (pOtherItem) { // ????????,???ID??????true
		CItemRecord* pOtherItemRecord = pOtherItem->GetItemInfo();
		if (pItemRecord->lID == pOtherItemRecord->lID) {
			return true;
		} else {
			g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_693));
			return false;
		}
	} else // ?????????,????true
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::PushItem(int iIndex, CItemCommand& rItem, int iItemNum) {
	// ?????Cmd??????Item?,??????
	CItemCommand* pItemCommand =
		dynamic_cast<CItemCommand*>(cmdItem[iIndex]->GetCommand());
	if (pItemCommand) {
		PopItem(iIndex);
	}

	if (iItemNum == 1) {
		// ??Item????????
		EquipInfo* pEquipInfo = new EquipInfo();
		pEquipInfo->iPos = g_stUIEquip.GetGoodsGrid()->GetDragIndex();
		pEquipInfo->iNum = iItemNum;
		equipItems[iIndex].resize(1);
		equipItems[iIndex][0] = pEquipInfo;

		// ?Item????????
		rItem.SetIsValid(false);

		// ????Item??Cmd?,???new???PopItem()???
		CItemCommand* pItemCmd = new CItemCommand(rItem);
		pItemCmd->SetIsValid(true);
		cmdItem[iIndex]->AddCommand(pItemCmd);
	}
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::PushNewEquips(CItemRecord& rRouleauRecord) {
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::PopItem(int iIndex) {
	// ??Cmd??Item,?Item??PushItem()??new??
	CItemCommand* pItemCommand =
		dynamic_cast<CItemCommand*>(cmdItem[iIndex]->GetCommand());
	if (pItemCommand)
		cmdItem[iIndex]->DelCommand(); // ??????delete Item

	// ?Item????????
	CCommandObj* pItem(0);
	EquipListIter iter = equipItems[iIndex].begin();
	EquipListIter end = equipItems[iIndex].end();
	for (; iter != end; ++iter) {
		pItem = g_stUIEquip.GetGoodsGrid()->GetItem((*iter)->iPos);
		if (pItem) {
			pItem->SetIsValid(true);
		}
	}

	ClearEquipList(iIndex);

	this->SetMakeEquipUI();

	return;
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::PushGemItem(int iIndex, CItemCommand& rItem) {
	if (iIndex < STONE_ITEM_NUM) {
		if (this->CanPushStone(iIndex, rItem)) {
			// ?????Cmd??????Item?,??????
			CItemCommand* pItemCommand =
				dynamic_cast<CItemCommand*>(cmdItem[iIndex]->GetCommand());
			if (pItemCommand) {
				PopItem(iIndex);
			}

			// ??Item????????
			EquipInfo* pEquipInfo = new EquipInfo();
			pEquipInfo->iPos = g_stUIEquip.GetGoodsGrid()->GetDragIndex();
			pEquipInfo->iNum = 1;
			equipItems[iIndex].push_back(pEquipInfo);

			// ?Item????????
			rItem.SetIsValid(false);

			// ????Item??Cmd?,???new???PopItem()???
			CItemCommand* pItemCmd = new CItemCommand(rItem);
			pItemCmd->SetIsValid(true);
			cmdItem[iIndex]->AddCommand(pItemCmd);
		}
	}
	this->SetMakeEquipUI();
}
//-----------------------------------------------------------------------------
bool CMakeEquipMgr::IsEquipMakeRouleau(CItemCommand& rItem) {
	CItemRecord* pItemRecord = rItem.GetItemInfo();

	if (pItemRecord && pItemRecord->sType == GEM_ROULEAU_TYPE || pItemRecord->sType == EQUIP_ROULEAU_TYPE) {
		return true;
	}
	return false;
}
//-----------------------------------------------------------------------------
bool CMakeEquipMgr::IsEquipFusionRouleau(CItemCommand& rItem) {
	CItemRecord* pItemRecord = rItem.GetItemInfo();

	if (pItemRecord && pItemRecord->sType == EQUIP_FUSION_ROULEAU_TYPE) {
		return true;
	}
	return false;
}
//-----------------------------------------------------------------------------
bool CMakeEquipMgr::IsEquipUpgradeRouleau(CItemCommand& rItem) {
	CItemRecord* pItemRecord = rItem.GetItemInfo();

	if (pItemRecord && pItemRecord->sType == EQUIP_UPGRADE_ROULEAU_TYPE) {
		return true;
	}
	return false;
}
//-----------------------------------------------------------------------------
bool CMakeEquipMgr::IsAppearanceEquip(CItemCommand& rItem) {
	CItemRecord* pItemRecord = rItem.GetItemInfo();

	return CItemRecord::IsVaildFusionID(pItemRecord);

	// if (pItemRecord && pItemRecord->lID > APPEAR_EQUIP_BASE_ID)
	// if (pItemRecord &&
	//	(pItemRecord->lID >= CItemRecord::enumItemFusionStart && pItemRecord->lID < CItemRecord::enumItemFusionEnd) /*&&
	//	rItem.GetData().GetFusionItemID() > 0*/ )
	//{
	//	return true;
	// }
	// return false;
}
//-----------------------------------------------------------------------------
bool CMakeEquipMgr::IsEquipFusionCatalyzer(CItemCommand& rItem) {
	CItemRecord* pItemRecord = rItem.GetItemInfo();

	if (pItemRecord && pItemRecord->sType == EQUIP_FUSION_CATALYZER_TYPE) {
		return true;
	}
	return false;
}
//-----------------------------------------------------------------------------
bool CMakeEquipMgr::IsSameAppearEquip(CItemCommand& rEquipItem, CItemCommand& rAppearItem) {
	CItemRecord* pEquipRecord = rEquipItem.GetItemInfo();
	CItemRecord* pAppearRecord = rAppearItem.GetItemInfo();

	// comment by Philip.Wu  2006-08-15  ??????????????
	// if (IsAppearanceEquip(rEquipItem))
	//{
	//	return false;
	//}

	if (pEquipRecord && pAppearRecord && pEquipRecord->sType == pAppearRecord->sType) {
		return true;
	}

	return false;
}
//---------------------------------------------------------------------
bool CMakeEquipMgr::IsEquipUpgradeSpar(CItemCommand& rItem) {
	CItemRecord* pItemRecord = rItem.GetItemInfo();

	if (pItemRecord && pItemRecord->sType == EQUIP_UPGRADE_SPAR) {
		return true;
	}
	return false;
}
//---------------------------------------------------------------------
bool CMakeEquipMgr::IsFusionEquip(CItemCommand& rItem) {
	// CItemRecord* pItemRecord = rItem.GetItemInfo();
	// SItemGrid& rItemData =rItem.GetData();
	// if ((pItemRecord->lID >= CItemRecord::enumItemFusionStart && pItemRecord->lID < CItemRecord::enumItemFusionEnd)
	//	&& rItemData.GetFusionItemID() > 0)
	//{
	//	return true;
	// }
	// return false;
	CItemRecord* pItemRecord = rItem.GetItemInfo();

	if (pItemRecord) {
		short sType = pItemRecord->sType;
		//	Close by alfred.shi 20080912 ???????

		switch (sType) {
		case enumItemTypeSword:
		case enumItemTypeGlave:
		case enumItemTypeBow:
		case enumItemTypeHarquebus:
		case enumItemTypeStylet:
		case enumItemTypeCosh:
		case enumItemTypeShield:
		case enumItemTypeHair:
		case enumItemTypeClothing:
		case enumItemTypeGlove:
		case enumItemTypeBoot:
		case 25:
		case 26:
		case 27:
		case 81:
		case 82:
		case 83:
			return true;
		default:
			return false;
		}
	}

	return false;
}
//-----------------------------------------------------------------------------
// ????
//-----------------------------------------------------------------------------
void CMakeEquipMgr::_MainMouseEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	string name = pSender->GetName();
	if (name == "btnClose" || name == "btnForgeNo") { /// ????

		g_stUIMakeEquip.ShowMakeEquipForm(false);
		return;
	} else if (name == "btnForgeYes") {
		if (g_stUIMakeEquip.m_iType == EQUIP_FUSION_TYPE) {
			if (!g_stUIMakeEquip.cmdItem[2]->GetCommand()) {
				g_stUIBox.ShowSelectBox(_evtFusionNoCatalyzerConfirmEvent,
										RES_STRING(CL_LANGUAGE_MATCH_694),
										true);
			} else {
				g_stUIMakeEquip.SendMakeEquipProtocol();
			}
		} else {
			g_stUIMakeEquip.SendMakeEquipProtocol();
		}
	}

	return;
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::_DragEvtRouleau(CGuiData* pSender, CCommandObj* pItem, bool& isAccept) {
	isAccept = false;

	CGoodsGrid* pGood = dynamic_cast<CGoodsGrid*>(CDrag::GetParent());
	if (pGood != g_stUIEquip.GetGoodsGrid())
		return;

	CItemCommand* pItemCommand = dynamic_cast<CItemCommand*>(pItem);
	if (!pItemCommand)
		return;
	if (!(pItemCommand->GetIsValid()))
		return;

	switch (g_stUIMakeEquip.m_iType) {
	case MAKE_EQUIP_TYPE:
		if (g_stUIMakeEquip.IsEquipMakeRouleau(*pItemCommand)) {
			g_stUIMakeEquip.PushRouleau(*pItemCommand);
			g_stUIMakeEquip.SetMakeEquipUI();
		} else {
			g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_695));
		}
		break;
	case EQUIP_FUSION_TYPE:
		if (g_stUIMakeEquip.IsEquipFusionRouleau(*pItemCommand)) {
			g_stUIMakeEquip.PushRouleau(*pItemCommand);
			g_stUIMakeEquip.SetMakeEquipUI();
		} else {
			g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_696));
		}
		break;
	case EQUIP_UPGRADE_TYPE:
		if (g_stUIMakeEquip.IsEquipUpgradeRouleau(*pItemCommand)) {
			g_stUIMakeEquip.PushRouleau(*pItemCommand);
			g_stUIMakeEquip.SetMakeEquipUI();
		} else {
			g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_697));
		}
		break;
	case ELF_SHIFT_TYPE: // ????
		if (g_stUIMakeEquip.IsElfShiftStone(*pItemCommand)) {
			g_stUIMakeEquip.PushRouleau(*pItemCommand);
			g_stUIMakeEquip.SetMakeEquipUI();
		} else {
			g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_698));
		}
	}

	return;
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::_DragEvtEquipItem0(CGuiData* pSender, CCommandObj* pItem, bool& isAccept) {
	g_stUIMakeEquip.DragEvtEquipItem(0, pSender, pItem, isAccept);
	return;
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::_DragEvtEquipItem1(CGuiData* pSender, CCommandObj* pItem, bool& isAccept) {
	g_stUIMakeEquip.DragEvtEquipItem(1, pSender, pItem, isAccept);
	return;
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::_DragEvtEquipItem2(CGuiData* pSender, CCommandObj* pItem, bool& isAccept) {
	g_stUIMakeEquip.DragEvtEquipItem(2, pSender, pItem, isAccept);
	return;
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::_DragEvtEquipItem3(CGuiData* pSender, CCommandObj* pItem, bool& isAccept) {
	g_stUIMakeEquip.DragEvtEquipItem(3, pSender, pItem, isAccept);
	return;
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::_OnClose(CForm* pForm, bool& IsClose) {
	g_stUIMakeEquip.Clear();
	CS_ItemForgeAsk(false);
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::_evtConfirmEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	CS_ItemForgeAnswer(nMsgType == CForm::mrYes);
}
//-----------------------------------------------------------------------------
void CMakeEquipMgr::_evtConfirmCancelEvent(CForm* pForm) {
	CS_ItemForgeAnswer(false);
	pForm->SetIsShow(false);
}
//---------------------------------------------------------------------
void CMakeEquipMgr::_evtFusionNoCatalyzerConfirmEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	if (nMsgType == CForm::mrYes) {
		g_stUIMakeEquip.SendMakeEquipProtocol();
	}
}

//-----------------------------------------------------------------------------
// ????
//-----------------------------------------------------------------------------
void CMakeEquipMgr::DragEvtEquipItem(int index, CGuiData* pSender, CCommandObj* pItem, bool& isAccept) {
	isAccept = false;

	CGoodsGrid* pGood = dynamic_cast<CGoodsGrid*>(CDrag::GetParent());
	if (pGood != g_stUIEquip.GetGoodsGrid())
		return;

	CItemCommand* pItemCommand = dynamic_cast<CItemCommand*>(pItem);
	if (!pItemCommand)
		return;

	// if (!(pItemCommand->GetIsValid())) return; //removed this line, so you can drag 2x of the same gem in.
	// howver this could lead to issue with apparel, so make sure to check type.
	if (!pItemCommand->GetIsValid()) {
		SItemGrid item = pItemCommand->GetData();
		int sType = pItemCommand->GetItemInfo()->sType;
		if ((sType != GEN_STONE_TYPE && sType != FORGE_STONE_TYPE) || item.sNum == 1) {
			return;
		}
	}

	if (!cmdRouleau->GetCommand()) {
		// by Philip.Wu  ????????,????????????,??????????
		switch (this->m_iType) {
		case MAKE_EQUIP_TYPE:
		case EQUIP_FUSION_TYPE:
		case EQUIP_UPGRADE_TYPE:
			g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_699));
			break;

		case ELF_SHIFT_TYPE:
			g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_700));
			break;

		default:
			g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_701));
			break;
		}

		return;

		// ????????,????
		// if (IsEquipMakeRouleau(*pItemCommand) ||
		//	IsEquipFusionRouleau(*pItemCommand) ||
		//	IsEquipUpgradeRouleau(*pItemCommand))
		//{
		//	g_pGameApp->MsgBox("????????");
		//	return;
		//}
		// else
		//{
		//	g_pGameApp->MsgBox("??????");
		//	return;
		//}
	}

	switch (g_stUIMakeEquip.m_iType) {
	case MAKE_EQUIP_TYPE:
		if (this->IsMakeGem()) {
			PushGemItem(index, *pItemCommand);
		} else {
			PushEquipItem(index, *pItemCommand);
		}
		break;

	case EQUIP_FUSION_TYPE:
		PushEquipFusionItem(index, *pItemCommand);
		break;

	case EQUIP_UPGRADE_TYPE:
		PushEquipUpgradeItem(index, *pItemCommand);
		break;

	case ELF_SHIFT_TYPE:
		PushElfShiftItem(index, *pItemCommand);
		break;
	}

	this->SetMakeEquipUI();

	return;
}

//-----------------------------------------------------------------------------
bool CMakeEquipMgr::IsMakeGem() {
	CItemCommand* pItemCommand =
		dynamic_cast<CItemCommand*>(cmdRouleau->GetCommand());
	if (pItemCommand && pItemCommand->GetItemInfo()->sType == GEM_ROULEAU_TYPE) {
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::SetMakeEquipUI() {
	// memForgeItemState->SetCaption("????????");
	// memForgeItemState->ProcessCaption();
	switch (m_iType) {
	case MAKE_EQUIP_TYPE:
		if (IsMakeGem() && cmdItem[0]->GetCommand() && cmdItem[1]->GetCommand()) {
			labForgeGold->SetCaption(StringSplitNum(CalMakeEquipMoney()));
			btnYes->SetIsEnabled(true);
		} else {
			labForgeGold->SetCaption("");
			btnYes->SetIsEnabled(false);
		}
		break;
	case EQUIP_FUSION_TYPE:
		if (cmdRouleau->GetCommand() && cmdItem[0]->GetCommand() && cmdItem[1]->GetCommand()) {
			labForgeGold->SetCaption(StringSplitNum(CalMakeEquipMoney()));
			btnYes->SetIsEnabled(true);
		} else {
			labForgeGold->SetCaption("");
			btnYes->SetIsEnabled(false);
		}
		break;
	case EQUIP_UPGRADE_TYPE:
		if (cmdRouleau->GetCommand() && cmdItem[0]->GetCommand() && cmdItem[1]->GetCommand()) {
			labForgeGold->SetCaption(StringSplitNum(CalMakeEquipMoney()));
			btnYes->SetIsEnabled(true);
		} else {
			labForgeGold->SetCaption("");
			btnYes->SetIsEnabled(false);
		}
	case ELF_SHIFT_TYPE:
		if (cmdRouleau->GetCommand() && cmdItem[0]->GetCommand() && cmdItem[1]->GetCommand()) {
			labForgeGold->SetCaption(StringSplitNum(CalMakeEquipMoney()));
			btnYes->SetIsEnabled(true);
		} else {
			labForgeGold->SetCaption("0");
			btnYes->SetIsEnabled(false);
		}

		break;
	}
}

//-----------------------------------------------------------------------------
void CMakeEquipMgr::SendMakeEquipProtocol() {
	stNetItemForgeAsk kNetItemForgeAsk;
	kNetItemForgeAsk.chType = char(m_iType); // ??

	if (m_iType == MAKE_EQUIP_TYPE) {
		if (IsMakeGem()) {
			// ??0
			kNetItemForgeAsk.SGroup[0].sCellNum = 1;
			kNetItemForgeAsk.SGroup[0].pCell = new SForgeCell::SCell[1];
			kNetItemForgeAsk.SGroup[0].pCell[0].sNum = 1;
			kNetItemForgeAsk.SGroup[0].pCell[0].sPosID = m_iRouleauPos;

			// ??1~2
			for (int i(1); i <= STONE_ITEM_NUM; ++i) {
				kNetItemForgeAsk.SGroup[i].sCellNum = 1; // ?????1
				kNetItemForgeAsk.SGroup[i].pCell = new SForgeCell::SCell[1];
				kNetItemForgeAsk.SGroup[i].pCell[0].sNum = equipItems[i - 1][0]->iNum;
				kNetItemForgeAsk.SGroup[i].pCell[0].sPosID = equipItems[i - 1][0]->iPos;
			}
		} else {
			for (int i(0); i < ITEM_NUM; ++i) {
				kNetItemForgeAsk.SGroup[i].sCellNum = 1; // ?????1
				kNetItemForgeAsk.SGroup[i].pCell = new SForgeCell::SCell[1];
				kNetItemForgeAsk.SGroup[i].pCell[1].sNum = 1;
				// kNetItemForgeAsk.SGroup[i].pCell[1].sPosID = m_iForgeItemPos[i];
			}
		}
	} else { // ?????
		// ??0
		kNetItemForgeAsk.SGroup[0].sCellNum = 1;
		kNetItemForgeAsk.SGroup[0].pCell = new SForgeCell::SCell[1];
		kNetItemForgeAsk.SGroup[0].pCell[0].sNum = 1;
		kNetItemForgeAsk.SGroup[0].pCell[0].sPosID = m_iRouleauPos;

		int iNum(0);
		if (m_iType == EQUIP_FUSION_TYPE) {
			if (!cmdItem[2]->GetCommand()) {
				iNum = FUSION_NUM - 1;
			} else {
				iNum = FUSION_NUM;
			}
		} else if (m_iType == EQUIP_UPGRADE_TYPE) {
			iNum = UPGRADE_NUM;
		} else if (m_iType == ELF_SHIFT_TYPE) {
			iNum = SHIFT_NUM;
		}

		for (int i(1); i <= iNum; ++i) {
			kNetItemForgeAsk.SGroup[i].sCellNum = 1; // ?????1
			kNetItemForgeAsk.SGroup[i].pCell = new SForgeCell::SCell[1];
			kNetItemForgeAsk.SGroup[i].pCell[0].sNum = equipItems[i - 1][0]->iNum;
			kNetItemForgeAsk.SGroup[i].pCell[0].sPosID = equipItems[i - 1][0]->iPos;
		}
	}

	CS_ItemForgeAsk(true, &kNetItemForgeAsk);

	ShowMakeEquipForm(false);

	return;
}

//-----------------------------------------------------------------------------
long CMakeEquipMgr::CalMakeEquipMoney() {
	CItemCommand* pItemCommand(0);
	long iLevelPlusOne = 0, nLevel1 = 0, nLevel2 = 0;
	switch (m_iType) {
	case MAKE_EQUIP_TYPE:
		return MAKE_EQUIP_MONEY;
		break;
	case EQUIP_FUSION_TYPE:
		pItemCommand = dynamic_cast<CItemCommand*>(cmdItem[1]->GetCommand());
		return EQUIP_FUSION_MONEY * pItemCommand->GetItemInfo()->sNeedLv;
		break;
	case EQUIP_UPGRADE_TYPE:
		// Cost formula: 200,000 + (CurrentLevel × 50,000) - matches server forge.lua
		pItemCommand = dynamic_cast<CItemCommand*>(cmdItem[0]->GetCommand());
		iLevelPlusOne = pItemCommand->GetData().GetItemLevel(); // Current upgrade level (not +1)
		return 200000 + (iLevelPlusOne * 50000);
		break;
	case ELF_SHIFT_TYPE:
		// ????(????)
		pItemCommand = dynamic_cast<CItemCommand*>(cmdItem[0]->GetCommand());
		nLevel1 = pItemCommand->GetData().GetItemLevel();
		pItemCommand = dynamic_cast<CItemCommand*>(cmdItem[1]->GetCommand());
		nLevel2 = pItemCommand->GetData().GetItemLevel();
		return (nLevel1 >= 60 || nLevel2 >= 60) ? 0 : (60 - nLevel1) * (60 - nLevel2) * 10000;
		break;
	}
	return 0;
}

// ??????????????
bool CMakeEquipMgr::IsElfShiftStone(CItemCommand& rItem) {
	CItemRecord* pItem = rItem.GetItemInfo();
	if (pItem != nullptr && pItem->lID == 3918 || pItem->lID == 3919 || pItem->lID == 3920 ||
		pItem->lID == 3921 || pItem->lID == 3922 || pItem->lID == 3924 || pItem->lID == 3925) {
		return true;
	}

	return false;
}

// ?????
bool CMakeEquipMgr::IsElfShiftItem(CItemCommand& rItem) {
	CItemRecord* pItemRecord = rItem.GetItemInfo();
	if (pItemRecord && pItemRecord->sType == 59) {
		return true;
	}

	return false;
}

// ????
void CMakeEquipMgr::PushElfShiftItem(int iIndex, CItemCommand& rItem) {
	CItemCommand* pItemCommand = nullptr;
	SItemHint sItemHint;
	memset(&sItemHint, 0, sizeof(SItemHint));
	sItemHint.Convert(rItem.GetData(), rItem.GetItemInfo());

	// ??????
	int nLevel = sItemHint.sInstAttr[ITEMATTR_VAL_STR] +
				 sItemHint.sInstAttr[ITEMATTR_VAL_AGI] +
				 sItemHint.sInstAttr[ITEMATTR_VAL_DEX] +
				 sItemHint.sInstAttr[ITEMATTR_VAL_CON] +
				 sItemHint.sInstAttr[ITEMATTR_VAL_STA];

	if (20 > nLevel) {
		g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_702));
		return;
	}

	if (iIndex == 0) {
		pItemCommand = dynamic_cast<CItemCommand*>(cmdItem[1]->GetCommand());

		// ???????,???????????ID??
		if (IsElfShiftItem(rItem) &&
			(nullptr == pItemCommand || (rItem.GetItemInfo()->lID != pItemCommand->GetItemInfo()->lID))) {
			PushItem(iIndex, rItem, 1);
		} else {
			g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_703));
			return;
		}
	} else if (iIndex == 1) {
		pItemCommand = dynamic_cast<CItemCommand*>(cmdItem[0]->GetCommand());

		// ???????,???????????ID??
		if (IsElfShiftItem(rItem) &&
			(nullptr == pItemCommand || (rItem.GetItemInfo()->lID != pItemCommand->GetItemInfo()->lID))) {
			PushItem(iIndex, rItem, 1);
		} else {
			g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_703));
			return;
		}
	}
}

void CMakeEquipMgr::PopElfShiftItem(int iIndex) {
	PopItem(iIndex);
}

} // end of namespace GUI
