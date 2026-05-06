// OfflineStall.h - Database-Persisted Offline Stall System
// PKODev Project - January 2026
//=============================================================================
// This system allows players to leave their stalls running after disconnect.
// Stalls are saved to the database and restored as virtual NPCs on server startup.
// Unlike the original corsairs-online implementation, stalls persist across
// server restarts and don't consume player memory slots while offline.
//=============================================================================

#pragma once

#ifndef _OFFLINE_STALL_H_
#define _OFFLINE_STALL_H_

#include "CharStall.h"
#include "Character.h"
#include <map>
#include <set>
#include <vector>
#include <string>
#include <mutex>

// Maximum items per stall (must match ROLE_MAXNUM_STALL_GOODS)
#define OFFLINE_STALL_MAX_ITEMS 20
#define OFFLINE_STALL_MAX_KITBAG 48

// Tracks a sold item for kitbag cleanup when owner logs back in
struct SSoldItemInfo {
    BYTE  byKitbagIndex;    // Original kitbag slot
    BYTE  bySoldCount;      // How many were sold from this slot
    bool  bFullySold;       // True = entire stack sold, false = partial
};

// Serialized stall item structure for database storage
// Contains the FULL item data including all attributes, gems, forge level, etc.
struct SOfflineStallItem {
    BYTE  byGrid;       // Grid position in stall display (0-19)
    BYTE  byCount;      // Quantity being sold (may be less than item stack)
    __int64 llMoney;    // Price per item (changed from DWORD for 100B max)
    BYTE  byKitbagIndex;// Original kitbag index (for reference)
    BYTE  byOrigCount;  // Original count when stall was created
    
    // Full item data (copied from kitbag at stall creation)
    // This preserves all attributes: forge level, gems, durability, etc.
    SItemGrid itemGrid;
};

// Offline stall data loaded from database
// Equipment appearance data for offline stall NPC rendering
// Stores only the fields needed for visual appearance (item ID, forge level, forge param)
struct SOfflineStallEquipLook {
    short sID;          // Item ID (0 = empty slot)
    char  chForgeLv;    // Forge level (for glow effects)
    long  lForgeParam;  // Forge parameter (for visual effects)
    DWORD dwDBID;       // Database ID (for client lookup)
    short sNeedLv;      // Required level (sent in look packets)
    short sNum;         // Item count
    short sEndure0;     // Current endurance
    short sEndure1;     // Max endurance
    short sEnergy0;     // Current energy
    short sEnergy1;     // Max energy
    bool  bValid;       // Is item valid
    bool  bItemTradable;// Is item tradable
    long  expiration;   // Item expiration
    long  lInstID;      // Instance ID (DBParam INST_ID)
    short sInstAttr[5][2]; // Instance attributes (defITEM_INSTANCE_ATTR_NUM = 5)
};

struct SOfflineStallInfo {
    DWORD dwStallID;            // Database stall_id
    DWORD dwChaID;              // Character database ID
    DWORD dwActID;              // Account ID
    char  szChaName[50];        // Character name
    char  szStallTitle[64];     // Stall shop title
    short sLookFace;            // Character face appearance
    short sLookHair;            // Character hair appearance
    short sJob;                 // Character job/class
    BYTE  byStallLevel;         // Stall skill level (determines booth appearance)
    char  szMapName[64];        // Map name
    int   nPosX;                // X position
    int   nPosY;                // Y position
    short sAngle;               // Facing direction (0-360)
    time_t tCreated;            // Creation time
    time_t tExpire;             // Expiration time
    BYTE  byItemCount;          // Number of items in stall
    __int64 llPendingGold;      // Gold accumulated from sales (running total)
    __int64 llLastSaleGold;     // Gold from most recent sale (delta for DB update)
    
    // Items
    SOfflineStallItem items[OFFLINE_STALL_MAX_ITEMS];
    
    // Sold items tracking (for kitbag cleanup when owner logs back in)
    BYTE  bySoldCount;          // Number of sold item records
    SSoldItemInfo soldItems[OFFLINE_STALL_MAX_ITEMS]; // Sold item records
    
    // Kitbag snapshot for item DB IDs
    DWORD dwKitbagDBIDs[OFFLINE_STALL_MAX_KITBAG]; // Kitbag item database IDs
    
    // Equipment appearance (for rendering the offline stall NPC with the player's gear)
    SOfflineStallEquipLook equipLook[34]; // enumEQUIP_NUM = 34
    
    // Runtime state (not persisted)
    CCharacter* pVirtualNPC;    // Virtual NPC representing this stall
    DWORD dwWorldID;            // World ID assigned to virtual NPC
    bool  bActive;              // Is stall currently active in world
};

// Virtual NPC for offline stall
class COfflineStallNPC : public CCharacter {
public:
    COfflineStallNPC();
    virtual ~COfflineStallNPC();
    
    void Initialize(SOfflineStallInfo* pStallInfo);
    void Cleanup();
    
    // Mark this as an offline stall NPC
    virtual bool IsOfflineStallNPC() const { return true; }
    
    // Override Run to skip normal character processing (prevents server hang)
    virtual void Run(dbc::uLong ulCurTick) override { /* Do nothing - static stall NPC */ }
    
    // Override GetStallName to return stall title for packet writing
    virtual dbc::cChar* GetStallName() override { return m_pStallInfo ? m_pStallInfo->szStallTitle : ""; }
    
    // Get stall info
    SOfflineStallInfo* GetOfflineStallInfo() { return m_pStallInfo; }
    DWORD GetOwnerChaID() const { return m_pStallInfo ? m_pStallInfo->dwChaID : 0; }
    const char* GetStallTitle() const { return m_pStallInfo ? m_pStallInfo->szStallTitle : ""; }
    
    // Send stall data to a viewing player
    void SendStallDataTo(CCharacter& viewer);
    
    // Check if stall has expired
    bool IsExpired() const;
    
    // Handle purchase from this stall
    bool BuyItem(CCharacter& buyer, BYTE byIndex, BYTE byCount);
    
    // Update stall after purchase (saves to DB)
    void UpdateAfterPurchase(BYTE byIndex, BYTE byCount, __int64 llGoldEarned);
    
protected:
    SOfflineStallInfo* m_pStallInfo;
};

// Offline Stall Manager - Singleton
class COfflineStallMgr {
public:
    static COfflineStallMgr& Instance();
    
    // Initialization
    bool Initialize();
    void Shutdown();
    
    // Load all offline stalls from database (called on server startup)
    bool LoadAllStalls();
    
    // Load stalls for a specific map
    bool LoadStallsForMap(const char* szMapName);
    
    // Create an offline stall when player disconnects
    bool CreateOfflineStall(CPlayer* pPlayer, CCharacter* pStaller);
    
    // Remove offline stall when owner logs back in
    // Returns pending gold that should be given to the player
    // Optionally populates soldItems/soldCount with sold item info for kitbag cleanup
    __int64 RemoveOfflineStall(DWORD dwChaID, SSoldItemInfo* pSoldItems = nullptr, BYTE* pSoldCount = nullptr);
    
    // Check if character has an active offline stall
    bool HasOfflineStall(DWORD dwChaID);
    
    // Get offline stall info by character ID
    SOfflineStallInfo* GetStallByOwner(DWORD dwChaID);
    
    // Get offline stall NPC by world ID
    COfflineStallNPC* GetStallNPC(DWORD dwWorldID);
    
    // Handle purchase from offline stall
    bool ProcessPurchase(CCharacter& buyer, DWORD dwStallWorldID, BYTE byIndex, BYTE byCount);
    
    // Mark a stall for deferred removal (safe to call from within NPC methods)
    void MarkStallForRemoval(DWORD dwChaID);
    
    // Periodic update - check for expired stalls
    void Update(DWORD dwCurTime);
    
    // Get stall count
    size_t GetStallCount() const { return m_mapStalls.size(); }
    
    // Serialize stall items from CStallData + CKitbag into SOfflineStallItem array.
    // Uses GetGridContByID(byIndex) since byIndex is a kitbag sPosID.
    // Returns the number of items successfully serialized (0 on failure).
    static BYTE SerializeStallItems(mission::CStallData* pStallData, CKitbag& kitbag,
                                    SOfflineStallItem* pOutItems);
    
    // Deserialize stall items from database
    static bool DeserializeStallItems(const BYTE* pItemData, size_t itemDataSize,
                                      BYTE itemCount, SOfflineStallInfo* pOutInfo);
    
    // Cleanup for a returning player: removes sold items from kitbag, credits gold,
    // sends system notice. Call during login when HasOfflineStall() is true.
    // Returns true if cleanup was performed.
    bool CleanupForReturningPlayer(CCharacter* pMainCha, DWORD dwChaId);
    
    // Add a loaded stall to the manager (called from DB load)
    bool AddLoadedStall(SOfflineStallInfo* pInfo);
    
    // Save stall update to database
    bool SaveStallUpdate(SOfflineStallInfo* pInfo);

private:
    COfflineStallMgr();
    ~COfflineStallMgr();
    
    // Prevent copying
    COfflineStallMgr(const COfflineStallMgr&) = delete;
    COfflineStallMgr& operator=(const COfflineStallMgr&) = delete;
    
    // Create virtual NPC for a stall
    bool CreateVirtualNPC(SOfflineStallInfo* pInfo);
    
    // Remove virtual NPC
    void RemoveVirtualNPC(SOfflineStallInfo* pInfo);
    
    // Cleanup expired stalls
    void CleanupExpiredStalls();
    
    // Generate unique world ID for virtual NPCs
    DWORD GenerateWorldID();
    
private:
    // Map of stall ID -> stall info
    std::map<DWORD, SOfflineStallInfo*> m_mapStalls;
    
    // Map of character ID -> stall ID (for quick lookup)
    std::map<DWORD, DWORD> m_mapOwnerToStall;
    
    // Map of world ID -> stall NPC
    std::map<DWORD, COfflineStallNPC*> m_mapWorldIDToNPC;
    
    // Set of character IDs pending deferred removal
    std::set<DWORD> m_setDeferredRemovals;
    
    // Mutex for thread-safe access
    std::mutex m_mutex;
    
    // World ID counter for virtual NPCs
    DWORD m_dwNextWorldID;
    
    // Last cleanup time
    DWORD m_dwLastCleanupTime;
    
    // Cleanup interval (check every 5 minutes)
    static const DWORD CLEANUP_INTERVAL = 5 * 60 * 1000;
    
    // Is initialized
    bool m_bInitialized;
};

// Global accessor
#define g_OfflineStallMgr COfflineStallMgr::Instance()

#endif // _OFFLINE_STALL_H_
