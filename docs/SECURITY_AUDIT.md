# PKO Server Security Audit Report

> **Audit Date:** January 2025  
> **Scope:** PKODev Server Codebase (AccountServer, GateServer, GameServer, GroupServer)  
> **Classification:** Internal Security Assessment

---

## Executive Summary

This audit covers six security domains: Network Security, Authentication, Database Security, Input Validation, Game Exploit Prevention, and Encryption. The codebase shows **significant modernization efforts** with RSA/AES encryption, bcrypt password hashing, and Anti-DDoS protection. However, **critical SQL injection vulnerabilities** and legacy encryption weaknesses remain.

| Category | Risk Level | Status |
|----------|------------|--------|
| Network Security | 🟢 LOW | Well-implemented Anti-DDoS and packet validation |
| Authentication | 🟢 LOW | Modern bcrypt hashing, RSA key exchange |
| Database Security | 🔴 **CRITICAL** | SQL injection in `_get_row` filter parameters |
| Input Validation | 🟡 MEDIUM | Text filtering exists but inconsistently applied |
| Game Exploits | 🟡 MEDIUM | Anti-dupe exists, but race conditions possible |
| Encryption | 🟡 MEDIUM | Modern RSA/AES, but legacy DES still in codebase |

---

## 1. Network Security

### 1.1 Anti-DDoS Protection ✅ GOOD

**File:** [ToClient.cpp](source/src/gateserver/ToClient.cpp)

**Implementation:**
- IP-based connection rate limiting
- Packet rate limiting per connection
- Escalating ban durations (configurable via `GateServer.cfg`)
- Memory-bounded tracking (50,000 IP limit)
- Warning decay after 30 seconds of good behavior

**Code Sample (Lines 90-160):**
```cpp
bool ToClient::VerifyDDoS(DataSocket* datasock) {
    // Memory safeguard: limit map size to prevent unbounded growth during DDoS
    const size_t MAX_TRACKED_IPS = 50000;
    if (m_mapConnectionInfo.size() >= MAX_TRACKED_IPS) {
        // Unknown IP during overload - reject immediately
        Disconnect(datasock, 0, -31);
        return true;
    }
    // ... rate limiting logic
}
```

**Configuration Options:**
- `ConnectionMinInterval` - Minimum ms between connections
- `MaxConnectionsPerSecond` - Threshold before warnings
- `MaxPacketsPerSecond` - Packet rate limit
- `MaxLoginAttemptsPerMinute` - Login attempt throttle
- `BanDurationMinutes` - Base ban duration (escalates on repeat)

**Risk Level:** 🟢 LOW - Well-implemented protections

---

### 1.2 Packet Parsing

**File:** [Receiver.cpp](source/src/serversdk/Receiver.cpp), [Packet.cpp](source/src/serversdk/Packet.cpp)

**Findings:**
- Maximum packet size enforced: `16 * 1024` bytes (16KB)
- Length prefix validation before processing
- `ReadString()` null-terminates in-place

**Potential Issue - Buffer Handling:**
```cpp
// Packet.cpp - ReadString modifies buffer in-place
cChar *RPacket::ReadString(uShort *len) {
    // Adds null terminator by modifying underlying buffer
}
```
This is a code smell but not exploitable due to buffer size checks.

**Risk Level:** 🟢 LOW

---

## 2. Authentication

### 2.1 Password Hashing ✅ EXCELLENT

**File:** [DataBaseCtrl.cpp](source/src/accountserver/DataBaseCtrl.cpp)

**Implementation:**
- **Botan bcrypt** with work factor 10
- Passwords hashed before storage
- Uses `Botan::generate_bcrypt()` and `Botan::check_bcrypt()`

**Code (Lines 84-87):**
```cpp
// Hash password with bcrypt before storing (work factor 10)
Botan::AutoSeeded_RNG rng;
std::string bcryptHash = Botan::generate_bcrypt(password, rng, 10);
```

**Risk Level:** 🟢 LOW - Industry-standard implementation

---

### 2.2 Key Exchange ✅ EXCELLENT

**File:** [ToClient.cpp](source/src/gateserver/ToClient.cpp#L500-L535)

**Implementation:**
- RSA 3072-bit key generated on server startup
- Client receives server public key via `CMD_MC_RSA_HANDSHAKE_1`
- Client sends its public key, server responds with RSA-encrypted AES key
- AES-256 used for session encryption
- OAEP padding with SHA-256

**Code (Lines 528-538):**
```cpp
Botan::PK_Encryptor_EME enc(*l_ply->m_clientPublicKey, rng, "OAEP(SHA-256)");
std::vector<uint8_t> aes_encrypted = enc.encrypt((uint8_t*)l_ply->m_AESKey, AES_KEY_LENGTH, rng);
std::vector<uint8_t> iv_encrypted = enc.encrypt((uint8_t*)l_ply->m_IV, AES_IV_LENGTH, rng);
```

**Risk Level:** 🟢 LOW - Modern cryptographic standards

---

### 2.3 Multi-Login Prevention ✅ GOOD

**File:** [GroupServerAppServ.cpp](source/src/groupserver/GroupServerAppServ.cpp#L956-L973)

**Implementation:**
- `m_LoginList` tracks active sessions
- Duplicate login attempts trigger disconnect of existing session
- Anti-dupe measures clear character data on login

**Code (Lines 971-978):**
```cpp
// Remove characters on login. Also a bonus anti-dupe.
for (short i = 0; i < l_ply->m_chanum; i++) {
    char luaCmd[64];
    sprintf(luaCmd, "ClearOnlineChars(%d)", l_ply->m_chaid[i]);
    // ...
}
```

**Risk Level:** 🟢 LOW

---

## 3. Database Security

### 3.1 SQL Injection Vulnerabilities 🔴 CRITICAL

Multiple locations use unsanitized user input in SQL queries via `_get_row()` filter parameters.

#### Vulnerability #1: Account Name Lookup

**File:** [DBConnect.cpp](source/src/groupserver/DBConnect.cpp#L108-L111)

```cpp
int TBLAccounts::FetchRowByActName(const char szAccount[]) {
    char filter[200];
    _snprintf_s(filter, sizeof(filter), _TRUNCATE, "act_name='%s'", szAccount);
    if (_get_row(m_buf, 7, param, filter, &l_retrow)) {
```

**Exploitation:**
```
Username: admin' OR '1'='1' --
Results in: WHERE act_name='admin' OR '1'='1' --'
```

**Impact:** Authentication bypass, data exfiltration

---

#### Vulnerability #2: Character Name Lookup

**File:** [DBConnect.cpp](source/src/groupserver/DBConnect.cpp#L167-L170)

```cpp
int TBLCharacters::FetchRowByChaName(const char* cha_name) {
    char filter[200];
    _snprintf_s(filter, sizeof(filter), _TRUNCATE, "cha_name='%s'", cha_name);
```

**Exploitation:**
```
Character: test'; DROP TABLE character; --
```

**Impact:** Character data manipulation, potential table destruction

---

#### Vulnerability #3: GameServer Character Verification

**File:** [GameDB.cpp](source/src/gameserver/GameDB.cpp#L19-L27)

```cpp
BOOL CTableCha::VerifyName(const char* pszName) {
    char filter[80];
    _snprintf_s(filter, sizeof(filter), _TRUNCATE, "cha_name='%s'", pszName);
    bool ret = _get_row(buf, 1, param, filter);
```

**Impact:** Character creation bypass, data theft

---

#### Vulnerability #4: Guild Name Lookup

**File:** [DBConnect.cpp](source/src/groupserver/DBConnect.cpp#L730)

```cpp
_snprintf_s(filter, sizeof(filter), _TRUNCATE, "guild_name='%s'", guild_name);
```

**Impact:** Guild system manipulation

---

### 3.2 Recommended Fix

**Option A: Input Sanitization (Quick Fix)**
```cpp
std::string SanitizeSqlInput(const char* input) {
    std::string result;
    for (const char* p = input; *p; ++p) {
        if (*p == '\'') result += "''";  // Escape single quotes
        else result += *p;
    }
    return result;
}
```

**Option B: Parameterized Queries (Proper Fix)**

Convert all `_get_row()` calls to use stored procedures like the existing modern patterns:

```cpp
// Good pattern already in codebase (DBConnect.cpp Line 88-89):
const auto ret = stored_procedure("{CALL dbo.AccountSaveInsert(?, ?)}", 
    "dbo", "AccountSaveInsert", 2, act_name, cha_ids);
```

---

### 3.3 SQL Injection in InsertUser ⚠️ PARTIALLY FIXED

**File:** [DataBaseCtrl.cpp](source/src/accountserver/DataBaseCtrl.cpp#L97-L103)

The bcrypt hash is safe (no user-controlled SQL-special chars), but username/email are vulnerable:

```cpp
_snprintf_s(sqlBuf, sizeof(sqlBuf), _TRUNCATE, 
    "INSERT INTO account_login ... VALUES ('%s', '%s', '%s', ...)",
    username.c_str(), bcryptHash.c_str(), email.c_str());
```

**Note:** If username/email are validated before reaching this function, the risk is reduced. However, the code comment indicates stored procedures should be used:
```cpp
// Use modern stored procedure with parameterized query (SQL injection protected)
```

**Risk Level:** 🔴 CRITICAL - Requires immediate remediation

---

## 4. Input Validation

### 4.1 Text Filtering System ✅ PARTIAL

**Files:** 
- [GroupServerAppInit.cpp](source/src/groupserver/GroupServerAppInit.cpp#L113-L115)
- [GroupServerAppServ.cpp](source/src/groupserver/GroupServerAppServ.cpp#L1345)

**Implementation:**
- `CTextFilter` class loads banned words from `ChaNameFilter.txt`
- Applied to character names, guild names, mottos
- Uses `IsLegalText()` check

**Code:**
```cpp
// GroupServerAppInit.cpp
CTextFilter::LoadFile("ChaNameFilter.txt");

// GroupServerAppServ.cpp
if (!CTextFilter::IsLegalText(CTextFilter::NAME_TABLE, l_chaname)) {
    // Reject
}
```

**Gap:** Filter is applied server-side but SQL injection through filter bypass is still possible.

**Risk Level:** 🟡 MEDIUM

---

### 4.2 Chat Message Handling

**File:** [CharacterPrl.cpp](source/src/gameserver/CharacterPrl.cpp#L296-L350)

**Implementation:**
- Rate limiting via `_dwLastSayTick` (configurable interval)
- GM commands start with `&` and require `GetGMLev()` check
- Chat content passed to Lua `HandleChat()` for custom validation

**Code (Lines 296-310):**
```cpp
case CMD_CM_SAY: {
    DWORD dwNowTick = GetTickCount();
    if (dwNowTick - _dwLastSayTick < (DWORD)g_Config.m_lSayInterval) {
        SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00001));
        break;
    }
    _dwLastSayTick = dwNowTick;
    // ...
    if (*l_content == '&') { // GM command
        Char chGMLv = GetPlayer()->GetGMLev();
        if (chGMLv == 0 || chGMLv > 150)
            SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00002));
        else
            DoCommand(l_content + 1, l_retlen - 1);
    }
}
```

**Risk Level:** 🟢 LOW - Proper validation

---

### 4.3 Character Name Validation

**Inconsistent application:** Some paths validate names, others bypass.

**Validated paths:**
- [GroupServerAppServ.cpp](source/src/groupserver/GroupServerAppServ.cpp#L1345): `CTextFilter::IsLegalText()`

**Unvalidated paths (SQL injection risk):**
- [GameDB.cpp](source/src/gameserver/GameDB.cpp#L22): Direct to SQL filter

**Risk Level:** 🟡 MEDIUM

---

## 5. Game Exploit Prevention

### 5.1 Trade System Validation ✅ GOOD

**File:** [CharTrade.cpp](source/src/gameserver/CharTrade.cpp)

**Validations:**
- Both parties must confirm (`IsTradeLocked()`)
- Money amount checked against character balance
- IMP (premium currency) limit: 2,000,000 max
- Item tradability flags checked
- Bag space verification before completion
- Boat trading has special ownership validation

**Code (Lines 1100-1150):**
```cpp
// Validate money
if (money > pCha->GetMoney()) {
    return ERR_TRADE_MONEY_INVALID;
}
// Validate IMP limit
if (imp > 2000000) {
    return ERR_TRADE_IMP_LIMIT;
}
// Verify bag space
if (!pCha->GetKitbag().HasSpace(itemCount)) {
    return ERR_TRADE_NO_SPACE;
}
```

**Potential Race Condition:**
Trade validation and execution are not atomic. If a player spends money between validation and execution, the trade could complete with insufficient funds.

**Recommended Fix:**
```cpp
// Use database transaction
BEGIN TRANSACTION
    -- Validate + Execute atomically
COMMIT
```

**Risk Level:** 🟡 MEDIUM

---

### 5.2 Anti-Bot System (CheatX) ✅ GOOD

**File:** [Character.cpp](source/src/gameserver/Character.cpp#L6500-L6600)

**Implementation:**
- Picture-based CAPTCHA challenges
- Random interval triggering
- Configurable difficulty

**Code:**
```cpp
void CCharacter::InitCheatX() {
    // Initialize captcha system with picture challenges
    // Sends verification at random intervals
}
```

**Risk Level:** 🟢 LOW

---

### 5.3 Item Duplication Prevention

**File:** [GroupServerAppServ.cpp](source/src/groupserver/GroupServerAppServ.cpp#L971-978)

**Implementation:**
- `ClearOnlineChars()` called on login to remove stale character data
- Multi-login prevention clears previous session

**Comment in code (Line 972):**
```cpp
// Remove characters on login. Also a bonus anti-dupe.
```

**Potential Issues:**
1. Network race conditions during trade/transfer
2. Server crash during item operations could leave inconsistent state

**Risk Level:** 🟡 MEDIUM

---

### 5.4 Movement/Speed Hack Prevention

**File:** [MoveAble.cpp](source/src/gameserver/MoveAble.cpp#L650-800)

**Implementation:**
- Server calculates expected movement time based on `ATTR_MSPD`
- Validates position updates against time elapsed
- Logs excessive movement times

**Code (Lines 755-758):**
```cpp
if (dwMoveTime + dwEyeMoveTime >= 60)
    LG("map_time", "roll[%s] move cost time too long, time = %u\n", 
       GetLogName(), dwMoveTime + dwEyeMoveTime);
```

**Gap:** Logging only, no active rejection of invalid movement.

**Recommended Fix:**
```cpp
if (CalculatedDistance > MaxPossibleDistance * 1.1f) {
    // Reject and rollback position
    SetPos(previousValidPosition);
    return false;
}
```

**Risk Level:** 🟡 MEDIUM

---

## 6. Encryption

### 6.1 Modern Encryption ✅ EXCELLENT

**Files:**
- [ToClient.cpp](source/src/gateserver/ToClient.cpp#L56-60) - RSA key generation
- [ToClient.cpp](source/src/gateserver/ToClient.cpp#L520-545) - Key exchange

**Implementation:**
- RSA 3072-bit for key exchange (generated fresh on server start)
- AES-256 for session encryption
- OAEP padding with SHA-256
- Botan library (well-audited cryptographic library)

**Risk Level:** 🟢 LOW

---

### 6.2 Legacy DES Encryption ⚠️ DEPRECATED

**File:** [enclib.cpp](source/src/enclib/enclib.cpp)

**Issues:**
1. **Static 8-byte key:** `__byte g_key[8] = {0};`
2. **Weak DES algorithm:** 56-bit effective key strength
3. **XOR obfuscation:** Trivial to reverse
4. **Predictable noise:** Uses `rand()` seeded with `time()`

**Code (Lines 50-75):**
```cpp
int Encrypt(__byte* buf, int len, const __byte* pwd, int plen) {
    // Generate 4 noise bytes with rand()
    tmp[0] = (__byte)rand() & 0xFF;
    // ... XOR obfuscation
    for (int i = 4; i < apd;) {
        ptr[i] = ptr[0] ^ ptr[i];  // Weak XOR
    }
    deskey((__byte*)g_key, EN0);  // Static key!
    // DES encryption
}
```

**Usage:** Check if this is still used for any live functionality. If only for legacy data migration, risk is reduced.

**Recommended Actions:**
1. Search codebase for `Encrypt()`/`Decrypt()` calls from enclib
2. Migrate any data encrypted with DES to modern AES
3. Remove or deprecate enclib.cpp

**Risk Level:** 🟡 MEDIUM (if still in active use)

---

## 7. Summary of Recommendations

### Critical (Fix Immediately)

| Issue | File | Line | Fix |
|-------|------|------|-----|
| SQL Injection in FetchRowByActName | DBConnect.cpp | 110 | Use stored procedure |
| SQL Injection in FetchRowByChaName | DBConnect.cpp | 167 | Use stored procedure |
| SQL Injection in VerifyName | GameDB.cpp | 22 | Use stored procedure |
| SQL Injection in InsertUser | DataBaseCtrl.cpp | 97 | Use stored procedure |

### High Priority

| Issue | File | Recommendation |
|-------|------|----------------|
| Movement validation | MoveAble.cpp | Add active rejection, not just logging |
| Trade race conditions | CharTrade.cpp | Add database transaction wrapping |
| Legacy DES still in codebase | enclib.cpp | Audit usage and deprecate |

### Medium Priority

| Issue | File | Recommendation |
|-------|------|----------------|
| Inconsistent name validation | Multiple | Centralize validation before DB calls |
| Item dupe on server crash | Multiple | Add transaction logging/recovery |

---

## 8. Positive Security Findings

The codebase demonstrates **significant security investment**:

1. ✅ **Modern cryptography** - RSA 3072-bit, AES-256, bcrypt
2. ✅ **Anti-DDoS protection** - Configurable, escalating bans
3. ✅ **Rate limiting** - Chat, login, connection attempts
4. ✅ **Multi-login prevention** - Session tracking with cleanup
5. ✅ **Text filtering** - `CTextFilter` for names/chat
6. ✅ **Anti-bot system** - CheatX CAPTCHA
7. ✅ **Trade validation** - Money, items, bag space checks
8. ✅ **Stored procedures** - Modern DB patterns exist (just not universally applied)

---

## Appendix A: Files Reviewed

| File | Purpose | Security Notes |
|------|---------|----------------|
| `ToClient.cpp` | GateServer client handler | Anti-DDoS, RSA handshake |
| `DataBaseCtrl.cpp` | AccountServer DB | bcrypt, SQL (mixed) |
| `DBConnect.cpp` | GroupServer DB | SQL injection risk |
| `GameDB.cpp` | GameServer DB | SQL injection risk |
| `CharTrade.cpp` | Trade system | Good validation, race risk |
| `CharacterPrl.cpp` | Command handling | GM checks, rate limiting |
| `GroupServerAppServ.cpp` | Session management | Multi-login prevention |
| `MoveAble.cpp` | Movement system | Validation exists, needs rejection |
| `enclib.cpp` | Legacy encryption | Deprecated, weak |
| `Packet.cpp` | Packet parsing | Buffer checks in place |

---

## Appendix B: SQL Injection Fix Template

Replace all `_get_row()` with filter parameters with stored procedures:

**Before (Vulnerable):**
```cpp
char filter[200];
_snprintf_s(filter, sizeof(filter), _TRUNCATE, "cha_name='%s'", cha_name);
_get_row(m_buf, 3, param, filter, &l_retrow);
```

**After (Secure):**
```cpp
const auto ret = stored_procedure("{CALL dbo.GetCharacterByName(?)}", 
    "dbo", "GetCharacterByName", 1, cha_name);
```

---

*End of Security Audit Report*
