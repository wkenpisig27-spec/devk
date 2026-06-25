# Auto-Attack System

> **Status:** Implemented (Phase 2 complete)  
> **Files:** `source/src/game/AutoAttack.cpp`, `source/include/game/AutoAttack.h`, `source/include/game/UICommand.h`

---

## Goal

Allow players to toggle a continuous auto-attack mode that:
- Automatically attacks a targeted enemy each frame cycle
- Prioritizes skills placed in the **topBar** (the clickable fast-bar above the skill bar) in slot order (left to right = highest to lowest priority)
- Falls back to the **inborn melee attack** when no topBar skill is available or off cooldown
- Respects skill cooldowns — a skill is only re-cast once its cooldown expires, not on a naive timer poll
- Produces identical visual feedback (cooldown fan animation, cooldown countdown) as if the player manually clicked the skill

---

## How to Use

1. Click the **Auto-Attack toggle button** (checkbox style) in the UI
   - First click: **ON** — begins auto-attack loop
   - Second click: **OFF** — stops
2. Target an enemy (required — auto-attack does nothing without a selected target)
3. Place skills you want auto-cast into the **topBar slots** in priority order (slot 1 = highest priority)
4. The system will cycle through them in order and cast the first available (not on cooldown) skill each pass

---

## Architecture

### Entry Point

`CAutoAttack::FrameMoveToggle()` — called every game frame when auto-attack is enabled.

### Execution Flow

```
FrameMoveToggle()
  │
  ├─ Guard: toggle enabled? target valid?
  │
  ├─ For each topBar slot (nTag 24..35, priority 0..11):
  │   ├─ Skip if _slotNextCheck[i] hasn't expired (client-side cooldown prediction)
  │   ├─ Locate CFastCommand with matching nTag
  │   ├─ Get CSkillCommand → CSkillRecord
  │   ├─ Skip if IsAttackTime() is false (server-confirmed cooldown)
  │   ├─ Skip if IsValid() fails (SP, state, level check — NOT target-type check)
  │   ├─ If self/instant skill (GetDistance()<=0 or SELF target):
  │   │     UseCommand() → CAttackState(self) → StartCommand() → AniClock ✓
  │   └─ If enemy-targeted skill:
  │         SetReadyCommand(pSkillCmd) + ActAttackCha() → CAttackState picks up command
  │         → StartCommand() → AniClock ✓
  │   └─ On cast: _slotNextCheck[i] = now + GetFireSpeed()   [then return]
  │
  └─ Fallback: inborn melee
      ├─ Skip if _dwMeleeNextCheck hasn't expired
      ├─ Skip if IsAttackTime() is false
      └─ ActAttackCha(inborn) → _dwMeleeNextCheck = now + fireSpeed
```

### Per-Slot Cooldown Prediction

Instead of polling every 500ms, each topBar slot has its own `_slotNextCheck[12]` timestamp:

- After casting slot `i`: `_slotNextCheck[i] = dwNow + pSkill->GetFireSpeed()`
- This is **client-side prediction** — avoids redundant checks mid-cooldown
- A second guard `pSkill->IsAttackTime(dwNow)` uses the **server-confirmed** cooldown time from `CSkillRecord::_dwAttackTime`
- Both layers must pass before a cast is attempted

### Skill Validation: `IsValid()` vs `IsUse()`

| Function | Checks | Used for |
|---|---|---|
| `IsValid(skill, self)` | SP cost, character state (`enumChaStateUseSkill`), skill level | Whether the player *can* use the skill at all |
| `IsUse(skill, self, target)` | Everything in `IsValid` + target type match (SELF/ENEMY/TEAM/etc.) | Whether the skill can be used *on a specific target* |

Auto-attack uses `IsValid()` — passing the enemy as target to `IsUse()` would permanently block self-buff skills (e.g. Berserk, Stealth) because their `chApplyTarget == enumSKILL_TYPE_SELF` requires `pSelf == pTarget`.

### Self vs Enemy Branching

`CSkillCommand::IsAtOnce()` is protected, so we replicate its logic using public `CSkillRecord` fields:

```cpp
bool isAtOnce = pSkill->GetIsActive()           // toggle skill (already active → cancel)
             || pSkill->GetDistance() <= 0       // zero-distance = instant/melee
             || pSkill->chApplyTarget == enumSKILL_TYPE_SELF; // self-buff
```

- **`isAtOnce = true`** → `UseCommand()` directly → creates `CAttackState` targeting self
- **`isAtOnce = false`** → `SetReadyCommand(pSkillCmd)` + `ActAttackCha()` → `CAttackState` picks up the command via `GetReadyCommand()`, calls `StartCommand()` → AniClock allocated once

### AniClock Pool Correctness

The old approach called `StartCommand()` manually every 500ms → each call allocated a new `CAniClock` pool slot while the old one was still marked active (`_bUpdate=true`) → pool exhaustion after `MAX_ANI_CLOCK` casts → no animation.

The fix: let the attack state machine call `StartCommand()` exactly once per cast, via `SetCommand(this)` stored in `CAttackState`. This is identical to how a manual button click works.

---

## Key Files

| File | Role |
|---|---|
| `source/src/game/AutoAttack.cpp` | Core: `FrameMoveToggle()`, `AttackStart()`, `Reset()` |
| `source/include/game/AutoAttack.h` | Class definition: `_slotNextCheck[12]`, `_dwMeleeNextCheck`, `_bToggleEnabled` |
| `source/include/game/UICommand.h` | Added `SetReadyCommand()` static helper |
| `source/src/game/UISkillCommand.cpp` | `UseCommand()`, `StartCommand()`, `IsAllowUse()` |
| `source/src/game/UIFastCommand.cpp` | `Exec()` — reference for how manual clicks work |
| `source/src/game/IsSkillUse.cpp` | `IsValid()` vs `IsUse()` implementation |
| `source/include/common/SkillRecord.h` | `GetFireSpeed()`, `IsAttackTime()`, `chApplyTarget` |

---

## What Has Been Achieved

- [x] Toggle button activates/deactivates auto-attack mode
- [x] TopBar skills cast in priority order (slot 1 → slot N)
- [x] Melee fallback when no topBar skill is ready
- [x] Per-slot cooldown prediction — no naive polling
- [x] AniClock fan animation works (no pool exhaustion)
- [x] Cooldown countdown numbers display correctly
- [x] Self-buff skills (Berserk, Stealth) correctly cast on self
- [x] Enemy-targeted skills (Illusion Slash, Shadow Slash, Poison Dart) correctly cast on target
- [x] Toggle-type active skills correctly cancelled when active
- [x] Dead target detection — corpse is deselected immediately so attacks don't spam the air
- [x] Auto-target nearest enemy — `chkAutoTarget` checkbox activates automatic monster selection when the current target dies or has no target; `SetAutoTarget()` / `GetAutoTarget()` Lua API also available
- [x] SP management — skill casting pauses when SP drops below 20% of max; melee fallback still fires; recovers automatically once SP regenerates
- [x] Pause on UI open — melee fallback skips when bank window, player trade, or NPC shop is open
- [x] Melee enable/disable — `SetMeleeEnabled(false)` suppresses the melee fallback for ranged/caster builds; controlled via Lua (`SetMeleeEnabled`, `GetMeleeEnabled`)

---

## Known Limitations

- Auto-attack does not auto-select a target — player must have a target selected
- No range check before attempting skills — the skill's attack state handles out-of-range internally (character moves into range first via `CTraceAttackState`), but auto-attack won't move the player to a target if they walk away
- Self-buff skills with prerequisite conditions (e.g. Stealth requiring no combat state) may be attempted and silently fail if the server rejects them — the client prediction will still set `_slotNextCheck` and wait the full cooldown before retrying

---

## Suggested Improvements & Future Features

### High Priority

#### 1. Auto-Target Nearest Enemy ✅ DONE
~~When the current target dies or becomes invalid, automatically select the nearest valid enemy.~~
Implemented: `_bAutoTarget` / `SetAutoTarget()` / `chkAutoTarget` UI checkbox. Scans `_pChaArray` for nearest `IsMonster()` character and calls `SetTargetInfo()`.

#### 2. Skill Condition Awareness
Some skills have prerequisites (e.g. Stealth requires not being in combat, some skills require a buff to be active). Currently we rely on the server to reject them and the client wastes a cooldown slot.
- Add optional per-slot condition callbacks or check `GetIsActive()` for prerequisite buffs before attempting
- At minimum: detect if the server never confirmed the cast (no `_dwAttackTime` update) and reduce retry penalty

#### 3. Dead Target Detection & Stop ✅ DONE
~~When the target dies, auto-attack should stop or wait for a new target rather than spamming attacks at a corpse.~~
Implemented: after `GetTarget()`, if `pTarget && !pTarget->IsEnabled()` → `RemoveTarget()` + set `pTarget = nullptr` so auto-target picks up.

### Medium Priority

#### 4. Configurable Skill Priority via Drag-and-Drop
The current system uses topBar slot position as priority. An explicit priority UI (drag skills up/down in a list) would be more flexible.
- Would require a new `CAutoAttackMgr` UI form with a priority list widget
- Persist priority order in user settings

#### 5. SP Management — Stop Casting if Low SP ✅ DONE
~~If the player's SP drops below a threshold, pause skill casting and fall back to melee only.~~
Implemented: `bSkillsAllowed = (nSP * 100 / nMaxSP >= 20)` guards the entire skill loop. Melee fallback continues until SP recovers.

#### 6. Pet/Fairy Skill Integration
Trigger pet/fairy skills in a similar priority loop.
- Would need to access the fairy's skill list via the fairy slot (not topBar) and add a separate cooldown tracking array

#### 7. Skill Rotation Mode
Instead of "cast the first available skill in priority order", support a **rotation mode** that cycles through all slots in a fixed sequence regardless of which is next available (waits for each in turn).
- Useful for classes with strict rotation-based optimal DPS
- Add a `_eSkillMode: enumPriority / enumRotation` enum to `CAutoAttack`

### Low Priority / Quality of Life

#### 8. Auto-Attack Status HUD
Show a small indicator on screen when auto-attack is active — e.g. a glowing border on the topBar, or a persistent status icon near the character.
- Avoids confusion about whether auto-attack is on or off

#### 9. Pause on UI Open ✅ DONE
~~Automatically pause auto-attack when trade windows, banks, or crafting UIs are open.~~
Implemented: `FrameMoveToggle()` checks `g_stUIBank`, `g_stUITrade.IsTrading()`, and `g_stUINpcTrade.GetIsShow()` before melee fallback and returns early if any are open.

#### 10. Configurable Melee Enable/Disable ✅ DONE
~~Let the player opt out of melee fallback (useful for ranged/caster builds that don't want to run into melee range).~~
Implemented: `_bMeleeEnabled` flag, `SetMeleeEnabled()` / `IsMeleeEnabled()` accessors, and Lua API `SetMeleeEnabled(bool)` / `GetMeleeEnabled()`. Melee fallback is skipped when disabled.

---

## Technical Notes for Future Work

- `MAX_FAST_COL = 12` — topBar slots use `nTag` range `[24, 35]` (`MAX_FAST_COL*2` to `MAX_FAST_COL*3 - 1`)
- `slotIndex = nTag - MAX_FAST_COL*2` (0..11)
- `pSkill->GetFireSpeed()` returns `_Skill.lResumeTime` in milliseconds — the client-side cooldown duration
- `pSkill->IsAttackTime(dwNow)` returns `dwNow >= _dwAttackTime` — server-confirmed cooldown check
- `CCommandObj::_pCommand` (static) is the "pending command" that `ActAttackCha`'s `CAttackState` picks up via `GetReadyCommand()` and stores via `SetCommand()` — this is how `StartCommand()` is routed back to the correct `CSkillCommand` for AniClock allocation
