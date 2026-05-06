#include "stdafx.h"
#include "uihelpinfoform.h"
#include "helpinfoset.h"
#include "npchelper.h"
#include "worldscene.h"
#include "gameapp.h"
#include "mapset.h"
#include "character.h"
#include "uiboxform.h"
#include "effectobj.h"
#include "AutoPathService.h"

using namespace std;
using namespace GUI;

bool CHelpInfoMgr::Init() {
	m_pFormMain = _FindForm("frmHelpchat");
	if (!m_pFormMain)
		return false;

	m_pMemoContent = dynamic_cast<CMemoEx*>(m_pFormMain->Find("memCtrl"));
	if (!m_pMemoContent)
		return false;
	m_pMemoContent->evtClickItem = _ItemClickEvent;
	m_pMemoContent->Refresh();

	return true;
}

void CHelpInfoMgr::ShowHelpInfo(bool show, const char* HelpTitle) {
	if (show) {
		NET_MISPAGE page;
		memset(&page, 0x0, sizeof(page));
		const char* HelpInfo = GetHelpInfo(HelpTitle);
		if (HelpInfo)
			strncpy(page.szDesp, GetHelpInfo(HelpTitle), ROLE_MAXNUM_DESPSIZE - 1);

		m_pMemoContent->Init();
		m_pMemoContent->SetMisPage(page);
		m_pMemoContent->SetIsShow(true);

		m_pFormMain->Show();
	} else
		m_pFormMain->Hide();
}

bool CHelpInfoMgr::IsShown() {
	return m_pFormMain->GetIsShow();
}

void CHelpInfoMgr::_ItemClickEvent(string strItem) {
	SAutoPathResult result = CAutoPathService::NavigateFromText(strItem);
	if (result.success) {
		return;
	}

	if (result.error == EAutoPathError::InBoatMode) {
		g_stUIBox.ShowMsgBox(nullptr, "No routing on sea.");
		return;
	}

	if (result.error == EAutoPathError::DifferentMap) {
		g_stUIBox.ShowMsgBox(nullptr, "Not on the same map.");
		return;
	}
}