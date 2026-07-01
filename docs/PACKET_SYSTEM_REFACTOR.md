# Packet System Refactor — Change Log & Architecture Guide

**Project:** `devk` (PKO private server + client)  
**Last updated:** 2026-06-27  
**Scope:** Option B / Phase 3 network modernization (wire-compatible incremental refactor)

This document describes **what changed** in the packet pipeline, dispatch system, session identity, and inter-server security — and **how the pieces fit together** after the refactor work completed through Track E batch 5.

For the original audit and roadmap, see [`NETWORK_AUDIT.md`](NETWORK_AUDIT.md).  
For day-to-day execution status, see [`helper/network-tests/STATUS.md`](../helper/network-tests/STATUS.md).

---

## Executive summary

We modernized a 2003-era `dbc` packet stack **without changing the on-wire protocol** visible to the game client. The work falls into five themes:

| Theme | What it solves | Status |
|-------|----------------|--------|
| **Sprint 1 hardening** | Unbounded reads, silent parse failures, weak limits | Done (Phase 1) |
| **OpcodeMeta + registry** | Giant `switch(cmd)` maintenance debt | Done on Gate + Game (Tracks B5/B6) |
| **Session handles** | Raw pointer identity on hot paths | **Done** (Track A phases 1–3 in-game + phase 5 TP SyncCall) |
| **Backplane PSK** | Plaintext inter-server TCP | Done (Track D) |
| **OpcodeIngress + PacketReader** | Fail-closed ingress + typed reads | **Done** (Track E batches 1–10) |

**Deferred:** None for Phase 3 core tracks.

---

## Track C — Gate forward without recv-buffer sharing (M6, 2026-07-01)

**Problem:** `ReRouteToGameServer` / `ReRouteToGroupServer` used `WPacket(recvbuf).Duplicate()` then appended a 16-byte session trailer. `Duplicate()` allocated exactly `GetPktLen()` bytes, so each `WriteLong`/`WriteLongLong` often triggered a **grow + recopy** (up to 3 extra copies per forward).

**Unsafe alternative (not done):** `WPacket(recvbuf)` without copy shares the receive `rbuf`; appending the trailer **mutates the live recv buffer**.

**Solution:** `WPacket::ForwardFromReceive(rpk, NetLimits::kGateSessionTrailerBytes)` — one allocation sized `pktLen + 16`, one `MemCpy`, trailer append without realloc.

**Scope:** In-game `ReRouteToGameServer` / `ReRouteToGroupServer` only. SyncCall paths (login, BGNPLAY) still use `Duplicate()` — they may change cmd/body and must not touch `recvbuf`.

**Files:** `Packet.h/cpp`, `NetLimits.h`, `ToClient.cpp`

---

## What did *not* change

Understanding boundaries is as important as the new code:

- **Client wire format** — frame layout, opcode numbers, RSA/AES login, CM/CP/MC/PC bands unchanged.
- **16-byte gate trailers** — still `{identity, addr}` = 8 + 8 bytes; identity field *meaning* changed internally (session slot/gen vs raw pointer) but **size and position are the same**.
- **Login / char-select SyncCall path** — session trailers on all TP_* after login bind (Track A phase 5, 2026-07-01).
- **Game logic thread model** — single `PKQueue` game thread preserved.
- **Track C** — ReRoute uses `ForwardFromReceive` (one alloc + one copy); SyncCall paths still `Duplicate()`.

---

## Architecture: before vs after

### Before (circa pre-2026 audit)

```
Client ──► Gate OnProcessData
              ├── switch(cmd) / implicit band math (cmd/500)
              ├── READ_* macros (no RemainData check)
              ├── ReRouteToGameServer: Duplicate() + WriteLongLong(gate_ptr)
              └── Game: giant switch in CharacterPrl / GameAppNet
Inter-server links: plaintext TCP, pointer identity on wire
```

### After (2026-06-27)

```
Client ──► Gate OnProcessData
              ├── ValidateClientToGateOpcode / ValidateGateSyncClientOpcode
              ├── OpcodeHandlerRegistry (Gate domain) → handler
              ├── ReRouteToGameServer: session trailer {slot, gen, gm_addr}
              └── SyncCall opcodes on comm thread (login, BGNPLAY, …)

GameServer ──► ProcessPacket
              ├── ValidateGameCharacterOpcode / ValidateGameAppOpcode
              ├── OpcodeHandlerRegistry (GameCharacter / GameApp domains)
              └── Session resolve from trailer (CM/TM/PM in-game)

GroupServer ──► ValidateGroupIngressOpcode (ServeCall + ProcessData)
AccountServer ──► BackplaneAuth + ValidateAccountIngressOpcode

Inter-server: HMAC-SHA256 PSK hello (CMD_OS/SO_BACKPLANE_HELLO) before normal traffic
```

### Server topology (unchanged)

```
AccountServer ◄──PA/AP──► GroupServer ◄──TP/PT, PM/MP──► GateServer ◄──CM/MC, CP/PC──► Client
                                                              ▲
                                                              └── TM/MT ── GameServer(s)
```

---

## Commit timeline (network refactor)

| Commit | Summary |
|--------|---------|
| `304926a0` | Phase 0 baseline + Phase 1 Sprint 1 (I1–I7) |
| `53baee2c` | Phase 2 foundation — OpcodeMeta, PacketReader/Writer, registry pilots |
| `665a8778` | Hotfix — 32 KB client packet limits; SyncCall on comm thread for login/BGNPLAY |
| `c7dfce09` | Track B6 — CharacterPrl → opcode registry (112 handlers) |
| `273ecf7b` | Track B5 — GameAppNet → registry (35 handlers); isolated dispatch domains |
| `39b9b1ad` | Track A phase 1 — SessionHandle + SessionManager |
| `d7e3d6ae` | Track A phase 2 — CM forward session trailer |
| `4608d69a` | Track A phase 2b — Gate↔Group session trailer |
| `cf43da06` | Track A phase 3 — TM/PM game paths session trailer |
| `32a45fdb` | Track D — BackplaneAuth PSK on all inter-server links |
| `5b4d7f2e` | Track E batches 1–4 — OpcodeIngress + PacketReader rollout |
| `1e2585f4` | Track E batch 5 — Gate BGNPLAY/NEWCHA probe, MP mins, boat/guild PacketReader |

---

## Phase 1 — Sprint 1 hardening (foundation)

These changes predate the registry/ingress work but are part of the packet story:

| ID | Change | Key files |
|----|--------|-----------|
| **I4** | Read contract — callers must check `ReadString`/`ReadSequence` null | `Packet.cpp`, `PacketReadUtils.h` |
| **I5** | Fail-closed on parse exceptions | `PacketPipeline.h`, `Receiver.cpp` |
| **I2** | Recv alloc caps, inter-server limits | `NetLimits.h`, `Receiver.cpp` |
| **I1** | Connection + traffic rate limiting | `ToClient.cpp` |
| **I6** | AES-GCM wire integrity option | `PacketEncryption.cpp`, `CommEncrypt=2` |
| **I7** | `volatile` → `atomic` on hot socket flags | `DataSocket` |

**Production lesson:** Login responses can exceed 16 KB. Client limits were raised to 32 KB in `NetLimits.h`. SyncCall handlers (login, BGNPLAY, NEWCHA, …) **must run on the comm thread** — queued handlers let `recvbuf` age past the 10s deadline → `ERR_MC_NETEXCP`.

---

## Phase 2 — Opcode metadata & handler registry

### OpcodeMeta (M1)

Machine-readable opcode inventory drives band checks and ingress validation.

| File | Role |
|------|------|
| `source/include/common/OpcodeMeta.h` | `OpcodeBand`, `LookupOpcodeMeta`, `OpcodeName` |
| `source/include/common/OpcodeMetaTable.inc` | Generated/static table (~all non-base opcodes) |
| `source/src/common/OpcodeMeta.cpp` | Lookup implementation |
| `helper/network-tests/data/opcodes.csv` | CSV export from `scripts/generate_opcode_table.py` |

**Bands** (from `NetCommand.h`): CM/CP (client), TM/PM/MM (game), TP/PT (group), PA/AP (account), OS/SO (monitor/internal).

### PacketReader / PacketWriter (M5-prep)

Typed, bounds-checked reads/writes over `RPacket`/`WPacket` — **does not consume the cmd field** (call site reads cmd first).

```cpp
net::PacketReader reader(pk);
uLong worldId = 0;
if (!reader.Long(worldId)) {
    return true; // fail-closed at handler level
}
// Forward remainder to legacy code:
legacyHandler(reader.Raw());
```

| File | Role |
|------|------|
| `source/include/common/PacketReader.h` | Char, Short, Long, LongLong, String, Raw() |
| `source/include/common/PacketWriter.h` | Symmetric write helpers |
| `source/include/common/PacketReadUtils.h` | Shared string read contract |

### OpcodeHandlerRegistry (M2)

Replaces per-file `switch(cmd)` with sorted tables and binary search.

```cpp
struct OpcodeHandlerEntry {
    uint16_t opcode;
    OpcodeHandlerFn handler;
    const char* name;
    uint16_t minPayloadBytes = 0;  // Track E
};

enum class OpcodeDispatchDomain : uint8_t {
    Gate,
    GameCharacter,
    GameApp,
    Count
};
```

**Critical fix (B5):** A single global registry caused GameApp to dispatch CM packets through Character handlers. **Isolated domains** per process area fixed movement/chat regressions.

| Domain | Registered handlers | Legacy switch |
|--------|---------------------|---------------|
| **Gate** | All bulk-routed CM/CP | Explicit SyncCall + RSA/login only |
| **GameCharacter** | 112 (`CharacterPrl.cpp`) | Empty — `default` only |
| **GameApp** | 35 (`GameAppNet.cpp`) | Empty — `default` router to MM / Character |

**Dispatch flow:**

1. Read cmd from packet.
2. Ingress validation (Track E — see below).
3. `DispatchOpcodeHandler(domain, cmd, ctx, sock, pk)`.
4. If no handler → legacy path / default router.

---

## Track A — Session handles (M3)

### Problem

Gate forwarded in-game packets with **raw `Player*` / gate pointer** in the trailer. Stale or forged pointers were a class of exploit; validation relied on a side registry alone.

### Solution

**SessionHandle** — `{slot, generation}` (8 bytes on wire: two `WriteLong`s).

| File | Role |
|------|------|
| `source/include/common/SessionHandle.h` | Handle type |
| `source/include/common/SessionManager.h` | Allocate, bind, resolve, validate |
| `source/src/common/SessionManager.cpp` | Implementation in `Common.lib` |

### Trailer layout (16 bytes — unchanged size)

**Gate → Game (in-game CM/TM/PM):**
```
WriteLong(slot) + WriteLong(generation) + WriteLongLong(gm_addr)
Reverse read: gm_addr, generation, slot → ResolvePlayerFromGateTrailer
```

**Gate → Group (TP SyncCall + in-game CP/MP — unified, 2026-07-01):**
```
WriteLong(slot) + WriteLong(generation) + WriteLongLong(gp_addr)
Reverse read on Group: gp_addr, generation, slot → ResolvePlayerFromGateTrailer
```
Session bound at `TP_USER_LOGIN` success; SyncCall and in-game CP/MP use `AppendInGameGroupTrailer` (R1.3 removed thin wrapper).

**TM_ENTERMAP (enter world):** session appended to extended trailer; Game binds mirror on `GatePlayer`.

### Validation model

- **Gate→Group (SyncCall + in-game):** session bind at login; `ValidatePlayerSession` authoritative on bound players.
- **Gate→Game (in-game CM/TM/PM):** session bind at enter-map; `ResolvePlayerFromGateTrailer` on Game.
- **Fail-closed:** ReRoute and SyncCall reject without valid session after login.
- **Legacy pointer registry:** retained for `MP_ENTERMAP` fallback and internal MakePointer lookups only.

### Log grep strings

- `SessionManager ReRoute cmd=`
- `SessionManager CM session cmd=`
- `SessionManager Group MP_ENTERMAP bound`
- `SessionManager ReRoute REJECT` / `CM REJECT`

---

## Track D — Backplane PSK auth (M4)

### Problem

Account ↔ Group ↔ Gate ↔ Game links were **plaintext TCP**. Any host on the backplane network could inject PA/TP/MM packets.

### Solution

Mutual **HMAC-SHA256** handshake before normal protocol traffic.

| Opcode | Direction | Purpose |
|--------|-----------|---------|
| `CMD_OS_BACKPLANE_HELLO` (6510) | Initiator → Listener | PSK proof |
| `CMD_SO_BACKPLANE_HELLO` (7010) | Listener → Initiator | Ack |

| File | Role |
|------|------|
| `source/include/serversdk/BackplaneAuth.h` | API |
| `source/src/serversdk/BackplaneAuth.cpp` | HMAC, state machine |

**Config** (`server/*.cfg` section `[Backplane]` — one per server process: `GateServer.cfg`, `GameServer.cfg`, `GroupServer.cfg`, `AccountServer.cfg`):
```ini
RequireAuth=1
PSK=pko-backplane-dev-shared-secret   ; required when RequireAuth=1; rotate for production
HandshakeTimeoutMs=5000
; RequireAuth=1 with empty PSK → server refuses start (BackplaneAuth FATAL, exit 1)
; RequireAuth=0 → legacy accept (no backplane handshake)
```

**Integration points:**

| Link | Initiator | Listener |
|------|-----------|----------|
| Game → Gate | `GameServerApp::ConnectGate` | `ToGameServer::OnServeCall` |
| Gate → Group | `ConnectGroupServer` | `GroupServerApp::OnServeCall` |
| Group → Account | `InitACTSvrConnect` | `AccountServer2::OnServeCall` |

Wrong PSK → disconnect reason **-41**; grep `BackplaneAuth` logs.

**Deploy rule:** All four servers must share the same PSK when `RequireAuth=1`.

---

## Track E — OpcodeIngress + PacketReader (M5)

Central **fail-closed ingress** module in `Common.lib`, driven by OpcodeMeta + per-handler `minPayloadBytes`.

### Core API

| Function | Where used | Behavior |
|----------|------------|----------|
| `ValidateKnownOpcode` | All paths | Reject base opcodes (`isBase`) |
| `ValidateOpcodeBand` | All paths | cmd must match expected band |
| `ValidateMinPayload` | All paths | `RemainData() >= minBytes` after cmd consumed |
| `ValidateClientToGateOpcode` | Gate `OnProcessData` | CM/CP + registry/meta + handler min |
| `ValidateGateSyncClientOpcode` | Gate RSA/login/SyncCall set | Band + known opcode; variable mins |
| `ValidateGameCharacterOpcode` | `CCharacter::ProcessPacket` | CM band + handler min |
| `ValidateGameAppOpcode` | `CGameApp::ProcessPacket` / `ProcessInterGameMsg` | Only registered opcodes |
| `ValidateGroupIngressOpcode` | Group `OnServeCall` / `OnProcessData` | TP/PA/OS vs CP/MP paths |
| `ValidateAccountIngressOpcode` | Account `OnProcessData` / `OnServeCall` | PA band + known opcode |

**Reject logging:**
```
OpcodeIngress [reject] cmd=N name=… reason=… peer=…
OpcodeIngress [summary] rejects=N in last 60s
```

Gate may disconnect on reject (`-32`); Game/Group/Account drop packet.

### minPayloadBytes semantics

- **`0`** — skip size check (variable strings, encrypted bodies, empty OK).
- **`> 0`** — minimum bytes **after cmd** is read; rejects truncated malicious packets early.
- Set conservatively when fixed fields precede strings; never guess on variable layouts.

`DispatchOpcodeHandler` also enforces registry `minPayloadBytes` before invoking the handler.

### Track E batch summary

#### Batch 1 — Pilot
- New `OpcodeIngress.h/cpp` in Common.lib.
- Gate + GameCharacter + GameApp wired.
- Gate ~10 handler mins; PacketReader on `CmBeginAction`, `CmDieReturn`, etc.

#### Batch 2 — Expansion
- `ValidateGroupIngressOpcode` (ServeCall + ProcessData).
- GameApp TM/PM/MM mins (incl. `CMD_TM_GOOUTMAP`=17 — critical logout path).
- Character ping/stall/mission mins.
- Gate `CmKitbagUnlock` → PacketWriter rebuild path.
- 60s reject summary counter.

#### Batch 3 — Cross-server ingress
- **AccountServer** `ValidateAccountIngressOpcode` (replaces hardcoded 3000–3050).
- **Group CP hot map** — PING, TEAM_ACCEPT/REFUSE, FRND_ACCEPT (min=4).
- **Gate** `ValidateGateSyncClientOpcode` for RSA/login/TransmitCall.
- **Character** trade/forge PacketReader (~8 handlers).
- **GameApp** MM string-handler mins (SAY2ALL, STORE_BUY, AUCTION, …).

#### Batch 4 — Trade completion
- **Character** chartrade ITEM/MONEY/VALIDATEDATA/VALIDATE PacketReader.
- **Group CP session** mins — SESS_CREATE/SAY/ADD/LEAVE.
- **Gate** non-empty body check — BGNPLAY/NEWCHA/DELCHA (min=1).
- **Account** PA string handlers → PacketReader (CHANGEPASS, REGISTER, GM ban/unban).

#### Batch 5 — Lifecycle + MP
- **Gate** `ProbeSyncClientStrings` on **duplicate buffer** before SyncCall — validates BGNPLAY (1 string), NEWCHA (2 strings) without consuming forward buffer; `ERR_MC_NETEXCP` on failure.
- **Group MP min map** — ENTERMAP=1, MASTER_FINISH/GUILD approve/kick/create=4, CHALL money=12, CANRECEIVEREQUESTS=6.
- **Character** ~17 handlers → PacketReader — boat, repair, fight, skill, forge-canaction, validate-slot, entity-event (`reader.Raw()` to `MsgProc`), guild tryfor/approve/reject/kick.

### PacketReader migration pattern

**Simple fixed header:**
```cpp
bool CCharacter::OpcodeHandle_CmGuildApprove(void* ctx, DataSocket*, RPacket& pk) {
    net::PacketReader reader(pk);
    uLong chaId = 0;
    if (!reader.Long(chaId)) return true;
    Guild::cmd_GuildApprove(static_cast<CCharacter*>(ctx)->GetPlyMainCha(), chaId);
    return true;
}
```

**Fixed prefix + legacy tail:**
```cpp
bool CCharacter::OpcodeHandle_CmRequestTalkOrTrade(void* ctx, DataSocket*, RPacket& pk) {
    net::PacketReader reader(pk);
    uLong npcId = 0;
    if (!reader.Long(npcId)) return true;
    // remainder consumed by NPC handler via reader.Raw()
    ...
}
```

**Gate non-consuming probe:**
```cpp
WPacket copy = WPacket(recvbuf).Duplicate();
RPacket probe(copy);
net::PacketReader reader(probe);
// validate on probe; forward original recvbuf unchanged
```

---

## End-to-end packet flow (2026)

### Client login → enter world

1. **TCP** → Gate `:1973`
2. **RSA/AES handshake** — `ValidateGateSyncClientOpcode` (min=0)
3. **`CMD_CM_LOGIN`** — SyncCall on comm thread → Group → Account
4. **`CMD_CM_BGNPLAY`** — `ProbeSyncClientStrings` → `ValidateGateSyncClientOpcode` → SyncCall `TP_BGNPLAY`
5. Gate allocates **session handle**, kicks dupes (`CMD_TM_KICKCHA`), **`EnterMap`** → Game
6. **`CMD_MP_ENTERMAP`** — Group binds session mirror
7. In-world **CM** (move, chat, …) — session trailer on gate→game forward; `ValidateGameCharacterOpcode` + registry handler + PacketReader

### Client → Gate → Game (in-world CM)

```
OnProcessData
  → ValidateClientToGateOpcode(cmd, pk, peer)
  → DispatchOpcodeHandler(Gate, cmd, …)  OR  ReRouteToGameServer
       → AppendInGameGameTrailer(slot, gen, gm_addr)
  → Game ProcessPacket
       → ValidateGameCharacterOpcode
       → DispatchOpcodeHandler(GameCharacter, cmd, …)
       → PacketReader in handler
```

### Group CP (party/friend/chat)

```
OnProcessData
  → BackplaneAuth::AllowProcessData
  → ValidateGroupIngressOpcode(cmd, pk, peer, ProcessData)
       → LookupGroupProcessDataMinPayload (CP + MP maps)
  → switch → CP_TEAM_INVITE / CP_SESS_SAY / …
```

---

## Key source files (quick reference)

| Area | Files |
|------|-------|
| **Opcodes** | `common/NetCommand.h`, `OpcodeMeta.*`, `OpcodeMetaTable.inc` |
| **Registry** | `OpcodeHandlerRegistry.h/cpp` |
| **Ingress** | `OpcodeIngress.h/cpp` |
| **Typed I/O** | `PacketReader.h`, `PacketWriter.h`, `PacketReadUtils.h` |
| **Sessions** | `SessionHandle.h`, `SessionManager.h/cpp` |
| **Backplane** | `BackplaneAuth.h/cpp` |
| **Gate client path** | `gateserver/ToClient.cpp` |
| **Gate ↔ game** | `gateserver/ToGameServer.cpp` |
| **Gate ↔ group** | `gateserver/ToGroupServer.cpp` |
| **Game dispatch** | `gameserver/GameAppNet.cpp`, `CharacterPrl.cpp` |
| **Group dispatch** | `groupserver/GroupServerAppServ.cpp` |
| **Account** | `accountserver/AccountServer2.cpp` |
| **Transport** | `serversdk/Receiver.cpp`, `Packet.cpp`, `Comm.cpp` |
| **Limits** | `common/NetLimits.h` |

---

## Build & deploy

**Build:** Visual Studio 18, Release|x64. `Common.lib` must rebuild before server exes when `OpcodeIngress` or `SessionManager` changes.

**Stop before link:** `GateServer.exe`, `GameServer.exe`, `GroupServer.exe`, `AccountServer.exe` — otherwise LNK1104 (exe locked).

**Deploy sets:**

| Change | Minimum deploy |
|--------|----------------|
| Track E Gate only | Gate |
| Track E Game | Gate + Game |
| Track E Group ingress | + Group |
| Track D backplane | All four servers |
| Track A session | Gate + Game (+ Group for CP/MP path) |

**Start order:** Account → Group → Gate → Game.

Output paths:
- `source/bin/Release/gateserver/GateServer.exe`
- `source/bin/Release/gameserver/GameServer.exe`
- `source/bin/Release/groupserver/GroupServer.exe`
- `source/bin/Release/accountserver/AccountServer.exe`

---

## Testing checklist

### Automated
- `helper/network-tests/net-smoke.py` — T0 connect + ping

### Manual (after any packet-path change)
1. Login → char select → enter world
2. Move + chat
3. Logout / char switch (`CMD_TM_GOOUTMAP`)
4. Player trade full cycle (if touched chartrade handlers)
5. Party/friend CP (if touched Group ingress)
6. Grep logs — should be **empty** during normal play:
   - `OpcodeIngress [reject]`
   - spurious `SessionManager … REJECT`
   - `BackplaneAuth` failures (with correct PSK)

### Phase 3 exit gate (not yet formally signed off)
- [x] Legacy switches empty or registry-only (Game)
- [x] Session handles primary in-game
- [x] Backplane auth in default configs
- [ ] Track E complete (PacketReader tail remaining)
- [ ] 30-minute soak clean

---

## Remaining work

| Track | Item | Notes |
|-------|------|-------|
| **E batch 6+** | Master/prentice, guild disband/motto, movement tail | See STATUS.md batch 7 |
| **E** | Group CP team-kick / friend-refuse mins | Low risk |
| **E** | Gate DELCHA string probe | Encrypted password tail |
| **C** | Zero-copy gate forward | Deferred — ~0.02 KB/s at user load |
| **A** | Pointer registry cleanup on login path | Optional |
| **Phase 3** | 30-min soak + exit gate sign-off | Operational |

---

## Related documents

| Document | Purpose |
|----------|---------|
| [`NETWORK_AUDIT.md`](NETWORK_AUDIT.md) | Master audit Phases 1–6, comparison to modern MMORPG stacks |
| [`NETWORK_SECURITY.md`](NETWORK_SECURITY.md) | Broader network security notes |
| [`helper/network-tests/STATUS.md`](../helper/network-tests/STATUS.md) | Live execution tracker |
| [`helper/network-tests/BASELINE.md`](../helper/network-tests/BASELINE.md) | Ports, limits, smoke tests |
| [`helper/network-tests/data/opcodes.csv`](../helper/network-tests/data/opcodes.csv) | Opcode inventory |

---

## Glossary

| Term | Meaning |
|------|---------|
| **Band** | Opcode range family (CM, CP, TM, …) — roughly `cmd / 500` |
| **Trailer** | 16-byte gate-appended identity block on forwarded packets |
| **SyncCall** | RPC-style request/response on same TCP connection |
| **Ingress** | Validation at packet entry before handler logic |
| **minPayload** | Minimum body bytes required after cmd field |
| **Domain** | Isolated handler registry (Gate / GameCharacter / GameApp) |
| **PSK** | Pre-shared key for backplane HMAC handshake |
