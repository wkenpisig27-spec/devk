# Option B / Phase-6 — Execution Status

Track progress for autonomous refactoring. Update after each completed task.

**Current phase:** Phase 3 exit + **Audit remediation** (see [`AUDIT_REMEDIATION_PLAN.md`](AUDIT_REMEDIATION_PLAN.md))  
**Last updated:** 2026-07-02  
**Master audit:** [`docs/NETWORK_AUDIT.md`](../../docs/NETWORK_AUDIT.md)  
**Refactor guide:** [`docs/PACKET_SYSTEM_REFACTOR.md`](../../docs/PACKET_SYSTEM_REFACTOR.md)

---

## Phase 0 — Baseline & safety net

| Task | Status | Date | Notes |
|------|--------|------|-------|
| 0.1 Opcode CSV from NetCommand.h | done | 2026-06-26 | `scripts/generate_opcode_table.py` → `data/opcodes.csv` |
| 0.1 Build target inventory | done | 2026-06-26 | `BASELINE.md` |
| 0.1 Network config documentation | done | 2026-06-26 | Gate/Game cfg documented |
| 0.2 T0 smoke test script | done | 2026-06-26 | `net-smoke.py` (connect + ping automated) |
| 0.3 NET_AUDIT_DIAG flag | done | 2026-06-26 | `NetAuditDiag.h` + `Receiver.cpp` |
| 0.4 Release build verify | done | 2026-06-26 | Release\|x64 build |
| 0.4 T0-connect automated | done | 2026-06-26 | PASS cmd=943 (RSA handshake) |
| 0.4 T0-ping automated | done | 2026-06-26 | PASS |
| 0.4 T0-login manual | done | 2026-06-27 | Login + char select + enter world + chat |
| **Phase 0 exit gate** | **done** | 2026-06-26 | Baseline locked |

---

## Phase 1 — Immediate fixes (Sprint 1)

| Task | Status | Date | Notes |
|------|--------|------|-------|
| I4 Read contract + caller audit | done | 2026-06-26 | `Packet.cpp/h`, `PacketReader.h` |
| I5 Fail closed on parse exceptions | done | 2026-06-26 | `PacketPipeline.h`; Receiver, CommRPC, PacketQueue |
| I2 Alloc caps / inter-server limits | done | 2026-06-26 | `NetLimits.h`, `__recvbuf_cap`, Receiver |
| I1 Connection rate limiting | done | 2026-06-26 | OnConnect + traffic flood disconnect |
| I3 Steady clock keepalive | done | 2026-06-26 | `GetSteadyMs()` |
| I7 volatile → atomic | done | 2026-06-26 | DataSocket flags/stats |
| I6 AES-GCM wire integrity | done | 2026-06-26 | CommEncrypt=2, WireGcm in PacketEncryption |
| **Phase 1 exit gate** | **done** | 2026-06-26 | Sprint 1 complete |

---

## Phase 2 — Foundation

| Task | Status | Date | Notes |
|------|--------|------|-------|
| M1 Opcode table (OpcodeMeta) | done | 2026-06-26 | `OpcodeMeta.h/cpp`, `OpcodeMetaTable.inc` |
| M5-prep PacketReader/Writer | done | 2026-06-26 | `PacketReader.h`, `PacketWriter.h` |
| M2-prep Handler registry + pilots | done | 2026-06-26 | Gate: PING, SAY, KITBAG_UNLOCK, etc. |
| M2 ToClient full registry migration | done | 2026-06-26 | Bulk CM/CP route handlers |
| **Production hotfixes** | done | 2026-06-27 | 32 KB client packet limits; SyncCall on comm thread (login, BGNPLAY); commit `665a8778` |
| **Phase 2 exit gate** | **done** | 2026-06-27 | T0-login + T0-enter + chat verified |

---

## Phase 3 — Medium-term core (in progress)

| Track | Task | Status | Notes |
|-------|------|--------|-------|
| A | M3 Session handles (slot + generation) | **done** | Phases 1–3 + 5 complete (2026-07-01): all Gate→Group paths use session trailers |
| B | M2 Bulk migration — Gate lifecycle (B1) | mostly done | SyncCall on comm thread covers login/BGNPLAY |
| B | M2 Bulk migration — GameAppNet (B5) | **done** | batch 3: 35 handlers total (2026-06-27) |
| B | M2 Bulk migration — CharacterPrl (B6) | **done** | 2026-06-27 — 112 handlers; legacy switch empty |
| C | M6 Zero-copy gate forwarding | **done** | `WPacket::ForwardFromReceive` in ReRoute paths (2026-07-01) |

**Audit remediation (post `a04ce8d6`):** [`AUDIT_REMEDIATION_PLAN.md`](AUDIT_REMEDIATION_PLAN.md) — batches R1–R7 from principal review. **R3 backplane + ops hardening done (2026-07-02):** R3.1 PSK fail-closed, R3.2 session slot exhaustion logging. **R5 session model completion done (2026-07-02):** R5.1 MP_ENTERMAP, R5.2 TM_ENTERMAP, R5.3 TP_SYNC_PLYLST session restore.
| D | M4 Backplane PSK auth | **done** | 2026-06-27 — `BackplaneAuth` HMAC-SHA256; OS/SO opcodes 6510/7010 |
| E | M5 PacketReader everywhere | **done** | Track E batch 10: ViewItemInfo PacketReader (2026-07-01) |

**Phase 3 exit gate:** Legacy switches empty or assert-only; session handles primary; backplane auth in default configs; 30-min soak clean.

### Track D — Backplane PSK auth (2026-06-27)

Mutual HMAC-SHA256 handshake on inter-server TCP links before normal protocol traffic.

| Link | Initiator | Listener | Integration |
|------|-----------|----------|-------------|
| Game → Gate | `GameServerApp::ConnectGate` | `ToGameServer::OnServeCall` | SyncCall hello before `CMD_MM_GATE_CONNECT` |
| Gate → Group | `ConnectGroupServer` | `GroupServerApp::OnServeCall` | SyncCall hello before `CMD_TP_LOGIN` |
| Group → Account | `InitACTSvrConnect` | `AccountServer2::OnServeCall` | SyncCall hello before `CMD_PA_LOGIN` |

Wire opcodes (Monitor band, internal only): `CMD_OS_BACKPLANE_HELLO` (6510) / `CMD_SO_BACKPLANE_HELLO` (7010).

Config: `[Backplane]` `PSK`, `RequireAuth`, `HandshakeTimeoutMs` in `server/*.cfg` (`GateServer.cfg`, `GameServer.cfg`, `GroupServer.cfg`, `AccountServer.cfg`). Empty PSK + `RequireAuth=0` = legacy accept. **`RequireAuth=1` with empty PSK → FATAL at startup (exit 1, R3.1).**

Files: `BackplaneAuth.h/cpp`, `NetCommand.h`, Gate/Game/Group/Account integration, default cfgs enabled.

Build: Release\|x64 LIBDBC + Gate + Game + Group + Account (see build log).

Manual: matching PSK → login/enter/chat; wrong PSK → backplane disconnect reason -41 in `BackplaneAuth` log.

### Track B6 — CharacterPrl registry (batch 1, 2026-06-27)

`CCharacter::ProcessPacket` now tries `DispatchOpcodeHandler` first; unmigrated opcodes fall through to legacy switch.

Registered handlers (10):

| Opcode | # | Handler |
|--------|---|---------|
| CMD_CM_BOSSTIMER_REQUEST | 115 | Boss timer sync |
| CMD_CM_RANK | 97 | Exp rank query |
| CMD_CM_CANCELEXIT | 437 | Cancel logout timer |
| CMD_CM_CHECK_PING | 17 | Client ping ack |
| CMD_CM_ENDACTION | 7 | End action |
| CMD_CM_DIE_RETURN | 10 | Relive selection |
| CMD_CM_MISLOG | 325 | Mission log list |
| CMD_CM_MISLOGINFO | 326 | Mission log detail |
| CMD_CM_MISLOG_CLEAR | 327 | Mission log clear |
| CMD_CM_MAP_MASK | 18 | Fog-of-war mask query |

Files: `CharacterPrl.cpp`, `Character.h`, `GameApp.cpp` (`RegisterCharacterOpcodeHandlers` at init).

Build: `CharacterPrl.cpp` compiles Release\|x64; link blocked (GameServer.exe in use). Opcode meta test PASS.

**Manual verification (2026-06-27):** T0-login, T0-enter, chat, and in-game checks for batch 1 + batch 2 handlers — **PASS** (user confirmed).

Blockers: none for batch 3. ~80 switch cases remain.

### Track B6 — CharacterPrl registry (batch 2, 2026-06-27)

Added 10 handlers (20 total on registry):

| Opcode | # | Handler |
|--------|---|---------|
| CMD_CM_SAY | 1 | Chat / GM commands |
| CMD_CM_SYNATTR | 8 | Reassign attribute points |
| CMD_CM_REFRESH_DATA | 16 | Sync equip attrs |
| CMD_CM_KITBAG_UNLOCK | 32 | Unlock inventory |
| CMD_CM_KITBAG_CHECK | 33 | Check lock state |
| CMD_CM_STALLSEARCH | 87 | Stall item search |
| CMD_CM_STALL_ALLDATA | 330 | Start stall setup |
| CMD_CM_BOAT_GETINFO | 344 | Boat info query |
| CMD_CM_READBOOK_START | 349 | Begin reading |
| CMD_CM_READBOOK_CLOSE | 350 | End reading |

Files: `CharacterPrl.cpp`, `Character.h` (same init path as batch 1).

Build: `CharacterPrl.cpp` compiles Release\|x64; link blocked (GameServer.exe in use).

Blockers: none for batch 3.

### Track B6 — CharacterPrl registry (batch 3, 2026-06-27)

Added 10 handlers (30 total on registry):

| Opcode | # | Handler |
|--------|---|---------|
| CMD_CM_BEGINACTION | 6 | Combat/movement action start |
| CMD_CM_UPDATEHAIR | 20 | Change hair style |
| CMD_CM_KITBAG_LOCK | 31 | Lock inventory |
| CMD_CM_KITBAG_AUTOLOCK | 34 | Set autolock flag |
| CMD_CM_STALL_OPEN | 331 | Open stall |
| CMD_CM_STALL_CLOSE | 333 | Close stall |
| CMD_CM_FORGE | 335 | Forge item by index |
| CMD_CM_CREATE_BOAT | 338 | Create boat |
| CMD_CM_UPDATEBOAT_PART | 339 | Update boat part |
| CMD_CM_BOAT_CANCEL | 340 | Cancel boat build |

Files: `CharacterPrl.cpp`, `Character.h` (same init path as batch 1–2).

Build: Release\|x64 **PASS** (`GameServer.vcxproj`, CharacterPrl.cpp compiled and linked).

Blockers: none for batch 4. ~70 switch cases remain.

**Manual verification (2026-06-27):** T0-login, T0-enter, chat, and in-game checks for batch 1–3 handlers including BEGINACTION — **PASS** (user confirmed).

**Recommended batch 4 (~10):** BOAT_LUANCH, BOAT_SELECT, BOAT_BAGSEL, ENTITY_EVENT, STALL_BUY, SKILLUPGRADE, TEAM_FIGHT_ASK/ASR, ITEM_REPAIR_ASK/ASR (boat/stall/combat-adjacent, self-contained bodies).

### Track A — M3 session handles (phase 1, 2026-06-27)

**Design:** Option A dual validation — CM forward trailer unchanged (`gate_ptr + gm_addr`); session `{slot, gen}` allocated on Gate at `EnterMap`, appended to `CMD_TM_ENTERMAP` trailer (+8 bytes, backward-compatible via `RemainData()` fork). Game binds mirror handle on `GatePlayer` at enter-map; `ValidatePlayerPointer` requires pointer registry **and** session registry match when handle is set.

**New files:**
- `source/include/common/SessionHandle.h`
- `source/include/common/SessionManager.h`
- `source/src/common/SessionManager.cpp` (in `Common.lib`)

**Wired:**
- Gate `Player::m_sessionHandle`, `GateServer::m_sessionManager`, `EnsurePlayerSession` / `ReleasePlayerSession`
- `GameServer::EnterMap` — allocate + append slot/gen to TM_ENTERMAP
- `ToClient::ReRouteToGameServer` — phase 1 logged session; phase 2 writes session trailer
- Game `GatePlayer::m_sessionHandle`, per-gate `SessionManager`, `BindPlayerSession` / `ResolveSession`
- `OpcodeHandle_TmEntermap` — read extended trailer, bind session after `ADDPLAYER`
- Dual validation in `ValidatePlayerPointer` + gate `ValidatePlayerPointer`

**Build (2026-06-27):** `Common.lib` **PASS**; GateServer + GameServer **compile PASS**; link blocked (`GateServer.exe` / `GameServer.exe` in use).

**Manual test:** T0-login → T0-enter → move/chat; grep logs for `SessionManager` (`Gate allocated`, `TM_ENTERMAP`, `ReRoute`, `GameServer bound`).

### Track A — M3 session handles (phase 2, 2026-06-27)

**Trailer swap (gate→game CM forward):** first identity field repurposed from `WriteLongLong(gate_ptr)` to `WriteLong(slot) + WriteLong(generation)`; second field unchanged `WriteLongLong(gm_addr)`. Total trailer still 16 bytes.

**Gate `ReRouteToGameServer`:** fail-closed if `!m_sessionHandle.IsValid()` in-game; writes session + gm_addr.

**Game default router (`GameAppNet.cpp`):** CM/TM/PM bands read reverse trailer `gm_addr`, `generation`, `slot` → `ResolvePlayerFromGateTrailer` → `ValidatePlayerSession`. Legacy pointer path retained for non-session bands only.

**Validation demotion:** `ValidatePlayerPointer` on game + gate skips pointer registry when player has bound session; `ValidatePlayerSession` is fail-closed (requires valid handle). Legacy pointer registry retained for TM/PM paths without session.

**Log lines (session-only CM path):**
- Gate: `SessionManager ReRoute cmd=… session slot=… gen=…`
- Gate reject: `SessionManager ReRoute REJECT cmd=…: no valid session`
- Game accept: `SessionManager CM session cmd=… slot=… gen=… player=…`
- Game reject: `SessionManager CM REJECT cmd=… session slot=… gen=…`

**Manual test checklist:** T0-login → T0-enter → move (`CM session cmd=6`) → chat (`CM session cmd=1`). Deploy gate + game together.

**GroupServer scope:** **done (phase 2b)** — see section below; TP SyncCall login path still uses pointer trailer.

**Build (2026-06-27, phase 2b):** GateServer + GameServer + GroupServer Release|x64 **PASS**. Deploy all three together; manual T0 verify pending.

### Track A — M3 session handles (phase 2b, 2026-06-27)

**Trailer swap (gate→group in-game forward):** first identity field repurposed from `WriteLongLong(gate_ptr)` to `WriteLong(slot) + WriteLong(generation)`; second field unchanged `WriteLongLong(gp_addr)`. Total trailer still 16 bytes (mirrors phase 2 CM path).

**Gate paths updated:**
- `ReRouteToGroupServer` — fail-closed if `!m_sessionHandle.IsValid()` when `gp_addr && gm_addr`; writes session + gp_addr via `AppendInGameGroupTrailer`
- `OpcodeHandle_CpPing` — same session trailer when in-world
- `CMD_MP_ENTERMAP` (ToGameServer.cpp) — session trailer sync to GroupServer at enter-map (bind event)

**GroupServer session support:**
- `GroupServerApp::m_sessionManager`, `Player::m_sessionHandle`
- `BindPlayerSession` / `ReleasePlayerSession` / `ResolveSession` / `ValidatePlayerSession`
- `ResolvePlayerFromGateTrailer` — session resolve + dual gp_addr check; `CMD_MP_ENTERMAP` binds mirror handle on first packet
- `OnProcessData` CP/MP ingress uses session trailer (fail-closed in-world)
- `OnServeCall` TP SyncCall path unchanged (pointer trailer for login/char select)
- `ValidatePlayerPointer` — session-authoritative when handle bound (dual validation pattern)

**Wire format (gate→group in-game, 16-byte trailer):**
```
WriteLong(slot) + WriteLong(generation) + WriteLongLong(gp_addr)
Reverse read on Group: gp_addr, generation, slot
```

**Log lines (group path):**
- Gate: `SessionManager ReRoute Group cmd=… session slot=… gen=…`
- Gate: `SessionManager EnterMap MP_ENTERMAP session slot=… gen=…`
- Group bind: `SessionManager Group MP_ENTERMAP bound session slot=… gen=…`
- Group accept: `SessionManager Group session cmd=… slot=… gen=… player=…`
- Group reject: `SessionManager Group REJECT cmd=… session slot=… gen=…`

**Manual test checklist:** T0-login → T0-enter → move → chat → friend invite or party invite (group path) → logout. Deploy gate + game + group together.

**Out of scope (phase 2b):** PM/TM game paths still pointer trailer; GroupServer internal MakePointer friend lookups; pointer registry cleanup.

**Build (2026-06-27):** `Common.lib` **PASS**; GateServer + GameServer + GroupServer Release\|x64 **PASS** (`GateServer.exe`, `GameServer.exe`, `GroupServer.exe` linked).

### Track A — M3 session handles (phase 3, 2026-06-27)

**Trailer swap (gate→game in-game TM/PM forward):** same 16-byte layout as phase 2 CM — `WriteLong(slot) + WriteLong(generation) + WriteLongLong(gm_addr)`. Reverse read on Game: `gm_addr`, `generation`, `slot`.

**Gate paths updated:**
- `AppendInGameGameTrailer` — fail-closed without valid session; writes `{slot, gen, gm_addr}`
- `ReRouteToGameServer` — uses `AppendInGameGameTrailer` (CM + in-game forwards)
- `CMD_TM_GOOUTMAP` — CM_LOGOUT, CM_ENDPLAY, offline-mode success path
- `CMD_TM_OFFLINE_MODE` — SyncCall trailer migrated to session format

**Game paths updated:**
- `GameServerApp::ResolvePlayerFromGateTrailer` — session resolve + gm_addr dual check (mirror GroupServer)
- `ProcessPacket` default router — TM band (1000–1499) and PM band (4500+) use session trailer via shared helper; PM broadcast null-player path preserved
- `OpcodeHandle_TmGooutmap` — session resolve instead of MakePointer
- `TM_OFFLINE_MODE` SyncCall handler — session resolve instead of gate pointer trailer

**Wire format (gate→game in-game, 16-byte trailer):**
```
WriteLong(slot) + WriteLong(generation) + WriteLongLong(gm_addr)
Reverse read on Game: gm_addr, generation, slot
```

**Log lines (game TM/PM path):**
- Gate: `SessionManager ReRoute Game cmd=… session slot=… gen=…`
- Gate reject: `SessionManager ReRoute Game REJECT cmd=…: no valid session`
- Game accept: `SessionManager Game session cmd=… slot=… gen=… player=…`
- Game reject: `SessionManager Game REJECT cmd=… session slot=… gen=…`

**Session release (unchanged, verified):** Gate `UnregisterPlayer` → `ReleasePlayerSession`; Game `DelPlayer` → `ReleasePlayerSession`.

**Intentionally kept on pointers:**
- `TM_ENTERMAP` gate ptr + session extension (bind event)
- `CMD_MP_ENTERMAP` group fallback when session not yet allocated
- TP/SyncCall login/char-select paths (gate→group)
- Game→Gate `WRITE_LONGLONG(MakeULong(pc->gate))` routing (gate connection identity)
- `CharacterCmd.cpp` enter-map response gm_addr write
- `OnGateDisconnect` player list walk
- Group internal friend MakePointer lookups
- Game→Group PM responses (Group→Game direction)

**Manual test checklist:** T0-login → T0-enter → move → chat → logout → char switch (EndPlay) → offline stall (if enabled) → party/guild PM. Deploy gate + game together.

**Build (2026-06-27, phase 3):** GateServer + GameServer Release\|x64 — **compile PASS**, link **FAIL (LNK1104)** — `GateServer.exe` / `GameServer.exe` locked (servers running). Stop processes and rebuild to produce binaries.

### Track A — M3 session handles (phase 4, 2026-07-01) — **REVERTED for SyncCall**

**Decision (2026-07-01):** TP SyncCall (login/char-select: `TP_BGNPLAY`, `TP_ENDPLAY`, `TP_USER_LOGOUT`, etc.) uses **session trailer** `{slot, generation, gp_addr}` after phase 5c/5d (same 16-byte layout as in-game CP/MP). Phase 4 legacy pointer trailer was the interim baseline between failed phase 4 attempt and 5a login bind.

**Why reverted:** Phase 4 put session trailers on SyncCall while Gate still sent legacy bytes (or vice versa). Group `OnServeCall` misread legacy tail as `{gp_addr, gen, slot}` → `ValidatePlayerPointer` saw `expectedGtAddr=1` (generation) → **ERR_PT_KICKUSER (529)** on BGNPLAY. Logs: `Group REJECT cmd=2005 session slot=0 gen=1`, `gtAddr mismatch (expected=1, got=…)`, `OnServeCall CMD 2005: Invalid player trailer`.

**Gate (SyncCall, post phase 5):**
- TP SyncCall uses `AppendInGameGroupTrailer` after `gp_addr` check (same `{slot, gen, gp_addr}` as in-game CP/MP)
- `TP_USER_LOGIN` — `AppendTpLoginRequestTrailer` + `WriteLongLong(MakeULong(l_ply))`

**Group (SyncCall):**
- `OnServeCall` — `ResolvePlayerFromGateTrailer` (session resolve)
- Session bind at `TP_USER_LOGIN` success; `MP_ENTERMAP` fallback retained until R5.1

**Still session-based (by design):** in-game Gate→Group CP/MP (`AppendInGameGroupTrailer`); Gate→Game CM/TM/PM (`AppendInGameGameTrailer`); `CMD_MP_ENTERMAP` bind event.

Build: GateServer + GroupServer Release\|x64; deploy **both** to `server/` (symlink or copy from `source/bin/Release/`). Restart all server processes before retest.

**Manual test checklist:** T0-login → BGNPLAY (char select) → enter map → move/chat → logout; grep logs — no `OnServeCall CMD 2005`, no `gtAddr mismatch (expected=1`, no spurious `Group REJECT cmd=2005 session`.

### Track A — M3 session handles (phase 5) — **PENDING: TP SyncCall full migration**

**Goal:** Extend session `{slot, generation}` trailers to **all Gate→Group TP SyncCall** paths so Track A is complete. In-game forwards (phases 2–3) already use sessions; this phase closes the last legacy pointer gap on the char-select / account lifecycle RPC path.

**Prerequisite:** Phase 4 revert verified — T0-login → BGNPLAY → enter map **PASS** on legacy SyncCall trailer (2026-07-01).

**Why a separate phase:** Phase 4 attempted this in one shot without coordinated bind timing → **529 ERR_PT_KICKUSER** on BGNPLAY. Gate and Group must deploy **together**; Group must bind session **before** any SyncCall that carries a session trailer.

---

#### Current vs target trailer layout (achieved 2026-07-01)

| Path | Layout |
|------|--------|
| Gate→Group **SyncCall** (TP_*) | `{slot, generation, gp_addr}` (16 B) |
| Gate→Group **SendData** (CP/MP) | `{slot, generation, gp_addr}` |
| Gate→Game **SendData** (CM/TM/PM) | `{slot, generation, gm_addr}` |
| `TP_USER_LOGIN` request tail | `{slot, generation, MakeULong(gate_ply), client IP}` |

Reverse read on Group `OnServeCall` (post-login TP_*):
```
gp_addr = ReverseReadLongLong()
generation = ReverseReadLong()
slot = ReverseReadLong()
→ ResolveSession(slot, generation) + gp_addr dual-check
```

---

#### Sub-phases (implement in order; deploy Gate + Group together after each)

| Sub | Task | Status | Files (primary) |
|-----|------|--------|-----------------|
| **5a** | **Group session bind at login** — on `TP_USER_LOGIN` success, read Gate session from request tail and `BindPlayerSession`; echo handle in response tail for Gate mirror | **done (2026-07-01)** | `GroupServerAppServ.cpp` (`TP_USER_LOGIN`), `ToClient.cpp` (login request/response parse) |
| **5b** | **Gate login request trailer** — `AppendTpLoginRequestTrailer` (or inline): `{slot, gen}` before `{MakeULong(gate_ply)}`; keep `EnsurePlayerSession` before SyncCall | **done (2026-07-01)** | `GateServer.cpp`, `GateServer.h`, `ToClient.cpp` |
| **5c** | **Migrate `AppendTpGroupSyncTrailer`** — write `{slot, gen, gp_addr}` (same helper body as `AppendInGameGroupTrailer`; consider merge/dedupe) | **done (2026-07-01)** | `GateServer.cpp` |
| **5d** | **Group `OnServeCall` session resolve** — replace legacy reverse-read + `syncCallLegacy=true` with `ResolvePlayerFromGateTrailer`-style session path; retain legacy fallback behind compile flag or one-release dual-read | **done (2026-07-01)** | `GroupServerAppServ.cpp`, `GroupServerApp.h` |
| **5e** | **Opcode sweep** — all SyncCall call sites use session trailer | **done (2026-07-01)** | `ToClient.cpp` → `AppendInGameGroupTrailer` |
| **5f** | **Remove legacy SyncCall fallback** | **done (2026-07-01)** | `ValidatePlayerPointer` session path; R1.3 removed `AppendTpGroupSyncTrailer` wrapper |
| **5g** | **Docs + exit gate** — update docs; T0 full lifecycle verified | **done (2026-07-01)** | docs, manual T0 |

**Phase 5 exit gate (2026-07-01):** User verified login → BGNPLAY → enter map → ENDPLAY → char switch → logout/re-login. All sub-phases complete.

**SyncCall opcodes (Gate→Group via `AppendInGameGroupTrailer` + `gp_addr` check):**

| Opcode | Handler | Notes |
|--------|---------|-------|
| `CMD_TP_BGNPLAY` (2005) | char enter — **critical T0 path** | test first after 5a–5d |
| `CMD_TP_ENDPLAY` | char exit / return to select | |
| `CMD_TP_USER_LOGOUT` | account logout | |
| `CMD_TP_NEWCHA` | create character | `ProbeSyncClientStrings` before SyncCall |
| `CMD_TP_DELCHA` | delete character | |
| `CMD_TP_CHANGEPASS` | change password | |
| `CMD_TP_REGISTER` | account register | |
| `CMD_TP_CREATE_PASSWORD2` | secondary password create | |
| `CMD_TP_UPDATE_PASSWORD2` | secondary password update | |

**Out of scope for phase 5:** Game→Gate routing by gate connection pointer; Group internal `MakePointer` friend lookups; Account PA SyncCall (no session manager on Account).

---

#### Implementation rules (learned from phase 4 failure)

1. **Bind before trailer** — Group `BindPlayerSession` must run in `TP_USER_LOGIN` success **before** any post-login SyncCall can arrive.
2. **Deploy atomically** — Gate + Group binaries from the same build; never mix phase-4 Group with phase-5 Gate (or vice versa).
3. **Same 16-byte size** — wire size unchanged; only field semantics change (pointer halves → slot+gen+gp_addr).
4. **Comm-thread rule unchanged** — all SyncCall handlers stay on comm thread (login, BGNPLAY, NEWCHA, …).
5. **Fail-closed** — if Gate has no valid session at SyncCall time, return `ERR_MC_NETEXCP` to client (do not fall back to legacy mid-migration).
6. **Optional dual-read (5d)** — during rollout, detect trailer format: if `generation` looks like a pointer high-bit pattern, reject loudly rather than silently mis-parse.

---

#### Log grep (phase 5 acceptance)

**Expect after success:**
- `SessionManager TP_USER_LOGIN bound session slot=… gen=…`
- `SessionManager TP SyncCall cmd=2005 session slot=… gen=…`
- `SessionManager Group session cmd=… slot=… gen=…` on BGNPLAY path

**Must NOT appear:**
- `OnServeCall CMD 2005: Invalid player pointer`
- `gtAddr mismatch (expected=1, got=…)` (generation misread as gate addr)
- `Group REJECT cmd=2005 session slot=0 gen=1` with legacy bytes on wire

---

#### Manual test checklist (phase 5 exit gate)

1. Fresh login → char list → **BGNPLAY** → enter world → move/chat
2. **ENDPLAY** → back to char select → enter different char
3. **NEWCHA** / **DELCHA** (if test accounts available)
4. **Logout** and re-login (session slot reuse / generation bump)
5. Duplicate-login kick path still works
6. In-game CP/MP forwards still session-based (regression)
7. 30-min soak, grep `SessionManager` + `Security` logs clean

**Build/deploy:** GateServer + GroupServer Release\|x64 together → `server/` symlinks → restart via `Connect server.bat`.

**Phase 5 exit gate:** All sub-phases 5a–5g done; legacy SyncCall pointer trailer removed; T0 + soak PASS.

### Track B6 — CharacterPrl registry (batch 4, 2026-06-27)

Added 10 handlers (40 total on registry):

| Opcode | # | Handler |
|--------|---|---------|
| CMD_CM_SKILLUPGRADE | 11 | Skill level upgrade |
| CMD_CM_TEAM_FIGHT_ASK | 21 | Team fight challenge |
| CMD_CM_TEAM_FIGHT_ASR | 22 | Team fight answer |
| CMD_CM_ITEM_REPAIR_ASK | 23 | Item repair request |
| CMD_CM_ITEM_REPAIR_ASR | 24 | Item repair answer |
| CMD_CM_STALL_BUY | 332 | Buy from player stall |
| CMD_CM_BOAT_LUANCH | 341 | Launch boat (typo preserved) |
| CMD_CM_BOAT_BAGSEL | 342 | Boat bag selection |
| CMD_CM_BOAT_SELECT | 343 | Select boat |
| CMD_CM_ENTITY_EVENT | 345 | Mission entity event |

Files: `CharacterPrl.cpp`, `Character.h` (same init path as batch 1–3).

Build: Release\|x64 **PASS** (`GameServer.vcxproj`, CharacterPrl.cpp compiled and linked).

Blockers: none for batch 5. ~60 switch cases remain.

**Recommended batch 5 (~10):** ITEM_FORGE_CANACTION, VALIDATE_SLOT_ITEM, ITEM_FORGE_ASK/ASR, ITEM_LOTTERY_ASK, LIFESKILL_ASK/ASR, KITBAG_EXPAND (crafting/repair-adjacent, self-contained bodies).

### Track B6 — CharacterPrl registry (batch 5, 2026-06-27)

Added 10 handlers (50 total on registry):

| Opcode | # | Handler |
|--------|---|---------|
| CMD_CM_PING | 15 | Gate ping relay (cha ping query) |
| CMD_CM_ITEM_FORGE_ASK | 25 | Item forge request |
| CMD_CM_ITEM_FORGE_ASR | 26 | Item forge answer |
| CMD_CM_TIGER_START | 27 | Tiger minigame start |
| CMD_CM_ITEM_FORGE_CANACTION | 29 | Forge action enable/disable |
| CMD_CM_KITBAG_EXPAND | 36 | Inventory expansion via IMP |
| CMD_CM_LIFESKILL_ASR | 80 | Life skill answer |
| CMD_CM_LIFESKILL_ASK | 81 | Life skill request |
| CMD_CM_ITEM_LOTTERY_ASK | 95 | Item lottery request |
| CMD_CM_VALIDATE_SLOT_ITEM | 110 | Forge slot validation |

Files: `CharacterPrl.cpp`, `Character.h` (same init path as batch 1–4).

Build: Release\|x64 **PASS** (`GameServer.vcxproj`, CharacterPrl.cpp compiled and linked).

Blockers: none for batch 6. ~50 switch cases remain.

**Recommended batch 6 (~10):** TIGER_STOP, STORE_OPEN_ASK + STORE fallthrough group (LIST/BUY/CHANGE/QUERY/CLOSE/VIP), REQUESTTALK/REQUESTTRADE, KITBAGTEMP_SYNC, ITEM_LOCK/UNLOCK_ASK, GAME_REQUEST_PIN (store/tiger/NPC-adjacent).

### Track B6 — CharacterPrl registry (batch 6, 2026-06-27)

Added 15 handlers (65 total on registry; 14 opcodes — REQUESTTALK/REQUESTTRADE share one handler):

| Opcode | # | Handler |
|--------|---|---------|
| CMD_CM_TIGER_STOP | 28 | Tiger minigame stop |
| CMD_CM_KITBAGTEMP_SYNC | 35 | Temp kitbag sync + store accept |
| CMD_CM_STORE_OPEN_ASK | 41 | IGS store open (password gate) |
| CMD_CM_STORE_LIST_ASK | 42 | IGS store list |
| CMD_CM_STORE_BUY_ASK | 43 | IGS store buy |
| CMD_CM_STORE_CHANGE_ASK | 44 | IGS store exchange |
| CMD_CM_STORE_QUERY | 45 | IGS transaction query |
| CMD_CM_STORE_VIP | 46 | IGS VIP |
| CMD_CM_STORE_CLOSE | 48 | IGS store close |
| CMD_CM_REQUESTTALK | 301 | NPC talk (shared handler) |
| CMD_CM_REQUESTTRADE | 308 | NPC trade (shared handler) |
| CMD_CM_ITEM_LOCK_ASK | 99 | Lock item in kitbag |
| CMD_CM_ITEM_UNLOCK_ASK | 100 | Unlock item in kitbag |
| CMD_CM_GAME_REQUEST_PIN | 101 | PIN-gated script action |

**STORE fallthrough:** Legacy `CMD_CM_STORE_OPEN_ASK` case had no `break` and fell into the shared `operateIGS` Lua block. Preserved via anonymous `HandleStoreOperate()` helper — `OpcodeHandle_CmStoreOpenAsk` runs open/password logic then calls it on success; LIST/BUY/CHANGE/QUERY/VIP/CLOSE handlers call the same helper directly. CLOSE still clears `SetStoreEnable(false)` + `ForgeAction(false)`.

Files: `CharacterPrl.cpp`, `Character.h` (same init path as batch 1–5).

Build: Release\|x64 **PASS** (`GameServer.vcxproj`, CharacterPrl.cpp compiled and linked).

Blockers: none for batch 7. ~44 switch cases remain.

**Recommended batch 7 (~10):** VOLUNTER group (OPEN/LIST/ADD/DEL/SEL/ASR), CHARTRADE group (REQUEST/ACCEPT/CANCEL/ITEM/MONEY), MASTER/PRENTICE invites, MISSION/TALKPAGE/FUNCITEM (NPC/mission-adjacent).

### Track B6 — CharacterPrl registry (batch 7, 2026-06-27)

Added 20 handlers (84 total on registry; 20 opcodes):

| Opcode | # | Handler |
|--------|---|---------|
| CMD_CM_CHARTRADE_REQUEST | 312 | Player trade request |
| CMD_CM_CHARTRADE_ACCEPT | 313 | Accept trade |
| CMD_CM_CHARTRADE_REJECT | 314 | Reject trade (no-op) |
| CMD_CM_CHARTRADE_CANCEL | 315 | Cancel trade |
| CMD_CM_CHARTRADE_ITEM | 316 | Trade item slot |
| CMD_CM_CHARTRADE_MONEY | 319 | Trade gold/IMP |
| CMD_CM_CHARTRADE_VALIDATEDATA | 317 | Validate trade item data |
| CMD_CM_CHARTRADE_VALIDATE | 318 | Confirm trade |
| CMD_CM_VOLUNTER_OPEN | 65 | Volunteer list open |
| CMD_CM_VOLUNTER_LIST | 61 | Volunteer list page |
| CMD_CM_VOLUNTER_ADD | 62 | Add to volunteer list |
| CMD_CM_VOLUNTER_DEL | 63 | Remove from volunteer list |
| CMD_CM_VOLUNTER_SEL | 64 | Select volunteer (party ask) |
| CMD_CM_VOLUNTER_ASR | 66 | Volunteer party answer |
| CMD_CM_MASTER_INVITE | 71 | Apprentice invites mentor |
| CMD_CM_MASTER_ASR | 72 | Mentor answers apprentice invite |
| CMD_CM_MASTER_DEL | 73 | Apprentice removes mentor |
| CMD_CM_PRENTICE_DEL | 74 | Mentor removes apprentice |
| CMD_CM_PRENTICE_INVITE | 75 | Mentor invites apprentice |
| CMD_CM_PRENTICE_ASR | 76 | Apprentice answers mentor invite |

**Fallthrough / shared helpers:** No switch fallthrough in these groups (each legacy case had `break`). CHARTRADE handlers share anonymous `CharTradeRateLimitOk()` — preserves economy-block + 200 ms `m_dwLastTradePacketTime` gate used by all trade opcodes except REJECT (empty no-op). VOLUNTER and MASTER/PRENTICE are independent per-opcode handlers.

**Skipped (not in CharacterPrl switch):** `CMD_CM_MISSION` (322), `CMD_CM_TALKPAGE` (302), `CMD_CM_FUNCITEM` (303) — routed via NPC `MsgProc` on `CMD_CM_REQUESTTALK` (301), already on registry batch 6.

Files: `CharacterPrl.cpp`, `Character.h` (same init path as batch 1–6).

Build: Release\|x64 **PASS** (`GameServer.vcxproj`; `/m:1 /FS` needed when GameServer.exe holds PDB lock).

Blockers: none. **Track B6 complete** — legacy switch empty (`default` only).

**Recommended batch 8 (~10):** GUILD group (PUTNAME, TRYFOR, TRYFORCFM, LISTTRYPLAYER, APPROVE, REJECT, KICK, LEAVE, DISBAND, MOTTO, CHALLENGE, LEIZHU, PERM), TM_CHANGE_PERSONINFO, SAY2CAMP, PK_CTRL (social/guild-adjacent).

### Track B6 — CharacterPrl registry (batch 8, 2026-06-27)

Added 28 handlers (112 total on registry; 28 opcodes) — **final CharacterPrl switch cleanup**:

| Opcode | Handler |
|--------|---------|
| CMD_PM_GUILDBANK | Guild bank item/gold ops + ack |
| CMD_PM_PUSHTOGUILDBANK | Push item to guild bank |
| CMD_TM_CHANGE_PERSONINFO | Motto + icon update |
| CMD_CM_GUILD_PERM | Set guild member permission |
| CMD_CM_GUILD_PUTNAME | Create guild |
| CMD_CM_GUILD_TRYFOR | Apply to guild |
| CMD_CM_GUILD_TRYFORCFM | Confirm guild application |
| CMD_CM_GUILD_LISTTRYPLAYER | List applicants |
| CMD_CM_GUILD_APPROVE | Approve applicant |
| CMD_CM_GUILD_REJECT | Reject applicant |
| CMD_CM_GUILD_KICK | Kick member |
| CMD_CM_GUILD_LEAVE | Leave guild |
| CMD_CM_GUILD_DISBAND | Disband guild (client) |
| CMD_CM_GUILD_MOTTO | Set guild motto |
| CMD_PM_GUILD_DISBAND | Disband guild (group) |
| CMD_CM_GUILD_CHALLENGE | Guild challenge |
| CMD_CM_GUILD_LEIZHU | Guild leizhu |
| CMD_CM_SAY2CAMP | CTF/guild-war camp chat |
| CMD_CM_GM_SEND | IGS GM send |
| CMD_CM_GM_RECV | IGS GM recv |
| CMD_CM_PK_CTRL | PK mode toggle |
| CMD_CM_CHEAT_CHECK | Anti-cheat answer |
| CMD_CM_BIDUP | Auction bid |
| CMD_CM_ANTIINDULGENCE | Anti-indulgence scale flag |
| CMD_CM_REQUEST_DROP_RATE | Drop rate query |
| CMD_CM_REQUEST_EXP_RATE | EXP rate query |
| CMD_CM_GET_PLAYER_BATTLE_POINT | Battle power query |
| CMD_CM_REQUEST_CHEST_PREVIEW | Chest loot preview |

**Shared helpers:** `HandlePmGuildBank()` — preserves rate-limit + mandatory `CMD_MP_GUILDBANK` ack (must not break early on rate limit). Guild bank gold/item logic unchanged.

**Legacy switch:** `ProcessPacket` switch now contains only `default: break;` — all opcodes dispatched via registry.

Files: `CharacterPrl.cpp`, `Character.h` (same init path as batch 1–7).

Build: Release\|x64 **PASS** (`GameServer.vcxproj`, CharacterPrl.cpp compiled and linked).

Blockers: none. **Track B6 complete.**

### Track E — OpcodeIngress + PacketReader pilot (batch 1, 2026-06-27)

**Design:** Central ingress validation module driven by `OpcodeMeta` (band + known opcode) with per-handler `minPayloadBytes` on `OpcodeHandlerEntry`. Fail-closed at call sites — Gate disconnects (`-32`), Game drops packet. Bulk-routed CM/CP gap opcodes (registered but not in meta table) still allowed at Gate.

**New files:**
- `source/include/common/OpcodeIngress.h`
- `source/src/common/OpcodeIngress.cpp` (in `Common.lib`)

**API:**
- `ValidateKnownOpcode` — meta lookup, reject `isBase`
- `ValidateOpcodeBand` — band must match
- `ValidateMinPayload` — `RemainData() >= minBytes` after cmd consumed
- `ValidateClientToGateOpcode` — CM/CP band + known (or registered) + handler min
- `ValidateGameCharacterOpcode` — CM band + known + handler min
- `ValidateGameAppOpcode` — only when handler registered; TM/PM/MM band + known + handler min

**Registry changes:**
- `OpcodeHandlerEntry.minPayloadBytes` (default 0)
- `LookupOpcodeHandler` exported
- `DispatchOpcodeHandler` enforces min payload before handler invoke

**Gate pilot minPayload (~10 handlers):**

| Opcode | min | Notes |
|--------|-----|-------|
| CMD_CP_PING | 0 | empty body OK |
| CMD_CM_SAY | 0 | variable string |
| CMD_CM_KITBAG_UNLOCK | 0 | encrypted sequence |
| CMD_CM_ITEM_UNLOCK_ASK | 0 | encrypted + slot |
| CMD_CM_ENDACTION | 0 | route-only |
| CMD_CM_OFFLINE_MODE | 0 | route-only |
| CMD_CM_BEGINACTION | 5 | world ID + action type char |
| CMD_CM_CHECK_PING | 0 | empty body OK |
| CMD_CM_SYNATTR | 0 | variable attrs |
| CMD_CM_REFRESH_DATA | 8 | two longs |

**Wired:**
- Gate `ToClient::OnProcessData` — `ValidateClientToGateOpcode` before registry dispatch
- Game `CCharacter::ProcessPacket` — `ValidateGameCharacterOpcode` before dispatch
- Game `CGameApp::ProcessPacket` / `ProcessInterGameMsg` — `ValidateGameAppOpcode` only for registered opcodes (default router trailer path unchanged)

**PacketReader pilot:**
- Gate: `OpcodeHandle_CmItemUnlockAsk` (slot read); `CmSay` already migrated
- Game Character: `OpcodeHandle_CmBeginAction` (world ID), `OpcodeHandle_CmDieReturn` (relive char)

**Log line:** `OpcodeIngress [reject] cmd=N name=… reason=… peer=…`

**Build (2026-06-27):** `Common.lib` + `GateServer.exe` + `GameServer.exe` Release\|x64 **PASS** (VS 18 MSBuild).

**Manual test checklist:** T0-login → T0-enter → move (`CMD_CM_BEGINACTION`) → chat (`CMD_CM_SAY`) → ping → kitbag unlock → logout. Grep logs for spurious `OpcodeIngress [reject]`.

**Recommended batch 2:** Extend minPayload to more Gate explicit handlers + GameApp TM/PM/MM handlers with known fixed headers; migrate Gate `CmKitbagUnlock` sequence read; CharacterPrl READ_* → PacketReader for stall/boat group (~10 handlers); optional `OpcodeMeta.minPayload` column in generator (only if table-driven mins outweigh per-handler entries).

### Track E — OpcodeIngress + PacketReader (batch 2, 2026-06-27)

**minPayload expansion — GameApp (`RegisterAllGameAppOpcodeHandlers`):**

| Opcode | min | Notes |
|--------|-----|-------|
| CMD_PM_TEAM | 1 | team msg type char |
| CMD_PM_GUILDINFO | 4 | cha DBID long |
| CMD_PM_GUILD_CHALLMONEY | 12 | long + longlong before strings |
| CMD_PM_GUILD_CHALL_PRIZEMONEY | 12 | long + longlong before strings |
| CMD_TM_GOOUTMAP | 17 | 1 char body + 16-byte session trailer |
| CMD_PM_EXPSCALE | 8 | two longs |
| CMD_MM_UPDATEGUILDBANK | 4 | guild ID long |
| CMD_MM_UPDATEGUILDBANKGOLD | 4 | guild ID long |
| CMD_MM_GUILD_APPROVE | 4 | guild ID long before strings |
| CMD_MM_ADDCREDIT | 8 | cha DBID + credit long |
| CMD_MM_ADDMONEY | 8 | cha DBID + money long |

**minPayload expansion — Character (`RegisterCharacterOpcodeHandlers`):**

| Opcode | min | Notes |
|--------|-----|-------|
| CMD_CM_PING | 28 | long + longlong + long + long + longlong |
| CMD_CM_STALLSEARCH | 4 | item ID long |
| CMD_CM_MISLOGINFO | 2 | mission ID short |
| CMD_CM_MISLOG_CLEAR | 2 | mission ID short |
| CMD_CM_KITBAG_AUTOLOCK | 1 | autolock char |
| CMD_CM_KITBAG_CHECK | 0 | empty body |
| CMD_CM_KITBAG_LOCK | 0 | empty body |
| CMD_CM_KITBAG_UNLOCK | 0 | password string |
| CMD_CM_BOAT_GETINFO | 0 | empty body |
| CMD_CM_STALL_ALLDATA | 0 | variable stall payload |
| CMD_CM_SYNATTR | 0 | variable attr list |

**PacketReader migration (~10 handlers):**
- Gate: `OpcodeHandle_CmKitbagUnlock` — `PacketWriter` rebuild after encrypted sequence decrypt
- Character: `CmPing`, `CmStallSearch`, `CmRefreshData`, `CmMisLogInfo`, `CmMisLogClear`, `CmKitbagUnlock`, `CmKitbagAutolock` (+ batch 1 `CmBeginAction`, `CmDieReturn`)

**GroupServer ingress:**
- `ValidateGroupIngressOpcode(cmd, pk, peer, GroupIngressPath)` in `OpcodeIngress.h/cpp`
- `ServeCall`: fail-closed on band not TP/PA/OS + unknown opcode
- `ProcessData`: strict known-opcode check for CP/MP only; TP/AP server messages pass through unchanged
- Wired in `GroupServerAppServ.cpp` `OnServeCall` (after `CMD_OS_BACKPLANE_HELLO`) and `OnProcessData` (after `BackplaneAuth::AllowProcessData`)

**Reject rate summary:** static counter in `LogReject`; logs `[summary] rejects=N in last 60s` when count > 0.

**Build (2026-06-27):** `Common.lib` compile **PASS**; `GateServer.exe` / `GameServer.exe` / `GroupServer.exe` compile **PASS**, link **BLOCKED** (`LNK1104` — exes locked by running server processes). Stop servers and relink to refresh binaries.

**Manual test checklist:** login (`TP_LOGIN`/`TP_USER_LOGIN`) → enter map → move → chat → CP ping/party/friend if available → kitbag unlock → logout; grep for spurious `OpcodeIngress [reject]` on Gate/Game/Group.

**Recommended batch 3:** Gate explicit handler minPayload for login RSA path handlers; Character trade/forge group PacketReader; GameApp MM string handlers with conservative mins; GroupServer per-handler registry + minPayload for CP/MP player switch; AccountServer ingress pilot.

### Track E — OpcodeIngress + PacketReader (batch 3, 2026-06-27)

**AccountServer ingress pilot:**
- `ValidateAccountIngressOpcode(cmd, pk, peer)` — PA band + known opcode via OpcodeMeta; skips `CMD_OS_BACKPLANE_HELLO`
- Wired in `AccountServer2.cpp` `OnProcessData` + `OnServeCall` (after BackplaneAuth, before switch)
- Fail-closed: drop packet + `[AccountServer] Invalid CMD` Security log (replaces hardcoded 3000–3050 range)

**GroupServer CP hot-opcode min map (ProcessData path only):**

| Opcode | min | Notes |
|--------|-----|-------|
| CMD_CP_PING | 4 | ping value long |
| CMD_CP_TEAM_ACCEPT | 4 | inviter cha ID long |
| CMD_CP_TEAM_REFUSE | 4 | inviter cha ID long |
| CMD_CP_FRND_ACCEPT | 4 | inviter cha ID long |
| CMD_CP_TEAM_INVITE | 0 | string leads |
| CMD_CP_FRND_INVITE | 0 | string leads |
| CMD_CP_TEAM_LEAVE | 0 | empty body |

**Gate sync/login path:**
- `ValidateGateSyncClientOpcode` — CM/CP band + known opcode for RSA/login/TransmitCall set; min=0 (variable encrypted payloads)
- Wired in `ToClient::OnProcessData` before `CM_RSA_HANDSHAKE1` / `DispatchSyncClientOpcode`
- Payload comments in `CM_RSA_HANDSHAKE1` + `CM_LOGIN`

**Character trade/forge PacketReader (~8 handlers):**
- `CmForge`, `CmItemForgeAsk`, `CmItemForgeAsr`
- `CmChartradeRequest`, `Accept`, `Reject`, `Cancel`
- `CmRequestTalkOrTrade` (NPC ID long + remainder forwarded via `reader.Raw()`)

**GameApp minPayload additions:**

| Opcode | min | Notes |
|--------|-----|-------|
| CMD_PM_SAY2ALL | 4 | cha DBID long before string |
| CMD_PM_SAY2TRADE | 4 | cha DBID long before string |
| CMD_MM_GUILD_CHALL_PRIZEMONEY | 12 | long + longlong |
| CMD_MM_STORE_BUY | 12 | three longs |
| CMD_MM_AUCTION | 4 | cha DBID long |

**Character registry minPayload additions:**

| Opcode | min | Notes |
|--------|-----|-------|
| CMD_CM_FORGE | 1 | forge index char |
| CMD_CM_ITEM_FORGE_ASK | 1 | cancel/confirm char |
| CMD_CM_ITEM_FORGE_ASR | 1 | answer char |
| CMD_CM_REQUESTTALK | 4 | NPC/world ID long |
| CMD_CM_REQUESTTRADE | 4 | NPC ID long |
| CMD_CM_CHARTRADE_REQUEST | 5 | type char + cha ID long |
| CMD_CM_CHARTRADE_ACCEPT | 5 | type char + cha ID long |
| CMD_CM_CHARTRADE_REJECT | 0 | empty body |
| CMD_CM_CHARTRADE_CANCEL | 5 | type char + cha ID long |

**Build (2026-06-27):** `Common.lib` compile **PASS**; `GateServer.exe` / `GameServer.exe` / `GroupServer.exe` / `AccountServer.exe` compile **PASS** (`CharacterPrl.cpp`, `GameAppNet.cpp`, `ToClient.cpp`, `AccountServer2.cpp`, `OpcodeIngress.cpp`), link **BLOCKED** (`LNK1104` — exes locked by running server processes). Stop servers and relink to refresh binaries.

**Manual test checklist:** login (RSA + CM_LOGIN) → enter → move → chat → player trade (request/accept/cancel) → NPC trade (`CMD_CM_REQUESTTRADE`) → forge ask/asr → party invite/accept (CP) → friend invite → grep for spurious `OpcodeIngress [reject]` on Gate/Game/Group/Account.

**Recommended batch 4:** Character chartrade item/money/validate PacketReader; GroupServer CP say/session opcodes min map; Gate BGNPLAY/NEWCHA fixed-header mins; AccountServer PA string-handler PacketReader; optional `OpcodeDispatchDomain::Group` registry for MP hot path; Track C zero-copy gate forwarding pilot.

### Track E — OpcodeIngress + PacketReader (batch 4, 2026-06-27)

**Character chartrade PacketReader (4 handlers):**
- `CmChartradeItem`, `CmChartradeMoney`, `CmChartradeValidatedata`, `CmChartradeValidate`
- Registry minPayload: ITEM=9, MONEY=7 (fixed header before currency-specific tail), VALIDATEDATA/VALIDATE=5

**GroupServer CP session min map (ProcessData path):**

| Opcode | min | Notes |
|--------|-----|-------|
| CMD_CP_SESS_CREATE | 1 | player count char |
| CMD_CP_SESS_SAY | 4 | session ID long before string |
| CMD_CP_SESS_ADD | 4 | session ID long before string |
| CMD_CP_SESS_LEAVE | 4 | session ID long |
| CMD_CP_SAY2* | 0 | string-led (unchanged) |

**Gate sync/login non-empty body check:**
- `LookupGateSyncMinPayload`: `CMD_CM_BGNPLAY`, `CMD_CM_NEWCHA`, `CMD_CM_DELCHA` → min=1 (reject empty body; strings remain variable)

**AccountServer PA PacketReader:**
- `CMD_PA_CHANGEPASS`, `CMD_PA_REGISTER`, `CMD_PA_GMBANACCOUNT`, `CMD_PA_GMUNBANACCOUNT` — typed string reads via `net::PacketReader`

**Build (2026-06-27):** stop running server exes before link.

**Manual test checklist:** login → char select (BGNPLAY) → new/del char if used → player trade full cycle (item + money + validate) → CP session chat → grep `OpcodeIngress [reject]` during normal play.

**Recommended batch 5:** Gate BGNPLAY/NEWCHA PacketReader pre-check (non-consuming duplicate path); Group MP hot-opcode min map; Character remaining READ_* handlers; optional `OpcodeDispatchDomain::Group` registry pilot; Phase 3 exit soak + commit Track E batches 1–4.

### Track E — OpcodeIngress + PacketReader (batch 5, 2026-06-27)

**Gate sync path pre-check (non-consuming):**
- `ProbeSyncClientStrings` on duplicate buffer before GroupServer SyncCall
- `CM_BGNPLAY`: require readable char name string
- `CM_NEWCHA`: require readable name + birth strings
- Fail-closed: `CMD_MC_*` + `ERR_MC_NETEXCP` without consuming forward buffer

**GroupServer MP hot-opcode min map (ProcessData path):**

| Opcode | min | Notes |
|--------|-----|-------|
| CMD_MP_ENTERMAP | 1 | switch char |
| CMD_MP_MASTER_FINISH | 4 | prentice cha ID long |
| CMD_MP_GUILD_APPROVE / KICK / CREATE | 4 | leading long |
| CMD_MP_GUILD_CHALLMONEY / CHALL_PRIZEMONEY | 12 | long + longlong before strings |
| CMD_MP_CANRECEIVEREQUESTS | 6 | cha ID long + short |
| CMD_MP_TEAM_CREATE / MASTER_* | 0 | string-led |

**Character PacketReader (~17 handlers):**
- Boat/repair/fight: `CmBoatLuanch`, `CmBoatSelect`, `CmBoatBagsel`, `CmTeamFightAsk/Asr`, `CmItemRepairAsk/Asr`, `CmSkillupgrade`
- Crafting: `CmItemForgeCanaction`, `CmValidateSlotItem`, `CmEntityEvent` (forwards remainder via `reader.Raw()`)
- Guild: `CmGuildTryfor`, `CmGuildTryforcfm`, `CmGuildApprove`, `CmGuildReject`, `CmGuildKick`

**Manual test checklist:** login → BGNPLAY/NEWCHA → boat NPC if available → item repair → guild approve/kick → MP enter map (char switch) → grep `OpcodeIngress [reject]`.

**Recommended batch 6:** Character store/volunteer/lifeskill PacketReader tail; Group CP team-kick/friend-refuse mins; Gate DELCHA probe; 30-min Phase 3 exit soak.

### Track E — OpcodeIngress + PacketReader (batch 6, 2026-06-27)

**Gate DELCHA pre-check:**
- `ProbeDelChaPayload` — char name string + minimum encrypted sequence header on duplicate buffer
- Fail-closed: `CMD_MC_DELCHA` + `ERR_MC_NETEXCP` before SyncCall

**Group CP min map additions:**

| Opcode | min | Notes |
|--------|-----|-------|
| CMD_CP_TEAM_KICK | 4 | kicked cha ID long |
| CMD_CP_FRND_REFUSE | 4 | inviter cha ID long |
| CMD_CP_FRND_DELETE | 4 | deleted friend cha ID long |

**Character PacketReader (~14 handlers + registry mins):**
- Tiger: `CmTigerStart` (10), `CmTigerStop` (6)
- Store: `CmStoreOpenAsk` (string via reader; remainder to Lua via `reader.Raw()`)
- Lifeskill: `CmLifeskillAsk`, `CmLifeskillAsr` (type+npc+grids; preserves case fallthrough)
- Volunteer: `CmVolunterOpen` (2), `CmVolunterList` (4), `CmVolunterSel`, `CmVolunterAsr` (2+string)
- Misc: `CmItemLockAsk` (1), `CmGameRequestPin` (string)

**Manual test checklist:** login → del char (if used) → IGS store open/buy → tiger game → lifeskill NPC → volunteer list → party kick / friend delete → grep `OpcodeIngress [reject]`.

**Recommended batch 7:** Master/prentice CM handlers; guild disband/motto PacketReader; remaining `BeginAction`/movement tail; Phase 3 30-min soak sign-off.

### Track E — OpcodeIngress + PacketReader (batch 7, 2026-06-27)

**Character PacketReader (~17 handlers + registry mins):**
- Master/prentice: `CmMasterInvite`, `CmMasterAsr`, `CmMasterDel`, `CmPrenticeInvite`, `CmPrenticeAsr`, `CmPrenticeDel` (string+long or short+string+long)
- Guild: `CmGuildPutname` (1), `CmGuildPerm` (8), `CmGuildDisband`, `CmGuildMotto`, `CmGuildChallenge` (5), `CmGuildLeizhu` (5)
- Social/misc: `TmChangePersoninfo` (2), `CmSay2camp`, `CmPkCtrl` (1), `PmPushToGuildbank`
- Movement: `CmBeginAction` now passes `reader.Raw()` to `BeginAction` (world ID consumed first)

**Registry minPayloadBytes additions:**

| Opcode group | min | Notes |
|--------------|-----|-------|
| CM_MASTER/PRENTICE invite+del | 4 | char ID long |
| CM_MASTER/PRENTICE asr | 2 | response short |
| TM_CHANGE_PERSONINFO | 2 | icon short |
| CM_GUILD_PUTNAME | 1 | confirm char |
| CM_GUILD_PERM | 8 | target + permission longs |
| CM_GUILD_CHALLENGE/LEIZHU | 5 | level char + money long |
| CM_PK_CTRL | 1 | mode char |

Build: Release\|x64 **PASS** (`GameServer.vcxproj`, `CharacterPrl.cpp` compiled and linked).

**Manual test checklist:** login → master/prentice invite+accept → guild create/motto/disband → camp chat on CTF/guild war map → PK toggle → person info icon → grep `OpcodeIngress [reject]`.

**Recommended batch 8:** Remaining READ_* tail (stall, bank, GM send, cheat check, lottery); Phase 3 30-min soak sign-off.

### Track E — OpcodeIngress + PacketReader (batch 8, 2026-06-27)

**Character PacketReader (~12 handlers/helpers + registry mins):**
- Guild bank: `HandlePmGuildBank` (bankType char + item/gold sub-ops)
- Chat: `CmSay` (sequence via `reader.Raw().ReadSequence`)
- Lottery: `CmItemLotteryAsk` (confirm char + grid groups; mirrors forge pattern)
- Kitbag: `CmItemUnlockAsk` (password string + slot char; inlined from `ItemUnlockRequest`)
- Hair: `Cmd_ChangeHair` (script ID short + per-item grid shorts)
- GM/IGS: `CmGmSend` (npc long + title/content strings), `CmGmRecv` (npc long)
- Anti-cheat: `CmCheatCheck` (answer string)
- Auction: `CmBidup` (npc long + item short + price long, YORN-gated)
- Misc: `CmRequestChestPreview` (chest item ID long)

**Registry minPayloadBytes additions:**

| Opcode | min | Notes |
|--------|-----|-------|
| CMD_PM_GUILDBANK | 1 | bankType char |
| CMD_CM_GM_SEND | 4 | npc ID long before strings |
| CMD_CM_GM_RECV | 4 | npc ID long |
| CMD_CM_CHEAT_CHECK | 0 | variable string |
| CMD_CM_BIDUP | 0 | YORN script gate before fixed header |
| CMD_CM_ITEM_LOTTERY_ASK | 1 | confirm char |
| CMD_CM_REQUEST_CHEST_PREVIEW | 4 | chest item ID long |
| CMD_CM_UPDATEHAIR | 2 | hair script ID short |
| CMD_CM_STALL_OPEN | 4 | staller char ID long |
| CMD_CM_STALL_BUY | 7 | char ID + grid char + count short |

**Skipped (no READ_* in CharacterPrl handler body):**
- `CmStallAlldata`, `CmStallClose` — delegate full `RPacket` to `CharStall.cpp` (`packet.ReadString`/`ReadLong`, not READ_* macros here)
- `CmCreateBoat`, `CmUpdateboatPart` — delegate to `CharBoat.cpp`
- `CmSynAttr`, `CmEndAction`, `CmKitbagtempSync` — delegate remainder to existing methods/Lua

**Remaining READ_* in CharacterPrl.cpp (pre-batch 9):** ~58 occurrences, all in `BeginAction()` action sub-switch — **migrated in batch 9**.

Build: Release\|x64 **PASS** (`GameServer.vcxproj`, `CharacterPrl.cpp` compiled and linked).

**Manual test checklist:** login → chat → stall open/buy → guild bank deposit/withdraw → hair change → GM send/recv (IGS) → lottery → chest preview → grep `OpcodeIngress [reject]`.

**Recommended next:** Commit batches 6–9; 30-min Phase 3 exit soak.

### Track E — OpcodeIngress + PacketReader (batch 9, 2026-07-01)

**BeginAction sub-switch PacketReader migration (`CharacterPrl.cpp`):**
- `BeginAction(RPACKET pk)` — single `net::PacketReader` after pre-checks; all action subtype reads migrated from READ_* macros
- **55 READ_* migrated** across action cases: MOVE (sequence), SKILL (move path + sequence + skill/target longs), STOP_STATE, LEAN, ITEM_PICK/THROW/LOCK/UNLOCK/USE/UNFIX/POS/DELETE/INFO, BANK, REQUESTGUILDLOGS, SHORTCUT, TEMP, EVENT, FACE, SKILL_POSE, PK_CTRL
- **2 READ_SEQ** → `reader.Raw().ReadSequence()` (MOVE + SKILL move-then-cast path)
- Fail-closed: truncated payload → early `return` (void handler; matches registry handler style)
- `enumACTION_ITEM_INFO` passes `reader.Raw()` to `ViewItemInfo()` (helper still uses READ_* in `Character.cpp`)

**Registry minPayloadBytes update:**

| Opcode | min | Notes |
|--------|-----|-------|
| CMD_CM_BEGINACTION | 5 | world ID long + action type char (was 4) |

**Remaining READ_* after batch 9:**
- `CharacterPrl.cpp`: **0 active** (2 commented-out in `enumACTION_LOOK` stub)
- `Character.cpp`: **3** in `ViewItemInfo()` (delegated from `enumACTION_ITEM_INFO`)

**Skipped (no active reads):**
- `enumACTION_LOOK` — client payload reads commented out; broadcast stub empty
- `enumACTION_CLOSE_BANK`, `enumACTION_REQUESTGUILDBANK`, `enumACTION_UPDATEGUILDLOGS` — no packet reads

Build: Release\|x64 **PASS** (`GameServer.vcxproj`, `CharacterPrl.cpp` compiled and linked).

**Gate follow-up:** `ToClient.cpp` `CMD_CM_BEGINACTION` registry min aligned **4 → 5** (matches Game + ingress table). GateServer Release\|x64 **PASS**.

**Manual test checklist:** login → move (CM_BEGINACTION MOVE) → skill with move path → item pick/use/throw/pos → bank deposit/withdraw in action → guild bank open → shortcut bar → temp appearance → event NPC → face/sit pose → PK toggle → grep `OpcodeIngress [reject]` during normal play.

**Recommended next:** Commit batches 6–10; 30-min Phase 3 exit soak.

### Track E — OpcodeIngress + PacketReader (batch 10, 2026-07-01)

**ViewItemInfo PacketReader (`Character.cpp`):**
- `CCharacter::ViewItemInfo` — view type char + grid short (bag) or trade index short (trade paths)
- Fail-closed: truncated payload → `return FALSE`
- Called from `BeginAction` `enumACTION_ITEM_INFO` via `reader.Raw()` (action type already consumed)

**Remaining READ_* in Character handler paths:**
- `CharacterPrl.cpp`: **0 active** (2 commented-out in `enumACTION_LOOK` stub)
- `Character.cpp` (`ViewItemInfo`): **0** (migrated)

Build: Release\|x64 **PASS** (`GameServer.vcxproj`, `Character.cpp` compiled and linked).

**Manual test checklist:** login → inspect bag item → inspect trade boat item (if available) → grep `OpcodeIngress [reject]`.

**Recommended next:** Commit batches 6–10; **Track A phase 5** (TP SyncCall session migration); 30-min Phase 3 exit soak; Track C (optional).

### Track B5 — GameAppNet registry (batch 1, 2026-06-27)

`CGameApp::ProcessPacket` now tries `DispatchOpcodeHandler` first (via `GameAppPacketContext { app, gate }`); unmigrated opcodes fall through to legacy switch + default router.

Registered handlers (10):

| Opcode | # | Handler |
|--------|---|---------|
| CMD_TM_LOGIN_ACK | 1005 | Gate login ack / name bind |
| CMD_TM_MAPENTRY_NOMAP | 1008 | No-op (no map entry) |
| CMD_PM_TEAM | 4501 | ProcessTeamMsg |
| CMD_PM_GUILDINFO | 4504 | ProcessGuildMsg |
| CMD_PM_GUILD_CHALLMONEY | 4505 | ProcessGuildChallMoney |
| CMD_TM_MAPENTRY | 1007 | ProcessDynMapEntry |
| CMD_PM_GARNER2_UPDATE | 4507 | ProcessGarner2Update |
| CMD_PM_SAY2ALL | 4509 | World chat relay |
| CMD_PM_SAY2TRADE | 4510 | Trade chat relay |
| CMD_PM_GUILD_CHALL_PRIZEMONEY | 4506 | Forward to MM guild chall prize |

**Context pattern:** `GameAppPacketContext { CGameApp* app; GateServer* gate; }` passed as `void* ctx` to `OpcodeHandlerFn` (Gate ToClient uses `ToClient*` directly; GameApp needs both app and gate).

Files: `GameAppNet.cpp`, `GameApp.h`, `GameApp.cpp` (`RegisterGameAppOpcodeHandlers` after `RegisterCharacterOpcodeHandlers`).

Build: Release\|x64 **PASS** (`GameServer.vcxproj`, GameAppNet.cpp compiled and linked).

**Legacy switch:** 3 explicit cases remain (`CMD_TM_ENTERMAP`, `CMD_TM_GOOUTMAP`, `CMD_PM_EXPSCALE`) + `default` router (MM band → `ProcessInterGameMsg`, CM/TM player → `CCharacter::ProcessPacket`).

Blockers: none for batch 2.

**Recommended batch 2 (~3 top-level + pilot):** `CMD_TM_ENTERMAP`, `CMD_TM_GOOUTMAP` (critical enter/leave path — test T0-enter carefully), `CMD_PM_EXPSCALE` (nested switch). Then begin `ProcessInterGameMsg` MM cases (guild bank sync, GM query/kick, ADDCREDIT/ADDMONEY — lower coupling than enter/leave).

### Track B5 — GameAppNet registry (batch 2, 2026-06-27)

Added 13 handlers (23 total on registry):

| Opcode | # | Handler |
|--------|---|---------|
| CMD_TM_ENTERMAP | 1003 | Enter map / create player (T0-enter path) |
| CMD_TM_GOOUTMAP | 1004 | Leave map / logout / offline stall |
| CMD_PM_EXPSCALE | 4511 | Anti-indulgence exp scale (nested ulTime switch) |
| CMD_MM_UPDATEGUILDBANK | 4023 | Sync guild bank kitbag to members |
| CMD_MM_UPDATEGUILDBANKGOLD | 4024 | Broadcast guild bank gold |
| CMD_MM_GUILD_MOTTO | 4014 | Guild motto sync (lSrcID = guild ID) |
| CMD_MM_GUILD_DISBAND | 4011 | Guild disband broadcast |
| CMD_MM_GUILD_KICK | 4010 | Guild kick single player |
| CMD_MM_GUILD_APPROVE | 4009 | Guild approve applicant |
| CMD_MM_GUILD_REJECT | 4008 | Guild reject applicant |
| CMD_MM_ADDCREDIT | 4019 | Add player credit |
| CMD_MM_ADDMONEY | 4021 | Add player gold |
| CMD_MM_NOTICE | 4013 | LocalNotice broadcast |

**ProcessInterGameMsg dispatch:** Option B — same `RegisterGameAppOpcodeHandlers()` registry. After reading MM header (`lSrcID`, `sNum`, `lGatePlayerAddr`, `lGatePlayerID`), `ProcessInterGameMsg` builds extended `GameAppPacketContext { app, gate, lSrcID, lGatePlayerID, lGatePlayerAddr }` and calls `DispatchOpcodeHandler` before legacy switch. Guild handlers that used `lSrcID` read it from context; `ProcessPacket` passes zeros for inter-game fields.

**ProcessPacket legacy switch:** No explicit cases remain — only `default` router (MM band → `ProcessInterGameMsg`, CM/TM player → `CCharacter::ProcessPacket`).

Files: `GameAppNet.cpp`, `GameApp.h` (same init path as batch 1).

Build: Release\|x64 **PASS** (`GameAppNet.cpp` compiled and linked → `source/bin/Release/gameserver/GameServer.exe`).

Blockers: none for batch 3. **Manual T0-enter / logout test recommended** for ENTERMAP/GOOUTMAP.

**Hotfix (2026-06-27):** B5 broke movement/chat — `OpcodeHandlerRegistry` was a single global table; `GameApp::ProcessPacket` dispatched CM/PM packets through Character handlers with wrong `ctx`. Fixed by `OpcodeDispatchDomain` (Gate / GameCharacter / GameApp) isolated registries.

**Recommended batch 3 (~10):** `ProcessInterGameMsg` GM/query group — `CMD_MM_QUERY_CHAPING`, `CMD_MM_QUERY_CHA`, `CMD_MM_QUERY_CHAITEM`, `CMD_MM_CALL_CHA`, `CMD_MM_GOTO_CHA`, `CMD_MM_KICK_CHA`, `CMD_MM_CHA_NOTICE`, `CMD_MM_DO_STRING`, `CMD_MM_LOGIN`, `CMD_MM_GUILD_CHALL_PRIZEMONEY` (uses lSrcID/lGatePlayerAddr from context).

### Track B5 — GameAppNet registry (batch 3, 2026-06-27)

Added 12 handlers (35 total on registry) — **final ProcessInterGameMsg switch cleanup**:

| Opcode | # | Handler |
|--------|---|---------|
| CMD_MM_QUERY_CHAPING | 4012 | GM ping query |
| CMD_MM_QUERY_CHA | 4003 | GM character location query |
| CMD_MM_QUERY_CHAITEM | 4004 | GM kitbag query |
| CMD_MM_CALL_CHA | 4005 | GM summon character |
| CMD_MM_GOTO_CHA | 4006 | GM goto character (2-phase) |
| CMD_MM_KICK_CHA | 4007 | GM kick character |
| CMD_MM_CHA_NOTICE | 4016 | GM notice (local or targeted) |
| CMD_MM_DO_STRING | 4015 | GM Lua dostring |
| CMD_MM_LOGIN | 4017 | After-player-login hook |
| CMD_MM_GUILD_CHALL_PRIZEMONEY | 4018 | Guild challenge prize payout |
| CMD_MM_STORE_BUY | 4020 | IGS store buy accept |
| CMD_MM_AUCTION | 4022 | Auction end script |

**ProcessInterGameMsg legacy switch:** Only `default: break;` remains — all MM opcodes dispatched via registry after MM header read + `GameAppPacketContext`.

**ProcessPacket legacy switch:** No explicit cases — only `default` router (MM band → `ProcessInterGameMsg`, CM/TM player → `CCharacter::ProcessPacket`).

Files: `GameAppNet.cpp`, `GameApp.h` (same init path as batch 1–2).

Build: Release\|x64 compile **PASS** (`GameAppNet.cpp`); link blocked (GameServer.exe in use).

Blockers: none. **Track B5 complete** — both ProcessPacket and ProcessInterGameMsg legacy switches empty (`default` only).

---

## Known regressions

- (none recorded)

---

## Last smoke test run

```
2026-06-26 15:48:13 — T0-connect PASS, T0-ping PASS (127.0.0.1:1973)
2026-06-27 — T0-login, T0-enter, chat PASS (manual, post 665a8778)
2026-07-01 — Track A phase 5 T0 PASS: login, BGNPLAY, enter map, ENDPLAY, char switch, logout/re-login
See last-smoke-result.txt for automated output.
```

---

## Resume prompt for next session

> Continue **Audit Remediation** — read [`AUDIT_REMEDIATION_PLAN.md`](AUDIT_REMEDIATION_PLAN.md). **R1–R2.2 + R3 + R5 complete.** Next: optional **R2.3** or **R4.1** (PM broadcast) or **R6.1** (30-min soak + Phase 3 exit). **Manual T0 pending for R5:** login → enter → ENDPLAY → char switch; **Group restart soak** for TP_SYNC_PLYLST session restore. Deploy matched Gate+Group (+Game for R5.2) from same build.
