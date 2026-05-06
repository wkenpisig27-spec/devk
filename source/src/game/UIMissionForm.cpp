#include "StdAfx.h"
#include "uimissionform.h"
#include "uiform.h"
#include "uimemo.h"
#include "PacketCmd.h"
#include "gameapp.h"
#include "uiboxform.h"
#include "AutoPathService.h"

using namespace std;

using namespace GUI;

namespace {
const char* GetAutoPathFailureMessage(EAutoPathError error) {
	switch (error) {
	case EAutoPathError::InBoatMode:
		return "No routing on sea";
	case EAutoPathError::DifferentMap:
		return "Target is on another map. Use teleporter/portal first.";
	case EAutoPathError::InvalidLink:
		return "No auto-route available for this objective.";
	case EAutoPathError::InvalidCoordinates:
		return "Objective route has invalid coordinates.";
	default:
		return nullptr;
	}
}

void ShowAutoPathFailure(const SAutoPathResult& result, bool automatic) {
	if (result.success) {
		return;
	}

	const char* message = GetAutoPathFailureMessage(result.error);
	if (!message) {
		return;
	}

	static DWORD s_lastNotifyTick = 0;
	static EAutoPathError s_lastError = EAutoPathError::None;
	const DWORD now = GetTickCount();
	if (automatic && s_lastError == result.error && (now - s_lastNotifyTick) < 3000) {
		return;
	}

	s_lastNotifyTick = now;
	s_lastError = result.error;
	g_stUIBox.ShowMsgBox(nullptr, message, automatic);
}
} // namespace
//---------------------------------------------------------------------------
// class CMissionMgr  ???????
//---------------------------------------------------------------------------

CMissionMgr::CMissionMgr() {
	m_pMisForm = nullptr;
	m_pMisInfo = nullptr;
	m_pMisClose = nullptr;
	m_pMisBtn1 = nullptr;
	m_pMisBtn2 = nullptr;

	m_dwNpcID = -1;
	m_byMisCmd = -1;
}

CMissionMgr::~CMissionMgr() {
}

bool CMissionMgr::Init() // ?????????
{
	// npc??
	m_pMisForm = _FindForm("frmNPCMission");
	if (!m_pMisForm) {
		LG("gui", RES_STRING(CL_LANGUAGE_MATCH_740));
		return false;
	}

	m_pMisForm->evtEntrustMouseEvent = _MouseEvent;
	m_pMisInfo = dynamic_cast<CMemoEx*>(m_pMisForm->Find("memMission"));
	m_pMisInfo->evtClickItem = _ItemClickEvent;

	if (!m_pMisInfo) {
		Error(RES_STRING(CL_LANGUAGE_MATCH_45), m_pMisForm->GetName(), "memMission");
		return false;
	}

	m_pMisBtn1 = dynamic_cast<CTextButton*>(m_pMisForm->Find("btnYes"));
	if (!m_pMisBtn1) {
		Error(RES_STRING(CL_LANGUAGE_MATCH_45), m_pMisForm->GetName(), "btnYes");
		return false;
	}

	m_pMisBtn2 = dynamic_cast<CTextButton*>(m_pMisForm->Find("btnComplete"));
	if (!m_pMisBtn2) {
		Error(RES_STRING(CL_LANGUAGE_MATCH_45), m_pMisForm->GetName(), "btnComplete");
		return false;
	}

	m_pMisClose = dynamic_cast<CTextButton*>(m_pMisForm->Find("btnClose"));
	if (!m_pMisClose) {
		Error(RES_STRING(CL_LANGUAGE_MATCH_45), m_pMisForm->GetName(), "btnClose");
		return false;
	}

	return true;
}

void CMissionMgr::End() {
}

void CMissionMgr::_ItemClickEvent(string strItem) {
	SAutoPathResult result = CAutoPathService::NavigateFromText(strItem);
	ShowAutoPathFailure(result, false);
}

void CMissionMgr::_MouseEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	string strName = pSender->GetName();
	if (stricmp("frmNPCMission", pSender->GetForm()->GetName()) == 0) {
		// ???????,??????
		if (strName == "btnNo" || strName == "btnClose") {
			pSender->GetForm()->Close();
		} else if (strName == "btnYes" || strName == "btnComplete") {
			BYTE bySel = 0;
			if (g_stUIMission.m_pMisInfo->IsSelPrize()) {
				bySel = g_stUIMission.m_pMisInfo->GetSelPrize();
				if (bySel == (BYTE)-1) {
					g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_741));
					return;
				}
			}
			pSender->GetForm()->Close();
			CS_MissionPage(g_stUIMission.m_dwNpcID, g_stUIMission.m_byMisCmd, bySel);

			if (g_stUIMission.m_byMisCmd == ROLE_MIS_BTNACCEPT) {
				std::string autoPathPayload;
				if (!g_stUIMission.m_pMisInfo->TryGetFirstClickableNavPayload(autoPathPayload)) {
					g_stUIMission.m_pMisInfo->TryGetMissionAutoRoutePayload(autoPathPayload);
				}
				if (!autoPathPayload.empty()) {
					SAutoPathResult result = CAutoPathService::NavigateFromPayload(autoPathPayload);
					ShowAutoPathFailure(result, true);
				}
			}
		}
	}
}

void CMissionMgr::CloseForm() {
	if (m_pMisForm->GetIsShow())
		m_pMisForm->Close();
}

void CMissionMgr::ShowMissionPage(DWORD dwNpcID, BYTE byCmd, const NET_MISPAGE& page) {
	m_pMisBtn1->SetIsShow(false);
	m_pMisBtn2->SetIsShow(false);

	if (byCmd == ROLE_MIS_BTNACCEPT) // ????
	{
		m_pMisBtn1->SetIsShow(true);
	} else if (byCmd == ROLE_MIS_BTNDELIVERY) // ????
	{
		m_pMisBtn2->SetIsShow(true);
		m_pMisBtn2->SetIsEnabled(true);
		m_pMisInfo->SetIsSelect(TRUE);
	} else if (byCmd == ROLE_MIS_BTNPENDING) // ???,????
	{
		m_pMisBtn2->SetIsShow(true);
		m_pMisBtn2->SetIsEnabled(false);
	}

	m_dwNpcID = dwNpcID;
	m_byMisCmd = byCmd;

	m_pMisInfo->Init();
	m_pMisInfo->SetMisPage(page);
	m_pMisForm->SetIsShow(true);
}
