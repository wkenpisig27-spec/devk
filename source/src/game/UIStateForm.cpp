#include "StdAfx.h"
#include "uistateform.h"
#include "uiformmgr.h"
#include "uilabel.h"
#include "uitextbutton.h"
#include "gameapp.h"
#include "character.h"
#include "uiprogressbar.h"
#include "commfunc.h"
#include "ChaAttr.h"
#include "procirculate.h"
#include "packetcmd.h"
#include "tools.h"
#include "GuildData.h"
#include "uiboatform.h"
#include "worldscene.h"
#include "rmlui/RmlUiManager.h"
#include "rmlui/RmlUiCharacterForm.h"
#include <unordered_map>
using namespace std;
using namespace GUI;

int g_BattlePoints = 0;

void RmlCharacter_OnClose() {
	g_stUIState.HideCharacterUi();
}

void RmlCharacter_OnAllocate(const char* btnId) {
	if (!btnId)
		return;

	std::unordered_map<std::string, int> buttonAttributeMap = {
		{"btnStr", ATTR_STR},
		{"btnAgi", ATTR_AGI},
		{"btnCon", ATTR_CON},
		{"btnSta", ATTR_STA},
		{"btnDex", ATTR_DEX}};

	auto it = buttonAttributeMap.find(btnId);
	if (it == buttonAttributeMap.end())
		return;

	CCharacter* pCha = g_stUIBoat.GetHuman();
	if (!pCha || pCha->getGameAttr()->get(ATTR_AP) <= 0)
		return;

	CChaAttr attr;
	attr.ResetChangeFlag();
	attr.DirectSetAttr(it->second, 1);
	attr.SetChangeBitFlag(it->second);

	CProCirculateCS* proCir = (CProCirculateCS*)g_NetIF->GetProCir();
	proCir->SynBaseAttribute(&attr);
	g_stUIState.RefreshStateFrm();
}

//---------------------------------------------------------------------------
//  class CStateMgr
//---------------------------------------------------------------------------
bool CStateMgr::Init() {
	CFormMgr& mgr = CFormMgr::s_Mgr;
	frmState = _FindForm("frmState");
	if (!frmState)
		return false;
	frmState->evtShow = _evtMainShow;
	// Data host only — player chrome is Notice Rml character panel.
	frmState->SetHotKey(0);
	frmState->SetIsEscClose(false);
	frmState->SetIsShow(false);
	CFormMgr::s_Mgr.AddHotKeyEvent(_OnCharacterHotKey);

	labStateName = dynamic_cast<CLabelEx*>(frmState->Find("labStateName"));
	if (!labStateName)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmState->GetName(), "labStateName");
	labStateName->SetIsCenter(true);

	FORM_CONTROL_LOADING_CHECK(labGuildName, frmState, CLabelEx, "preperty.clu", "labStateGuid");

	labStateJob = dynamic_cast<CLabelEx*>(frmState->Find("labStateJob"));
	if (!labStateJob)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmState->GetName(), "labStateJob");

	labStateLevel = dynamic_cast<CLabelEx*>(frmState->Find("labStateLevel"));
	if (!labStateLevel)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmState->GetName(), "labStateLevel");

	labStatePoint = dynamic_cast<CLabelEx*>(frmState->Find("labStatePoint"));
	if (!labStatePoint)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmState->GetName(), "labStatePoint");

	labSkillPoint = dynamic_cast<CLabelEx*>(frmState->Find("labSkillPoint"));
	if (!labSkillPoint)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmState->GetName(), "labSkillPoint");

	labFameShow = dynamic_cast<CLabelEx*>(frmState->Find("labFameShow"));
	if (!labFameShow)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmState->GetName(), "labFameShow");

	labBattlepoints = dynamic_cast<CLabelEx*>(frmState->Find("labBattlepoints"));
	if (!labBattlepoints)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmState->GetName(), "labBattlepoints");

	btnStr = dynamic_cast<CTextButton*>(frmState->Find("btnStr"));
	if (!btnStr)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmState->GetName(), "btnStr");
	btnStr->evtMouseClick = MainMouseDown;
	btnStr->evtMouseDownContinue = MainMouseDownContinue;

	btnAgi = dynamic_cast<CTextButton*>(frmState->Find("btnAgi"));
	if (!btnAgi)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmState->GetName(), "btnAgi");
	btnAgi->evtMouseClick = MainMouseDown;
	btnAgi->evtMouseDownContinue = MainMouseDownContinue;

	btnCon = dynamic_cast<CTextButton*>(frmState->Find("btnCon"));
	if (!btnCon)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmState->GetName(), "btnCon");
	btnCon->evtMouseClick = MainMouseDown;
	btnCon->evtMouseDownContinue = MainMouseDownContinue;

	btnSta = dynamic_cast<CTextButton*>(frmState->Find("btnSta"));
	if (!btnSta)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmState->GetName(), "btnSta");
	btnSta->evtMouseClick = MainMouseDown;
	btnSta->evtMouseDownContinue = MainMouseDownContinue;

	btnDex = dynamic_cast<CTextButton*>(frmState->Find("btnDex"));
	if (!btnDex)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmState->GetName(), "btnDex");
	btnDex->evtMouseClick = MainMouseDown;
	btnDex->evtMouseDownContinue = MainMouseDownContinue;

	labStateEXP = dynamic_cast<CLabelEx*>(frmState->Find("labStateEXP"));
	if (!labStateEXP)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmState->GetName(), "labStateEXP");
	labStateEXP->SetIsCenter(true);

	labStateHP = dynamic_cast<CLabelEx*>(frmState->Find("labStateHP"));
	if (!labStateHP)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmState->GetName(), "labStateHP");
	labStateHP->SetIsCenter(true);

	labStateSP = dynamic_cast<CLabelEx*>(frmState->Find("labStateSP"));
	if (!labStateSP)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmState->GetName(), "labStateSP");
	labStateSP->SetIsCenter(true);

	labStrshow = (CLabelEx*)frmState->Find("labStrshow");
	labDexshow = (CLabelEx*)frmState->Find("labDexshow");
	labAgishow = (CLabelEx*)frmState->Find("labAgishow");
	labConshow = (CLabelEx*)frmState->Find("labConshow");
	labStashow = (CLabelEx*)frmState->Find("labStashow");
	labSailLevel = (CLabelEx*)frmState->Find("labSailLevel");
	labSailEXP = (CLabelEx*)frmState->Find("labSailEXP");

	labMinAtackShow = (CLabelEx*)frmState->Find("labMinAtackShow");
	labMaxAtackShow = (CLabelEx*)frmState->Find("labMaxAtackShow");
	labFleeShow = (CLabelEx*)frmState->Find("labFleeShow");
	labAspeedShow = (CLabelEx*)frmState->Find("labAspeedShow");
	labMspeedShow = (CLabelEx*)frmState->Find("labMspeedShow");
	labHitShow = (CLabelEx*)frmState->Find("labHitShow");
	labDefenceShow = (CLabelEx*)frmState->Find("labDefenceShow");
	labPhysDefineShow = (CLabelEx*)frmState->Find("labPhysDefineShow");
	return true;
}

void CStateMgr::End() {
}

void CStateMgr::FrameMove(DWORD dwTime) {
	if (IsCharacterUiVisible()) {
		static CTimeWork time(100);
		if (time.IsTimeOut(dwTime))
			RefreshStateFrm();
	}
	// Never leave legacy chrome visible.
	if (frmState && frmState->GetIsShow())
		frmState->SetIsShow(false);
}

void CStateMgr::_evtMainShow(CGuiData* pSender) {
	(void)pSender;
	// Something asked frmState to show — redirect to Notice Rml and hide host.
	if (g_stUIState.frmState)
		g_stUIState.frmState->SetIsShow(false);
	g_stUIState.ShowCharacterUi();
}

bool CStateMgr::_OnCharacterHotKey(char& key, int& control) {
	(void)control;
	if (key != 'A' && key != 'a')
		return false;
	if (!CFormMgr::s_Mgr.GetEnableHotKey())
		return false;
	if (!dynamic_cast<CWorldScene*>(CGameApp::GetCurScene()))
		return false;
	g_stUIState.ToggleCharacterUi();
	return true; // swallow Alt+A so lua cannot reopen legacy frmState
}

void CStateMgr::ShowCharacterUi() {
	if (frmState && frmState->GetIsShow())
		frmState->SetIsShow(false);

	if (!CRmlUiManager::Instance().IsReady())
		return;

	CRmlUiCharacterForm::Instance().Show();
	RefreshStateFrm();
}

void CStateMgr::HideCharacterUi() {
	CRmlUiCharacterForm::Instance().Hide();
	if (frmState && frmState->GetIsShow())
		frmState->SetIsShow(false);
}

bool CStateMgr::IsCharacterUiVisible() const {
	return CRmlUiCharacterForm::Instance().IsVisible();
}

void CStateMgr::ToggleCharacterUi() {
	if (IsCharacterUiVisible()) {
		HideCharacterUi();
		return;
	}
	ShowCharacterUi();
}

void CStateMgr::AllocateAttribute(int attributeType) {
	CCharacter* pCha = g_stUIBoat.GetHuman();
	if (!pCha || pCha->getGameAttr()->get(ATTR_AP) <= 0)
		return;

	CChaAttr attr;
	attr.ResetChangeFlag();
	attr.DirectSetAttr(attributeType, 1);
	attr.SetChangeBitFlag(attributeType);

	CProCirculateCS* proCir = (CProCirculateCS*)g_NetIF->GetProCir();
	proCir->SynBaseAttribute(&attr);
}

void CStateMgr::RefreshStateFrm() {
	if (!IsCharacterUiVisible())
		return;

	CCharacter* pCha = g_stUIBoat.GetHuman();
	if (!pCha)
		return;

	SGameAttr* pCChaAttr = pCha->getGameAttr();
	if (!pCChaAttr)
		return;

	char pszCha[256] = {0};
	CRmlUiCharacterForm::StateView view;

	snprintf(pszCha, sizeof(pszCha), "%lld/%lld", pCChaAttr->get(ATTR_HP), pCChaAttr->get(ATTR_MXHP));
	view.hp = pszCha;
	{
		const LONG64 hp = pCChaAttr->get(ATTR_HP);
		const LONG64 mxhp = pCChaAttr->get(ATTR_MXHP);
		view.hpPct = (mxhp > 0) ? (float)((hp * 100.0) / mxhp) : 0.f;
	}

	LONG64 num = pCChaAttr->get(ATTR_CEXP);
	LONG64 curlev = pCChaAttr->get(ATTR_CLEXP);
	LONG64 nextlev = pCChaAttr->get(ATTR_NLEXP);

	LONG64 max = nextlev - curlev;
	num = num - curlev;
	if (num < 0)
		num = 0;

	if (max != 0) {
		view.expPct = (float)((num * 100.0) / max);
		sprintf(pszCha, "%4.2f%%", view.expPct);
	} else {
		view.expPct = 0.f;
		sprintf(pszCha, "0.00%%");
	}
	view.exp = pszCha;

	num = pCChaAttr->get(ATTR_SP);
	max = pCChaAttr->get(ATTR_MXSP);
	snprintf(pszCha, sizeof(pszCha), "%lld/%lld", num, max);
	view.sp = pszCha;
	view.spPct = (max > 0) ? (float)((num * 100.0) / max) : 0.f;

	view.name = pCha->getName() ? pCha->getName() : "";
	if (CGuildData::GetGuildID())
		view.guild = CGuildData::GetGuildName();
	else
		view.guild.clear();

	view.job = g_GetJobName((short)pCChaAttr->get(ATTR_JOB));

	sprintf(pszCha, "%d", (int)pCChaAttr->get(ATTR_LV));
	view.level = pszCha;

	sprintf(pszCha, "%d", (int)pCChaAttr->get(ATTR_AP));
	view.statPts = pszCha;

	sprintf(pszCha, "%d", (int)pCChaAttr->get(ATTR_TP));
	view.skillPts = pszCha;

	view.showAllocate = pCChaAttr->get(ATTR_AP) > 0;

	sprintf(pszCha, "%d", (int)pCChaAttr->get(ATTR_STR));
	view.str = pszCha;
	sprintf(pszCha, "%d", (int)pCChaAttr->get(ATTR_DEX));
	view.dex = pszCha;
	sprintf(pszCha, "%d", (int)pCChaAttr->get(ATTR_AGI));
	view.agi = pszCha;
	sprintf(pszCha, "%d", (int)pCChaAttr->get(ATTR_CON));
	view.con = pszCha;
	sprintf(pszCha, "%d", (int)pCChaAttr->get(ATTR_STA));
	view.sta = pszCha;

	sprintf(pszCha, "%d", (int)pCChaAttr->get(ATTR_MNATK));
	view.minAtk = pszCha;
	sprintf(pszCha, "%d", (int)pCChaAttr->get(ATTR_MXATK));
	view.maxAtk = pszCha;
	sprintf(pszCha, "%d", (int)pCChaAttr->get(ATTR_FLEE));
	view.flee = pszCha;

	{
		int v = (int)pCChaAttr->get(ATTR_ASPD);
		if (v == 0)
			sprintf(pszCha, "-1");
		else
			sprintf(pszCha, "%d", 100000 / v);
		view.aspeed = pszCha;
	}

	sprintf(pszCha, "%d", (int)pCChaAttr->get(ATTR_MSPD));
	view.mspeed = pszCha;
	sprintf(pszCha, "%d", (int)pCChaAttr->get(ATTR_HIT));
	view.hit = pszCha;
	sprintf(pszCha, "%d", (int)pCChaAttr->get(ATTR_DEF));
	view.def = pszCha;
	sprintf(pszCha, "%d", (int)pCChaAttr->get(ATTR_PDEF));
	view.pdef = pszCha;
	sprintf(pszCha, "%d", (int)pCChaAttr->get(ATTR_FAME));
	view.fame = pszCha;

	sprintf(pszCha, "%ld", g_BattlePoints);
	view.battle = pszCha;

	CRmlUiCharacterForm::Instance().ApplyState(view);

	// Keep legacy labels in sync if anything still reads them.
	if (labStateHP)
		labStateHP->SetCaption(view.hp.c_str());
	if (labStateEXP)
		labStateEXP->SetCaption(view.exp.c_str());
	if (labStateSP)
		labStateSP->SetCaption(view.sp.c_str());
	if (labStateName)
		labStateName->SetCaption(view.name.c_str());
	if (labGuildName)
		labGuildName->SetCaption(view.guild.c_str());
	if (labStateJob)
		labStateJob->SetCaption(view.job.c_str());
	if (labStateLevel)
		labStateLevel->SetCaption(view.level.c_str());
	if (labStatePoint)
		labStatePoint->SetCaption(view.statPts.c_str());
	if (labSkillPoint)
		labSkillPoint->SetCaption(view.skillPts.c_str());
	if (labStrshow)
		labStrshow->SetCaption(view.str.c_str());
	if (labDexshow)
		labDexshow->SetCaption(view.dex.c_str());
	if (labAgishow)
		labAgishow->SetCaption(view.agi.c_str());
	if (labConshow)
		labConshow->SetCaption(view.con.c_str());
	if (labStashow)
		labStashow->SetCaption(view.sta.c_str());
	if (labMinAtackShow)
		labMinAtackShow->SetCaption(view.minAtk.c_str());
	if (labMaxAtackShow)
		labMaxAtackShow->SetCaption(view.maxAtk.c_str());
	if (labFleeShow)
		labFleeShow->SetCaption(view.flee.c_str());
	if (labAspeedShow)
		labAspeedShow->SetCaption(view.aspeed.c_str());
	if (labMspeedShow)
		labMspeedShow->SetCaption(view.mspeed.c_str());
	if (labHitShow)
		labHitShow->SetCaption(view.hit.c_str());
	if (labDefenceShow)
		labDefenceShow->SetCaption(view.def.c_str());
	if (labPhysDefineShow)
		labPhysDefineShow->SetCaption(view.pdef.c_str());
	if (labFameShow)
		labFameShow->SetCaption(view.fame.c_str());

	if (btnStr)
		btnStr->SetIsShow(view.showAllocate);
	if (btnAgi)
		btnAgi->SetIsShow(view.showAllocate);
	if (btnCon)
		btnCon->SetIsShow(view.showAllocate);
	if (btnSta)
		btnSta->SetIsShow(view.showAllocate);
	if (btnDex)
		btnDex->SetIsShow(view.showAllocate);

	// Battle points: poll slowly; display uses cached g_BattlePoints (no SetInnerRML spam).
	static CTimeWork battlePoll(2000);
	if (battlePoll.IsTimeOut(CGameApp::GetCurTick()))
		CS_RequestBattlePoint();
	UpdateBattlePointDisplay();
}

void CStateMgr::UpdateBattlePointDisplay() {
	char pszCha[32];
	sprintf(pszCha, "%ld", g_BattlePoints);

	if (labBattlepoints)
		labBattlepoints->SetCaption((const char*)pszCha);

	if (IsCharacterUiVisible())
		CRmlUiCharacterForm::Instance().SetBattlePoints(pszCha);
}

long CStateMgr::BattlePoints() {
	return g_BattlePoints;
}

void CStateMgr::MainMouseDown(CGuiData* pSender, int x, int y, DWORD key) {
	(void)x;
	(void)y;
	(void)key;
	if (!pSender)
		return;

	std::unordered_map<std::string, int> buttonAttributeMap = {
		{"btnStr", ATTR_STR},
		{"btnAgi", ATTR_AGI},
		{"btnCon", ATTR_CON},
		{"btnSta", ATTR_STA},
		{"btnDex", ATTR_DEX}};

	auto it = buttonAttributeMap.find(pSender->GetName());
	if (it != buttonAttributeMap.end())
		AllocateAttribute(it->second);
}

void CStateMgr::MainMouseDownContinue(CGuiData* pSender) {
	MainMouseDown(pSender, 0, 0, 0);
}
