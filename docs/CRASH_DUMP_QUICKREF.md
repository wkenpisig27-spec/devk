# Crash Dump Quick Reference

## When You Get a Crash Dump File

### Quick Analysis in Visual Studio

```
1. File → Open → File → Select crash_*.dmp
2. Click "Debug with Native Only"
3. View → Call Stack (Ctrl+Alt+C)
4. View → Modules (Ctrl+Alt+U)
```

### What You'll See

- **Call Stack Window:** Shows function calls leading to the crash
- **Exception Code:** The type of crash (e.g., 0xC0000005 = Access Violation)
- **Exception Address:** Where in memory the crash occurred
- **Modules Window:** All loaded DLLs and their versions

## Common Crash Types

| Exception Code | Name | Common Cause |
|---------------|------|--------------|
| 0xC0000005 | Access Violation | Null pointer, bad memory access |
| 0xC000001D | Illegal Instruction | CPU instruction not supported |
| 0xC0000094 | Integer Division by Zero | Divide by zero |
| 0xC000008C | Array Bounds Exceeded | Buffer overflow |
| 0xE06D7363 | C++ Exception | Unhandled throw |
| 0xC00000FD | Stack Overflow | Infinite recursion, huge locals |

## File Locations After Crash

### Client
```
client/
  ├── crash_20260122_143045.dmp  ← Minidump
  └── exception.txt               ← Text log
```

### Servers
```
server/
  ├── crash_20260122_143045.dmp  ← Minidump
  ├── exception.txt               ← Text log (AccountServer/GateServer)
  └── LOG/
      └── exception.txt           ← Text log (GameServer/GroupServer)
```

## Debugging Without Symbols

If you don't have .pdb files:

1. **Module addresses** - Still shows what DLL crashed
2. **Assembly view** - Can see raw instructions
3. **Memory view** - Can inspect memory at crash point
4. **Thread info** - Can see which thread crashed

**Better solution:** Always save .pdb files when building releases!

## Common Debugging Commands (Visual Studio)

| Action | Shortcut | Purpose |
|--------|----------|---------|
| View Call Stack | Ctrl+Alt+C | See function chain |
| View Threads | Ctrl+Alt+H | All thread states |
| View Modules | Ctrl+Alt+U | Loaded DLLs |
| View Memory | Ctrl+Alt+M,1 | Raw memory |
| View Disassembly | Alt+8 | Assembly code |
| Find Symbol | Ctrl+Alt+F3 | Search for function |

## WinDbg Commands Cheat Sheet

```cmd
!analyze -v              # Automatic crash analysis
k                        # Call stack
kp                       # Call stack with parameters
lm                       # List modules
!threads                 # All threads
.exr -1                  # Exception record
.ecxr                    # Set context to exception
r                        # Show registers
dv                       # Display local variables
!address                 # Memory regions
!handle                  # Handle information
.reload                  # Reload symbols
```

## Production Monitoring Script

Save as `check_crashes.bat` in server directory:

```batch
@echo off
echo Checking for crash dumps...
dir /b crash_*.dmp 2>nul
if errorlevel 1 (
    echo No crash dumps found.
) else (
    echo WARNING: Crash dumps detected!
    echo Please analyze and investigate.
)
```

Run this in a scheduled task or monitoring script.

## Enabling Release Build Symbols

For production debugging with optimization:

**Visual Studio Project Settings:**
```
Configuration: Release
Platform: Win32

C/C++ → General:
  Debug Information Format: Program Database (/Zi)

Linker → Debugging:
  Generate Debug Info: Yes (/DEBUG)
  
Linker → Optimization:
  References: Yes (/OPT:REF)
  Enable COMDAT Folding: Yes (/OPT:ICF)
```

This gives you symbols while keeping release optimizations.

## Archiving Old Dumps

```batch
@echo off
REM Keep only last 20 dumps
setlocal enabledelayedexpansion
set count=0
for /f "delims=" %%f in ('dir /b /o-d crash_*.dmp 2^>nul') do (
    set /a count+=1
    if !count! gtr 20 (
        echo Deleting old dump: %%f
        del "%%f"
    )
)
```

## Symbol Server Setup (Advanced)

If managing multiple builds:

```
1. Create symbol store folder: C:\SymbolStore
2. After each build:
   symstore add /r /f "bin\Release\*.pdb" /s C:\SymbolStore /t "ProjectName"
3. In debugger symbol paths:
   C:\SymbolStore;srv*c:\symbols*https://msdl.microsoft.com/download/symbols
```

## Emergency Crash Analysis (No Tools)

If you only have the .dmp file and no debugger:

1. Use Windows Error Reporting:
   - Open Control Panel → Action Center → View reliability history
   - Double-click crash event
   - Shows basic info: exception code, fault module, offset

2. Read exception.txt:
   - Timestamp and basic stack trace included
   - Function names if symbols were available at crash time

## Crash Prevention Best Practices

1. **Null checks:** Always check pointers before dereferencing
2. **Bounds checks:** Validate array indices
3. **Exception handling:** Use try/catch blocks
4. **Smart pointers:** Use shared_ptr/unique_ptr to prevent leaks
5. **Initialize variables:** Don't use uninitialized memory
6. **Thread safety:** Protect shared data with mutexes
7. **Validate input:** Check user/network data before using

## Testing Crash Handling

Force a crash to test dump generation:

```cpp
// In a test function
int* crashTest = nullptr;
*crashTest = 42;  // Intentional access violation
```

Or call:
```cpp
RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, 0, nullptr);
```

Verify both exception.txt and crash_*.dmp are created.

## Further Reading

- **Visual Studio Docs:** Search for "Debug dump files"
- **WinDbg Tutorial:** Download from Windows SDK
- **Debugging Tools:** https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/

---

**Quick Help:** For step-by-step guides, see [CRASH_DUMP_GUIDE.md](CRASH_DUMP_GUIDE.md)
