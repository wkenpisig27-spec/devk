# Phase 0 — Network Baseline Inventory

Frozen snapshot for Option B / Phase-6 refactoring. Update only when protocol or build layout changes intentionally.

**Generated:** 2026-06-26 (limits updated 2026-06-27)  
**Audit report:** [`docs/NETWORK_AUDIT.md`](../../docs/NETWORK_AUDIT.md)  
**Opcode CSV:** `helper/network-tests/data/opcodes.csv` (regenerate via `scripts/generate_opcode_table.py`)

---

## Build targets (Release | x64)

| Target | Project | Solution | Output role |
|--------|---------|----------|-------------|
| **LIBDBC** | `source/build/serversdk/LIBDBC.vcxproj` | `source/source.sln` | Network SDK static lib |
| **GateServer** | `source/build/gateserver/GateServer.vcxproj` | `source/source.sln` | Client-facing gate |
| **GameServer** | `source/build/gameserver/GameServer.vcxproj` | `source/source.sln` | Game logic |
| **GroupServer** | `source/build/groupserver/GroupServer.vcxproj` | `source/source.sln` | Social/login hub |
| **AccountServer** | `source/build/accountserver/AccountServer.vcxproj` | `source/source.sln` | Account DB auth |
| **Game (client)** | `source/build/game/game.vcxproj` | `source/source.sln` | Game client |

**Build command (Visual Studio developer shell):**

```bat
msbuild source\source.sln /p:Configuration=Release /p:Platform=x64 /m
```

**Build verify script:** `helper/network-tests/scripts/build-release.ps1`

---

## Runtime network configuration

### Gate — `server/GateServer.cfg`

| Section | Key | Current value | Effect |
|---------|-----|---------------|--------|
| Main | Version | 32144 | Client version check on login |
| ToClient | IP / Port | 0.0.0.0 / **1973** | Client listen |
| ToClient | CommEncrypt | 1 | 1=AES-CTR, 2=AES-GCM (+16 B tag/packet) |
| ToClient | EnablePing | 180 | Keepalive timeout (seconds, min 10 enforced in SDK) |
| ToClient | MaxConnection | 1000 | Max client sockets |
| ToGameServer | IP / Port | 127.0.0.1 / **1971** | GameServer listen (gate connects out) |
| GroupServer | IP / Port | 127.0.0.1 / **1975** | GroupServer (gate connects out) |
| AntiDDoS | ProxyProtocol | 0 | SmartProxy PROXY v1 header parsing |
| AntiDDoS | ConnectionRateLimit | 1 | Per-IP connection + traffic rate limits (I1) |
| AntiDDoS | ConnectionMinInterval | 100 | Min ms between accepts from same IP |
| AntiDDoS | MaxConnectionsPerSecond | 5 | Max new connections per second per IP |
| AntiDDoS | MaxRecvBytesPerSecond | 12288 | Disconnect recv flood threshold (12 KB/s) |
| AntiDDoS | MaxPacketsPerSecond | 500 | Disconnect recv packet flood threshold |

### Game — `server/GameServer.cfg`

| Section | Key | Value | Effect |
|---------|-----|-------|--------|
| Gate | gate | 127.0.0.1, **1971** | Gate→Game bind port |
| Socket | keep_alive | 120 | `BeginWork` keepalive seconds |

### Client gate list — `server/resource/ServerSet.txt`

| Field | Value |
|-------|-------|
| Gate IP | 127.0.0.1 |
| Default port (client) | **1973** (from `LoginScene` / config) |

---

## SetPKParse / buffer settings (code)

| Component | File | SetPKParse (max frame / recv cap) |
|-----------|------|-----------------------------------|
| Client NetIF | `game/NetIF.cpp:598` | 32 KB / 32 KB (`NetLimits::kClientGameMaxPacket`) |
| Gate ToClient | `gateserver/ToClient.cpp:171` | 32 KB / 32 KB (`NetLimits::kClientMaxPacket`) |
| Gate ToGameServer | `gateserver/ToGameServer.cpp:23` | 32 KB / 32 KB |
| Gate ToGroupServer | `gateserver/ToGroupServer.cpp:130` | 32 KB / 32 KB |
| GameServerApp | `gameserver/GameServerApp.cpp:191` | 32 KB / 32 KB |
| GroupServerApp | `groupserver/GroupServerApp.cpp:189` | 16 KB / 16 KB |
| AccountServer2 | `accountserver/AccountServer2.cpp:90` | 4 KB / 4 KB |
| OuterServer (GM) | `gameserver/OuterServer.cpp:21` | 4 KB / 4 KB |

**Framing:** 2-byte big-endian length at offset 0; 2-byte big-endian cmd at start of payload; +4 byte SESS when `RPCMGR` enabled.

### Gate SyncCall dispatch (2026-06-27)

GroupServer **SyncCall** opcodes (`CM_LOGIN`, `CM_BGNPLAY`, `CM_ENDPLAY`, new/del char, etc.) must run on the **communicator thread** via `DispatchSyncClientOpcode()` in `ToClient.cpp`. Queuing to the processor pool lets `recvbuf` age past the 10s SyncCall deadline → client disconnect `-5` / `ERR_MC_NETEXCP`.

---

## Diagnostic flags

| Flag | Location | Default | Purpose |
|------|----------|---------|---------|
| `NET_AUDIT_DIAG` | `source/include/common/NetAuditDiag.h` | **0** | Per-packet recv log in `Receiver.cpp` |
| `OpcodeMeta` | `source/include/common/OpcodeMeta.h` | — | 546-entry sorted lookup from `NetCommand.h` (M1) |
| `OpcodeHandlerRegistry` | `source/include/common/OpcodeHandlerRegistry.h` | — | Sorted opcode→handler dispatch (M2-prep) |
| `PacketReader` / `PacketWriter` | `source/include/common/PacketReader.h` | — | Typed R/W facades over I4 contract (M5-prep) |
| `DS_DECRYPT_FAIL` / `DS_HANDLER_EXCP` / `DS_PACKET_PIPELINE` | `source/include/serversdk/Comm.h` | — | Disconnect reasons -11/-12/-13 (I5) |
| `PacketPipelineFailDisconnect` | `source/include/serversdk/PacketPipeline.h` | — | Log + disconnect on handler/decrypt faults |
| `GetSteadyMs()` | `source/include/serversdk/Comm.h` | — | Keepalive/disconnect timers (I3, ms since `BeginWork`) |
| `GetCurrentTick()` | `Comm.h` | — | Legacy tick for bandwidth stats / game timeouts (GetTickCount-based) |
| DataSocket atomics | `source/include/serversdk/DataSocket.h` | — | `m_delflag`, `m_sendflag`, `m_recvflag`, stats (I7) |
| `g_bCommAppDebug` | `serversdk/Comm.cpp` | false | `SetCommAppDebug(true)` — select loop spam |

Enable audit logging in a server project: add preprocessor `NET_AUDIT_DIAG=1` to **LIBDBC** Release config.

---

## Smoke test suite (T0)

| ID | Script | Automated | Notes |
|----|--------|-----------|-------|
| T0-connect | `net-smoke.py` | Yes | TCP + `CMD_MC_RSA_HANDSHAKE_1` (943) |
| T0-ping | `net-smoke.py` | Yes | Send `CMD_CP_PING` (6022), stay connected |
| T0-login | Manual / client | No | RSA+AES login via `ProCirculateCS::Login` |
| T0-enter | Manual / client | No | Char select → `CMD_CM_BGNPLAY` → enter map |
| T0-inter-server | Log check | Partial | `server/LOG/GameServer/*/GameLogin.log` gate entry |

**Run automated tests** (GateServer must be running):

```bat
python helper\network-tests\net-smoke.py
```

---

## Opcode bands (`NetCommand.h`)

| Base macro | Value | Direction |
|------------|-------|-----------|
| CMD_CM_BASE | 0 | Client → GameServer |
| CMD_MC_BASE | 500 | GameServer → Client |
| CMD_TM_BASE | 1000 | Gate → GameServer |
| CMD_MT_BASE | 1500 | GameServer → Gate |
| CMD_TP_BASE | 2000 | Gate → GroupServer |
| CMD_PT_BASE | 2500 | Group → Gate |
| CMD_PA_BASE | 3000 | Group → Account |
| CMD_AP_BASE | 3500 | Account → Group |
| CMD_MM_BASE | 4000 | Game ↔ Game |
| CMD_PM_BASE | 4500 | Group → Game |
| CMD_PC_BASE | 5000 | Group → Client |
| CMD_MP_BASE | 5500 | Game → Group |
| CMD_CP_BASE | 6000 | Client → Group |
| CMD_OS_BASE | 6500 | Monitor → Server |
| CMD_SO_BASE | 7000 | Server → Monitor |
| CMD_TC_BASE | 7500 | Gate → Client |

**Protocol flag:** `NET_PROTOCOL_ENCRYPT 1` in `NetCommand.h:8`

---

## Phase 0 acceptance checklist

- [ ] Opcode CSV ≥ 90 entries (`data/opcodes.csv`)
- [ ] Release\|x64 build: LIBDBC + 4 servers + client
- [ ] T0-connect PASS (automated)
- [ ] T0-ping PASS (automated)
- [ ] T0-login PASS (manual client login once)
- [ ] `NET_AUDIT_DIAG` header present; Receiver wired (default off)
- [ ] This BASELINE.md + STATUS.md committed under `helper/network-tests/`
