# devk AI Agent Instructions

> **Project:** devk — Cross-platform PKO (Pirates King Online) MMORPG Engine  
> **Platform:** Windows (x64) + Linux (x86_64, GCC)  
> **Toolset:** Visual Studio 2022, DirectX 9 + DXVK, LuaJIT, Botan, ICU 76  
> **Upstream source:** `slimepirates` (Windows-only) — devk is its Linux/cross-platform port

---

## Table of Contents

1. [User Terminology Reference](#user-terminology-reference)
2. [Workspace Overview](#workspace-overview)
3. [Project Overview](#project-overview)
4. [Architecture & Design Patterns](#architecture--design-patterns)
5. [Development Environment & Build](#development-environment--build)
6. [Code Standards & Linux-Safe Rules](#code-standards--linux-safe-rules)
7. [Working with the Client](#working-with-the-client)
8. [Working with Servers](#working-with-servers)
9. [Porting from slimepirates](#porting-from-slimepirates)
10. [Database Scripts](#database-scripts)
11. [Lua UI Forms](#lua-ui-forms)
12. [Shaders & Rendering](#shaders--rendering)
13. [Common Pitfalls & Solutions](#common-pitfalls--solutions)
14. [Quick Reference](#quick-reference)

---

## User Terminology Reference

| User Term | Refers To | Description |
|-----------|-----------|-------------|
| **"client"** / **"client files"** | `client/` | Game client runtime, scripts, assets |
| **"server"** / **"server files"** | `server/` | Server executables, configs, game content |
| **"source"** / **"source files"** | `source/` | C++ source code and build system |
| **"database"** / **"db"** | `database/` | SQL setup scripts |
| **"docs"** | `docs/` | Technical documentation |
| **"helper"** / **"tools"** | `helper/` | Shaders, launcher, utility tools |
| **"slimepirates"** | `C:\Users\Ken\Desktop\Github\slimepirates\` | Upstream Windows-only source — has newer fixes to port |
| **"third_party"** / **"libs"** | `third_party/` | Library sources — READ ONLY |
| **"website"** | `website/` | PHP web application for account management |
| **"security"** | `security/` | Security configs and scripts |

---

## Workspace Overview

```
devk/                          # Workspace root
├── .github/                   # Copilot instructions, agents, prompts
├── client/                    # Game client runtime
├── database/                  # SQL database init scripts [01]–[11]
├── docs/                      # Technical documentation
├── helper/                    # Shaders, haproxy, launcher, tools
├── security/                  # Security configs
├── server/                    # Server runtime (binaries + configs + resource)
├── source/                    # C++ source code and build system
├── third_party/               # Library sources (REFERENCE ONLY)
├── tools/                     # Map/nav conversion tools
├── website/                   # PHP web application
└── LINUX_SETUP.md             # Linux build/setup guide
```

### `/client` — Game Client Runtime

```
client/
├── system/                    # Core executables and DLLs (symlinked from source/bin)
│   ├── Game.exe               # Main game client
│   ├── CaLua.dll              # Lua scripting engine
│   └── MindPower3D_D9R.dll    # DirectX 9 rendering engine
├── animation/                 # Character/NPC animation files (*.lab)
├── effect/                    # Visual effects
├── font/                      # Font files
├── map/                       # 3D terrain maps
├── model/                     # 3D character/object models
├── music/                     # Background music
├── scripts/                   # Lua UI scripts (*.clu files — compiled Lua)
│   └── lua/
│       ├── gui.clu            # Master UI loader — loads all form scripts
│       └── forms/             # Per-feature form scripts
├── shader/                    # Binary DirectX shader programs (*.vsh)
├── texture/                   # 2D textures
├── user/                      # User settings and local cache
├── start.bat                  # Launch game client
├── compile.bat                # Quick compile wrapper
└── symlinks.bat               # Creates symlinks to source/bin (run as admin, uses %~dp0)
```

**Key client logs** (for debugging crashes):
- `client/client.log` — DebugView output, C++ execution trace
- `client/calua_err.txt` — Lua compile/runtime errors
- `client/init.log` — Startup sequence (UI init order)
- `client/lua_err.txt` — Additional Lua errors

### `/database` — SQL Scripts

```
database/
├── [01]AccountServer.sql      # AccountServer DB + tables + stored procedures
├── [02]GameDB.sql             # GameDB + all tables + 159 stored procedures
├── [03]WebsiteDB.sql          # Website DB
├── [04]CreateSQLLogin.sql     # SQL logins (must run AFTER [01]–[03])
├── [05]CreateAccount.sql      # Account creation procedures
├── [06]Guild.sql              # Guild system tables
├── [07]OfflineStalls.sql      # Offline stall system
├── [08]EconomyAnalytics.sql   # Economy tracking
├── [09]NewsSystem.sql         # News/announcement system
├── [10]PasswordResetTokens.sql # Password reset tokens
└── [11]InventoryExpansion128.sql # Optional: 128-slot inventory
```

**IMPORTANT:** Run in numbered order `[01] → [11]`. Databases must exist before `[04]CreateSQLLogin.sql`.

### `/server` — Server Runtime

```
server/
├── AccountServer.exe          # Auth server
├── GateServer.exe             # Client gateway
├── GameServer.exe             # Game world server
├── GroupServer.exe            # Cross-server features
├── AccountServer.cfg          # Auth server config (IP, port, DB)
├── GateServer.cfg             # Gateway routing config
├── GameServer.cfg             # Game server config
├── GroupServer.cfg            # Group server config
├── en_US.res                  # Localization resource bundle
├── TrustedIPs.txt             # IPs trusted for proxy protocol
├── ChaNameFilter.txt          # Character name blacklist
├── resource/                  # Game content data (text files + binary .bin)
│   ├── script/                # Server-side Lua scripts (quests, NPCs, AI)
│   └── [map folders]/         # Per-map NPC/spawn/map binary files (*.bin — gitignored)
├── addons/                    # Server plugins/extensions
└── LOG/                       # Server log files (gitignored)
```

**Server startup order:** AccountServer → GateServer → GroupServer → GameServer

| Server | Default Port | Purpose |
|--------|-------------|---------|
| AccountServer | 1973 | Authentication, account management |
| GateServer | 1975 | Client gateway, load balancing, anti-DDoS |
| GroupServer | 5020 | Guilds, friends, cross-server state |
| GameServer | 5816+ | World simulation, combat, items, Lua AI |

### `/source` — C++ Source Code

```
source/
├── source.sln                 # Master Visual Studio solution
├── src/                       # C++ source files
│   ├── game/                  # Game client (Game.exe)
│   ├── engine/                # MindPower3D rendering engine (DLL)
│   ├── calua/                 # Lua C API wrapper (CaLua.dll)
│   ├── accountserver/         # Account authentication server
│   ├── gateserver/            # Connection gateway server
│   ├── gameserver/            # Game world server
│   ├── groupserver/           # Cross-server features server
│   ├── serversdk/             # Shared server SDK (networking, RPC)
│   ├── common/                # Shared utilities (math, string, TableData)
│   ├── util/                  # DB helpers (db.cpp, db3.cpp, db3.h)
│   ├── status/                # Character status/buff system
│   ├── enclib/                # Encryption (Botan)
│   ├── audiosdl/              # SDL audio wrapper
│   ├── icuhelper/             # ICU text processing
│   └── infonet/               # Network info utilities
├── build/                     # Visual Studio .vcxproj files
├── include/                   # Header files organized by component
├── lib/x64/                   # Precompiled libraries (Release/ and Debug/)
├── bin/Release/               # Build output (gitignored)
├── CMakeLists.txt             # Linux/CMake build
└── symlinks.bat               # Deploy binaries to client/ and server/
```

### `/helper` — Utility Tools

```
helper/
├── shaders/                   # Shader source and build pipeline
│   ├── hlsl/                  # HLSL shader sources
│   │   └── common.hlsli       # Shared lighting helpers (cel-shading CalcLighting)
│   ├── compile_lit_shaders.ps1 # Compiles .hlsl → binary .vsh shaders
│   ├── encrypt_one.ps1        # Shader encryption helper
│   └── compile_shaders.lua    # Shader mapping definitions
├── haproxy/                   # HAProxy DDoS protection config
├── launcher/                  # Game launcher
├── patchgen/                  # Patch generation tools
├── reshade/                   # ReShade post-processing
└── translation/               # Localization tools
```

---

## Project Overview

devk is a cross-platform modernized version of Pirates King Online (PKO), a legacy C++ MMORPG from the mid-2000s. Key characteristics:

- **Client:** DirectX 9 + DXVK (Vulkan translation) game client with cel-shading outline renderer
- **Server:** Multi-tier C++ backend (AccountServer, GateServer, GameServer, GroupServer)
- **Engine:** MindPower3D rendering engine (DX9, custom vertex shaders)
- **Scripting:** LuaJIT (Lua 5.1) for game logic, quest AI, and client UI
- **Cross-platform:** Compiles on both Windows (MSVC, VS2022) and Linux (GCC, CMake)
- **Upstream:** `slimepirates` is the Windows-only fork; devk ports its fixes with Linux compatibility

---

## Architecture & Design Patterns

### Multi-Tier Server Architecture

```
Client (Game.exe)
    ↓ TCP
GateServer (gateway, anti-DDoS, routing)
    ↓                    ↓
AccountServer       GameServer(s)
(auth/billing)      (world logic)
                         ↓
                    GroupServer
                    (guilds, cross-server)
                         ↓
                    SQL Server (GameDB, AccountServer DB)
```

### Client Architecture

#### Scene System

Three main scenes manage the full game lifecycle:

| Scene | Purpose | Entry |
|-------|---------|-------|
| `LoginScene` | Authentication | App start |
| `SelectChaScene` | Character selection | After login |
| `WorldScene` | Game world | After char select |

#### UI System (C++ ↔ Lua)

- All UI manager classes derive from `CUIInterface`
- Registered via global constructor — order in `UIGlobalVar.cpp` defines init order
- `CUIInterface::All_Init()` initializes all 44 UI managers in sequence — any failure halts startup
- Each manager has a matching `.clu` script file defining its forms/widgets
- Lua scripts communicate with C++ via registered functions in `lua_ui.h`, `lua_gamectrl.h`, etc.

#### Character Architecture

```cpp
CCharacter   // DATA: position, stats, inventory, skills, state flags
    ↓
CActor       // PRESENTATION: animation, model rendering
    ↓
CSceneObj    // BASE: scene object lifecycle
```

**Rule:** `CCharacter` = DATA/STATE. `CActor` = PRESENTATION/ANIMATION. Never mix.

#### Character States (Bitmask)

```cpp
// States are non-exclusive bitmasks (ChaState.h)
enumChaStateMove     = 0x0001
enumChaStateAttack   = 0x0002
enumChaStateUseSkill = 0x0004
enumChaStateTrade    = 0x0008
// Check: character->HasState(enumChaStateMove)
// Set:   character->AddState(enumChaStateMove)
```

#### Network Message Conventions

```
CM_*       Client → GateServer (Middle)
SM_*       Server → Client (via GateServer)
GM_*       GameServer → GateServer
GS_*       GroupServer functions
SCENEMSG_* Local scene messages only (never sent over network)
```

### GameServer Architecture

- **Character state** is authoritative on GameServer only. GateServer only routes.
- **NPC/AI** runs via Lua scripts in `server/resource/script/`
- **Lua AI functions** are implemented as `inline` functions in `include/GameServer/lua_gamectrl.h`
- `REGFN(FunctionName)` macro in `lua_gamectrl.cpp` registers each function with the Lua VM
- **Database pattern:** Load all character data at login → cache in memory → batch persist at logout

### Packet Wire Format (Critical)

The IMP (premium currency) wire format is conditional — do not unify:

```cpp
if (currency == 1) {
    READ_LONG(IMP);       // 4 bytes — premium currency
} else {
    READ_LONGLONG(gold);  // 8 bytes — standard gold
}
```

Buff serialization is 4 fields (must match DB schema):
```cpp
// SStateData2String / String2SStateData — always 4 fields:
// sStateID, nLeftTime, nLevel, sSourceItemID
```

---

## Development Environment & Build

### Windows Build

**Solution:** `source/source.sln` in Visual Studio 2022

**Build configuration:** `Release|x64` for production, `Debug|x64` for debugging

**MSBuild command (full solution):**
```powershell
cd "C:\Users\Ken\Desktop\Github\devk\source"
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" `
  "source.sln" /p:Configuration=Release /p:Platform=x64 /m /v:minimal
```

**Build specific project:**
```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" `
  "build\gameserver\GameServer.vcxproj" /p:Configuration=Release /p:Platform=x64 /v:minimal
```

**Build output locations:**

| Binary | Output Path |
|--------|-------------|
| Game.exe | `source/bin/Release/game/` |
| MindPower3D_D9R.dll | `source/bin/Release/mindpower3d/` |
| CaLua.dll | `source/bin/Release/calua/` |
| AccountServer.exe | `source/bin/Release/accountserver/` |
| GateServer.exe | `source/bin/Release/gateserver/` |
| GameServer.exe | `source/bin/Release/gameserver/` |
| GroupServer.exe | `source/bin/Release/groupserver/` |

**After build, deploy:**
```powershell
cd "C:\Users\Ken\Desktop\Github\devk\source"
.\symlinks.bat  # Creates symlinks to client/system/ and server/
```

### Linux Build

See `LINUX_SETUP.md` for full setup. Uses CMake:
```bash
cd source
cmake -B build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux -j$(nproc)
```

### Dependencies

| Library | Version | Location |
|---------|---------|----------|
| DirectX SDK | 9.x (DX9) | `source/include/directx/` |
| DXVK | 2.x | Vulkan DX9 translation layer |
| LuaJIT | 5.1 compat | `third_party/luajit/`, `source/lib/x64/` |
| Botan | 2.19.5 | `third_party/Botan/` |
| ICU | 76.1 | `third_party/icu/`, `source/include/unicode/` |
| SDL2 / SDL2_mixer | 2.x | `third_party/SDL2/` |
| Discord RPC | Latest | `third_party/discord-rpc/` |

### Build Warnings Safe to Ignore

- `LNK4098` — LIBCMT conflicts (handled via /NODEFAULTLIB)
- `LNK4099` — Missing PDB for lua51.lib (precompiled)
- `D9025` — Compiler flag overrides
- Botan deprecation warnings — library-internal

---

## Code Standards & Linux-Safe Rules

**This is the #1 rule for all code changes. devk must compile and run on both Linux (GCC) and Windows (MSVC).**

### Type Safety (Critical)

| ❌ Do NOT use | ✅ Use instead | Reason |
|--------------|----------------|--------|
| `long` | `int` or `int32_t` | `long` is 64-bit on Linux, 32-bit on Windows |
| `__int64` | `long long` or `int64_t` | MSVC-specific |
| `%ld` | `%d` (for int) or `%lld` (for int64) | Format string mismatch on Linux |
| `%I64d` | `%lld` | MSVC-specific format |

```cpp
// ❌ Wrong (compiles on MSVC, breaks GCC)
long nGold = 1000000L;
LG("gold", "Player has %ld gold", nGold);

// ✅ Correct
int nGold = 1000000;
LG("gold", "Player has %d gold", nGold);

// ✅ Correct for 64-bit values
long long nGoldEx = 1000000000LL;
LG("gold", "Player has %lld gold", nGoldEx);
```

### Windows-Only APIs

Wrap all Windows-specific calls in platform guards:

```cpp
// ❌ Wrong — breaks Linux build
GetModuleFileNameA(NULL, buf, sizeof(buf));
WritePrivateProfileString("section", "key", value, "./config.ini");
::Sleep(100);

// ✅ Correct — guarded
#ifdef PKO_PLATFORM_WINDOWS
    GetModuleFileNameA(NULL, buf, sizeof(buf));
#else
    readlink("/proc/self/exe", buf, sizeof(buf));
#endif

// ✅ Correct — use cross-platform alternatives
// For Sleep: use usleep() on Linux, Sleep() on Windows (both wrapped in platform code)
```

### Naming Conventions

| Type | Convention | Example |
|------|-----------|---------|
| Class | `C` prefix | `CCharacter`, `CGameApp` |
| Struct | `s` or `x` prefix | `stNetMoveInfo`, `xShipInfo` |
| State Machine | `ST*` | `STMove`, `STAttack` |
| UI Manager | `C*Mgr` | `CSystemMgr`, `CMissionMgr` |
| UI Form classes | `C*Form` or `C*Mgr` | `CKnowledgeBase`, `CBoothMgr` |
| Lua bindings | `lua_*.h` | `lua_gamectrl.h`, `lua_ui.h` |
| Network packets | see conventions | `CM_*`, `SM_*`, `GM_*` |

### Code Style

- Match the surrounding file's style — this codebase has mixed conventions
- Only comment code that genuinely needs clarification — no obvious comments
- Prefer minimal, surgical changes — do not refactor unrelated code
- Chinese comments exist throughout the legacy code — preserve them, do not delete

---

## Working with the Client

### Adding a New UI Manager

1. **Create C++ class** deriving from `CUIInterface`:
```cpp
class CMyFeature : public CUIInterface {
public:
    bool Init() override;
    void End() override;
    // ... widget pointers
};
```

2. **Declare global instance** in `UIGlobalVar.cpp` — position determines init order:
```cpp
CMyFeature g_stUIMyFeature;  // Added here — init at position N in All_Init sequence
```

3. **Include the header** at top of `UIGlobalVar.cpp`.

4. **Create matching `.clu` file** in `client/scripts/lua/forms/myfeature.clu`:
   - Must define all forms/widgets that `Init()` looks up
   - Register it in `gui.clu` with `UI_LoadScript("scripts/lua/forms/myfeature.clu")`

5. **Verify widget names exactly match** — `_FindForm("frmName")` and `frm->Find("widgetName")` are case-sensitive string lookups.

### UI Init Crash Debugging

When the client crashes at startup with `[N/44] ClassName Init()...` in `client.log`:

1. Find the Init() method for that class in `source/src/game/UI*.cpp`
2. Look for `_FindForm("name")` returning null → check `.clu` has `UI_CreateForm("name", ...)`
3. Look for `Find("widget")` returning null → check `.clu` has `UI_CreateCompent(frm, type, "widget", ...)`
4. Check `client/calua_err.txt` for Lua syntax errors that prevent the `.clu` from loading

**Lua type constants** (from `gui.clu`):
```lua
LABELEX_TYPE     = 1
BUTTON_TYPE      = 2
EDIT_TYPE        = 4
CHECK_TYPE       = 8
LIST_TYPE        = 6
PAGE_TYPE        = 11
COMMAND_ONE_TYPE = 22
SCROLL_TYPE      = 24
CHECK_GROUP_TYPE = 10
IMAGE_TYPE       = 3
COMBO_TYPE       = 7
PROGRESS_TYPE    = 13
```

### Lua Binding Pattern (lua_gamectrl.h)

All Lua AI functions for the GameServer are implemented as `inline` in the header (not the .cpp):

```cpp
// In source/include/GameServer/lua_gamectrl.h
inline int lua_MyFunction(lua_State* L) {
    // 1. Validate arg count
    if (lua_gettop(L) < 2) {
        PARAM_ERROR("MyFunction");
        return 0;
    }
    // 2. Extract args
    int arg1 = (int)lua_tointeger(L, 1);
    const char* arg2 = lua_tostring(L, 2);
    // 3. Execute
    int result = DoWork(arg1, arg2);
    // 4. Return
    lua_pushinteger(L, result);
    return 1;
}
```

Register in `source/src/gameserver/lua_gamectrl.cpp`:
```cpp
REGFN(MyFunction);  // Maps Lua "MyFunction" → lua_MyFunction
```

---

## Working with Servers

### Server Communication Pattern

All inter-server communication is **asynchronous via DataSocket**. Never block in callbacks:

```cpp
// ❌ Wrong — deadlocks the server
void RequestData() {
    SendRequest(gameServer, REQUEST_DATA);
    while (!HasResponse()) Sleep(100);  // BLOCKS
}

// ✅ Correct — fire and return
void RequestData() {
    SendRequest(gameServer, REQUEST_DATA);
    // Response arrives in OnProcessData() callback
}

void OnProcessData(DataSocket* socket, char* data, int len) {
    if (IsDataResponse(data)) HandleResponse(data);
    // Return immediately
}
```

### Adding a New Server Packet

1. **Define packet type** in appropriate `PacketCmd.h`
2. **GateServer routes** in `ToClient.cpp` — forward only, no character state
3. **GameServer handles** in the GameServer packet handler — owns character objects
4. **GroupServer** for cross-server state (guilds, teams, etc.)

**Critical rule:** GameServer owns all character objects. GateServer only routes. Never access character state in GateServer.

### Anti-DDoS (GateServer)

```cpp
struct AntiDDoSInfo {
    int m_checkWarning = 5;   // Warn threshold (commands/sec)
    int m_checkError   = 7;   // Blacklist threshold
    int m_checkSpan    = 1;   // Check interval (seconds)
    bool m_bIsBlacklisted;    // Drop all packets if true
};
```

### Packet Sanitization (AccountServer)

AccountServer uses `PS::` (PacketSanitizer) namespace for all user input validation:
```cpp
PS::IsSafeString(str)   // Validate string input
PS::IsValidEmail(email) // Validate email format
// Always validate before any DB operation
```

### Database Integration

Pattern: **load at login → cache in memory → persist at logout**

```cpp
// Always use stored procedures — never raw SQL
const auto ret = stored_procedure("{CALL dbo.ReadAllData(?, ?)}", 
    "dbo", "ReadAllData", 2, &chaId, &param);
if (DBOK(ret)) { /* success */ }

// Use db3.h helpers for inserts/updates
CSQLUpdate update("dbo.CharacterUpdate");
update.SetColumn("gold", nGold);    // SetColumn(const char*, int, char=LONG) — 3-arg only
update.SetColumn("name", szName);   // Never add a 2-arg overload — causes ambiguity
update.Execute(hConn);
```

**db3.h overload rule:** Only `SetColumn(const char*, int, char=LONG)` (3-arg with default) exists. Never add a plain 2-arg overload — it creates ambiguity with the defaulted 3-arg version.

---

## Porting from slimepirates

slimepirates (`C:\Users\Ken\Desktop\Github\slimepirates`) is the upstream Windows-only source. It receives bug fixes and features that need to be ported into devk. Always preserve Linux compatibility when porting.

### Porting Checklist

Before applying any slimepirates change to devk:

- [ ] Replace `long` → `int` (or appropriate explicit type)
- [ ] Replace `%ld` → `%d` (or `%lld` for 64-bit)
- [ ] Replace `__int64` → `long long` or `int64_t`
- [ ] Remove or guard Windows-only APIs (`#ifdef PKO_PLATFORM_WINDOWS`)
- [ ] Skip blocks referencing removed features (see below)
- [ ] For binary files: use `Copy-Item -LiteralPath` (bracket `[]` in filenames break standard PowerShell paths)

### Features Removed from devk — Do NOT Port

| Feature | What to skip |
|---------|-------------|
| **Badge system** | `m_nBadgeLevel` field, any `WRITE_CHAR(WtPk, (Char)m_nBadgeLevel)` calls |
| **Portal cache** | `portalcache`, `Portal` struct, `CMD_TC_PORTALTIMES` — infrastructure missing entirely |
| **UIItemAuction** | `UIItemAuction.cpp/.h` — file doesn't exist in devk |

### Key Porting Notes

**`lua_gamectrl.h`** — Lua AI implementations are `inline` in the header (not the `.cpp`). When slimepirates adds a new `REGFN(X)` in the `.cpp`, the matching `inline int lua_X(...)` must be added to the header.

**`db3.h`** — Only one `SetColumn` overload: `SetColumn(const char*, char=LONG)` for int, `SetColumn(const char*, const char*)` for string. The slimepirates version may differ — verify overloads don't create ambiguity.

**IMP currency** — Read format is conditional (see [Packet Wire Format](#packet-wire-format-critical)).

**Buff serialization** — `SStateData2String`/`String2SStateData` must always serialize 4 fields: `sStateID`, `nLeftTime`, `nLevel`, `sSourceItemID`. Reducing to 3 breaks DB compatibility.

**Cherry-picking commits:**
```powershell
# Add slimepirates as remote (if not already)
cd "C:\Users\Ken\Desktop\Github\devk\source"
git remote add slimepirates "C:\Users\Ken\Desktop\Github\slimepirates\source"
git fetch slimepirates

# Cherry-pick a commit
git cherry-pick <commit-hash>
# Resolve conflicts, then apply Linux-safe fixes
```

**Binary shader conflicts during cherry-pick:**
```powershell
# git checkout --theirs doesn't work reliably for binary conflicts
# Copy directly instead:
Copy-Item -LiteralPath "C:\Users\Ken\Desktop\Github\slimepirates\client\shader\[shader].vsh" `
          -Destination "C:\Users\Ken\Desktop\Github\devk\client\shader\"
git add "client/shader/[shader].vsh"
```

---

## Database Scripts

### Fresh Install Execution Order

Run scripts in this exact order on a new SQL Server instance:

| # | File | Creates | Notes |
|---|------|---------|-------|
| [01] | `AccountServer.sql` | AccountServer DB + tables + 13 SPs | Auth, login, accounts |
| [02] | `GameDB.sql` | GameDB + all tables + 159 SPs | Characters, items, guilds, boats |
| [03] | `WebsiteDB.sql` | WebsiteDB | Web dashboard |
| [04] | `CreateSQLLogin.sql` | SQL logins: pko_account, pko_game, pko_web | **MUST run after [01]–[03]** |
| [05] | `CreateAccount.sql` | Account creation SPs | |
| [06] | `Guild.sql` | Guild system | |
| [07] | `OfflineStalls.sql` | offline_stalls table | |
| [08] | `EconomyAnalytics.sql` | Economy tracking | |
| [09] | `NewsSystem.sql` | News/announcements | |
| [10] | `PasswordResetTokens.sql` | Password reset | |
| [11] | `InventoryExpansion128.sql` | 128-slot inventory | Optional — changes VARCHAR(7000)→VARCHAR(MAX) |

**Why [04] is last among setup scripts:** `CreateSQLLogin.sql` maps SQL logins to specific databases (`USE AccountServer`, `sp_addrolemember`). If the databases don't exist yet, this fails.

### Key Stored Procedure Rules

- **Always use stored procedures** — never raw SQL queries
- `[02]GameDB.sql` includes all Phase 2.5 SPs inline — no separate patch file needed
- `VARCHAR(max)` vs `VARCHAR(7000)`: base schema uses 7000 for compatibility; expansion script reverts to MAX

### Column Name Gotchas

| C++ Reference | SQL Column | Table |
|---------------|------------|-------|
| `money` | `gold` | `guild` |
| `type` | `type_id` | `Resource` |
| `banklog` | `banklog` | `guild` |

---

## Lua UI Forms

### Form/Widget Matching Rule

Every C++ UI manager's `Init()` must find all its forms and widgets in the corresponding `.clu` file. The lookup is by exact string name.

```cpp
// C++ Init() — critical lookups
frmMyForm = _FindForm("frmMyForm");     // NULL → return false → client crash
CButton* btn = frmMyForm->Find("btnOK"); // NULL → return Error() → client crash
```

```lua
-- Matching .clu — names must match exactly
frmMyForm = UI_CreateForm("frmMyForm", ...)
btnOK = UI_CreateCompent(frmMyForm, BUTTON_TYPE, "btnOK", ...)
```

### Init() Return Behavior

- `return false` → `CUIInterface::All_Init()` halts, client exits (shown as crash at `[N/44]`)
- `return Error(...)` → same result (Error() returns false)
- `assert()` failures → hard crash (access violation or abort)

The 44 UI managers initialize in global declaration order in `UIGlobalVar.cpp`. A crash at `[N/44]` means manager N's `Init()` returned false.

### system.clu — frmGame Widget List

The `frmGame` form (game options) requires these `cbx*` widget groups (all created via `addSystemOption()`):

`cbxRunmodel`, `cbxLockmodel`, `cbxHelpmodel`, `cbxCameraMode_p`, `cbxAppmodel`, `cbxEffmodel`, `cbxShowMounts_p`, `cbxStatemodel`, `cbxEnemyNames_p`, `cbxShowBars_p`, `cbxShowPercentages_p`, `cbxShowInfo_p`, `cbxFramerate_p`, `cbxDisableMelee_p`, `cbxOutline_p`

### bosstimer.clu

`LG("scripts", ...)` is not available in devk's Lua environment. If slimepirates code uses `LG()` in `.clu` files, replace with a comment or remove.

---

## Shaders & Rendering

### Shader Pipeline

Binary `.vsh` files in `client/shader/` are compiled from HLSL sources in `helper/shaders/hlsl/`.

**Compile shaders:**
```powershell
cd "C:\Users\Ken\Desktop\Github\devk\helper\shaders"
.\compile_lit_shaders.ps1
```

### Rendering Features (Ported from slimepirates)

- **DXVK 2.x** — DirectX 9 → Vulkan translation; config in `client/system/dxvk.conf`
- **Cel-shading outline pass** — extra draw pass in `lwItem.cpp` and `lwPhysique.cpp`
- **3-band quantized lighting** — implemented in `helper/shaders/hlsl/common.hlsli` via `CalcLighting()`
- **VS_CONST_REG_EYE_POS = c20** — camera eye position in object space (replaces old SPECULAR_PARAMS)
- **LRU mesh cache** — in `MPResManger.cpp/.h`

### common.hlsli — CalcLighting

The `CalcLighting()` function in `common.hlsli` uses cel-shading (3-band quantization):

```hlsl
// CEL_BAND0=0.40, CEL_BAND1=0.75, CEL_LEVEL0/1/2
// Do NOT replace with half-lambert — that was the old approach
```

### lwxRenderCtrlVS.h — VS Constant Register Map

```cpp
enum EVSConstReg {
    VS_CONST_REG_BASE       = 0,
    VS_CONST_REG_VIEWPROJ   = 4,   // 4x4 matrix
    VS_CONST_REG_LIGHT_DIR  = 8,
    VS_CONST_REG_EYE_POS    = 20,  // Camera eye pos (replaced SPECULAR_PARAMS)
    VS_CONST_REG_MAT_PALETTE = 26,
    VS_CONST_REG_LIGHT_AMB  = 95,
    VS_CONST_REG_TS0_UVMAT  = 96,
    VS_CONST_REG_TS1_UVMAT  = 100,
    VS_CONST_REG_TS2_UVMAT  = 104,
};
```

---

## Common Pitfalls & Solutions

### 1. `long` type on Linux

❌ `long nGold = GetGold();` — 64-bit on Linux, causes data corruption  
✅ `int nGold = GetGold();` or `int64_t nBigValue = GetValue();`

### 2. Portal Cache References

❌ Any reference to `portalcache`, `Portal` struct, `CMD_TC_PORTALTIMES`  
✅ These don't exist in devk — skip or remove those code blocks entirely

### 3. Badge System

❌ `WRITE_CHAR(WtPk, (Char)m_nBadgeLevel)` — field doesn't exist  
✅ Remove — `m_nBadgeLevel` was stripped from `Character.h`

### 4. SetColumn Ambiguity in db3.h

❌ Adding `SetColumn(const char*, int)` (2-arg) alongside the existing 3-arg defaulted version  
✅ Only `SetColumn(const char*, int, char=LONG)` should exist — the default makes 2-arg calls ambiguous

### 5. Bracket Filenames in PowerShell

❌ `Copy-Item "client\shader\[my_shader].vsh" ...` — PowerShell treats `[]` as glob  
✅ `Copy-Item -LiteralPath "client\shader\[my_shader].vsh" ...`

### 6. ui_LoadScript / Missing .clu Crash

❌ Adding `UI_LoadScript("scripts/lua/forms/newfeature.clu")` in `gui.clu` without creating the file  
✅ Always create the `.clu` file first; any Lua load error causes `CKnowledgeBase::Init()` (or the next form) to fail

### 7. Blocking RPC Calls on Servers

❌ `while (!HasResponse()) Sleep(100);` in server callback  
✅ Fire and return — handle the response in the async `OnProcessData()` callback

### 8. GateServer Accessing Character State

❌ Looking up `CCharacter*` in GateServer  
✅ GateServer only routes packets. `CCharacter` objects live in GameServer only.

### 9. %~dp0 vs %cd% in Batch Files

❌ `%cd%` in batch files run as admin — UAC resets CWD to `C:\Windows\System32`  
✅ `%~dp0` always refers to the batch file's own directory regardless of elevation

### 10. Stale Conflict Markers

After a messy cherry-pick, verify no `<<<<<<<` markers remain in source files before building:
```powershell
Select-String -Path "source\include\**\*.h" -Pattern "<<<<<<<" -Recurse
```

---

## Quick Reference

### File Locations

| Component | Source | Headers | Build Project |
|-----------|--------|---------|---------------|
| Game client | `src/game/` | `include/game/` | `build/game/game.vcxproj` |
| Engine | `src/engine/` | `include/engine/` | `build/engine/MindPower3D.vcxproj` |
| CaLua | `src/calua/` | `include/calua/` | `build/calua/CaLua.vcxproj` |
| AccountServer | `src/accountserver/` | `include/AccountServer/` | `build/accountserver/` |
| GateServer | `src/gateserver/` | `include/GateServer/` | `build/gateserver/` |
| GameServer | `src/gameserver/` | `include/GameServer/` | `build/gameserver/` |
| GroupServer | `src/groupserver/` | `include/GroupServer/` | `build/groupserver/` |
| Common | `src/common/` | `include/common/` | `build/common/` |
| Util/DB | `src/util/` | `include/util/` | `build/util/` |
| ServerSDK | `src/serversdk/` | `include/ServerSDK/` | `build/serversdk/` |

### Key Files

| Purpose | File |
|---------|------|
| UI init sequence | `source/src/game/UIGlobalVar.cpp` |
| All 44 UI managers list | `source/src/game/UIGlobalVar.cpp` lines 92–135 |
| Lua AI functions | `source/include/GameServer/lua_gamectrl.h` (inline impls) |
| Lua AI registration | `source/src/gameserver/lua_gamectrl.cpp` (REGFN macros) |
| Character class | `source/include/GameServer/Character.h` |
| Buff serialization | `source/src/gameserver/Character.cpp` (SStateData2String) |
| IMP wire format | `source/src/gameserver/CharacterPrl.cpp` |
| DB helpers | `source/include/util/db3.h`, `source/src/util/db3.cpp` |
| Master UI script | `client/scripts/lua/gui.clu` |
| Game options form | `client/scripts/lua/forms/system.clu` |
| Shader lighting | `helper/shaders/hlsl/common.hlsli` |
| VS const registers | `source/include/engine/lwxRenderCtrlVS.h` |

### Documentation

| Topic | File |
|-------|------|
| Linux build setup | `LINUX_SETUP.md` |
| Build guide | `docs/BUILD_GUIDE.md` |
| Crash dump analysis | `docs/CRASH_DUMP_GUIDE.md` |
| Network security | `docs/NETWORK_SECURITY.md` |
| DX9 migration | `docs/DX9_MIGRATION_PLAN.md` |
| Security audit | `docs/SECURITY_AUDIT.md` |
| DB stored procedures | `database/stored_procedures_verification.md` |

### Development Workflow

1. Edit source in `source/src/`
2. Build with Visual Studio (`Release|x64`)
3. Run `source/symlinks.bat` to deploy to `client/system/` and `server/`
4. Start servers: AccountServer → GateServer → GroupServer → GameServer
5. Launch client via `client/start.bat`
6. Check `client/client.log` (DebugView) and `client/calua_err.txt` for errors

### Testing Checklist

- [ ] Compiles on Windows without errors
- [ ] No `long` used for game data (use `int` or explicit types)
- [ ] No `%ld` format strings
- [ ] No Windows-only APIs outside `#ifdef PKO_PLATFORM_WINDOWS`
- [ ] No references to removed features (badge, portal cache, item auction)
- [ ] UI managers: `.clu` defines all forms/widgets that `Init()` requires
- [ ] Lua bindings: validate arg count before stack access
- [ ] Server callbacks: no blocking waits
- [ ] DB operations: use stored procedures, not raw SQL
- [ ] Binary files copied with `-LiteralPath` if filenames contain `[]`

