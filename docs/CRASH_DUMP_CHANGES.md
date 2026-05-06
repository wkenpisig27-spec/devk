# Crash Dump Implementation - Summary of Changes

## What Was Added

Automatic crash dump (.dmp) file generation for all executables when they crash, providing detailed debugging information without requiring a debugger to be attached 24/7.

## Technical Implementation

### Core Crash Dump System

**File:** `source/src/util/ErrorHandler.cpp`
- Added `#include <DbgHelp.h>` and `#pragma comment(lib, "DbgHelp.lib")`
- Implemented `CreateMiniDump()` static method using Windows `MiniDumpWriteDump()` API
- Modified `UnhandledExceptionFilter()` to create timestamped dump files: `crash_YYYYMMDD_HHMMSS.dmp`

**File:** `source/include/util/ErrorHandler.h`
- Added declaration for `CreateMiniDump()` method

### Server Integration

All server executables now initialize crash dump handling at startup:

1. **AccountServer** (`source/src/accountserver/main.cpp`)
   - Added `#include "ErrorHandler.h"`
   - Added `ErrorHandler::Initialize()` call in main()

2. **GateServer** (`source/src/gateserver/main.cpp`)
   - Added `#include "ErrorHandler.h"`
   - Added `ErrorHandler::Initialize()` call in main()

3. **GameServer** (`source/src/gameserver/GameSMain.cpp`)
   - Added `#include "ErrorHandler.h"`
   - Added `ErrorHandler::Initialize()` call in main()

4. **GroupServer** (`source/src/groupserver/GroupSMain.cpp`)
   - Added `#include "ErrorHandler.h"`
   - Added `ErrorHandler::Initialize()` call in main()

### Client Integration

**Client** (`source/src/game/Main.cpp`)
- Already had `ErrorHandler::Initialize()` - no changes needed
- Will now automatically generate dumps on crash

## When a Crash Occurs

**Before (existing behavior):**
- Creates `exception.txt` with timestamp and text stack trace

**After (new behavior):**
- Creates `exception.txt` with timestamp and text stack trace
- **PLUS** Creates `crash_20260122_143045.dmp` minidump file with:
  - Complete call stacks for all threads
  - Thread information (IDs, priorities, times)
  - Loaded/unloaded module list
  - Handle information
  - Exception context and registers

## Minidump Contents

The dump includes these flags:
- `MiniDumpNormal` - Essential crash information
- `MiniDumpWithHandleData` - Open handle information
- `MiniDumpWithThreadInfo` - Thread details
- `MiniDumpWithUnloadedModules` - Recently unloaded DLLs

**File Size:** Typically 100KB-2MB (compact, production-friendly)

## How to Use

### When a Server/Client Crashes:

1. Find the generated `.dmp` file in the application directory
2. Open it in Visual Studio: File → Open → File → select .dmp
3. Click "Debug with Native Only"
4. View the complete call stack at the moment of crash

### For Best Results:

- Keep the matching `.pdb` (symbol) files from your build
- Place .pdb files in the same directory as the .dmp file when debugging
- Or use Release builds with /DEBUG flag enabled

## Files Modified

| File | Changes |
|------|---------|
| `source/include/util/ErrorHandler.h` | Added CreateMiniDump() declaration |
| `source/src/util/ErrorHandler.cpp` | Implemented minidump generation logic |
| `source/src/accountserver/main.cpp` | Added ErrorHandler::Initialize() |
| `source/src/gateserver/main.cpp` | Added ErrorHandler::Initialize() |
| `source/src/gameserver/GameSMain.cpp` | Added ErrorHandler::Initialize() |
| `source/src/groupserver/GroupSMain.cpp` | Added ErrorHandler::Initialize() |

## Dependencies

- **DbgHelp.dll** - Built into Windows (XP SP1+)
- **DbgHelp.lib** - Automatically linked via pragma
- **No external dependencies required**

## Testing

To test the crash dump generation:

1. Build the project
2. Run any server/client executable
3. Force a crash (access violation, null pointer, etc.)
4. Verify both files are created:
   - `exception.txt` (existing)
   - `crash_YYYYMMDD_HHMMSS.dmp` (new)
5. Open the .dmp file in Visual Studio to verify it's valid

## Next Steps

1. **Build the solution** to compile the changes
2. **Test** on a development server by forcing a crash
3. **Verify** that .dmp files are generated and can be opened in VS
4. **Deploy** to production servers
5. **Monitor** for crash dump files and analyze them when crashes occur

## Additional Documentation

See [CRASH_DUMP_GUIDE.md](CRASH_DUMP_GUIDE.md) for complete usage guide including:
- Detailed debugging instructions
- WinDbg usage
- Symbol server setup
- Production best practices
- Security considerations

---

**Implementation Date:** January 22, 2026  
**Status:** Complete and ready for testing
