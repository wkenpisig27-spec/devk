#include "StdAfx.h"
#include "uimissionform.h"
#include "uiform.h"
#include "uimemo.h"
#include "PacketCmd.h"
#include "gameapp.h"
#include "uiboxform.h"
#include "AutoPathService.h"
#include "ItemRecord.h"
#include "CharacterRecord.h"
#include "rmlui/RmlUiNpcMissionForm.h"

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

	m_pMisForm->SetIsEscClose(false);
	m_pMisForm->SetIsShow(false);
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
	HideMissionUi();
}

void CMissionMgr::HideMissionUi() {
	CRmlUiNpcMissionForm::Instance().Hide();
	if (m_pMisForm && m_pMisForm->GetIsShow())
		m_pMisForm->SetIsShow(false);
}

bool CMissionMgr::IsMissionUiVisible() const {
	return CRmlUiNpcMissionForm::Instance().IsVisible();
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
	if (m_pMisForm && m_pMisForm->GetIsShow())
		m_pMisForm->SetIsShow(false);

	CRmlUiNpcMissionForm::MissionView view;
	auto strip = [](const char* text) {
		std::string out;
		if (!text)
			return out;
		for (const char* p = text; *p; ++p) {
			if (*p == '_') {
				out += '\n';
				continue;
			}
			if (*p == '<') {
				const char* end = strchr(p, '>');
				if (!end)
					break;
				if (strncmp(p, "<nav:", 5) == 0) {
					const char* bar = strchr(p, '|');
					if (bar && bar < end)
						out.append(bar + 1, (size_t)(end - bar - 1));
				}
				p = end;
				continue;
			}
			out += *p;
		}
		return out;
	};

	view.title = strip(page.szName);
	view.body = strip(page.szDesp);
	view.showAccept = (byCmd == ROLE_MIS_BTNACCEPT);
	view.showComplete = (byCmd == ROLE_MIS_BTNDELIVERY || byCmd == ROLE_MIS_BTNPENDING);
	view.completeEnabled = (byCmd == ROLE_MIS_BTNDELIVERY);
	view.prizePick = (page.byPrizeNum > 0 && page.byPrizeSelType == mission::PRIZE_SELONE);
	m_prizePick = view.prizePick;

	char buf[256];
	for (int i = 0; i < page.byNeedNum; ++i) {
		CRmlUiNpcMissionForm::NeedView need;
		const NET_MISNEED& n = page.MisNeed[i];
		if (n.byType == mission::MIS_NEED_ITEM) {
			CItemRecord* item = GetItemRecordInfo(n.wParam1);
			sprintf_s(buf, "Obtain %s  %d/%d", item ? item->szName : "Unknown", n.wParam3, n.wParam2);
			need.text = buf;
			need.done = n.wParam2 > 0 && n.wParam3 >= n.wParam2;
		} else if (n.byType == mission::MIS_NEED_KILL) {
			CChaRecord* cha = GetChaRecordInfo(n.wParam1);
			sprintf_s(buf, "Hunt %s  %d/%d", cha ? cha->szName : "Unknown", n.wParam3, n.wParam2);
			need.text = buf;
			need.done = n.wParam2 > 0 && n.wParam3 >= n.wParam2;
		} else if (n.byType == mission::MIS_NEED_DESP) {
			need.text = strip(n.szNeed);
		} else {
			need.text = "Unknown objective";
		}
		if (!need.text.empty())
			view.needs.push_back(std::move(need));
	}

	for (int i = 0; i < page.byPrizeNum; ++i) {
		CRmlUiNpcMissionForm::PrizeView prize;
		prize.index = i;
		prize.selectable = view.prizePick;
		const NET_MISPRIZE& p = page.MisPrize[i];
		if (p.byType == mission::MIS_PRIZE_ITEM) {
			CItemRecord* item = GetItemRecordInfo(p.wParam1);
			sprintf_s(buf, "%d %s", p.wParam2, item ? item->szName : "Unknown item");
			prize.text = buf;
			if (item && item->GetIconFile())
				prize.iconPath = item->GetIconFile();
		} else if (p.byType == mission::MIS_PRIZE_MONEY) {
			sprintf_s(buf, "%dG", p.wParam1);
			prize.text = buf;
		} else if (p.byType == mission::MIS_PRIZE_FAME) {
			sprintf_s(buf, "%d Reputation", p.wParam1);
			prize.text = buf;
		} else if (p.byType == mission::MIS_PRIZE_CESS) {
			sprintf_s(buf, "%d Commerce", p.wParam1);
			prize.text = buf;
		} else {
			prize.text = "Unknown reward";
		}
		view.prizes.push_back(std::move(prize));
	}

	CRmlUiNpcMissionForm::Instance().ApplyView(view);
}

void CMissionMgr::ConfirmMission() {
	BYTE bySel = 0;
	if (m_prizePick) {
		const int prize = CRmlUiNpcMissionForm::Instance().GetSelectedPrize();
		if (prize < 0) {
			g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_741));
			return;
		}
		bySel = (BYTE)prize;
	}

	std::string autoPathPayload;
	if (m_byMisCmd == ROLE_MIS_BTNACCEPT && m_pMisInfo) {
		if (!m_pMisInfo->TryGetFirstClickableNavPayload(autoPathPayload))
			m_pMisInfo->TryGetMissionAutoRoutePayload(autoPathPayload);
	}

	const DWORD npcId = m_dwNpcID;
	const BYTE cmd = m_byMisCmd;
	HideMissionUi();
	CS_MissionPage(npcId, cmd, bySel);
	if (!autoPathPayload.empty()) {
		SAutoPathResult result = CAutoPathService::NavigateFromPayload(autoPathPayload);
		ShowAutoPathFailure(result, true);
	}
}

void RmlNpcMis_OnClose() {
	g_stUIMission.HideMissionUi();
}

void RmlNpcMis_OnAccept() {
	g_stUIMission.ConfirmMission();
}

void RmlNpcMis_OnComplete() {
	g_stUIMission.ConfirmMission();
}

void RmlNpcMis_OnPrize(int index) {
	(void)index;
}

