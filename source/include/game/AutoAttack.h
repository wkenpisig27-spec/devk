//----------------------------------------------------------------------
// 名称:自动攻击
// 作者:lh 2004-06-15
// 用途:用于船到达攻击时间自动攻击，并且尽可能不影响玩家的移动
//----------------------------------------------------------------------

#pragma once

class CCharacter;
class CSkillRecord;
class CMouseDown;
class CIsStart;

class CAutoAttack {
public:
	CAutoAttack(CMouseDown* pMouseDown);
	void Reset();

	void FrameMove();
	void Cancel() { _eStyle = eNone; }

	void SetIsStart(bool v) { _IsStart = v; }
	bool GetIsStart() { return _IsStart; }

	// Toggle auto-attack mode
	void ToggleAutoAttack();
	void SetToggleEnabled(bool v) { _bToggleEnabled = v; }
	bool IsToggleEnabled() const { return _bToggleEnabled; }
	void FrameMoveToggle();

	// Auto-target: automatically select nearest monster when current target dies
	void SetAutoTarget(bool v) { _bAutoTarget = v; }
	bool IsAutoTarget() const { return _bAutoTarget; }

	// Melee fallback: allow/disallow the inborn melee attack in the fallback path
	void SetMeleeEnabled(bool v) { _bMeleeEnabled = v; }
	bool IsMeleeEnabled() const { return _bMeleeEnabled; }

	// Target filter: if non-empty, only monsters whose name matches this string are
	// considered valid auto-targets.  Empty string = any monster.
	void SetTargetFilter(const char* name) {
		if (name) strncpy(_szTargetFilter, name, sizeof(_szTargetFilter) - 1);
		else _szTargetFilter[0] = '\0';
		_szTargetFilter[sizeof(_szTargetFilter) - 1] = '\0';
	}
	const char* GetTargetFilter() const { return _szTargetFilter; }

	// 自动攻击
	bool AttackStart(CCharacter* pMain, CSkillRecord* pSkill, CCharacter* pCha);
	bool AttackStart(CCharacter* pMain, CSkillRecord* pSkill, int nScrX, int nScrY);
	bool AttackMoveTo(int nScrX, int nScrY);

	// 自动跟随
	bool Follow(CCharacter* pMain, CCharacter* pTarget);

private:
	void _CaleDist();
	CCharacter* _FindNearestEnemy(CCharacter* pMain); // Scan scene for closest valid monster

private:
	enum eStyle {
		eNone,
		eFollow,
		eAttack,
	};

	eStyle _eStyle;
	bool _IsStart;

	bool _bToggleEnabled;
	bool _bAutoTarget;           // auto-select nearest enemy when current target becomes invalid
	bool _bMeleeEnabled;         // whether melee fallback is allowed (default true)
	char _szTargetFilter[64];    // only target monsters with this name (empty = any)
	DWORD _slotNextCheck[12]; // Per topBar slot predicted next-cast time (one per MAX_FAST_COL slot)
	DWORD _dwMeleeNextCheck;  // Melee fallback predicted next-cast time

	CCharacter* _pTarget;
	CCharacter* _pMain;
	int _nAttackX, _nAttackY;
	CSkillRecord* _pSkill;

	CMouseDown* _pMouseDown;

	bool _IsMove;
	int _nMoveX, _nMoveY;

	int _nTotalDis;
};

class CIsStart {
public:
	CIsStart(CAutoAttack* pAuto) : _pAuto(pAuto) { _pAuto->SetIsStart(false); }
	~CIsStart() {
		if (!_pAuto->GetIsStart())
			_pAuto->Cancel();
	}

private:
	CAutoAttack* _pAuto;
};
