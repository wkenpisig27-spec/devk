# Offline License System Integration Guide

## Overview

The offline license system validates licenses locally without any network connection.

```
┌─────────────────────────────────────────────────┐
│              YOU (License Provider)              │
│                                                  │
│  1. Customer sends you their HWID               │
│  2. You run LicenseGenerator.exe                │
│  3. You send them the .lic file                 │
└─────────────────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────┐
│              CUSTOMER                            │
│                                                  │
│  1. Places license.lic next to the .exe         │
│  2. Runs game/server                            │
│  3. License validated locally                   │
└─────────────────────────────────────────────────┘
```

## Step 1: Change the Secret Key

**CRITICAL:** Before compiling anything, change the secret key!

### In `helper/license/LicenseGenerator.cpp` (line ~30):
```cpp
static const char* LICENSE_SECRET_KEY = "YOUR_NEW_SECRET_KEY_HERE";
```

### In `src/common/LicenseValidator.cpp` (line ~36):
```cpp
static const char* LICENSE_SECRET_KEY = "YOUR_NEW_SECRET_KEY_HERE";
```

Use a strong random key: `openssl rand -hex 32`

**The keys MUST be identical!**

## Step 2: Add to Visual Studio Projects

Add these files to each project that needs license protection:

### Files to Add:
- `src/common/LicenseValidator.h`
- `src/common/LicenseValidator.cpp`

### Required Libraries (Linker > Input > Additional Dependencies):
```
crypt32.lib
iphlpapi.lib
bcrypt.lib
```

## Step 3: Add License Check to Each Binary

### GameServer (`src/gameserver/GameSMain.cpp`)

Add include at the top:
```cpp
#include "LicenseValidator.h"
```

Add validation at the VERY START of main():
```cpp
int main(int argc, char* argv[]) {
    // =============================================
    // LICENSE CHECK - MUST BE FIRST!
    // =============================================
    if (!License::ValidateOrExit("license.lic", "GameServer")) {
        return 1;
    }
    // =============================================
    
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    // ... rest of existing code
}
```

### AccountServer (`src/accountserver/main.cpp`)

```cpp
#include "LicenseValidator.h"

int main(int argc, char* argv[]) {
    if (!License::ValidateOrExit("license.lic", "AccountServer")) {
        return 1;
    }
    // ... rest of existing code
}
```

### GateServer (`src/gateserver/main.cpp`)

```cpp
#include "LicenseValidator.h"

int main(int argc, char* argv[]) {
    if (!License::ValidateOrExit("license.lic", "GateServer")) {
        return 1;
    }
    // ... rest of existing code
}
```

### GroupServer (`src/groupserver/GroupSMain.cpp`)

```cpp
#include "LicenseValidator.h"

int main(int argc, char* argv[]) {
    if (!License::ValidateOrExit("license.lic", "GroupServer")) {
        return 1;
    }
    // ... rest of existing code
}
```

### Game Client (`src/game/Main.cpp`)

```cpp
#include "LicenseValidator.h"

// In WinMain or main:
int main(int argc, char* argv[]) {
    if (!License::ValidateOrExit("license.lic", "GameClient")) {
        return 1;
    }
    // ... rest of existing code
}
```

## Step 4: Advanced Usage (Optional)

For more control over validation:

```cpp
#include "LicenseValidator.h"

int main() {
    License::Validator validator;
    
    // Get machine's HWID
    std::string hwid = License::Validator::getHardwareId();
    printf("Machine HWID: %s\n", hwid.c_str());
    
    // Validate license
    License::LicenseInfo info = validator.validateFile("license.lic", "MyProduct");
    
    if (!info.valid) {
        // Handle specific errors
        switch (info.result) {
            case License::ValidationResult::FILE_NOT_FOUND:
                printf("License file not found!\n");
                break;
            case License::ValidationResult::INVALID_LICENSE:
                printf("License is invalid or corrupted!\n");
                break;
            case License::ValidationResult::LICENSE_EXPIRED:
                printf("License has expired!\n");
                break;
            case License::ValidationResult::HWID_MISMATCH:
                printf("License is for a different machine!\n");
                printf("Your HWID: %s\n", hwid.c_str());
                break;
            default:
                printf("Unknown error: %s\n", info.message.c_str());
        }
        return 1;
    }
    
    // License is valid!
    printf("Licensed to: %s\n", info.owner.c_str());
    printf("Product type: %s\n", info.productType.c_str());
    
    if (info.expiresAt > 0) {
        time_t now = time(nullptr);
        int daysLeft = (info.expiresAt - now) / 86400;
        printf("Expires in %d days\n", daysLeft);
    }
    
    // Continue with application...
    return 0;
}
```

## Step 5: Deployment

When distributing your software:

1. **Build** all binaries with license validation integrated
2. **Send customers** the `GetHWID.exe` tool
3. **Customer runs** GetHWID and sends you the result
4. **You generate** a license: `LicenseGenerator.exe -o "Customer" -h "their-hwid"`
5. **Send back** the `license.lic` file
6. **Customer places** `license.lic` in same folder as the executable

## File Layout for Customers

```
GameServer/
├── GameServer.exe
├── license.lic          <-- The license file goes here
├── GameDB.cfg
└── ... other files

Client/
├── Game.exe
├── license.lic          <-- License file for client
└── ... other files
```

## Troubleshooting

### "License file not found"
- Ensure `license.lic` is in the same directory as the executable
- Check the filename is exactly `license.lic`

### "Invalid or corrupted license"
- License was generated with a different secret key
- File was modified or corrupted
- Regenerate the license

### "License is for a different machine"
- The HWID in the license doesn't match
- Customer may have changed hardware
- Use `GetHWID.exe` to get current HWID and regenerate

### "License has expired"
- The expiry date has passed
- Generate a new license with extended date

## Security Considerations

1. **Never distribute `LicenseGenerator.exe`** - only you should have it
2. **Keep your secret key safe** - if leaked, anyone can generate licenses
3. **Consider obfuscating your binaries** - use tools like UPX, Themida, etc.
4. **The license system deters** casual copying but isn't uncrackable
5. **For maximum security**, combine with code obfuscation and anti-debugging
