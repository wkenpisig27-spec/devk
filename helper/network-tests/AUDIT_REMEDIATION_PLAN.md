# Audit Remediation Plan — Autonomous Execution

**Source:** Principal review of commits `304926a0..a04ce8d6` (2026-07-01)  
**Goal:** Close audit findings, retire obsolete code, tighten security, exit Phase 3 cleanly  
**Status tracker:** Update task rows here + [`STATUS.md`](STATUS.md) after each completed batch  
**Do not** implement new features in this plan — fix, clean, harden only.

---

## How to use this document (agents)

1. Read [`STATUS.md`](STATUS.md) and this file.
2. Pick the **lowest-numbered incomplete batch** whose dependencies are satisfied.
3. Implement only that batch; build Release|x64 for touched projects.
4. Run the batch **acceptance checklist** (manual T0 where noted).
5. Mark tasks `done` with date; commit with message referencing batch ID (e.g. `fix(network): R1 dead switch cleanup`).
6. Stop if a batch fails acceptance — fix before starting the next.

**Deploy rule:** Gate+Group+Game from same build when touching session/trailer/ingress paths.

**Build targets (Release|x64):**

| Project | Path |
|---------|------|
| LIBDBC | `source/build/serversdk/LIBDBC.vcxproj` |
| Common | `source/build/common/Common.vcxproj` |
| GateServer | `source/build/gateserver/GateServer.vcxproj` |
| GroupServer | `source/build/groupserver/GroupServer.vcxproj` |
| GameServer | `source/build/gameserver/GameServer.vcxproj` |

---

## Priority overview

| Phase | Batches | Theme | Risk | Est. effort |
|-------|---------|-------|------|-------------|
| **R1** | R1.1–R1.4 | Dead code cleanup (zero behavior change) | Low | 1 session |
| **R2** | R2.1–R2.3 | Ingress fail-closed + Gate disconnect | Medium | 1 session |
| **R3** | R3.1–R3.2 | Backplane + ops hardening | Low | 0.5 session |
| **R4** | R4.1–R4.2 | PM broadcast + GameApp behavior | Medium | 1 session |
| **R5** | R5.1–R5.3 | Session model completion (resync/fallback) | High | 2 sessions |
| **R6** | R6.1 | Phase 3 exit soak + sign-off | Low | 0.5 session (wall clock) |
| **R7** | R7.x | Long-term (optional, post Phase 3) | Varies | Backlog |

---

## R1 — Dead code cleanup (no behavior change)

**Exit gate:** Builds pass; T0-login → enter → move (smoke only).

### R1.1 — Remove empty legacy switches

| Field | Value |
|-------|-------|
| **Audit refs** | F-11, F-12 |
| **Files** | `source/src/gameserver/CharacterPrl.cpp`, `source/src/gameserver/GameAppNet.cpp` |
| **Action** | Delete empty `switch` after `DispatchOpcodeHandler` in `CCharacter::ProcessPacket` (~3024–3027) and `CGameApp::ProcessInterGameMsg` (~1705–1707). Keep `T_B`/`T_E` macros if still needed for surrounding function. |
| **Verify** | `grep "switch (usCmd)" CharacterPrl.cpp` — ProcessPacket has no switch; GameAppNet ProcessInterGameMsg has no switch. |
| **Build** | GameServer |
| **Acceptance** | Compile PASS; login → enter → one CM action (move/skill). |
| **Status** | pending |

### R1.2 — Remove unreachable `ProcessPacket` pointer branch

| Field | Value |
|-------|-------|
| **Audit refs** | F-01 |
| **Files** | `source/src/gameserver/GameAppNet.cpp` (`CGameApp::ProcessPacket` default branch) |
| **Action** | Remove `else { MakePointer + ValidatePlayerPointer }` block (~430–442). Session band path only for CM/TM/PM. Optionally add comment: all valid non-MM bands use session trailer. |
| **Verify** | No `MakePointer` in `ProcessPacket` default branch. |
| **Build** | GameServer |
| **Acceptance** | T0-enter; move; chat; logout. |
| **Status** | pending |

### R1.3 — DRY gate SyncCall trailer helper

| Field | Value |
|-------|-------|
| **Audit refs** | F-13, cleanup P1 |
| **Files** | `source/src/gateserver/GateServer.cpp`, `GateServer.h`, `source/src/gateserver/ToClient.cpp` |
| **Action** | Remove `AppendTpGroupSyncTrailer`; replace ~10 call sites with `AppendInGameGroupTrailer` after inline `gp_addr` check (or fold check into a single `AppendGroupSyncTrailer` name). |
| **Build** | GateServer |
| **Acceptance** | Login → BGNPLAY → enter; NEWCHA if test account available. |
| **Status** | pending |

### R1.4 — Trim Group `ValidatePlayerPointer` API

| Field | Value |
|-------|-------|
| **Audit refs** | cleanup P1 |
| **Files** | `GroupServerApp.h`, `GroupServerAppServ.cpp` |
| **Action** | Remove unused `generation` parameter (only call passes `0`). Keep registry + session branches. |
| **Build** | GroupServer |
| **Acceptance** | Login + char select + enter. |
| **Status** | pending |

### R1.5 — Doc sync (audit drift)

| Field | Value |
|-------|-------|
| **Audit refs** | F-14 |
| **Files** | `helper/network-tests/STATUS.md`, `docs/PACKET_SYSTEM_REFACTOR.md` |
| **Action** | Remove references to `syncCallLegacy`, phase-4 interim baseline as current; document final session-on-SyncCall state. Add link to this remediation plan. |
| **Status** | pending |

**R1 commit suggestion:** `chore(network): R1 audit dead-code cleanup (F-01, F-11–F-14)`

---

## R2 — Ingress fail-closed

**Exit gate:** Invalid opcode tests rejected; no T0 regression.

**Depends on:** R1.2 recommended first (pointer branch removed before tightening ingress).

### R2.1 — GameApp ingress default fail-closed

| Field | Value |
|-------|-------|
| **Audit refs** | F-02 |
| **Files** | `source/src/common/OpcodeIngress.cpp`, optionally `GameAppNet.cpp` |
| **Action** | Change `ValidateGameAppOpcode`: if opcode is in TM/PM/MM band (per `IsGameAppBand`) but **not** in registry → `LogReject` + return false. Registered opcodes keep current min-payload checks. |
| **Design note** | All 35 GameApp handlers are registered (B5 complete). Unregistered in-band opcode = bug or attack → reject. |
| **Build** | Common + GameServer |
| **Acceptance** | Full T0 + guild/party PM if available; grep logs for spurious `[reject]`. |
| **Status** | **done** |

### R2.2 — Gate disconnect on unhandled registered opcode

| Field | Value |
|-------|-------|
| **Audit refs** | F-10 |
| **Files** | `source/src/gateserver/ToClient.cpp` (`OnProcessData`) |
| **Action** | After `ValidateClientToGateOpcode` passes and `DispatchOpcodeHandler` returns false → log + `Disconnect(datasock, 50, -32)` (same as invalid opcode). |
| **Build** | GateServer |
| **Acceptance** | Normal play unaffected; automated connect/ping still PASS. |
| **Status** | **done** |

### R2.3 — Group ProcessData ingress (optional tighten)

| Field | Value |
|-------|-------|
| **Audit refs** | F-06 related (Group band bypass) |
| **Files** | `OpcodeIngress.cpp`, `GroupServerAppServ.cpp` |
| **Action** | For AP-band packets on Account socket: validate known opcode before switch; unknown → log + drop (do **not** KickUser on Account link — see audit F-06). |
| **Risk** | Medium — test Account login thoroughly. |
| **Acceptance** | Login → enter; AccountServer log clean. |
| **Status** | pending (optional) |

**R2 commit suggestion:** `fix(security): R2 fail-closed ingress (F-02, F-10)`

---

## R3 — Backplane + ops hardening

**Depends on:** none (can parallel R1).

### R3.1 — Fatal on RequireAuth without PSK

| Field | Value |
|-------|-------|
| **Audit refs** | F-04 |
| **Files** | `BackplaneAuth.cpp`, server `*.cfg` |
| **Action** | In `SetClusterConfig` or server startup: if `requireAuth && psk.empty()` → log fatal + `exit(1)` (or refuse to accept connections with explicit error). Document in `server/*.cfg` comments. |
| **Build** | LIBDBC + all servers linking BackplaneAuth |
| **Acceptance** | Valid PSK → login works; empty PSK + RequireAuth=1 → server refuses start OR logs clear fatal. |
| **Status** | pending |

### R3.2 — Session slot monitoring (document + log)

| Field | Value |
|-------|-------|
| **Audit refs** | F-07, F-08 |
| **Files** | `SessionManager.cpp`, `docs/NETWORK_AUDIT.md` |
| **Action** | On failed `Allocate`, log active count estimate (`m_nextSlot - freeList.size()`). Add ops note: 65536 slot cap per SessionManager instance. |
| **Status** | pending |

**R3 commit suggestion:** `fix(security): R3 backplane misconfig fail-closed (F-04)`

---

## R4 — PM broadcast behavior

**Depends on:** R1.2, R2.1.

### R4.1 — Investigate + fix `ProcessGroupBroadcast`

| Field | Value |
|-------|-------|
| **Audit refs** | F-06 |
| **Files** | `GameAppNet.cpp`, `GameApp.h` |
| **Action** | 1) Git history / pre-B5 behavior for PM broadcast when session resolve fails. 2) Either implement minimal broadcast (guild/world relay path) OR remove call sites and log `SessionManager PM broadcast reject cmd=…`. **Do not leave empty stub.** |
| **Build** | GameServer |
| **Acceptance** | Party/guild chat PM paths tested; no silent no-op. |
| **Status** | pending |

### R4.2 — Pre-existing Garner2 bug (out of audit range, low hanging fruit)

| Field | Value |
|-------|-------|
| **Audit refs** | F-15 |
| **Files** | `GameAppNet.cpp` `ProcessGarner2Update` ~1735 |
| **Action** | Change `FindPlayerByDBChaID(chaid[0])` → `chaid[i]` in loop. |
| **Acceptance** | Code review only unless Garner2 event testable. |
| **Status** | pending (optional) |

**R4 commit suggestion:** `fix(game): R4 PM broadcast / Garner2 lookup (F-06, F-15)`

---

## R5 — Session model completion

**High risk — one sub-batch per commit; full T0 after each.**

**Depends on:** R1 complete; R2.1 recommended.

### R5.1 — Remove Group `MP_ENTERMAP` pointer fallback (if login bind sufficient)

| Field | Value |
|-------|-------|
| **Audit refs** | F-03 |
| **Files** | `GroupServerAppServ.cpp` `ResolvePlayerFromGateTrailer` |
| **Action** | Remove block at ~243–250. If `ResolveSession` fails on `MP_ENTERMAP` → reject + log. |
| **Pre-check** | Confirm Gate always sends session trailer on MP_ENTERMAP (`ToGameServer.cpp` 494–499). |
| **Deploy** | Gate + Group together |
| **Acceptance** | Login → enter → move; ENDPLAY → re-enter different char. |
| **Status** | pending |

### R5.2 — Remove legacy TM_ENTERMAP 10-byte trailer path

| Field | Value |
|-------|-------|
| **Audit refs** | F-09 (legacy trailer) |
| **Files** | `GameAppNet.cpp` `OpcodeHandle_TmEntermap`, `ToGameServer.cpp` EnterMap |
| **Action** | Require 18-byte session trailer; reject short trailer with log. Remove `else if (trailerRemain >= 10)` branch. |
| **Deploy** | Gate + Game |
| **Acceptance** | Enter map only via session path; no enter failures. |
| **Status** | pending |

### R5.3 — `TP_SYNC_PLYLST` session restore

| Field | Value |
|-------|-------|
| **Audit refs** | F-05, F-18 |
| **Files** | `ToGroupServer.cpp`, `GroupServerAppServ.cpp` |
| **Action** | Extend wire format: Gate sends `{slot, gen, gate_ptr, gp_addr}` per player on sync list. Group `BindPlayerSession` on restore. **Wire change** — Gate+Group atomic deploy. Fallback: force disconnect all clients on Gate–Group reconnect (document in ops). |
| **Design choice** | Prefer session restore if Gate still has `m_sessionHandle` for each Player; else document mandatory re-login after Group restart. |
| **Acceptance** | Restart Group only → clients recover OR clean re-login prompt; no pointer-only resync. |
| **Status** | pending |

**R5 commit suggestion (per sub-batch):** `feat(network): R5.x session model completion (F-03, F-05, F-09)`

---

## R6 — Phase 3 exit

**Depends on:** R1 + R2 + R3.1 minimum; R4–R5 as team prioritizes.

### R6.1 — 30-minute soak + sign-off

| Field | Value |
|-------|-------|
| **Action** | Run stack 30 min with 1–2 clients: login, enter, move, skill, chat, party/guild PM, logout, re-login. |
| **Log grep** | No `Invalid player trailer`, no `[reject]` storms, no `SessionManager REJECT` during normal play. |
| **Update** | Mark Phase 3 exit gate **done** in `STATUS.md`. |
| **Status** | pending |

### R6.2 — Release tag + audit closure note

| Field | Value |
|-------|-------|
| **Action** | Tag commit; add `AUDIT_REMEDIATION_PLAN.md` closure section listing deferred R7 items. |
| **Status** | pending |

---

## R7 — Backlog (post Phase 3, optional)

| ID | Audit ref | Task | Notes |
|----|-----------|------|-------|
| R7.1 | F-16 | GameAppNet `READ_*` → PacketReader | Handler-by-handler; low risk batch |
| R7.2 | F-17 | IOCP transport (L1) | Large; trigger when >500 clients |
| R7.3 | F-09 | SyncCall `ForwardFromReceive` where safe | Perf only; login/BGNPLAY keep Duplicate |
| R7.4 | F-18 | MC/MP fan-out session migration | Replace `MakePointer` multicast with session list |
| R7.5 | — | `CharacterCmd.cpp` READ_* migration | Out of CharacterPrl registry path |
| R7.6 | — | Group `TP_USER_LOGIN` use Duplicate for Account SyncCall | Stability hardening (shared recvbuf) |

---

## Regression checklist (run after every batch touching network)

```
[ ] T0-connect automated (net-smoke.py)
[ ] T0-ping automated
[ ] T0-login manual
[ ] T0-enter (BGNPLAY → world)
[ ] Move + chat in-world
[ ] ENDPLAY → char select → enter alt char (if available)
[ ] Logout → re-login
[ ] Deploy matched Gate+Group (+Game if touched)
```

---

## Task status board (update in place)

| Batch | Status | Date | Commit |
|-------|--------|------|--------|
| R1.1 | **done** | 2026-07-01 | `ab0fe7d3` |
| R1.2 | **done** | 2026-07-01 | `ab0fe7d3` |
| R1.3 | **done** | 2026-07-01 | `ab0fe7d3` |
| R1.4 | **done** | 2026-07-01 | `ab0fe7d3` |
| R1.5 | **done** | 2026-07-01 | `ab0fe7d3` |
| R2.1 | **done** | 2026-06-26 | `89cef60a` |
| R2.2 | **done** | 2026-06-26 | `89cef60a` |
| R2.3 | pending | | |
| R3.1 | pending | | |
| R3.2 | pending | | |
| R4.1 | pending | | |
| R4.2 | pending | | |
| R5.1 | pending | | |
| R5.2 | pending | | |
| R5.3 | pending | | |
| R6.1 | pending | | |
| R6.2 | pending | | |

---

## Resume prompt (copy for agents)

> Continue **Audit Remediation Plan** (`helper/network-tests/AUDIT_REMEDIATION_PLAN.md`). Find lowest pending batch with satisfied dependencies. Implement one batch only; build Release|x64; run batch acceptance checklist; update status board + STATUS.md; commit with `R#` prefix. Do not start R5 until R1 complete. Deploy Gate+Group+Game together when touching sessions/ingress.

---

## Finding → batch mapping

| Finding | Batch |
|---------|-------|
| F-01 dead ProcessPacket pointer branch | R1.2 |
| F-02 ValidateGameAppOpcode fail-open | R2.1 |
| F-03 MP_ENTERMAP fallback | R5.1 |
| F-04 Backplane PSK empty | R3.1 |
| F-05 TP_SYNC_PLYLST | R5.3 |
| F-06 ProcessGroupBroadcast stub | R4.1 |
| F-07 Session exhaustion | R3.2 |
| F-08 Session memory footprint | R3.2 (doc) |
| F-09 ForwardFromReceive / legacy trailer | done (Track C); R5.2 for TM_ENTERMAP |
| F-10 Gate unhandled opcode | R2.2 |
| F-11 CharacterPrl empty switch | R1.1 |
| F-12 GameAppNet empty switch | R1.1 |
| F-13 AppendTpGroupSyncTrailer wrapper | R1.3 |
| F-14 Doc drift | R1.5 |
| F-15 Garner2 chaid[i] bug | R4.2 |
| F-16 READ_* in GameAppNet | R7.1 |
| F-17 select/IOCP | R7.2 |
| F-18 MC/MP MakePointer fan-out | R7.4 |
