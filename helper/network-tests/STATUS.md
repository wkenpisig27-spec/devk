# Option B / Phase-6 — Execution Status

Track progress for autonomous refactoring. Update after each completed task.

**Current phase:** Phase 3 — Medium-term core (Tracks A–E)  
**Last updated:** 2026-06-27  
**Master audit:** [`docs/NETWORK_AUDIT.md`](../../docs/NETWORK_AUDIT.md)

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
| A | M3 Session handles (slot + generation) | **in_progress** | Phase 1 prototype: SessionManager, gate alloc at EnterMap, TM_ENTERMAP extension, dual validation |
| B | M2 Bulk migration — Gate lifecycle (B1) | mostly done | SyncCall on comm thread covers login/BGNPLAY |
| B | M2 Bulk migration — GameAppNet (B5) | **done** | batch 3: 35 handlers total (2026-06-27) |
| B | M2 Bulk migration — CharacterPrl (B6) | **done** | 2026-06-27 — 112 handlers; legacy switch empty |
| C | M6 Zero-copy gate forwarding | pending | Drop `Duplicate()` in `ReRouteToGameServer` |
| D | M4 Backplane PSK auth | pending | Gate↔Game/Group/Account |
| E | M5 PacketReader everywhere | pending | After each B-batch migration |

**Phase 3 exit gate:** Legacy switches empty or assert-only; session handles primary; backplane auth in default configs; 30-min soak clean.

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
- `ToClient::ReRouteToGameServer` — log session (pointer trailer unchanged)
- Game `GatePlayer::m_sessionHandle`, per-gate `SessionManager`, `BindPlayerSession` / `ResolveSession`
- `OpcodeHandle_TmEntermap` — read extended trailer, bind session after `ADDPLAYER`
- Dual validation in `ValidatePlayerPointer` + gate `ValidatePlayerPointer`

**Build (2026-06-27):** `Common.lib` **PASS**; GateServer + GameServer **compile PASS**; link blocked (`GateServer.exe` / `GameServer.exe` in use).

**Manual test:** T0-login → T0-enter → move/chat; grep logs for `SessionManager` (`Gate allocated`, `TM_ENTERMAP`, `ReRoute`, `GameServer bound`).

**Phase 2 remains:** repurpose 8-byte CM trailer to `{slot, gen}`; remove pointer registry path; GroupServer session sync.

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
See last-smoke-result.txt for automated output.
```

---

## Resume prompt for next session

> Continue Option B Phase 3. **Track B6 done** (112 CharacterPrl registry entries). **Track B5 done** (35 GameAppNet handlers). **Track A phase 1 in progress** (session handles prototype). Next: Track A phase 2 (trailer swap) or Track C (zero-copy gate forwarding). Read `docs/NETWORK_AUDIT.md` and this file. CM forward wire unchanged.
