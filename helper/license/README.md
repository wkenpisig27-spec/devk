# PKO Offline License System

This folder contains tools for generating and validating offline licenses.

## Files

| File | Purpose | Distribute? |
|------|---------|-------------|
| `LicenseGenerator.cpp` | Generate license files | **NO! Keep private!** |
| `GetHWID.cpp` | Extract machine Hardware ID | Yes, give to customers |
| `build.bat` | Compile the tools | No |

## Quick Start

### 1. Build the Tools

```batch
build.bat
```

This creates:
- `LicenseGenerator.exe` - Your private license generator
- `GetHWID.exe` - Tool to give to customers

### 2. Generate a License

**Interactive mode:**
```batch
LicenseGenerator.exe
```

**Command line mode:**
```batch
LicenseGenerator.exe -o "Customer Name" -f license.lic
```

**With machine binding:**
```batch
LicenseGenerator.exe -o "Customer" -h "abc123def456..." -f license.lic
```

**With expiry date:**
```batch
LicenseGenerator.exe -o "Customer" -e 2025-12-31 -f license.lic
```

### 3. Get Customer's HWID

Give `GetHWID.exe` to the customer. They run it and send you the displayed ID.

Then generate a machine-bound license:
```batch
LicenseGenerator.exe -o "Customer" -h "their-hwid-here" -f license.lic
```

## Integration

Add the license check to your game binaries. See `src/common/LicenseValidator.h`.

### Example Integration (GameServer)

```cpp
#include "LicenseValidator.h"

int main(int argc, char* argv[]) {
    // Validate license first!
    if (!License::ValidateOrExit("license.lic", "GameServer")) {
        return 1;  // Exit if invalid
    }
    
    // ... rest of your code
}
```

## License Types

| Type | HWID | Expiry | Use Case |
|------|------|--------|----------|
| Universal | `*` | Never | Testing, trusted users |
| Machine-bound | Specific HWID | Never | Production licenses |
| Time-limited | `*` or HWID | Date | Trial/subscription |

## Security Notes

1. **NEVER distribute LicenseGenerator.exe** - Only you should have this!
2. **Change the secret key** in both:
   - `LicenseGenerator.cpp` (line ~30)
   - `src/common/LicenseValidator.cpp` (line ~36)
3. The secret key MUST be identical in both files
4. Use a strong random key: `openssl rand -hex 32`
5. Compile the tools AFTER changing the key

## Command Line Options

```
LicenseGenerator.exe [options]

Options:
  -o, --owner <name>     Owner name (required)
  -t, --type <type>      Product type: full, server, client (default: full)
  -h, --hwid <hwid>      Hardware ID or * for any machine (default: *)
  -e, --expires <date>   Expiry date YYYY-MM-DD or 'never' (default: never)
  -f, --file <path>      Output file path (default: license.lic)
  --help                 Show help
```

## Examples

```batch
# Universal perpetual license
LicenseGenerator.exe -o "John Doe" -f john.lic

# Machine-bound license
LicenseGenerator.exe -o "Jane" -h "a1b2c3d4e5f6..." -f jane.lic

# 1-year trial
LicenseGenerator.exe -o "Trial User" -e 2026-01-21 -f trial.lic

# Server-only license
LicenseGenerator.exe -o "Company" -t server -f company_server.lic
```
