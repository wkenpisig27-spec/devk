# x64 Client Migration - Library Acquisition Checklist

> **Created:** January 7, 2026  
> **Project:** PKODev Client x64 Migration  
> **Target:** `source/lib/x64/Release/` and `source/lib/x64/Debug/`

---

## Overview

This checklist covers all libraries needed to build the PKODev client for x64 architecture.

### Library Status Summary

| Status | Count | Description |
|--------|-------|-------------|
| ✅ | 12 | Already have x64 version |
| 🔨 | 5 | Need to build from project source |
| 📥 | 6 | Need to download x64 version |
| ❓ | 1 | Unknown - may need replacement |

---

## ✅ Already Available (No Action Needed)

These libraries are already in `source/lib/x64/Release/`:

- [x] `lua51.lib` - LuaJIT
- [x] `botan.lib` - Cryptography
- [x] `Common.lib` - Common utilities
- [x] `util.lib` - Utility functions
- [x] `LIBDBC.lib` - Database connectivity
- [x] `InfoNet.lib` - Network library
- [x] `ICUHelper.lib` - ICU wrapper
- [x] `icudt.lib` - ICU data
- [x] `icuuc.lib` - ICU common
- [x] `icuin.lib` - ICU i18n
- [x] `icuind.lib` - ICU i18n (debug)
- [x] `icuucd.lib` - ICU common (debug)

---

## 📥 Download Required

### 1. DirectX Libraries (from Windows SDK)

**Source:** Windows SDK 10.0.26100.0 (already installed!)

**Location:** `C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64\`

**Available in your Windows SDK (just need to copy or reference):**
- [x] `d3d9.lib` - Direct3D 9 ✅ Available
- [x] `dxguid.lib` - DirectX GUIDs ✅ Available  
- [x] `ddraw.lib` - DirectDraw (legacy) ✅ Available
- [x] `dinput8.lib` - DirectInput ✅ Available

**Copy command:**
```powershell
$sdk = "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64"
$dest = "C:\Users\pisig\Desktop\Github\pkodev\source\lib\x64\Release"
Copy-Item "$sdk\d3d9.lib" $dest
Copy-Item "$sdk\dxguid.lib" $dest  
Copy-Item "$sdk\ddraw.lib" $dest
Copy-Item "$sdk\dinput8.lib" $dest
```

---

### 2. D3DX9 Library (Legacy - Requires DirectX SDK)

> ⚠️ **Note:** D3DX9 is deprecated but required for this project. You have Win32 versions but need x64.

**Current status:** Only Win32 versions in `source/lib/Win32/Release/d3dx9.lib`

**Option A - Install DirectX SDK (June 2010):**
1. Download: https://www.microsoft.com/en-us/download/details.aspx?id=6812
2. Install DirectX SDK (June 2010)
3. Copy from: `C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Lib\x64\`
   - [ ] `d3dx9.lib`
   - [ ] `d3dx9d.lib` (debug)

**Option B - Use Microsoft.DXSDK.D3DX NuGet package:**
```powershell
# In Visual Studio Package Manager Console
Install-Package Microsoft.DXSDK.D3DX -Version 9.29.952.3
```
Then copy x64 libs from packages folder.

---

### 2. SDL2 Libraries

**Source:** https://github.com/libsdl-org/SDL/releases

**Download:** SDL2-devel-2.x.x-VC.zip (latest stable)

**Steps:**
1. Go to https://github.com/libsdl-org/SDL/releases
2. Download `SDL2-devel-x.xx.x-VC.zip`
3. Extract and copy from `lib\x64\`:
   - [ ] `SDL2.lib` → rename to `SDL.lib`
   - [ ] `SDL2main.lib` → rename to `SDLmain.lib`
4. Copy DLL to client: `SDL2.dll`

---

### 3. SDL2_mixer Libraries

**Source:** https://github.com/libsdl-org/SDL_mixer/releases

**Download:** SDL2_mixer-devel-2.x.x-VC.zip

**Steps:**
1. Go to https://github.com/libsdl-org/SDL_mixer/releases
2. Download `SDL2_mixer-devel-x.x.x-VC.zip`
3. Extract and copy from `lib\x64\`:
   - [ ] `SDL2_mixer.lib` → rename to `SDL_mixer.lib`
4. Copy DLLs to client: `SDL2_mixer.dll` + codec DLLs

---

### 4. Discord RPC Library

**Source:** https://github.com/discord/discord-rpc

**Option A - Download Pre-built:**
1. Go to https://github.com/discord/discord-rpc/releases
2. Download `discord-rpc-win.zip`
3. Extract and copy from `lib\`:
   - [ ] `discord-rpc.lib` (x64 version)

**Option B - Build from Source:**
```powershell
git clone https://github.com/discord/discord-rpc.git
cd discord-rpc
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```
Output: `src/Release/discord-rpc.lib`

---

## 🔨 Build from Project Source

These libraries need to be built from your existing source code with x64 configuration.

### 5. AudioSDL.lib

**Project:** `source/build/audiosdl/AudioSDL.vcxproj`

**Steps:**
1. Open `AudioSDL.vcxproj` in Visual Studio
2. Add x64 platform configuration (copy from Win32)
3. Build for x64 Release
4. Output: `source/lib/x64/Release/AudioSDL.lib`

- [ ] Add x64 configuration to project
- [ ] Build x64 Release
- [ ] Build x64 Debug (optional)

---

### 6. Status.lib

**Project:** `source/build/status/Status.vcxproj`

**Steps:**
1. Open `Status.vcxproj` in Visual Studio
2. Add x64 platform configuration
3. Build for x64 Release
4. Output: `source/lib/x64/Release/Status.lib`

- [ ] Add x64 configuration to project
- [ ] Build x64 Release
- [ ] Build x64 Debug (optional)

---

### 7. EncLib.lib

**Project:** `source/build/enclib/EncLib.vcxproj`

**Steps:**
1. Open `EncLib.vcxproj` in Visual Studio
2. Add x64 platform configuration
3. Build for x64 Release
4. Output: `source/lib/x64/Release/EncLib.lib`

- [ ] Add x64 configuration to project
- [ ] Build x64 Release
- [ ] Build x64 Debug (optional)

---

### 8. CaLua.lib

**Project:** `source/build/calua/CaLua.vcxproj`

**Status:** ✅ Already has x64 configuration!

**Steps:**
1. Open solution and select x64 platform
2. Build for x64 Release
3. Output: `source/lib/x64/Release/CaLua.lib`

- [ ] Build x64 Release
- [ ] Build x64 Debug (optional)

---

### 9. MindPower3D.lib (Engine DLL)

**Project:** `source/build/engine/MindPower3D.vcxproj`

**Steps:**
1. Open `MindPower3D.vcxproj` in Visual Studio
2. Add x64 platform configuration
3. Update DirectX library paths for x64
4. Build for x64 Release
5. Output: 
   - `source/lib/x64/Release/MindPower3D_D9R.lib`
   - `source/bin/x64/Release/MindPower3D_D9R.dll`

- [ ] Add x64 configuration to project
- [ ] Update library paths
- [ ] Build x64 Release
- [ ] Build x64 Debug (optional)

---

## ❓ Unknown / May Need Replacement

### 10. PAI.lib (Billing/Payment Library)

**Status:** Unknown - appears to be a proprietary billing library

**Options:**

**Option A - Find x64 version:**
- [ ] Check if you have source code for PAI library
- [ ] Check `source-libs/` for PAI source
- [ ] Contact original vendor for x64 build

**Option B - Remove dependency:**
- [ ] Check what PAI.lib provides (billing functions?)
- [ ] Implement stub functions if not needed
- [ ] Remove from project if unused

**Option C - Build from source (if available):**
- [ ] Locate PAI source code
- [ ] Add x64 configuration
- [ ] Build for x64

**Investigation needed:**
```powershell
# Search for PAI references in codebase
Get-ChildItem -Path "source" -Recurse -Include "*.cpp","*.h" | Select-String -Pattern "PAI|pai\.lib" -List
```

---

## 📋 System Libraries (Auto-provided by Visual Studio)

These are provided by Visual Studio for x64 builds automatically:

- [x] `atls.lib` - ATL Server library
- [x] `Ws2_32.lib` - Winsock
- [x] `Psapi.lib` - Process Status API
- [x] `winmm.lib` - Windows Multimedia
- [x] `legacy_stdio_definitions.lib` - Legacy CRT

---

## 📁 Target Directory Structure

After completing this checklist, your `source/lib/x64/` should contain:

```
source/lib/x64/
├── Debug/
│   ├── AudioSDL_D.lib
│   ├── CaLua.lib
│   ├── Common_D.lib
│   ├── d3dx9d.lib
│   ├── EncLib_D.lib
│   ├── ICUHelper_D.lib
│   ├── icuucd.lib
│   ├── icuind.lib
│   ├── LIBDBC_D.lib
│   ├── MindPower3D_D9D.lib
│   ├── Status_D.lib
│   └── util_D.lib
│
└── Release/
    ├── AudioSDL.lib          🔨 Build
    ├── botan.lib             ✅ Have
    ├── CaLua.lib             🔨 Build (has config)
    ├── Common.lib            ✅ Have
    ├── d3d9.lib              📥 Windows SDK
    ├── d3dx9.lib             📥 DirectX SDK
    ├── ddraw.lib             📥 Windows SDK
    ├── dinput8.lib           📥 Windows SDK
    ├── discord-rpc.lib       📥 Download/Build
    ├── dxguid.lib            📥 Windows SDK
    ├── EncLib.lib            🔨 Build
    ├── ICUHelper.lib         ✅ Have
    ├── icudt.lib             ✅ Have
    ├── icuin.lib             ✅ Have
    ├── icuuc.lib             ✅ Have
    ├── InfoNet.lib           ✅ Have
    ├── LIBDBC.lib            ✅ Have
    ├── lua51.lib             ✅ Have
    ├── MindPower3D_D9R.lib   🔨 Build
    ├── PAI.lib               ❓ Unknown
    ├── SDL.lib               📥 Download
    ├── SDLmain.lib           📥 Download
    ├── SDL_mixer.lib         📥 Download
    ├── Status.lib            🔨 Build
    └── util.lib              ✅ Have
```

---

## 🚀 Recommended Order of Operations

1. **DirectX libraries** - Copy from Windows SDK (easiest)
2. **SDL2 libraries** - Download pre-built (quick)
3. **Discord RPC** - Download pre-built release
4. **CaLua** - Already has x64 config, just build
5. **AudioSDL, Status, EncLib** - Add x64 configs and build
6. **MindPower3D** - Add x64 config with DX9 paths
7. **PAI.lib** - Investigate and resolve last

---

## 📝 Notes

- Always copy both `.lib` files AND corresponding `.dll` files to client
- Debug builds need `*_D.lib` or `*d.lib` variants
- Test each library individually before full client build
- Keep Win32 builds working during migration for fallback

---

## Quick Commands

**Check Windows SDK location:**
```powershell
Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\Lib" -Directory | Select-Object Name
```

**Check DirectX SDK location:**
```powershell
Test-Path "C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)"
```

**List current x64 libraries:**
```powershell
Get-ChildItem "source\lib\x64\Release" -Filter "*.lib" | Select-Object Name
```
