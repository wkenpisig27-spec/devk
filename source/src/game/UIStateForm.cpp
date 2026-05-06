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
#include <unordered_map>
using namespace std;
using namespace GUI;

int g_BattlePoints = 0;

//---------------------------------------------------------------------------
//  class CStateMgr
//---------------------------------------------------------------------------
bool CStateMgr::Init() {
	CFormMgr& mgr = CFormMgr::s_Mgr;
	frmState = _FindForm("frmState"); // ���Ա���
	if (!frmState)
		return false;
	frmState->evtShow = _evtMainShow;

	// frmState �ڵĿؼ��б�
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

	// 6��
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

	// btnLuk  = dynamic_cast<CTextButton *>(frmState->Find("btnLuk"));
	// if( !btnLuk )			return Error( "msgui.clu����<%s>���Ҳ����ؼ�<%s>", frmState->GetName(), "btnLuk" );
	// btnLuk->evtMouseClick = MainMouseDown;

	btnDex = dynamic_cast<CTextButton*>(frmState->Find("btnDex"));
	if (!btnDex)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmState->GetName(), "btnDex");
	btnDex->evtMouseClick = MainMouseDown;
	btnDex->evtMouseDownContinue = MainMouseDownContinue;

	// frmState�µĽ������ؼ�
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

	// 6����������
	labStrshow = (CLabelEx*)frmState->Find("labStrshow");
	labDexshow = (CLabelEx*)frmState->Find("labDexshow");
	labAgishow = (CLabelEx*)frmState->Find("labAgishow");
	labConshow = (CLabelEx*)frmState->Find("labConshow");
	labStashow = (CLabelEx*)frmState->Find("labStashow");
	// labLukshow  = 	( CLabelEx *)frmState->Find( "labLukshow" );
	labSailLevel = (CLabelEx*)frmState->Find("labSailLevel");
	labSailEXP = (CLabelEx*)frmState->Find("labSailEXP");

	// 8����������
	labMinAtackShow = (CLabelEx*)frmState->Find("labMinAtackShow");
	labMaxAtackShow = (CLabelEx*)frmState->Find("labMaxAtackShow");
	labFleeShow = (CLabelEx*)frmState->Find("labFleeShow");
	labAspeedShow = (CLabelEx*)frmState->Find("labAspeedShow");
	labMspeedShow = (CLabelEx*)frmState->Find("labMspeedShow");
	labHitShow = (CLabelEx*)frmState->Find("labHitShow");
	labDefenceShow = (CLabelEx*)frmState->Find("labDefenceShow");
	// labCriticalShow  = 	( CLabelEx *)frmState->Find( "labCriticalShow" );
	// labMfShow        = 	( CLabelEx *)frmState->Find( "labMfShow" );
	labPhysDefineShow = (CLabelEx*)frmState->Find("labPhysDefineShow");
	return true;
}

void CStateMgr::End() {
}

void CStateMgr::FrameMove(DWORD dwTime) {
	if (frmState->GetIsShow()) {
		static CTimeWork time(100);
		if (time.IsTimeOut(dwTime))
			RefreshStateFrm();
	}
}

void CStateMgr::_evtMainShow(CGuiData* pSender) {
	g_stUIState.RefreshStateFrm();
}

void CStateMgr::RefreshStateFrm() {
	CForm* f = g_stUIState.frmState;
	if (!f->GetIsShow())
		return;

	CCharacter* pCha = g_stUIBoat.GetHuman();
	if (!pCha)
		return;

	SGameAttr* pCChaAttr = pCha->getGameAttr();
	if (!pCChaAttr)
		return;

	char pszCha[256] = {0};

	// Ѫ��
	if (labStateHP) {
		snprintf(pszCha, sizeof(pszCha), "%lld/%lld", pCChaAttr->get(ATTR_HP), pCChaAttr->get(ATTR_MXHP));
		labStateHP->SetCaption(pszCha);
	}

	// ��Ҿ���
	LONG64 num = pCChaAttr->get(ATTR_CEXP);
	LONG64 curlev = pCChaAttr->get(ATTR_CLEXP);
	LONG64 nextlev = pCChaAttr->get(ATTR_NLEXP);

	LONG64 max = nextlev - curlev;
	num = num - curlev;
	if (num < 0)
		num = 0;

	if (labStateEXP) {
		if (max != 0)
			sprintf(pszCha, "%4.2f%%", num * 100.0f / max);
		else
			sprintf(pszCha, "0.00%");

		labStateEXP->SetCaption(pszCha);
	}

	// ħ��
	num = pCChaAttr->get(ATTR_SP);
	max = pCChaAttr->get(ATTR_MXSP);
	if (labStateSP) {
		snprintf(pszCha, sizeof(pszCha), "%lld/%lld", num, max);
		labStateSP->SetCaption(pszCha);
	}

	// �������
	if (labStateName) {
		sprintf(pszCha, "%s", pCha->getName());
		labStateName->SetCaption((const char*)pszCha);
	}

	// ����
	if (labGuildName) {
		if (CGuildData::GetGuildID()) {
			labGuildName->SetCaption(CGuildData::GetGuildName().c_str());
		} else {
			labGuildName->SetCaption("");
		}
	}

	// ���ְҵ
	if (labStateJob) {
		sprintf(pszCha, "%s", g_GetJobName((short)pCChaAttr->get(ATTR_JOB)));
		labStateJob->SetCaption((const char*)pszCha);
	}

	// ��ҵȼ�
	if (labStateLevel) {
		sprintf(pszCha, "%d", pCChaAttr->get(ATTR_LV)); // ��ʾ��ɫ�ȼ�
		labStateLevel->SetCaption((const char*)pszCha);
	}

	// ������Ե�
	if (labStatePoint) {
		sprintf(pszCha, "%d", pCChaAttr->get(ATTR_AP)); // ��ʾ��ɫ�����Ե�
		labStatePoint->SetCaption((const char*)pszCha);
	}

	if (labSkillPoint) {
		sprintf(pszCha, "%d", pCChaAttr->get(ATTR_TP)); // ��ʾ��ɫ�ļ��ܵ�
		labSkillPoint->SetCaption((const char*)pszCha);
	}

	// ���Ե����0������ʾ6���������԰�ť
	if (pCChaAttr->get(ATTR_AP) > 0) {
		btnStr->SetIsShow(true);
		btnAgi->SetIsShow(true);
		btnCon->SetIsShow(true);
		btnSta->SetIsShow(true);
		// btnLuk->SetIsShow(true);
		btnDex->SetIsShow(true);
	} else {
		btnStr->SetIsShow(false);
		btnAgi->SetIsShow(false);
		btnCon->SetIsShow(false);
		btnSta->SetIsShow(false);
		// btnLuk->SetIsShow(false);
		btnDex->SetIsShow(false);
	}
	// 6����������
	if (labStrshow) {
		sprintf(pszCha, "%d", pCChaAttr->get(ATTR_STR));
		labStrshow->SetCaption((const char*)pszCha);
	}

	if (labDexshow) {
		sprintf(pszCha, "%d", pCChaAttr->get(ATTR_DEX));
		labDexshow->SetCaption((const char*)pszCha);
	}

	if (labAgishow) {
		sprintf(pszCha, "%d", pCChaAttr->get(ATTR_AGI));
		labAgishow->SetCaption((const char*)pszCha);
	}

	if (labConshow) {
		sprintf(pszCha, "%d", pCChaAttr->get(ATTR_CON));
		labConshow->SetCaption((const char*)pszCha);
	}

	if (labStashow) {
		sprintf(pszCha, "%d", pCChaAttr->get(ATTR_STA));
		labStashow->SetCaption((const char*)pszCha);
	}

	if (labSailLevel) {
		sprintf(pszCha, "%d", pCChaAttr->get(ATTR_SAILLV));
		labSailLevel->SetCaption((const char*)pszCha);
	}

	if (labSailEXP) {
		sprintf(pszCha, "%d", pCChaAttr->get(ATTR_CSAILEXP));
		labSailEXP->SetCaption((const char*)pszCha);
	}

	// sprintf( pszCha , "%d" , pCChaAttr->get(ATTR_LUK));
	// if(labLukshow)		labLukshow->SetCaption( (const char* ) pszCha);

	// 8����������
	if (labMinAtackShow) {
		sprintf(pszCha, "%d", pCChaAttr->get(ATTR_MNATK)); // ��С������
		labMinAtackShow->SetCaption((const char*)pszCha);
	}

	if (labMaxAtackShow) {
		sprintf(pszCha, "%d", pCChaAttr->get(ATTR_MXATK)); // ��󹥻���
		labMaxAtackShow->SetCaption((const char*)pszCha);
	}

	if (labFleeShow) {
		sprintf(pszCha, "%d", pCChaAttr->get(ATTR_FLEE)); // ������
		labFleeShow->SetCaption((const char*)pszCha);
	}

	if (labAspeedShow) {
		int v = pCChaAttr->get(ATTR_ASPD);
		if (v == 0)
			sprintf(pszCha, "-1");
		else
			sprintf(pszCha, "%d", 100000 / v); // �������

		labAspeedShow->SetCaption((const char*)pszCha);
	}

	if (labMspeedShow) {
		sprintf(pszCha, "%d", pCChaAttr->get(ATTR_MSPD));
		labMspeedShow->SetCaption((const char*)pszCha);
	}

	if (labHitShow) {
		sprintf(pszCha, "%d", pCChaAttr->get(ATTR_HIT)); // ������
		labHitShow->SetCaption((const char*)pszCha);
	}

	if (labDefenceShow) {
		sprintf(pszCha, "%d", pCChaAttr->get(ATTR_DEF)); // ������
		labDefenceShow->SetCaption((const char*)pszCha);
	}

	// if(labCriticalShow)
	//{
	//	sprintf( pszCha , "%d" , pCChaAttr->get(ATTR_CRT)); // ������
	//	labCriticalShow->SetCaption( (const char* ) pszCha);
	// }

	// if(labMfShow)
	//{
	//	sprintf( pszCha , "%d" , pCChaAttr->get(ATTR_MF));	 // Ѱ����
	//	labMfShow->SetCaption( (const char* ) pszCha);
	// }

	if (labPhysDefineShow) {
		sprintf(pszCha, "%d", pCChaAttr->get(ATTR_PDEF)); // �����ֿ�
		labPhysDefineShow->SetCaption((const char*)pszCha);
	}

	if (labFameShow) {
		sprintf(pszCha, "%d", pCChaAttr->get(ATTR_FAME)); // ����
		labFameShow->SetCaption((const char*)pszCha);
	}

	if (labBattlepoints) {
		CS_RequestBattlePoint();
		sprintf(pszCha, "%ld", g_BattlePoints); // ����
		labBattlepoints->SetCaption((const char*)pszCha);
	}
}

void CStateMgr::MainMouseDown(CGuiData* pSender, int x, int y, DWORD key) {
	CCharacter* pCha = g_stUIBoat.GetHuman();
	if (!pCha || pCha->getGameAttr()->get(ATTR_AP) <= 0)
		return;

	CChaAttr attr;
	attr.ResetChangeFlag();

	// Map button names to attribute types
	std::unordered_map<std::string, int> buttonAttributeMap = {
		{"btnStr", ATTR_STR},
		{"btnAgi", ATTR_AGI},
		{"btnCon", ATTR_CON},
		{"btnSta", ATTR_STA},
		{"btnDex", ATTR_DEX}};

	std::string name = pSender->GetName();

	// Check if the button name is in the map
	auto it = buttonAttributeMap.find(name);
	if (it != buttonAttributeMap.end()) {
		int attributeType = it->second;

		attr.DirectSetAttr(attributeType, 1);
		attr.SetChangeBitFlag(attributeType);

		CProCirculateCS* proCir = (CProCirculateCS*)g_NetIF->GetProCir();
		proCir->SynBaseAttribute(&attr);
	}
}

void CStateMgr::MainMouseDownContinue(CGuiData* pSender) {
	CCharacter* pCha = g_stUIBoat.GetHuman();
	if (!pCha || pCha->getGameAttr()->get(ATTR_AP) <= 0)
		return;

	CChaAttr attr;
	attr.ResetChangeFlag();

	// Map button names to attribute types
	std::unordered_map<std::string, int> buttonAttributeMap = {
		{"btnStr", ATTR_STR},
		{"btnAgi", ATTR_AGI},
		{"btnCon", ATTR_CON},
		{"btnSta", ATTR_STA},
		{"btnDex", ATTR_DEX}};

	std::string name = pSender->GetName();

	// Check if the button name is in the map
	auto it = buttonAttributeMap.find(name);
	if (it != buttonAttributeMap.end()) {
		int attributeType = it->second;

		attr.DirectSetAttr(attributeType, 1);
		attr.SetChangeBitFlag(attributeType);

		CProCirculateCS* proCir = (CProCirculateCS*)g_NetIF->GetProCir();
		proCir->SynBaseAttribute(&attr);
	}
}