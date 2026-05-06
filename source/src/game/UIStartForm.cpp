#include "StdAfx.h"
#include "uistartform.h"
#include "uiform.h"
#include "uitextbutton.h"
#include "uiformmgr.h"
#include "uiprogressbar.h"
#include "uilabel.h"
#include "netchat.h"
#include "packetcmd.h"
#include "gameapp.h"
#include "uigrid.h"
#include "character.h"
#include "uiheadsay.h"
#include "uimenu.h"
#include "uilist.h"
#include "uigrid.h"
#include "UIGlobalVar.h"
#include "UIMisLogForm.h"
#include "UICozeForm.h"
#include "NetGuild.h"
#include "uiguildmgr.h"
#include "worldscene.h"
#include "uititle.h"
#include "uiboxform.h"
#include "shipfactory.h"
#include "uiboatform.h"
#include "arearecord.h"
#include "isskilluse.h"
#include "ui3dcompent.h"
#include "smallmap.h"
#include "mapset.h"
#include "uiequipform.h"
#include "uiTradeForm.h"
#include "uiFindTeamForm.h"
#include "uistoreform.h"
#include "uidoublepwdform.h"
#include "uiitemcommand.h"
#include "uiminimapform.h"
#include "uibankform.h"
#include "uiboothform.h"
#include "uitradeform.h"
#include "UIChat.h"
#include "UITeam.h"
#include "AutoAttack.h"
// Add by lark.li 20080811 begin
#include "UITeam.h"
// End
#include "StringLib.h"
#include "LootFilter.h"
#include "UIScroll.h"

#include <algorithm>

using namespace std;
using namespace GUI;

static CForm* frmSelectOriginRelive = nullptr;

//---------------------------------------------------------------------------
// class CStartMgr
//---------------------------------------------------------------------------
CMenu* CStartMgr::mainMouseRight = nullptr;
CTextButton* CStartMgr::btnQQ = nullptr;
CCharacter2D* CStartMgr::pMainCha = nullptr;
CCharacter2D* CStartMgr::pTarget = nullptr;

// Safe helper to get target character by ID - prevents dangling pointer crashes
// Returns nullptr if character no longer exists in scene
CCharacter* CStartMgr::GetTarget() {
	if (targetInfoID == 0) {
		return nullptr;
	}
	CGameScene* pScene = CGameApp::GetCurScene();
	if (!pScene) {
		return nullptr;
	}
	// Look up character by ID - this is the ONLY safe way to access it
	CCharacter* pCha = pScene->SearchByID(targetInfoID);
	if (!pCha || !pCha->IsValid()) {
		return nullptr;
	}
	return pCha;
}

static char szBuf[32] = {0};

float g_ExpBonus = 1.0;
float g_DropBonus = 1.0;

bool sortcol(const vector<int>& v1, const vector<int>& v2) {
	return v1[1] < v2[1];
}

// Scroll change callback for drop list
static void _evtDropScrollChange(CGuiData* pSender) {
	g_stUIStart.OnDropScrollChange();
}

// Mouse wheel scroll handler for monster drop list
static bool _onDropListMouseScroll(int& nScroll) {
	return g_stUIStart.HandleDropListScroll(nScroll);
}

void CStartMgr::CleanDropListForm() {
	for (int i = 0; i < DROP_VISIBLE_COUNT; i++) {
		if (listMobDrops[i]) {
			listMobDrops[i]->DelCommand();
		}
		if (LabMobItems[i]) {
			LabMobItems[i]->SetCaption("");
			LabMobItems[i]->SetHint("");
		}
		if (LabMobRates[i])
			LabMobRates[i]->SetCaption("");
		if (checkDropFilter[i])
			checkDropFilter[i]->SetIsShow(false);
	}
}

void CStartMgr::SetMonsterInfo() {
	// Use safe ID-based lookup to get target character
	CCharacter* pCachedCha = GetTarget();
	if (!pCachedCha) {
		// Target is no longer valid, clean up UI
		if (frmMonsterInfo) {
			frmMonsterInfo->Hide();
		}
		return;
	}

	CChaRecord* charInfo = GetChaRecordInfo(pCachedCha->getMobID());
	if (!charInfo) {
		return;
	}

	// Clear and rebuild drop data
	m_vDropData.clear();
	m_vDropData.resize(defCHA_INIT_ITEM_NUM, vector<int>(2, 0));

	int max = defCHA_INIT_ITEM_NUM;
	for (int i = 0; i < max; i++) {
		m_vDropData[i][0] = ((int)charInfo->lItem[i][0]);
		m_vDropData[i][1] = (int)charInfo->lItem[i][1];
		if (GetItemRecordInfo(m_vDropData[i][0]) == nullptr) {
			max = i;
			break;
		}
	}

	// Resize to actual item count and sort
	m_vDropData.resize(max);
	sort(m_vDropData.begin(), m_vDropData.end(), sortcol);

	// Store total count and reset scroll position
	m_nDropTotalCount = max;
	m_nDropScrollPos = 0;

	// Update scroll range if scroll exists
	if (pMobDropScroll) {
		float fMax = (float)(m_nDropTotalCount - DROP_VISIBLE_COUNT);
		if (fMax < 0.0f) fMax = 0.0f;
		pMobDropScroll->SetRange(0.0f, fMax);
		pMobDropScroll->Reset();
		pMobDropScroll->SetIsShow(m_nDropTotalCount > DROP_VISIBLE_COUNT);
	}

	// Update the display
	UpdateDropListDisplay();

	// Get main character safely
	CGameScene* pScene = g_pGameApp->GetCurScene();
	if (!pScene || !pScene->GetMainCha()) {
		return;
	}

	long chaLevel = pScene->GetMainCha()->getLv();
	long mobLevel = charInfo->lLv;
	long levelDif = chaLevel - mobLevel;
	double b = 1;

	if (levelDif >= 4) {
		b = std::min<unsigned short>(10, 1 + (0.4 * abs(levelDif - 4)));
	} else if (levelDif <= -10) {
		b = std::min<unsigned short>(4, 1 + abs(levelDif - 10) * 0.1);
	}

	double ExpAdd = floor(std::max<double>(1, charInfo->lCExp / b)) * g_ExpBonus;
	
	// Set UI labels with null checks
	if (LabMobLevel) LabMobLevel->SetCaption(std::to_string(charInfo->lLv).c_str());
	if (LabMobexp) LabMobexp->SetCaption(std::to_string(ExpAdd).c_str());
	if (LabMobHP) LabMobHP->SetCaption(std::to_string(charInfo->lMxHp).c_str());
	if (LabMobAttack) LabMobAttack->SetCaption((std::to_string(charInfo->lMnAtk) + "/" + std::to_string(charInfo->lMxAtk)).c_str());
	if (LabMobHitRate) LabMobHitRate->SetCaption(std::to_string(charInfo->lHit).c_str());
	if (LabMobDodge) LabMobDodge->SetCaption(std::to_string(charInfo->lFlee).c_str());
	if (LabMobDef) LabMobDef->SetCaption(std::to_string(charInfo->lDef).c_str());
	if (LabMobPR) LabMobPR->SetCaption(std::to_string(charInfo->lPDef).c_str());
	if (LabMobAtSpeed) LabMobAtSpeed->SetCaption(std::to_string(charInfo->lASpd).c_str());
	if (LabMobMSpeed) LabMobMSpeed->SetCaption(std::to_string(charInfo->lMSpd).c_str());

	if (frmMonsterInfo) {
		frmMonsterInfo->Refresh();
	}
}

void CStartMgr::ShowMonsterInfoByCharId(int charId) {
	if (!frmMonsterInfo) return;
	CChaRecord* charInfo = GetChaRecordInfo(charId);
	if (!charInfo) return;

	m_vDropData.clear();
	m_vDropData.resize(defCHA_INIT_ITEM_NUM, vector<int>(2, 0));

	int max = defCHA_INIT_ITEM_NUM;
	for (int i = 0; i < max; i++) {
		m_vDropData[i][0] = (int)charInfo->lItem[i][0];
		m_vDropData[i][1] = (int)charInfo->lItem[i][1];
		if (GetItemRecordInfo(m_vDropData[i][0]) == nullptr) {
			max = i;
			break;
		}
	}
	m_vDropData.resize(max);
	sort(m_vDropData.begin(), m_vDropData.end(), sortcol);

	m_nDropTotalCount = max;
	m_nDropScrollPos = 0;

	if (pMobDropScroll) {
		float fMax = (float)(m_nDropTotalCount - DROP_VISIBLE_COUNT);
		if (fMax < 0.0f) fMax = 0.0f;
		pMobDropScroll->SetRange(0.0f, fMax);
		pMobDropScroll->Reset();
		pMobDropScroll->SetIsShow(m_nDropTotalCount > DROP_VISIBLE_COUNT);
	}

	UpdateDropListDisplay();

	if (LabMobLevel)   LabMobLevel->SetCaption(std::to_string(charInfo->lLv).c_str());
	if (LabMobexp)     LabMobexp->SetCaption(std::to_string((long long)(charInfo->lCExp * g_ExpBonus)).c_str());
	if (LabMobHP)      LabMobHP->SetCaption(std::to_string(charInfo->lMxHp).c_str());
	if (LabMobAttack)  LabMobAttack->SetCaption((std::to_string(charInfo->lMnAtk) + "/" + std::to_string(charInfo->lMxAtk)).c_str());
	if (LabMobHitRate) LabMobHitRate->SetCaption(std::to_string(charInfo->lHit).c_str());
	if (LabMobDodge)   LabMobDodge->SetCaption(std::to_string(charInfo->lFlee).c_str());
	if (LabMobDef)     LabMobDef->SetCaption(std::to_string(charInfo->lDef).c_str());
	if (LabMobPR)      LabMobPR->SetCaption(std::to_string(charInfo->lPDef).c_str());
	if (LabMobAtSpeed) LabMobAtSpeed->SetCaption(std::to_string(charInfo->lASpd).c_str());
	if (LabMobMSpeed)  LabMobMSpeed->SetCaption(std::to_string(charInfo->lMSpd).c_str());

	frmMonsterInfo->Refresh();
	frmMonsterInfo->Show();
}

void CStartMgr::UpdateDropListDisplay() {
	// Clear all visible slots first
	for (int i = 0; i < DROP_VISIBLE_COUNT; i++) {
		if (listMobDrops[i]) {
			listMobDrops[i]->DelCommand();
		}
		if (LabMobItems[i]) {
			LabMobItems[i]->SetCaption("");
			LabMobItems[i]->SetHint("");
		}
		if (LabMobRates[i]) {
			LabMobRates[i]->SetCaption("");
		}
		if (checkDropFilter[i]) {
			checkDropFilter[i]->SetIsShow(false);
		}
	}

	// Fill visible slots with data based on scroll position
	for (int i = 0; i < DROP_VISIBLE_COUNT; i++) {
		int dataIndex = m_nDropScrollPos + i;
		if (dataIndex >= m_nDropTotalCount) {
			break;
		}

		CItemRecord* rInfo = GetItemRecordInfo(m_vDropData[dataIndex][0]);
		if (!rInfo)
			continue;

		CItemCommand* rItem = new CItemCommand(rInfo);
		if (!rItem)
			continue;

		listMobDrops[i]->AddCommand(rItem);
		listMobDrops[i]->SetIsEnabled(false);

		const char* item_name = rInfo->szName;
		char get_name[128] = {0};
		sprintf(get_name, "%s", StringLimit(item_name, 16).c_str());
		LabMobItems[i]->SetCaption(get_name);

		float calcuDrop = (m_vDropData[dataIndex][1] != 0) ? (10000 / static_cast<float>(m_vDropData[dataIndex][1])) * g_DropBonus : 0;
		calcuDrop = std::min(calcuDrop, 100.0f);

		char item_rate[25];
		snprintf(item_rate, sizeof(item_rate), "%0.2f%%", calcuDrop);
		LabMobRates[i]->SetCaption(item_rate);

		if (checkDropFilter[i]) {
			checkDropFilter[i]->nTag = rInfo->lID;
			checkDropFilter[i]->SetIsShow(true);
			checkDropFilter[i]->SetIsChecked(!g_lootFilter->HasFilteredItem(rInfo->lID));
		}
	}
}

void CStartMgr::OnDropScrollChange() {
	if (pMobDropScroll) {
		m_nDropScrollPos = (int)pMobDropScroll->GetStep().GetPosition();
		UpdateDropListDisplay();
	}
}

bool CStartMgr::HandleDropListScroll(int nScroll) {
	if (!frmMonsterInfo || !frmMonsterInfo->GetIsShow())
		return false;
	if (!frmMonsterInfo->InRect(CGuiData::GetMouseX(), CGuiData::GetMouseY()))
		return false;
	if (m_nDropTotalCount <= DROP_VISIBLE_COUNT)
		return false;
	if (pMobDropScroll)
		return pMobDropScroll->MouseScroll(nScroll);
	return false;
}

void CStartMgr::SetTargetInfo(CCharacter* pTargetCha) {
	// Validate input pointer and its validity state
	if (!pTargetCha || !pTargetCha->IsValid()) {
		return;
	}

	// Validate UI elements exist before accessing them
	if (!labTargetInfoName || !labTargetLevel || !frmTargetInfo) {
		return;
	}

	char chaLv[16] = {0};
	snprintf(chaLv, sizeof(chaLv), "%d", pTargetCha->getLv());
	labTargetInfoName->SetCaption(pTargetCha->getName());
	targetInfoID = pTargetCha->getHumanID();
	RefreshTargetLifeNum(pTargetCha->getHP(), pTargetCha->getHPMax());
	labTargetLevel->SetCaption(chaLv);
	frmTargetInfo->Show();

	// Check if target changed and hide monster info if different mob type
	// Use safe lookup to get old cached target
	CCharacter* pOldCached = GetTarget();
	if (pOldCached && frmMonsterInfo) {
		if (pOldCached->getMobID() != pTargetCha->getMobID()) {
			frmMonsterInfo->Hide();
		}
	}

	// Store the target's ID - GetTarget() will use this for safe lookups
	targetInfoID = pTargetCha->getHumanID();
	
	// Only refresh model if the character is still valid
	if (pTargetCha->IsValid()) {
		RefreshTargetModel(pTargetCha);
	}
}

void CStartMgr::RefreshTargetModel(CCharacter* pTargetCha) {
	// Validate both the UI component and the character pointer
	if (!pTarget || !pTargetCha || !pTargetCha->IsValid()) {
		return;
	}

	static stNetTeamChaPart stTeamPart;
	stTeamPart.Convert(pTargetCha->GetPart());

	if (!pTargetCha->IsPlayer()) {
		pTarget->LoadCha(pTargetCha->getMobID());
	}
	if (pTargetCha->IsPlayer()) {
		pTarget->UpdataFace(stTeamPart);
	} else {
		pTarget->UpdataFace(stTeamPart, false);
	}
}

void CStartMgr::RemoveTarget() {
	if (frmTargetInfo) {
		frmTargetInfo->Hide();
	}
	if (frmMonsterInfo) {
		frmMonsterInfo->Hide();
	}
	// Clear the target ID - GetTarget() will now return nullptr
	targetInfoID = 0;
}

void CStartMgr::UpdateBackDrop() {
	CCharacter* pMain = CGameScene::GetMainCha();
	int nArea = CGameApp::GetCurScene()->GetTerrain()->GetTile(pMain->GetCurX() / 100, pMain->GetCurY() / 100)->getIsland();
	CWorldScene* world = dynamic_cast<CWorldScene*>(CGameApp::GetCurScene());

	if (nArea == world->GetOldMainChaInArea()) {
		return;
	}

	world->SetOldMainChaInArea(nArea);

	char buf[64];

	if (nArea) {
		sprintf(buf, "texture/ui/corsairs/npcBackdrop/%d.tga", nArea);
	} else {
		sprintf(buf, "texture/ui/corsairs/npcBackdrop/sea.tga");
	}
	CCompent* imgBackDropPlayer = dynamic_cast<CCompent*>(g_stUIStart.frmDetail->Find("imgBackDropPlayer"));
	CCompent* imgBackDropTarget = dynamic_cast<CCompent*>(g_stUIStart.frmTargetInfo->Find("imgBackDropTarget"));
	CCompent* teamBackDrops[4];

	for (int i = 0; i < 4; i++) {
		char formName[32];
		char imgName[32];
		sprintf(formName, "frmTeamMenber%d", i + 1);
		sprintf(imgName, "imgBackDropTeam%d", i + 1);
		teamBackDrops[i] = dynamic_cast<CCompent*>(g_stUIStart._FindForm(formName)->Find(imgName));
	}

	// https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c/25450408#25450408 # 3
	if (GetFileAttributes(buf) == INVALID_FILE_ATTRIBUTES) {
		imgBackDropPlayer->GetImage()->LoadImage("texture/ui/corsairs/npcBackdrop/0.tga", 55, 44, 0, 0, 0);
		imgBackDropTarget->GetImage()->LoadImage("texture/ui/corsairs/npcBackdrop/0.tga", 55, 44, 0, 0, 0);
		for (int i = 0; i < 4; i++) {
			teamBackDrops[i]->GetImage()->LoadImage("texture/ui/corsairs/npcBackdrop/0.tga", 55, 44, 0, 0, 0);
		}
	} else {
		imgBackDropPlayer->GetImage()->LoadImage(buf, 55, 44, 0, 0, 0);
		imgBackDropTarget->GetImage()->LoadImage(buf, 55, 44, 0, 0, 0);
		for (int i = 0; i < 4; i++) {
			teamBackDrops[i]->GetImage()->LoadImage(buf, 55, 44, 0, 0, 0);
		}
	}
}

void CStartMgr::RefreshTargetLifeNum(long num, long max) {
	if (num < 0) {
		num = 0;
	}
	if (num > max) {
		num = max;
	}
	if (max == 0) {
		max = 1;
		num = 0;
	}
	proTargetInfoHP->SetRange(0.0f, (float)max);
	proTargetInfoHP->SetPosition((float)num);
	if (num == 0) {
		RemoveTarget();
	}
}

void CStartMgr::_TargetRenderEvent(C3DCompent* pSender, int x, int y) {
	pTarget->Render();
}

bool CStartMgr::Init() {
	_IsNewer = false;
	_IsCanTeam = true;

	{
		frmTargetInfo = _FindForm("frmTargetInfo");
		if (frmTargetInfo) {
			frmTargetInfo->Refresh();
			proTargetInfoHP = dynamic_cast<CProgressBar*>(frmTargetInfo->Find("frmTargetInfoHP"));
			proTargetInfoHP->SetPosition(100.0f);

			labTargetInfoName = dynamic_cast<CLabel*>(frmTargetInfo->Find("frmTargetInfoName"));

			labTargetLevel = dynamic_cast<CLabel*>(frmTargetInfo->Find("labTargetLv"));

			btnMonsterInfo = dynamic_cast<CTextButton*>(frmTargetInfo->Find("btnMonsterInfo"));
			if (!btnMonsterInfo)
				return false;
			btnMonsterInfo->evtMouseClick = _evtShowMonsterInfo;

			C3DCompent* p3D = dynamic_cast<C3DCompent*>(frmTargetInfo->Find("d3dTarget"));
			if (!p3D)
				return Error(RES_STRING(CL_LANGUAGE_MATCH_473), frmDetail->GetName(), "d3dTarget");

			p3D->SetRenderEvent(_TargetRenderEvent);

			RECT rt;
			rt.left = p3D->GetX();
			rt.right = p3D->GetX2();
			rt.top = p3D->GetY();
			rt.bottom = p3D->GetY2();

			pTarget = new CCharacter2D;
			pTarget->Create(rt);
		}
	}

	{
		frmMonsterInfo = _FindForm("frmMonsterInfo");
		if (frmMonsterInfo) {
			frmMonsterInfo->Refresh();
			lstMobDrop = dynamic_cast<CForm*>(frmMonsterInfo->Find("lstMobDrop"));
			assert(lstMobDrop != nullptr);
			lstMobInfo = dynamic_cast<CForm*>(frmMonsterInfo->Find("lstMobInfo"));
			assert(lstMobInfo != nullptr);
			listInfo = dynamic_cast<CPage*>(frmMonsterInfo->Find("pgeSkill"));
			assert(listInfo != nullptr);
			listInfo->evtSelectPage = _evtMobPageIndexChange;

			for (int i = 0; i < DROP_VISIBLE_COUNT; i++) {
				char buf_list[25] = {0};
				sprintf(buf_list, "listMobDrops%d", i);
				listMobDrops[i] = dynamic_cast<COneCommand*>(frmMonsterInfo->Find(buf_list));
				if (!listMobDrops[i])
					return false;

				char buf_name[25] = {0};
				sprintf(buf_name, "LabMobItems%d", i);
				LabMobItems[i] = dynamic_cast<CLabelEx*>(frmMonsterInfo->Find(buf_name));
				if (!LabMobItems[i])
					return false;

				char buf_rate[25] = {0};
				sprintf(buf_rate, "LabMobRates%d", i);
				LabMobRates[i] = dynamic_cast<CLabelEx*>(frmMonsterInfo->Find(buf_rate));
				if (!LabMobRates[i])
					return false;

				char buf_filter[25] = {0};
				sprintf(buf_filter, "checkDropFilter%d", i);
				checkDropFilter[i] = static_cast<CCheckBox*>(frmMonsterInfo->Find(buf_filter));
				if (checkDropFilter[i]) {
					checkDropFilter[i]->evtCheckChange = _evtCheckLootFilter;
				}
				if (!checkDropFilter[i])
					return false;
			}

			LabMobLevel = dynamic_cast<CLabelEx*>(frmMonsterInfo->Find("LabMobLevel"));
			LabMobexp = dynamic_cast<CLabelEx*>(frmMonsterInfo->Find("LabMobexp"));
			LabMobHP = dynamic_cast<CLabelEx*>(frmMonsterInfo->Find("LabMobHP"));
			LabMobAttack = dynamic_cast<CLabelEx*>(frmMonsterInfo->Find("LabMobAttack"));
			LabMobHitRate = dynamic_cast<CLabelEx*>(frmMonsterInfo->Find("LabMobHitRate"));
			LabMobDodge = dynamic_cast<CLabelEx*>(frmMonsterInfo->Find("LabMobDodge"));
			LabMobDef = dynamic_cast<CLabelEx*>(frmMonsterInfo->Find("LabMobDef"));
			LabMobPR = dynamic_cast<CLabelEx*>(frmMonsterInfo->Find("LabMobPR"));
			LabMobAtSpeed = dynamic_cast<CLabelEx*>(frmMonsterInfo->Find("LabMobAtSpeed"));
			LabMobMSpeed = dynamic_cast<CLabelEx*>(frmMonsterInfo->Find("LabMobMSpeed"));

			// Initialize drop list scroll support
			pMobDropScroll = dynamic_cast<CScroll*>(frmMonsterInfo->Find("scrMobDrop"));
			if (pMobDropScroll) {
				pMobDropScroll->evtChange = _evtDropScrollChange;
				pMobDropScroll->SetIsShow(false);
			}

			// Register mouse wheel scroll handler for the drop list
			CFormMgr::s_Mgr.AddMouseScrollEvent(_onDropListMouseScroll);

			// Initialize scroll tracking variables
			m_nDropScrollPos = 0;
			m_nDropTotalCount = 0;
		}
	}

	{
		frmDetail = _FindForm("frmDetail");
		if (frmDetail) {
			frmDetail->Refresh();
			proMainHP = dynamic_cast<CProgressBar*>(frmDetail->Find("proMainHP1"));
			if (!proMainHP)
				return Error(RES_STRING(CL_LANGUAGE_MATCH_473), frmDetail->GetName(), "proMainHP1");
			proMainHP->SetPosition(0.0f);

			proMainSP = dynamic_cast<CProgressBar*>(frmDetail->Find("proMainSP"));
			if (!proMainSP)
				return Error(RES_STRING(CL_LANGUAGE_MATCH_473), frmDetail->GetName(), "proMainSP");
			proMainSP->SetPosition(0.0f);

			proMainExp = dynamic_cast<CProgressBar*>(frmDetail->Find("proMainEXP"));
			if (!proMainExp) {
				Error(RES_STRING(CL_LANGUAGE_MATCH_473), frmDetail->GetName(), "proMainEXP");
			} else {
				proMainExp->SetPosition(0.0f);
			}

			labMainName = dynamic_cast<CLabel*>(frmDetail->Find("labMainID"));
			if (!labMainName)
				Error(RES_STRING(CL_LANGUAGE_MATCH_473), frmDetail->GetName(), "labMainID");

			labMainLevel = dynamic_cast<CLabel*>(frmDetail->Find("labMainLv"));
			if (!labMainLevel)
				Error(RES_STRING(CL_LANGUAGE_MATCH_473), frmDetail->GetName(), "labMainLv");

			imgLeader = dynamic_cast<CImage*>(frmDetail->Find("imgLeader"));
			if (!imgLeader)
				Error(RES_STRING(CL_LANGUAGE_MATCH_473), frmDetail->GetName(), "imgLeader");

			C3DCompent* d3dSelfDown = dynamic_cast<C3DCompent*>(frmDetail->Find("d3dSelfDown"));
			if (!d3dSelfDown)
				return Error(RES_STRING(CL_LANGUAGE_MATCH_473), frmDetail->GetName(), "d3dSelfDown");
			// d3dSelfDown->SetRenderEvent( _MainChaRenderEvent );
			d3dSelfDown->evtMouseDown = _evtSelfMouseDown;
			d3dSelfDown->SetMouseAction(enumMA_Skill);

			C3DCompent* p3D = dynamic_cast<C3DCompent*>(frmDetail->Find("d3dSelf"));
			if (!p3D)
				return Error(RES_STRING(CL_LANGUAGE_MATCH_473), frmDetail->GetName(), "d3dSelf");

			p3D->SetRenderEvent(_MainChaRenderEvent);
			// p3D->evtMouseDown = _evtSelfMouseDown;
			// p3D->SetMouseAction( enumMA_Skill );

			RECT rt;
			rt.left = p3D->GetX();
			rt.right = p3D->GetX2();
			rt.top = p3D->GetY();
			rt.bottom = p3D->GetY2();

			pMainCha = new CCharacter2D;
			pMainCha->Create(rt);
		}

		// ???????????????
		mnuSelf = CMenu::FindMenu("selfMouseRight");
		if (!mnuSelf)
			return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmMain800->GetName(), "selfMouseRight");
		mnuSelf->evtListMouseDown = _OnSelfMenu;
	}

	// frmMain800????
	{
		frmMain800 = _FindForm("frmMain800");
		frmMain800->evtEntrustMouseEvent = _evtTaskMouseEvent;

		tlCity = dynamic_cast<CTitle*>(frmMain800->Find("tlCity"));
		tlField = dynamic_cast<CTitle*>(frmMain800->Find("tlField"));

		// grdHeart = dynamic_cast<CGrid*>(frmMain800->Find("grdHeart"));
		// if( !grdHeart )	return Error( "msgui.clu????<%s>??????????<%s>", frmMain800->GetName(), "grdHeart" );
		// grdHeart->evtSelectChange = _evtChaHeartChange;

		// grdAction = dynamic_cast<CGrid*>(frmMain800->Find("grdAction"));
		// if( !grdAction ) return Error( "msgui.clu????<%s>??????????<%s>", frmMain800->GetName(), "grdAction" );
		// grdAction->evtSelectChange = _evtChaActionChange;
	}

	// ???????
	// proMainHP1 =  dynamic_cast<CProgressBar *> ( frmMain800->Find("proMainHP1") );
	// if( !proMainHP1 ) return Error( "msgui.clu????<%s>??????????<%s>", frmMain800->GetName(), "proMainHP1" );
	// proMainHP1->SetPosition(10.0f );
	//
	// proMainHP2 =  dynamic_cast<CProgressBar *> ( frmMain800->Find("proMainHP2") );
	// if( !proMainHP2 ) return Error( "msgui.clu????<%s>??????????<%s>", frmMain800->GetName(), "proMainHP2" );
	// proMainHP2->SetPosition(10.0f );

	// proMainHP3 =  dynamic_cast<CProgressBar *> ( frmMain800->Find("proMainHP3") );
	// if( !proMainHP3 ) return Error( "msgui.clu????<%s>??????????<%s>", frmMain800->GetName(), "proMainHP3" );
	// proMainHP3->SetPosition(10.0f );

	// proMainSP =  dynamic_cast<CProgressBar *> ( frmMain800->Find("proMainSP") );
	// if( !proMainSP ) return Error( "msgui.clu????<%s>??????????<%s>", frmMain800->GetName(), "proMainSP" );
	//  	proMainSP->SetPosition (10.0f );

	//_pShowExp = dynamic_cast<CLabel*>(frmMain800->Find( "labMainEXP" ) );
	//_pShowLevel = dynamic_cast<CLabel*>(frmMain800->Find( "labMainLV" ) );

	// frmMainFun????
	{
		frmMainFun = _FindForm("frmMainFun");
		if (!frmMainFun)
			return false;

		frmMainFun->evtEntrustMouseEvent = _evtStartFormMouseEvent;

		// QQ
		/*FORM_CONTROL_LOADING_CHECK(btnQQ,frmMainFun,CTextButton,"msgui.clu","btnQQ");
		btnQQ->GetImage()->LoadImage("texture/ui/main800.tga",32,32,4,136,201);*/

		// ???????????????
		btnLevelUpHelp = dynamic_cast<CTextButton*>(frmMainFun->Find("btnLevelUpHelp"));
		// FORM_CONTROL_LOADING_CHECK(btnLevelUpHelp, frmMainFun, CTextButton, "msgui.clu", "btnLevelUpHelp");
		if (btnLevelUpHelp)
			btnLevelUpHelp->SetFlashCycle();
	}

	// ??????
	mainMouseRight = CMenu::FindMenu("mainMouseRight");
	if (!mainMouseRight) {
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmMain800->GetName(), "mainMouseRight");
	}
	mainMouseRight->evtListMouseDown = _evtPopMenu;

	// ??????????
	frmMainChaRelive = _FindForm("frmRelive");
	if (!frmMainChaRelive)
		return false;
	frmMainChaRelive->evtEntrustMouseEvent = _evtReliveFormMouseEvent;

	// ?????????L???
	frmShipSail = _FindForm("frmShipsail"); // ???????
	if (!frmShipSail)
		return false;

	labCanonShow = (CLabelEx*)frmShipSail->Find("labCanonShow1");
	labSailorShow = (CLabelEx*)frmShipSail->Find("labSailorShow1");
	labLevelShow = (CLabelEx*)frmShipSail->Find("labLvship");
	labExpShow = (CLabelEx*)frmShipSail->Find("labExpship");

	proSailor = (CProgressBar*)frmShipSail->Find("proSailor"); // ?;�?????
	proCanon = (CProgressBar*)frmShipSail->Find("proCanon");   // ??????????
	frmShipSail->SetIsShow(false);

	//	Modify by alfred.shi 20080828
	CTextButton* btn1 = (CTextButton*)frmShipSail->Find("btnShip");
	if (!btn1)
		return false;
	btn1->evtMouseClick = _evtShowBoatAttr;

	// ???????
	frmFollow = _FindForm("frmFollow");
	if (!frmFollow)
		return false;

	labFollow = dynamic_cast<CLabel*>(frmFollow->Find("labFollow"));
	if (!labFollow)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmFollow->GetName(), "labFollow");

	// ???????
	frmMainPet = _FindForm("frmMainPet");
	if (!frmMainPet)
		return false;

	frmMainPet->Hide();

	labPetLv = dynamic_cast<CLabel*>(frmMainPet->Find("labPetLv"));
	if (!labPetLv)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmMainPet->GetName(), "labPetLv");

	imgPetHead = dynamic_cast<CImage*>(frmMainPet->Find("imgPetHead"));
	if (!imgPetHead)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmMainPet->GetName(), "imgPetHead");

	proPetHP = dynamic_cast<CProgressBar*>(frmMainPet->Find("proPetHP"));
	if (!proPetHP)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmMainPet->GetName(), "proPetHP");

	proPetSP = dynamic_cast<CProgressBar*>(frmMainPet->Find("proPetSP"));
	if (!proPetSP)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmMainPet->GetName(), "proPetSP");

	// Mount UI initialization
	frmMainMount = _FindForm("frmMainMount");
	if (frmMainMount) {
		frmMainMount->Hide();

		labMountLv = dynamic_cast<CLabel*>(frmMainMount->Find("labMountLv"));
		imgMountHead = dynamic_cast<CImage*>(frmMainMount->Find("imgMountHead"));
		proMountHP = dynamic_cast<CProgressBar*>(frmMainMount->Find("proMountHP"));
		proMountSP = dynamic_cast<CProgressBar*>(frmMainMount->Find("proMountSP"));
	} else {
		// Mount UI form not found - optional feature
		labMountLv = nullptr;
		imgMountHead = nullptr;
		proMountHP = nullptr;
		proMountSP = nullptr;
	}

	//
	// Help system
	//
	frmHelpSystem = CFormMgr::s_Mgr.Find("frmHelpSystem");
	if (!frmHelpSystem)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), "frmHelpSystem", "frmHelpSystem");

	lstHelpList = dynamic_cast<CList*>(frmHelpSystem->Find("lstHelpList"));
	if (!lstHelpList)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmHelpSystem->GetName(), "lstHelpList");
	lstHelpList->evtSelectChange = _evtHelpListChange;

	frmHelpSystem->evtEntrustMouseEvent = _evtStartFormMouseEvent;

	char szName[64] = {0};
	for (int i = 0; i < HELP_PICTURE_COUNT; ++i) {
		sprintf(szName, "imgHelpShow%d_1", i + 1);
		imgHelpShow1[i] = dynamic_cast<CImage*>(frmHelpSystem->Find(szName));
		if (!imgHelpShow1[i])
			return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmHelpSystem->GetName(), szName);

		sprintf(szName, "imgHelpShow%d_2", i + 1);
		imgHelpShow2[i] = dynamic_cast<CImage*>(frmHelpSystem->Find(szName));
		if (!imgHelpShow2[i])
			return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmHelpSystem->GetName(), szName);

		sprintf(szName, "imgHelpShow%d_3", i + 1);
		imgHelpShow3[i] = dynamic_cast<CImage*>(frmHelpSystem->Find(szName));
		if (!imgHelpShow3[i])
			return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmHelpSystem->GetName(), szName);

		sprintf(szName, "imgHelpShow%d_4", i + 1);
		imgHelpShow4[i] = dynamic_cast<CImage*>(frmHelpSystem->Find(szName));
		if (!imgHelpShow4[i])
			return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmHelpSystem->GetName(), szName);

		if (i > 0) {
			imgHelpShow1[i]->SetIsShow(false);
			imgHelpShow2[i]->SetIsShow(false);
			imgHelpShow3[i]->SetIsShow(false);
			imgHelpShow4[i]->SetIsShow(false);
		} else {
			imgHelpShow1[i]->SetIsShow(true);
			imgHelpShow2[i]->SetIsShow(true);
			imgHelpShow3[i]->SetIsShow(true);
			imgHelpShow4[i]->SetIsShow(true);
		}
	}

	//
	// ??????t????
	//
	frmBag = CFormMgr::s_Mgr.Find("frmBag");
	if (!frmBag)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), "frmBag", "frmBag");
	frmBag->evtEntrustMouseEvent = _evtStartFormMouseEvent;

	//
	// ????t???
	//
	frmSociliaty = CFormMgr::s_Mgr.Find("frmSociliaty");
	if (!frmSociliaty)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), "frmSociliaty", "frmSociliaty");
	frmSociliaty->evtEntrustMouseEvent = _evtStartFormMouseEvent;

	return true;
}

void CStartMgr::ShowShipSailForm(bool isShow /* = true  */) {
	UpdateShipSailForm();
	frmShipSail->SetIsShow(isShow);
}
void CStartMgr::UpdateShipSailForm() {
	CCharacter* pMain = CGameScene::GetMainCha();
	if (!pMain || !pMain->IsBoat())
		return;

	SGameAttr* pAttr = pMain->getGameAttr();

	char buf[128];

	sprintf(buf, "%d/%d", pAttr->get(ATTR_HP), pAttr->get(ATTR_MXHP));
	labSailorShow->SetCaption(buf);
	proSailor->SetRange((float)0, (float)(pAttr->get(ATTR_MXHP)));
	proSailor->SetPosition((float)(pAttr->get(ATTR_HP)));

	sprintf(buf, "%d/%d", pAttr->get(ATTR_SP), pAttr->get(ATTR_MXSP));
	labCanonShow->SetCaption(buf);
	proCanon->SetRange((float)0, (float)(pAttr->get(ATTR_MXSP)));
	proCanon->SetPosition((float)(pAttr->get(ATTR_SP)));

	labLevelShow->SetCaption(itoa(pAttr->get(ATTR_LV), buf, 10));
	labExpShow->SetCaption(itoa(pAttr->get(ATTR_CEXP), buf, 10));
}

void CStartMgr::End() {
	// pMainCha and pTarget are UI character models we own - safe to delete
	SAFE_DELETE(pMainCha);
	SAFE_DELETE(pTarget);
	
	// Clear target ID - no pointers to clean up anymore
	targetInfoID = 0;
	
	// frmMonsterInfo is managed by the form manager, don't delete it
	// Just clear our reference
	frmMonsterInfo = nullptr;
}

void CStartMgr::ShowQueryReliveForm(int nType, const char* str) {
	stSelectBox* pOriginRelive = g_stUIBox.ShowSelectBox(_evtOriginReliveFormMouseEvent, str, false);
	frmSelectOriginRelive = pOriginRelive->frmDialog;
	frmSelectOriginRelive->nTag = nType;
}

void CStartMgr::_evtOriginReliveFormMouseEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	frmSelectOriginRelive = nullptr;
	if (pSender->GetForm()->nTag == enumEPLAYER_RELIVE_ORIGIN) {
		if (nMsgType == CForm::mrYes) {
			CS_DieReturn(enumEPLAYER_RELIVE_ORIGIN);
			g_stUIStart.frmMainChaRelive->SetIsShow(false);
		} else {
			CS_DieReturn(enumEPLAYER_RELIVE_NORIGIN);
		}
	} else {
		if (nMsgType == CForm::mrYes) {
			CS_DieReturn(enumEPLAYER_RELIVE_MAP);
			g_stUIStart.frmMainChaRelive->SetIsShow(false);
		} else {
			CS_DieReturn(enumEPLAYER_RELIVE_NOMAP);
		}
	}
}

void CStartMgr::_evtReliveFormMouseEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	// if( name=="btnReCity" )
	{
		CS_DieReturn(enumEPLAYER_RELIVE_CITY);
		pSender->GetForm()->SetIsShow(false);
		if (frmSelectOriginRelive) {
			frmSelectOriginRelive->SetIsShow(false);
			frmSelectOriginRelive = nullptr;
		}
	}
}

void CStartMgr::_evtStartFormMouseEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	string name = pSender->GetName();

	if (name == "btnState") // ??????????e
	{
		CForm* f = CFormMgr::s_Mgr.Find("frmState");
		if (f) {
			f->SetIsShow(!f->GetIsShow());
		}
		return;
	}
	// else if( name=="btnItem" )	// ??????????
	//{
	//	CForm* f = CFormMgr::s_Mgr.Find( "frmItem" );
	//	if( f )
	//	{
	//		f->SetIsShow( !f->GetIsShow() );
	//	}
	//	return;
	// }
	else if (name == "btnSkill") // ??????????
	{
		CForm* f = CFormMgr::s_Mgr.Find("frmSkill");
		if (f) {
			f->SetIsShow(!f->GetIsShow());
		}
		return;
	} else if (name == "btnMission") // ??????????
	{
		CForm* f = CFormMgr::s_Mgr.Find("frmMission");
		if (f) {
			f->SetIsShow(!f->GetIsShow());
		}
		return;
	}
	// else if( name=="btnGuild" )	    // ??????????
	//{
	//	CForm* f = CFormMgr::s_Mgr.Find( "frmManage" );
	//	if( f )
	//	{
	//		f->SetIsShow( !f->GetIsShow() );
	//	}
	//	return;
	// }
	else if (name == "btnHelp") {
		CForm* f = CFormMgr::s_Mgr.Find("frmHelpSystem");
		if (f) {
			f->evtEntrustMouseEvent = _HelpFrmMainMouseEvent;
			f->SetIsShow(!f->GetIsShow());
		}
		return;
	}

	// ???????????		Add by alfred.shi 20080822	beign
	else if (name == "btnShip1") {
		CForm* f = CFormMgr::s_Mgr.Find("frmStartHelp");
		if (f) {
			f->evtEntrustMouseEvent = _HelpFrmMainMouseEvent;
			f->SetIsShow(!f->GetIsShow());
		}

		return;
	}
	//	End

	// if( name=="btnOpenHelp" )
	//{
	//	CForm * frm = CFormMgr::s_Mgr.Find("frmVHelp");
	//	if( frm )
	//	{
	//		frm->evtEntrustMouseEvent = _NewFrmMainMouseEvent;
	//		frm->nTag = 0;
	//		frm->ShowModal();
	//	}
	//	return;
	// }

	//	Add by alfred.shi 20080822	begin
	if (name == "btnShip2") {
		CForm* frm = CFormMgr::s_Mgr.Find("frmVHelp");
		if (frm) {
			frm->evtEntrustMouseEvent = _NewFrmMainMouseEvent;
			frm->nTag = 0;
			frm->ShowModal();
		}
		return;
	}
	//	End

	else if (name == "btnSystem") // ??????????
	{
		CForm* f = CFormMgr::s_Mgr.Find("frmSystem");
		if (f)
			f->SetIsShow(!f->GetIsShow());
		return;
	} else if (name == "btnQQ") {
		CForm* f = CFormMgr::s_Mgr.Find("frmQQ");
		if (f) {
			f->SetIsShow(!f->GetIsShow());
		}
		return;
	} else if (name == "btnLevelUpHelp") // ?????????????
	{
		SGameAttr* pAttr = CGameScene::GetMainCha()->getGameAttr();

		LONG64 nLevel = pAttr->get(ATTR_LV);
		g_stUIStart.ShowHelpSystem(true, nLevel + HELP_LV1_BEGIN - 1);

		g_stUIStart.ShowLevelUpHelpButton(false);
	} else if (name == "btnInfoCenter") // ?????????h????L???
	{
		bool bShow = g_stUIStart.frmHelpSystem->GetIsShow();
		g_stUIStart.ShowHelpSystem(!bShow);
	} else if (name == "btnOpenBag") // ????????t????
	{
		g_stUIEquip.GetItemForm()->SetIsShow(!g_stUIEquip.GetItemForm()->GetIsShow());
		// g_stUIStart.ShowBagButtonForm(! g_stUIStart.frmBag->GetIsShow());
		// g_stUIStart.ShowSociliatyButtonForm(false);

		// g_stUIStart.frmBag->SetIsShow(! g_stUIStart.frmBag->GetIsShow());
		// g_stUIStart.frmSociliaty->SetIsShow(false);
	} else if (name == "btnGuild") // ??????t????
	{
		// g_stUIStart.ShowSociliatyButtonForm(! g_stUIStart.frmSociliaty->GetIsShow());
		// g_stUIStart.ShowBagButtonForm(false);

		// g_stUIStart.frmSociliaty->SetIsShow(! g_stUIStart.frmSociliaty->GetIsShow());
		// g_stUIStart.frmBag->SetIsShow(false);

		CForm* f = CFormMgr::s_Mgr.Find("frmManage");
		if (f) {
			bool a = f->GetIsShow();
			f->SetIsShow(!a);
			//	Add by alfred.shi 20080905	begin
			CCharacter* pMainCha = CGameScene::GetMainCha();
			if (pMainCha->getGuildID() <= 0) {
				g_pGameApp->MsgBox("You are not in a guild.");
			}
			//	End.
		}
		return;

	}
	// else if( name == "btnOpenItem")	// ??????????
	//{
	//	CForm* f = CFormMgr::s_Mgr.Find( "frmItem" );
	//	if( f )
	//	{
	//		f->SetIsShow( !f->GetIsShow() );
	//	}
	//	return;
	// }
	else if (name == "btnOpenTempBag") // ?????????
	{
		// g_stUIStore.ShowTempKitbag();
	} else if (name == "btnOpenStore") // ?????
	{
		// ???d????,?????
		// g_stUIStore.ShowStoreWebPage();

		// ???????????????????????????????
		g_stUIDoublePwd.SetType(CDoublePwdMgr::STORE_OPEN_ASK);
		g_stUIDoublePwd.ShowDoublePwdForm();
	} else if (name == "btnOpenGuild") // ???????
	{

		CForm* f = CFormMgr::s_Mgr.Find("frmManage");
		if (f) {
			bool a = f->GetIsShow();
			f->SetIsShow(!a);
			//	Add by alfred.shi 20080905	begin
			CCharacter* pMainCha = CGameScene::GetMainCha();
			if (pMainCha->getGuildID() <= 0) {
				g_pGameApp->MsgBox("You are not in a guild.");
			}
			//	End.
		}
		return;
	} else if (name == "btnOpenTeam") // ????
	{
		CCharacter* pMainCha = CGameScene::GetMainCha();

		// ??? 8 ?????�?????	Modify by alfred.shi 20080902	begin
		if (g_stUIFindTeam.IsShowFom())
			g_stUIFindTeam.ShowFindTeamForm(false);
		else if (pMainCha && !pMainCha->IsBoat() && pMainCha->getGameAttr()->get(ATTR_LV) >= 8) {
			CS_VolunteerOpen((short)CFindTeamMgr::FINDTEAM_PAGE_SIZE);
		} //	End
		else {
			if (pMainCha->IsBoat()) {
				g_pGameApp->SysInfo(RES_STRING(CL_LANGUAGE_MATCH_888));
			} else {
				g_pGameApp->SysInfo(RES_STRING(CL_LANGUAGE_MATCH_866));
				g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_866)); //	Add by alfred.shi 20080905
			}
		}
	}

	return;
}

void CStartMgr::_evtSelfMouseDown(CGuiData* pSender, int x, int y, DWORD key) {
	CWorldScene* pScene = dynamic_cast<CWorldScene*>(CGameApp::GetCurScene());
	if (!pScene)
		return;

	CCharacter* pMain = CGameScene::GetMainCha();
	if (!pMain)
		return;

	if (key & Mouse_LDown) {
		CSkillRecord* pSkill = pMain->GetReadySkillInfo();
		if (pSkill && g_SkillUse.IsUse(pSkill, pMain, pMain)) {
			pScene->GetMouseDown().ActAttackCha(pMain, pSkill, pMain);
		}
	} else if ((key & Mouse_RDown) && (pMain->GetTeamLeaderID() != 0)) {
		pSender->GetForm()->PopMenu(g_stUIStart.mnuSelf, x, y);
	}
}

void CStartMgr::MainChaDied() {
	if (frmMainChaRelive) {
		int nLeft = (g_pGameApp->GetWindowHeight() - frmMainChaRelive->GetWidth()) / 2;
		int nTop = (g_pGameApp->GetWindowHeight() - frmMainChaRelive->GetHeight()) / 2;
		nTop -= 80;
		frmMainChaRelive->SetPos(nLeft, nTop);
		frmMainChaRelive->Refresh();

		static CLabel* pInfo = dynamic_cast<CLabel*>(frmMainChaRelive->Find("labReCity"));
		CCharacter* pCha = CGameScene::GetMainCha();
		bool IsShow = true;
		if (pInfo && pCha) {
			if (pCha->IsBoat()) {
				pInfo->SetCaption(RES_STRING(CL_LANGUAGE_MATCH_761));
			} else {
				pInfo->SetCaption(RES_STRING(CL_LANGUAGE_MATCH_762));

				if (CGameScene* pScene = CGameApp::GetCurScene()) {
					if (CMapInfo* pInfo = pScene->GetCurMapInfo()) {
						// Modify by lark.li 20080719 begin
						// if( stricmp( pInfo->szDataName, "teampk" )==0 )
						if (stricmp(pInfo->szDataName, "teampk") == 0 || stricmp(pInfo->szDataName, "starena1") == 0 || stricmp(pInfo->szDataName, "starena2") == 0 || stricmp(pInfo->szDataName, "starena3") == 0)
							IsShow = false;
						// End
					}
				}
			}
		}

		// add by Philip.Wu  ????????????h???z????????????
		CUIInterface::MainChaMove();

		// add by Philip.Wu  2006-07-05  ?????????????????
		// BUG????TEST-32  ?????????????????????bug
		g_stUITrade.CloseAllForm();
		// add by Philip.Wu  2006-07-12  ??????????????????
		CWorldScene* pWorldScene = dynamic_cast<CWorldScene*>(g_pGameApp->GetCurScene());
		if (pWorldScene && pWorldScene->GetShipMgr()) {
			pWorldScene->GetShipMgr()->CloseForm();
		}

		if (IsShow)
			frmMainChaRelive->Show();

		if (frmMonsterInfo->GetIsShow())
			RemoveTarget();
	}
}

void CStartMgr::CheckMouseDown(int x, int y) {
	// if( frmMainFun->GetIsShow() )
	//{
	//	if( !frmMainFun->InRect(x,y) )
	//	{
	//		//frmMainFun->SetIsShow(false);
	//	}
	// }

	// if ( grdAction->GetIsShow() )
	//{
	//	if ( !grdAction->InRect(x,y) )
	//		grdAction->SetIsShow(false);
	// }

	// if ( grdHeart->GetIsShow() )
	//{
	//	if ( !grdHeart->InRect(x,y) )
	//		grdHeart->SetIsShow(false);
	// }

	// if ( g_stUICoze.GetFaceGrid()->GetIsShow() )
	//{
	//	if ( !g_stUICoze.GetFaceGrid()->InRect(x,y) )
	//		g_stUICoze.GetFaceGrid()->SetIsShow(false);
	// }
}

void CStartMgr::_evtTaskMouseEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	string name = pSender->GetName();

	if (name == "btnStart") {
		CForm* f = CFormMgr::s_Mgr.Find("frmMainFun");
		if (f) {
			f->SetIsShow(!f->GetIsShow());
		}
		return;
	}
	// else if( name=="btnAction" )
	//{
	//	g_stUIStart.grdHeart->SetIsShow( false );
	//	g_stUICoze.GetFaceGrid()->SetIsShow( false );
	//	g_stUIStart.grdAction->SetIsShow(!g_stUIStart.grdAction->GetIsShow() );
	//	return;
	// }
	// else if( name=="btnBrow" )
	//{
	//	g_stUIStart.grdAction->SetIsShow( false );
	//	g_stUICoze.GetFaceGrid()->SetIsShow( false );
	//	g_stUIStart.grdHeart->SetIsShow( !g_stUIStart.grdHeart->GetIsShow() );
	//	return;
	// }
	// else if( name=="btnChatFace" )
	//{
	//	g_stUIStart.grdAction->SetIsShow( false );
	//	g_stUIStart.grdHeart->SetIsShow( false );
	//	g_stUICoze.GetFaceGrid()->SetIsShow( !g_stUICoze.GetFaceGrid()->GetIsShow() );
	//	return;
	// }

	// ?????j??
	if (pSender->nTag > 10) {
		CCharacter* c = CGameScene::GetMainCha();
		if (!c)
			return;

		c->ChangeReadySkill(pSender->nTag);
	}
	return;
}

void CStartMgr::_evtChaActionChange(CGuiData* pSender) {
	CCharacter* pCha = g_pGameApp->GetCurScene()->GetMainCha();
	if (!pCha)
		return;

	pSender->SetIsShow(false);

	CGrid* p = dynamic_cast<CGrid*>(pSender);
	if (!p)
		return;
	CGraph* r = p->GetSelect();
	int nIndex = p->GetSelectIndex();
	if (r) {
		pCha->GetActor()->PlayPose(r->nTag, true, true);
	}
}

void CStartMgr::_evtChaHeartChange(CGuiData* pSender) {
	CCharacter* pCha = CGameScene::GetMainCha();
	if (!pCha)
		return;
	pSender->SetIsShow(false);

	CGrid* p = dynamic_cast<CGrid*>(pSender);
	if (!p)
		return;
	CGraph* r = p->GetSelect();
	int nIndex = p->GetSelectIndex();
	if (r) {
		pCha->GetHeadSay()->SetFaceID(nIndex);
		char szFaceID[10] = {0};
		sprintf(szFaceID, "***%d", nIndex);
		CS_Say(szFaceID);
	}
}

void GUI::CStartMgr::_evtMobPageIndexChange(CGuiData* pSender) {
	int index = g_stUIStart.listInfo->GetIndex();
	if (index == 0) {
		g_stUIStart.lstList = g_stUIStart.lstMobDrop;
	} else if (index == 1) {
		g_stUIStart.lstList = g_stUIStart.lstMobInfo;
	}
}

void CStartMgr::RefreshMainLifeNum(long num, long max) {
	////HP?i?
	// char szHP[32] = { 0 };
	// if ( num < 0 )	num = 0;
	// sprintf( szHP,"%d/%d", num, max);
	// szHP[sizeof(szHP)-1] = '\0';

	// float f = (float) num /(float) max;
	// CProgressBar* pBar = nullptr;
	// if( f < 0.334 )
	//{
	//	pBar = proMainHP3;
	//	proMainHP2->SetIsShow(false);
	//	proMainHP1->SetIsShow(false);
	// }
	// else if( f > 0.666 )
	//{
	//	pBar = proMainHP1;
	//	proMainHP2->SetIsShow(false);
	//	proMainHP3->SetIsShow(false);
	// }
	// else
	//{
	//	pBar = proMainHP2;
	//	proMainHP1->SetIsShow(false);
	//	proMainHP3->SetIsShow(false);
	// }

	// pBar->SetIsShow(true);
	// pBar->SetPosition( 10.0f * f ) ;

	if (proMainHP) {
		proMainHP->SetRange(0.0f, (float)max);
		proMainHP->SetPosition((float)num);
	}
}

void CStartMgr::RefreshMainExperience(LONG64 num, LONG64 curlev, LONG64 nextlev) {
	LG("exp", RES_STRING(CL_LANGUAGE_MATCH_763), num, curlev, nextlev, 100.0f * (float)(num - curlev) / (float)(nextlev - curlev));

	//// EXP?i?
	// long max = nextlev - curlev;
	// num = num - curlev;
	// if ( num < 0 ) num = 0;

	// if (max!=0)
	//	sprintf( szBuf , "%4.2f%%" , num*100.0f/max );
	// else
	//	sprintf( szBuf , "0.00%");
	// szBuf[sizeof(szBuf)-1] = '\0';

	//_pShowExp->SetCaption( szBuf );
	if (proMainExp) {

		proMainExp->SetRange(0, nextlev - curlev);
		proMainExp->SetPosition(num - curlev);
	}
	/*
		if ( proMainExp )
		{

			proMainExp->SetRange(0, nextlev - curlev);
			proMainExp->SetPosition((float)(num - curlev) / (float)(nextlev - curlev));
		}
		*/
}

// void CStartMgr::RefreshLifeExperience(long num, long curlev, long nextlev)
//{
//		if(proLifeExp){
//			proLifeExp->SetRange(0, nextlev-curlev);
//			proLifeExp->SetPosition(num-curlev);
//
//		}
//
// }

void CStartMgr::RefreshMainName(const char* szName) {
	if (labMainName) {
		labMainName->SetCaption(szName);
	}
}

void CStartMgr::RefreshLv(long l) {
	if (labMainLevel) {
		sprintf(szBuf, "%d", l);
		labMainLevel->SetCaption(szBuf);
	}
}

void CStartMgr::RefreshMainSP(long num, long max) {
	// SP?i?
	if (proMainSP) {
		proMainSP->SetRange(0.0f, (float)max);
		proMainSP->SetPosition((float)num);
	}
}

// ?????????????
void CStartMgr::_evtPopMenu(CGuiData* pSender, int x, int y, DWORD key) {
	mainMouseRight->SetIsShow(false);
	CMenuItem* pItem = mainMouseRight->GetSelectMenu();
	if (!pItem)
		return;
	string str = pItem->GetString();
	if (str == RES_STRING(CL_LANGUAGE_MATCH_764)) // ??????
	{
		CCharacter* pCha = (CCharacter*)mainMouseRight->GetPointer();
		CCharacter* pMain = CGameScene::GetMainCha();
		if (pCha && pMain && pCha != pMain && pCha->IsEnabled() && pMain->IsEnabled() && ((!pCha->IsBoat() && !pMain->IsBoat()) || (pCha->IsBoat() && pMain->IsBoat()))) {
			if (pMain->IsBoat() || pMain->getGameAttr()->get(ATTR_LV) >= 6) {
				CS_RequestTrade(mission::TRADE_CHAR, mainMouseRight->nTag);
			} else {
				// ??????6?????�????????????
				g_pGameApp->SysInfo(RES_STRING(CL_LANGUAGE_MATCH_864));
			}
		} else {
			g_pGameApp->SysInfo(RES_STRING(CL_LANGUAGE_MATCH_765)); // ?????????????????????????
		}
	} else if (str == RES_STRING(CL_LANGUAGE_MATCH_482)) // ???????
	{
		CCharacter* pCha = (CCharacter*)mainMouseRight->GetPointer();
		CCharacter* pMain = CGameScene::GetMainCha();
		if (!pCha || !pMain)
			return;

		if (pMain->IsBoat() || pMain->getGameAttr()->get(ATTR_LV) >= 7) {
			CS_Frnd_Invite(pCha->getHumanName());
		} else {
			// ??????7?????�?????????????
			g_pGameApp->SysInfo(RES_STRING(CL_LANGUAGE_MATCH_865));
		}
	} else if (str == RES_STRING(CL_LANGUAGE_MATCH_484)) // ???????
	{
		CCharacter* pCha = (CCharacter*)mainMouseRight->GetPointer();
		CCharacter* pMain = CGameScene::GetMainCha();
		if (!pCha || !pMain)
			return;

		if ((pMain->IsBoat() || pMain->getGameAttr()->get(ATTR_LV) >= 8) &&
			(pCha->IsBoat() || pCha->getGameAttr()->get(ATTR_LV) >= 8)) {
			CS_Team_Invite(pCha->getHumanName());
		} else {
			// ??????8?????�?????????????
			g_pGameApp->SysInfo(RES_STRING(CL_LANGUAGE_MATCH_866));
		}

		return;
	} else if (str == RES_STRING(CL_LANGUAGE_MATCH_483)) // ??????
	{
		CS_Team_Leave();
		return;
	} else if (str == RES_STRING(CL_LANGUAGE_MATCH_481)) // ??????
	{
		CCharacter* pCha = (CCharacter*)mainMouseRight->GetPointer();
		if (pCha) {
			CCozeForm::GetInstance()->OnPrivateNameSet(pCha->getHumanName());
		}
		return;
	} else if (str == RES_STRING(CL_LANGUAGE_MATCH_766)) // ???????
	{
		CCharacter* pCha = (CCharacter*)mainMouseRight->GetPointer();
		CCharacter* pMain = CGameScene::GetMainCha();
		if (pCha && pMain && pCha->IsBoat() && pMain->IsBoat()) {
			CS_RequestTrade(mission::TRADE_BOAT, mainMouseRight->nTag);
		} else {
			g_pGameApp->SysInfo(RES_STRING(CL_LANGUAGE_MATCH_767));
		}
		return;
	} else if (str == RES_STRING(CL_LANGUAGE_MATCH_768)) // ????
	{
		CCharacter* pCha = (CCharacter*)mainMouseRight->GetPointer();
		if (pCha && !pCha->IsMainCha()) {
			CWorldScene* pScene = dynamic_cast<CWorldScene*>(CGameApp::GetCurScene());
			if (pScene) {
				pScene->GetMouseDown().ActShop(CGameScene::GetMainCha(), pCha);
			}
		}
		return;
	} else if (str == RES_STRING(CL_LANGUAGE_MATCH_769)) // ??????
	{
		CCharacter* pCha = (CCharacter*)mainMouseRight->GetPointer();
		if (pCha)
			CS_TeamFightAsk(pCha->getAttachID(), pCha->lTag, enumFIGHT_TEAM);
		return;
	} else if (str == RES_STRING(CL_LANGUAGE_MATCH_770)) // ??????
	{
		CCharacter* pCha = (CCharacter*)mainMouseRight->GetPointer();
		if (pCha)
			CS_TeamFightAsk(pCha->getAttachID(), pCha->lTag, enumFIGHT_MONOMER);
		return;
	} else if (str == RES_STRING(CL_LANGUAGE_MATCH_855)) // ??????
	{
		CCharacter* pCha = (CCharacter*)mainMouseRight->GetPointer();
		CCharacter* pMain = CGameScene::GetMainCha();
		if (pCha && pMain && !pCha->IsBoat() && !pMain->IsBoat()) {
			const char* szName = pCha->getHumanName();
			CS_MasterInvite(pCha->getHumanName(), mainMouseRight->nTag);
		} else {
			// ???????????????????????
			g_pGameApp->SysInfo(RES_STRING(CL_LANGUAGE_MATCH_888));
		}
	} else if (str == RES_STRING(CL_LANGUAGE_MATCH_859)) // ???????
	{
		CCharacter* pCha = (CCharacter*)mainMouseRight->GetPointer();
		CCharacter* pMain = CGameScene::GetMainCha();
		if (pCha && pMain && !pCha->IsBoat() && !pMain->IsBoat()) {
			const char* szName = pCha->getHumanName();
			CS_PrenticeInvite(pCha->getHumanName(), mainMouseRight->nTag);
		} else {
			// ???????????????????????
			g_pGameApp->SysInfo(RES_STRING(CL_LANGUAGE_MATCH_888));
		}
	} else if (str == "Check Eq") {
		// Safely view another player's equipment
		CCharacter* pCha = (CCharacter*)mainMouseRight->GetPointer();
		CCharacter* pMain = CGameScene::GetMainCha();
		if (pCha && pMain && pCha->IsValid() && pMain->IsValid() && !pCha->IsHide()) {
			g_stUIEquip.UpdataEquipSpy(pCha->GetPart(), pCha);
		}
	} else if (str == "Follow") {
		CWorldScene* pScene = dynamic_cast<CWorldScene*>(g_pGameApp->GetCurScene());
		CCharacter* pCha = (CCharacter*)mainMouseRight->GetPointer();
		if (pCha) {
			pScene->GetMouseDown().GetAutoAttack()->Follow(g_pGameApp->GetCurScene()->GetMainCha(), pCha);
		}
	} else if (str == "Block") {
		CCharacter* pCha = (CCharacter*)mainMouseRight->GetPointer();
		CTeam* pTeam = g_stUIChat.GetTeamMgr()->Find(enumTeamBlocked);
		if (!pTeam->FindByName(pCha->getHumanName()))
			pTeam->Add(-1, pCha->getHumanName(), "", 9);
	} else if (str == "Unblock") {
		CCharacter* pCha = (CCharacter*)mainMouseRight->GetPointer();
		g_stUIChat.GetTeamMgr()->Find(enumTeamBlocked)->DelByName(pCha->getHumanName());
	} else if (str == "Manage") {
		CCharacter* pCha = (CCharacter*)mainMouseRight->GetPointer();
		CS_RequestTalk(pCha->getAttachID(), 0);
	}

	g_stUIStart.frmMain800->Refresh();
}

void CStartMgr::AskTeamFight(const char* str) {
	g_stUIBox.ShowSelectBox(_evtAskTeamFightMouseEvent, str, false);
}

void CStartMgr::_evtAskTeamFightMouseEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	CS_TeamFightAnswer(nMsgType == CForm::mrYes);
}

// ??????????
void CStartMgr::PopMenu(CCharacter* pCha) {
	// g_pGameApp->GetCurScene()->GetTerrainName();

	if (pCha->IsPlayer() && !g_stUIStart.IsCanTeam())
		return;

	if (g_stUIBank.GetBankGoodsGrid()->GetForm()->GetIsShow()) // ?????j???g??????????????????????
		return;

	if (g_stUIBooth.GetBoothItemsGrid()->GetForm()->GetIsShow()) // ?????j???g??????????????????
		return;

	if (g_stUITrade.IsTrading()) // ?????j???g????????????????????
		return;

	if (mainMouseRight && pCha && pCha->IsValid() && !pCha->IsHide() && (pCha->IsPlayer() || pCha->IsMonster())) // && (pCha->IsPlayer() || pCha->IsMonster())
	{
		mainMouseRight->nTag = pCha->getAttachID();
		mainMouseRight->SetPointer(pCha);

		CCharacter* pMain = CGameScene::GetMainCha();
		if (!pMain)
			return;

		if (!pMain->IsValid() || pMain->IsHide())
			return;

		if (pCha->GetIsPet())
			return; // ?????????????????

		int nMainGuildID = pMain->getGuildID();
		int nChaGuildID = pCha->getGuildID();
		if (nMainGuildID > 0 && nChaGuildID > 0) {
			if (g_stUIMap.IsGuildWar() && ((nMainGuildID <= 100 && nChaGuildID > 100) || (nMainGuildID > 100 && nChaGuildID <= 100)))
				return; // ??????
		}

		mainMouseRight->SetAllEnabled(false);
		const int nCount = mainMouseRight->GetCount();
		CMenuItem* pItem = nullptr;
		const char* MapName = g_pGameApp->GetCurScene()->GetTerrainName(); // ???????j?????? Add by ning.yan 20080715
		// Add by sunny.sun20080820
		// Begin
		if (stricmp(MapName, "starena1") == 0 || stricmp(MapName, "starena2") == 0 || stricmp(MapName, "starena3") == 0)
			return;
		for (int i = 0; i < nCount; i++) {
			pItem = mainMouseRight->GetMenuItem(i);

			if (stricmp(pItem->GetString(), "Manage") == 0) {
				if (!pCha->getIsPlayerCha() && !pCha->IsMonster() && pCha->getShopName()[0] == 0) {
					pItem->SetIsHide(false);
					pItem->SetIsEnabled(true);
				} else {
					pItem->SetIsHide(true);
					pItem->SetIsEnabled(false);
				}
			}

			// if(!pCha->getIsPlayerCha()){
			//	continue;
			// }

			if (stricmp(pItem->GetString(), RES_STRING(CL_LANGUAGE_MATCH_764)) == 0) {
				if (pMain != pCha && pMain->IsEnabled() && pCha->IsEnabled() && ((pMain->IsBoat() && pCha->IsBoat()) || (!pMain->IsBoat() && !pCha->IsBoat())) && !pCha->IsMonster()) {
					pItem->SetIsEnabled(true);
					pItem->SetIsHide(false);
				} else {
					pItem->SetIsHide(pCha->IsMonster());
					pItem->SetIsEnabled(false);
				}
			} else if (stricmp(pItem->GetString(), RES_STRING(CL_LANGUAGE_MATCH_482)) == 0) {
				pItem->SetIsHide(pCha->IsMonster());
				pItem->SetIsEnabled(pMain != pCha);
			} else if (stricmp(pItem->GetString(), RES_STRING(CL_LANGUAGE_MATCH_484)) == 0) { // ????????????????? Add by ning.yan  20080715 Begin
				// if( stricmp( MapName,"starena1") == 0 || stricmp( MapName,"starena2") == 0 || stricmp( MapName,"starena3") == 0 )
				//	pItem->SetIsEnabled( false );
				//// End
				// else
				pItem->SetIsHide(pCha->IsMonster());
				pItem->SetIsEnabled(g_stUIStart.IsCanTeam() && pMain != pCha && (pMain->GetTeamLeaderID() == 0 || (pMain->IsTeamLeader() && pCha->GetTeamLeaderID() != pMain->GetTeamLeaderID())));
			} else if (stricmp(pItem->GetString(), RES_STRING(CL_LANGUAGE_MATCH_483)) == 0) { // ???????????????????  Add by ning.yan  20080715 Begin
				// if( stricmp( MapName,"starena1") == 0 || stricmp( MapName,"starena2") == 0 || stricmp( MapName,"starena3") == 0 )
				//	pItem->SetIsEnabled( false );
				//// End
				// else
				pItem->SetIsHide(pCha->IsMonster());
				pItem->SetIsEnabled(g_stUIStart.IsCanTeam() && pMain->GetTeamLeaderID() != 0 && pCha->GetTeamLeaderID() == pMain->GetTeamLeaderID());
			} else if (stricmp(pItem->GetString(), RES_STRING(CL_LANGUAGE_MATCH_481)) == 0) {
				pItem->SetIsHide(pCha->IsMonster());
				pItem->SetIsEnabled(pMain != pCha);
			} else if (stricmp(pItem->GetString(), RES_STRING(CL_LANGUAGE_MATCH_766)) == 0) {
				pItem->SetIsHide(pCha->IsMonster());
				pItem->SetIsEnabled(pMain != pCha && pMain->IsBoat() && pMain->IsEnabled() && pCha->IsBoat() && pCha->IsEnabled());
			} else if (stricmp(pItem->GetString(), RES_STRING(CL_LANGUAGE_MATCH_768)) == 0) {
				pItem->SetIsHide(pCha->IsMonster());
				pItem->SetIsEnabled(pMain != pCha && pMain->IsEnabled() && !pMain->IsShop() && pCha->IsEnabled() && pCha->IsShop());
			} else if (stricmp(pItem->GetString(), RES_STRING(CL_LANGUAGE_MATCH_769)) == 0) {
				pItem->SetIsHide(pCha->IsMonster());
				pItem->SetIsEnabled(g_stUIStart.IsCanTeam() && pMain != pCha && pMain->IsEnabled() && pMain->IsTeamLeader() && !pMain->IsShop() && pCha->IsEnabled() && pCha->IsTeamLeader() && !pCha->IsShop());
			} else if (stricmp(pItem->GetString(), RES_STRING(CL_LANGUAGE_MATCH_770)) == 0) {
				pItem->SetIsHide(pCha->IsMonster());
				pItem->SetIsEnabled(g_stUIStart.IsCanTeam() && pMain != pCha && pCha->IsPlayer());
			} else if (stricmp(pItem->GetString(), RES_STRING(CL_LANGUAGE_MATCH_855)) == 0) // ??????
			{
				pItem->SetIsHide(pCha->IsMonster());
				pItem->SetIsEnabled(pMain != pCha && pCha->IsPlayer() && pMain->getGameAttr() && pMain->getGameAttr()->get(ATTR_LV) <= 40);
				//&& pCha->getGameAttr()  && pCha->getGameAttr()->get(ATTR_LV) > 40 );
			} else if (stricmp(pItem->GetString(), RES_STRING(CL_LANGUAGE_MATCH_859)) == 0) // ???????
			{
				pItem->SetIsHide(pCha->IsMonster());
				pItem->SetIsEnabled(pMain != pCha && pCha->IsPlayer() && pMain->getGameAttr() && pMain->getGameAttr()->get(ATTR_LV) > 40);
				//&& pCha->getGameAttr()  && pCha->getGameAttr()->get(ATTR_LV) <= 40 );
			} else if (stricmp(pItem->GetString(), "Check Eq") == 0) {
				if (pCha->IsPlayer()) { // pMain->getGMLv() == 99  Enabling for Non-GMs Mdr.st May 2020 - FPO Alpha
					pItem->SetIsEnabled(pMain != pCha);
					pItem->SetIsHide(false);
				} else {
					pItem->SetIsHide(true);
					pItem->SetIsEnabled(false);
				}

			} else if (stricmp(pItem->GetString(), "Follow") == 0) {
				pItem->SetIsHide(pCha->IsMonster());
				pItem->SetIsEnabled(pMain != pCha && pCha->IsPlayer());
			} else if (stricmp(pItem->GetString(), "Block") == 0) {

				if ((!g_stUIChat.GetTeamMgr()->Find(enumTeamBlocked)->FindByName(pCha->getHumanName())) && !pCha->IsMonster()) {

					pItem->SetIsHide(false);
					pItem->SetIsEnabled(pMain != pCha);
				} else {
					pItem->SetIsHide(true);
				}
			} else if (stricmp(pItem->GetString(), "Unblock") == 0) {
				if (g_stUIChat.GetTeamMgr()->Find(enumTeamBlocked)->FindByName(pCha->getHumanName()) && !pCha->IsMonster()) {
					pItem->SetIsHide(false);
					pItem->SetIsEnabled(pMain != pCha);
				} else {
					pItem->SetIsHide(true);
				}
			}
		}

		if (mainMouseRight->IsAllDisabled()) {
			mainMouseRight->SetIsShow(false);
			return;
		}

		int x = 0, y = 0;
		g_Render.WorldToScreen(pCha->GetPos().x, pCha->GetPos().y, pCha->GetPos().z, &x, &y);

		if (CForm::GetActive() && CForm::GetActive()->IsNormal())
			CForm::GetActive()->PopMenu(mainMouseRight, x, y);
		else
			frmMain800->PopMenu(mainMouseRight, x, y);
	}
}

void CStartMgr::CloseForm() {
	// if( frmMainFun->GetIsShow() )
	//	frmMainFun->Close();

	// grdAction->SetIsShow(false);
	// grdHeart->SetIsShow(false);
	// g_stUICoze.GetFaceGrid()->SetIsShow( false );
}

CTextButton* CStartMgr::GetShowQQButton() {
	return btnQQ;
}

void CStartMgr::ShowBigText(const char* str) {
	int nType = 0;
	CCharacter* pMain = CGameScene::GetMainCha();
	if (pMain && CGameApp::GetCurScene() && CGameApp::GetCurScene()->GetTerrain()) {
		int nArea = CGameApp::GetCurScene()->GetTerrain()->GetTile(pMain->GetCurX() / 100, pMain->GetCurY() / 100)->getIsland();
		CAreaInfo* pArea = GetAreaInfo(nArea);
		if (pArea) {
			nType = pArea->chType;
		}
	}

	if (nType) {
		if (tlCity)
			tlCity->SetIsShow(false);

		if (tlField) {
			tlField->SetCaption(str);
			// tlField->SetFont(15);
			tlField->SetIsShow(true);
		}
	} else {
		if (tlField)
			tlField->SetIsShow(false);

		if (tlCity) {

			tlCity->SetCaption(str);
			tlCity->SetIsShow(true);
		}
	}
}

void CStartMgr::FrameMove(DWORD dwTime) {
	static CTimeWork time(100);
	if (time.IsTimeOut(dwTime)) {
		if (frmShipSail->GetIsShow()) {
			UpdateShipSailForm();
		}
	}

	static CTimeWork pet_time(300);

	if (pet_time.IsTimeOut(dwTime)) {
		if (g_stUIBoat.GetHuman()) {
			SItemGrid Data = g_stUIBoat.GetHuman()->GetPart().SLink[enumEQUIP_FAIRY];
			if (Data.IsValid() && CGameScene::GetMainCha()) {
				CItemRecord* pItemInfo = GetItemRecordInfo(Data.sID);
				if (!pItemInfo || pItemInfo->sType != enumItemTypePet) {
					if (frmMainPet->GetIsShow()) {
						frmMainPet->Hide();
					}
				} else {
					proPetHP->SetRange(0, float(Data.sEndure[1] / 50));
					proPetHP->SetPosition(float(Data.sEndure[0] / 50));

					proPetSP->SetRange(0, Data.sEnergy[1]);
					proPetSP->SetPosition(Data.sEnergy[0]);

					// Flash green when growth is full (500ms interval)
					static DWORD dwPetFlashTimer = 0;
					static bool IsPetFlash = false;
					DWORD dwNow = GetTickCount();
					if (dwNow - dwPetFlashTimer > 500) {
						dwPetFlashTimer = dwNow;
						IsPetFlash = !IsPetFlash;
					}
					if (IsPetFlash && Data.sEnergy[1] == Data.sEnergy[0]) {
						proPetSP->GetImage()->SetColor(0xff00ff00);
					} else {
						proPetSP->GetImage()->SetColor(0xFFFFFFFF);
					}
					if (!frmMainPet->GetIsShow()) {
						frmMainPet->Show();
					}
				}
			} else if (frmMainPet->GetIsShow()) {
				frmMainPet->Hide();
			}
		} else if (frmMainPet->GetIsShow()) {
			frmMainPet->Hide();
		}
	}

	// Mount UI update (similar to fairy)
	// If no dedicated frmMainMount exists, we skip mount UI display
	// The mount status can still be viewed in the equipment tooltip
	if (frmMainMount) {
		if (g_stUIBoat.GetHuman()) {
			SItemGrid MountData = g_stUIBoat.GetHuman()->GetPart().SLink[enumEQUIP_MOUNT];
			if (MountData.IsValid() && CGameScene::GetMainCha()) {
				CItemRecord* pMountInfo = GetItemRecordInfo(MountData.sID);
				if (!pMountInfo || pMountInfo->sType != enumItemMount) {
					if (frmMainMount->GetIsShow()) {
						frmMainMount->Hide();
					}
				} else {
					// Update stamina bar (sEndure[0] = current, sEndure[1] = max)
					if (proMountHP) {
						proMountHP->SetRange(0, float(MountData.sEndure[1] / 50));
						proMountHP->SetPosition(float(MountData.sEndure[0] / 50));
					}

					// Update growth/EXP bar (sEnergy[0] = current, sEnergy[1] = max)
					if (proMountSP) {
						proMountSP->SetRange(0, MountData.sEnergy[1]);
						proMountSP->SetPosition(MountData.sEnergy[0]);

						// Flash green when growth is full (500ms interval)
						static DWORD dwMountFlashTimer = 0;
						static bool IsMountFlash = false;
						DWORD dwMountNow = GetTickCount();
						if (dwMountNow - dwMountFlashTimer > 500) {
							dwMountFlashTimer = dwMountNow;
							IsMountFlash = !IsMountFlash;
						}
						if (IsMountFlash && MountData.sEnergy[1] == MountData.sEnergy[0]) {
							proMountSP->GetImage()->SetColor(0xff00ff00);
						} else {
							proMountSP->GetImage()->SetColor(0xFFFFFFFF);
						}
					}

					// Refresh mount icon and level display
					RefreshMount(MountData);

					if (!frmMainMount->GetIsShow()) {
						frmMainMount->Show();
					}
				}
			} else if (frmMainMount->GetIsShow()) {
				frmMainMount->Hide();
			}
		} else if (frmMainMount->GetIsShow()) {
			frmMainMount->Hide();
		}
	}
}

void CStartMgr::_evtShowBoatAttr(CGuiData* pSender, int x, int y, DWORD key) // ??????????
{
	xShipFactory* pkShip = ((CWorldScene*)g_pGameApp->GetCurScene())->GetShipMgr()->_factory;
	if (pkShip && pkShip->sbf.wnd->GetIsShow()) {
		pkShip->sbf.wnd->SetIsShow(false);
	} else {
		CS_GetBoatInfo();
	}
}

void CStartMgr::SwitchMap() {
	frmMainChaRelive->Close();
	CMenu::CloseAll();
	if (!(dynamic_cast<CWorldScene*>(CGameApp::GetCurScene())))
		return;

	if (!_IsNewer)
		return;

	CForm* frm = CFormMgr::s_Mgr.Find("frmVHelp");
	if (frm) {
		frm->evtEntrustMouseEvent = _NewFrmMainMouseEvent;
		frm->nTag = 1;
		frm->ShowModal();
	}
}

//~ ??????? =================================================================
void CStartMgr::_NewFrmMainMouseEvent(CCompent* pSender, int nMsgType,
									  int x, int y, DWORD dwKey) {
	string name = pSender->GetName();
	if (name == "btnNo" || name == "btnClose") {
		if (pSender->GetForm()->nTag == 1)
			CBoxMgr::ShowMsgBox(_CloseEvent, RES_STRING(CL_LANGUAGE_MATCH_771));
		else
			pSender->GetForm()->Close();
	}
}

void CStartMgr::_HelpFrmMainMouseEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	string name = pSender->GetName();
	if (name == "btnOpenHelp") {
		CForm* frm = CFormMgr::s_Mgr.Find("frmVHelp");
		if (frm) {
			frm->evtEntrustMouseEvent = _NewFrmMainMouseEvent;
			frm->nTag = 0;
			frm->ShowModal();
		}
		return;
	}
}

void CStartMgr::_CloseEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	CForm* frm = CFormMgr::s_Mgr.Find("frmVHelp");
	if (frm) {
		frm->Close();
	}
}

void CStartMgr::SysLabel(const char* pszFormat, ...) {
	char szBuf[2048] = {0};

	va_list list;
	va_start(list, pszFormat);
	vsprintf(szBuf, pszFormat, list);
	va_end(list);

	labFollow->SetCaption(szBuf);
	frmFollow->Show();
}

void CStartMgr::SysHide() {
	if (frmFollow->GetIsShow())
		frmFollow->Hide();
}

void CStartMgr::_MainChaRenderEvent(C3DCompent* pSender, int x, int y) {
	pMainCha->Render();
	// pTarget->Render();
}

void CStartMgr::RefreshMainFace(stNetChangeChaPart& stPart) {
	if (pMainCha) {
		static stNetTeamChaPart stTeamPart;
		stTeamPart.Convert(stPart);
		pMainCha->UpdataFace(stTeamPart);
	}
}

void CStartMgr::_OnSelfMenu(CGuiData* pSender, int x, int y, DWORD key) {
	CMenu* pMenu = dynamic_cast<CMenu*>(pSender);
	if (!pMenu)
		return;

	pMenu->SetIsShow(false);

	CMenuItem* pItem = pMenu->GetSelectMenu();
	if (!pItem)
		return;

	string str = pItem->GetString();
	// Modify by sunny.sun20080820
	// Begin
	const char* MapName = g_pGameApp->GetCurScene()->GetTerrainName(); // ???????j??????
	if (stricmp(MapName, "starena1") == 0 || stricmp(MapName, "starena2") == 0 || stricmp(MapName, "starena3") == 0) {
		pItem->SetIsEnabled(false);
	} else {
		pItem->SetIsEnabled(true);
		if (str == RES_STRING(CL_LANGUAGE_MATCH_483)) {
			CS_Team_Leave();
		}
	}
	// End
}

void CStartMgr::SetIsLeader(bool v) {
	imgLeader->SetIsShow(v);
}

bool CStartMgr::GetIsLeader() {
	return imgLeader->GetIsShow();
}

bool CStartMgr::IsCanTeamAndInfo() {
	if (!_IsCanTeam)
		g_pGameApp->SysInfo(RES_STRING(CL_LANGUAGE_MATCH_772));
	return _IsCanTeam;
}

void CStartMgr::RefreshPet() {
	if (g_stUIBoat.GetHuman()) {
		SItemGrid pGrid = g_stUIBoat.GetHuman()->GetPart().SLink[enumEQUIP_FAIRY];
		SItemGrid pApp = g_stUIBoat.GetHuman()->GetPart().SLink[enumEQUIP_FAIRYAPP];

		int ID = pGrid.sID;
		if (ID == 0) {
			return;
		}
		if (g_stUIBoat.GetHuman()->GetIsShowApparel() && pApp.sID != 0) {
			ID = pApp.sID;
		}
		CItemRecord* pInfo = GetItemRecordInfo(ID);
		int nLevel = pGrid.GetInstAttr(ITEMATTR_VAL_STR) + pGrid.GetInstAttr(ITEMATTR_VAL_AGI) + pGrid.GetInstAttr(ITEMATTR_VAL_DEX) + pGrid.GetInstAttr(ITEMATTR_VAL_CON) + pGrid.GetInstAttr(ITEMATTR_VAL_STA);

		sprintf(szBuf, "%d", nLevel);
		labPetLv->SetCaption(szBuf);

		static CGuiPic* Pic = imgPetHead->GetImage();
		Pic->LoadImage(pInfo->GetIconFile(),
					   imgPetHead->GetWidth(), imgPetHead->GetHeight(),
					   0,
					   4, 4);
	}
}
void CStartMgr::RefreshPet(SItemGrid pGrid, SItemGrid pApp) {
	int ID = pGrid.sID;
	if (g_stUIBoat.GetHuman()->GetIsShowApparel() && pApp.sID != 0) {
		ID = pApp.sID;
	}
	CItemRecord* pInfo = GetItemRecordInfo(ID);
	if (pInfo) {
		int nLevel = pGrid.GetInstAttr(ITEMATTR_VAL_STR) + pGrid.GetInstAttr(ITEMATTR_VAL_AGI) + pGrid.GetInstAttr(ITEMATTR_VAL_DEX) + pGrid.GetInstAttr(ITEMATTR_VAL_CON) + pGrid.GetInstAttr(ITEMATTR_VAL_STA);

		sprintf(szBuf, "%d", nLevel);
		labPetLv->SetCaption(szBuf);

		static CGuiPic* Pic = imgPetHead->GetImage();
		Pic->LoadImage(pInfo->GetIconFile(),
					   imgPetHead->GetWidth(), imgPetHead->GetHeight(),
					   0,
					   4, 4);
	}
}

void CStartMgr::RefreshPet(CItemCommand* pItem) {
	static CItemRecord* pInfo = nullptr;
	pInfo = pItem->GetItemInfo();

	static SItemHint s_item;
	memset(&s_item, 0, sizeof(SItemHint));
	s_item.Convert(pItem->GetData(), pInfo);

	// ???�?????,???
	int nLevel = s_item.sInstAttr[ITEMATTR_VAL_STR] + s_item.sInstAttr[ITEMATTR_VAL_AGI] + s_item.sInstAttr[ITEMATTR_VAL_DEX] + s_item.sInstAttr[ITEMATTR_VAL_CON] + s_item.sInstAttr[ITEMATTR_VAL_STA];

	sprintf(szBuf, "%d", nLevel);
	labPetLv->SetCaption(szBuf);

	static CGuiPic* Pic = imgPetHead->GetImage();
	Pic->LoadImage(pInfo->GetIconFile(),
				   imgPetHead->GetWidth(), imgPetHead->GetHeight(),
				   0,
				   4, 4);
}

void CStartMgr::RefreshMount() {
	if (!frmMainMount || !imgMountHead || !labMountLv) return;
	
	SItemGrid pGrid = g_stUIBoat.GetHuman()->GetPart().SLink[enumEQUIP_MOUNT];
	if (!pGrid.IsValid()) return;
	
	RefreshMount(pGrid);
}

void CStartMgr::RefreshMount(SItemGrid pGrid) {
	if (!frmMainMount || !imgMountHead || !labMountLv) return;
	
	int ID = pGrid.sID;
	CItemRecord* pInfo = GetItemRecordInfo(ID);
	if (!pInfo) return;
	
	// Get mount level from ITEMATTR_FORGE (chForgeLv)
	int nLevel = pGrid.GetInstAttr(ITEMATTR_FORGE);
	if (nLevel <= 0) nLevel = 1;  // Default to level 1 if not set
	
	sprintf(szBuf, "%d", nLevel);
	labMountLv->SetCaption(szBuf);
	
	// Load mount icon from item info
	CGuiPic* Pic = imgMountHead->GetImage();
	if (Pic) {
		Pic->LoadImage(pInfo->GetIconFile(),
					   imgMountHead->GetWidth(), imgMountHead->GetHeight(),
					   0,
					   4, 4);
	}
}

void CStartMgr::ShowHelpSystem(bool bShow, int nIndex) {
	int nCount = g_stUIStart.lstHelpList->GetItems()->GetCount();

	if (0 > nIndex || nCount <= nIndex) {
		// ???
		frmHelpSystem->SetIsShow(bShow);
		return;
	}

	for (int i = 0; i < nCount; ++i) {
		imgHelpShow1[i]->SetIsShow(nIndex == i);
		imgHelpShow2[i]->SetIsShow(nIndex == i);
		imgHelpShow3[i]->SetIsShow(nIndex == i);
		imgHelpShow4[i]->SetIsShow(nIndex == i);
	}

	frmHelpSystem->SetIsShow(bShow);
}

void CStartMgr::ShowLevelUpHelpButton(bool bShow) {
	if (btnLevelUpHelp) {
		btnLevelUpHelp->SetIsShow(bShow);
	}
}

void CStartMgr::ShowInfoCenterButton(bool bShow) {
	CTextButton* btnInfoCenter = dynamic_cast<CTextButton*>(frmMainFun->Find("btnInfoCenter"));
	if (btnInfoCenter) {
		btnInfoCenter->SetIsShow(bShow);
	}
}

void CStartMgr::_evtHelpListChange(CGuiData* pSender) {
	// g_pGameApp->MsgBox("Index: %d\nName:  %s", nIndex, pSender->GetName());//g_stUIStart.lstHelpList->GetSelectItem()->
	CListItems* pItems = g_stUIStart.lstHelpList->GetItems();
	int nIndex = pItems->GetIndex(g_stUIStart.lstHelpList->GetSelectItem());

	// g_stUIStart.ShowLevelUpHelpButton(false);
	g_stUIStart.ShowHelpSystem(true, nIndex);
}

void CStartMgr::ShowBagButtonForm(bool bShow) {
	if (frmBag) {
		frmBag->SetPos(frmMainFun->GetX2() - frmBag->GetWidth() - 109, frmMainFun->GetY() - frmBag->GetHeight() + 41);
		frmBag->Refresh();

		frmBag->SetIsShow(bShow);
	}
}

void CStartMgr::ShowSociliatyButtonForm(bool bShow) {
	if (frmSociliaty) {
		frmSociliaty->SetPos(frmMainFun->GetX2() - frmSociliaty->GetWidth() - 67, frmMainFun->GetY() - frmSociliaty->GetHeight() + 39);
		frmSociliaty->Refresh();

		frmSociliaty->SetIsShow(bShow);
	}
}

void CStartMgr::FetchRates() {
	CS_RequestDropRate();
	CS_RequestExpRate();
}

void CStartMgr::_evtShowMonsterInfo(CGuiData* pSender, int x, int y, DWORD key) {
	CForm* frm = CFormMgr::s_Mgr.Find("frmMonsterInfo");
	if (frm->GetIsShow()) {
		frm->SetIsShow(false);
		return;
	}
	g_stUIStart.FetchRates();
	g_pGameApp->Waiting(true);
}

void CStartMgr::_evtCheckLootFilter(CGuiData* pSender) {
	CCheckBox* chkDrop = dynamic_cast<CCheckBox*>(pSender);
	if (!chkDrop)
		return;

	CWorldScene* pScene = dynamic_cast<CWorldScene*>(CGameApp::GetCurScene());
	if (!pScene)
		return;

	int itemId = chkDrop->nTag;
	CItemRecord* pInfo = GetItemRecordInfo(itemId);
	if (!pInfo)
		return;

	bool bCheck = chkDrop->GetIsChecked();

	if (bCheck) {
		g_pGameApp->SysInfo("Loot Filter: %s is now visible", pInfo->szName);
	} else {
		g_pGameApp->SysInfo("Loot Filter: %s is now hidden", pInfo->szName);
	}
	pScene->FilterItemsByItemID(itemId, !bCheck);
}

// end