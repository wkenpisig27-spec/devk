# PKODev Build Guide

> **Last Updated:** January 27, 2026  
> **Platform:** x64 (64-bit)  
> **Visual Studio:** 2022 (v143)

---

## Quick Start

### Prerequisites

1. **Visual Studio 2022** (Community, Professional, or Enterprise)
   - Workload: "Desktop development with C++"
   - Individual components: Windows SDK, MSVC v143 toolset

2. **DirectX 8 SDK** headers (already included in `source/include/directx/`)

3. **Third-party libraries** (already included in `source/lib/` and `third_party/`)

---

## Building the Solution

### Method 1: Visual Studio IDE

1. Open `source/source.sln` in Visual Studio 2022
2. Select configuration: **Release | x64**
3. Build → Build Solution (Ctrl+Shift+B)
4. Output: `source/bin/Release/`

### Method 2: MSBuild Command Line

**Full Solution Build (Release, x64):**
```powershell
cd "<workspace>\source"
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "source.sln" /p:Configuration=Release /p:Platform=x64 /m /v:minimal
```

**Clean Build:**
```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "source.sln" /p:Configuration=Release /p:Platform=x64 /t:Clean
```

**Debug Build:**
```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "source.sln" /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
```

**Build Specific Project Only:**
```powershell
# Example: Build only GameServer
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "build\GameServer\GameServer.vcxproj" /p:Configuration=Release /p:Platform=x64 /v:minimal

# Example: Build only Game Client
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "build\game\game.vcxproj" /p:Configuration=Release /p:Platform=x64 /v:minimal
```

---

## Build Configurations

| Configuration | Platform | Output Path | Purpose |
|---------------|----------|-------------|---------|
| **Release** | x64 | `bin/Release/` | Production builds, optimized |
| **Debug** | x64 | `bin/Debug/` | Development, with debug symbols |

---

## Build Output Locations

After a successful build, binaries are located at:

| Component | Output Path |
|-----------|-------------|
| **Game Client** | `bin/Release/game/Game.exe` |
| **Rendering Engine** | `bin/Release/mindpower3d/MindPower3D_D8R.dll` |
| **Lua Scripting** | `bin/Release/calua/CaLua.dll` |
| **AccountServer** | `bin/Release/accountserver/AccountServer.exe` |
| **GateServer** | `bin/Release/gateserver/GateServer.exe` |
| **GameServer** | `bin/Release/gameserver/GameServer.exe` |
| **GroupServer** | `bin/Release/groupserver/GroupServer.exe` |

---

## Post-Build Deployment

After building, deploy binaries to client and server directories:

```powershell
cd "<workspace>\source"
.\symlinks.bat
```

This creates symbolic links from `bin/Release/` to:
- `../client/system/` (Game.exe, DLLs)
- `../server/` (Server executables)

---

## Solution Projects

The solution (`source.sln`) contains the following projects organized by category:

### Client
| Project | Output | Description |
|---------|--------|-------------|
| `game` | Game.exe | Main game client executable |
| `MindPower3D` | MindPower3D_D8R.dll | DirectX 8 rendering engine |

### Lua
| Project | Output | Description |
|---------|--------|-------------|
| `CaLua` | CaLua.dll | Lua C API wrapper for scripting |

### Server
| Project | Output | Description |
|---------|--------|-------------|
| `AccountServer` | AccountServer.exe | Authentication & account management |
| `GateServer` | GateServer.exe | Client connection gateway |
| `GameServer` | GameServer.exe | Game world logic & simulation |
| `GroupServer` | GroupServer.exe | Cross-server features (guilds, chat) |

### Libraries
| Project | Output | Description |
|---------|--------|-------------|
| `AudioSDL` | AudioSDL.lib | SDL2 audio wrapper |
| `Common` | Common.lib | Shared utilities |
| `EncLib` | EncLib.lib | Encryption (Botan) |
| `ICUHelper` | ICUHelper.lib | Unicode text processing |
| `InfoNet` | InfoNet.lib | Network information |
| `LIBDBC` | LIBDBC.lib | Database connectivity |
| `Status` | Status.lib | Character status/buff system |
| `Util` | Util.lib | General utilities |

---

## Build Warnings (Safe to Ignore)

The following warnings are expected and can be safely ignored:

| Warning | Description | Resolution |
|---------|-------------|------------|
| **LNK4098** | LIBCMT library conflicts | Handled via /NODEFAULTLIB linker flag |
| **LNK4099** | Missing PDB for lua51.lib | Precompiled library, no source PDB |
| **D9025** | Compiler flag overrides | Exception handling settings |
| **Botan deprecation** | Library-internal warnings | No functional impact |

---

## Dependencies

### Included Libraries

| Library | Version | Location | Purpose |
|---------|---------|----------|---------|
| DirectX SDK | 8.x | `include/directx/` | Rendering |
| LuaJIT | 2.x | `lib/`, `include/luajit/` | Scripting |
| SDL2 | 2.x | `lib/`, `include/sdl/` | Audio, input |
| SDL2_mixer | 2.x | `lib/` | Audio mixing |
| Botan | 2.x | `lib/`, `include/botan/` | Cryptography |
| ICU | 76.1 | `lib/`, `include/unicode/` | Text processing |

### Runtime DLLs Required

These DLLs must be present in `client/system/` for the game to run:

- `icudt76.dll` - ICU data
- `icuin76.dll` - ICU i18n
- `icuuc76.dll` - ICU common
- `SDL2.dll` - SDL2 runtime
- `SDL2_mixer.dll` - SDL2 Mixer runtime

---

## Troubleshooting

### Build Fails: Cannot find MSBuild

**Error:** `The term 'MSBuild.exe' is not recognized...`

**Solution:** Use the full path to MSBuild:
```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" ...
```

Or add MSBuild to your PATH via Developer Command Prompt.

### Build Fails: Missing SDK

**Error:** `Windows SDK version X not found`

**Solution:** Install the required Windows SDK via Visual Studio Installer:
1. Open Visual Studio Installer
2. Modify your VS2022 installation
3. Under "Individual Components", select the required Windows SDK

### Build Fails: Missing Platform Toolset

**Error:** `The build tools for v143 cannot be found`

**Solution:** Ensure "MSVC v143 - VS 2022 C++ x64/x86 build tools" is installed:
1. Open Visual Studio Installer
2. Modify installation
3. Select the v143 toolset under "Individual Components"

### Link Errors: Unresolved Symbols

**Error:** `LNK2019: unresolved external symbol`

**Solution:** 
1. Verify all library projects are built first
2. Check that library paths in project settings are correct
3. Ensure Release/Debug configuration matches across projects

---

## Development Tips

### Rebuild Single Project
Instead of rebuilding the entire solution, rebuild only the changed project:
```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "build\game\game.vcxproj" /p:Configuration=Release /p:Platform=x64 /t:Rebuild
```

### Parallel Building
Use `/m` flag for parallel builds (already included in examples):
```powershell
/m              # Use all available cores
/m:4            # Use specific number of cores
```

### Verbose Output
For debugging build issues, increase verbosity:
```powershell
/v:detailed     # Detailed output
/v:diagnostic   # Maximum verbosity
```

### Build Specific Configuration
```powershell
/p:Configuration=Debug      # Debug build
/p:Configuration=Release    # Release build
/p:Platform=x64            # 64-bit build
```

---

## See Also

- [CRASH_DUMP_GUIDE.md](CRASH_DUMP_GUIDE.md) - Debugging crash dumps
- [X64_LIBRARY_CHECKLIST.md](X64_LIBRARY_CHECKLIST.md) - 64-bit migration notes
- [copilot-instructions.md](../.github/copilot-instructions.md) - Full project documentation

---

*Last updated: January 27, 2026*
