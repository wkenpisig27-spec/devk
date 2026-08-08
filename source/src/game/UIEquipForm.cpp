#include "StdAfx.h"
#include "uiequipform.h"
#include "uiform.h"
#include "uiskilllist.h"
#include "packetcmd.h"
#include "Scene.h"
#include "Character.h"
#include "uifastcommand.h"
#include "uigoodsgrid.h"
#include "NetProtocol.h"
#include "gameapp.h"
#include "uiitemcommand.h"
#include "uilabel.h"
#include "tools.h"
#include "uitradeform.h"
#include "uiboxform.h"
#include "packetcmd.h"
#include "netprotocol.h"
#include "uiboxform.h"
#include "uiboatform.h"
#include "chastate.h"
#include "SkillStateRecord.h"
#include "ProCirculate.h"
#include "stpose.h"
#include "uiboxform.h"
#include "StringLib.h"
#include "UICheckBox.h"
#include "UIDoublePwdForm.h"
#include "UIStoreForm.h"
#include "GlobalVar.h"
#include "EffectObj.h"
#include "UIMenu.h"
#include "UINpcTradeForm.h"
#include "UIFormMgr.h"
#include "WorldScene.h"
#include "UICozeForm.h"
#include <algorithm>
#include <string>

#include "rmlui/RmlUiInventoryForm.h"
#include "rmlui/RmlUiManager.h"

using namespace std;

using namespace GUI;

namespace {

enum class RmlInvPendingConfirm {
	None,
	DeleteSelected,
	ThrowSelected,
	LockSelected,
	SellSelected,
	ThrowSingle,
};

RmlInvPendingConfirm g_rmlInvPendingConfirm = RmlInvPendingConfirm::None;
int g_rmlInvPendingThrowNum = 1;

bool ShowRmlItemConfirm(const char* title, const char* message, const char* hint, RmlInvPendingConfirm action) {
	if (!CRmlUiInventoryForm::Instance().IsVisible())
		return false;
	g_rmlInvPendingConfirm = action;
	CRmlUiInventoryForm::Instance().ShowConfirmModal(title, message, hint);
	return true;
}

} // namespace

namespace {

class CChestPreviewItemCommand : public CItemCommand {
public:
	explicit CChestPreviewItemCommand(CItemRecord* item) : CItemCommand(item) {}

	void Render(int x, int y) override {
		CItemCommand::Render(x, y);

		const char* rateText = GetOwnDefText();
		if (!rateText || !rateText[0]) {
			return;
		}

		int width = 0;
		int height = 0;
		CGuiFont::s_Font.GetSize(rateText, width, height);
		const int textX = x + ((32 - width) / 2);
		const int textY = y + 32 - height;
		GetRender().FillFrame(textX, textY, textX + width, textY + height, 0xE0ADF6F7);
		CGuiFont::s_Font.Render(rateText, textX, textY, COLOR_BLACK);
	}
};

std::string BuildChestPreviewTitle(int chestID) {
	auto* chestInfo = GetItemRecordInfo(chestID);
	if (!chestInfo) {
		return "Chest Preview";
	}

	return std::string(chestInfo->szName) + " Rates";
}

} // namespace

static char szBuf[32] = {0};

CGuiPic CEquipMgr::_imgCharges[enumEQUIP_NUM];
CEquipMgr::SSplitItem CEquipMgr::SSplit;
int CEquipMgr::lIMP = 0;

bool g_IsNumberTopBar{false};

extern CDoublePwdMgr g_stUIDoublePwd;

void CEquipMgr::ThrowSelectedItems(CGoodsGrid* grid) {
	if (!grid) {
		return;
	}

	auto pSelf = g_stUIBoat.FindCha(grid);
	if (!pSelf) {
		return;
	}

	stNetItemThrow sItemThrow;
	sItemThrow.lPosX = CGameApp::GetCurScene()->GetMouseMapX() * 100.0f;
	sItemThrow.lPosY = CGameApp::GetCurScene()->GetMouseMapY() * 100.0f;

	if (!g_stUIEquip._GetThrowPos(reinterpret_cast<int&>(sItemThrow.lPosX), reinterpret_cast<int&>(sItemThrow.lPosY))) {
		return;
	}

	for (auto i = 0, n = grid->GetMaxNum(); i < n; ++i) {
		if (!grid->IsItemSelected(i)) {
			continue;
		}

		auto item = grid->GetItem(i);
		if (!item || !item->GetIsValid()) {
			continue;
		}

		sItemThrow.lNum = item->GetTotalNum();
		sItemThrow.sGridID = i;
		CS_BeginAction(pSelf, enumACTION_ITEM_THROW, reinterpret_cast<void*>(&sItemThrow));
	}

	grid->ResetItemSelections();
}

void CEquipMgr::LockSelectedItems(CGoodsGrid* grid) {
	if (!grid) {
		return;
	}

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

		stNetLockItem info;
		info.sGridID = i;
		CS_BeginAction(g_stUIBoat.GetHuman(), enumACTION_ITEM_LOCK, &info);
	}

	grid->ResetItemSelections();
}

void CEquipMgr::DeleteSelectedItems(CGoodsGrid* grid) {
	if (!grid) {
		return;
	}

	for (auto i = 0, n = grid->GetMaxNum(); i < n; ++i) {
		if (!grid->IsItemSelected(i)) {
			continue;
		}

		auto item = static_cast<CItemCommand*>(grid->GetItem(i));
		if (!item) {
			continue;
		}

		// Allow deletion of items marked invalid due to low durability (red items).
		// Items invalidated by UI operations (trade/forge/compose) are still protected
		// because those systems lock the kitbag, which the server checks independently.
		if (!item->GetIsValid() && item->GetData().sEndure[0] > 49) {
			continue;
		}

		if (!item->GetItemInfo()->chIsDel) {
			continue;
		}

		stNetDelItem info;
		info.sGridID = i;
		CS_BeginAction(g_stUIBoat.GetHuman(), enumACTION_ITEM_DELETE, &info);
	}

	grid->ResetItemSelections();
}

//---------------------------------------------------------------------------
// class CMainMgr
//---------------------------------------------------------------------------
bool CEquipMgr::Init() {
	CForm* frmMain800 = _FindForm("frmMain800");

	///////////???????
	frmSkill = _FindForm("frmSkill");
	if (!frmSkill)
		return false;
	frmSkill->evtShow = _evtSkillFormShow;

	lstFightSkill = dynamic_cast<CSkillList*>(frmSkill->Find("lstSkill"));
	if (!lstFightSkill)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmSkill->GetName(), "lstSkill");
	lstFightSkill->evtUpgrade = _evtSkillUpgrade;

	lstLifeSkill = dynamic_cast<CSkillList*>(frmSkill->Find("lstSkillW"));
	if (!lstLifeSkill)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmSkill->GetName(), "lstSkillW");
	lstLifeSkill->evtUpgrade = _evtSkillUpgrade;

	lstSailSkill = dynamic_cast<CSkillList*>(frmSkill->Find("lstSkillS"));
	if (!lstSailSkill)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmSkill->GetName(), "lstSkillS");
	lstSailSkill->evtUpgrade = _evtSkillUpgrade;
	lstSailSkill->SetIsShowUpgrade(false);

	labPoint = dynamic_cast<CLabel*>(frmSkill->Find("labPoint"));
	if (!labPoint)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmSkill->GetName(), "labPoint");

	labPointLife = dynamic_cast<CLabel*>(frmSkill->Find("labPoint1"));
	if (!labPointLife)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmSkill->GetName(), "labPoint1");

	frmInv = _FindForm("frmInv");

	if (!frmInv)
		return false;
	frmInv->evtShow = _evtEquipFormShow;
	frmInv->evtClose = _evtEquipFormClose;
	frmInv->evtEntrustMouseEvent = _evtItemFormMouseEvent;

	frmChestPreview = _FindForm("frmChestPreview");
	if (frmChestPreview) {
		grdChestPreview = dynamic_cast<CGoodsGrid*>(frmChestPreview->Find("grdChestPreview"));
		labChestPreviewTitle = dynamic_cast<CLabelEx*>(frmChestPreview->Find("labChestPreviewTitle"));

		if (grdChestPreview) {
			grdChestPreview->SetShowStyle(CGoodsGrid::enumSmall);
		}

		ClearChestPreview();
		frmChestPreview->SetIsShow(false);
	}

	grdItem = dynamic_cast<CGoodsGrid*>(frmInv->Find("grdItem"));
	if (!grdItem)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmInv->GetName(), "grdItem");

	lblGold = dynamic_cast<CLabel*>(frmInv->Find("labItemgoldnumber"));
	if (!lblGold)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmInv->GetName(), "labItemgoldnumber");

	GetGoodsGrid()->SetSelectEnable(true);
	GetGoodsGrid()->evtThrowItem = evtThrowItemEvent;
	GetGoodsGrid()->evtSwapItem = evtSwapItemEvent;
	GetGoodsGrid()->evtBeforeAccept = _evtDragToGoodsEvent;
	GetGoodsGrid()->evtRMouseEvent = _evtRMouseGridEvent;

	imgLock = dynamic_cast<CImage*>(frmInv->Find("imgLock"));
	if (!imgLock)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmInv->GetName(), "imgLock");

	imgUnLock = dynamic_cast<CImage*>(frmInv->Find("imgUnLock"));
	if (!imgUnLock)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmInv->GetName(), "imgUnLock");

	SetIsLock(false); // I???????

	///////////////???????
	CForm* frmFast = _FindForm("frmFast");
	if (!frmFast)
		return false;

	// ???�????t
	CTextButton* btnFastUp = dynamic_cast<CTextButton*>(frmFast->Find("btnFastUp"));
	if (!btnFastUp)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmMain800->GetName(), "btnFastUp");
	btnFastUp->evtMouseClick = _evtButtonClickEvent;

	CTextButton* btnFastDown = dynamic_cast<CTextButton*>(frmFast->Find("btnFastDown"));
	if (!btnFastUp)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmMain800->GetName(), "btnFastDown");
	btnFastDown->evtMouseClick = _evtButtonClickEvent;

	CForm* frmFast2 = _FindForm("frmFast2");
	if (!frmFast2)
		return false;

	_pFastCommands = new CFastCommand*[SHORT_CUT_NUM];
	memset(_pFastCommands, 0, sizeof(CFastCommand*) * SHORT_CUT_NUM);
	char szName[30] = {0};

	for (auto i = 0; i < MAX_FAST_COL; ++i) {
		sprintf(szName, "fscMainF%d", i);
		_pFastCommands[i] = dynamic_cast<CFastCommand*>(frmFast->Find(szName));
		if (_pFastCommands[i]) {
			_pFastCommands[i]->nTag = i;
			_pFastCommands[i]->evtChange = _evtFastChange;
		}
	}

	for (auto row = 1; row < MAX_FAST_ROW - 1; ++row) {
		for (auto i = 0; i < MAX_FAST_COL; ++i) {
			auto index = row * MAX_FAST_COL + i;
			_pFastCommands[index] = new CFastCommand(*_pFastCommands[i]);
			_pFastCommands[index]->AddForm();

			_pFastCommands[index]->nTag = index;
			_pFastCommands[index]->evtChange = _evtFastChange;
		}
	}

	for (auto i = MAX_FAST_COL * 2; i < MAX_FAST_COL * 3; ++i) {
		sprintf(szName, "fscMainF2%d", i - 12);
		_pFastCommands[i] = dynamic_cast<CFastCommand*>(frmFast2->Find(szName));
		if (_pFastCommands[i]) {
			_pFastCommands[i]->nTag = i;
			_pFastCommands[i]->evtChange = _evtFastChange;
			_pFastCommands[i]->topBar = true;
		}
	}

	//_pActiveFastLabel = dynamic_cast<CLabel*>(frmMain800->Find( "labFast" ) );
	_ActiveFast(0);

	/////////////// ?????

	memset(cnmEquip, 0, sizeof(cnmEquip));
	cnmEquip[enumEQUIP_HEAD] = dynamic_cast<COneCommand*>(frmInv->Find("cmdArmet"));
	cnmEquip[enumEQUIP_BODY] = dynamic_cast<COneCommand*>(frmInv->Find("cmdBody"));
	cnmEquip[enumEQUIP_GLOVE] = dynamic_cast<COneCommand*>(frmInv->Find("cmdGlove"));
	cnmEquip[enumEQUIP_SHOES] = dynamic_cast<COneCommand*>(frmInv->Find("cmdShoes"));
	cnmEquip[enumEQUIP_LHAND] = dynamic_cast<COneCommand*>(frmInv->Find("cmdLeftHand"));
	cnmEquip[enumEQUIP_RHAND] = dynamic_cast<COneCommand*>(frmInv->Find("cmdRightHand"));
	cnmEquip[enumEQUIP_NECK] = dynamic_cast<COneCommand*>(frmInv->Find("cmdNecklace"));
	cnmEquip[enumEQUIP_HAND1] = dynamic_cast<COneCommand*>(frmInv->Find("cmdCirclet1"));
	cnmEquip[enumEQUIP_HAND2] = dynamic_cast<COneCommand*>(frmInv->Find("cmdCirclet2"));
	cnmEquip[enumEQUIP_Jewelry1] = dynamic_cast<COneCommand*>(frmInv->Find("cmdJewelry1"));
	cnmEquip[enumEQUIP_Jewelry2] = dynamic_cast<COneCommand*>(frmInv->Find("cmdJewelry2"));
	cnmEquip[enumEQUIP_Jewelry3] = dynamic_cast<COneCommand*>(frmInv->Find("cmdJewelry3"));
	cnmEquip[enumEQUIP_Jewelry4] = dynamic_cast<COneCommand*>(frmInv->Find("cmdJewelry4"));
	cnmEquip[enumEQUIP_WING] = dynamic_cast<COneCommand*>(frmInv->Find("cmdWing"));

	cnmEquip[enumEQUIP_CLOAK] = dynamic_cast<COneCommand*>(frmInv->Find("cmdCloak"));
	cnmEquip[enumEQUIP_FAIRY] = dynamic_cast<COneCommand*>(frmInv->Find("cmdPet"));
	cnmEquip[enumEQUIP_REAR] = dynamic_cast<COneCommand*>(frmInv->Find("cmdRearPet"));
	cnmEquip[enumEQUIP_MOUNT] = dynamic_cast<COneCommand*>(frmInv->Find("cmdMount"));

	cnmEquip[enumEQUIP_HEADAPP] = dynamic_cast<COneCommand*>(frmInv->Find("cmdArmetApp"));
	cnmEquip[enumEQUIP_FACEAPP] = dynamic_cast<COneCommand*>(frmInv->Find("cmdFaceApp"));
	cnmEquip[enumEQUIP_BODYAPP] = dynamic_cast<COneCommand*>(frmInv->Find("cmdBodyApp"));
	cnmEquip[enumEQUIP_GLOVEAPP] = dynamic_cast<COneCommand*>(frmInv->Find("cmdGloveApp"));
	cnmEquip[enumEQUIP_SHOESAPP] = dynamic_cast<COneCommand*>(frmInv->Find("cmdShoesApp"));
	cnmEquip[enumEQUIP_FAIRYAPP] = dynamic_cast<COneCommand*>(frmInv->Find("cmdPetApp"));
	cnmEquip[enumEQUIP_GLOWAPP] = dynamic_cast<COneCommand*>(frmInv->Find("cmdGlowApp"));
	cnmEquip[enumEQUIP_DAGGERAPP] = dynamic_cast<COneCommand*>(frmInv->Find("cmdDaggerApp"));
	cnmEquip[enumEQUIP_GUNAPP] = dynamic_cast<COneCommand*>(frmInv->Find("cmdGunApp"));
	cnmEquip[enumEQUIP_SWORD1APP] = dynamic_cast<COneCommand*>(frmInv->Find("cmdSword1App"));
	cnmEquip[enumEQUIP_GREATSWORDAPP] = dynamic_cast<COneCommand*>(frmInv->Find("cmdGreatSwordApp"));
	cnmEquip[enumEQUIP_STAFFAPP] = dynamic_cast<COneCommand*>(frmInv->Find("cmdStaffApp"));
	cnmEquip[enumEQUIP_BOWAPP] = dynamic_cast<COneCommand*>(frmInv->Find("cmdBowApp"));
	cnmEquip[enumEQUIP_SWORD2APP] = dynamic_cast<COneCommand*>(frmInv->Find("cmdSword2App"));
	cnmEquip[enumEQUIP_SHIELDAPP] = dynamic_cast<COneCommand*>(frmInv->Find("cmdShieldApp"));

	for (int i = 0; i < enumEQUIP_NUM; i++) {
		if (cnmEquip[i]) {
			cnmEquip[i]->evtBeforeAccept = _evtEquipEvent;
			// cnmEquip[i]->evtMouseClick = _UnequipPart;
			// cnmEquip[i]->evtThrowItem = _evtThrowEquipEvent;	// ???????????????

			cnmEquip[i]->SetActivePic(&_imgCharges[i]);
		}
	}

	int nTotalSkill = CSkillRecordSet::I()->GetLastID() + 1;
	CSkillRecord* pInfo;
	for (int i = 0; i < nTotalSkill; i++) {
		pInfo = GetSkillRecordInfo(i);
		if (pInfo) {
			if (pInfo->nStateID)
				_cancels.push_back(pInfo);
		}
	}

	int nTotalState = CSkillStateRecordSet::I()->GetLastID() + 1;
	CSkillStateRecord* pState;
	for (int i = 0; i < nTotalState; i++) {
		pState = GetCSkillStateRecordInfo(i);
		if (pState) {
			if (pState->sChargeLink >= 0 && pState->sChargeLink < enumEQUIP_NUM) {
				_charges.push_back(pState);
			}
		}
	}

	frmItemSpy = _FindForm("frmItemSpy"); // ???????
	frmItemSpy->evtShow = _evtSpyFormShow;
	frmItemSpy->evtClose = _evtSpyFormClose;
	if (!frmItemSpy)
		return false;
	// frmItemSpy->evtShow = _evtItemFormShow;
	// frmItemSpy->evtClose = _evtItemFormClose;
	// frmItemSpy->evtEntrustMouseEvent = _evtItemFormMouseEvent;

	/////////////// ?????
	memset(cnmEquipSpy, 0, sizeof(cnmEquipSpy));
	cnmEquipSpy[enumEQUIP_HEAD] = dynamic_cast<COneCommand*>(frmItemSpy->Find("cmdArmet"));
	cnmEquipSpy[enumEQUIP_BODY] = dynamic_cast<COneCommand*>(frmItemSpy->Find("cmdBody"));
	cnmEquipSpy[enumEQUIP_GLOVE] = dynamic_cast<COneCommand*>(frmItemSpy->Find("cmdGlove"));
	cnmEquipSpy[enumEQUIP_SHOES] = dynamic_cast<COneCommand*>(frmItemSpy->Find("cmdShoes"));
	cnmEquipSpy[enumEQUIP_LHAND] = dynamic_cast<COneCommand*>(frmItemSpy->Find("cmdLeftHand"));
	cnmEquipSpy[enumEQUIP_RHAND] = dynamic_cast<COneCommand*>(frmItemSpy->Find("cmdRightHand"));
	cnmEquipSpy[enumEQUIP_NECK] = dynamic_cast<COneCommand*>(frmItemSpy->Find("cmdNecklace"));
	cnmEquipSpy[enumEQUIP_HAND1] = dynamic_cast<COneCommand*>(frmItemSpy->Find("cmdCirclet1"));
	cnmEquipSpy[enumEQUIP_HAND2] = dynamic_cast<COneCommand*>(frmItemSpy->Find("cmdCirclet2"));
	cnmEquipSpy[enumEQUIP_Jewelry1] = dynamic_cast<COneCommand*>(frmItemSpy->Find("cmdJewelry1"));
	cnmEquipSpy[enumEQUIP_Jewelry2] = dynamic_cast<COneCommand*>(frmItemSpy->Find("cmdJewelry2"));
	cnmEquipSpy[enumEQUIP_Jewelry3] = dynamic_cast<COneCommand*>(frmItemSpy->Find("cmdJewelry3"));
	cnmEquipSpy[enumEQUIP_Jewelry4] = dynamic_cast<COneCommand*>(frmItemSpy->Find("cmdJewelry4"));
	cnmEquipSpy[enumEQUIP_WING] = dynamic_cast<COneCommand*>(frmItemSpy->Find("cmdWing"));

	cnmEquipSpy[enumEQUIP_CLOAK] = dynamic_cast<COneCommand*>(frmItemSpy->Find("cmdCloak"));
	cnmEquipSpy[enumEQUIP_FAIRY] = dynamic_cast<COneCommand*>(frmItemSpy->Find("cmdPet"));
	cnmEquipSpy[enumEQUIP_REAR] = dynamic_cast<COneCommand*>(frmItemSpy->Find("cmdRearPet"));
	cnmEquipSpy[enumEQUIP_MOUNT] = dynamic_cast<COneCommand*>(frmItemSpy->Find("cmdMount"));

	C3DCompent* ui3dEqSpyCha = (C3DCompent*)frmItemSpy->Find("ui3dEqSpyCha");
	if (ui3dEqSpyCha) {
		ui3dEqSpyCha->SetRenderEvent(_EqSpyRenderEvent);
	}

	CTextButton* btnLeft3d = (CTextButton*)frmItemSpy->Find("btnLeft3d");
	if (!btnLeft3d) {
		Error(RES_STRING(CL_LANGUAGE_MATCH_45),
			  frmItemSpy->GetName(), "btnLeft3d");
		return false;
	}
	btnLeft3d->evtMouseClick = _RotateSpyLeft;
	btnLeft3d->evtMouseDownContinue = _RotateSpyLeftContinue;

	CTextButton* btnRight3d = (CTextButton*)frmItemSpy->Find("btnRight3d");
	if (!btnRight3d) {
		Error(RES_STRING(CL_LANGUAGE_MATCH_45),
			  frmItemSpy->GetName(), "btnRight3d");
		return false;
	}
	btnRight3d->evtMouseClick = _RotateSpyRight;
	btnRight3d->evtMouseDownContinue = _RotateSpyRightContinue;

	C3DCompent* ui3dCha = (C3DCompent*)frmInv->Find("ui3dCha");
	if (ui3dCha) {
		ui3dCha->SetRenderEvent(_ChaRenderEvent);
	}

	CTextButton* btnChaLeft3d = (CTextButton*)frmInv->Find("btnChaLeft3d");
	btnChaLeft3d->evtMouseClick = _RotateChaLeft;
	btnChaLeft3d->evtMouseDownContinue = _RotateChaLeftContinue;

	CTextButton* btnChaRight3d = (CTextButton*)frmInv->Find("btnChaRight3d");
	btnChaRight3d->evtMouseClick = _RotateChaRight;
	btnChaRight3d->evtMouseDownContinue = _RotateChaRightContinue;

	btnOpenTempBag = (CTextButton*)frmInv->Find("btnOpenTempBag");
	btnOpenTempBag->evtMouseClick = _ClickTempBag;

	if (rightClickItemMenu = CMenu::FindMenu("inventoryItemRightClick"); rightClickItemMenu) {
		rightClickItemMenu->evtListMouseDown = [](CGuiData* pSender, int x, int y, DWORD key) {
			auto menu = static_cast<CMenu*>(pSender);
			menu->SetIsShow(false);
			if (!menu->GetSelectMenu())
				return;

			constexpr std::string_view menu_string[] = {"Delete Item", "Throw Item", "Lock Item", "Unlock Item", "Sell Item", "Box Rates", "Send to Chat"};
			constexpr const char* actions[] = {"delete", "throw", "lock", "unlock", "sell", "boxrates", "chat"};
			const std::string_view selected = menu->GetSelectMenu()->GetString();
			for (size_t i = 0; i < 7; ++i) {
				if (selected == menu_string[i]) {
					g_stUIEquip.ExecuteBagContextAction(actions[i]);
					return;
				}
			}
		};
	}

	stateDrags = _FindForm("stateDrags");
	if (!stateDrags) {
		return false;
	}
	stateDrags->evtMouseDragEnd = _OnDragStates;

	return true;
}

void CEquipMgr::_ClickTempBag(CGuiData* pSender, int x, int y, DWORD key) {
	string name = pSender->GetName();
	g_stUIStore.ShowTempKitbag();
}

void CEquipMgr::_UnequipPart(CGuiData* pSender, int x, int y, DWORD key) {
	if (!CGameScene::GetMainCha())
		return;

	string name = pSender->GetName();
	int linkID = 0;
	if (name == "eqhelm") {
		linkID = 0;
	} else if (name == "eqboot") {
		linkID = 4;
	} else if (name == "eqglove") {
		linkID = 3;
	} else if (name == "eqbody") {
		linkID = 2;
	} else {
		return;
	}

	stNetItemUnfix item;
	item.chLinkID = linkID;
	item.sGridID = 0;

	if (g_stUIBoat.GetHuman() == CGameScene::GetMainCha()) {
		CActor* pActor = g_stUIBoat.GetHuman()->GetActor();
		CEquipState* pState = new CEquipState(pActor);
		pState->SetUnfixData(item);
		pActor->SwitchState(pState);
	} else {
		CS_BeginAction(g_stUIBoat.GetHuman(), enumACTION_ITEM_UNFIX, (void*)&item);
	}
}

void CEquipMgr::End() {
	// RefreshServerShortCut(); Human??????
}

void CEquipMgr::_evtSkillUpgrade(CSkillList* pSender, CSkillCommand* pSkill) {
	if (!pSkill)
		return;

	int nSkillID = pSkill->GetSkillID();
	g_NetIF->GetProCir()->SkillUpgrade(nSkillID, 1);
	return;
}

void CEquipMgr::LoadingCall() {
	CCharacter* pMain = CGameScene::GetMainCha();
	if (pMain) {
		LONG64 nJob = pMain->getGameAttr()->get(ATTR_JOB);

		for (int i = 0; i < 4; i++) {
			CSkillList* pSkillList = GetSkillList(i);
			if (!pSkillList)
				continue;

			int nCount = pSkillList->GetCount();
			CSkillCommand* pObj = nullptr;
			for (int j = 0; j < nCount; j++) {
				pObj = pSkillList->GetCommand(j);
				if (pObj) {
					pObj->GetSkillRecord()->Refresh(nJob);
				}
			}
		}
	}
}

void CEquipMgr::SynSkillBag(DWORD dwCharID, stNetSkillBag* pSSkillBag) {
	CCharacter* pCha = g_stUIBoat.GetHuman();
	if (!pCha || pCha->getAttachID() != dwCharID) {
		LG("protocol", RES_STRING(CL_LANGUAGE_MATCH_547));
		return;
	}

	LONG64 nJob = pCha->getGameAttr()->get(ATTR_JOB);
	int nCount = pSSkillBag->SBag.GetCount();
	SSkillGridEx* pSBag = pSSkillBag->SBag.GetValue();
	switch (pSSkillBag->chType) {
	case enumSYN_SKILLBAG_INIT: // ????????'????????????l????????
	{
		lstSailSkill->Clear();
		lstFightSkill->Clear();
		lstLifeSkill->Clear();

		_skills.clear();

		CSkillRecord* pInfo = nullptr;
		CSkillCommand* tmp = nullptr;
		for (int i = 0; i < nCount; i++) {
			pInfo = GetSkillRecordInfo(pSBag[i].sID);
			if (!pInfo) {
				LG("protocol", RES_STRING(CL_LANGUAGE_MATCH_548), pSBag[i].sID);
				continue;
			}
			pInfo->GetSkillGrid() = pSBag[i];
			pInfo->Refresh(nJob);
			if (!pInfo->IsShow())
				continue;

			tmp = new CSkillCommand(pInfo);
			GetSkillList(pInfo->chFightType)->AddCommand(tmp);

			_skills.push_back(pInfo);
		}
	} break;
	case enumSYN_SKILLBAG_ADD: // ???????????????????l?????????
	{
		CSkillRecord* pInfo = nullptr;
		CSkillCommand* tmp = nullptr;
		for (int i = 0; i < nCount; i++) {
			pInfo = GetSkillRecordInfo(pSBag[i].sID);
			if (!pInfo) {
				LG("protocol", RES_STRING(CL_LANGUAGE_MATCH_549), pSBag[i].sID);
				continue;
			}
			pInfo->GetSkillGrid() = pSBag[i];
			pInfo->Refresh(nJob);
			if (!pInfo->IsShow())
				continue;

			tmp = new CSkillCommand(pInfo);

			GetSkillList(pInfo->chFightType)->AddCommand(tmp);
			_skills.push_back(pInfo);
		}
	} break;
	case enumSYN_SKILLBAG_MODI: // ??l???
	{
		CSkillRecord* pInfo = nullptr;
		CSkillCommand* tmp = nullptr;
		for (int i = 0; i < nCount; i++) {
			pInfo = GetSkillRecordInfo(pSBag[i].sID);
			if (!pInfo) {
				LG("protocol", RES_STRING(CL_LANGUAGE_MATCH_550), pSBag[i].sID);
				continue;
			}
			pInfo->GetSkillGrid() = pSBag[i];
			pInfo->Refresh(nJob);
			if (!pInfo->IsShow())
				continue;

			if (pSBag[i].chLv == 0 && !GetSkillList(pInfo->chFightType)->DelSkill(pSBag[i].sID)) {
				LG("protocol", RES_STRING(CL_LANGUAGE_MATCH_551), pSBag[i].sID);
				continue;
			}
		}
	} break;
	default:
		LG("protocol", RES_STRING(CL_LANGUAGE_MATCH_552), pSSkillBag->chType);
		return;
	}

	lstSailSkill->Refresh();
	lstFightSkill->Refresh();
	lstLifeSkill->Refresh();
}

CSkillRecord* CEquipMgr::FindSkill(int nID) {
	for (vskill::iterator it = _skills.begin(); it != _skills.end(); it++)
		if ((*it)->nID == nID)
			return *it;

	return nullptr;
}

// defItemShortCutType
void CEquipMgr::FastChange(int nIndex, short sGridID, char chType, bool update) {
	if (update) {
		_pFastCommands[nIndex]->AddCommand(GetGoodsGrid()->GetItem(sGridID));
	} else {
		stNetShortCutChange param;
		memset(&param, 0, sizeof(param));
		param.chIndex = nIndex;
		param.chType = chType;
		param.shyGrid = sGridID;

		/*
		if(_pFastCommands[ nIndex ]->GetCommand2()){
			param.shyGrid2 = _pFastCommands[ nIndex ]->GetCommand2()->nTag;
		}
		*/

		CS_BeginAction(g_stUIBoat.GetHuman(), enumACTION_SHORTCUT, (void*)&param);
		stNetShortCut& SCut = _stShortCut;
		SCut.chType[nIndex] = chType;
		SCut.byGridID[nIndex] = sGridID;
	}
}

void CEquipMgr::_evtFastChange(CGuiData* pSender, CCommandObj* pItem, bool& isAccept) {
	CGoodsGrid* pGrid = dynamic_cast<CGoodsGrid*>(CDrag::GetParent());
	if (pGrid && pGrid != g_stUIEquip.GetGoodsGrid())
		return;

	CFastCommand* pFast = dynamic_cast<CFastCommand*>(pSender);
	if (!pFast)
		return;

	isAccept = true;

	char chType = 0;
	short sGridID = 0;
	int nIndex = pFast->nTag;
	if (!g_stUIEquip._GetCommandShortCutType(pItem, chType, sGridID)) {
		isAccept = false;
		g_pGameApp->SysInfo(RES_STRING(CL_LANGUAGE_MATCH_553), pItem->GetName());
		return;
	}
	g_stUIEquip.FastChange(nIndex, sGridID, chType);
}

void CEquipMgr::UpdataEquipData(const stNetChangeChaPart& SPart, CCharacter* pCha) {
	// ID?????????,???????
	CItemCommand* pItem = nullptr;
	for (int i = 0; i < enumEQUIP_NUM; i++) {
		// Modify by lark.li 20080818 begin
		// if( SPart.SLink[i].sID==0 ) continue;
		if (cnmEquip[i] == nullptr || SPart.SLink[i].sID == 0)
			continue;
		// End

		pItem = dynamic_cast<CItemCommand*>(cnmEquip[i]->GetCommand());
		if (pItem && pItem->GetItemInfo()->lID == SPart.SLink[i].sID) {
			SItemGrid& Item = pItem->GetData();
			Item.bValid = SPart.SLink[i].bValid;
			Item.sEndure[0] = SPart.SLink[i].sEndure[0];
			Item.sEnergy[0] = SPart.SLink[i].sEnergy[0];
			Item.bItemTradable = SPart.SLink[i].bItemTradable;
			Item.expiration = SPart.SLink[i].expiration;

		} else {
			LG("error", RES_STRING(CL_LANGUAGE_MATCH_554), (pItem ? pItem->GetItemInfo()->lID : 0), SPart.SLink[i].sID);
		}
	}
}

void CEquipMgr::UpdataEquip(const stNetChangeChaPart& SPart, CCharacter* pCha) {
	// I?????????????
	memcpy(&stEquip, &SPart, sizeof(SPart));
	for (int i = 0; i < enumEQUIP_PART_NUM; i++) {
		if (stEquip.SLink[i].sID == pCha->GetDefaultChaInfo()->sSkinInfo[i])
			stEquip.SLink[i].sID = 0;
	}
	// for( int i=enumEQUIP_PART_NUM; i<enumEQUIP_NUM; i++ )
	//{
	//     if( stEquip.SLink[i].sID==enumEQUIP_BOTH_HAND )
	//         stEquip.SLink[i].sID = 0;
	// }

	// ????UI?????
	for (int i = 0; i < enumEQUIP_NUM; i++) {
		_UpdataEquip(stEquip.SLink[i], i);
	}
	if (IsInventoryUiVisible())
		MarkChaPreviewDirty();
	RefreshRmlInventory();
}

void CEquipMgr::UpdataEquipSpy(const stNetChangeChaPart& SPart, CCharacter* pCha) {
	// I?????????????
	eqSpyTarget = pCha;

	CLabel* labTitle = (CLabel*)frmItemSpy->Find("labTitle");
	char buf[64];
	sprintf(buf, "Stalk - %s", pCha->getName());
	labTitle->SetCaption(buf);

	memcpy(&stEquipSpy, &SPart, sizeof(SPart));
	for (int i = 0; i < enumEQUIP_PART_NUM; i++) {
		if (stEquipSpy.SLink[i].sID == pCha->GetDefaultChaInfo()->sSkinInfo[i])
			stEquipSpy.SLink[i].sID = 0;
	}
	for (int i = 0; i < enumEQUIP_NUM; i++) {
		_UpdataEquipSpy(stEquipSpy.SLink[i], i);
	}

	frmItemSpy->Show();
}

bool CEquipMgr::_UpdataEquipSpy(SItemGrid& Item, int nLink) {
	if (!cnmEquipSpy[nLink])
		return false;

	int nItemID = Item.sID;
	if (nItemID > 0 && nItemID != enumEQUIP_BOTH_HAND) {
		CItemCommand* pItem = dynamic_cast<CItemCommand*>(cnmEquipSpy[nLink]->GetCommand());
		if (pItem && pItem->GetItemInfo()->lID == nItemID) {
			pItem->SetData(Item);
			return true;
		}

		CItemRecord* pInfo = GetItemRecordInfo(nItemID);
		if (!pInfo) {
			return false;
		}

		pItem = new CItemCommand(pInfo);
		pItem->SetData(Item);
		cnmEquipSpy[nLink]->AddCommand(pItem);
	} else {
		cnmEquipSpy[nLink]->DelCommand();
	}
	return true;
}

void CEquipMgr::RenderSpy(int x, int y) {
	// Validate target still exists and is valid before rendering
	if (!eqSpyTarget || !eqSpyTarget->IsValid() || !spyModel) {
		return;
	}
	RenderModel(x, y, eqSpyTarget, spyModel, eqSpyTargetRotate, refreshSpyModel);
	refreshSpyModel = false;
}

void CEquipMgr::SwitchMap() {
	chaModel = 0;
	eqSpyTarget = 0;
	spyModel = 0;
	refreshChaModel = true;
	refreshSpyModel = true;
	HideChestPreview();
	CRmlUiManager::Instance().HideInventoryForm();
}

void CEquipMgr::RenderModel(int x, int y, CCharacter* original, CCharacter* model, int rotation, bool refresh, float scaleBias) {
	g_Render.LookAt(D3DXVECTOR3(11.0f, 36.0f, 10.0f), D3DXVECTOR3(8.70f, 12.0f, 8.0f), MPRender::VIEW_3DUI);

	MPMatrix44 matrix = *model->GetMatrix();

	model->SetUIYaw(180 + rotation);

	// SetUIScaleDis is a divisor — lower bias = larger on-screen model.
	if (scaleBias < 0.05f)
		scaleBias = 0.05f;
	const float resScale = (float)g_Render.GetScrWidth() / (float)TINY_RES_X;
	int typeID = model->getTypeID();
	float baseDis = 13.5f;
	if (typeID == 3) {
		baseDis = 13.0f;
	} else if (typeID == 1) {
		baseDis = 13.5f;
	} else if (typeID == 4) {
		baseDis = 12.0f;
	} else if (typeID == 2) {
		baseDis = 14.5f;
	}
	model->SetUIScaleDis(baseDis * resScale * scaleBias);

	if (refresh) {
		model->UpdataFace(original->GetPart());
	}

	model->SetMatrix(&matrix);

	g_Render.SetTransformView(&g_Render.GetWorldViewMatrix());

	model->RenderForUI(x, y, true);

	original->CheckIsFightArea();
	model->FightSwitch(original->GetIsFight());
	DWORD pose = original->GetCurPoseType();
	float vel = original->GetPoseVelocity();

	switch (pose) {
	case POSE_FLY_RUN:
		pose = POSE_RUN;
		break;
	case POSE_FLY_SEAT:
		pose = POSE_SEAT;
		break;
	case POSE_FLY_SHOW:
		pose = POSE_SHOW;
		break;
	case POSE_FLY_WAITING:
		pose = POSE_WAITING;
		break;
	case POSE_SEAT2:
		pose = POSE_WAITING;
		break;
	case POSE_LEAN:
		pose = POSE_WAITING;
		break;
	}
	model->PlayPose(pose, PLAY_LOOP_SMOOTH);
}

void CEquipMgr::RenderCha(int x, int y, float scaleBias) {
	if (!chaModel) {
		refreshChaModel = false;
		return;
	}
	CCharacter* original = g_stUIBoat.GetHuman();
	if (!original)
		original = g_pGameApp->GetCurScene() ? g_pGameApp->GetCurScene()->GetMainCha() : nullptr;
	if (!original) {
		refreshChaModel = false;
		return;
	}
	RenderModel(x, y, original, chaModel, chaModelRotate, refreshChaModel, scaleBias);
	refreshChaModel = false;

	// Fairy / glow / wing / cloak effects are scene objects — draw them in the UI view
	// after the body is placed by RenderForUI.
	g_Render.SetCurrentView(MPRender::VIEW_3DUI);
	for (int i = 0; i < enumEQUIP_NUM; ++i) {
		if (CEffectObj* eff = chaModel->GetHandEff(i)) {
			eff->SetHide(false);
			eff->FrameMove(0);
			eff->Render();
		}
	}
	g_Render.SetCurrentView(MPRender::VIEW_WORLD);
}

void CEquipMgr::RotateCha(eDirectType enumDirect) {
	chaModelRotate += 180;
	chaModelRotate += -((int)(enumDirect)) * 15;
	chaModelRotate = (chaModelRotate + 360) % 360;
	chaModelRotate -= 180;
}

void CEquipMgr::EnsureChaPreviewModel() {
	CCharacter* pCha = g_stUIBoat.GetHuman();
	if (!pCha || !CGameApp::GetCurScene())
		return;

	if (chaModel && chaModel->IsValid() && chaModel->getTypeID() == pCha->getTypeID()) {
		refreshChaModel = true;
		return;
	}

	if (chaModel) {
		chaModel->SetValid(false);
		chaModel = nullptr;
	}

	chaModel = CGameApp::GetCurScene()->AddCharacter(pCha->getTypeID());
	if (!chaModel)
		return;

	lwIByteSet* res_bs = g_Render.GetInterfaceMgr()->res_mgr->GetByteSet();
	BYTE loadtex_flag = res_bs->GetValue(OPT_RESMGR_LOADTEXTURE_MT);
	BYTE loadmesh_flag = res_bs->GetValue(OPT_RESMGR_LOADMESH_MT);
	res_bs->SetValue(OPT_RESMGR_LOADTEXTURE_MT, 0);
	res_bs->SetValue(OPT_RESMGR_LOADMESH_MT, 0);
	res_bs->SetValue(OPT_RESMGR_LOADTEXTURE_MT, loadtex_flag);
	res_bs->SetValue(OPT_RESMGR_LOADMESH_MT, loadmesh_flag);

	chaModel->SetIsForUI(true);
	chaModel->EnableAI(false);
	chaModel->SetHide(true);
	chaModel->GetActor()->SetSleep();
	refreshChaModel = true;
}

void CEquipMgr::ReleaseChaPreviewModel() {
	if (chaModel) {
		chaModel->SetValid(false);
		chaModel = nullptr;
	}
	refreshChaModel = true;
}

void CEquipMgr::RenderChaPreview(int centerX, int centerY) {
	if (!chaModel || !chaModel->IsValid())
		EnsureChaPreviewModel();
	// Tall Notice preview well — fill height without overflowing the header.
	RenderCha(centerX, centerY, 0.42f);
}

void CEquipMgr::RotateChaPreviewLeft() {
	RotateCha(LEFT);
}

void CEquipMgr::RotateChaPreviewRight() {
	RotateCha(RIGHT);
}

void CEquipMgr::MarkChaPreviewDirty() {
	refreshChaModel = true;
}

void CEquipMgr::_RotateChaLeft(CGuiData* sender, int x, int y, DWORD key) {
	g_stUIEquip.RotateCha(LEFT);
}

//~ ==================================================================
void CEquipMgr::_RotateChaRight(CGuiData* sender, int x, int y, DWORD key) {
	g_stUIEquip.RotateCha(RIGHT);
}

void CEquipMgr::_RotateChaLeftContinue(CGuiData* sender) {
	g_stUIEquip.RotateCha(LEFT);
}

void CEquipMgr::_RotateChaRightContinue(CGuiData* sender) {
	g_stUIEquip.RotateCha(RIGHT);
}

void CEquipMgr::RotateSpy(eDirectType enumDirect) {
	eqSpyTargetRotate += 180;
	eqSpyTargetRotate += -((int)(enumDirect)) * 15;
	eqSpyTargetRotate = (eqSpyTargetRotate + 360) % 360;
	eqSpyTargetRotate -= 180;
}

void CEquipMgr::_EqSpyRenderEvent(C3DCompent* pSender, int x, int y) {
	g_stUIEquip.RenderSpy(x, y);
}
void CEquipMgr::_ChaRenderEvent(C3DCompent* pSender, int x, int y) {
	g_stUIEquip.RenderCha(x, y);
}

void CEquipMgr::_RotateSpyLeft(CGuiData* sender, int x, int y, DWORD key) {
	g_stUIEquip.RotateSpy(LEFT);
}

//~ ==================================================================
void CEquipMgr::_RotateSpyRight(CGuiData* sender, int x, int y, DWORD key) {
	g_stUIEquip.RotateSpy(RIGHT);
}

//~ ==================================================================
void CEquipMgr::_RotateSpyLeftContinue(CGuiData* sender) {
	g_stUIEquip.RotateSpy(LEFT);
}

//~ ==================================================================
void CEquipMgr::_RotateSpyRightContinue(CGuiData* sender) {
	g_stUIEquip.RotateSpy(RIGHT);
}

bool CEquipMgr::_UpdataEquip(SItemGrid& Item, int nLink) {
	if (!cnmEquip[nLink])
		return false;

	int nItemID = Item.sID;
	if (nItemID > 0 && nItemID != enumEQUIP_BOTH_HAND) {
		CItemCommand* pItem = dynamic_cast<CItemCommand*>(cnmEquip[nLink]->GetCommand());
		if (pItem && pItem->GetItemInfo()->lID == nItemID) {
			pItem->SetData(Item);
			return true;
		}

		CItemRecord* pInfo = GetItemRecordInfo(nItemID);
		if (!pInfo) {
			LG("UpdataEquip", RES_STRING(CL_LANGUAGE_MATCH_555), nItemID);
			return false;
		}

		pItem = new CItemCommand(pInfo);
		pItem->SetData(Item);
		cnmEquip[nLink]->AddCommand(pItem);
	} else {
		cnmEquip[nLink]->DelCommand();
	}

	return true;
}

bool CEquipMgr::_GetCommandShortCutType(CCommandObj* pItem, char& chType, short& sGridID) {
	chType = 0;
	sGridID = 0;

	if (!pItem)
		return true;

	if (g_stUIEquip.GetGoodsGrid() && pItem->GetParent() == g_stUIEquip.GetGoodsGrid()) {
		chType = defItemShortCutType;
		sGridID = pItem->GetIndex();
	} else if (g_stUIEquip.lstFightSkill && g_stUIEquip.lstFightSkill == pItem->GetParent()) {
		CSkillCommand* pSkill = dynamic_cast<CSkillCommand*>(pItem);

		if (pSkill->GetSkillRecord()->chType == 2) {
			return false;
		}

		if (pSkill) {
			chType = defSkillFightShortCutType;
			sGridID = pSkill->GetSkillID();
		}
	} else if (g_stUIEquip.lstLifeSkill && g_stUIEquip.lstLifeSkill == pItem->GetParent()) {
		CSkillCommand* pSkill = dynamic_cast<CSkillCommand*>(pItem);
		if (pSkill) {
			chType = defSkillLifeShortCutType;
			sGridID = pSkill->GetSkillID();
		}
	} else if (g_stUIEquip.lstSailSkill && g_stUIEquip.lstSailSkill == pItem->GetParent()) {
		CSkillCommand* pSkill = dynamic_cast<CSkillCommand*>(pItem);
		if (pSkill) {
			chType = defSkillSailShortCutType;
			sGridID = pSkill->GetSkillID();
		}
	}

	return true;
}

int CEquipMgr::RefreshServerShortCut() {
	int nCount = 0;
	stNetShortCut tmp;
	memset(&tmp, 0, sizeof(tmp));
	for (int i = 0; i < SHORT_CUT_NUM; i++) {
		if (_pFastCommands[i]) {
			_GetCommandShortCutType(_pFastCommands[i]->GetCommand(), tmp.chType[i], tmp.byGridID[i]);
		}
	}

	stNetShortCutChange param;
	for (int i = 0; i < SHORT_CUT_NUM; i++) {
		if ((tmp.chType[i] != _stShortCut.chType[i]) || (tmp.byGridID[i] != _stShortCut.byGridID[i])) {
			param.chIndex = i;
			param.chType = tmp.chType[i];
			param.shyGrid = tmp.byGridID[i];
			CS_BeginAction(g_stUIBoat.GetHuman(), enumACTION_SHORTCUT, (void*)&param);
			nCount++;

			LG("shortcut", "Index:%d, Type:%d, GridID:%d\n", param.chIndex, param.chType, param.shyGrid);
		}
	}
	memcpy(&_stShortCut, &tmp, sizeof(tmp));
	LG("shortcut", "Total:%d\n\n", nCount);
	return nCount;
}

void CEquipMgr::UpdateShortCut(stNetShortCut& stShortCut) {
	memcpy(&_stShortCut, &stShortCut, sizeof(_stShortCut));
	if (_pFastCommands) {
		for (unsigned int i = 0; i < SHORT_CUT_NUM; i++) {
			if (_pFastCommands[i])
				_pFastCommands[i]->DelCommand();
		}
	}

	int nIndex = 0;
	CGoodsGrid* getGoods = GetGoodsGrid();
	for (DWORD i = 0; i < SHORT_CUT_NUM; i++) {
		if (_pFastCommands[i]) {
			if (getGoods && stShortCut.chType[i] == defItemShortCutType) {
				_pFastCommands[i]->AddCommand(getGoods->GetItem(stShortCut.byGridID[i]));
			} else if (lstFightSkill && stShortCut.chType[i] == defSkillFightShortCutType) {
				_pFastCommands[i]->AddCommand(lstFightSkill->FindSkill(stShortCut.byGridID[i]));
			} else if (lstLifeSkill && stShortCut.chType[i] == defSkillLifeShortCutType) {
				_pFastCommands[i]->AddCommand(lstLifeSkill->FindSkill(stShortCut.byGridID[i]));
			} else if (lstSailSkill && stShortCut.chType[i] == defSkillSailShortCutType) {
				_pFastCommands[i]->AddCommand(lstSailSkill->FindSkill(stShortCut.byGridID[i]));
			}
		}
	}
}

void CEquipMgr::DelFastCommand(CCommandObj* pObj) {
	if (pObj && pObj->GetIsFast()) {
		CFastCommand* pFast = CFastCommand::FintFastCommand(pObj, true);
		if (pFast) {
			int nIndex = pFast->nTag;
			stNetShortCutChange param;
			memset(&param, 0, sizeof(param));
			param.chIndex = nIndex;
			CS_BeginAction(g_stUIBoat.GetHuman(), enumACTION_SHORTCUT, (void*)&param);

			// ??????????????????,?????????i?
			stNetShortCut& SCut = _stShortCut;
			SCut.chType[nIndex] = 0;
			SCut.byGridID[nIndex] = 0;
			pFast->DelCommand();
		}
	}
}

void CEquipMgr::_evtButtonClickEvent(CGuiData* pSender, int x, int y, DWORD key) {
	string name = pSender->GetName();
	if (name == "btnFastUp") {
		g_stUIEquip._ActiveFast(g_stUIEquip._nFastCur - 1);
		return;
	} else if (name == "btnFastDown") {
		g_stUIEquip._ActiveFast(g_stUIEquip._nFastCur + 1);
		return;
	}
}

void CEquipMgr::_evtEquipEvent(CGuiData* pSender, CCommandObj* pItem, bool& isAccept) {
	isAccept = false;

	CGoodsGrid* pGood = dynamic_cast<CGoodsGrid*>(CDrag::GetParent());
	if (pGood != g_stUIEquip.GetGoodsGrid())
		return;

	CItemCommand* pItemCommand = dynamic_cast<CItemCommand*>(pItem);
	if (!pItemCommand)
		return;

	CGameScene* pScene = g_pGameApp->GetCurScene();
	if (!pScene)
		return;

	CCharacter* pCha = pScene->GetMainCha();
	if (!pCha)
		return;

	COneCommand* pCom = dynamic_cast<COneCommand*>(pSender);
	if (!pCom)
		return;

	stNetUseItem info;
	info.sGridID = g_stUIEquip.grdItem->GetDragIndex();
	CS_BeginAction(g_stUIBoat.GetHuman(), enumACTION_ITEM_USE, &info);
}

void CEquipMgr::_evtDeleteItemYesNoEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	if (nMsgType != CForm::mrYes)
		return;

	stNetDelItem info;
	info.sGridID = g_stUIEquip.rightClickItemIndex;
	CS_BeginAction(g_stUIBoat.GetHuman(), enumACTION_ITEM_DELETE, &info);
}

void CEquipMgr::_evtLockItemYesNoEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	if (nMsgType != CForm::mrYes) {
		return;
	}
	CS_DropLock(g_stUIEquip.rightClickItemIndex);
}

void CEquipMgr::ShowSellPrompt() {
	grdItem->SetDragIndex(rightClickItemIndex);
	g_stUINpcTrade.LocalSaleToNpc(g_stUINpcTrade.grdNPCtradeWeapon,
								  grdItem,
								  rightClickItemIndex,
								  grdItem->GetItem(g_stUIEquip.rightClickItemIndex));
}

void CEquipMgr::_evtThrowEquipEvent(CGuiData* pSender, CCommandObj* pItem, bool& isThrow) {
	isThrow = false;

	CGameScene* pScene = CGameApp::GetCurScene();
	if (!pScene)
		return;

	CCharacter* pMain = CGameScene::GetMainCha();
	if (!pMain)
		return;

	COneCommand* pCom = dynamic_cast<COneCommand*>(CDrag::GetParent());
	if (pCom && pCom->nTag > 0) {
		int x = (int)(pScene->GetMouseMapX() * 100.0f);
		int y = (int)(pScene->GetMouseMapY() * 100.0f);

		if (!g_stUIEquip._GetThrowPos(x, y))
			return;

		stUnfix& unfix = g_stUIEquip._sUnfix;
		unfix.Reset();

		unfix.nGridID = -2;
		unfix.nLink = pCom->nTag;
		unfix.nX = x;
		unfix.nY = y;
		unfix.pItem = pItem;
		g_stUIEquip._StartUnfix(unfix);
	}
}

bool CEquipMgr::ExecFastKey(int key) {
	if (key == VK_TAB || (key == VK_OEM_6 && ::GetAsyncKeyState(VK_CONTROL))) {
		_ActiveFast(_nFastCur + 1);
		return true;
	}

	bool g_FKeysInUse{false};

	if (key >= 112 && key <= 123)
		g_FKeysInUse = true;

	if (key == VK_RETURN && g_IsNumberTopBar && !g_FKeysInUse && !CCozeForm::GetInstance()->IsChatBoxActive())
		CCozeForm::GetInstance()->ActivateChatBox();

	if (g_IsNumberTopBar && !g_FKeysInUse && !g_pGameApp->IsCtrlPress() && key != 8 && !CCozeForm::GetInstance()->IsChatBoxActive()) {
		auto numberID = key - '0';
		if (numberID >= 0 && numberID <= 9) {
			if (numberID == 0) {
				numberID = 10;
			}
			numberID -= 1;
			key = numberID;
		}
	} else {
		key -= VK_F1;
	}

	if (key < 0 || key > (int)11) {
		return false;
	}

	if (!CFormMgr::s_Mgr.GetEnableHotKey()) {
		return false;
	}

	// check if we are using the top bar
	if (g_pGameApp->IsShiftPress() && !g_IsNumberTopBar) {
		key = 2 * MAX_FAST_COL + key;
	} else if (g_IsNumberTopBar && !g_FKeysInUse) {
		key = 2 * MAX_FAST_COL + key;
	} else {
		key = _nFastCur * MAX_FAST_COL + key;
	}

	if (_pFastCommands[key]) {
		_pFastCommands[key]->Exec();
		return true;
	}
	return false;
}

void CEquipMgr::_ActiveFast(int num) {
	if (num >= 2) {
		num = 0;
	}

	if (num < 0) {
		num = 2 - 1;
	}

	int count = (int)SHORT_CUT_NUM;
	for (int i = 0; i < count; i++) {
		if (_pFastCommands[i]) {
			_pFastCommands[i]->SetIsShow(false);
		}
	}

	count = (num + 1) * MAX_FAST_COL;
	for (int i = num * MAX_FAST_COL; i < count; i++) {
		if (_pFastCommands[i]) {
			_pFastCommands[i]->SetIsShow(true);
		}
	}

	count = 3 * MAX_FAST_COL;
	for (int i = 2 * MAX_FAST_COL; i < count; i++) {
		if (_pFastCommands[i]) {
			_pFastCommands[i]->SetIsShow(true);
		}
	}
	_nFastCur = num;

	// sprintf( szBuf, "%d", _nFastCur + 1 );
	//_pActiveFastLabel->SetCaption( szBuf );
}

void CEquipMgr::evtSwapItemEvent(CGuiData* pSender, int nFirst, int nSecond, bool& isSwap) {
	isSwap = false;
	CGoodsGrid* pGood = dynamic_cast<CGoodsGrid*>(CDrag::GetParent());

	CCharacter* pSelf = g_stUIBoat.FindCha(pGood);
	if (!pSelf)
		return;

	if (pSelf->IsBoat() && pSelf != CGameScene::GetMainCha()) {
		g_pGameApp->SysInfo(RES_STRING(CL_LANGUAGE_MATCH_557));
		return;
	}

	CItemCommand* pItem = dynamic_cast<CItemCommand*>(pGood->GetItem(nSecond));
	CItemCommand* pTarget = dynamic_cast<CItemCommand*>(pGood->GetItem(nFirst));
	// Low-URE fairies/gear are GetIsValid()==false but should still swap in-bag.
	if (!pItem)
		return;
	if (pItem->GetItemInfo() && pItem->GetItemInfo()->lID == 32767)
		return;
	if (pTarget && pTarget->GetItemInfo() && pTarget->GetItemInfo()->lID == 32767)
		return;

	if (g_pGameApp->IsShiftPress()) {
		if (pItem && pItem->GetTotalNum() > 1) {
			if (!pTarget || pTarget->GetItemInfo()->nID == pItem->GetItemInfo()->nID) {
				SSplit.nFirst = nFirst;
				SSplit.nSecond = nSecond;
				SSplit.pSelf = pSelf;
				CBoxMgr::ShowNumberBox(SSplitItem::_evtSplitItemEvent, pItem->GetTotalNum());
				return;
			}
		}
	}

	stNetItemPos info;
	info.sSrcGridID = nSecond;
	info.sSrcNum = 0;
	info.sTarGridID = nFirst;
	CS_BeginAction(pSelf, enumACTION_ITEM_POS, (void*)&info);
}

void CEquipMgr::SSplitItem::_evtSplitItemEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	if (nMsgType != CForm::mrYes)
		return;

	stNumBox* pBox = (stNumBox*)pSender->GetForm()->GetPointer();
	if (!pBox)
		return;

	stNetItemPos info;
	info.sSrcGridID = SSplit.nSecond;
	info.sSrcNum = pBox->GetNumber();
	info.sTarGridID = SSplit.nFirst;
	CS_BeginAction(SSplit.pSelf, enumACTION_ITEM_POS, (void*)&info);
}

bool CEquipMgr::_GetThrowPos(int& x, int& y) {
	CCharacter* pCha = CGameScene::GetMainCha();
	if (!pCha)
		return false;

	GetDistancePos(pCha->GetCurX(), pCha->GetCurY(), x, y, 100, x, y);
	if (CGameApp::GetCurScene()->GetIsBlockWalk(pCha, x, y)) {
		x = pCha->GetCurX();
		y = pCha->GetCurY();
	}
	return true;
}

void CEquipMgr::evtThrowItemEvent(CGuiData* pSender, int id, bool& isThrow) {
	isThrow = false;

	if (!CGameApp::GetCurScene())
		return;

	CCharacter* pMain = CGameScene::GetMainCha();
	if (!pMain)
		return;

	auto pGood = dynamic_cast<CGoodsGrid*>(pSender);

	CCommandObj* obj = pGood->GetItem(id);

	CCharacter* pSelf = g_stUIBoat.FindCha(pGood);
	if (!pSelf)
		return;

	// add by Philip.Wu  2006-07-05
	// ?????????????????????????????????????�? BUG
	if (!obj->GetIsValid())
		return;

	int x = (int)(CGameApp::GetCurScene()->GetMouseMapX() * 100.0f);
	int y = (int)(CGameApp::GetCurScene()->GetMouseMapY() * 100.0f);

	if (!g_stUIEquip._GetThrowPos(x, y))
		return;

	stThrow& sthrow = g_stUIEquip._sThrow;
	sthrow.nX = x;
	sthrow.nY = y;
	sthrow.nGridID = id;
	sthrow.pSelf = pSelf;

	// ???????????
	CItemCommand* pItem = dynamic_cast<CItemCommand*>(obj);
	if (pItem && pItem->GetItemInfo()->sType == 43) {
		stSelectBox* pBox = g_stUIBox.ShowSelectBox(_evtThrowBoatDialogEvent,
													RES_STRING(CL_LANGUAGE_MATCH_558),
													true);
		if (pBox) {
			pBox->pointer = &sthrow;
			return;
		}
	}

	if (obj->GetIsPile() && obj->GetTotalNum() > 1) {
		stNumBox* pBox = g_stUIBox.ShowNumberBox(_evtThrowDialogEvent, obj->GetTotalNum());
		if (pBox) {
			pBox->pointer = &sthrow;
			return;
		}
	}

	g_stUIEquip._SendThrowData(sthrow);
}

void CEquipMgr::_evtThrowDialogEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	if (nMsgType != CForm::mrYes)
		return;

	stNumBox* pBox = (stNumBox*)pSender->GetForm()->GetPointer();
	if (!pBox)
		return;

	stThrow* p = (stThrow*)pBox->pointer;
	if (!p)
		return;

	g_stUIEquip._SendThrowData(*p, pBox->GetNumber());
}

void CEquipMgr::_evtThrowBoatDialogEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	if (nMsgType != CForm::mrYes)
		return;

	stSelectBox* pBox = (stSelectBox*)pSender->GetForm()->GetPointer();
	if (!pBox)
		return;

	stThrow* p = (stThrow*)pBox->pointer;
	if (!p)
		return;

	g_stUIEquip._SendThrowData(*p, 1);
}

void CEquipMgr::_SendThrowData(const stThrow& sthrow, int nThrowNum) {
	if (sthrow.pSelf->IsBoat() && sthrow.pSelf != CGameScene::GetMainCha()) {
		g_pGameApp->SysInfo(RES_STRING(CL_LANGUAGE_MATCH_557));
		return;
	}

	auto throwDeleteConfirm = [](CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
		if (nMsgType != CForm::mrYes) {
			return;
		}
		stSelectBox* pSelect = static_cast<stSelectBox*>(pSender->GetForm()->GetPointer());
		stThrow* pItem = (stThrow*)pSelect->pointer;

		if (pSelect && pSelect->pointer) {
			stNetItemThrow item;
			item.lNum = (int)pSelect->dwTag;
			item.sGridID = pItem->nGridID;
			item.lPosX = pItem->nX;
			item.lPosY = pItem->nY;

			CS_BeginAction(pItem->pSelf, enumACTION_ITEM_THROW, (void*)&item);
		}
	};

	if (CWorldScene::_IsThrowItemHint) {
		// Keep throw target on EquipMgr's persistent _sThrow for the confirm callback.
		g_stUIEquip._sThrow = sthrow;
		g_rmlInvPendingThrowNum = nThrowNum;
		if (ShowRmlItemConfirm("Throw Item",
							   RES_STRING(CL_UIEQUIPFORM_CPP_00001),
							   "",
							   RmlInvPendingConfirm::ThrowSingle)) {
			return;
		}

		stSelectBox* pSelectBox = g_stUIBox.ShowSelectBox(throwDeleteConfirm, RES_STRING(CL_UIEQUIPFORM_CPP_00001), true);

		pSelectBox->dwTag = nThrowNum;
		pSelectBox->pointer = (void*)&g_stUIEquip._sThrow;
	} else {
		stNetItemThrow item;
		item.lNum = nThrowNum;
		item.sGridID = sthrow.nGridID;
		item.lPosX = sthrow.nX;
		item.lPosY = sthrow.nY;

		CS_BeginAction(sthrow.pSelf, enumACTION_ITEM_THROW, (void*)&item);
	}
}

void CEquipMgr::_evtSkillFormShow(CGuiData* pSender) {
	g_stUIEquip.RefreshUpgrade();
}

void CEquipMgr::UpdateIMP(int IMP) {
	lIMP = IMP;
	// CForm* frmInv = _FindForm("frmInv");
	CLabel* lblIMP = dynamic_cast<CLabel*>(frmInv->Find("labItemIMPnumber"));
	char szBuf[16];
	sprintf(szBuf, "%s", StringSplitNum(IMP));
	lblIMP->SetCaption(szBuf);
	RefreshRmlInventory();
}

void CEquipMgr::FrameMove(DWORD dwTime) {
	static CTimeWork time(100);
	if (time.IsTimeOut(dwTime)) {
		CCharacter* pCha = g_stUIBoat.GetHuman();
		if (!pCha)
			return;

		SGameAttr* pGameAttr = pCha->getGameAttr();
		if (frmSkill->GetIsShow()) {
			// ???????????
			sprintf(szBuf, "%d", pGameAttr->get(ATTR_TP));
			labPoint->SetCaption(szBuf);

			sprintf(szBuf, "%d", pGameAttr->get(ATTR_LIFETP));
			labPointLife->SetCaption(szBuf);

			RefreshUpgrade();
		}

		CCharacter* pMainCha = CGameScene::GetMainCha();
		if (pMainCha) {
			// ??�???????
			CSkillCommand::GetActiveImage()->Next();

			CChaStateMgr* pState = pMainCha->GetStateMgr();

			for (vskill::iterator it = _cancels.begin(); it != _cancels.end(); it++)
				(*it)->SetIsActive(pState->HasSkillState((*it)->nStateID));

			if (frmInv->GetIsShow() || CRmlUiInventoryForm::Instance().IsVisible()) {
				sprintf(szBuf, "%s", StringSplitNum(pGameAttr->get(ATTR_GD)));
				if (frmInv->GetIsShow() && lblGold)
					lblGold->SetCaption(szBuf);

				if (CRmlUiInventoryForm::Instance().IsVisible()) {
					char impBuf[32];
					sprintf(impBuf, "%s", StringSplitNum(GetIMP()));
					CRmlUiInventoryForm::Instance().SetCurrency(szBuf, impBuf);
				}

				for (int i = 0; i < enumEQUIP_NUM; i++)
					_imgCharges[i].Next();

				CItemCommand* pItem = nullptr;
				for (states::iterator it = _charges.begin(); it != _charges.end(); it++) {
					if (COneCommand* pCmd = cnmEquip[(*it)->sChargeLink]) {
						pItem = dynamic_cast<CItemCommand*>(pCmd->GetCommand());
						if (pItem && pItem->GetItemInfo()->sType == 29) {
							pCmd->SetIsShowActive(pState->HasSkillState((*it)->chID));
						} else {
							pCmd->SetIsShowActive(false);
						}
					}
				}
			}
		}
	}
}

void CEquipMgr::RefreshUpgrade() {
	CForm* f = frmSkill;
	if (!f->GetIsShow())
		return;

	CCharacter* pCha = CGameScene::GetMainCha();
	if (!pCha)
		return;

	SGameAttr* pAttr = pCha->getGameAttr();

	lstFightSkill->SetIsShowUpgrade(pAttr->get(ATTR_TP) > 0);
	lstLifeSkill->SetIsShowUpgrade(pAttr->get(ATTR_LIFETP) > 0);

	RefreshSkillJob(pAttr->get(ATTR_JOB));
}

void CEquipMgr::RefreshSkillJob(int nJob) {
	int count = lstFightSkill->GetCount();
	CSkillCommand* tmp = nullptr;
	CSkillList* pList = nullptr;
	for (int j = 0; j < 2; j++) {
		pList = GetSkillList(j);
		for (int i = 0; i < count; i++) {
			tmp = pList->GetCommand(i);
			if (tmp) {
				tmp->GetSkillRecord()->Refresh(nJob);
			}
		}
	}
}

void CEquipMgr::UnfixToGrid(CCommandObj* pItem, int nGridID, int nLink) {

	_sUnfix.Reset();

	_sUnfix.nLink = nLink;
	_sUnfix.nGridID = nGridID;
	_sUnfix.pItem = pItem;

	_StartUnfix(_sUnfix);
}
/*
void CEquipMgr::_evtItemFormClose( CForm *pForm, bool& IsClose )
{
	if ( g_stUITrade.IsTrading() ){
		IsClose = false;
	}

	if(g_stUIEquip.chkLinkEqForm->GetIsChecked()&& g_stUIEquip.frmEquip->GetIsShow()){
		g_stUIEquip.frmEquip->Hide();
	}
}*/

void CEquipMgr::_evtEquipFormClose(CForm* pForm, bool& IsClose) {
	if (g_stUITrade.IsTrading()) {
		IsClose = false;
	}
	// Keep preview alive if Rml inventory is still open.
	if (!CRmlUiInventoryForm::Instance().IsVisible())
		g_stUIEquip.ReleaseChaPreviewModel();

	g_stUIEquip.GetGoodsGrid()->ResetItemSelections();
	g_stUIEquip.HideChestPreview();
}

void CEquipMgr::_evtSpyFormClose(CForm* pForm, bool& IsClose) {
	if (g_stUIEquip.spyModel) {
		g_stUIEquip.spyModel->SetValid(false);
	}
	// Clear dangling pointers to prevent crashes
	g_stUIEquip.eqSpyTarget = nullptr;
	g_stUIEquip.spyModel = nullptr;
}

void CEquipMgr::_evtSpyFormShow(CGuiData* pSender) {
	// Validate target still exists before accessing
	if (!g_stUIEquip.eqSpyTarget || !g_stUIEquip.eqSpyTarget->IsValid()) {
		g_pGameApp->SysInfo("Target is no longer available");
		if (pSender && pSender->GetForm()) {
			pSender->GetForm()->Close();
		}
		return;
	}
	int typeID = g_stUIEquip.eqSpyTarget->getTypeID();

	g_stUIEquip.spyModel = CGameApp::GetCurScene()->AddCharacter(typeID);

	lwIByteSet* res_bs = g_Render.GetInterfaceMgr()->res_mgr->GetByteSet();
	BYTE loadtex_flag = res_bs->GetValue(OPT_RESMGR_LOADTEXTURE_MT);
	BYTE loadmesh_flag = res_bs->GetValue(OPT_RESMGR_LOADMESH_MT);
	res_bs->SetValue(OPT_RESMGR_LOADTEXTURE_MT, 0);
	res_bs->SetValue(OPT_RESMGR_LOADMESH_MT, 0);
	res_bs->SetValue(OPT_RESMGR_LOADTEXTURE_MT, loadtex_flag);
	res_bs->SetValue(OPT_RESMGR_LOADMESH_MT, loadmesh_flag);
	g_stUIEquip.spyModel->SetIsForUI(true);
	g_stUIEquip.spyModel->EnableAI(false);
	g_stUIEquip.spyModel->SetHide(true);
	g_stUIEquip.spyModel->GetActor()->SetSleep();
	g_stUIEquip.refreshSpyModel = true;
}

void CEquipMgr::_evtEquipFormShow(CGuiData* pSender) {
	g_stUIEquip.EnsureChaPreviewModel();
}

/*
void CEquipMgr::_evtItemFormShow(CGuiData *pSender)
{
	if(g_stUIStore.GetStoreForm()->GetIsShow())
	{
		g_stUIEquip.frmItem->SetIsShow(false);
	}
	if(g_stUIEquip.chkLinkEqForm->GetIsChecked()&& !g_stUIEquip.frmEquip->GetIsShow()){
		g_stUIEquip.frmEquip->Show();
	}
}*/

void CEquipMgr::_SendUnfixData(const stUnfix& unfix, int nUnfixNum) {
	if (!CGameScene::GetMainCha())
		return;

	if (unfix.nLink == -1)
		return;

	stNetItemUnfix item;
	item.chLinkID = unfix.nLink;
	item.sGridID = unfix.nGridID;
	item.lPosX = unfix.nX;
	item.lPosY = unfix.nY;

	if (g_stUIBoat.GetHuman() == CGameScene::GetMainCha()) {
		CActor* pActor = g_stUIBoat.GetHuman()->GetActor();
		CEquipState* pState = new CEquipState(pActor);
		pState->SetUnfixData(item);
		pActor->SwitchState(pState);
	} else {
		CS_BeginAction(g_stUIBoat.GetHuman(), enumACTION_ITEM_UNFIX, (void*)&item);
	}
}

void CEquipMgr::_evtUnfixDialogEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	if (nMsgType != CForm::mrYes)
		return;

	stNumBox* pBox = (stNumBox*)pSender->GetForm()->GetPointer();
	if (!pBox)
		return;

	stUnfix* p = (stUnfix*)pBox->pointer;
	if (!p)
		return;

	g_stUIEquip._SendUnfixData(*p, pBox->GetNumber());
}

void CEquipMgr::_StartUnfix(stUnfix& unfix) {
	CCommandObj* pItem = unfix.pItem;
	if (!pItem)
		return;

	if (unfix.nGridID == -2) {
		// if( !pItem->IsAllowThrow() )
		//{
		//     g_pGameApp->SysInfo( "??????????????" );
		//     return;
		// }

		if (!_GetThrowPos(unfix.nX, unfix.nY))
			return;
	}

	if (pItem->GetIsPile() && pItem->GetTotalNum() > 1) {
		stNumBox* pBox = g_stUIBox.ShowNumberBox(_evtUnfixDialogEvent, pItem->GetTotalNum());
		if (pBox) {
			pBox->pointer = &unfix;
			return;
		}
	}

	_SendUnfixData(unfix);
}

CItemCommand* CEquipMgr::GetEquipItem(unsigned int nLink) {
	if (nLink < enumEQUIP_NUM && cnmEquip[nLink])
		return dynamic_cast<CItemCommand*>(cnmEquip[nLink]->GetCommand());
	return nullptr;
}

bool CEquipMgr::IsEquipCom(COneCommand* pCom) {
	return frmInv == pCom->GetForm();
}

CGuiPic* CEquipMgr::GetChargePic(unsigned int n) {
	if (n < enumEQUIP_NUM)
		return &_imgCharges[n];
	return nullptr;
}

// TODO(Ogge):
//  Implement filter for multi-selecting
void CEquipMgr::_evtRMouseGridEvent(CGuiData* pSender, CCommandObj* pItem, int nGridID) {
	if (!g_stUIBoat.GetHuman())
		return;

	CItemCommand* pItemCommand = dynamic_cast<CItemCommand*>(pItem);
	if (!pItemCommand) {
		return;
	}

	if (g_pGameApp->IsShiftPress()) {
		pItem->ExecRightClick();
		return;
	}

	{
		auto ShowOnlyRelevevantMenuItems = [&](CItemCommand* item, CMenu* menu) {
			for (auto beg = 0, end = menu->GetCount(); beg < end; ++beg) {
				menu->GetMenuItem(beg)->SetIsHide(true);
			}

			// The only option available for locked items is unlock
			if (item->IsLocked()) {
				menu->FindMenuItem("Unlock Item")->SetIsHide(false);
				return;
			}

			const auto allowThrow = static_cast<bool>(item->GetItemInfo()->chIsThrow);
			const auto allowTrade = static_cast<bool>(item->GetItemInfo()->chIsTrade);
			const auto allowDelete = static_cast<bool>(item->GetItemInfo()->chIsDel);

			// Lock option
			if ((allowThrow || allowTrade || allowDelete) && item->GetItemInfo()->IsLockable()) {
				menu->FindMenuItem("Lock Item")->SetIsHide(false);
			}

			if (allowThrow) {
				menu->FindMenuItem("Throw Item")->SetIsHide(false);
			}

			if (allowDelete) {
				menu->FindMenuItem("Delete Item")->SetIsHide(false);
			}

			if (g_stUINpcTrade.GetIsShow() && !item->IsLocked() && allowTrade) {
				menu->FindMenuItem("Sell Item")->SetIsHide(false);
			}

			if (g_stUIEquip.IsChestPreviewSupported(item->GetItemInfo()->lID)) {
				if (auto* boxRatesMenu = menu->FindMenuItem("Box Rates"); boxRatesMenu) {
					boxRatesMenu->SetIsHide(false);
				}
			}

			if (auto* sendToChatMenu = menu->FindMenuItem("Send to Chat"); sendToChatMenu) {
				sendToChatMenu->SetIsHide(false);
			}
		};

		g_stUIEquip.rightClickItemIndex = nGridID >= 0 ? nGridID : pItemCommand->GetIndex();
		ShowOnlyRelevevantMenuItems(pItemCommand, g_stUIEquip.rightClickItemMenu);

		// frmInv is hidden while Notice Rml inventory is open — PopMenu on a visible host
		// so FormMgr can render/hit-test the menu (drawn again above Rml in GameAppRender).
		CForm* host = g_stUIEquip.GetItemForm();
		if (!host || !host->GetIsShow()) {
			host = CForm::GetActive();
			if (!host || !host->GetIsShow() || !host->IsNormal())
				host = CFormMgr::s_Mgr.Find("frmMain800");
		}
		if (host)
			host->PopMenu(g_stUIEquip.rightClickItemMenu, CForm::GetMouseX(), CForm::GetMouseY());
	}
	// TODO: Possible collision with new right click menu method with boat information
	stNetItemInfo info;
	info.chType = mission::VIEW_CHAR_BAG;
	info.sGridID = nGridID;
	CS_BeginAction(g_stUIBoat.GetHuman(), enumACTION_ITEM_INFO, &info);
}

void CEquipMgr::RequestChestPreview(int chestItemID) {
	if (chestItemID <= 0) {
		return;
	}

	auto it = chestPreviewCache.find(chestItemID);
	if (it != chestPreviewCache.end() && !it->second.itemIDs.empty()) {
		ShowChestPreviewByID(chestItemID);
		return;
	}

	pendingChestPreviewItemID = chestItemID;
	g_pGameApp->Waiting(true);
	CS_RequestChestPreview(chestItemID);
}

void CEquipMgr::OnChestPreviewData(int chestItemID, const std::vector<int>& itemIDs, const std::vector<int>& quantities, const std::vector<int>& weights, int totalWeight) {
	if (chestItemID <= 0) {
		return;
	}

	if (itemIDs.empty() || quantities.size() != itemIDs.size() || weights.size() != itemIDs.size() || totalWeight <= 0) {
		chestPreviewCache.erase(chestItemID);

		if (pendingChestPreviewItemID == chestItemID) {
			pendingChestPreviewItemID = -1;
			g_pGameApp->Waiting(false);
			g_pGameApp->SysInfo("Chest preview unavailable.");
		}
		return;
	}

	chestPreviewCache[chestItemID] = {itemIDs, quantities, weights, totalWeight};

	if (pendingChestPreviewItemID == chestItemID) {
		pendingChestPreviewItemID = -1;
		g_pGameApp->Waiting(false);
		ShowChestPreviewByID(chestItemID);
	}
}

void CEquipMgr::ShowChestPreviewByID(int chestItemID) {
	if (!frmChestPreview || !grdChestPreview) {
		return;
	}

	auto it = chestPreviewCache.find(chestItemID);
	if (it == chestPreviewCache.end()) {
		return;
	}

	const auto& data = it->second;
	const auto& itemIDs = data.itemIDs;
	const auto& quantities = data.quantities;
	const auto& weights = data.weights;
	const auto totalWeight = data.totalWeight;
	if (itemIDs.empty() || itemIDs.size() != quantities.size() || itemIDs.size() != weights.size() || totalWeight <= 0) {
		return;
	}

	ClearChestPreview();

	if (labChestPreviewTitle) {
		const auto previewTitle = BuildChestPreviewTitle(chestItemID);
		labChestPreviewTitle->SetCaption(previewTitle.c_str());
	}

	const auto maxSlots = min(static_cast<int>(itemIDs.size()), grdChestPreview->GetMaxNum());

	std::vector<int> sortedIndices(maxSlots);
	for (auto i = 0; i < maxSlots; ++i) sortedIndices[i] = i;
	std::sort(sortedIndices.begin(), sortedIndices.end(), [&](int a, int b) {
		return weights[a] > weights[b];
	});

	auto slot = 0;
	for (auto idx : sortedIndices) {
		auto* itemInfo = GetItemRecordInfo(itemIDs[idx]);
		if (!itemInfo) {
			continue;
		}

		auto* previewItem = new CChestPreviewItemCommand(itemInfo);

		char rateText[64] = {0};
		const auto chance = (static_cast<double>(weights[idx]) * 100.0) / static_cast<double>(totalWeight);
		if (quantities[idx] > 1) {
			sprintf(rateText, "%.2f%% x%d", chance, quantities[idx]);
		} else {
			sprintf(rateText, "%.2f%%", chance);
		}
		previewItem->SetOwnDefText(rateText);

		if (!grdChestPreview->SetItem(slot++, previewItem)) {
			delete previewItem;
		}
	}

	auto left = CForm::GetMouseX() + 16;
	auto top = CForm::GetMouseY() + 16;

	if (left + frmChestPreview->GetWidth() > GetRender().GetScreenWidth()) {
		left = GetRender().GetScreenWidth() - frmChestPreview->GetWidth();
	}

	if (top + frmChestPreview->GetHeight() > GetRender().GetScreenHeight()) {
		top = GetRender().GetScreenHeight() - frmChestPreview->GetHeight();
	}

	left = max(left, 0);
	top = max(top, 0);

	frmChestPreview->SetPos(left, top);
	frmChestPreview->Refresh();
	frmChestPreview->SetIsShow(true);
}

void CEquipMgr::HideChestPreview() {
	if (!frmChestPreview) {
		return;
	}

	ClearChestPreview();
	frmChestPreview->SetIsShow(false);
}

bool CEquipMgr::IsChestPreviewSupported(int itemID) const {
	auto* info = GetItemRecordInfo(itemID);
	return info && strcmp(info->szAttrEffect, "GiveRandomItems") == 0;
}

void CEquipMgr::ClearChestPreview() {
	if (grdChestPreview) {
		grdChestPreview->Clear();
		grdChestPreview->Reset();
	}

	if (labChestPreviewTitle) {
		labChestPreviewTitle->SetCaption("Chest Preview");
	}
}

void CEquipMgr::_evtRepairEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	CS_ItemRepairAnswer(nMsgType == CForm::mrYes);
}

void CEquipMgr::ShowRepairMsg(const char* pItemName, long lMoney) {
	char szBuf[255] = {0};
	sprintf(szBuf, RES_STRING(CL_LANGUAGE_MATCH_559), pItemName, lMoney);
	g_stUIBox.ShowSelectBox(_evtRepairEvent, szBuf, true);
}

void CEquipMgr::CloseAllForm() {
	HideInventoryUi();

	if (frmSkill && frmSkill->GetIsShow()) {
		frmSkill->SetIsShow(false);
	}

	HideChestPreview();
}

void CEquipMgr::ShowInventoryUi() {
	if (frmInv)
		frmInv->SetIsShow(false);

	if (!CRmlUiManager::Instance().IsReady()) {
		if (frmInv)
			frmInv->SetIsShow(true);
		return;
	}

	EnsureChaPreviewModel();
	CRmlUiInventoryForm& rml = CRmlUiInventoryForm::Instance();
	rml.Show();
	RefreshRmlInventory();
}

void CEquipMgr::HideInventoryUi() {
	if (CGoodsGrid* bag = GetGoodsGrid())
		bag->ResetItemSelections();
	CRmlUiInventoryForm::Instance().Hide();
	ReleaseChaPreviewModel();
	if (frmInv && frmInv->GetIsShow())
		frmInv->SetIsShow(false);
}

bool CEquipMgr::IsInventoryUiVisible() const {
	if (CRmlUiInventoryForm::Instance().IsVisible())
		return true;
	return frmInv && frmInv->GetIsShow();
}

void CEquipMgr::ToggleInventoryUi() {
	if (IsInventoryUiVisible()) {
		HideInventoryUi();
		return;
	}
	ShowInventoryUi();
}

void CEquipMgr::ShowBagContextMenu(int bagIndex) {
	CGoodsGrid* bag = GetGoodsGrid();
	if (!bag || bagIndex < 0)
		return;
	CItemCommand* cmd = dynamic_cast<CItemCommand*>(bag->GetItem(bagIndex));
	if (!cmd)
		return;
	if (cmd->GetItemInfo() && cmd->GetItemInfo()->lID == 32767)
		return;

	// Plain RMB on an unselected slot → single select. RMB on an already-selected slot
	// (or Ctrl+RMB) keeps multi-select so batch Throw/Lock/Delete/Sell work.
	if (!bag->IsItemSelected(bagIndex)) {
		if (g_pGameApp->IsCtrlPress())
			return;
		bag->SelectItemOnly(bagIndex);
	}

	rightClickItemIndex = bagIndex;

	// Refresh selection chrome after menu path may have changed it.
	std::vector<char> selFlags((size_t)bag->GetMaxNum(), 0);
	for (int i = 0; i < bag->GetMaxNum(); ++i)
		selFlags[(size_t)i] = bag->IsItemSelected(i) ? 1 : 0;
	CRmlUiInventoryForm::Instance().ApplyBagSelection(selFlags);

	// Match legacy: refresh server item-info for the clicked grid.
	stNetItemInfo info;
	info.chType = mission::VIEW_CHAR_BAG;
	info.sGridID = (short)bagIndex;
	CS_BeginAction(g_stUIBoat.GetHuman(), enumACTION_ITEM_INFO, &info);

	CRmlUiInventoryForm& rml = CRmlUiInventoryForm::Instance();
	if (rml.IsVisible()) {
		if (g_pGameApp->IsShiftPress()) {
			cmd->ExecRightClick();
			return;
		}

		CRmlUiInventoryForm::CtxMenuFlags flags;
		if (cmd->IsLocked()) {
			flags.unlockItem = true;
		} else {
			CItemRecord* rec = cmd->GetItemInfo();
			const bool allowThrow = rec && rec->chIsThrow;
			const bool allowTrade = rec && rec->chIsTrade;
			const bool allowDelete = rec && rec->chIsDel;
			if ((allowThrow || allowTrade || allowDelete) && rec && rec->IsLockable())
				flags.lockItem = true;
			flags.throwItem = allowThrow;
			flags.deleteItem = allowDelete;
			if (g_stUINpcTrade.GetIsShow() && allowTrade)
				flags.sellItem = true;
			if (rec && IsChestPreviewSupported(rec->lID))
				flags.boxRates = true;
			flags.sendToChat = true;
		}
		rml.ShowContextMenu(CForm::GetMouseX(), CForm::GetMouseY(), flags);
		return;
	}

	if (rightClickItemMenu)
		_evtRMouseGridEvent(bag, cmd, bagIndex);
}

void CEquipMgr::ExecuteBagContextAction(const char* action) {
	if (!action || !action[0])
		return;

	if (strcmp(action, "delete") == 0) {
		if (ShowRmlItemConfirm("Delete Item",
							   "Item will vanish if you delete it.",
							   "Do you wish to delete the selected item(s)?",
							   RmlInvPendingConfirm::DeleteSelected)) {
			return;
		}
		auto deleteConfirm = [](CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
			if (nMsgType != CForm::mrYes)
				return;
			auto pSelect = static_cast<stSelectBox*>(pSender->GetForm()->GetPointer());
			if (pSelect && pSelect->pointer)
				g_stUIEquip.DeleteSelectedItems(static_cast<CGoodsGrid*>(pSelect->pointer));
		};
		auto pSelectBox = g_stUIBox.ShowSelectBox(deleteConfirm, RES_STRING(CL_LANGUAGE_MATCH_556), true);
		pSelectBox->pointer = reinterpret_cast<void*>(GetGoodsGrid());
		return;
	}

	if (strcmp(action, "throw") == 0) {
		if (CWorldScene::_IsThrowItemHint) {
			if (GetGoodsGrid() && GetGoodsGrid()->GetSelectedItemCount() == 1) {
				bool unused = false;
				evtThrowItemEvent(GetGoodsGrid(), rightClickItemIndex, unused);
				return;
			}
			if (ShowRmlItemConfirm("Throw Item",
								   RES_STRING(CL_UIEQUIPFORM_CPP_00001),
								   "Selected items will be dropped on the ground.",
								   RmlInvPendingConfirm::ThrowSelected)) {
				return;
			}
			auto throwConfirm = [](CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
				if (nMsgType != CForm::mrYes)
					return;
				auto pSelect = static_cast<stSelectBox*>(pSender->GetForm()->GetPointer());
				if (pSelect && pSelect->pointer)
					g_stUIEquip.ThrowSelectedItems(static_cast<CGoodsGrid*>(pSelect->pointer));
			};
			auto pSelectBox = g_stUIBox.ShowSelectBox(throwConfirm, RES_STRING(CL_UIEQUIPFORM_CPP_00001), true);
			pSelectBox->pointer = reinterpret_cast<void*>(GetGoodsGrid());
			return;
		}
		ThrowSelectedItems(GetGoodsGrid());
		return;
	}

	if (strcmp(action, "lock") == 0) {
		if (ShowRmlItemConfirm("Lock Item",
							   "Do you wish to lock the item?",
							   "Locked items cannot be traded, thrown, or deleted until unlocked.",
							   RmlInvPendingConfirm::LockSelected)) {
			return;
		}
		auto lockItemsConfirm = [](CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
			if (nMsgType != CForm::mrYes)
				return;
			auto pSelect = static_cast<stSelectBox*>(pSender->GetForm()->GetPointer());
			if (pSelect && pSelect->pointer)
				LockSelectedItems(static_cast<CGoodsGrid*>(pSelect->pointer));
		};
		auto pSelectBox = g_stUIBox.ShowSelectBox(lockItemsConfirm, "Do you wish to lock the item?", true);
		pSelectBox->pointer = reinterpret_cast<void*>(GetGoodsGrid());
		return;
	}

	if (strcmp(action, "unlock") == 0) {
		if (GetGoodsGrid() && GetGoodsGrid()->GetSelectedItemCount() <= 1) {
			g_stUIDoublePwd.SetLockGridID(rightClickItemIndex);
			g_stUIDoublePwd.SetType(CDoublePwdMgr::ITEM_UNLOCK);
			g_stUIDoublePwd.ShowDoublePwdForm();
			return;
		}
		g_stUIDoublePwd.SetType(CDoublePwdMgr::MULTI_ITEM_UNLOCK);
		g_stUIDoublePwd.ShowDoublePwdForm();
		return;
	}

	if (strcmp(action, "sell") == 0) {
		if (GetGoodsGrid() && GetGoodsGrid()->GetSelectedItemCount() == 1) {
			ShowSellPrompt();
			return;
		}
		if (ShowRmlItemConfirm("Sell Item", "Sell all selected items?", "", RmlInvPendingConfirm::SellSelected))
			return;
		auto sellConfirm = [](CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
			if (nMsgType != CForm::mrYes)
				return;
			g_stUINpcTrade.SellSelectedItems(g_stUIEquip.GetGoodsGrid());
		};
		auto pSelectBox = g_stUIBox.ShowSelectBox(sellConfirm, "Sell all selected items?", true);
		pSelectBox->pointer = reinterpret_cast<void*>(GetGoodsGrid());
		return;
	}

	if (strcmp(action, "boxrates") == 0) {
		auto* item = dynamic_cast<CItemCommand*>(GetGoodsGrid() ? GetGoodsGrid()->GetItem(rightClickItemIndex) : nullptr);
		if (item && item->GetItemInfo())
			RequestChestPreview(item->GetItemInfo()->lID);
		return;
	}

	if (strcmp(action, "chat") == 0) {
		auto* chat = CCozeForm::GetInstance();
		auto* grid = GetGoodsGrid();
		if (!chat || !grid)
			return;

		auto buildRawLink = [](CItemCommand* item) -> std::string {
			const SItemGrid& d = item->GetData();
			std::string t = "[";
			t += item->GetItemInfo()->szName;
			t += "|";
			t += std::to_string(item->GetItemInfo()->lID);
			t += "|";
			t += std::to_string(static_cast<int>(d.chForgeLv));
			t += "|";
			t += std::to_string(d.lDBParam[enumITEMDBP_FORGE]);
			t += "]";
			return t;
		};

		constexpr int kMaxLinks = 5;
		int linked = 0;
		bool hasSelection = false;
		for (auto i = 0, n = grid->GetMaxNum(); i < n; ++i) {
			if (grid->IsItemSelected(i)) {
				hasSelection = true;
				break;
			}
		}

		chat->ActivateChatBox();
		if (hasSelection) {
			for (int slotIdx : grid->GetSelectionOrder()) {
				if (linked >= kMaxLinks)
					break;
				auto* item = dynamic_cast<CItemCommand*>(grid->GetItem(slotIdx));
				if (!item || !item->GetItemInfo())
					continue;
				chat->AddItemLinkToEdit(item->GetItemInfo()->szName, buildRawLink(item));
				++linked;
			}
		} else {
			auto* item = dynamic_cast<CItemCommand*>(grid->GetItem(rightClickItemIndex));
			if (item && item->GetItemInfo())
				chat->AddItemLinkToEdit(item->GetItemInfo()->szName, buildRawLink(item));
		}
	}
}

void CEquipMgr::RequestTogglePackageLock() {
	if (GetIsLock()) {
		g_stUIDoublePwd.SetType(CDoublePwdMgr::PACKAGE_UNLOCK);
		g_stUIDoublePwd.ShowDoublePwdForm();
		return;
	}
	if (CRmlUiInventoryForm::Instance().IsVisible()) {
		CRmlUiInventoryForm::Instance().ShowLockConfirmModal();
		return;
	}
	CBoxMgr::ShowSelectBox(_CheckLockMouseEvent, RES_STRING(CL_LANGUAGE_MATCH_824), true);
}

void CEquipMgr::RequestExpandBag() {
	if (CRmlUiInventoryForm::Instance().IsVisible()) {
		int unlocked = 0;
		if (CGoodsGrid* bag = GetGoodsGrid()) {
			const int maxNum = bag->GetMaxNum();
			for (int i = 0; i < maxNum; ++i) {
				CItemCommand* cmd = dynamic_cast<CItemCommand*>(bag->GetItem(i));
				if (!cmd) {
					unlocked++;
					continue;
				}
				CItemRecord* info = cmd->GetItemInfo();
				if (info && info->lID == 32767)
					continue;
				unlocked++;
			}
		}
		CRmlUiInventoryForm::Instance().ShowExpandBagModal(unlocked, 6, 100);
		return;
	}
	_evtExpandBagClick();
}

void CEquipMgr::RequestTempBag() {
	g_stUIStore.ShowTempKitbag();
}

static bool IsFairyStaminaEmpty(CItemCommand* cmd) {
	if (!cmd)
		return false;
	CItemRecord* info = cmd->GetItemInfo();
	if (!info || info->sType != enumItemTypePet)
		return false;
	// Constitution shown as sEndure/50; 0..49 raw URE displays as empty.
	return cmd->GetData().sEndure[0] <= 49;
}

void CEquipMgr::RefreshRmlInventory() {
	CRmlUiInventoryForm& rml = CRmlUiInventoryForm::Instance();
	if (!rml.IsVisible())
		return;

	static const int kLeft[] = {enumEQUIP_HEAD, enumEQUIP_BODY, enumEQUIP_GLOVE, enumEQUIP_SHOES, enumEQUIP_NECK};
	static const int kRight[] = {enumEQUIP_RHAND, enumEQUIP_LHAND, enumEQUIP_HAND1, enumEQUIP_HAND2, enumEQUIP_WING};
	static const int kBottom[] = {
		enumEQUIP_Jewelry1, enumEQUIP_Jewelry2, enumEQUIP_Jewelry3, enumEQUIP_Jewelry4,
		enumEQUIP_CLOAK, enumEQUIP_FAIRY, enumEQUIP_REAR, enumEQUIP_MOUNT};

	static const int kAppLeft[] = {enumEQUIP_HEADAPP, enumEQUIP_FACEAPP, enumEQUIP_BODYAPP, enumEQUIP_GLOVEAPP, enumEQUIP_SHOESAPP};
	static const int kAppRight[] = {enumEQUIP_FAIRYAPP, enumEQUIP_GLOWAPP, enumEQUIP_DAGGERAPP, enumEQUIP_GUNAPP, enumEQUIP_SWORD1APP};
	static const int kAppBottom[] = {
		enumEQUIP_GREATSWORDAPP, enumEQUIP_STAFFAPP, enumEQUIP_BOWAPP, enumEQUIP_SWORD2APP, enumEQUIP_SHIELDAPP};

	const bool apparel = rml.IsEquipModeApparel();
	const int* leftLinks = apparel ? kAppLeft : kLeft;
	const int* rightLinks = apparel ? kAppRight : kRight;
	const int* bottomLinks = apparel ? kAppBottom : kBottom;
	const int leftCount = apparel ? (int)(sizeof(kAppLeft) / sizeof(kAppLeft[0])) : (int)(sizeof(kLeft) / sizeof(kLeft[0]));
	const int rightCount = apparel ? (int)(sizeof(kAppRight) / sizeof(kAppRight[0])) : (int)(sizeof(kRight) / sizeof(kRight[0]));
	const int bottomCount = apparel ? (int)(sizeof(kAppBottom) / sizeof(kAppBottom[0])) : (int)(sizeof(kBottom) / sizeof(kBottom[0]));

	auto placeholderFor = [](int link) -> const char* {
		switch (link) {
		case enumEQUIP_HEAD:
		case enumEQUIP_HEADAPP:
			return "ui/rml/frames/notice/slots/helm.tga";
		case enumEQUIP_FACE:
		case enumEQUIP_FACEAPP:
			return "ui/rml/frames/notice/slots/face.tga";
		case enumEQUIP_BODY:
		case enumEQUIP_BODYAPP:
			return "ui/rml/frames/notice/slots/body.tga";
		case enumEQUIP_GLOVE:
		case enumEQUIP_GLOVEAPP:
			return "ui/rml/frames/notice/slots/glove.tga";
		case enumEQUIP_SHOES:
		case enumEQUIP_SHOESAPP:
			return "ui/rml/frames/notice/slots/shoes.tga";
		case enumEQUIP_NECK:
			return "ui/rml/frames/notice/slots/neck.tga";
		case enumEQUIP_RHAND:
		case enumEQUIP_SWORD1APP:
		case enumEQUIP_SWORD2APP:
			return "ui/rml/frames/notice/slots/sword.tga";
		case enumEQUIP_LHAND:
		case enumEQUIP_SHIELDAPP:
			return "ui/rml/frames/notice/slots/shield.tga";
		case enumEQUIP_HAND1:
		case enumEQUIP_HAND2:
		case enumEQUIP_Jewelry1:
		case enumEQUIP_Jewelry2:
		case enumEQUIP_Jewelry3:
		case enumEQUIP_Jewelry4:
			return "ui/rml/frames/notice/slots/ring.tga";
		case enumEQUIP_WING:
			return "ui/rml/frames/notice/slots/wing.tga";
		case enumEQUIP_CLOAK:
			return "ui/rml/frames/notice/slots/cloak.tga";
		case enumEQUIP_FAIRY:
		case enumEQUIP_FAIRYAPP:
			return "ui/rml/frames/notice/slots/fairy.tga";
		case enumEQUIP_REAR:
			return "ui/rml/frames/notice/slots/rear.tga";
		case enumEQUIP_MOUNT:
			return "ui/rml/frames/notice/slots/mount.tga";
		case enumEQUIP_GLOWAPP:
			return "ui/rml/frames/notice/slots/glow.tga";
		case enumEQUIP_DAGGERAPP:
			return "ui/rml/frames/notice/slots/dagger.tga";
		case enumEQUIP_GUNAPP:
			return "ui/rml/frames/notice/slots/gun.tga";
		case enumEQUIP_GREATSWORDAPP:
			return "ui/rml/frames/notice/slots/greatsword.tga";
		case enumEQUIP_STAFFAPP:
			return "ui/rml/frames/notice/slots/staff.tga";
		case enumEQUIP_BOWAPP:
			return "ui/rml/frames/notice/slots/bow.tga";
		default:
			return nullptr;
		}
	};

	auto fillEquip = [&](const int* links, int count, std::vector<RmlInvSlotView>& out) {
		out.clear();
		out.reserve(count);
		for (int i = 0; i < count; ++i) {
			RmlInvSlotView view;
			view.id = links[i];
			if (const char* ph = placeholderFor(links[i]))
				view.placeholderPath = ph;
			if (CItemCommand* item = GetEquipItem((unsigned int)links[i])) {
				if (CItemRecord* info = item->GetItemInfo())
					view.iconPath = info->GetIconFile();
				view.iconDimmed = IsFairyStaminaEmpty(item);
			}
			out.push_back(view);
		}
	};

	std::vector<RmlInvSlotView> left, right, bottom;
	fillEquip(leftLinks, leftCount, left);
	fillEquip(rightLinks, rightCount, right);
	fillEquip(bottomLinks, bottomCount, bottom);
	rml.SetEquipSlots(left, right, bottom);
	rml.SetPackageLocked(GetIsLock());

	CGoodsGrid* bag = GetGoodsGrid();
	std::vector<RmlInvSlotView> bagSlots;
	int used = 0;
	int unlocked = 0;
	int cols = 6;
	const char* filter = rml.GetFilter();

	const EItemInvTab invTab = CItemRecord::InvTabFromFilter(filter);
	const bool showAllSlots = (invTab == enumInvTabAll);

	if (bag) {
		cols = bag->GetCol() > 0 ? bag->GetCol() : 6;
		const int maxNum = bag->GetMaxNum();
		bagSlots.reserve(maxNum);
		for (int i = 0; i < maxNum; ++i) {
			CItemCommand* cmd = dynamic_cast<CItemCommand*>(bag->GetItem(i));
			if (!cmd) {
				unlocked++;
				// Keep empty drop targets on the full Items view only.
				if (showAllSlots) {
					RmlInvSlotView view;
					view.id = i;
					bagSlots.push_back(view);
				}
				continue;
			}
			CItemRecord* info = cmd->GetItemInfo();
			if (info && info->lID == 32767) {
				if (showAllSlots) {
					RmlInvSlotView view;
					view.id = i;
					view.locked = true;
					bagSlots.push_back(view);
				}
				continue;
			}
			unlocked++;
			used++;
			if (info && !info->MatchesInvTab(invTab))
				continue;
			RmlInvSlotView view;
			view.id = i;
			if (info)
				view.iconPath = info->GetIconFile();
			view.qty = cmd->GetTotalNum();
			view.iconDimmed = IsFairyStaminaEmpty(cmd);
			view.selected = bag->IsItemSelected(i);
			bagSlots.push_back(view);
		}
	}

	rml.SetBagSlots(bagSlots, cols);
	rml.SetCapacity(used, unlocked > 0 ? unlocked : (int)bagSlots.size());

	char goldBuf[32] = "0";
	char impBuf[32] = "0";
	if (CCharacter* pCha = g_stUIBoat.GetHuman()) {
		if (SGameAttr* attr = pCha->getGameAttr())
			sprintf(goldBuf, "%s", StringSplitNum(attr->get(ATTR_GD)));
	}
	sprintf(impBuf, "%s", StringSplitNum(GetIMP()));
	rml.SetCurrency(goldBuf, impBuf);
}

void CEquipMgr::UseBagItem(int bagIndex) {
	CGoodsGrid* bag = GetGoodsGrid();
	if (!bag || bagIndex < 0)
		return;
	CItemCommand* cmd = dynamic_cast<CItemCommand*>(bag->GetItem(bagIndex));
	if (!cmd)
		return;
	if (cmd->GetItemInfo() && cmd->GetItemInfo()->lID == 32767)
		return;
	// Allow equipping exhausted fairies; other invalid items stay blocked.
	if (!cmd->GetIsValid()) {
		CItemRecord* info = cmd->GetItemInfo();
		if (!info || info->sType != enumItemTypePet)
			return;
	}
	cmd->Exec();
}

void CEquipMgr::MoveBagItem(int srcBagIndex, int dstBagIndex, short srcNum) {
	if (srcBagIndex < 0 || dstBagIndex < 0 || srcBagIndex == dstBagIndex)
		return;

	CGoodsGrid* bag = GetGoodsGrid();
	if (!bag)
		return;

	CCharacter* pSelf = g_stUIBoat.FindCha(bag);
	if (!pSelf)
		pSelf = g_stUIBoat.GetHuman();
	if (!pSelf)
		return;

	if (pSelf->IsBoat() && pSelf != CGameScene::GetMainCha()) {
		g_pGameApp->SysInfo(RES_STRING(CL_LANGUAGE_MATCH_557));
		return;
	}

	CItemCommand* pItem = dynamic_cast<CItemCommand*>(bag->GetItem(srcBagIndex));
	CItemCommand* pTarget = dynamic_cast<CItemCommand*>(bag->GetItem(dstBagIndex));
	// Do not block on GetIsValid(): fairies/gear with low URE are marked invalid
	// (can't use/equip) but must still be rearrangeable in the bag.
	if (!pItem)
		return;
	if (pItem->GetItemInfo() && pItem->GetItemInfo()->lID == 32767)
		return;
	if (pTarget && pTarget->GetItemInfo() && pTarget->GetItemInfo()->lID == 32767)
		return;

	if (srcNum <= 0 && g_pGameApp->IsShiftPress() && pItem->GetTotalNum() > 1) {
		if (!pTarget || (pTarget->GetItemInfo() && pItem->GetItemInfo() &&
						 pTarget->GetItemInfo()->lID == pItem->GetItemInfo()->lID)) {
			SSplit.nFirst = dstBagIndex;
			SSplit.nSecond = srcBagIndex;
			SSplit.pSelf = pSelf;
			CBoxMgr::ShowNumberBox(SSplitItem::_evtSplitItemEvent, pItem->GetTotalNum());
			return;
		}
	}

	stNetItemPos info;
	info.sSrcGridID = (short)srcBagIndex;
	info.sSrcNum = srcNum > 0 ? srcNum : 0;
	info.sTarGridID = (short)dstBagIndex;
	CS_BeginAction(pSelf, enumACTION_ITEM_POS, (void*)&info);
}

void CEquipMgr::UnequipLink(int link, int bagIndex) {
	if (link < 0 || link >= enumEQUIP_NUM)
		return;
	CItemCommand* eq = GetEquipItem((unsigned int)link);
	if (!eq)
		return;
	UnfixToGrid(eq, bagIndex, link);
}

void RmlInv_OnClose() {
	g_stUIEquip.HideInventoryUi();
}

void RmlInv_OnFilterChanged() {
	g_stUIEquip.RefreshRmlInventory();
}

void RmlInv_OnToggleApparel(bool apparel) {
	CRmlUiInventoryForm::Instance().SetEquipModeApparel(apparel);
	g_stUIEquip.RefreshRmlInventory();
}

void RmlInv_OnToggleLock() {
	g_stUIEquip.RequestTogglePackageLock();
}

void RmlInv_OnExpandBag() {
	g_stUIEquip.RequestExpandBag();
}

void RmlInv_OnTempBag() {
	g_stUIEquip.RequestTempBag();
}

void RmlInv_OnLockConfirm() {
	CS_LockKitbag();
}

void CEquipMgr::OnRmlItemConfirmYes() {
	const RmlInvPendingConfirm action = g_rmlInvPendingConfirm;
	g_rmlInvPendingConfirm = RmlInvPendingConfirm::None;

	switch (action) {
	case RmlInvPendingConfirm::DeleteSelected:
		DeleteSelectedItems(GetGoodsGrid());
		break;
	case RmlInvPendingConfirm::ThrowSelected:
		ThrowSelectedItems(GetGoodsGrid());
		break;
	case RmlInvPendingConfirm::LockSelected:
		LockSelectedItems(GetGoodsGrid());
		break;
	case RmlInvPendingConfirm::SellSelected:
		g_stUINpcTrade.SellSelectedItems(GetGoodsGrid());
		break;
	case RmlInvPendingConfirm::ThrowSingle: {
		if (!_sThrow.pSelf)
			break;
		stNetItemThrow item;
		item.lNum = g_rmlInvPendingThrowNum;
		item.sGridID = _sThrow.nGridID;
		item.lPosX = _sThrow.nX;
		item.lPosY = _sThrow.nY;
		CS_BeginAction(_sThrow.pSelf, enumACTION_ITEM_THROW, (void*)&item);
		break;
	}
	default:
		break;
	}
	RefreshRmlInventory();
}

void CEquipMgr::OnRmlItemConfirmNo() {
	g_rmlInvPendingConfirm = RmlInvPendingConfirm::None;
}

void RmlInv_OnItemConfirmYes() {
	g_stUIEquip.OnRmlItemConfirmYes();
}

void RmlInv_OnItemConfirmNo() {
	g_stUIEquip.OnRmlItemConfirmNo();
}

void RmlInv_OnContextAction(const char* action) {
	g_stUIEquip.ExecuteBagContextAction(action);
}

void RmlInv_OnExpandConfirm() {
	if (g_stUIEquip.GetIMP() < 100) {
		g_pGameApp->SysInfo("Not enough IMP (need 100).");
		return;
	}
	CS_KitbagExpand();
}

void RmlInv_OnBagDblClick(int index) {
	g_stUIEquip.UseBagItem(index);
	g_stUIEquip.MarkChaPreviewDirty();
	g_stUIEquip.RefreshRmlInventory();
}

bool RmlInv_OnBagMouseDown(int index, bool ctrl) {
	CGoodsGrid* bag = g_stUIEquip.GetGoodsGrid();
	if (!bag || index < 0)
		return false;

	CItemCommand* cmd = dynamic_cast<CItemCommand*>(bag->GetItem(index));
	if (!cmd)
		return false;
	if (cmd->GetItemInfo() && cmd->GetItemInfo()->lID == 32767)
		return false;
	if (!cmd->GetCanDrag())
		return false;

	auto syncSelection = [bag]() {
		std::vector<char> flags((size_t)bag->GetMaxNum(), 0);
		for (int i = 0; i < bag->GetMaxNum(); ++i)
			flags[(size_t)i] = bag->IsItemSelected(i) ? 1 : 0;
		CRmlUiInventoryForm::Instance().ApplyBagSelection(flags);
	};

	if (ctrl) {
		bag->ToggleItemSelected(index);
		syncSelection();
		return true; // suppress drag
	}

	if (bag->GetSelectedItemCount() > 0) {
		bag->ResetItemSelections();
		syncSelection();
	}
	return false;
}

void RmlInv_OnBagRightClick(int index) {
	g_stUIEquip.ShowBagContextMenu(index);
}

void RmlInv_OnEquipDblClick(int link) {
	g_stUIEquip.UnequipLink(link, -1);
	g_stUIEquip.MarkChaPreviewDirty();
	g_stUIEquip.RefreshRmlInventory();
}

void RmlInv_OnDrop(int srcBag, int srcEquip, int dstBag, int dstEquip) {
	if (srcBag >= 0 && dstBag >= 0) {
		g_stUIEquip.MoveBagItem(srcBag, dstBag);
	} else if (srcBag >= 0 && dstEquip >= 0) {
		g_stUIEquip.UseBagItem(srcBag);
		g_stUIEquip.MarkChaPreviewDirty();
	} else if (srcEquip >= 0 && dstBag >= 0) {
		g_stUIEquip.UnequipLink(srcEquip, dstBag);
		g_stUIEquip.MarkChaPreviewDirty();
	}
	// UI rebuild is deferred to RmlInv_OnItemDragEnd so the drag source isn't destroyed mid-drop.
}

void RmlInv_OnItemDragEnd() {
	g_stUIEquip.RefreshRmlInventory();
}

void RmlInv_ApplyItemHint(int bagIndex, int equipLink, int mouseX, int mouseY) {
	CItemCommand* cmd = nullptr;
	if (bagIndex >= 0) {
		CGoodsGrid* bag = g_stUIEquip.GetGoodsGrid();
		if (bag)
			cmd = dynamic_cast<CItemCommand*>(bag->GetItem(bagIndex));
	} else if (equipLink >= 0) {
		cmd = g_stUIEquip.GetEquipItem((unsigned int)equipLink);
	}
	if (!cmd)
		return;

	CGuiData::SetHintItem(cmd);
	cmd->ReadyForHint(mouseX, mouseY, nullptr);
}

void RmlInv_RenderItemHint() {
	if (CItemObj* item = CGuiData::GetHintItem())
		item->RenderHint(0, 0);
}

void RmlInv_RenderChaPreview(int centerX, int centerY) {
	g_stUIEquip.RenderChaPreview(centerX, centerY);
}

void RmlInv_RotatePreviewLeft() {
	g_stUIEquip.RotateChaPreviewLeft();
}

void RmlInv_RotatePreviewRight() {
	g_stUIEquip.RotateChaPreviewRight();
}

// ??�??????j????????????
int CEquipMgr::GetItemCount(int nID) {
	int nRet = 0;
	for (int i = 0; i < grdItem->GetMaxNum(); ++i) {
		CItemCommand* pItem = dynamic_cast<CItemCommand*>(grdItem->GetItem(i));
		if (pItem && pItem->GetItemInfo()) {
			if (pItem->GetItemInfo()->lID == nID) {
				nRet += pItem->GetTotalNum();
			}
		}
	}

	return nRet;
}

void CEquipMgr::_evtItemFormMouseEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	string strName = pSender->GetName();

	if (strName == "btnLock") {
		if (g_stUIEquip.GetIsLock()) {
			// ????
			g_stUIDoublePwd.SetType(CDoublePwdMgr::PACKAGE_UNLOCK);
			g_stUIDoublePwd.ShowDoublePwdForm();
		} else {
			CBoxMgr::ShowSelectBox(_CheckLockMouseEvent, RES_STRING(CL_LANGUAGE_MATCH_824), true);
		}
	} else if (strName == "btnExpandBag") {
		_evtExpandBagClick();
	}
}

void CEquipMgr::SetIsLock(bool bLock) {
	imgLock->SetIsShow(bLock);
	imgUnLock->SetIsShow(!bLock);
	if (CRmlUiInventoryForm::Instance().IsVisible())
		CRmlUiInventoryForm::Instance().SetPackageLocked(bLock);
}

bool CEquipMgr::GetIsLock() {
	return imgLock->GetIsShow();
}

// ???? MSGBOX ???
void CEquipMgr::_CheckLockMouseEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	if (nMsgType == CForm::mrYes) {
		CS_LockKitbag();
	}
}

void CEquipMgr::_evtExpandBagClick() {
	g_stUIBox.ShowSelectBox(_evtExpandBagConfirm,
		"Do you want to extend your inventory by +6 slots for 100 IMP?", true);
}

void CEquipMgr::_evtExpandBagConfirm(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	if (nMsgType != CForm::mrYes)
		return;

	if (g_stUIEquip.GetIMP() < 100) {
		g_pGameApp->SysInfo("Not enough IMP (need 100).");
		return;
	}

	CS_KitbagExpand();
}

void CEquipMgr::_OnDragStates(CGuiData* pSender, int x, int y, DWORD key) {
	if (g_stUIEquip.stateDrags->GetTop() <= 0) {
		g_stUIEquip.stateDrags->SetPos(g_stUIEquip.stateDrags->GetLeft(), 0);
		g_stUIEquip.stateDrags->Refresh();
	}

	if (g_stUIEquip.stateDrags->GetLeft() <= 0) {
		g_stUIEquip.stateDrags->SetPos(0, g_stUIEquip.stateDrags->GetTop());
		g_stUIEquip.stateDrags->Refresh();
	}

	if (g_stUIEquip.stateDrags->GetBottom() >= GetRender().GetScreenHeight()) {
		g_stUIEquip.stateDrags->SetPos(g_stUIEquip.stateDrags->GetLeft(), GetRender().GetScreenHeight() - g_stUIEquip.stateDrags->GetHeight());
		g_stUIEquip.stateDrags->Refresh();
	}

	if (g_stUIEquip.stateDrags->GetRight() >= GetRender().GetScreenWidth()) {
		g_stUIEquip.stateDrags->SetPos(GetRender().GetScreenWidth() - g_stUIEquip.stateDrags->GetWidth() + 20, g_stUIEquip.stateDrags->GetTop());
		g_stUIEquip.stateDrags->Refresh();
	}
}