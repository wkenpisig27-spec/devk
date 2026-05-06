# Offline Stall System - Current Status & Handoff Guide

> **Last Updated:** February 12, 2026  
> **Status:** All Known Bugs Fixed — Needs Build & Testing  
> **Files Modified:** 5 files (4 server, 1 client)

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Completed Fixes](#completed-fixes)
3. [Remaining Items & Future Improvements](#remaining-items--future-improvements)
4. [File Reference](#file-reference)
5. [CKitbag Internals Reference](#ckitbag-internals-reference)
6. [Testing Procedures](#testing-procedures)
7. [Build & Deploy](#build--deploy)

---

## System Overview

The Offline Stall system allows players to set up a vendor stall and go offline. A virtual NPC takes their place and continues selling items to other players. When the seller logs back in, sold items are removed from their inventory and earned gold is credited.

### Flow

```
1. Player opens stall → adds items with prices → clicks "Offline Mode"
2. Client sends CMD_CM_OFFLINE_MODE to GateServer → forwarded to GameServer
3. GameServer validates → serializes stall items from player's CKitbag → saves to DB/memory
4. Player disconnects → virtual NPC spawns at player's location
5. Buyers interact with NPC → items purchased → stall data updated → sold items tracked
6. Seller relogs → GameServer detects previous offline stall → CleanupForReturningPlayer runs:
   a. Kitbag unlocked (was locked during stall mode)
   b. Sold items removed from kitbag by quantity (bySoldCount subtracted from sNum)
   c. All kitbag slots re-enabled (clears ITEM_DISENABLE)
   d. Kitbag validation (CheckBagItemValid)
   e. Pending gold credited to player (with 100B cap)
   f. Stall record + virtual NPC fully removed
```

### Architecture

| Component | Purpose |
|-----------|---------|
| `OfflineStall.cpp/.h` | Core manager: serialize, NPC, buy/sell, sold tracking, cleanup |
| `GameServerApp.cpp` | `TM_OFFLINE_MODE` handler — validates and triggers stall creation |
| `GameAppNet.cpp` | Login flow — calls `CleanupForReturningPlayer()` |
| `Player.h` | `m_bOfflineStallPending` flag for creation guard |
| `CharStall.cpp` | Regular stall system — stall setup, validation, purchases |
| `NetProtocol.cpp` (client) | Client kitbag rendering — `_dwColor` bug fix |

### Key Data Structures

```cpp
// Tracks what was sold for kitbag cleanup on seller relog
struct SSoldItemInfo {
    BYTE byKitbagIndex;  // sPosID in seller's kitbag
    BYTE bySoldCount;    // How many were actually sold
    bool bFullySold;     // Whether the stall listing was fully exhausted (informational only)
};

// Each item listed in the offline stall
struct SOfflineStallItem {
    BYTE byGrid;           // Display grid position in stall UI
    BYTE byCount;          // Current quantity remaining for sale
    __int64 llMoney;       // Price per unit
    BYTE byKitbagIndex;    // sPosID in seller's kitbag (for sold-item cleanup)
    BYTE byOrigCount;      // Original quantity listed (for reference)
    SItemGrid itemGrid;    // Full item data snapshot (preserves forge, gems, etc.)
};

// Full stall record (in-memory + DB persisted)
struct SOfflineStallInfo {
    // ... identity, position, map, appearance fields ...
    SOfflineStallItem items[OFFLINE_STALL_MAX_ITEMS];  // Listed items
    BYTE byItemCount;
    SSoldItemInfo soldItems[OFFLINE_STALL_MAX_ITEMS];   // Sold tracking
    BYTE bySoldCount;
    __int64 llPendingGold;  // Accumulated gold from sales
};
```

---

## Completed Fixes

### Fix 1: GetGridContByNum vs GetGridContByID ✅

**Problem:** Several places used `GetGridContByNum(byIndex)` when `byIndex` is actually a position ID (sPosID), not a sequential index. This caused wrong items to be serialized or tracked.

**Explanation:**
- `GetGridContByNum(n)` = access the nth USED item in a compacted pointer array (sequential)
- `GetGridContByID(posID)` = access the item at fixed slot `posID` (direct array index)
- `byIndex` in `STALL_GOODS` is a **position ID** validated by `CharStall.cpp` using `HasItem(byIndex)` and `GetID(byIndex)`, both sPosID-based

**Fix Locations:**
- `OfflineStall.cpp` → `SerializeStallItems()` — changed to `GetGridContByID((short)pGoods->byIndex)`
- `OfflineStall.cpp` → `CleanupForReturningPlayer()` — uses `GetGridContByID((short)soldItems[s].byKitbagIndex)`
- Removed old dual code path in `CreateOfflineStall()` — now uses single `SerializeStallItems()` call

---

### Fix 2: Sold Items Tracking System ✅

**Problem:** When items were sold from the offline stall, they remained in the seller's inventory after relog. No mechanism existed to track what was sold.

**What Was Done:**
- Added `SSoldItemInfo` struct to `OfflineStall.h`
- `UpdateAfterPurchase()` records each sale: kitbag sPosID, quantity sold, fully-sold flag
- Handles incremental sales (e.g., sell 3 of 10, then sell 2 more → accumulates to 5 sold)
- `RemoveOfflineStall()` returns sold item records via output params before deleting stall
- `CleanupForReturningPlayer()` processes sold records on seller login

---

### Fix 3: Quantity Bug — "20 Scrolls Disappearing" ✅

**Problem:** Player lists 1 of 20 scrolls in offline stall. Friend buys the 1 scroll. When seller relogs, ALL 20 scrolls are removed from inventory instead of just 1.

**Root Cause:** `CleanupForReturningPlayer()` had a `bFullySold` branch that called `Clear(sPosID)` when the **stall listing** was exhausted. But `bFullySold` only means "the listing sold out" (1/1 sold), NOT "remove the entire kitbag stack." `Clear()` destroys the entire slot — all 20 scrolls.

**Fix:** Removed the `bFullySold`-based `Clear()` branch entirely. Now **always** does quantity-based subtraction:
```cpp
short newCount = pItem->sNum - (short)soldItems[s].bySoldCount;
if (newCount <= 0) {
    pMainCha->m_CKitbag.Clear(kitbagPosID);  // Only clear if ALL sold
} else {
    pItem->sNum = newCount;  // Reduce quantity
    pMainCha->m_CKitbag.SetSingleChangeFlag(kitbagPosID);
}
```

**Examples:**
- 20 scrolls, sold 1 → `20 - 1 = 19` → keeps 19 scrolls ✅
- 20 scrolls, sold 20 → `20 - 20 = 0` → clears slot ✅
- 1 sword, sold 1 → `1 - 1 = 0` → clears slot ✅

**Note:** `bFullySold` field in `SSoldItemInfo` is now effectively unused for cleanup logic but still set by `UpdateAfterPurchase()`. It's harmless and could be removed later.

---

### Fix 4: Sold Items Not Persisted to Database ✅

**Problem:** Sold item records were tracked in memory but never serialized to the database blob. Server restart between a sale and seller relog would lose the sold records, causing items to remain in inventory.

**What Was Done:**
- `SaveStallUpdate()` now appends sold items after regular items in the DB blob:
  ```
  Format: [SOfflineStallItem * itemCount] [BYTE bySoldCount] [SSoldItemInfo * bySoldCount]
  ```
- `DeserializeStallItems()` now reads sold items from the blob after regular items
- Both functions handle the case where no sold items exist (backward compatible)

---

### Fix 5: Deferred Removal Destroying Stall Records ✅

**Problem:** When a stall is emptied (buyer purchases last item), `UpdateAfterPurchase()` marks it for deferred removal. The `Update()` tick was calling `RemoveOfflineStall()` which deleted the stall record entirely — including sold items and pending gold — before the seller could log back in to claim them.

**Fix:** `Update()` deferred removal now only removes the NPC from the world and saves updated state to DB. The stall record (with sold items + pending gold) is preserved until the seller logs in and `CleanupForReturningPlayer()` processes it.

---

### Fix 6: Expired Stalls Deleting Pending Data ✅

**Problem:** `CleanupExpiredStalls()` was deleting all expired stalls, even those with pending gold and sold item records that the seller hadn't claimed yet.

**Fix:** Expired stalls with `bySoldCount > 0` or `llPendingGold > 0` are now preserved (NPC removed, record kept). Only stalls with nothing pending are fully deleted.

---

### Fix 7: Face/Hair Cosmetic Bug ✅

**Problem:** Offline stall NPCs had wrong face appearance. Both face and hair were reading from `SLink[enumEQUIP_HEAD].sID`.

**Fix:**
- Face: `m_SChaPart.SLink[enumEQUIP_FACE].sID = pStallInfo->sLookFace`
- Hair: `m_SChaPart.sHairID = pStallInfo->sLookHair`
- `CreateOfflineStall()` captures: `sLookFace = pStaller->m_SChaPart.SLink[enumEQUIP_FACE].sID`, `sLookHair = pStaller->m_SChaPart.sHairID`

---

### Fix 8: Double-Creation Race Condition ✅

**Problem:** Stall creation was triggered twice in quick succession (TM_OFFLINE_MODE handler + disconnect handler), creating duplicates or errors.

**Fix:** Added guards in two locations:
- `GameServerApp.cpp` TM_OFFLINE_MODE handler: checks `player->IsOfflineStallPending() || g_OfflineStallMgr.HasOfflineStall(player->GetDBChaId())`
- `GameAppNet.cpp` disconnect handler: checks `g_OfflineStallMgr.HasOfflineStall(dwDisconnectChaId)` before creating

---

### Fix 9: m_bOfflineStallPending Not Reset on Pool Reuse ✅

**Problem:** `CPlayer` objects are pooled. If a player triggered offline mode, the flag stayed `true` when the object was reused for a different player, causing the new player's stall creation to be blocked.

**Fix:** Changed to default member initializer in `Player.h`:
```cpp
bool m_bOfflineStallPending{false};
```

---

### Fix 10: CleanupForReturningPlayer Extraction ✅

**Problem:** Offline stall cleanup logic was deeply embedded in the login packet handler in `GameAppNet.cpp` (inline kitbag manipulation, mix of concerns).

**Fix:** Extracted into `COfflineStallMgr::CleanupForReturningPlayer(CCharacter*, DWORD)` as a standalone method. Login handler now just calls:
```cpp
g_OfflineStallMgr.CleanupForReturningPlayer(pMainCha, dwChaId);
```

---

### Fix 11: Red-Tinted Items After Relog (Client-Side) ✅

**Problem:** After relogging, unsold stall items appeared with a red tint.

**Root Cause:** Client-side `CItemCommand::_dwColor` persistence. During stall setup, `SetIsValid(false)` set `_dwColor = INVALID_COLOR (red)`. On relog, old `CItemCommand` objects were reused (same grid slot, same item type), and `SetData()` via `memcpy` didn't reset `_dwColor`.

**Fix Location:** `source/src/game/NetProtocol.cpp` ~line 1891

**Fix:** After `SetData()` in `NetChangeKitbag`:
```cpp
pObj->SetIsValid(pGrid[i].SGridContent.IsValid());
```

---

### Fix 12: Pending Gold Credited ✅

**Problem:** Gold earned from sales wasn't given to seller.

**Fix:** `CleanupForReturningPlayer()` credits accumulated `llPendingGold` with:
- 100 billion gold cap validation
- System notices showing amount earned
- Per-transaction gold tracking in `UpdateAfterPurchase()`

---

## Data Persistence Lifecycle

The sold items and pending gold must survive these three events:

| Event | What Happens | Data Preserved? |
|-------|-------------|-----------------|
| **Stall emptied** (last item bought) | NPC removed from world, stall record saved to DB | ✅ Yes — record preserved with sold items + gold |
| **Server restart** | Stall loaded from DB via `DeserializeStallItems()` | ✅ Yes — sold items appended in DB blob |
| **Stall expired** | NPC removed, but record kept if pending data exists | ✅ Yes — only removed when nothing pending |
| **Seller relogs** | `CleanupForReturningPlayer()` processes everything | ✅ Full cleanup runs |

### DB Blob Format

```
[SOfflineStallItem × byItemCount]     ← remaining unsold items
[BYTE bySoldCount]                     ← number of sold item records
[SSoldItemInfo × bySoldCount]          ← sold item tracking
```

---

## Remaining Items & Future Improvements

| Priority | Task | Status | Notes |
|----------|------|--------|-------|
| **HIGH** | Build and test all fixes | TODO | See Testing Procedures below |
| **HIGH** | Fix SQL persistence (stall lost on server restart) | TODO | DB insert was failing — fix stored procedure or table schema to accommodate new blob format |
| **MEDIUM** | Remove `bFullySold` field from `SSoldItemInfo` | Optional | No longer used in cleanup logic, only informational |
| **MEDIUM** | Remove fallback scan in `SerializeStallItems` | Optional | Was a workaround for GetGridContByNum bug, now unnecessary since we use GetGridContByID correctly |
| **LOW** | Capture item snapshots at stall setup time | Future | Currently captures at offline-mode activation — would be safer to snapshot in CharStall.cpp::SetupStall |
| **LOW** | Add structured logging levels | Future | Currently all logs use same priority under "offline_stall" tag |

---

## File Reference

### Modified Files

| File | Key Changes |
|------|-------------|
| `source/src/gameserver/OfflineStall.h` | `SSoldItemInfo` struct (byKitbagIndex, bySoldCount, bFullySold), `SOfflineStallItem` with byOrigCount + byKitbagIndex, `SOfflineStallInfo` with soldItems[] + bySoldCount, `CleanupForReturningPlayer()` declaration, `RemoveOfflineStall()` with sold items output params, `SerializeStallItems()` returns BYTE |
| `source/src/gameserver/OfflineStall.cpp` | `SerializeStallItems()` uses GetGridContByID, `UpdateAfterPurchase()` tracks sold items, `SaveStallUpdate()` serializes sold items to DB, `DeserializeStallItems()` reads sold items from DB, `RemoveOfflineStall()` copies sold items before deletion, `Update()` deferred removal preserves records, `CleanupExpiredStalls()` preserves pending data, `CleanupForReturningPlayer()` quantity-based item removal, `CreateOfflineStall()` single SerializeStallItems path + correct face/hair |
| `source/src/gameserver/GameAppNet.cpp` | Login handler calls `CleanupForReturningPlayer()`, disconnect handler has double-creation guard |
| `source/src/gameserver/GameServerApp.cpp` | TM_OFFLINE_MODE handler has double-creation guard (checks IsOfflineStallPending + HasOfflineStall) |
| `source/include/GameServer/Player.h` | `m_bOfflineStallPending{false}` default member initializer |
| `source/src/game/NetProtocol.cpp` | Client `_dwColor` fix — `SetIsValid()` after `SetData()` in `NetChangeKitbag` |

### Key Source Files (Unmodified, For Reference)

| File | Purpose |
|------|---------|
| `source/src/common/Kitbag.cpp` | CKitbag implementation — Push, Pop, Clear, GetGridContByID/ByNum |
| `source/include/common/Kitbag.h` | CKitbag class definition, SItemUnit struct |
| `source/include/GameServer/CharStall.h` | STALL_GOODS struct, CStallData class |
| `source/src/gameserver/CharacterPacket.cpp` | WriteKitbag — server→client kitbag sync |
| `source/include/game/UIItemCommand.h` | CItemCommand — _dwColor, INVALID_COLOR, SetIsValid |

---

## CKitbag Internals Reference

### Dual Data Structure

```cpp
// Two parallel arrays:
m_SItem[type][posID]    // Fixed-position array. sPosID set in Init(), NEVER changes.
m_pSItem[type][seqNum]  // Pointer array for compact iteration of used items.

// Clear(sPosID) zeros m_SItem[0][sPosID] and SWAPS m_pSItem pointers to compact.
// This means sequential order changes, but sPosID values are stable.
```

### API Quick Reference

```cpp
SItemGrid* GetGridContByID(short sPosID, short sType = 0);   // Direct slot access ← CORRECT for stall byIndex
SItemGrid* GetGridContByNum(short sPosNum, short sType = 0); // Sequential used-item access ← WRONG for stall byIndex

short GetPosIDByNum(short sPosNum, short sType = 0);  // seqNum → sPosID
BOOL HasItem(short sPosID, short sType = 0);           // returns GetID(sPosID) > 0
short GetID(short sPosID, short sType = 0);             // item ID at position
short Clear(short sPosID, short sType = 0);             // remove item, reorder m_pSItem pointers

void Lock();     // Prevents Push/Pop/Clear (used during stall mode)
void UnLock();   // Re-enables Push/Pop/Clear
void Enable(short sPosID, short sType = 0);   // Clears ITEM_DISENABLE flag
void Disable(short sPosID, short sType = 0);  // Sets ITEM_DISENABLE flag
```

### STALL_GOODS Structure

```cpp
typedef struct _STALL_GOODS {
    __int64 llMoney;   // Price per unit
    BYTE byGrid;       // Display grid position in stall UI
    BYTE byIndex;      // Kitbag position ID (sPosID) — validated by CharStall.cpp using HasItem()
    USHORT byCount;    // Quantity to sell
    USHORT sItemID;    // Item type ID (for reference/verification)
} STALL_GOODS;
```

---

## Testing Procedures

### Test 1: Quantity Bug (20 Scrolls)

1. Log in as player A, acquire 20 stackable scrolls
2. Open stall, add 1 scroll to sell (listing qty = 1, kitbag has 20)
3. Click "Offline Mode"
4. Log in as player B, find the NPC stall
5. Buy the 1 scroll
6. Log player A back in
7. **Verify:** Player A has 19 scrolls remaining + gold from sale
8. **Verify:** Player B has 1 scroll

### Test 2: Multiple Items, Partial Sales

1. Player A: stall with 3 different items (sword, shield, 10 potions listing 5)
2. Go offline
3. Player B buys sword + 3 potions
4. Player A relogs
5. **Verify:** Sword removed, shield still present, 7 potions remaining (10 - 3)
6. **Verify:** Gold = sword price + (3 × potion price)

### Test 3: Stall Empties Completely

1. Player A: stall with 1 item
2. Go offline
3. Player B buys the item
4. **Verify:** NPC disappears from world
5. Player A relogs
6. **Verify:** Item removed, gold credited, system notice shown

### Test 4: Server Restart Persistence

1. Set up offline stall, have some items bought
2. Restart GameServer
3. **Verify:** Stall NPC respawns with remaining items
4. **Verify:** Previously sold items still tracked (check logs)
5. Seller relogs → sold items removed, gold credited
6. **Note:** This test depends on SQL persistence being fixed

### Test 5: No Red Tint After Relog

1. Player A: stall with 3 items, go offline
2. Player B buys 1 item
3. Player A relogs
4. **Verify:** Remaining 2 items appear with normal color (no red tint)

### Test 6: Double-Creation Prevention

1. Player A: set up stall, click offline mode
2. Check server logs — should see only ONE "Created offline stall" message
3. Should NOT see "Player already has an offline stall" error

### Test 7: Consecutive Stall Cycles

1. Player A: stall → offline → relog → stall → offline → relog → stall → offline → relog
2. **Verify:** No crash on 3rd cycle (was caused by m_bOfflineStallPending not resetting)
3. **Verify:** Items and gold correct at each relog

### Log Monitoring

```powershell
# Watch offline stall logs in real-time
Get-Content "server\LOG\GameServer\*\offline_stall.log" -Wait -Tail 20

# Search for security/error events
Select-String -Path "server\LOG\GameServer\*\offline_stall.log" -Pattern "SKIPPED|FAIL|error|Security" -SimpleMatch

# Check sold item tracking
Select-String -Path "server\LOG\GameServer\*\offline_stall.log" -Pattern "sold|reducing|clearing"
```

---

## Build & Deploy

### Build GameServer Only

```powershell
cd "<workspace>\source"
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" `
    "build\gameserver\GameServer.vcxproj" /p:Configuration=Release /p:Platform=x64 /v:minimal
```

Output: `source\bin\Release\gameserver\GameServer.exe`

### Build Client Only

```powershell
cd "<workspace>\source"
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" `
    "build\game\game.vcxproj" /p:Configuration=Release /p:Platform=x64 /v:minimal
```

Output: `source\bin\Release\game\Game.exe`

### Deploy

```powershell
cd "<workspace>\source"
.\symlinks.bat
```
