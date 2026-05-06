// OfflineStall.cpp - Database-Persisted Offline Stall System
// PKODev Project - January 2026
//=============================================================================

#include "stdafx.h"
#include "OfflineStall.h"
#include "GameDB.h"
#include "GameApp.h"
#include "Config.h"
#include "SubMap.h"
#include "MapRes.h"
#include "log.h"
#include "Kitbag.h"
#include "MapEntry.h"
#include "Player.h"
#include "GameAppNet.h"
#include "ItemRecord.h"
#include "CharacterRecord.h"
#include "SkillStateType.h"
#include "SkillRecord.h"
#include <ctime>

//=============================================================================
// COfflineStallNPC Implementation
//=============================================================================

COfflineStallNPC::COfflineStallNPC()
    : m_pStallInfo(nullptr) {
    // CRITICAL: Call Initially() to properly initialize ALL base Entity fields.
    // Without this, m_pCEyeshotCellNext, m_pCEyeshotCellLast, and m_pCEyeshotHost
    // contain garbage values which causes crashes in SubMap::Add() and 
    // CEyeshotCell::AddEntity() when they check/dereference these pointers.
    // The Entity constructor does NOT initialize these fields — only Initially() does,
    // and it's normally called by the entity pool allocator (CAlloc::alloc()).
    // Since we create this NPC via 'new' (outside the pool), we must call it manually.
    Initially();
    
    // Set m_lHandle to an invalid sentinel value so that if Free() is ever called
    // (e.g., by CEyeshotCell destructor), ReturnEntity() will safely no-op.
    // Type byte 0xFF doesn't match any entity pool type (CHA=0x02, ITEM=0x03, etc.)
    // so ReturnEntity() falls through all if-else branches and does nothing.
    SetHandle(0xFF000000);
    SetHoldID(-1);
    
    // Set a very high lifetime so CheckLifeTime() won't trigger Free()
    _dwLifeTime = 0xFFFFFFFF;
    
    // Disable AI processing
    m_AIType = 0;
    m_AITarget = 0;
    
    // Mark as not a player character
    m_pCPlayer = nullptr;
    
    // Set existence state to waiting (static NPC)
    SetExistState(enumEXISTS_WAITING);
}

COfflineStallNPC::~COfflineStallNPC() {
    Cleanup();
}

void COfflineStallNPC::Initialize(SOfflineStallInfo* pStallInfo) {
    if (!pStallInfo) return;
    
    m_pStallInfo = pStallInfo;
    
    // Set basic character properties
    SetName(pStallInfo->szChaName);
    SetID(pStallInfo->dwWorldID);
    
    // Set radius (standard NPC/player radius)
    SetRadius(40);
    
    // Set character category (type ID) - this is used by client for model lookup
    m_SChaPart.sTypeID = pStallInfo->sJob;
    SetCat(pStallInfo->sJob);  // m_cat must match for GetCat() to return correct value
    
    // Get character record for this type
    m_pCChaRecord = GetChaRecordInfo(pStallInfo->sJob);
    if (m_pCChaRecord) {
        // Initialize character attributes from the record
        m_CChaAttr.Init(pStallInfo->sJob, false);
    }
    
    // Set character type to PLAYER so client renders it as a player with stall
    setAttr(ATTR_CHATYPE, enumCHACTRL_PLAYER);
    
    // Set appearance based on stored look data
    m_SChaPart.sHairID = pStallInfo->sLookHair;
    
    // Restore full equipment appearance from saved data
    for (int i = 0; i < enumEQUIP_NUM; i++) {
        m_SChaPart.SLink[i].sID = pStallInfo->equipLook[i].sID;
        m_SChaPart.SLink[i].chForgeLv = pStallInfo->equipLook[i].chForgeLv;
        m_SChaPart.SLink[i].SetDBParam(enumITEMDBP_FORGE, pStallInfo->equipLook[i].lForgeParam);
        m_SChaPart.SLink[i].SetDBParam(enumITEMDBP_INST_ID, pStallInfo->equipLook[i].lInstID);
        m_SChaPart.SLink[i].dwDBID = pStallInfo->equipLook[i].dwDBID;
        m_SChaPart.SLink[i].sNeedLv = pStallInfo->equipLook[i].sNeedLv;
        m_SChaPart.SLink[i].sNum = pStallInfo->equipLook[i].sNum;
        m_SChaPart.SLink[i].sEndure[0] = pStallInfo->equipLook[i].sEndure0;
        m_SChaPart.SLink[i].sEndure[1] = pStallInfo->equipLook[i].sEndure1;
        m_SChaPart.SLink[i].sEnergy[0] = pStallInfo->equipLook[i].sEnergy0;
        m_SChaPart.SLink[i].sEnergy[1] = pStallInfo->equipLook[i].sEnergy1;
        m_SChaPart.SLink[i].bValid = pStallInfo->equipLook[i].bValid;
        m_SChaPart.SLink[i].bItemTradable = pStallInfo->equipLook[i].bItemTradable;
        m_SChaPart.SLink[i].expiration = pStallInfo->equipLook[i].expiration;
        for (int j = 0; j < defITEM_INSTANCE_ATTR_NUM; j++) {
            m_SChaPart.SLink[i].sInstAttr[j][0] = pStallInfo->equipLook[i].sInstAttr[j][0];
            m_SChaPart.SLink[i].sInstAttr[j][1] = pStallInfo->equipLook[i].sInstAttr[j][1];
        }
    }
    
    // Set icon (player portrait icon, can use 0 or a default value)
    SetIcon(1);  // Default player icon
    
    // Set position
    SetPos(Point(pStallInfo->nPosX, pStallInfo->nPosY));
    
    // Set facing direction
    SetAngle(pStallInfo->sAngle);
    
    // Set existence state to WAITING (standing idle)
    SetExistState(enumEXISTS_WAITING);
    
    // Enable eyeshot so other characters can see this NPC
    SetEyeshotAbility(true);
    m_bActiveEyeshot = true;
    
    // NOTE: Stall skill state (SSTATE_STALL) is set in CreateVirtualNPC AFTER entering the map
    
    LG("offline_stall", "Initialized offline stall NPC: %s (owner: %s, ID: %u, type: %d)\n",
       pStallInfo->szStallTitle, pStallInfo->szChaName, pStallInfo->dwChaID, pStallInfo->sJob);
}

void COfflineStallNPC::Cleanup() {
    if (m_pStallInfo) {
        m_pStallInfo->pVirtualNPC = nullptr;
        m_pStallInfo->bActive = false;
        m_pStallInfo = nullptr;
    }
}

void COfflineStallNPC::SendStallDataTo(CCharacter& viewer) {
    if (!m_pStallInfo) {
        viewer.SystemNotice("This offline stall is no longer available.");
        return;
    }
    
    LG("offline_stall", "SendStallDataTo: %s viewing %s's stall (%d items)\n",
       viewer.GetName(), m_pStallInfo->szChaName, m_pStallInfo->byItemCount);
    
    WPACKET packet = GETWPACKET();
    WRITE_CMD(packet, CMD_MC_STALL_ALLDATA);
    WRITE_LONG(packet, GetID());  // Stall owner ID (this NPC's world ID)
    
    WRITE_CHAR(packet, m_pStallInfo->byItemCount);
    WRITE_STRING(packet, m_pStallInfo->szStallTitle);
    
    for (BYTE i = 0; i < m_pStallInfo->byItemCount; i++) {
        SOfflineStallItem& item = m_pStallInfo->items[i];
        SItemGrid& itemGrid = item.itemGrid;
        
        WRITE_CHAR(packet, i);                  // Grid position
        WRITE_SHORT(packet, itemGrid.sID);      // Item ID from stored item
        WRITE_CHAR(packet, item.byCount);       // Quantity being sold
        WRITE_LONGLONG(packet, item.llMoney);   // Price (64-bit)
        
        // Get item record for type info
        CItemRecord* pItemRec = (CItemRecord*)GetItemRecordInfo(itemGrid.sID);
        if (!pItemRec) {
            LG("offline_stall", "ERROR: Item ID %d not found in item records\n", itemGrid.sID);
            viewer.SystemNotice("Error loading stall item data.");
            return;
        }
        
        WRITE_SHORT(packet, pItemRec->sType);
        
        if (pItemRec->sType == enumItemTypeBoat) {
            // Boats are not supported for offline stalls
            WRITE_CHAR(packet, 0);
        } else {
            // Write ACTUAL item instance attributes from stored item data
            WRITE_SHORT(packet, itemGrid.sEndure[0]);  // Current durability
            WRITE_SHORT(packet, itemGrid.sEndure[1]);  // Max durability
            WRITE_SHORT(packet, itemGrid.sEnergy[0]);  // Current energy
            WRITE_SHORT(packet, itemGrid.sEnergy[1]);  // Max energy
            WRITE_CHAR(packet, itemGrid.chForgeLv);    // Forge level (+27, etc.)
            WRITE_CHAR(packet, itemGrid.IsValid() ? 1 : 0);
            WRITE_CHAR(packet, itemGrid.bItemTradable ? 1 : 0);
            WRITE_LONG(packet, itemGrid.expiration);
            
            WRITE_LONG(packet, itemGrid.GetDBParam(enumITEMDBP_FORGE));    // FORGE param
            WRITE_LONG(packet, itemGrid.GetDBParam(enumITEMDBP_INST_ID));  // INST_ID
            
            // Write instance attributes (gems, etc.)
            if (itemGrid.IsInstAttrValid()) {
                WRITE_CHAR(packet, 1);  // Has instance attributes
                for (int k = 0; k < defITEM_INSTANCE_ATTR_NUM; k++) {
                    WRITE_SHORT(packet, itemGrid.sInstAttr[k][0]);  // Attribute ID
                    WRITE_SHORT(packet, itemGrid.sInstAttr[k][1]);  // Attribute value
                }
            } else {
                WRITE_CHAR(packet, 0);  // No instance attributes
            }
        }
    }
    
    viewer.ReflectINFof(&viewer, packet);
    LG("offline_stall", "Sent stall data to %s\n", viewer.GetName());
}

bool COfflineStallNPC::IsExpired() const {
    if (!m_pStallInfo) return true;
    
    time_t now = time(nullptr);
    return now >= m_pStallInfo->tExpire;
}

bool COfflineStallNPC::BuyItem(CCharacter& buyer, BYTE byIndex, BYTE byCount) {
    if (!m_pStallInfo || byIndex >= m_pStallInfo->byItemCount) {
        buyer.SystemNotice("Invalid stall item!");
        return false;
    }
    
    SOfflineStallItem& item = m_pStallInfo->items[byIndex];
    
    // Validate count
    if (byCount > item.byCount || byCount == 0) {
        buyer.SystemNotice("Invalid item count!");
        return false;
    }
    
    // Calculate total cost (64-bit for large prices)
    __int64 llTotalCost = item.llMoney * byCount;
    
    // Check buyer has enough gold
    if (buyer.getAttr(ATTR_GD) < llTotalCost) {
        buyer.SystemNotice("Not enough gold!");
        return false;
    }
    
    // Check buyer has inventory space
    if (buyer.m_CKitbag.IsFull()) {
        buyer.SystemNotice("Inventory is full!");
        return false;
    }
    
    // Deduct gold from buyer
    buyer.setAttr(ATTR_GD, buyer.getAttr(ATTR_GD) - llTotalCost);
    buyer.SynAttr(enumATTRSYN_TRADE);
    
    // Give item to buyer - copy the FULL item data including all attributes
    // This preserves forge level, gems, durability, and all other item properties
    SItemGrid itemGrid = item.itemGrid;  // Copy full item data from stored stall item
    itemGrid.sNum = byCount;             // Set count to amount being purchased
    itemGrid.dwDBID = 0;                 // Clear DBID - will get new ID when saved to buyer's inventory
    itemGrid.SetChange(true);            // Mark as changed for database persistence
    
    // Try to add to kitbag using Push()
    short sPosID = defKITBAG_DEFPUSH_POS;
    short sPushRet = buyer.m_CKitbag.Push(&itemGrid, sPosID);
    if (sPushRet != enumKBACT_SUCCESS) {
        // Failed - refund gold
        buyer.setAttr(ATTR_GD, buyer.getAttr(ATTR_GD) + llTotalCost);
        buyer.SynAttr(enumATTRSYN_TRADE);
        buyer.SystemNotice("Failed to add item to inventory!");
        return false;
    }
    
    // Update inventory display
    buyer.SynKitbagNew(enumSYN_KITBAG_TRADE);
    
    // Save buyer's gold + inventory to DB IMMEDIATELY to prevent rollback/duplication on crash.
    // Must happen BEFORE SaveStallUpdate() so that if a crash occurs between the two saves,
    // the buyer has already paid (safer failure mode than gold duplication).
    // This mirrors how player-to-player trade handles it in CharTrade.cpp.
    extern CGameDB game_db;
    game_db.SaveChaAssets(&buyer);
    
    // Store item info before update (for notification)
    short sItemID = item.itemGrid.sID;
    
    // Update stall (modifies item counts, may remove items)
    UpdateAfterPurchase(byIndex, byCount, llTotalCost);
    
    // Resend full stall data to buyer to refresh their stall UI
    // This is more reliable than trying to send incremental updates
    if (m_pStallInfo && m_pStallInfo->byItemCount > 0) {
        SendStallDataTo(buyer);
    } else {
        // Stall is now empty - close the stall window for buyer
        WPACKET closePacket = GETWPACKET();
        WRITE_CMD(closePacket, CMD_MC_STALL_CLOSE);
        WRITE_LONG(closePacket, GetID());
        buyer.ReflectINFof(&buyer, closePacket);
    }
    
    // Notify buyer of successful purchase
    CItemRecord* pItemRec = GetItemRecordInfo(sItemID);
    if (pItemRec) {
        buyer.SystemNotice("Purchased %d x %s for %u gold from offline stall.", 
                          byCount, pItemRec->szName, llTotalCost);
    }
    
    LG("offline_stall", "Purchase: %s bought %d x item %d from %s's offline stall for %u gold\n",
       buyer.GetName(), byCount, sItemID, m_pStallInfo ? m_pStallInfo->szChaName : "unknown", llTotalCost);
    
    return true;
}

void COfflineStallNPC::UpdateAfterPurchase(BYTE byIndex, BYTE byCount, __int64 llGoldEarned) {
    if (!m_pStallInfo || byIndex >= m_pStallInfo->byItemCount) return;
    
    SOfflineStallItem& item = m_pStallInfo->items[byIndex];
    
    // Track the sold item for kitbag cleanup when owner logs back in
    if (m_pStallInfo->bySoldCount < OFFLINE_STALL_MAX_ITEMS) {
        // Check if we already have an entry for this kitbag index
        bool bFound = false;
        for (BYTE s = 0; s < m_pStallInfo->bySoldCount; s++) {
            if (m_pStallInfo->soldItems[s].byKitbagIndex == item.byKitbagIndex) {
                m_pStallInfo->soldItems[s].bySoldCount += byCount;
                if (byCount >= item.byCount) {
                    m_pStallInfo->soldItems[s].bFullySold = true;
                }
                bFound = true;
                break;
            }
        }
        if (!bFound) {
            SSoldItemInfo& sold = m_pStallInfo->soldItems[m_pStallInfo->bySoldCount];
            sold.byKitbagIndex = item.byKitbagIndex;
            sold.bySoldCount = byCount;
            sold.bFullySold = (byCount >= item.byCount);
            m_pStallInfo->bySoldCount++;
        }
    }
    
    // Reduce item count
    if (byCount >= item.byCount) {
        // Item depleted - remove from stall
        // Shift remaining items down
        for (BYTE i = byIndex; i < m_pStallInfo->byItemCount - 1; i++) {
            m_pStallInfo->items[i] = m_pStallInfo->items[i + 1];
        }
        m_pStallInfo->byItemCount--;
        
        // Clear last slot
        memset(&m_pStallInfo->items[m_pStallInfo->byItemCount], 0, sizeof(SOfflineStallItem));
    } else {
        item.byCount -= byCount;
    }
    
    // Track gold earned from this sale (delta for DB update)
    m_pStallInfo->llLastSaleGold = llGoldEarned;
    
    // Add gold to pending (running total for in-memory tracking)
    m_pStallInfo->llPendingGold += llGoldEarned;
    
    // Save to database
    g_OfflineStallMgr.SaveStallUpdate(m_pStallInfo);
    
    // Check if stall is now empty - mark for deferred deletion
    // We can't delete the NPC here because we're still inside its method call!
    if (m_pStallInfo->byItemCount == 0) {
        LG("offline_stall", "Stall %s (owner: %s) is now empty - marking for deferred removal\n",
           m_pStallInfo->szStallTitle, m_pStallInfo->szChaName);
        
        // Mark stall as inactive so it will be removed in next Update() tick
        // This prevents crash from deleting NPC while still in its method call
        g_OfflineStallMgr.MarkStallForRemoval(m_pStallInfo->dwChaID);
    }
}

//=============================================================================
// COfflineStallMgr Implementation
//=============================================================================

COfflineStallMgr& COfflineStallMgr::Instance() {
    static COfflineStallMgr instance;
    return instance;
}

COfflineStallMgr::COfflineStallMgr()
    : m_dwNextWorldID(0x80000000)  // Start high to avoid conflicts with normal entities
    , m_dwLastCleanupTime(0)
    , m_bInitialized(false) {
}

COfflineStallMgr::~COfflineStallMgr() {
    Shutdown();
}

bool COfflineStallMgr::Initialize() {
    if (m_bInitialized) return true;
    
    if (!g_Config.m_bOfflineStall) {
        LG("offline_stall", "Offline stall system is disabled in config\n");
        m_bInitialized = true;
        return true;
    }
    
    LG("offline_stall", "Initializing Offline Stall Manager...\n");
    
    m_dwLastCleanupTime = GetTickCount();
    m_bInitialized = true;
    
    return true;
}

void COfflineStallMgr::Shutdown() {
    LG("offline_stall", "Shutting down Offline Stall Manager...\n");
    
    // Remove all virtual NPCs
    for (auto& pair : m_mapStalls) {
        if (pair.second) {
            RemoveVirtualNPC(pair.second);
            delete pair.second;
        }
    }
    
    m_mapStalls.clear();
    m_mapOwnerToStall.clear();
    m_mapWorldIDToNPC.clear();
    
    m_bInitialized = false;
}

bool COfflineStallMgr::LoadAllStalls() {
    if (!g_Config.m_bOfflineStall) return true;
    
    LG("offline_stall", "Loading all offline stalls from database...\n");
    
    // Use game_db to load stalls
    extern CGameDB game_db;
    
    if (!game_db.LoadOfflineStalls(nullptr, this)) {
        LG("offline_stall", "Failed to load offline stalls from database\n");
        return false;
    }
    
    LG("offline_stall", "Loaded %zu offline stalls\n", m_mapStalls.size());
    return true;
}

bool COfflineStallMgr::LoadStallsForMap(const char* szMapName) {
    if (!g_Config.m_bOfflineStall || !szMapName) return true;
    
    LG("offline_stall", "Loading offline stalls for map: %s\n", szMapName);
    
    extern CGameDB game_db;
    
    if (!game_db.LoadOfflineStalls(szMapName, this)) {
        LG("offline_stall", "Failed to load offline stalls for map %s\n", szMapName);
        return false;
    }
    
    return true;
}

bool COfflineStallMgr::CreateOfflineStall(CPlayer* pPlayer, CCharacter* pStaller) {
    if (!g_Config.m_bOfflineStall) return false;
    if (!pPlayer || !pStaller) return false;
    
    mission::CStallData* pStallData = pPlayer->GetStallData();
    if (!pStallData) {
        LG("offline_stall", "No stall data for player %s\n", pStaller->GetName());
        return false;
    }
    
    // Check if player already has an offline stall (prevents double-creation race)
    DWORD dwChaID = pPlayer->GetDBChaId();
    if (HasOfflineStall(dwChaID)) {
        LG("offline_stall", "Player %s already has an offline stall - skipping creation\n", pStaller->GetName());
        return false;
    }
    
    // Get map info
    SubMap* pMap = pStaller->GetSubMap();
    if (!pMap) {
        LG("offline_stall", "Player %s has no map\n", pStaller->GetName());
        return false;
    }
    
    // Serialize stall items using the SINGLE correct code path.
    // SerializeStallItems uses GetGridContByID (sPosID-based lookup) which matches
    // how CharStall.cpp validates byIndex during stall setup.
    SOfflineStallItem tempItems[OFFLINE_STALL_MAX_ITEMS];
    memset(tempItems, 0, sizeof(tempItems));
    BYTE itemCount = SerializeStallItems(pStallData, pStaller->m_CKitbag, tempItems);
    
    if (itemCount == 0) {
        LG("offline_stall", "Failed to serialize stall items for %s\n", pStaller->GetName());
        return false;
    }
    
    // Prepare item data byte buffer for DB storage (serialize from tempItems)
    BYTE itemData[sizeof(SOfflineStallItem) * OFFLINE_STALL_MAX_ITEMS];
    size_t itemDataSize = sizeof(SOfflineStallItem) * itemCount;
    memcpy(itemData, tempItems, itemDataSize);
    
    // Serialize kitbag DB IDs for reference
    BYTE kitbagSnapshot[256];
    memset(kitbagSnapshot, 0, sizeof(kitbagSnapshot));
    for (int i = 0; i < 48 && i * sizeof(DWORD) < sizeof(kitbagSnapshot); i++) {
        SItemGrid* pItem = pStaller->m_CKitbag.GetGridContByNum((short)i);
        if (pItem) {
            memcpy(kitbagSnapshot + i * sizeof(DWORD), &pItem->dwDBID, sizeof(DWORD));
        }
    }
    
    // Get character appearance
    short sLookFace = pStaller->m_SChaPart.SLink[enumEQUIP_FACE].sID;
    short sLookHair = pStaller->m_SChaPart.sHairID;
    short sJob = pStaller->m_SChaPart.sTypeID;
    
    // Get stall skill level (skill ID 241) for booth appearance
    BYTE byStallLevel = 1; // Default to level 1
    SSkillGrid* pStallSkill = pStaller->m_CSkillBag.GetSkillContByID(241);
    if (pStallSkill) {
        byStallLevel = pStallSkill->chLv;
    }
    
    // Capture full equipment appearance for NPC rendering
    SOfflineStallEquipLook equipLook[34]; // enumEQUIP_NUM = 34
    memset(equipLook, 0, sizeof(equipLook));
    for (int i = 0; i < enumEQUIP_NUM; i++) {
        SItemGrid* pEquip = &pStaller->m_SChaPart.SLink[i];
        equipLook[i].sID = pEquip->sID;
        equipLook[i].chForgeLv = pEquip->chForgeLv;
        equipLook[i].lForgeParam = pEquip->GetDBParam(enumITEMDBP_FORGE);
        equipLook[i].lInstID = pEquip->GetDBParam(enumITEMDBP_INST_ID);
        equipLook[i].dwDBID = pEquip->dwDBID;
        equipLook[i].sNeedLv = pEquip->sNeedLv;
        equipLook[i].sNum = pEquip->sNum;
        equipLook[i].sEndure0 = pEquip->sEndure[0];
        equipLook[i].sEndure1 = pEquip->sEndure[1];
        equipLook[i].sEnergy0 = pEquip->sEnergy[0];
        equipLook[i].sEnergy1 = pEquip->sEnergy[1];
        equipLook[i].bValid = pEquip->bValid;
        equipLook[i].bItemTradable = pEquip->bItemTradable;
        equipLook[i].expiration = pEquip->expiration;
        for (int j = 0; j < defITEM_INSTANCE_ATTR_NUM; j++) {
            equipLook[i].sInstAttr[j][0] = pEquip->sInstAttr[j][0];
            equipLook[i].sInstAttr[j][1] = pEquip->sInstAttr[j][1];
        }
    }
    
    // Create in database
    extern CGameDB game_db;
    
    DWORD dwStallID = game_db.CreateOfflineStall(
        dwChaID,
        pStaller->GetName(),
        pPlayer->GetDBActId(),
        pStaller->GetStallName(),  // Stall title from character
        sLookFace,
        sLookHair,
        sJob,
        pMap->GetName(),
        pStaller->GetPos().x,
        pStaller->GetPos().y,
        pStaller->GetAngle(),
        g_Config.m_dwStallTime,  // Duration in hours
        itemCount,
        itemData,
        (DWORD)itemDataSize,
        kitbagSnapshot,
        sizeof(kitbagSnapshot),
        (const BYTE*)equipLook,
        sizeof(equipLook),
        byStallLevel
    );
    
    if (dwStallID == 0) {
        LG("offline_stall", "Database insert failed for %s - creating in-memory stall only\n", pStaller->GetName());
        // Continue anyway - we'll create an in-memory stall even if DB fails
        dwStallID = GenerateWorldID();  // Use generated ID as stall ID
    }
    
    LG("offline_stall", "Created offline stall for %s (stall_id: %u, items: %d, duration: %u hours)\n",
       pStaller->GetName(), dwStallID, itemCount, g_Config.m_dwStallTime);
    
    // Create in-memory stall info structure
    SOfflineStallInfo* pInfo = new SOfflineStallInfo();
    if (!pInfo) {
        LG("offline_stall", "Failed to allocate stall info for %s\n", pStaller->GetName());
        return false;
    }
    
    // Populate stall info
    pInfo->dwStallID = dwStallID;
    pInfo->dwChaID = dwChaID;
    pInfo->dwActID = pPlayer->GetDBActId();
    strncpy_s(pInfo->szChaName, sizeof(pInfo->szChaName), pStaller->GetName(), _TRUNCATE);
    strncpy_s(pInfo->szStallTitle, sizeof(pInfo->szStallTitle), pStaller->GetStallName(), _TRUNCATE);
    strncpy_s(pInfo->szMapName, sizeof(pInfo->szMapName), pMap->GetName(), _TRUNCATE);
    pInfo->nPosX = pStaller->GetPos().x;
    pInfo->nPosY = pStaller->GetPos().y;
    pInfo->sAngle = pStaller->GetAngle();
    pInfo->sLookFace = sLookFace;
    pInfo->sLookHair = sLookHair;
    pInfo->sJob = sJob;
    pInfo->byStallLevel = byStallLevel;
    memcpy(pInfo->equipLook, equipLook, sizeof(equipLook));  // Copy equipment appearance
    pInfo->llPendingGold = 0;
    pInfo->llLastSaleGold = 0;
    pInfo->tCreated = time(nullptr);
    pInfo->tExpire = pInfo->tCreated + (g_Config.m_dwStallTime * 3600);  // Hours to seconds
    pInfo->bActive = true;
    pInfo->pVirtualNPC = nullptr;
    pInfo->dwWorldID = 0;
    pInfo->bySoldCount = 0;  // No items sold yet
    memset(pInfo->soldItems, 0, sizeof(pInfo->soldItems));
    
    // Copy stall items from the already-serialized tempItems.
    // This is the SINGLE code path for item population — SerializeStallItems
    // already resolved each stall good to its correct kitbag item using
    // GetGridContByID(sPosID), so all item data is correct.
    pInfo->byItemCount = itemCount;
    memcpy(pInfo->items, tempItems, sizeof(SOfflineStallItem) * itemCount);
    
    // Add to tracking maps
    m_mapStalls[dwStallID] = pInfo;
    m_mapOwnerToStall[dwChaID] = dwStallID;
    
    // DON'T create the virtual NPC immediately! The player is still in the game at that position.
    // Mark as inactive - the NPC will be created automatically when the player disconnects.
    // See OnPlayerDisconnect() which will detect pending stalls and create NPCs safely.
    pInfo->bActive = false;  // Will be set to true when NPC spawns
    
    LG("offline_stall", "Created offline stall for %s (stall_id: %u). Virtual NPC will spawn after player disconnects.\n",
       pInfo->szChaName, dwStallID);
    
    return true;
}

__int64 COfflineStallMgr::RemoveOfflineStall(DWORD dwChaID, SSoldItemInfo* pSoldItems, BYTE* pSoldCount) {
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Initialize output params
    if (pSoldCount) *pSoldCount = 0;
    
    auto itOwner = m_mapOwnerToStall.find(dwChaID);
    if (itOwner == m_mapOwnerToStall.end()) {
        // Check database directly
        extern CGameDB game_db;
        return game_db.DeleteOfflineStallByCharId(dwChaID);
    }
    
    DWORD dwStallID = itOwner->second;
    auto itStall = m_mapStalls.find(dwStallID);
    
    __int64 llPendingGold = 0;
    
    if (itStall != m_mapStalls.end() && itStall->second) {
        SOfflineStallInfo* pInfo = itStall->second;
        llPendingGold = pInfo->llPendingGold;
        
        // Copy sold items info BEFORE deleting the stall
        if (pSoldItems && pSoldCount && pInfo->bySoldCount > 0) {
            *pSoldCount = pInfo->bySoldCount;
            memcpy(pSoldItems, pInfo->soldItems, sizeof(SSoldItemInfo) * pInfo->bySoldCount);
            LG("offline_stall", "Returning %d sold item records for cha_id %u\n",
               pInfo->bySoldCount, dwChaID);
        }
        
        // Remove virtual NPC from world
        RemoveVirtualNPC(pInfo);
        
        // Remove from world ID map
        if (pInfo->dwWorldID != 0) {
            m_mapWorldIDToNPC.erase(pInfo->dwWorldID);
        }
        
        LG("offline_stall", "Removed offline stall for cha_id %u (pending gold: %lld, sold items: %d)\n",
           dwChaID, llPendingGold, pInfo->bySoldCount);
        
        delete pInfo;
        m_mapStalls.erase(itStall);
    }
    
    m_mapOwnerToStall.erase(itOwner);
    
    // Delete from database
    extern CGameDB game_db;
    game_db.DeleteOfflineStallByCharId(dwChaID);
    
    return llPendingGold;
}

bool COfflineStallMgr::HasOfflineStall(DWORD dwChaID) {
    if (m_mapOwnerToStall.find(dwChaID) != m_mapOwnerToStall.end()) {
        return true;
    }
    
    // Check database
    extern CGameDB game_db;
    return game_db.HasOfflineStall(dwChaID);
}

SOfflineStallInfo* COfflineStallMgr::GetStallByOwner(DWORD dwChaID) {
    auto itOwner = m_mapOwnerToStall.find(dwChaID);
    if (itOwner == m_mapOwnerToStall.end()) {
        return nullptr;
    }
    
    auto itStall = m_mapStalls.find(itOwner->second);
    if (itStall == m_mapStalls.end()) {
        return nullptr;
    }
    
    return itStall->second;
}

COfflineStallNPC* COfflineStallMgr::GetStallNPC(DWORD dwWorldID) {
    auto it = m_mapWorldIDToNPC.find(dwWorldID);
    if (it == m_mapWorldIDToNPC.end()) {
        return nullptr;
    }
    return it->second;
}

bool COfflineStallMgr::ProcessPurchase(CCharacter& buyer, DWORD dwStallWorldID, BYTE byIndex, BYTE byCount) {
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(m_mutex);
    
    COfflineStallNPC* pNPC = GetStallNPC(dwStallWorldID);
    if (!pNPC) {
        buyer.SystemNotice("Stall not found!");
        return false;
    }
    
    if (pNPC->IsExpired()) {
        buyer.SystemNotice("This stall has expired!");
        return false;
    }
    
    return pNPC->BuyItem(buyer, byIndex, byCount);
}

void COfflineStallMgr::MarkStallForRemoval(DWORD dwChaID) {
    // Add to deferred removal set - will be processed in next Update() tick
    m_setDeferredRemovals.insert(dwChaID);
    LG("offline_stall", "Marked stall for deferred removal: cha_id=%u\n", dwChaID);
}

void COfflineStallMgr::Update(DWORD dwCurTime) {
    if (!g_Config.m_bOfflineStall) return;
    if (!m_bInitialized) return;

    // Lock for thread safety
    std::lock_guard<std::mutex> lock(m_mutex);

    // Process deferred removals first (stalls that were emptied by purchases)
    if (!m_setDeferredRemovals.empty()) {
        std::set<DWORD> removals = m_setDeferredRemovals;  // Copy to avoid iterator issues
        m_setDeferredRemovals.clear();
        
        for (DWORD dwChaID : removals) {
            // Don't fully delete the stall — just remove the NPC from the world.
            // The stall record (with sold items and pending gold) must be preserved
            // so the seller can claim them when they log back in.
            auto itOwner = m_mapOwnerToStall.find(dwChaID);
            if (itOwner == m_mapOwnerToStall.end()) continue;
            
            auto itStall = m_mapStalls.find(itOwner->second);
            if (itStall == m_mapStalls.end() || !itStall->second) continue;
            
            SOfflineStallInfo* pInfo = itStall->second;
            
            // Remove NPC from world (stall is empty, nothing more to sell)
            RemoveVirtualNPC(pInfo);
            
            // Save updated state to DB (preserves sold items + pending gold for seller to claim)
            SaveStallUpdate(pInfo);
            
            LG("offline_stall", "Stall emptied for cha_id=%u. NPC removed. Sold items: %d, pending gold: %lld. Record preserved for seller.\n",
               dwChaID, pInfo->bySoldCount, pInfo->llPendingGold);
        }
    }
    
    // Second, check for inactive stalls that need NPCs spawned
    // (These are stalls where player just clicked Offline Mode but hasn't disconnected yet)
    time_t nowSpawn = time(nullptr);
    std::vector<SOfflineStallInfo*> stallsToActivate;
    for (auto& pair : m_mapStalls) {
        SOfflineStallInfo* pInfo = pair.second;
        if (!pInfo || pInfo->bActive) continue;  // Skip active stalls
        
        // Never re-spawn an expired stall. CleanupExpiredStalls() sets bActive=false but keeps
        // the record alive (for pending gold), which would cause this loop to re-spawn it every
        // tick indefinitely. The expiry check here prevents that.
        if (nowSpawn >= pInfo->tExpire) continue;
        
        // Check if player is still online
        CPlayer* pOwner = g_pGameApp->GetPlayerByDBID(pInfo->dwChaID);
        if (!pOwner) {
            // Player has disconnected - safe to spawn NPC now!
            stallsToActivate.push_back(pInfo);
        }
    }
    
    // Spawn NPCs for stalls whose owners have disconnected
    for (SOfflineStallInfo* pInfo : stallsToActivate) {
        LG("offline_stall", "Player %s disconnected, spawning offline stall NPC now...\n", pInfo->szChaName);
        if (CreateVirtualNPC(pInfo)) {
            LG("offline_stall", "Successfully spawned offline stall NPC for %s at %s (%d, %d)\n",
               pInfo->szChaName, pInfo->szMapName, pInfo->nPosX, pInfo->nPosY);
        } else {
            LG("offline_stall", "ERROR: Failed to spawn offline stall NPC for %s\n", pInfo->szChaName);
        }
    }
    
    // Check cleanup interval
    if (dwCurTime - m_dwLastCleanupTime < CLEANUP_INTERVAL) {
        return;
    }
    
    m_dwLastCleanupTime = dwCurTime;
    CleanupExpiredStalls();
}

BYTE COfflineStallMgr::SerializeStallItems(mission::CStallData* pStallData, CKitbag& kitbag,
                                            SOfflineStallItem* pOutItems) {
    if (!pStallData || !pOutItems) return 0;
    
    BYTE outItemCount = 0;
    BYTE numGoods = pStallData->GetGoodsCount();
    LG("offline_stall", "SerializeStallItems: stall has %d goods, kitbag capacity=%d, used=%d\n",
       numGoods, kitbag.GetCapacity(), kitbag.GetUseGridNum());
    
    for (BYTE i = 0; i < numGoods && i < OFFLINE_STALL_MAX_ITEMS; i++) {
        const mission::STALL_GOODS* pGoods = pStallData->GetGoodsInfo(i);
        if (!pGoods) continue;
        
        // byIndex is a sPosID (position ID in the fixed kitbag array).
        // This is validated by CharStall.cpp StartStall() using HasItem(byIndex)
        // and GetID(byIndex), both of which use sPosID-based access.
        // We MUST use GetGridContByID here to match that same sPosID.
        SItemGrid* pKitbagItem = kitbag.GetGridContByID((short)pGoods->byIndex);
        
        if (!pKitbagItem || pKitbagItem->sID <= 0) {
            LG("offline_stall", "  Item %d: SKIPPED - GetGridContByID(%d) returned null (sItemID=%d)\n",
               i, pGoods->byIndex, pGoods->sItemID);
            continue;  // Skip items that can't be found
        }
        
        // Verify the item ID matches what the stall expects
        if (pKitbagItem->sID != pGoods->sItemID) {
            LG("offline_stall", "  Item %d: WARNING - sItemID mismatch! stall=%d, kitbag=%d at sPosID=%d\n",
               i, pGoods->sItemID, pKitbagItem->sID, pGoods->byIndex);
            // Still use it — the kitbag is authoritative, stall sItemID is for reference only
        }
        
        SOfflineStallItem& item = pOutItems[outItemCount];
        memset(&item, 0, sizeof(SOfflineStallItem));
        
        item.byGrid = pGoods->byGrid;
        item.byCount = (BYTE)pGoods->byCount;
        item.byOrigCount = (BYTE)pGoods->byCount;
        item.llMoney = pGoods->llMoney;
        item.byKitbagIndex = pGoods->byIndex;  // Store the sPosID for sold-item cleanup on relog
        item.itemGrid = *pKitbagItem;           // Full item data snapshot
        
        LG("offline_stall", "  Item %d: grid=%d, sID=%d, count=%d, price=%lld, sPosID=%d, dbid=%u, forge=%d\n",
           outItemCount, item.byGrid, item.itemGrid.sID, item.byCount, item.llMoney,
           item.byKitbagIndex, item.itemGrid.dwDBID, item.itemGrid.chForgeLv);
        
        outItemCount++;
    }
    
    LG("offline_stall", "SerializeStallItems: serialized %d/%d items\n", outItemCount, numGoods);
    return outItemCount;
}

bool COfflineStallMgr::DeserializeStallItems(const BYTE* pItemData, size_t itemDataSize,
                                              BYTE itemCount, SOfflineStallInfo* pOutInfo) {
    if (!pItemData || !pOutInfo) return false;
    
    pOutInfo->byItemCount = itemCount;
    
    size_t offset = 0;
    for (BYTE i = 0; i < itemCount && offset < itemDataSize; i++) {
        if (offset + sizeof(SOfflineStallItem) > itemDataSize) break;
        
        memcpy(&pOutInfo->items[i], pItemData + offset, sizeof(SOfflineStallItem));
        offset += sizeof(SOfflineStallItem);
    }
    
    // Read sold items tracking data if present (appended after regular items)
    pOutInfo->bySoldCount = 0;
    memset(pOutInfo->soldItems, 0, sizeof(pOutInfo->soldItems));
    if (offset + sizeof(BYTE) <= itemDataSize) {
        BYTE bySoldCount = 0;
        memcpy(&bySoldCount, pItemData + offset, sizeof(BYTE));
        offset += sizeof(BYTE);
        
        if (bySoldCount > OFFLINE_STALL_MAX_ITEMS) bySoldCount = OFFLINE_STALL_MAX_ITEMS;
        pOutInfo->bySoldCount = bySoldCount;
        
        for (BYTE i = 0; i < bySoldCount && offset + sizeof(SSoldItemInfo) <= itemDataSize; i++) {
            memcpy(&pOutInfo->soldItems[i], pItemData + offset, sizeof(SSoldItemInfo));
            offset += sizeof(SSoldItemInfo);
        }
        
        if (bySoldCount > 0) {
            LG("offline_stall", "DeserializeStallItems: loaded %d sold item records\n", bySoldCount);
        }
    }
    
    return true;
}

bool COfflineStallMgr::CreateVirtualNPC(SOfflineStallInfo* pInfo) {
    if (!pInfo) return false;
    
    // Find the map by name
    CMapRes* pMapRes = g_pGameApp->FindMapByName(pInfo->szMapName);
    if (!pMapRes) {
        LG("offline_stall", "Map %s not found for offline stall\n", pInfo->szMapName);
        return false;
    }
    
    // Get the SubMap (copy 0 = main instance)
    SubMap* pSubMap = pMapRes->GetCopy(0);
    if (!pSubMap) {
        LG("offline_stall", "SubMap not found for map %s\n", pInfo->szMapName);
        return false;
    }
    
    // Create virtual NPC
    COfflineStallNPC* pNPC = new COfflineStallNPC();
    if (!pNPC) return false;
    
    // Assign world ID
    pInfo->dwWorldID = GenerateWorldID();
    LG("offline_stall", "Generated WorldID=%u for stall ID %u\n", pInfo->dwWorldID, pInfo->dwStallID);
    pNPC->Initialize(pInfo);
    
    // Set up the shape for entering the map
    Square shape;
    shape.centre.x = pInfo->nPosX;
    shape.centre.y = pInfo->nPosY;
    shape.radius = 40; // Standard NPC radius
    
    LG("offline_stall", "Attempting to enter NPC into SubMap at [%d, %d]\n", pInfo->nPosX, pInfo->nPosY);
    
    // Add NPC to the SubMap so it can be found by FindCharacter
    if (!pSubMap->Enter(&shape, pNPC, -1)) { // -1 = force enter (no collision check)
        LG("offline_stall", "Failed to enter NPC into SubMap at [%d, %d]\n", 
           pInfo->nPosX, pInfo->nPosY);
        delete pNPC;
        return false;
    }
    
    LG("offline_stall", "Successfully entered NPC into SubMap\n");
    
    // Add stall skill state (SSTATE_STALL = 99) so the stall icon shows
    // We use g_pCSystemCha as the source because our NPC isn't in the entity space
    // (it was created with 'new' not from the entity pool, so IsValidEntity fails)
    // This is the same pattern used by CCharacter::Hide()
    // Use the saved stall level to get the correct booth appearance (default to 1)
    BYTE byStallLv = (pInfo->byStallLevel > 0) ? pInfo->byStallLevel : 1;
    pNPC->AddSkillState(0, g_pCSystemCha->GetID(), g_pCSystemCha->GetHandle(), 
                        enumSKILL_TYPE_SELF, enumSKILL_TAR_LORS, enumSKILL_EFF_HELPFUL, 
                        SSTATE_STALL, byStallLv, -1);
    
    // Broadcast stall name to players in eyeshot (this shows the stall title above the character)
    pNPC->SynStallName();
    
    LG("offline_stall", "Created offline stall NPC world_id=%u at map=%s pos=[%d,%d] title='%s'\n",
       pInfo->dwWorldID, pInfo->szMapName, pInfo->nPosX, pInfo->nPosY, pInfo->szStallTitle);
    
    pInfo->pVirtualNPC = pNPC;
    pInfo->bActive = true;
    
    m_mapWorldIDToNPC[pInfo->dwWorldID] = pNPC;
    
    return true;
}

void COfflineStallMgr::RemoveVirtualNPC(SOfflineStallInfo* pInfo) {
    if (!pInfo) {
        LG("offline_stall", "RemoveVirtualNPC: pInfo is NULL\n");
        return;
    }
    
    if (!pInfo->pVirtualNPC) {
        LG("offline_stall", "RemoveVirtualNPC: No NPC to remove for stall ID %u\n", pInfo->dwStallID);
        return;
    }
    
    LG("offline_stall", "RemoveVirtualNPC: Removing NPC for stall ID %u on map %s\n", pInfo->dwStallID, pInfo->szMapName);
    
    COfflineStallNPC* pNPC = dynamic_cast<COfflineStallNPC*>(pInfo->pVirtualNPC);
    if (pNPC) {
        // Remove world ID mapping first
        m_mapWorldIDToNPC.erase(pInfo->dwWorldID);
        
        // Remove from SubMap
        SubMap* pSubMap = pNPC->GetSubMap();
        if (pSubMap) {
            pSubMap->GoOut(pNPC);
        }
        
        // Cleanup and delete the NPC
        pNPC->Cleanup();
        delete pNPC;
        
        LG("offline_stall", "Removed offline stall NPC world_id=%u\n", pInfo->dwWorldID);
    }
    
    pInfo->pVirtualNPC = nullptr;
    pInfo->bActive = false;
}

void COfflineStallMgr::CleanupExpiredStalls() {
    // IMPORTANT: This is called from Update() which already holds m_mutex.
    // Do NOT call RemoveOfflineStall() here — it also locks m_mutex, 
    // causing a deadlock (std::mutex is not recursive).
    // Instead, do the cleanup inline.
    
    time_t now = time(nullptr);
    std::vector<DWORD> expiredOwners;  // ChaIDs to fully remove
    
    for (auto& pair : m_mapStalls) {
        if (pair.second && now >= pair.second->tExpire) {
            // Remove NPC from world if still active
            if (pair.second->bActive) {
                RemoveVirtualNPC(pair.second);
            }
            
            // Only fully remove if no sold items or gold pending claim
            if (pair.second->bySoldCount == 0 && pair.second->llPendingGold == 0) {
                expiredOwners.push_back(pair.second->dwChaID);
            } else {
                LG("offline_stall", "Expired stall for cha_id %u preserved: %d sold items, %lld gold pending claim\n",
                   pair.second->dwChaID, pair.second->bySoldCount, pair.second->llPendingGold);
            }
        }
    }
    
    // Remove expired stalls inline (no mutex re-lock)
    for (DWORD dwChaID : expiredOwners) {
        LG("offline_stall", "Removing expired stall for cha_id %u (nothing pending)\n", dwChaID);
        
        auto itOwner = m_mapOwnerToStall.find(dwChaID);
        if (itOwner == m_mapOwnerToStall.end()) continue;
        
        DWORD dwStallID = itOwner->second;
        auto itStall = m_mapStalls.find(dwStallID);
        
        if (itStall != m_mapStalls.end() && itStall->second) {
            // NPC already removed above, clean up world ID map
            if (itStall->second->dwWorldID != 0) {
                m_mapWorldIDToNPC.erase(itStall->second->dwWorldID);
            }
            delete itStall->second;
            m_mapStalls.erase(itStall);
        }
        
        m_mapOwnerToStall.erase(itOwner);
        
        // Delete from database
        extern CGameDB game_db;
        game_db.DeleteOfflineStallByCharId(dwChaID);
    }
    
    if (!expiredOwners.empty()) {
        LG("offline_stall", "Cleaned up %zu expired stalls\n", expiredOwners.size());
    }
}

DWORD COfflineStallMgr::GenerateWorldID() {
    return m_dwNextWorldID++;
}

bool COfflineStallMgr::SaveStallUpdate(SOfflineStallInfo* pInfo) {
    if (!pInfo) return false;
    
    // Serialize current items - buffer needs to accommodate full SItemGrid per item
    BYTE itemData[8192];
    memset(itemData, 0, sizeof(itemData));
    size_t offset = 0;
    
    for (BYTE i = 0; i < pInfo->byItemCount; i++) {
        memcpy(itemData + offset, &pInfo->items[i], sizeof(SOfflineStallItem));
        offset += sizeof(SOfflineStallItem);
    }
    
    // Append sold items tracking data after regular items.
    // Format: [BYTE bySoldCount] [SSoldItemInfo * bySoldCount]
    // This ensures sold item records survive server restarts.
    BYTE bySoldCount = pInfo->bySoldCount;
    memcpy(itemData + offset, &bySoldCount, sizeof(BYTE));
    offset += sizeof(BYTE);
    for (BYTE i = 0; i < bySoldCount; i++) {
        memcpy(itemData + offset, &pInfo->soldItems[i], sizeof(SSoldItemInfo));
        offset += sizeof(SSoldItemInfo);
    }
    
    // Save to database - pass only the delta gold earned from this sale,
    // because the SP adds it to the existing pending_gold in DB.
    extern CGameDB game_db;
    __int64 llGoldDelta = pInfo->llLastSaleGold;
    pInfo->llLastSaleGold = 0;  // Reset after saving
    return game_db.UpdateOfflineStallItems(
        pInfo->dwStallID,
        pInfo->byItemCount,
        itemData,
        (DWORD)offset,
        llGoldDelta
    );
}

bool COfflineStallMgr::CleanupForReturningPlayer(CCharacter* pMainCha, DWORD dwChaId) {
    if (!pMainCha) return false;
    if (!HasOfflineStall(dwChaId)) return false;
    
    // Get sold items info along with pending gold
    SSoldItemInfo soldItems[OFFLINE_STALL_MAX_ITEMS];
    BYTE soldCount = 0;
    memset(soldItems, 0, sizeof(soldItems));
    
    __int64 llPendingGold = RemoveOfflineStall(dwChaId, soldItems, &soldCount);
    
    LG("offline_stall", "CleanupForReturningPlayer: %s (cha_id=%u) soldCount=%d, pendingGold=%lld\n",
       pMainCha->GetName(), dwChaId, soldCount, llPendingGold);
    
    // Step 1: Unlock the kitbag (was locked during stall mode)
    pMainCha->m_CKitbag.UnLock();
    
    // Step 2: Remove sold items from the owner's kitbag.
    // byKitbagIndex is a sPosID — use GetGridContByID (not GetGridContByNum).
    int nItemsRemoved = 0;
    if (soldCount > 0) {
        LG("offline_stall", "  Removing %d sold item entries from %s's kitbag\n",
           soldCount, pMainCha->GetName());
        
        for (BYTE s = 0; s < soldCount; s++) {
            short kitbagPosID = (short)soldItems[s].byKitbagIndex;
            SItemGrid* pItem = pMainCha->m_CKitbag.GetGridContByID(kitbagPosID);
            if (pItem && pItem->sID > 0) {
                // Always use quantity-based reduction. bFullySold means the STALL LISTING
                // was exhausted (e.g., listed 1 of 20 scrolls and that 1 sold), NOT that
                // the entire kitbag stack should be removed. Only Clear() when the actual
                // kitbag quantity minus sold count reaches zero.
                short newCount = pItem->sNum - (short)soldItems[s].bySoldCount;
                if (newCount <= 0) {
                    LG("offline_stall", "  sPosID %d: sold %d of %d, clearing slot (sID=%d)\n",
                       kitbagPosID, soldItems[s].bySoldCount, pItem->sNum, pItem->sID);
                    pMainCha->m_CKitbag.Clear(kitbagPosID);
                    nItemsRemoved++;
                } else {
                    LG("offline_stall", "  sPosID %d: sold %d, reducing %d -> %d\n",
                       kitbagPosID, soldItems[s].bySoldCount, pItem->sNum, newCount);
                    pItem->sNum = newCount;
                    pMainCha->m_CKitbag.SetSingleChangeFlag(kitbagPosID);
                    nItemsRemoved++;
                }
            } else {
                LG("offline_stall", "  sPosID %d: item not found or empty\n", kitbagPosID);
            }
        }
    }
    
    // Step 3: Re-enable ALL kitbag items to clear ITEM_DISENABLE flag
    short sCapacity = pMainCha->m_CKitbag.GetCapacity();
    for (short i = 0; i < sCapacity; i++) {
        pMainCha->m_CKitbag.Enable(i, 0);
    }
    
    // Step 4: Defensive validation after cleanup
    pMainCha->CheckBagItemValid(&pMainCha->m_CKitbag);
    
    // Diagnostic: log remaining items
    short sDbgCount = pMainCha->m_CKitbag.GetUseGridNum();
    LG("offline_stall", "  Post-cleanup: %d items remaining (capacity=%d)\n", sDbgCount, sCapacity);
    for (short k = 0; k < sDbgCount; k++) {
        SItemGrid* pDbg = pMainCha->m_CKitbag.GetGridContByNum(k);
        if (pDbg) {
            short sPosID = pMainCha->m_CKitbag.GetPosIDByNum(k);
            CItemRecord* pDbgRec = GetItemRecordInfo(pDbg->sID);
            LG("offline_stall", "    [seq=%d sPosID=%d] sID=%d name=%s sNum=%d\n",
               k, sPosID, pDbg->sID,
               pDbgRec ? pDbgRec->szName : "UNKNOWN",
               pDbg->sNum);
        }
    }
    
    // Step 5: Credit pending gold
    if (llPendingGold > 0) {
        __int64 currentGold = pMainCha->getAttr(ATTR_GD);
        __int64 maxGold = 100000000000LL;  // 100 billion cap
        __int64 actualGold = llPendingGold;
        
        if (currentGold >= maxGold) {
            pMainCha->SystemNotice("Your offline stall earned %lld gold, but your inventory is at the maximum limit!", llPendingGold);
            actualGold = 0;
        } else if (currentGold + llPendingGold > maxGold) {
            actualGold = maxGold - currentGold;
            pMainCha->SystemNotice("Your offline stall earned %lld gold (capped at max), you received %lld gold!", llPendingGold, actualGold);
        } else {
            pMainCha->SystemNotice("Your offline stall earned %lld gold while you were away! %d item(s) sold.", llPendingGold, nItemsRemoved);
        }
        
        if (actualGold > 0) {
            pMainCha->setAttr(ATTR_GD, currentGold + actualGold);
            pMainCha->SynAttr(enumATTRSYN_TRADE);
        }
        LG("offline_stall", "  Gold credited: %lld (earned: %lld, items removed: %d)\n",
           actualGold, llPendingGold, nItemsRemoved);
    } else {
        pMainCha->SystemNotice("Your offline stall has been closed. %d item(s) sold.", nItemsRemoved);
        LG("offline_stall", "  Stall closed, no pending gold (items removed: %d)\n", nItemsRemoved);
    }
    
    return true;
}

// Add a new stall to the manager (called from DB load)
bool COfflineStallMgr::AddLoadedStall(SOfflineStallInfo* pInfo) {
    if (!pInfo) {
        LG("offline_stall", "AddLoadedStall: pInfo is NULL\n");
        return false;
    }
    
    LG("offline_stall", "AddLoadedStall: ID=%u, ChaID=%u, Map=%s\n", pInfo->dwStallID, pInfo->dwChaID, pInfo->szMapName);
    
    m_mapStalls[pInfo->dwStallID] = pInfo;
    m_mapOwnerToStall[pInfo->dwChaID] = pInfo->dwStallID;
    
    // Create virtual NPC
    LG("offline_stall", "Calling CreateVirtualNPC for stall ID %u\n", pInfo->dwStallID);
    CreateVirtualNPC(pInfo);
    
    LG("offline_stall", "AddLoadedStall complete: WorldID=%u\n", pInfo->dwWorldID);
    return true;
}
