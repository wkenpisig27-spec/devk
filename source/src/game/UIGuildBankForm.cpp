#include "StdAfx.h"

#include "UIGuildBankForm.h"
#include "uiform.h"
#include "uilabel.h"
#include "uiformmgr.h"
#include "uigoodsgrid.h"
#include "NetProtocol.h"
#include "uiboxform.h"
#include "uiEquipForm.h"
#include "UIGoodsGrid.h"
#include "uiItemCommand.h"
#include "uiform.h"
#include "uiBoatForm.h"
#include "packetcmd.h"
#include "Character.h"
#include "GameApp.h"
#include "UIGuildMgr.h"
#include "rmlui/RmlUiGuildForm.h"

#include "StringLib.h"
#include <vector>

namespace GUI {

// Static member definition for split item data
CGuildBankMgr::SSplitItem CGuildBankMgr::SSplit;

//=======================================================================
//	CGuildBankMgr 's Members
//=======================================================================

bool CGuildBankMgr::Init() // ?????????
{
	CFormMgr& mgr = CFormMgr::s_Mgr;

	frmBank = mgr.Find("frmManage"); // ??NPC??????
	if (!frmBank) {
		LG("gui", RES_STRING(CL_LANGUAGE_MATCH_438));
		return false;
	}

	grdBank = dynamic_cast<CGoodsGrid*>(frmBank->Find("guildBank"));

	labGuildMoney = dynamic_cast<CLabel*>(frmBank->Find("labGuildMoney"));
	btnGoldTake = dynamic_cast<CTextButton*>(frmBank->Find("btngoldtake"));
	btnGoldPut = dynamic_cast<CTextButton*>(frmBank->Find("btngoldput"));

	grdBank->evtBeforeAccept = CUIInterface::_evtDragToGoodsEvent;
	grdBank->evtSwapItem = _evtBankToBank;
	grdBank->evtUseCommand = _evtBankUseCommand; // dblclick → withdraw to bag (no CDrag)

	btnGoldPut->evtMouseClick = _OnClickGoldPut;
	btnGoldTake->evtMouseClick = _OnClickGoldTake;

	return true;
}

void CGuildBankMgr::UpdateGuildGold(const char* value) {
	m_goldText = value ? StringSplitNum(value) : "";
	if (labGuildMoney)
		labGuildMoney->SetCaption(m_goldText.c_str());
	if (CRmlUiGuildForm::Instance().IsVisible())
		CRmlUiGuildForm::Instance().SetGold(m_goldText.c_str());
}

bool CGuildBankMgr::IsBankOpen() const {
	if (CRmlUiGuildForm::Instance().IsVisible() && CRmlUiGuildForm::Instance().GetActiveTab() == 2)
		return true;
	return frmBank && frmBank->GetIsShow();
}

void CGuildBankMgr::PromptGoldTake() {
	_OnClickGoldTake(nullptr, 0, 0, 0);
}

void CGuildBankMgr::PromptGoldPut() {
	_OnClickGoldPut(nullptr, 0, 0, 0);
}

void CGuildBankMgr::RefreshGuildBankUi() {
	CRmlUiGuildForm& rml = CRmlUiGuildForm::Instance();
	if (!rml.IsVisible() || rml.GetActiveTab() != 2 || !grdBank)
		return;

	std::vector<RmlInvSlotView> slots;
	const int maxNum = grdBank->GetMaxNum();
	const int cols = grdBank->GetCol() > 0 ? grdBank->GetCol() : 8;
	slots.reserve((size_t)maxNum);
	for (int i = 0; i < maxNum; ++i) {
		RmlInvSlotView view;
		view.id = i;
		if (CItemCommand* cmd = dynamic_cast<CItemCommand*>(grdBank->GetItem(i))) {
			if (CItemRecord* info = cmd->GetItemInfo()) {
				if (info->lID != 32767) {
					view.iconPath = info->GetIconFile();
					view.qty = cmd->GetTotalNum();
				} else {
					view.locked = true;
				}
			}
		}
		slots.push_back(view);
	}
	rml.SetVaultSlots(slots, cols);
}

bool CGuildBankMgr::MoveBankItem(int srcBankIndex, int dstBankIndex) {
	if (!grdBank || srcBankIndex < 0 || dstBankIndex < 0 || srcBankIndex == dstBankIndex)
		return false;
	if (!grdBank->GetItem(srcBankIndex))
		return false;
	bool isSwap = false;
	_evtBankToBank(grdBank, dstBankIndex, srcBankIndex, isSwap);
	return true;
}

void CGuildBankMgr::_OnClickGoldPut(CGuiData* pSender, int x, int y, DWORD key) {
	g_stUIBox.ShowNumberBox(_EnterGoldPut, g_stUIBoat.GetHuman()->getGameAttr()->get(ATTR_GD), "Enter Gold", false);
}

void CGuildBankMgr::_EnterGoldPut(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	if (nMsgType != CForm::mrYes) {
		return;
	}
	stNumBox* kItemPriceBox = (stNumBox*)pSender->GetForm()->GetPointer();
	if (!kItemPriceBox)
		return;
	long long value = kItemPriceBox->GetNumber();
	if (value <= 0) {
		g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_451));
		return;
	}
	CS_GuildBankGiveGold(value);
}

void CGuildBankMgr::_OnClickGoldTake(CGuiData* pSender, int x, int y, DWORD key) {
	g_stUIBox.ShowNumberBox(_EnterGoldTake, 100000000000LL - (g_stUIBoat.GetHuman()->getGameAttr()->get(ATTR_GD)), "Enter Gold", false);
}

void CGuildBankMgr::_EnterGoldTake(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	if (nMsgType != CForm::mrYes) {
		return;
	}
	stNumBox* kItemPriceBox = (stNumBox*)pSender->GetForm()->GetPointer();
	if (!kItemPriceBox)
		return;
	long long value = kItemPriceBox->GetNumber();
	if (value <= 0) {
		g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_451));
		return;
	}
	CS_GuildBankTakeGold(value);
}

void CGuildBankMgr::_evtOnClose(CForm* pForm, bool& IsClose) // ??????
{
	CS_BeginAction(g_stUIBoat.GetHuman(), enumACTION_CLOSE_BANK, nullptr);

	CFormMgr::s_Mgr.SetEnableHotKey(HOTKEY_BANK, true); // ??????
}

//-------------------------------------------------------------------------
void CGuildBankMgr::ShowBank() // ????
{
	// ??????????

	if (!g_stUIBoat.GetHuman()) // ???
		return;

	char szBuf[32];
	sprintf(szBuf, "%s%s", g_stUIBoat.GetHuman()->getName(), RES_STRING(CL_LANGUAGE_MATCH_440)); // ????????
	// labCharName->SetCaption(szBuf);//??????

	frmBank->Show();

	// Rml inventory is the bag UI; guild transfers use index-based Move* wrappers.
	if (!g_stUIEquip.IsInventoryUiVisible())
		g_stUIEquip.ShowInventoryUi();

	CFormMgr::s_Mgr.SetEnableHotKey(HOTKEY_BANK, false); // ??????
}

//-------------------------------------------------------------------------
bool CGuildBankMgr::MoveBagToBank(int bagIndex, int bankSlot) {
	CGoodsGrid* bag = g_stUIEquip.GetGoodsGrid();
	if (!bag || !grdBank || bagIndex < 0)
		return false;

	CCommandObj* item = bag->GetItem(bagIndex);
	if (!item)
		return false;

	if (bankSlot < 0)
		bankSlot = grdBank->GetFreeIndex();
	if (bankSlot < 0)
		return false;

	bag->SetDragIndex(bagIndex);
	return PushToBank(*bag, *grdBank, bankSlot, *item);
}

//-------------------------------------------------------------------------
bool CGuildBankMgr::MoveBankToBag(int bankIndex, int bagSlot) {
	CGoodsGrid* bag = g_stUIEquip.GetGoodsGrid();
	if (!bag || !grdBank || bankIndex < 0)
		return false;

	CCommandObj* item = grdBank->GetItem(bankIndex);
	if (!item)
		return false;

	if (bagSlot < 0)
		bagSlot = bag->GetFreeIndex();
	if (bagSlot < 0)
		return false;

	grdBank->SetDragIndex(bankIndex);
	return PopFromBank(*grdBank, *bag, bagSlot, *item);
}

//-------------------------------------------------------------------------
void CGuildBankMgr::_evtBankUseCommand(CCommandObj* pSender, bool& isUse) {
	isUse = false;
	if (!g_stUIGuildBank.IsBankOpen())
		return;

	CItemCommand* item = dynamic_cast<CItemCommand*>(pSender);
	if (!item)
		return;

	const int bankIndex = item->GetIndex();
	if (bankIndex < 0)
		return;

	g_stUIGuildBank.MoveBankToBag(bankIndex, -1);
	g_stUIEquip.RefreshRmlInventory();
}

//-------------------------------------------------------------------------
bool CGuildBankMgr::PushToBank(CGoodsGrid& rkDrag, CGoodsGrid& rkSelf, int nGridID, CCommandObj& rkItem) {
#define EQUIP_TYPE 0
#define BANK_TYPE 1

	// ??????????????
	m_kNetBank.chSrcType = EQUIP_TYPE;
	m_kNetBank.sSrcID = rkDrag.GetDragIndex();
	// m_kNetBank.sSrcNum = ; ??????????
	m_kNetBank.chTarType = BANK_TYPE;
	m_kNetBank.sTarID = nGridID;

	// ?????????????
	CItemCommand* pkItemCmd = dynamic_cast<CItemCommand*>(&rkItem);
	if (!pkItemCmd)
		return false;
	CItemRecord* pkItemRecord = pkItemCmd->GetItemInfo();
	if (!pkItemRecord)
		return false;

	// if(pkItemRecord->sType == 59 && m_kNetBank.sSrcID == 1)
	//{
	//	g_pGameApp->MsgBox("?????????\n??????????????");
	//	return false;	// ?????????????
	// }

	// if(pkItemRecord->lID == 2520 || pkItemRecord->lID == 2521)
	if (pkItemRecord->lID == 2520 || pkItemRecord->lID == 2521 || pkItemRecord->lID == 6341 || pkItemRecord->lID == 6343 || pkItemRecord->lID == 6347 || pkItemRecord->lID == 6359 || pkItemRecord->lID == 6370 || pkItemRecord->lID == 6371 || pkItemRecord->lID == 6373 || pkItemRecord->lID >= 6376 && pkItemRecord->lID <= 6378 || pkItemRecord->lID >= 6383 && pkItemRecord->lID <= 6385) // modify by ning.yan 20080820 ???????????,???????????
	{
		// g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_958));	// "??????????!?????"
		g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_958)); // "??????????!?????"
		return false;
	}
	if (pkItemCmd->GetItemInfo()->GetIsPile() && pkItemCmd->GetTotalNum() > 1) { /*??????*/
		m_pkNumberBox =
			g_stUIBox.ShowNumberBox(_MoveItemsEvent, pkItemCmd->GetTotalNum(), RES_STRING(CL_LANGUAGE_MATCH_441), false);

		if (m_pkNumberBox->GetNumber() < pkItemCmd->GetTotalNum())
			return false;
		else
			return true;
	} else { /*??????*/
		g_stUIGuildBank.m_kNetBank.sSrcNum = 1;
		// CS_BeginAction(g_stUIBoat.GetHuman(), enumACTION_GUILDBANK, (void*)&(g_stUIGuildBank.m_kNetBank));
		CS_GuildBankOper(&g_stUIGuildBank.m_kNetBank);
		return true;
		// char buf[256] = { 0 };
		// sprintf(buf, "???????\n[%s]?", pkItemCmd->GetName());
		// g_stUIBox.ShowSelectBox(_MoveAItemEvent, buf, true);
		// return true;
	}
}

//-------------------------------------------------------------------------
bool CGuildBankMgr::PopFromBank(CGoodsGrid& rkDrag, CGoodsGrid& rkSelf, int nGridID, CCommandObj& rkItem) {
	// ??????????????
	m_kNetBank.chSrcType = BANK_TYPE;
	m_kNetBank.sSrcID = rkDrag.GetDragIndex();
	// m_kNetBank.sSrcNum = ; ??????????
	m_kNetBank.chTarType = EQUIP_TYPE;
	m_kNetBank.sTarID = nGridID;

	// ?????????????
	CItemCommand* pkItemCmd = dynamic_cast<CItemCommand*>(&rkItem);
	if (!pkItemCmd)
		return false;

	if (pkItemCmd->GetItemInfo()->GetIsPile() && pkItemCmd->GetTotalNum() > 1) { /*??????*/
		m_pkNumberBox =
			g_stUIBox.ShowNumberBox(_MoveItemsEvent, pkItemCmd->GetTotalNum(), RES_STRING(CL_LANGUAGE_MATCH_442), false);

		if (m_pkNumberBox->GetNumber() < pkItemCmd->GetTotalNum())
			return false;
		else
			return true;
	} else { /*??????*/
		g_stUIGuildBank.m_kNetBank.sSrcNum = 1;
		// CS_BeginAction(g_stUIBoat.GetHuman(), enumACTION_GUILDBANK, (void*)&(g_stUIGuildBank.m_kNetBank));
		CS_GuildBankOper(&g_stUIGuildBank.m_kNetBank);
		return true;

		// char buf[256] = { 0 };
		// sprintf(buf, "?????\n[%s]?", pkItemCmd->GetName());
		// g_stUIBox.ShowSelectBox(_MoveAItemEvent, buf, true);
		// return true;
	}
}

//-------------------------------------------------------------------------
void CGuildBankMgr::_MoveItemsEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) // ??????
{
	if (nMsgType != CForm::mrYes) // ????????
		return;

	int num = g_stUIGuildBank.m_pkNumberBox->GetNumber(); // ?????
	if (num > 0) {
		g_stUIGuildBank.m_kNetBank.sSrcNum = num;
		// CS_BeginAction(g_stUIBoat.GetHuman(), enumACTION_GUILDBANK, (void*)&(g_stUIGuildBank.m_kNetBank));
		CS_GuildBankOper(&g_stUIGuildBank.m_kNetBank);
	}
}

//-------------------------------------------------------------------------
void CGuildBankMgr::_MoveAItemEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) // ??????
{
	if (nMsgType != CForm::mrYes)
		return;

	g_stUIGuildBank.m_kNetBank.sSrcNum = 1;
	// CS_BeginAction(g_stUIBoat.GetHuman(), enumACTION_GUILDBANK, (void*)&(g_stUIGuildBank.m_kNetBank));//??????
	CS_GuildBankOper(&g_stUIGuildBank.m_kNetBank);
}

//-------------------------------------------------------------------------
void CGuildBankMgr::CloseForm() // ???????
{
	CUIGuildMgr::HideUi();
	if (frmBank && frmBank->GetIsShow()) {
		frmBank->Close();
		g_stUIEquip.HideInventoryUi();
	}
}

//-------------------------------------------------------------------------
void CGuildBankMgr::_evtBankToBank(CGuiData* pSender, int nFirst, int nSecond, bool& isSwap) // ?????????????
{
	isSwap = false;
	if (!g_stUIBoat.GetHuman())
		return;

	CGoodsGrid* pGood = dynamic_cast<CGoodsGrid*>(pSender);
	if (!pGood)
		return;

	// Check if Shift is pressed for splitting stacked items
	if (g_pGameApp->IsShiftPress()) {
		CItemCommand* pItem = dynamic_cast<CItemCommand*>(pGood->GetItem(nSecond));
		CItemCommand* pTarget = dynamic_cast<CItemCommand*>(pGood->GetItem(nFirst));
		
		// Can only split if source item exists and has quantity > 1
		if (pItem && pItem->GetTotalNum() > 1) {
			// Can only split to empty slot or slot with same item type
			if (!pTarget || (pTarget->GetItemInfo() && pItem->GetItemInfo() && 
				pTarget->GetItemInfo()->nID == pItem->GetItemInfo()->nID)) {
				// Store split parameters for callback
				SSplit.nFirst = nFirst;
				SSplit.nSecond = nSecond;
				g_stUIBox.ShowNumberBox(SSplitItem::_evtSplitItemEvent, pItem->GetTotalNum(), 
					RES_STRING(CL_LANGUAGE_MATCH_441), false);
				return;
			}
		}
	}

	g_stUIGuildBank.m_kNetBank.chSrcType = BANK_TYPE;
	g_stUIGuildBank.m_kNetBank.sSrcID = nSecond;
	g_stUIGuildBank.m_kNetBank.sSrcNum = 0;
	g_stUIGuildBank.m_kNetBank.chTarType = BANK_TYPE;
	g_stUIGuildBank.m_kNetBank.sTarID = nFirst;

	// CS_BeginAction(g_stUIBoat.GetHuman(), enumACTION_GUILDBANK, (void*)&(g_stUIGuildBank.m_kNetBank));
	CS_GuildBankOper(&g_stUIGuildBank.m_kNetBank);
}

//-------------------------------------------------------------------------
void CGuildBankMgr::SSplitItem::_evtSplitItemEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey)
{
	if (nMsgType != CForm::mrYes)
		return;

	stNumBox* pBox = (stNumBox*)pSender->GetForm()->GetPointer();
	if (!pBox)
		return;

	int num = pBox->GetNumber();
	if (num <= 0)
		return;

	g_stUIGuildBank.m_kNetBank.chSrcType = BANK_TYPE;
	g_stUIGuildBank.m_kNetBank.sSrcID = SSplit.nSecond;
	g_stUIGuildBank.m_kNetBank.sSrcNum = num;
	g_stUIGuildBank.m_kNetBank.chTarType = BANK_TYPE;
	g_stUIGuildBank.m_kNetBank.sTarID = SSplit.nFirst;

	CS_GuildBankOper(&g_stUIGuildBank.m_kNetBank);
}

} // end of namespace GUI
