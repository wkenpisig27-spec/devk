#include "stdafx.h"
#include "UIAutoAttackSettingsForm.h"
#include "AutoAttack.h"
#include "worldscene.h"
#include "gameapp.h"
#include "scene.h"
#include "character.h"

#include <set>
#include <string>

using namespace GUI;

// ---------------------------------------------------------------------------
// Note: g_stUIAutoAttackSettings is defined in UIGlobalVar.cpp along with all
// other UI manager singletons.  The extern declaration lives in the header.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static CAutoAttack* _GetAutoAttack() {
	CWorldScene* pScene = dynamic_cast<CWorldScene*>(g_pGameApp->GetCurScene());
	if (!pScene) return nullptr;
	return pScene->GetMouseDown().GetAutoAttack();
}

// ---------------------------------------------------------------------------
// CUIInterface overrides
// ---------------------------------------------------------------------------
bool CAutoAttackSettingsMgr::Init() {
	_frmAASettings = _FindForm("frmAASettings");
	if (!_frmAASettings)
		return Error("not found", "frmAASettings", "check autoattacksettings.clu");

	_frmAASettings->evtBeforeShow = _evtBeforeShow;

	_cbxAAEnabled = dynamic_cast<CCheckGroup*>(_frmAASettings->Find("cbxAAEnabled"));
	if (!_cbxAAEnabled)
		return Error("not found", "frmAASettings", "cbxAAEnabled");
	_cbxAAEnabled->evtSelectChange = _evtEnabledChange;

	_cbxAAAutoTarget = dynamic_cast<CCheckGroup*>(_frmAASettings->Find("cbxAAAutoTarget"));
	if (!_cbxAAAutoTarget)
		return Error("not found", "frmAASettings", "cbxAAAutoTarget");
	_cbxAAAutoTarget->evtSelectChange = _evtAutoTargetChange;

	_cbxAAMelee = dynamic_cast<CCheckGroup*>(_frmAASettings->Find("cbxAAMelee"));
	if (!_cbxAAMelee)
		return Error("not found", "frmAASettings", "cbxAAMelee");
	_cbxAAMelee->evtSelectChange = _evtMeleeChange;

	_cboAAFilter = dynamic_cast<CCombo*>(_frmAASettings->Find("cboAAFilter"));
	if (!_cboAAFilter)
		return Error("not found", "frmAASettings", "cboAAFilter");
	_cboAAFilter->evtSelectChange = _evtFilterChange;

	CTextButton* btnRefresh = dynamic_cast<CTextButton*>(_frmAASettings->Find("btnAARefresh"));
	if (btnRefresh)
		btnRefresh->evtMouseClick = _evtRefresh;

	return true;
}

void CAutoAttackSettingsMgr::End() {}

// ---------------------------------------------------------------------------
// Show / Refresh
// ---------------------------------------------------------------------------
void CAutoAttackSettingsMgr::Show() {
	if (!_frmAASettings) return;
	_SyncState();
	RefreshMonsterList();
	_frmAASettings->SetIsShow(true);
}

void CAutoAttackSettingsMgr::_SyncState() {
	CAutoAttack* pAA = _GetAutoAttack();
	if (!pAA || !_cbxAAEnabled) return;

	// Temporarily remove events so programmatic changes don't re-fire setters
	auto evtE  = _cbxAAEnabled->evtSelectChange;
	auto evtAT = _cbxAAAutoTarget->evtSelectChange;
	auto evtM  = _cbxAAMelee->evtSelectChange;
	_cbxAAEnabled->evtSelectChange    = nullptr;
	_cbxAAAutoTarget->evtSelectChange = nullptr;
	_cbxAAMelee->evtSelectChange      = nullptr;

	_cbxAAEnabled->SetActiveIndex   (pAA->IsToggleEnabled() ? 1 : 0);
	_cbxAAAutoTarget->SetActiveIndex(pAA->IsAutoTarget()    ? 1 : 0);
	_cbxAAMelee->SetActiveIndex     (pAA->IsMeleeEnabled()  ? 1 : 0);

	_cbxAAEnabled->evtSelectChange    = evtE;
	_cbxAAAutoTarget->evtSelectChange = evtAT;
	_cbxAAMelee->evtSelectChange      = evtM;
}

void CAutoAttackSettingsMgr::RefreshMonsterList() {
	if (!_cboAAFilter) return;

	CGameScene* pScene = CGameApp::GetCurScene();
	if (!pScene) return;
	CCharacter* pMain = CGameScene::GetMainCha();
	if (!pMain) return;

	// Save current filter selection so we can try to restore it
	const char* curText = _cboAAFilter->GetText();
	std::string savedSel = (curText && curText[0]) ? curText : "";

	// Collect unique monster names within SCAN_RADIUS
	std::set<std::string> names;
	const int cnt = (int)pScene->GetChaCnt();
	for (int i = 0; i < cnt; ++i) {
		CCharacter* p = &pScene->_pChaArray[i];
		if (!p->IsValid() || !p->IsEnabled() || p->IsHide()) continue;
		if (!p->IsMonster()) continue;
		if (pMain->DistanceFrom(p) > SCAN_RADIUS) continue;
		const char* name = p->getName();
		if (name && name[0]) names.insert(name);
	}

	// Temporarily disconnect the filter event so repopulation doesn't trigger it
	auto savedEvt = _cboAAFilter->evtSelectChange;
	_cboAAFilter->evtSelectChange = nullptr;

	_cboAAFilter->GetList()->Clear();
	_cboAAFilter->GetList()->Add("Any Monster");
	for (const auto& n : names)
		_cboAAFilter->GetList()->Add(n.c_str());

	// Restore prior selection if it's still in the list, else default to index 0
	int restoreIdx = 0;
	if (!savedSel.empty() && savedSel != "Any Monster") {
		int idx = 1;
		for (const auto& n : names) {
			if (n == savedSel) { restoreIdx = idx; break; }
			++idx;
		}
	}
	_cboAAFilter->GetList()->GetItems()->Select(restoreIdx);
	_cboAAFilter->evtSelectChange = savedEvt;
}

// ---------------------------------------------------------------------------
// Event handlers
// ---------------------------------------------------------------------------
void CAutoAttackSettingsMgr::_evtEnabledChange(CGuiData* pSender) {
	CCheckGroup* g = dynamic_cast<CCheckGroup*>(pSender);
	if (!g) return;
	CAutoAttack* pAA = _GetAutoAttack();
	if (!pAA) return;
	bool enabled = (g->GetActiveIndex() == 1);
	pAA->SetToggleEnabled(enabled);
	g_pGameApp->SysInfo(enabled ? "Auto-Attack: ON" : "Auto-Attack: OFF");
}

void CAutoAttackSettingsMgr::_evtAutoTargetChange(CGuiData* pSender) {
	CCheckGroup* g = dynamic_cast<CCheckGroup*>(pSender);
	if (!g) return;
	CAutoAttack* pAA = _GetAutoAttack();
	if (!pAA) return;
	bool v = (g->GetActiveIndex() == 1);
	pAA->SetAutoTarget(v);
	g_pGameApp->SysInfo(v ? "Auto-Target: ON" : "Auto-Target: OFF");
}

void CAutoAttackSettingsMgr::_evtMeleeChange(CGuiData* pSender) {
	CCheckGroup* g = dynamic_cast<CCheckGroup*>(pSender);
	if (!g) return;
	CAutoAttack* pAA = _GetAutoAttack();
	if (!pAA) return;
	bool v = (g->GetActiveIndex() == 1);
	pAA->SetMeleeEnabled(v);
	g_pGameApp->SysInfo(v ? "Melee: ON" : "Melee: OFF");
}

void CAutoAttackSettingsMgr::_evtFilterChange(CGuiData* pSender) {
	CCombo* c = dynamic_cast<CCombo*>(pSender);
	if (!c) return;
	CAutoAttack* pAA = _GetAutoAttack();
	if (!pAA) return;
	const char* txt = c->GetText();
	if (!txt || strcmp(txt, "Any Monster") == 0)
		pAA->SetTargetFilter("");
	else
		pAA->SetTargetFilter(txt);
}

void CAutoAttackSettingsMgr::_evtRefresh(CGuiData* pSender, int /*x*/, int /*y*/, DWORD /*key*/) {
	g_stUIAutoAttackSettings.RefreshMonsterList();
}

void CAutoAttackSettingsMgr::_evtBeforeShow(CForm* /*pForm*/, bool& bIsShow) {
	if (bIsShow)
		g_stUIAutoAttackSettings._SyncState();
}
