# devk Project Guidelines

## Overview

**devk** is a cross-platform (Linux + Windows) port of Pirates King Online (PKO) — a MMORPG server/client codebase. The upstream Windows-only source is `slimepirates`. When porting fixes from slimepirates into devk, always preserve Linux compatibility.

## Critical: Linux-Safe Code Rules

This is the #1 rule — all source changes must compile and run correctly on both Linux (GCC) and Windows (MSVC).

- **Never use `long` for game data fields.** `long` is 32-bit on Windows but 64-bit on Linux. Use `int` or explicit types (`int32_t`, `int64_t`, `LONGLONG`).
- **Never use `%ld` in format strings.** Use `%d` for `int`, `%lld` for 64-bit integers.
- **Never call Windows-only APIs** (`GetModuleFileName`, `WritePrivateProfileString` outside of `#ifdef PKO_PLATFORM_WINDOWS` guards, etc.).
- **Cross-platform guards**: Windows-specific code must be wrapped in `#ifdef PKO_PLATFORM_WINDOWS` / `#endif`.
- **Do not use `__int64`** — use `long long` or `int64_t`.

## Architecture

```
AccountServer   — Login/auth server (source/src/accountserver/)
GateServer      — Client gateway and session routing (source/src/gateserver/)
GroupServer     — Global state: guilds, friends, cross-zone (source/src/groupserver/)
GameServer      — World simulation, characters, items, Lua AI (source/src/gameserver/)
ServerSDK       — Shared networking and DB utilities (source/src/serversdk/, source/src/util/)
Client (Game)   — DirectX 9 + DXVK rendering, Lua UI (source/src/game/, source/src/engine/)
```

Database: SQL Server. Scripts live in `database/` numbered `[01]`–`[11]` in execution order.  
Lua UI scripts: `client/scripts/lua/forms/*.clu` — compiled Lua loaded at client startup.  
Shaders: Binary `.vsh` files in `client/shader/`; HLSL sources in `helper/shaders/hlsl/`.

## Known Removed / Missing Features

Do NOT add these back without explicit confirmation:

- **Badge system** (`m_nBadgeLevel`) — removed from `Character.h`/`Character.cpp`. Do not port badge writes.
- **Portal cache system** (`portalcache`, `Portal` struct, `CMD_TC_PORTALTIMES`) — does not exist in devk. Any code referencing these will fail to compile.
- **`UIItemAuction`** — exists in slimepirates but not devk. Do not add it.

## Porting from slimepirates

When cherry-picking or manually porting changes from slimepirates:

1. **Strip Windows-isms**: `long` → `int`, `%ld` → `%d`, remove Windows-only API calls.
2. **Check for removed features**: Portal cache, badge system, item auction — skip those blocks.
3. **Binary files** (`.vsh` shaders, `.bin` resource files): Copy directly with `Copy-Item -LiteralPath` (bracket filenames break standard paths in PowerShell).
4. **Verify `db3.h`**: Only one `SetColumn(const char*, int, char=LONG)` overload (3-arg with default). Adding a plain 2-arg overload causes ambiguity.
5. **`lua_gamectrl.h`**: Lua AI function implementations are `inline` in the header, not in the `.cpp`. Add new Lua functions there.
6. **IMP currency wire format**: `READ_LONG` (4 bytes) when `currency == 1`; `READ_LONGLONG` (8 bytes) when `currency == 0`. Do not unify to one type.

## Build

- **Windows**: Visual Studio, solution at `source/build/`
- **Linux**: See `LINUX_SETUP.md`
- **Shader compilation**: `helper/shaders/compile_lit_shaders.ps1` (PowerShell)

## Database Scripts

Run in numbered order on a fresh SQL Server instance:

```
[01] AccountServer.sql   — AccountServer database + tables
[02] GameDB.sql          — GameDB database + tables + stored procedures
[03] WebsiteDB.sql       — Website database
[04] CreateSQLLogin.sql  — SQL logins (must run AFTER databases exist)
[05]–[11]               — Optional systems (guilds, stalls, analytics, etc.)
```

## Lua UI Forms

Each C++ UI manager class has a matching `.clu` file. If adding a new UI manager:

- The `.clu` must define all forms/widgets the `::Init()` method looks up via `_FindForm()` and `Find()`.
- Missing forms cause a fatal crash at startup (`CUIInterface::All_Init` halts the sequence).
- Widget name constants are in the `.clu` and must exactly match string literals in the C++ `Init()`.

## Code Style

- Match the style of the surrounding file — this codebase has mixed conventions.
- Only comment code that needs clarification. Do not add obvious or redundant comments.
- Prefer minimal, surgical changes. Do not refactor unrelated code while fixing something else.
