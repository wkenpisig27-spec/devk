# PKO Source - Build Guide

## Directory Layout

```
source/
├── source.sln              # ═══ WINDOWS build (Visual Studio 2022)
├── CMakeLists.txt          # ═══ LINUX build (CMake + GCC/Clang)
│
├── src/                    # Shared C++ source code
├── include/                # Shared headers
│   └── linux_compat/       # Linux-specific platform shims
│
├── build/                  # Windows .vcxproj files (per-project)
├── bin/                    # Windows build output (Release/Debug)
├── lib/                    # Windows pre-built libraries (.lib)
│   └── Release/            # Release .lib files
├── tmp/                    # Windows intermediate files (.obj, .pch)
│
├── out/                    # Linux build output (generated, gitignored)
│   └── linux/
│       ├── bin/            # Compiled server binaries
│       └── lib/            # Compiled static libraries
│
└── scripts/                # ═══ BUILD & DEPLOY SCRIPTS
    ├── build-windows.bat   # MSBuild wrapper
    ├── build-linux.sh      # CMake configure + make
    ├── deploy-windows.bat  # Symlink binaries to client/server dirs
    ├── deploy-linux.sh     # Copy binaries to server deploy dir
    ├── start-servers.sh    # Launch servers in tmux (Linux)
    └── stop-servers.sh     # Stop all servers (Linux)
```

---

## Windows Build

**Requirements:**
- Visual Studio 2022 (v143 toolset)
- DirectX 8 SDK (included in `include/directx/`)
- x64 platform

### Quick Build

```batch
cd source\scripts
build-windows.bat              :: Release build
build-windows.bat Debug        :: Debug build
build-windows.bat Release clean :: Clean
```

### Visual Studio

1. Open `source/source.sln` in Visual Studio 2022
2. Select **Release | x64** (or Debug | x64)
3. Build Solution (Ctrl+Shift+B)

### Deploy (Windows)

```batch
cd source\scripts
deploy-windows.bat             :: Symlink Release binaries
deploy-windows.bat Debug       :: Symlink Debug binaries
```

Creates symbolic links:
- `bin/Release/` → `../server/*.exe` (server binaries)
- `bin/Release/` → `../client/system/` (client binaries + DLLs)

### Output

| Binary | Path |
|--------|------|
| AccountServer.exe | `bin/Release/accountserver/` |
| GateServer.exe | `bin/Release/gateserver/` |
| GroupServer.exe | `bin/Release/groupserver/` |
| GameServer.exe | `bin/Release/gameserver/` |
| Game.exe | `bin/Release/game/` |
| MindPower3D_D9R.dll | `bin/Release/mindpower3d/` |
| CaLua.dll | `bin/Release/calua/` |

---

## Linux Build

**Requirements:**
- GCC 10+ or Clang 12+
- CMake 3.16+
- Ubuntu/Debian packages:
  ```bash
  apt install build-essential cmake libunixodbc-dev libssl-dev \
              libicu-dev libluajit-5.1-dev libbotan-2-dev tmux
  ```

### Quick Build

```bash
cd source/scripts
chmod +x *.sh
./build-linux.sh                # Full build (configure + compile)
./build-linux.sh --build-only   # Recompile only (skip cmake)
./build-linux.sh --rebuild      # Clean + full build
./build-linux.sh --clean        # Remove build directory
```

### Manual Build

```bash
cd source
mkdir -p out/linux && cd out/linux
cmake ../.. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Deploy (Linux)

```bash
cd source/scripts
./deploy-linux.sh     # Copy binaries + configs to deploy dir
```

Default deploy directory: `/home/$USER/pkodev-server/`

Override: `PKO_DEPLOY_DIR=/custom/path ./deploy-linux.sh`

### Launch Servers

```bash
cd source/scripts
./start-servers.sh    # Starts all 4 servers in tmux
./stop-servers.sh     # Stops all servers
tmux attach -t pko    # View server consoles
```

### Output

| Binary | Path |
|--------|------|
| AccountServer | `out/linux/bin/` |
| GateServer | `out/linux/bin/` |
| GroupServer | `out/linux/bin/` |
| GameServer | `out/linux/bin/` |

> **Note:** Linux builds server binaries only. The game client (Game.exe, engine DLLs) is Windows-only.

---

## Environment Variables (Linux Scripts)

| Variable | Default | Description |
|----------|---------|-------------|
| `PKO_BUILD_DIR` | `source/out/linux` | CMake build directory |
| `PKO_BUILD_TYPE` | `Release` | CMake build type |
| `PKO_JOBS` | `$(nproc)` | Parallel compilation jobs |
| `PKO_DEPLOY_DIR` | `/home/$USER/pkodev-server` | Server deploy directory |

---

## Platform Differences

| Aspect | Windows | Linux |
|--------|---------|-------|
| Build system | Visual Studio .sln | CMake + Make |
| Compiler | MSVC v143 | GCC/Clang |
| Data model | LLP64 (`long` = 4 bytes) | LP64 (`long` = 8 bytes) |
| Networking | Winsock2 + IOCP | POSIX sockets + select() |
| Compatibility | Native | `include/linux_compat/` shims |
| Builds client? | Yes | No (server only) |
| Build output | `bin/Release/` | `out/linux/bin/` |
| Libraries | `lib/Release/*.lib` | System packages (apt) |

### Key Platform Notes

- **`long` type**: Fixed to `int` across server code for LP64 compatibility. This is a no-op on Windows (both are 4 bytes). See commit: `fix: replace long with int for Linux x64 LP64 compatibility`
- **Platform shims**: `include/linux_compat/` provides Windows API equivalents (windows.h, tchar.h, etc.)
- **`platform_compat.h`**: `include/serversdk/platform_compat.h` defines cross-platform types (DWORD, LONG, BOOL, etc.)
