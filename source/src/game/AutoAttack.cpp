#include "stdafx.h"
#include "autoattack.h"
#include "isskilluse.h"
#include "gameapp.h"
#include "skillrecord.h"
#include "mousedown.h"
#include "character.h"
#include "uistartform.h"
#include "uiminimapform.h"
#include "scene.h"
#include "uifastcommand.h"
#include "uiskillcommand.h"
#include "uibankform.h"
#include "UITradeForm.h"
#include "UINpcTradeForm.h"

#include "stmove.h"
#include "stattack.h"
#include "findpath.h"
#include "JobType.h"

extern bool g_bDisableMeleeForCasters;

CAutoAttack::CAutoAttack(CMouseDown* pMouseDown)
	: _pMouseDown(pMouseDown) {
	Reset();
}

void CAutoAttack::Reset() {
	_pTarget = nullptr;

	_pMain = nullptr;
	_pSkill = nullptr;
	_IsMove = false;

	_eStyle = eNone;
	_IsStart = false;

	_bToggleEnabled = false;
	_bAutoTarget = false;
	_bMeleeEnabled = true;
	_szTargetFilter[0] = '\0';
	memset(_slotNextCheck, 0, sizeof(_slotNextCheck));
	_dwMeleeNextCheck = 0;
}

bool CAutoAttack::AttackStart(CCharacter* pMain, CSkillRecord* pSkill, CCharacter* pCha) {
	if (!pMain || !pSkill || !pCha) {
		_eStyle = eNone;
		return false;
	}

	// Block melee (inborn) auto-attacks for caster classes
	if (g_bDisableMeleeForCasters && pSkill->chType == enumSKILL_INBORN) {
		long lJob = pMain->getGameAttr()->get(ATTR_JOB);
		if (lJob == JOB_TYPE_SHENGZHIZHE  ||  // Cleric
			lJob == JOB_TYPE_FENGYINSHI    ||  // Seal Master
			lJob == JOB_TYPE_HANGHAISHI)        // Voyager
		{
			_eStyle = eNone;
			return false;
		}
	}

	if (!g_SkillUse.IsUse(pSkill, pMain, pCha)) {
		g_pGameApp->SysInfo("%s", g_SkillUse.GetError());
		return false;
	}

	g_stUIStart.SysLabel(RES_STRING(CL_LANGUAGE_MATCH_4), g_stUIMap.IsPKSilver() ? "??????" : pCha->getName());

	_pMain = pMain;
	_pSkill = pSkill;
	_pTarget = pCha;
	_IsMove = false;
	_eStyle = eAttack;

	_CaleDist();

	_IsStart = true;
	return true;
}

bool CAutoAttack::AttackStart(CCharacter* pMain, CSkillRecord* pSkill, int nScrX, int nScrY) {
	if (!pMain || !pSkill) {
		_eStyle = eNone;
		return false;
	}

	if (!g_SkillUse.IsUse(pSkill, pMain, nullptr)) {
		g_pGameApp->SysInfo("%s", g_SkillUse.GetError());
		return false;
	}

	g_stUIStart.SysLabel(RES_STRING(CL_LANGUAGE_MATCH_5), nScrX / 100, nScrY / 100);

	_pTarget = nullptr;

	_eStyle = eAttack;
	_pMain = pMain;
	_pSkill = pSkill;
	_nAttackX = nScrX;
	_nAttackY = nScrY;
	_IsMove = false;

	_CaleDist();

	_IsStart = true;
	return true;
}

void CAutoAttack::_CaleDist() {
	if (_pSkill->GetDistance() > 0) {
		_nTotalDis = _pMain->GetDefaultChaInfo()->sRadii + _pSkill->GetDistance();
		if (_pTarget)
			_nTotalDis += _pTarget->GetDefaultChaInfo()->sRadii;
	} else {
		_eStyle = eNone;
	}
}

void CAutoAttack::FrameMove() {
	switch (_eStyle) {
	case eAttack: {
		if (!_pSkill->IsAttackTime(CGameApp::GetCurTick()))
			return;

		static DWORD time = 0;
		if (CGameApp::GetCurTick() < time)
			return;

		time = CGameApp::GetCurTick() + 500;
		if (!g_SkillUse.IsUse(_pSkill, _pMain, _pTarget)) {
			_eStyle = eNone;

			if (_IsMove) {
				_pMouseDown->ActMove(_pMain, _nMoveX, _nMoveY, true, false, false);
			}
			return;
		}

		if (_IsMove) {
			if (GetDistance(_nMoveX, _nMoveY, _pMain->GetCurX(), _pMain->GetCurY()) < 50) {
				_IsMove = false;
			}

			if (CAttackState::GetLastTime() > CServerMoveState::GetLastTime()) {
				_pMouseDown->ActMove(_pMain, _nMoveX, _nMoveY, false, false, false);
				return;
			}
		}

		if (_pTarget) {
			if (!_pTarget->IsValid() || !_pTarget->IsEnabled() || _pTarget->IsHide()) {
				_eStyle = eNone;
			} else {
				_pMouseDown->ActAttackCha(_pMain, _pSkill, _pTarget, false, false, false);
			}
		} else {
			if (GetDistance(_pMain->GetCurX(), _pMain->GetCurY(), _nAttackX, _nAttackY) > _nTotalDis) {
				bool IsWalkLine = false;
				if (!g_cFindPath.GetPathFindingState() /*g_cFindPath.Find( _pMain->GetScene(), _pMain, _pMain->GetCurX(), _pMain->GetCurY(), _nAttackX, _nAttackY, IsWalkLine )*/) {
					_eStyle = eNone;
					return;
				}
			}

			_pMouseDown->ActAttackArea(_pMain, _pSkill, _nAttackX, _nAttackY, false);
		}
	} break;
	case eFollow: {
		// Safety check: ensure both pointers are valid before dereferencing
		if (!_pMain || !_pTarget || !_pMain->IsValid() || !_pMain->IsEnabled() || !_pTarget->IsValid() || !_pTarget->IsPlayer()) {
			_eStyle = eNone;
		} else {
			static DWORD time = 0;
			if (CGameApp::GetCurTick() > time || (_pMain->GetIsArrive() && _pMain->GetActor()->IsEmpty())) {
				time = CGameApp::GetCurTick() + 1000;

				const int dis = 200;
				if (GetDistance(_pMain->GetServerX(), _pMain->GetServerY(), _pTarget->GetServerX(), _pTarget->GetServerY()) > dis) {
					int x, y;
					GetDistancePos(_pTarget->GetServerX(), _pTarget->GetServerY(), _pMain->GetServerX(), _pMain->GetServerY(), dis - 100, x, y);
					_pMouseDown->ActMove(_pMain, x, y, false, false, false);
				}
			}
		}
	} break;
	default:
		g_stUIStart.SysHide();
		break;
	}
}

bool CAutoAttack::AttackMoveTo(int nScrX, int nScrY) {
	if (_eStyle != eAttack)
		return false;

	// if( !_pTarget && GetDistance( _nAttackX, _nAttackY, nScrX, nScrY ) > _nTotalDis )
	//{
	//	_eStyle = eNone;
	//	return false;
	// }

	_IsMove = true;
	_nMoveX = nScrX;
	_nMoveY = nScrY;

	_pMouseDown->ActMove(_pMain, _nMoveX, _nMoveY, true, false, false);

	_IsStart = true;
	return true;
}

bool CAutoAttack::Follow(CCharacter* pMain, CCharacter* pTarget) {
	_eStyle = eNone;
	if (!pMain || !pTarget)
		return false;

	if (!pMain->IsValid() || !pTarget->IsValid())
		return false;

	if (!pTarget->IsPlayer())
		return false;

	if (pMain == pTarget)
		return false;

	g_stUIStart.SysLabel(RES_STRING(CL_LANGUAGE_MATCH_6), g_stUIMap.IsPKSilver() ? "??????" : pTarget->getName());

	_pTarget = pTarget;
	_pMain = pMain;
	_eStyle = eFollow;

	_IsStart = true;
	return true;
}

void CAutoAttack::ToggleAutoAttack() {
	_bToggleEnabled = !_bToggleEnabled;
	g_pGameApp->SysInfo(_bToggleEnabled ? "Auto-Attack: ON" : "Auto-Attack: OFF");
}

// Scan the scene for the closest valid, living monster that is not hidden.
// Used by FrameMoveToggle() when _bAutoTarget is enabled and no target is selected.
CCharacter* CAutoAttack::_FindNearestEnemy(CCharacter* pMain) {
	CGameScene* pScene = CGameApp::GetCurScene();
	if (!pScene)
		return nullptr;

	CCharacter* pNearest = nullptr;
	int nMinDist = INT_MAX;
	const int nCnt = (int)pScene->GetChaCnt();
	for (int i = 0; i < nCnt; ++i) {
		CCharacter* pCha = &pScene->_pChaArray[i];
		if (!pCha->IsValid() || !pCha->IsEnabled() || pCha->IsHide())
			continue;
		if (!pCha->IsMonster())
			continue; // Only target monsters, not players or NPCs
		if (pCha == pMain)
			continue;
		// Respect the name filter — skip monsters that don't match
		if (_szTargetFilter[0] != '\0' && _stricmp(pCha->getName(), _szTargetFilter) != 0)
			continue;
		const int dist = pMain->DistanceFrom(pCha);
		if (dist < nMinDist) {
			nMinDist = dist;
			pNearest = pCha;
		}
	}
	return pNearest;
}

void CAutoAttack::FrameMoveToggle() {
	if (!_bToggleEnabled)
		return;

	CCharacter* pMain = CGameScene::GetMainCha();
	if (!pMain || !pMain->IsValid() || !pMain->IsEnabled())
		return;

	// --- Dead target detection ---
	// GetTarget() returns nullptr for dead/invalid targets (checked by ID), but
	// the character may still exist as a corpse with IsEnabled()==false.  Clear it
	// so the auto-target logic below gets a chance to pick a new enemy.
	CCharacter* pTarget = g_stUIStart.GetTarget();
	if (pTarget && !pTarget->IsEnabled()) {
		g_stUIStart.RemoveTarget();
		pTarget = nullptr;
	}

	// --- Auto-target nearest monster ---
	// When _bAutoTarget is on and we have no valid target, scan the scene for the
	// nearest living monster and select it so combat continues uninterrupted.
	if (!pTarget && _bAutoTarget) {
		pTarget = _FindNearestEnemy(pMain);
		if (pTarget)
			g_stUIStart.SetTargetInfo(pTarget);
	}

	if (!pTarget || pTarget->IsHide())
		return;

	DWORD dwNow = CGameApp::GetCurTick();

	// --- SP management ---
	// If current SP is below 20 % of max, skip skill casting and fall through to
	// melee only.  This prevents draining SP to zero and silently failing skills.
	const long nSP = pMain->getGameAttr()->get(ATTR_SP);
	const long nMaxSP = pMain->getGameAttr()->get(ATTR_MXSP);
	const bool bSkillsAllowed = (nMaxSP <= 0) || (nSP * 100 / nMaxSP >= 20);

	if (bSkillsAllowed) {
		// Try topBar skill slots in priority order (nTag 24..35 = slots 0..11)
		int totalSlots = CFastCommand::GetFastCommandCount();
		for (int tag = (int)(MAX_FAST_COL * 2); tag < (int)(MAX_FAST_COL * 3); ++tag) {
			int slotIndex = tag - (int)(MAX_FAST_COL * 2); // 0..11

			// Skip until per-slot predicted cooldown expires
			if (dwNow < _slotNextCheck[slotIndex])
				continue;

			// Locate the topBar slot with this semantic nTag
			CFastCommand* pFast = nullptr;
			for (int vi = 0; vi < totalSlots; ++vi) {
				CFastCommand* p = CFastCommand::GetFastCommand(vi);
				if (p && p->topBar && p->nTag == tag) {
					pFast = p;
					break;
				}
			}
			if (!pFast)
				continue;

			CCommandObj* pCmd = pFast->GetCommand();
			if (!pCmd)
				continue;

			CSkillCommand* pSkillCmd = dynamic_cast<CSkillCommand*>(pCmd);
			if (!pSkillCmd)
				continue;

			CSkillRecord* pSkill = pSkillCmd->GetSkillRecord();
			if (!pSkill)
				continue;

			if (!pSkill->IsAttackTime(dwNow))
				continue;

			// Validate skill usability (SP, state, level) without target-type check.
			// IsUse() would reject self-buff skills (e.g. Berserk/Stealth) when pTarget is enemy.
			if (!g_SkillUse.IsValid(pSkill, pMain))
				continue;

			// Replicate CSkillCommand::IsAtOnce() using public CSkillRecord fields.
			// IsAtOnce() is protected so we can't call it from here directly.
			bool isAtOnce = pSkill->GetIsActive() || pSkill->GetDistance() <= 0 ||
			                pSkill->chApplyTarget == enumSKILL_TYPE_SELF;

			if (isAtOnce) {
				// Self-targeting or zero-distance skill: directly invoke UseCommand()
				// → CAttackState targeting self → SetCommand(this) → StartCommand() → AniClock
				pSkillCmd->UseCommand();
			} else {
				// Enemy-targeted skill: prime _pCommand so ActAttackCha's CAttackState
				// picks it up via GetReadyCommand() → StartCommand() → AniClock
				CCommandObj::SetReadyCommand(pSkillCmd);
				_pMouseDown->ActAttackCha(pMain, pSkill, pTarget, false, false, false);
			}

			// Record when this slot's cooldown is predicted to expire so we don't poll it
			int fireSpeed = pSkill->GetFireSpeed();
			_slotNextCheck[slotIndex] = dwNow + (fireSpeed > 0 ? (DWORD)fireSpeed : 500);
			return;
		}
	}

	// --- Melee fallback ---
	// Pause if bank or trade windows are open — interacting with them should take
	// priority over auto-attacking.  Skills are already blocked via IsAllowUse()
	// inside UseCommand(); the melee path needs its own explicit check.
	if (g_stUIBank.GetBankGoodsGrid()->GetForm()->GetIsShow())
		return;
	if (g_stUITrade.IsTrading())
		return;
	if (g_stUINpcTrade.GetIsShow())
		return;

	// Respect the per-player melee opt-out flag (casters, ranged builds, etc.)
	if (!_bMeleeEnabled)
		return;

	if (dwNow < _dwMeleeNextCheck)
		return;

	CSkillRecord* pInborn = CCharacter::GetDefaultSkillInfo();
	if (!pInborn || !pInborn->IsAttackTime(dwNow))
		return;

	if (g_SkillUse.IsUse(pInborn, pMain, pTarget)) {
		_pMouseDown->ActAttackCha(pMain, pInborn, pTarget, false, false, false);
		int fireSpeed = pInborn->GetFireSpeed();
		_dwMeleeNextCheck = dwNow + (fireSpeed > 0 ? (DWORD)fireSpeed : 500);
	}
}
