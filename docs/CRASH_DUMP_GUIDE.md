# Crash Dump Generation Guide

## Overview

This project now includes automatic crash dump generation for all server and client executables. When a crash occurs, the system will generate:
1. **exception.txt** - A text log with timestamp and stack trace (existing feature)
2. **crash_YYYYMMDD_HHMMSS.dmp** - A minidump file for detailed analysis (NEW)

## What Changed

### Core Components

#### ErrorHandler Class (source/src/util/ErrorHandler.cpp)
- Added `CreateMiniDump()` static method that uses Windows DbgHelp API
- Modified `UnhandledExceptionFilter()` to generate minidump files alongside exception.txt
- Uses `MiniDumpWriteDump()` from DbgHelp.dll to create crash dumps

#### Server Executables
All server main files now call `ErrorHandler::Initialize()` at startup:
- **AccountServer** (source/src/accountserver/main.cpp)
- **GateServer** (source/src/gateserver/main.cpp)
- **GameServer** (source/src/gameserver/GameSMain.cpp)
- **GroupServer** (source/src/groupserver/GroupSMain.cpp)

#### Client Executable
- **Game.exe** (source/src/game/Main.cpp) - Already had ErrorHandler::Initialize()

## How It Works

### When a Crash Occurs

1. The unhandled exception filter catches the crash
2. Creates a text log with stack trace in `exception.txt`
3. Generates a minidump file with timestamp: `crash_20260122_143045.dmp`
4. Both files are saved in the application's directory
5. The application then shows the standard crash dialog (or exits cleanly)

### Minidump Contents

The minidump includes:
- **MiniDumpNormal** - Thread stacks, loaded modules, basic process info
- **MiniDumpWithHandleData** - Handle information
- **MiniDumpWithThreadInfo** - Thread times and priority
- **MiniDumpWithUnloadedModules** - Recently unloaded DLLs

This provides essential debugging information while keeping file sizes manageable (~100KB-2MB typically).

## How to Analyze Crash Dumps

### Option 1: Visual Studio (Recommended)

1. Open Visual Studio (2015-2022)
2. File → Open → File → Select the .dmp file
3. Click "Debug with Native Only" (or Mixed if needed)
4. Visual Studio will show:
   - Call stack at time of crash
   - Thread information
   - Module (DLL/EXE) information
   - Exception code and address

**Important:** You need the matching .pdb (symbol) files from the build that created the executable.

### Option 2: WinDbg (Advanced)

1. Download WinDbg from Microsoft (part of Windows SDK)
2. Open the .dmp file in WinDbg
3. Set symbol path: `.sympath srv*c:\symbols*https://msdl.microsoft.com/download/symbols`
4. Load symbols: `.reload`
5. Analyze crash:
   ```
   !analyze -v     # Detailed analysis
   k              # Call stack
   lm             # Loaded modules
   !threads       # Thread information
   ```

### Option 3: Debugging Symbols Setup

For best results, ensure you have .pdb files:

1. **Debug Build:** Symbol files (.pdb) are generated automatically
2. **Release Build:** Enable symbol generation:
   - Project Properties → Linker → Debugging
   - Generate Debug Info: "Generate Debug Information (/DEBUG)"
   - This adds symbols without affecting optimization

## File Locations

### Client (Game.exe)
- Crash dumps saved to: `client/crash_*.dmp`
- Exception log: `client/exception.txt`

### Servers
- **AccountServer:** Crash dumps in server root or LOG directory
- **GateServer:** Crash dumps in server root or LOG directory  
- **GameServer:** Crash dumps in server root or LOG directory
- **GroupServer:** Crash dumps in server root or LOG directory

## Configuration Options

You can modify the amount of information captured in the dump by editing `ErrorHandler.cpp`:

```cpp
// Current setting (recommended for production):
MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(
    MiniDumpNormal |                  // Essential info only
    MiniDumpWithHandleData |         // Handle info
    MiniDumpWithThreadInfo |         // Thread details
    MiniDumpWithUnloadedModules      // Recently unloaded DLLs
);

// For more detailed dumps (larger files, 100MB+):
MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(
    MiniDumpWithFullMemory |         // All accessible memory
    MiniDumpWithHandleData |
    MiniDumpWithThreadInfo |
    MiniDumpWithUnloadedModules
);
```

## Dependencies

- **DbgHelp.dll** - Built into Windows (XP SP1 and later)
- **DbgHelp.lib** - Linked automatically via `#pragma comment(lib, "DbgHelp.lib")`
- No additional DLLs or installations required

## Troubleshooting

### Dump Files Not Generated

1. **Check permissions:** Ensure the application has write access to its directory
2. **Check disk space:** Minidumps need 100KB-2MB of free space
3. **Check minidump_status.log:** Created on successful dump generation

### Cannot Open Dump in Visual Studio

1. Ensure you're using the same VS version that compiled the executable
2. Copy matching .pdb files to the same directory as the .dmp file
3. Try "Debug with Native Only" instead of "Debug with Mixed"

### Symbols Not Loading

1. Build the project in Debug configuration for full symbols
2. Or enable /DEBUG for Release builds
3. Ensure .pdb files are in the same directory as the executable when it crashed

## Best Practices

### Production Servers

1. **Monitor dump files:** Set up automated collection of .dmp files
2. **Archive old dumps:** Keep last 10-20 dumps, delete older ones
3. **Alert on crashes:** Monitor for new crash_*.dmp files
4. **Symbol server:** Maintain a symbol server with all .pdb files for each release

### Development

1. **Always use Debug builds** during development for full debugging info
2. **Keep .pdb files** for every release build you deploy
3. **Test crash handling** by triggering intentional crashes
4. **Verify dumps open** in Visual Studio after each build

### Debugging Process

1. Collect the crash dump file
2. Identify the build/version that generated it
3. Get the matching .pdb files from that build
4. Open in Visual Studio or WinDbg
5. Analyze call stack and exception details
6. Fix the bug and deploy new version

## Security Considerations

Minidump files may contain:
- Memory contents (variable values, strings)
- Stack traces with function names
- Module names and paths
- Thread information

**Recommendation:** Treat dump files as sensitive and do not share publicly without reviewing contents.

## Additional Resources

- [Microsoft Docs: MiniDumpWriteDump](https://docs.microsoft.com/en-us/windows/win32/api/minidumpapiset/nf-minidumpapiset-minidumpwritedump)
- [Debugging with WinDbg](https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/)
- [Visual Studio Crash Dump Debugging](https://docs.microsoft.com/en-us/visualstudio/debugger/using-dump-files)

## Implementation Details

### Files Modified

1. `source/include/util/ErrorHandler.h` - Added CreateMiniDump declaration
2. `source/src/util/ErrorHandler.cpp` - Implemented minidump generation
3. `source/src/accountserver/main.cpp` - Added ErrorHandler::Initialize()
4. `source/src/gateserver/main.cpp` - Added ErrorHandler::Initialize()
5. `source/src/gameserver/GameSMain.cpp` - Added ErrorHandler::Initialize()
6. `source/src/groupserver/GroupSMain.cpp` - Added ErrorHandler::Initialize()

### Code Example

To manually trigger minidump generation in your own code:

```cpp
#include "ErrorHandler.h"

// In exception handler or crash scenario:
ErrorHandler::CreateMiniDump(pExceptionInfo, "custom_crash.dmp");
```

---

**Last Updated:** January 22, 2026  
**Version:** 1.0
