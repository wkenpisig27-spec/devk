#pragma once
#include "UIGlobalVar.h"
#include "UICheckBox.h"
#include "UICombo.h"
#include "UIList.h"

namespace GUI {

// ---------------------------------------------------------------------------
// CAutoAttackSettingsMgr
//
// Manages the frmAASettings "Auto-Attack Settings" window.
// Settings apply live as the player changes them — no Apply button.
// The Target Filter combo is repopulated from nearby monsters on Show().
// ---------------------------------------------------------------------------
class CAutoAttackSettingsMgr : public CUIInterface {
public:
	bool Init() override;
	void End() override;

	// Call to show the settings window.  Syncs current state and refreshes
	// the monster list before making the form visible.
	void Show();

	// Repopulate the Target Filter combo with unique monster names found
	// within SCAN_RADIUS world units of the main character.
	void RefreshMonsterList();

private:
	static constexpr int SCAN_RADIUS = 800;

	CForm*       _frmAASettings{};
	CCheckGroup* _cbxAAEnabled{};
	CCheckGroup* _cbxAAAutoTarget{};
	CCheckGroup* _cbxAAMelee{};
	CCombo*      _cboAAFilter{};

	// Sync checkbox group states to the current CAutoAttack settings.
	void _SyncState();

	// Event handlers — fire immediately on widget change (live-apply)
	static void _evtEnabledChange   (CGuiData* pSender);
	static void _evtAutoTargetChange(CGuiData* pSender);
	static void _evtMeleeChange     (CGuiData* pSender);
	static void _evtFilterChange    (CGuiData* pSender);
	static void _evtRefresh         (CGuiData* pSender, int x, int y, DWORD key);
	static void _evtBeforeShow      (CForm* pForm, bool& bIsShow);
};

} // namespace GUI

extern GUI::CAutoAttackSettingsMgr g_stUIAutoAttackSettings;
