#pragma once
#include "UIGlobalVar.h"

extern int g_BattlePoints;

namespace GUI {
// Player attributes — Notice Rml chrome; frmState remains a silent data host.
class CStateMgr : public CUIInterface {
public:
	void RefreshStateFrm();
	void UpdateBattlePointDisplay();
	long BattlePoints();

	void ShowCharacterUi();
	void HideCharacterUi();
	void ToggleCharacterUi();
	bool IsCharacterUiVisible() const;

protected:
	virtual bool Init();
	virtual void End();
	virtual void FrameMove(DWORD dwTime);

private:
	static void _evtMainShow(CGuiData* pSender);
	static void MainMouseDown(CGuiData* pSender, int x, int y, DWORD key);
	static void MainMouseDownContinue(CGuiData* pSender);
	static bool _OnCharacterHotKey(char& key, int& control);
	static void AllocateAttribute(int attributeType);

private:
	// frmState host widgets (kept for Init wiring; UI is Rml)
	CForm* frmState;
	CLabelEx* labName;
	CLabelEx* labGuildName;
	CLabelEx* labStateLevel;
	CLabelEx* labStatePoint;
	CLabelEx* labSkillPoint;
	CLabelEx* labJobShow;
	CLabelEx* labFameShow;
	CLabelEx* labBattlepoints;

	CLabelEx* labStrshow;
	CLabelEx* labDexshow;
	CLabelEx* labAgishow;
	CLabelEx* labConshow;
	CLabelEx* labStashow;
	CLabelEx* labLukshow;
	CLabelEx* labSailLevel;
	CLabelEx* labSailEXP;

	CLabelEx* labMinAtackShow;
	CLabelEx* labMaxAtackShow;
	CLabelEx* labFleeShow;
	CLabelEx* labAspeedShow;
	CLabelEx* labMspeedShow;
	CLabelEx* labHitShow;
	CLabelEx* labDefenceShow;
	CLabelEx* labPhysDefineShow;

	CTextButton* btnStr;
	CTextButton* btnAgi;
	CTextButton* btnCon;
	CTextButton* btnSta;
	CTextButton* btnDex;

	CLabelEx* labStateEXP;
	CLabelEx* labStateHP;
	CLabelEx* labStateSP;

	CLabelEx* labFameSho;
	CLabelEx* labStateName;
	CLabelEx* labStateJob;
};

} // namespace GUI
