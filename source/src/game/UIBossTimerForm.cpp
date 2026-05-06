#include "stdafx.h"
#include "UIBossTimerForm.h"
#include "UIBoxForm.h"
#include "PacketCmd.h"
#include "GameApp.h"
#include "UIGlobalVar.h"
#include "UIGuiData.h"

#include <algorithm>

using namespace std;

namespace GUI {

// Static mouse scroll handler for Form Manager
static bool _onBossTimerMouseScroll(int& nScroll) {
	return g_stUIBossTimer.HandleMouseScroll(nScroll);
}

// Static scroll bar change handler
static void _evtScrollChange(CGuiData* pSender) {
	g_stUIBossTimer.OnScrollChange();
}

CBossTimerMgr::CBossTimerMgr() 
	: frmBossTimer(nullptr)
	, labTitle(nullptr)
	, labNameHeader(nullptr)
	, labStatusHeader(nullptr)
	, labTimeHeader(nullptr)
	, btnClose(nullptr)
	, btnRefresh(nullptr)
	, btnScrollUp(nullptr)
	, btnScrollDown(nullptr)
	, labLastUpdate(nullptr)
	, m_nScrollOffset(0)
	, pScrollBar(nullptr)
	, m_dwLastUpdate(0)
{
	for (int i = 0; i < MAX_VISIBLE_BOSSES; ++i) {
		labBossName[i] = nullptr;
		labBossStatus[i] = nullptr;
		labBossTime[i] = nullptr;
	}
}

CBossTimerMgr::~CBossTimerMgr() {
}

bool CBossTimerMgr::Init() {
	frmBossTimer = CFormMgr::s_Mgr.Find("frmBossTimer");
	if (!frmBossTimer) {
		LG("gui", "frmBossTimer not found.\n");
		return false;
	}
	frmBossTimer->evtEntrustMouseEvent = _evtMouseButton;
	
	// Title label
	labTitle = dynamic_cast<CLabelEx*>(frmBossTimer->Find("labBossTitle"));
	if (!labTitle) {
		LG("gui", "frmBossTimer:labBossTitle not found.\n");
	}
	
	// Header labels (3 columns)
	labNameHeader = dynamic_cast<CLabelEx*>(frmBossTimer->Find("labBossNameHeader"));
	labStatusHeader = dynamic_cast<CLabelEx*>(frmBossTimer->Find("labBossStatusHeader"));
	labTimeHeader = dynamic_cast<CLabelEx*>(frmBossTimer->Find("labBossTimeHeader"));
	
	// Boss entry labels (3 columns: Name, Status, Time)
	char szName[32] = {0};
	for (int i = 0; i < MAX_VISIBLE_BOSSES; ++i) {
		sprintf(szName, "labBossName%d", i + 1);
		labBossName[i] = dynamic_cast<CLabelEx*>(frmBossTimer->Find(szName));
		if (!labBossName[i]) {
			LG("gui", "frmBossTimer:%s not found.\n", szName);
		}
		
		sprintf(szName, "labBossStatus%d", i + 1);
		labBossStatus[i] = dynamic_cast<CLabelEx*>(frmBossTimer->Find(szName));
		if (!labBossStatus[i]) {
			LG("gui", "frmBossTimer:%s not found.\n", szName);
		}
		
		sprintf(szName, "labBossTime%d", i + 1);
		labBossTime[i] = dynamic_cast<CLabelEx*>(frmBossTimer->Find(szName));
		if (!labBossTime[i]) {
			LG("gui", "frmBossTimer:%s not found.\n", szName);
		}
	}
	
	// Close button - handled via evtEntrustMouseEvent
	btnClose = dynamic_cast<CTextButton*>(frmBossTimer->Find("btnBossTimerClose"));
	
	// Refresh button - handled via evtEntrustMouseEvent  
	btnRefresh = dynamic_cast<CTextButton*>(frmBossTimer->Find("btnBossTimerRefresh"));
	
	// Last update label
	labLastUpdate = dynamic_cast<CLabelEx*>(frmBossTimer->Find("labBossLastUpdate"));
	
	// Scrollbar
	pScrollBar = dynamic_cast<CScroll*>(frmBossTimer->Find("scrBossTimer"));
	if (pScrollBar) {
		pScrollBar->evtChange = _evtScrollChange;
		pScrollBar->SetRange(0, 0);  // Will be updated when data arrives
		pScrollBar->SetPageNum(1);
	}
	
	// Register mouse wheel scroll handler
	CFormMgr::s_Mgr.AddMouseScrollEvent(_onBossTimerMouseScroll);
	
	return true;
}

void CBossTimerMgr::CloseForm() {
	if (frmBossTimer) {
		frmBossTimer->SetIsShow(false);
	}
}

void CBossTimerMgr::ShowForm(bool bShow) {
	if (frmBossTimer) {
		frmBossTimer->SetIsShow(bShow);
		if (bShow) {
			// Request fresh data when showing
			RequestData();
		}
	}
}

void CBossTimerMgr::ToggleForm() {
	if (frmBossTimer) {
		ShowForm(!frmBossTimer->GetIsShow());
	}
}

bool CBossTimerMgr::IsFormVisible() const {
	return frmBossTimer && frmBossTimer->GetIsShow();
}

void CBossTimerMgr::ClearData() {
	m_BossData.clear();
}

void CBossTimerMgr::AddBossEntry(int chaId, const char* name, int status, int remainingSeconds) {
	BossEntry entry;
	entry.chaId = chaId;
	entry.name = name ? name : "";
	entry.status = status;
	entry.remaining = remainingSeconds;
	m_BossData.push_back(entry);
}

void CBossTimerMgr::DataReady() {
	m_dwLastUpdate = CGameApp::GetCurTick();
	UpdateScrollBar();
	UpdateDisplay();
}

std::string CBossTimerMgr::FormatTime(int seconds) {
	if (seconds <= 0) {
		return "Alive";
	}
	
	int hours = seconds / 3600;
	int mins = (seconds % 3600) / 60;
	int secs = seconds % 60;
	
	char buf[64];
	if (hours > 0) {
		sprintf(buf, "%dh %02dm %02ds", hours, mins, secs);
	} else if (mins > 0) {
		sprintf(buf, "%dm %02ds", mins, secs);
	} else {
		sprintf(buf, "%ds", secs);
	}
	return buf;
}

std::string CBossTimerMgr::TruncateName(const std::string& name, size_t maxLen) {
	if (name.length() <= maxLen) {
		return name;
	}
	if (maxLen <= 3) {
		return name.substr(0, maxLen);
	}
	return name.substr(0, maxLen - 3) + "...";
}

void CBossTimerMgr::UpdateDisplay() {
	if (!frmBossTimer || !frmBossTimer->GetIsShow()) {
		return;
	}
	
	// Calculate elapsed time since last update
	DWORD elapsed = 0;
	if (m_dwLastUpdate > 0) {
		elapsed = (CGameApp::GetCurTick() - m_dwLastUpdate) / 1000;
	}
	
	// Sort data: dead bosses first (by remaining time), then alive bosses
	std::vector<BossEntry> sortedData = m_BossData;
	std::sort(sortedData.begin(), sortedData.end(), [](const BossEntry& a, const BossEntry& b) {
		if (a.status == 1 && b.status == 0) return true;
		if (a.status == 0 && b.status == 1) return false;
		if (a.status == 1 && b.status == 1) {
			return a.remaining < b.remaining;
		}
		return a.name < b.name;
	});
	
	// Clamp scroll offset
	int maxScroll = GetMaxScroll();
	if (m_nScrollOffset > maxScroll) m_nScrollOffset = maxScroll;
	if (m_nScrollOffset < 0) m_nScrollOffset = 0;
	
	// Update labels with scroll offset (3 columns: Name, Status, Time)
	for (int i = 0; i < MAX_VISIBLE_BOSSES; ++i) {
		if (!labBossName[i] || !labBossStatus[i] || !labBossTime[i]) continue;
		
		int dataIndex = i + m_nScrollOffset;
		if (dataIndex < (int)sortedData.size()) {
			const BossEntry& entry = sortedData[dataIndex];
			
			labBossName[i]->SetIsShow(true);
			labBossStatus[i]->SetIsShow(true);
			labBossTime[i]->SetIsShow(true);
			labBossName[i]->SetCaption(TruncateName(entry.name, 20).c_str());
			labBossName[i]->SetTextColor(0xFF000000);  // Black
			
			if (entry.status == 0) {
				// Alive
				labBossStatus[i]->SetCaption("Alive");
				labBossStatus[i]->SetTextColor(0xFF006600);  // Dark green
				labBossTime[i]->SetCaption("-");
				labBossTime[i]->SetTextColor(0xFF000000);    // Black
			} else {
				// Dead - calculate remaining time
				int adjustedRemaining = entry.remaining - (int)elapsed;
				if (adjustedRemaining < 0) adjustedRemaining = 0;
				
				labBossStatus[i]->SetCaption("Dead");
				labBossStatus[i]->SetTextColor(0xFFCC0000);  // Dark red
				
				if (adjustedRemaining > 0) {
					labBossTime[i]->SetCaption(FormatTime(adjustedRemaining).c_str());
					labBossTime[i]->SetTextColor(0xFFCC0000);  // Dark red
				} else {
					labBossTime[i]->SetCaption("Spawning...");
					labBossTime[i]->SetTextColor(0xFF996600);  // Dark orange
				}
			}
		} else {
			labBossName[i]->SetIsShow(false);
			labBossStatus[i]->SetIsShow(false);
			labBossTime[i]->SetIsShow(false);
		}
	}
	
	// Update last update label
	if (labLastUpdate) {
		if (m_dwLastUpdate > 0) {
			DWORD ago = elapsed;
			char buf[64];
			if (ago < 2) {
				sprintf(buf, "Just updated");
			} else if (ago < 60) {
				sprintf(buf, "Updated %ds ago", ago);
			} else {
				sprintf(buf, "Updated %dm ago", ago / 60);
			}
			labLastUpdate->SetCaption(buf);
		} else {
			labLastUpdate->SetCaption("No data");
		}
	}
}

void CBossTimerMgr::FrameMove(DWORD dwTime) {
	// Update display every second if visible
	static DWORD lastUpdate = 0;
	if (dwTime - lastUpdate >= 1000) {
		lastUpdate = dwTime;
		if (IsFormVisible()) {
			UpdateDisplay();
		}
	}
}

void CBossTimerMgr::RequestData() {
	// Send request to server for boss timer data
	CS_BossTimerRequest();
}

void CBossTimerMgr::ScrollUp() {
	if (m_nScrollOffset > 0) {
		m_nScrollOffset--;
		UpdateDisplay();
	}
}

void CBossTimerMgr::ScrollDown() {
	int maxScroll = GetMaxScroll();
	if (m_nScrollOffset < maxScroll) {
		m_nScrollOffset++;
		UpdateDisplay();
	}
}

void CBossTimerMgr::SetScrollOffset(int offset) {
	int maxScroll = GetMaxScroll();
	m_nScrollOffset = offset;
	if (m_nScrollOffset < 0) m_nScrollOffset = 0;
	if (m_nScrollOffset > maxScroll) m_nScrollOffset = maxScroll;
	UpdateDisplay();
}

int CBossTimerMgr::GetMaxScroll() const {
	int totalItems = (int)m_BossData.size();
	int maxScroll = totalItems - MAX_VISIBLE_BOSSES;
	return maxScroll > 0 ? maxScroll : 0;
}

void CBossTimerMgr::_evtMouseButton(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	string name = pSender->GetName();
	
	if (name == "btnBossTimerClose") {
		g_stUIBossTimer.CloseForm();
	} else if (name == "btnBossTimerRefresh") {
		g_stUIBossTimer.RequestData();
	}
}

bool CBossTimerMgr::HandleMouseScroll(int nScroll) {
	// Only handle if form is visible
	if (!frmBossTimer || !frmBossTimer->GetIsShow())
		return false;
	
	// Check if mouse is over the boss timer form
	if (!frmBossTimer->InRect(CGuiData::GetMouseX(), CGuiData::GetMouseY()))
		return false;
	
	// Only scroll if there's something to scroll
	if (GetMaxScroll() <= 0)
		return false;
	
	// If scrollbar exists, delegate to it (handles mouse wheel)
	if (pScrollBar) {
		return pScrollBar->MouseScroll(nScroll);
	}
	
	// Otherwise handle manually
	if (nScroll < 0) {
		ScrollDown();
	} else if (nScroll > 0) {
		ScrollUp();
	}
	
	return true;  // We handled the scroll
}

void CBossTimerMgr::OnScrollChange() {
	if (pScrollBar) {
		m_nScrollOffset = (int)pScrollBar->GetStep().GetPosition();
		UpdateDisplay();
	}
}

void CBossTimerMgr::UpdateScrollBar() {
	if (!pScrollBar)
		return;
	
	int maxScroll = GetMaxScroll();
	if (maxScroll > 0) {
		pScrollBar->SetRange(0, (float)maxScroll);
		pScrollBar->GetStep().SetPosition((float)m_nScrollOffset);
		pScrollBar->SetIsShow(true);
	} else {
		pScrollBar->SetRange(0, 0);
		pScrollBar->SetIsShow(false);
	}
}

} // namespace GUI

// Global instance
GUI::CBossTimerMgr g_stUIBossTimer;
