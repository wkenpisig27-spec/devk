#include "StdAfx.h"
#include "uinpctradeform.h"
#include "uiequipform.h"
#include "uigoodsgrid.h"
#include "uiitemcommand.h"
#include "uipage.h"
#include "packetcmd.h"
#include "gameapp.h"
#include "scene.h"
#include "character.h"
#include "uiboxform.h"
#include "uiedit.h"
#include "uiboatform.h"
#include "StringLib.h"
#include "rmlui/RmlUiNpcTradeForm.h"
#include "rmlui/RmlUiInventoryForm.h"
#include "rmlui/RmlUiManager.h"

#include <vector>

using namespace GUI;

namespace {

enum class RmlTradePending {
	None,
	BuyConfirm,
	SaleConfirm,
	BuyQty,
	SaleQty,
};

RmlTradePending g_rmlTradePending = RmlTradePending::None;

} // namespace

//---------------------------------------------------------------------------
// class CNpcTradeMgr
//---------------------------------------------------------------------------
bool CNpcTradeMgr::Init() {
	_dwNpcID = 0;
	_IsShow = false;

	// NPC锟斤拷锟阶憋拷锟斤拷
	frmNPCtrade = _FindForm("frmNPCtrade"); // 锟斤拷锟竭憋拷锟斤拷
	if (!frmNPCtrade)
		return false;

	CPage* pgeNPCtrade = (CPage*)frmNPCtrade->Find("pgeNPCtrade");
	if (!pgeNPCtrade)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmNPCtrade->GetName(), "pgeNPCtrade");

	// 锟斤拷锟阶碉拷锟斤拷锟斤拷锟斤拷
	grdNPCtradeWeapon = dynamic_cast<CGoodsGrid*>(frmNPCtrade->Find("grdNPCtradeWeapon"));
	if (!grdNPCtradeWeapon)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmNPCtrade->GetName(), "grdNPCtradeWeapon");
	grdNPCtradeWeapon->evtBeforeAccept = _evtDragToGoodsEvent;

	// 锟斤拷锟阶碉拷装锟斤拷锟斤拷
	grdNPCtradeEquip = dynamic_cast<CGoodsGrid*>(frmNPCtrade->Find("grdNPCtradeEquip"));
	if (!grdNPCtradeEquip)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmNPCtrade->GetName(), "grdNPCtradeEquip");
	grdNPCtradeEquip->evtBeforeAccept = _evtDragToGoodsEvent;

	// 锟斤拷锟阶碉拷药品锟斤拷
	grdNPCtradeOther = dynamic_cast<CGoodsGrid*>(frmNPCtrade->Find("grdNPCtradeOther"));
	if (!grdNPCtradeOther)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmNPCtrade->GetName(), "grdNPCtradeOther");
	grdNPCtradeOther->evtBeforeAccept = _evtDragToGoodsEvent;

	// Data host only — player chrome is Notice Rml trade.
	frmNPCtrade->SetHotKey(0);
	frmNPCtrade->SetIsEscClose(false);
	frmNPCtrade->SetIsShow(false);
	return true;
}

void CNpcTradeMgr::End() {
}

void CNpcTradeMgr::ShowTradePage(const NET_TRADEINFO& TradeInfo, BYTE byCmd, DWORD dwNpcID) {
	_dwNpcID = dwNpcID;

	if (frmNPCtrade) // 锟津开斤拷锟斤拷前锟斤拷删锟斤拷锟斤拷锟叫的碉拷锟竭ｏ拷锟斤拷锟竭猴拷锟斤拷锟斤拷
	{
		grdNPCtradeWeapon->Clear();
		grdNPCtradeEquip->Clear();
		grdNPCtradeOther->Clear();
	}
	CItemRecord* pItem;

	int j;
	for (j = 0; j < TradeInfo.TradePage[0].byCount; j++) {
		pItem = GetItemRecordInfo(TradeInfo.TradePage[0].sItemID[j]);
		if (!pItem) {
			LG("error", "msgCNpcTradeMgr::ShowTradePage item index[%d] out of range", TradeInfo.TradePage[0].sItemID[j]);
			continue;
		}

		CItemCommand* pObj = new CItemCommand(pItem);

		_NpcItemRefresh(pObj);
		if (!grdNPCtradeWeapon->SetItem(j, pObj)) {
			// delete pObj;
			SAFE_DELETE(pObj); // UI锟斤拷锟斤拷锟斤拷锟斤拷
			LG("error", "msgShowTradePage grdNPCtradeWeapon out of range\n");
		}
	}
	grdNPCtradeWeapon->GetScroll()->Reset();

	for (j = 0; j < TradeInfo.TradePage[1].byCount; j++) {
		pItem = GetItemRecordInfo(TradeInfo.TradePage[1].sItemID[j]);
		if (!pItem) {
			LG("error", "msgCNpcTradeMgr::ShowTradePage item index[%d] out of range", TradeInfo.TradePage[1].sItemID[j]);
			continue;
		}

		CItemCommand* pObj = new CItemCommand(pItem);
		_NpcItemRefresh(pObj);
		if (!grdNPCtradeEquip->SetItem(j, pObj)) {
			// delete pObj;
			SAFE_DELETE(pObj); // UI锟斤拷锟斤拷锟斤拷锟斤拷
			LG("error", "msgShowTradePage grdNPCtradeEquip out of range\n");
		}
	}
	grdNPCtradeEquip->GetScroll()->Reset();

	for (j = 0; j < TradeInfo.TradePage[2].byCount; j++) {
		pItem = GetItemRecordInfo(TradeInfo.TradePage[2].sItemID[j]);
		if (!pItem) {
			LG("error", "msgCNpcTradeMgr::ShowTradePage item index[%d] out of range", TradeInfo.TradePage[2].sItemID[j]);
			continue;
		}

		CItemCommand* pObj = new CItemCommand(pItem);
		_NpcItemRefresh(pObj);
		if (!grdNPCtradeOther->SetItem(j, pObj)) {
			// delete pObj;
			SAFE_DELETE(pObj); // UI锟斤拷锟斤拷锟斤拷锟斤拷
			LG("error", "msgShowTradePage grdNPCtradeEquip out of range\n");
		}
	}
	grdNPCtradeOther->GetScroll()->Reset();

	int nIndex = 0;
	if (TradeInfo.TradePage[0].byCount > 0) {
		nIndex = 0;
	} else if (TradeInfo.TradePage[1].byCount > 0) {
		nIndex = 1;
	} else {
		nIndex = 2;
	}
	CPage* pgeNPCtrade = dynamic_cast<CPage*>(frmNPCtrade->Find("pgeNPCtrade"));
	if (pgeNPCtrade)
		pgeNPCtrade->SetIndex(nIndex);

	if (frmNPCtrade) {
		frmNPCtrade->SetIsShow(false);
	}

	g_stUIEquip.ShowInventoryUi();
	ShowTradeUi(nIndex);

	_IsShow = true;
}

void CNpcTradeMgr::SaleToNpc(BYTE byIndex, BYTE byCount, USHORT sItemID, DWORD dwMoney) {
}

void CNpcTradeMgr::BuyFromNpc(BYTE byIndex, BYTE byCount, USHORT sItemID, DWORD dwMoney) {
}

void CNpcTradeMgr::_NpcItemRefresh(CItemCommand* pItem) {
	static SItemGrid data;
	memset(&data, 0, sizeof(data));
	data.SetValid(true);

	CItemRecord* pInfo = pItem->GetItemInfo();
	// Default properties for NPCs sale - Mdr October 2020
	data.expiration = 0;
	data.bItemTradable = true;

	if (pInfo->sType >= 1 && pInfo->sType <= 10) // 锟斤拷锟斤拷
	{
		// 锟酵久讹拷
		data.sEndure[0] = pInfo->sEndure[0];
		data.sEndure[1] = pInfo->sEndure[0];

		int i = 0;
		data.sInstAttr[i][0] = ITEMATTR_VAL_MNATK;
		data.sInstAttr[i][1] = pInfo->sMnAtkValu[0];

		i++;
		data.sInstAttr[i][0] = ITEMATTR_VAL_MXATK;
		data.sInstAttr[i][1] = pInfo->sMxAtkValu[0];

		switch (pInfo->sType) {
		case 1: // 锟斤拷锟街斤拷
			i++;
			data.sInstAttr[i][0] = ITEMATTR_COE_ASPD;
			data.sInstAttr[i][1] = pInfo->sASpdCoef;

			i++;
			data.sInstAttr[i][0] = ITEMATTR_VAL_HIT;
			data.sInstAttr[i][1] = pInfo->sHitValu[0];
			break;
		case 2: // 锟睫斤拷
			i++;
			data.sInstAttr[i][0] = ITEMATTR_VAL_DEF;
			data.sInstAttr[i][1] = pInfo->sDefValu[0];

			i++;
			data.sInstAttr[i][0] = ITEMATTR_VAL_MXHP;
			data.sInstAttr[i][1] = pInfo->sMxHpValu[0];
			break;
		case 3: // 锟斤拷
		case 4: // 锟斤拷枪
			i++;
			data.sInstAttr[i][0] = ITEMATTR_COE_ASPD;
			data.sInstAttr[i][1] = pInfo->sASpdCoef;

			i++;
			data.sInstAttr[i][0] = ITEMATTR_VAL_HIT;
			data.sInstAttr[i][1] = pInfo->sHitValu[0];
			break;
		case 7: // 匕锟斤拷
			i++;
			data.sInstAttr[i][0] = ITEMATTR_COE_MXSP;
			data.sInstAttr[i][1] = pInfo->sMxSpCoef;

			i++;
			data.sInstAttr[i][0] = ITEMATTR_VAL_STA;
			data.sInstAttr[i][1] = pInfo->sStaValu[0];

			i++;
			data.sInstAttr[i][0] = ITEMATTR_COE_MSPD;
			data.sInstAttr[i][1] = pInfo->sMSpdCoef;
			break;
		case 9: // 锟斤拷锟斤拷
			i++;
			data.sInstAttr[i][0] = ITEMATTR_VAL_STA;
			data.sInstAttr[i][1] = pInfo->sStaValu[0];

			i++;
			data.sInstAttr[i][0] = ITEMATTR_COE_MXSP;
			data.sInstAttr[i][1] = pInfo->sMxSpCoef;

			i++;
			data.sInstAttr[i][0] = ITEMATTR_VAL_MXHP;
			data.sInstAttr[i][1] = pInfo->sMxHpValu[0];
			break;
		}

		pItem->SetData(data);
	} else if (pInfo->sType == 22 || pInfo->sType == 11 || pInfo->sType == 27) {
		// 锟斤拷锟斤拷锟斤拷
		int i = 0;
		data.sInstAttr[i][0] = ITEMATTR_VAL_DEF;
		data.sInstAttr[i][1] = pInfo->sDefValu[0];

		// 锟酵久讹拷
		data.sEndure[0] = pInfo->sEndure[0];
		data.sEndure[1] = pInfo->sEndure[0];

		// 锟斤拷锟斤拷锟街匡拷
		i++;
		data.sInstAttr[i][0] = ITEMATTR_VAL_PDEF;
		data.sInstAttr[i][1] = pInfo->sPDef[0];

		pItem->SetData(data);
	} else if (pInfo->sType == 25) // 锟斤拷锟斤拷
	{
		int i = 0;
		data.sInstAttr[i][0] = ITEMATTR_VAL_MXHP;
		data.sInstAttr[i][1] = pInfo->sMxHpValu[0];

		i++;
		data.sInstAttr[i][0] = ITEMATTR_VAL_MXSP;
		data.sInstAttr[i][1] = pInfo->sMxSpValu[0];

		i++;
		data.sInstAttr[i][0] = ITEMATTR_VAL_HREC;
		data.sInstAttr[i][1] = pInfo->sHRecValu[0];

		i++;
		data.sInstAttr[i][0] = ITEMATTR_VAL_SREC;
		data.sInstAttr[i][1] = pInfo->sSRecValu[0];

		i++;
		data.sInstAttr[i][0] = ITEMATTR_VAL_PDEF;
		data.sInstAttr[i][1] = pInfo->sPDef[0];

		pItem->SetData(data);
	} else if (pInfo->sType == 26) // 锟斤拷指
	{
		int i = 0;
		data.sInstAttr[i][0] = ITEMATTR_VAL_MXATK;
		data.sInstAttr[i][1] = pInfo->sMxAtkValu[0];

		i++;
		data.sInstAttr[i][0] = ITEMATTR_VAL_DEF;
		data.sInstAttr[i][1] = pInfo->sDefValu[0];

		i++;
		data.sInstAttr[i][0] = ITEMATTR_VAL_FLEE;
		data.sInstAttr[i][1] = pInfo->sFleeValu[0];

		i++;
		data.sInstAttr[i][0] = ITEMATTR_VAL_HIT;
		data.sInstAttr[i][1] = pInfo->sHitValu[0];

		i++;
		data.sInstAttr[i][0] = ITEMATTR_VAL_CRT;
		data.sInstAttr[i][1] = pInfo->sCrtValu[0];

		pItem->SetData(data);
	} else if (pInfo->sType == 23) // 锟斤拷锟斤拷
	{
		// 锟斤拷锟斤拷锟斤拷
		int i = 0;
		data.sInstAttr[i][0] = ITEMATTR_VAL_DEF;
		data.sInstAttr[i][1] = pInfo->sDefValu[0];

		// 锟酵久讹拷
		data.sEndure[0] = pInfo->sEndure[0];
		data.sEndure[1] = pInfo->sEndure[0];

		// 锟斤拷锟斤拷锟斤拷
		i++;
		data.sInstAttr[i][0] = ITEMATTR_VAL_HIT;
		data.sInstAttr[i][1] = pInfo->sHitValu[0];

		pItem->SetData(data);
	} else if (pInfo->sType == 24) // 鞋锟斤拷
	{
		// 锟斤拷锟斤拷锟斤拷
		int i = 0;
		data.sInstAttr[i][0] = ITEMATTR_VAL_DEF;
		data.sInstAttr[i][1] = pInfo->sDefValu[0];

		// 锟酵久讹拷
		data.sEndure[0] = pInfo->sEndure[0];
		data.sEndure[1] = pInfo->sEndure[0];

		// 锟斤拷锟斤拷锟斤拷
		i++;
		data.sInstAttr[i][0] = ITEMATTR_VAL_FLEE;
		data.sInstAttr[i][1] = pInfo->sFleeValu[0];

		pItem->SetData(data);
	} else if (pInfo->sType == 20) // 帽锟斤拷
	{
		// 锟斤拷锟斤拷锟斤拷
		int i = 0;
		data.sInstAttr[i][0] = ITEMATTR_VAL_DEF;
		data.sInstAttr[i][1] = pInfo->sDefValu[0];

		// 锟酵久讹拷
		data.sEndure[0] = pInfo->sEndure[0];
		data.sEndure[1] = pInfo->sEndure[0];

		pItem->SetData(data);
	} else if (pInfo->sType == 29) // 锟斤拷锟斤拷
	{
		data.sEnergy[0] = pInfo->sEnergy[0];
		data.sEnergy[1] = pInfo->sEnergy[1];

		pItem->SetData(data);
	} else {
		pItem->SetData(data);
	}
}

void CNpcTradeMgr::LocalBuyFromNpc(CGoodsGrid* pNpcGrid, CGoodsGrid* pSelfGrid, int nGridID, CCommandObj* pItem) {
	int nIndex = 2;
	if (pNpcGrid == g_stUINpcTrade.GetNPCtradeWeaponGrid())
		nIndex = 0;
	else if (pNpcGrid == g_stUINpcTrade.GetNPCtradeEquipGrid())
		nIndex = 1;

	CItemCommand* pBuy = dynamic_cast<CItemCommand*>(pItem);
	if (!pBuy)
		return;

	int nBuyGrid = nGridID;
	int nBuyCount = 1;
	if (pBuy && pBuy->GetItemInfo()->GetIsPile()) {
		CItemRecord* pRecord = pBuy->GetItemInfo();
		CItemCommand* pInfo = 0;
		int count = pSelfGrid->GetMaxNum();
		for (int i = 0; i < count; i++) {
			pInfo = dynamic_cast<CItemCommand*>(pSelfGrid->GetItem(i));
			if (pInfo && pInfo->GetItemInfo() == pRecord) {
				nBuyGrid = i;
				break;
			}
		}
	}

	int nMax = -1;
	if (pBuy->GetPrice() > 0 && CGameScene::GetMainCha()) {
		nMax = (int)(CGameScene::GetMainCha()->getGameAttr()->get(ATTR_GD) / pBuy->GetPrice());
		if (nMax > pBuy->GetItemInfo()->nPileMax)
			nMax = pBuy->GetItemInfo()->nPileMax;

		if (nMax == 0) {
			g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_459));
			return;
		}
	}
	char buf[256] = {0};
	sprintf(buf, "%s[%s$]?", pBuy->GetName(), StringSplitNum(pBuy->GetPrice()));

	_sBuy.dwNpcID = _dwNpcID;
	_sBuy.nBuyGrid = nBuyGrid;
	_sBuy.nDragIndex = pNpcGrid->GetDragIndex();
	_sBuy.nIndex = nIndex;
	_sBuy.pBox = nullptr;

	if (CRmlUiNpcTradeForm::Instance().IsVisible()) {
		const char* icon = pBuy->GetItemInfo() ? pBuy->GetItemInfo()->GetIconFile() : "";
		char priceBuf[64];
		sprintf_s(priceBuf, "%s", StringSplitNum(pBuy->GetPrice()));
		if (pBuy->GetIsPile()) {
			g_rmlTradePending = RmlTradePending::BuyQty;
			CRmlUiNpcTradeForm::Instance().ShowQtyPrompt(true, pBuy->GetName(), priceBuf, icon,
														  pBuy->GetPrice(), nMax > 0 ? nMax : 1);
		} else {
			g_rmlTradePending = RmlTradePending::BuyConfirm;
			CRmlUiNpcTradeForm::Instance().ShowBuyConfirm(pBuy->GetName(), priceBuf, icon);
		}
		return;
	}

	if (pBuy->GetIsPile() && (_sBuy.pBox = g_stUIBox.ShowTradeBox(_BuyTradeEvent, pBuy->GetPrice(), nMax, buf))) {
		return;
	} else {
		char buf2[256] = {0};
		sprintf(buf2, RES_STRING(CL_LANGUAGE_MATCH_742), pBuy->GetName(), StringSplitNum(pBuy->GetPrice()));
		if (g_stUIBox.ShowSelectBox(_BuyEquipYesNoTradeEvent, buf2, true)) {
		}
		return;
	}

	CS_Buy(_dwNpcID, nIndex, (BYTE)pNpcGrid->GetDragIndex(), nBuyGrid, nBuyCount);
}

void CNpcTradeMgr::LocalSaleToNpc(CGoodsGrid* pNpcGrid, CGoodsGrid* pSelfGrid, int nGridID, CCommandObj* pItem) {
	CItemCommand* pSaleItem = dynamic_cast<CItemCommand*>(pItem);
	if (!pSaleItem)
		return;

	__int64 nPrice = pSaleItem->GetPrice() / 2;
	_sSale.dwNpcID = _dwNpcID;
	_sSale.nIndex = pSelfGrid->GetDragIndex();
	_sSale.pBox = nullptr;

	if (pSaleItem->GetItemInfo()->sType == 43) {
		CBoat* pBoat = g_stUIBoat.FindBoat(pSaleItem->GetData().GetDBParam(enumITEMDBP_INST_ID));
		if (pBoat) {
			nPrice = pBoat->GetCha()->getGameAttr()->get(ATTR_BOAT_PRICE) / 2;
		}
	}

	if (CRmlUiNpcTradeForm::Instance().IsVisible()) {
		const char* icon = pSaleItem->GetItemInfo() ? pSaleItem->GetItemInfo()->GetIconFile() : "";
		char priceBuf[64];
		sprintf_s(priceBuf, "%s", StringSplitNum(nPrice));
		if (pSaleItem->GetIsPile() && pSaleItem->GetTotalNum() > 1) {
			g_rmlTradePending = RmlTradePending::SaleQty;
			CRmlUiNpcTradeForm::Instance().ShowQtyPrompt(false, pSaleItem->GetName(), priceBuf, icon, nPrice,
														  pSaleItem->GetTotalNum());
		} else {
			g_rmlTradePending = RmlTradePending::SaleConfirm;
			CRmlUiNpcTradeForm::Instance().ShowSellConfirm(pSaleItem->GetName(), priceBuf, icon);
		}
		return;
	}

	if (pSaleItem->GetIsPile() && pSaleItem->GetTotalNum() > 1) {
		char buf[256] = {0};
		sprintf(buf, "%s[%s$]", pSaleItem->GetItemInfo()->szName, StringSplitNum(nPrice));
		if (_sSale.pBox = g_stUIBox.ShowTradeBox(_SaleTradeEvent, nPrice, pItem->GetTotalNum(), buf)) {
			return;
		}
	} else {
		char buf[256] = {0};
		sprintf(buf, RES_STRING(CL_LANGUAGE_MATCH_743), pSaleItem->GetName(), StringSplitNum(nPrice));
		if (g_stUIBox.ShowSelectBox(_SaleEquipYesNoTradeEvent, buf, true)) {
		}
		return;
	}

	CS_Sale(_dwNpcID, (BYTE)pSelfGrid->GetDragIndex(), pItem->GetTotalNum());
}

void CNpcTradeMgr::SellSelectedItems(CGoodsGrid* grid) {
	if (!grid) {
		return;
	}

	stNetSaleItem sItem;
	std::vector<stNetSaleItem> vItems;

	for (auto i = 0, n = grid->GetMaxNum(); i < n; ++i) {
		if (!grid->IsItemSelected(i)) {
			continue;
		}

		auto item = static_cast<CItemCommand*>(grid->GetItem(i));
		if (!item || !item->GetIsValid()) {
			continue;
		}

		if (item->IsLocked()) {
			continue;
		}

		sItem.byIndex = i;
		sItem.byCount = item->GetTotalNum();

		vItems.push_back(sItem);
	}

	CS_MultipleSale(_dwNpcID, vItems);
	grid->ResetItemSelections();
}

void CNpcTradeMgr::_BuyTradeEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	if (nMsgType != CForm::mrYes)
		return;

	stBuy& buy = g_stUINpcTrade._sBuy;
	CS_Buy(buy.dwNpcID, buy.nIndex, buy.nDragIndex, buy.nBuyGrid, buy.pBox->GetTradeNum());
}

void CNpcTradeMgr::_BuyEquipYesNoTradeEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	if (nMsgType != CForm::mrYes)
		return;

	stBuy& buy = g_stUINpcTrade._sBuy;
	CS_Buy(buy.dwNpcID, buy.nIndex, buy.nDragIndex, buy.nBuyGrid, 1);
}

void CNpcTradeMgr::_SaleTradeEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	if (nMsgType != CForm::mrYes)
		return;

	stSale& sale = g_stUINpcTrade._sSale;
	CS_Sale(sale.dwNpcID, sale.nIndex, sale.pBox->GetTradeNum());
}

void CNpcTradeMgr::_SaleEquipYesNoTradeEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	if (nMsgType != CForm::mrYes)
		return;

	stSale& sale = g_stUINpcTrade._sSale;
	CS_Sale(sale.dwNpcID, sale.nIndex, 1);
}

void CNpcTradeMgr::CloseForm() {
	if (!_IsShow && !CRmlUiNpcTradeForm::Instance().IsVisible())
		return;

	HideTradeUi(true);
}

bool CNpcTradeMgr::GetIsShow() const {
	return _IsShow || CRmlUiNpcTradeForm::Instance().IsVisible();
}

bool CNpcTradeMgr::IsTradeUiVisible() const {
	return CRmlUiNpcTradeForm::Instance().IsVisible();
}

void CNpcTradeMgr::ShowTradeUi(int initialPage) {
	if (frmNPCtrade && frmNPCtrade->GetIsShow())
		frmNPCtrade->SetIsShow(false);

	if (!CRmlUiManager::Instance().IsReady() || !CRmlUiNpcTradeForm::Instance().LoadOk()) {
		if (frmNPCtrade) {
			frmNPCtrade->SetPos(100, 100);
			frmNPCtrade->Refresh();
			frmNPCtrade->Show();
		}
		_IsShow = true;
		return;
	}

	CRmlUiNpcTradeForm& rml = CRmlUiNpcTradeForm::Instance();
	rml.SetActivePage(initialPage);
	rml.Show();
	rml.PlaceBesideInventory();
	RefreshTradeUi();
	_IsShow = true;
}

void CNpcTradeMgr::HideTradeUi(bool hideInventory) {
	_IsShow = false;
	g_rmlTradePending = RmlTradePending::None;
	CRmlUiNpcTradeForm::Instance().Hide();
	if (frmNPCtrade && frmNPCtrade->GetIsShow())
		frmNPCtrade->Close();

	if (hideInventory && g_stUIEquip.IsInventoryUiVisible())
		g_stUIEquip.HideInventoryUi();
}

void CNpcTradeMgr::RefreshTradeUi() {
	CRmlUiNpcTradeForm& rml = CRmlUiNpcTradeForm::Instance();
	if (!rml.IsVisible())
		return;

	CGoodsGrid* grids[3] = {grdNPCtradeWeapon, grdNPCtradeEquip, grdNPCtradeOther};
	const int page = rml.GetActivePage();
	CGoodsGrid* grid = (page >= 0 && page < 3) ? grids[page] : grdNPCtradeOther;
	if (!grid)
		return;

	std::vector<RmlNpcSlotView> slots;
	const int maxNum = grid->GetMaxNum();
	const int cols = grid->GetCol() > 0 ? grid->GetCol() : 4;
	slots.reserve((size_t)maxNum);
	for (int i = 0; i < maxNum; ++i) {
		RmlNpcSlotView view;
		view.id = i;
		view.page = page;
		if (CItemCommand* cmd = dynamic_cast<CItemCommand*>(grid->GetItem(i))) {
			if (CItemRecord* info = cmd->GetItemInfo()) {
				view.iconPath = info->GetIconFile();
				char priceBuf[32];
				sprintf_s(priceBuf, "%s", StringSplitNum(cmd->GetPrice()));
				view.priceText = priceBuf;
			}
		}
		slots.push_back(view);
	}
	rml.SetSlots(slots, cols);

	if (CCharacter* pCha = CGameScene::GetMainCha()) {
		char goldBuf[32];
		sprintf_s(goldBuf, "%s", StringSplitNum(pCha->getGameAttr()->get(ATTR_GD)));
		rml.SetGold(goldBuf);
	}
}

void CNpcTradeMgr::BuyFromShopSlot(int page, int shopIndex, int bagSlotHint) {
	CGoodsGrid* grids[3] = {grdNPCtradeWeapon, grdNPCtradeEquip, grdNPCtradeOther};
	if (page < 0 || page > 2 || !grids[page])
		return;
	CGoodsGrid* shop = grids[page];
	CGoodsGrid* bag = g_stUIEquip.GetGoodsGrid();
	if (!bag || shopIndex < 0)
		return;

	CCommandObj* item = shop->GetItem(shopIndex);
	if (!item)
		return;

	int bagSlot = bagSlotHint;
	if (bagSlot < 0)
		bagSlot = bag->GetFreeIndex();
	if (bagSlot < 0)
		return;

	shop->SetDragIndex(shopIndex);
	LocalBuyFromNpc(shop, bag, bagSlot, item);
}

void CNpcTradeMgr::SaleFromBagSlot(int bagIndex) {
	CGoodsGrid* bag = g_stUIEquip.GetGoodsGrid();
	if (!bag || bagIndex < 0)
		return;
	CCommandObj* item = bag->GetItem(bagIndex);
	if (!item)
		return;

	CGoodsGrid* shop = grdNPCtradeWeapon;
	if (!shop)
		shop = grdNPCtradeEquip;
	if (!shop)
		shop = grdNPCtradeOther;
	if (!shop)
		return;

	bag->SetDragIndex(bagIndex);
	LocalSaleToNpc(shop, bag, 0, item);
}

void CNpcTradeMgr::ConfirmPendingBuy(int count) {
	if (count <= 0)
		return;
	CS_Buy(_sBuy.dwNpcID, _sBuy.nIndex, _sBuy.nDragIndex, _sBuy.nBuyGrid, count);
}

void CNpcTradeMgr::ConfirmPendingSale(int count) {
	if (count <= 0)
		return;
	CS_Sale(_sSale.dwNpcID, _sSale.nIndex, count);
}

void CNpcTradeMgr::CancelPendingTrade() {
	_sBuy.pBox = nullptr;
	_sSale.pBox = nullptr;
}

void CNpcTradeMgr::ApplyShopItemHint(int page, int shopIndex, int mouseX, int mouseY) {
	CGoodsGrid* grids[3] = {grdNPCtradeWeapon, grdNPCtradeEquip, grdNPCtradeOther};
	if (page < 0 || page > 2 || !grids[page] || shopIndex < 0)
		return;
	CItemCommand* cmd = dynamic_cast<CItemCommand*>(grids[page]->GetItem(shopIndex));
	if (!cmd)
		return;
	CGuiData::SetHintItem(cmd);
	cmd->ReadyForHint(mouseX, mouseY, nullptr);
}

void RmlNpcTrade_OnClose() {
	g_stUINpcTrade.HideTradeUi(true);
}

void RmlNpcTrade_OnTab(int page) {
	CRmlUiNpcTradeForm::Instance().SetActivePage(page);
	g_stUINpcTrade.RefreshTradeUi();
}

void RmlNpcTrade_OnBuy(int page, int shopIndex, int bagSlot) {
	g_stUINpcTrade.BuyFromShopSlot(page, shopIndex, bagSlot);
}

void RmlNpcTrade_OnDrop(int srcPage, int srcShop, int srcBag, int dstBag) {
	if (srcPage >= 0 && srcShop >= 0 && dstBag >= 0)
		g_stUINpcTrade.BuyFromShopSlot(srcPage, srcShop, dstBag);
	else if (srcBag >= 0)
		g_stUINpcTrade.SaleFromBagSlot(srcBag);
}

void RmlNpcTrade_OnDragEnd(int srcPage, int srcShop, int mouseX, int mouseY) {
	if (srcPage < 0 || srcShop < 0)
		return;
	float x = 0.f, y = 0.f, w = 0.f, h = 0.f;
	if (CRmlUiInventoryForm::Instance().GetRootScreenRect(x, y, w, h) && w > 0.f) {
		if (mouseX >= (int)x && mouseY >= (int)y && mouseX <= (int)(x + w) && mouseY <= (int)(y + h))
			g_stUINpcTrade.BuyFromShopSlot(srcPage, srcShop, -1);
	}
}

void RmlNpcTrade_OnBagSell(int bagIndex) {
	g_stUINpcTrade.SaleFromBagSlot(bagIndex);
}

void RmlNpcTrade_OnConfirmYes() {
	const RmlTradePending pending = g_rmlTradePending;
	g_rmlTradePending = RmlTradePending::None;
	if (pending == RmlTradePending::BuyConfirm)
		g_stUINpcTrade.ConfirmPendingBuy(1);
	else if (pending == RmlTradePending::SaleConfirm)
		g_stUINpcTrade.ConfirmPendingSale(1);
}

void RmlNpcTrade_OnConfirmNo() {
	g_rmlTradePending = RmlTradePending::None;
	g_stUINpcTrade.CancelPendingTrade();
}

void RmlNpcTrade_OnQtyYes() {
	const int qty = CRmlUiNpcTradeForm::Instance().GetQtyValue();
	const RmlTradePending pending = g_rmlTradePending;
	g_rmlTradePending = RmlTradePending::None;
	if (pending == RmlTradePending::BuyQty)
		g_stUINpcTrade.ConfirmPendingBuy(qty);
	else if (pending == RmlTradePending::SaleQty)
		g_stUINpcTrade.ConfirmPendingSale(qty);
}

void RmlNpcTrade_OnQtyNo() {
	g_rmlTradePending = RmlTradePending::None;
	g_stUINpcTrade.CancelPendingTrade();
}

void RmlNpcTrade_OnQtyChanged(int qty) {
	(void)qty;
}

void RmlNpcTrade_ApplyItemHint(int page, int shopIndex, int mouseX, int mouseY) {
	g_stUINpcTrade.ApplyShopItemHint(page, shopIndex, mouseX, mouseY);
}

void RmlNpcTrade_RenderItemHint() {
	if (CItemObj* item = CGuiData::GetHintItem())
		item->RenderHint(0, 0);
}
