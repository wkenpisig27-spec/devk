#pragma once

#include "uiglobalvar.h"
#include "uiform.h"
#include "uiformmgr.h"
#include "uilabel.h"
#include "uitextbutton.h"
#include "uiscroll.h"

namespace GUI {

class CBossTimerMgr : public CUIInterface {
public:
	CBossTimerMgr();
	~CBossTimerMgr();

	// Show/hide the form
	void ShowForm(bool bShow = true);
	void ToggleForm();
	bool IsFormVisible() const;
	
	// Data management - called when receiving data from server
	void ClearData();
	void AddBossEntry(int chaId, const char* name, int status, int remainingSeconds);
	void DataReady();
	
	// Update display
	void UpdateDisplay();
	
	// Request data from server
	void RequestData();
	
	// Scroll functions
	void ScrollUp();
	void ScrollDown();
	void SetScrollOffset(int offset);
	int GetMaxScroll() const;
	bool HandleMouseScroll(int nScroll);
	void OnScrollChange();
	void UpdateScrollBar();

	static const int MAX_VISIBLE_BOSSES = 10;

protected:
	virtual bool Init() override;
	virtual void CloseForm() override;
	virtual void FrameMove(DWORD dwTime) override;

private:
	CForm* frmBossTimer;
	
	// Header labels
	CLabelEx* labTitle;
	CLabelEx* labNameHeader;
	CLabelEx* labStatusHeader;
	CLabelEx* labTimeHeader;
	
	// Boss entry labels (3 columns)
	CLabelEx* labBossName[MAX_VISIBLE_BOSSES];
	CLabelEx* labBossStatus[MAX_VISIBLE_BOSSES];
	CLabelEx* labBossTime[MAX_VISIBLE_BOSSES];
	
	// Buttons
	CTextButton* btnClose;
	CTextButton* btnRefresh;
	CTextButton* btnScrollUp;
	CTextButton* btnScrollDown;
	
	// Status label
	CLabelEx* labLastUpdate;
	
	// Scroll management
	int m_nScrollOffset;
	CScroll* pScrollBar;
	
	// Boss data storage
	struct BossEntry {
		int chaId;
		std::string name;
		int status;        // 0 = Alive, 1 = Dead (respawning)
		int remaining;     // Remaining seconds until respawn
	};
	std::vector<BossEntry> m_BossData;
	DWORD m_dwLastUpdate;  // Tick when data was last received
	
	// Format time for display
	static std::string FormatTime(int seconds);
	
	// Truncate long names for display
	static std::string TruncateName(const std::string& name, size_t maxLen);
	
	// Mouse event handler
	static void _evtMouseButton(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey);
};

} // namespace GUI

// Global instance
extern GUI::CBossTimerMgr g_stUIBossTimer;
