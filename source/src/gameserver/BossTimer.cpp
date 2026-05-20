// BossTimer.cpp - Simple boss respawn timer system
// Real-time based timers that persist across server restarts

#include "stdafx.h"
#include "BossTimer.h"
#include "Character.h"
#include "GameApp.h"
#include "MapRes.h"
#include "GameAppNet.h"
#include "NetCommand.h"
#include "SubMap.h"
#include "CompCommand.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <ctime>
#include <unordered_map>
#ifdef PKO_PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace BossTimer
{
    // In-memory data
    static std::unordered_map<unsigned int, BossConfig> g_BossConfigs;   // ChaID -> Config
    static std::unordered_map<unsigned int, BossState> g_BossStates;     // ChaID -> State (dead bosses only)
    static std::vector<PendingSpawn> g_PendingSpawns;                    // Bosses blocked at startup, waiting for timer
    static volatile bool g_Initialized = false;  // volatile: read from game thread + console handler thread
    static bool g_StateDirty = false;       // True if state needs saving to disk
    static DWORD g_LastSaveTick = 0;        // Last time we saved state to disk
    static DWORD g_LastCheckTick = 0;       // Last time we checked pending spawns

    static const DWORD SAVE_INTERVAL_MS = 30 * 1000;   // Save state every 30 seconds (if dirty)
    static const DWORD CHECK_INTERVAL_MS = 10 * 1000;  // Check pending spawns every 10 seconds

    // File paths
    static const char* CONFIG_FILE = "BossTracked.txt";
    static std::string g_StateFilePath;   // Set per-GS instance in Initialize() — e.g. BossTimers_GS1.txt

    //--------------------------------------------------------------------------
    // Helper: Get boss name from ChaRecord
    //--------------------------------------------------------------------------
    static const char* GetBossName(unsigned int chaId)
    {
        CChaRecord* pRec = GetChaRecordInfo(static_cast<int>(chaId));
        if (pRec)
            return pRec->szName;
        return "Unknown";
    }

    //--------------------------------------------------------------------------
    // Load config file (BossTracked.txt)
    // Format: ChaID,RespawnSeconds
    // Example: 789,7200
    //--------------------------------------------------------------------------
    static void LoadConfig()
    {
        g_BossConfigs.clear();

        std::ifstream file(CONFIG_FILE);
        if (!file.is_open())
        {
            LG("BossTimer", "Warning: %s not found - no bosses will be tracked\n", CONFIG_FILE);
            return;
        }

        std::string line;
        int count = 0;
        while (std::getline(file, line))
        {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#' || line[0] == '/')
                continue;

            // Parse: ChaID,RespawnSeconds
            unsigned int chaId = 0;
            unsigned int respawnSec = 0;
            if (sscanf(line.c_str(), "%u,%u", &chaId, &respawnSec) == 2)
            {
                BossConfig cfg;
                cfg.chaId = chaId;
                cfg.respawnSeconds = respawnSec;
                g_BossConfigs[chaId] = cfg;
                count++;

                LG("BossTimer", "Tracking boss %u (%s) - respawn: %u seconds\n", 
                   chaId, GetBossName(chaId), respawnSec);
            }
        }

        LG("BossTimer", "Loaded %d boss configurations from %s\n", count, CONFIG_FILE);
    }

    //--------------------------------------------------------------------------
    // Load state file (BossTimers.txt)
    // Format: ChaID,MapName,DeathTimestamp
    // Example: 789,garner,1738900000
    //--------------------------------------------------------------------------
    static void LoadState()
    {
        g_BossStates.clear();

        std::ifstream file(g_StateFilePath);
        if (!file.is_open())
        {
            LG("BossTimer", "No state file found - all bosses can spawn\n");
            return;
        }

        time_t now = time(nullptr);
        std::string line;
        int loaded = 0;
        int expired = 0;

        while (std::getline(file, line))
        {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#' || line[0] == '/')
                continue;

            // Parse: ChaID,MapName,DeathTimestamp
            unsigned int chaId = 0;
            char mapName[128] = {0};
            long long deathTime = 0;

            if (sscanf(line.c_str(), "%u,%127[^,],%lld", &chaId, mapName, &deathTime) == 3)
            {
                // Check if this boss is still tracked
                auto cfgIt = g_BossConfigs.find(chaId);
                if (cfgIt == g_BossConfigs.end())
                {
                    LG("BossTimer", "Ignoring untracked boss %u in state file\n", chaId);
                    continue;
                }

                // Calculate spawn time
                time_t spawnTime = static_cast<time_t>(deathTime) + cfgIt->second.respawnSeconds;
                
                if (now >= spawnTime)
                {
                    // Timer expired - boss can spawn
                    LG("BossTimer", "Boss %u (%s) timer expired - can spawn now\n", 
                       chaId, GetBossName(chaId));
                    expired++;
                }
                else
                {
                    // Timer still active - save state
                    BossState state;
                    state.chaId = chaId;
                    state.mapName = mapName;
                    state.deathTime = static_cast<time_t>(deathTime);
                    g_BossStates[chaId] = state;
                    
                    int remaining = static_cast<int>(spawnTime - now);
                    LG("BossTimer", "Boss %u (%s) on cooldown - %d seconds remaining\n", 
                       chaId, GetBossName(chaId), remaining);
                    loaded++;
                }
            }
        }

        LG("BossTimer", "Loaded %d active timers, %d expired\n", loaded, expired);
    }

    //--------------------------------------------------------------------------
    // Save state file (BossTimers.txt)
    // Uses atomic write (write to temp, then rename)
    //--------------------------------------------------------------------------
    static void SaveState()
    {
        // Write directly to state file (no temp+rename to avoid Windows rename failures)
        std::ofstream file(g_StateFilePath, std::ios::out | std::ios::trunc);
        if (!file.is_open())
        {
            LG("BossTimer", "ERROR: Cannot write to %s\n", g_StateFilePath.c_str());
            return;
        }

        file << "# BossTimer State File - DO NOT EDIT\n";
        file << "# Format: ChaID,MapName,DeathTimestamp\n";

        for (const auto& pair : g_BossStates)
        {
            const BossState& state = pair.second;
            file << state.chaId << "," << state.mapName << "," << static_cast<long long>(state.deathTime) << "\n";
        }

        file.flush();
        file.close();

        LG("BossTimer", "Saved %zu boss states to %s\n", g_BossStates.size(), g_StateFilePath.c_str());
    }

    //--------------------------------------------------------------------------
    // Public API
    //--------------------------------------------------------------------------

    void Initialize()
    {
        if (g_Initialized)
            return;

        LG("BossTimer", "=== BossTimer Initializing ===\n");

        // Use per-GS state file so multiple GS instances on the same host don't clobber each other.
        g_StateFilePath = std::string("BossTimers_") + g_Config.m_szName + ".txt";
        LG("BossTimer", "State file: %s\n", g_StateFilePath.c_str());

        LoadConfig();
        LoadState();
        
        g_Initialized = true;
        LG("BossTimer", "=== BossTimer Ready ===\n");
    }

    void Shutdown()
    {
        if (!g_Initialized)
            return;

        // Set flag FIRST so game thread stops touching g_BossStates
        g_Initialized = false;

        LG("BossTimer", "Shutting down - saving state\n");
        SaveState();
    }

    bool IsTrackedBoss(unsigned int chaId)
    {
        return g_BossConfigs.find(chaId) != g_BossConfigs.end();
    }

    bool IsOnCooldown(unsigned int chaId)
    {
        if (!g_Initialized || !IsTrackedBoss(chaId))
            return false;

        auto stateIt = g_BossStates.find(chaId);
        if (stateIt == g_BossStates.end())
            return false;  // No death recorded - not on cooldown

        const BossConfig& config = g_BossConfigs[chaId];
        time_t now = time(nullptr);
        time_t spawnTime = stateIt->second.deathTime + config.respawnSeconds;

        if (now >= spawnTime)
        {
            // Timer expired - clear death state
            g_BossStates.erase(stateIt);
            g_StateDirty = true;
            LG("BossTimer", "Boss %u (%s) cooldown expired (checked from respawn queue)\n",
               chaId, GetBossName(chaId));
            return false;
        }

        return true;  // Still on cooldown
    }

    bool CanSpawn(unsigned int chaId, const char* mapName,
                  int posX, int posY, short angle, int resumeTimeMs)
    {
        // Not initialized or not a tracked boss - allow spawn
        if (!g_Initialized || !IsTrackedBoss(chaId))
            return true;

        // Check if boss is on cooldown
        auto stateIt = g_BossStates.find(chaId);
        if (stateIt == g_BossStates.end())
        {
            // No death recorded - allow spawn
            return true;
        }

        // Calculate remaining time
        const BossState& state = stateIt->second;
        const BossConfig& config = g_BossConfigs[chaId];
        
        time_t now = time(nullptr);
        time_t spawnTime = state.deathTime + config.respawnSeconds;

        if (now >= spawnTime)
        {
            // Timer expired - remove state and allow spawn
            g_BossStates.erase(stateIt);
            g_StateDirty = true;  // Mark for batched save
            
            LG("BossTimer", "Boss %u (%s) timer expired - allowing spawn on %s\n", 
               chaId, GetBossName(chaId), mapName);
            return true;
        }
        else
        {
            // Still on cooldown - record as pending for deferred spawn
            int remaining = static_cast<int>(spawnTime - now);
            LG("BossTimer", "Boss %u (%s) blocked - %d seconds remaining, queued for deferred spawn on %s\n", 
               chaId, GetBossName(chaId), remaining, mapName);

            // Check if already in pending list
            bool alreadyPending = false;
            for (const auto& ps : g_PendingSpawns)
            {
                if (ps.chaId == chaId)
                {
                    alreadyPending = true;
                    break;
                }
            }
            if (!alreadyPending)
            {
                PendingSpawn ps;
                ps.chaId = chaId;
                ps.mapName = mapName ? mapName : "";
                ps.posX = posX;
                ps.posY = posY;
                ps.angle = angle;
                ps.resumeTimeMs = resumeTimeMs;
                g_PendingSpawns.push_back(ps);
            }

            return false;
        }
    }

    void OnBossDeath(unsigned int chaId, const char* mapName)
    {
        if (!g_Initialized || !IsTrackedBoss(chaId))
            return;

        // Record death
        BossState state;
        state.chaId = chaId;
        state.mapName = mapName ? mapName : "";
        state.deathTime = time(nullptr);

        g_BossStates[chaId] = state;
        
        const BossConfig& config = g_BossConfigs[chaId];
        LG("BossTimer", "Boss %u (%s) died on %s - respawn in %u seconds\n", 
           chaId, GetBossName(chaId), state.mapName.c_str(), config.respawnSeconds);

        // Save immediately - boss deaths are rare (once per day per boss)
        // and MUST be persisted to survive crashes/restarts
        SaveState();
        g_StateDirty = false;
    }

    //--------------------------------------------------------------------------
    // Load and merge boss states from ALL GS state files for UI display.
    // Scans the current directory for BossTimers_*.txt files, parses each one,
    // and merges into a single map. If the same boss appears in two files
    // (shouldn't happen normally), the most-recent death time wins.
    // This is read-only — it never modifies g_BossStates.
    //--------------------------------------------------------------------------
    static std::unordered_map<unsigned int, BossState> LoadAllStatesForDisplay()
    {
        std::unordered_map<unsigned int, BossState> merged;
        time_t now = time(nullptr);

#ifdef PKO_PLATFORM_WINDOWS
        WIN32_FIND_DATAA fd;
        HANDLE hFind = FindFirstFileA("BossTimers_*.txt", &fd);
        if (hFind == INVALID_HANDLE_VALUE)
            return merged;  // No files yet — all bosses alive

        do
        {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;

            std::ifstream file(fd.cFileName);
            if (!file.is_open())
                continue;

            std::string line;
            while (std::getline(file, line))
            {
                if (line.empty() || line[0] == '#' || line[0] == '/')
                    continue;

                unsigned int chaId = 0;
                char mapName[128] = {0};
                long long deathTime = 0;
                if (sscanf(line.c_str(), "%u,%127[^,],%lld", &chaId, mapName, &deathTime) != 3)
                    continue;

                auto cfgIt = g_BossConfigs.find(chaId);
                if (cfgIt == g_BossConfigs.end())
                    continue;  // Not a tracked boss

                time_t spawnTime = static_cast<time_t>(deathTime) + cfgIt->second.respawnSeconds;
                if (now >= spawnTime)
                    continue;  // Already expired — treat as alive

                // Keep whichever record has the most-recent death time
                auto existing = merged.find(chaId);
                if (existing == merged.end() || deathTime > static_cast<long long>(existing->second.deathTime))
                {
                    BossState state;
                    state.chaId = chaId;
                    state.mapName = mapName;
                    state.deathTime = static_cast<time_t>(deathTime);
                    merged[chaId] = state;
                }
            }
        }
        while (FindNextFileA(hFind, &fd));

        FindClose(hFind);
#else
        // Linux: fall back to in-memory g_BossStates (single-GS display)
        merged = g_BossStates;
#endif
        return merged;
    }

    void GetDisplayInfo(std::vector<BossDisplayInfo>& outList)
    {
        outList.clear();
        time_t now = time(nullptr);

        // Merge death records from all GS state files so cross-GS bosses
        // show the correct dead/alive status regardless of which GS the player is on.
        const auto mergedStates = LoadAllStatesForDisplay();

        // Go through ALL tracked bosses
        for (const auto& cfgPair : g_BossConfigs)
        {
            const BossConfig& config = cfgPair.second;
            BossDisplayInfo info;
            info.chaId = config.chaId;
            
            // Get name
            const char* name = GetBossName(config.chaId);
            strncpy(info.name, name, sizeof(info.name) - 1);
            info.name[sizeof(info.name) - 1] = '\0';

            // Check if dead (on cooldown) — uses merged cross-GS state
            auto stateIt = mergedStates.find(config.chaId);
            if (stateIt != mergedStates.end())
            {
                // Boss is dead - calculate remaining time
                const BossState& state = stateIt->second;
                time_t spawnTime = state.deathTime + config.respawnSeconds;
                
                if (now < spawnTime)
                {
                    info.status = 1;  // Dead
                    info.remainingSeconds = static_cast<unsigned int>(spawnTime - now);
                }
                else
                {
                    info.status = 0;  // Alive (timer just expired)
                    info.remainingSeconds = 0;
                }
            }
            else
            {
                // No death record across any GS — boss is alive
                info.status = 0;
                info.remainingSeconds = 0;
            }

            outList.push_back(info);
        }

        // Sort by chaId for consistent display
        std::sort(outList.begin(), outList.end(), 
            [](const BossDisplayInfo& a, const BossDisplayInfo& b) {
                return a.chaId < b.chaId;
            });
    }

    void SendToPlayer(::CCharacter* pCha)
    {
        if (!pCha || !g_Initialized)
            return;

        std::vector<BossDisplayInfo> displayList;
        GetDisplayInfo(displayList);

        if (displayList.empty())
        {
            LG("BossTimer", "No bosses to send to player %s\n", pCha->GetName());
            return;
        }

        // Build packet
        // Format: [Count:2][Entry:ChaID(4)+Status(1)+Remaining(4)+NameLen(1)+Name(var)]...
        WPacket wpk = GETWPACKET();
        WRITE_CMD(wpk, CMD_MC_BOSSTIMER_SYNC);
        WRITE_SHORT(wpk, static_cast<short>(displayList.size()));

        for (const BossDisplayInfo& info : displayList)
        {
            WRITE_LONG(wpk, info.chaId);
            WRITE_CHAR(wpk, info.status);
            WRITE_LONG(wpk, info.remainingSeconds);
            
            unsigned char nameLen = static_cast<unsigned char>(strlen(info.name));
            WRITE_CHAR(wpk, nameLen);
            WRITE_STRING(wpk, info.name);
        }

        // Send to player
        pCha->ReflectINFof(pCha, wpk);
        
        LG("BossTimer", "Sent %zu boss entries to player %s\n", displayList.size(), pCha->GetName());
    }

    void SetPendingResumeTime(unsigned int chaId, int timeMs)
    {
        for (auto& ps : g_PendingSpawns)
        {
            if (ps.chaId == chaId)
            {
                ps.resumeTimeMs = timeMs;
                return;
            }
        }
    }

    void Update(unsigned int dwCurTime)
    {
        if (!g_Initialized)
            return;

        // Batched save: write state to disk periodically if dirty
        if (g_StateDirty && (dwCurTime - g_LastSaveTick >= SAVE_INTERVAL_MS))
        {
            SaveState();
            g_StateDirty = false;
            g_LastSaveTick = dwCurTime;
        }

        // Check pending spawns periodically
        if (dwCurTime - g_LastCheckTick < CHECK_INTERVAL_MS)
            return;
        g_LastCheckTick = dwCurTime;

        if (g_PendingSpawns.empty())
            return;

        time_t now = time(nullptr);

        // Iterate pending spawns, try to spawn expired ones
        auto it = g_PendingSpawns.begin();
        while (it != g_PendingSpawns.end())
        {
            const PendingSpawn& ps = *it;

            // Check if timer has expired
            auto stateIt = g_BossStates.find(ps.chaId);
            if (stateIt != g_BossStates.end())
            {
                const BossConfig& config = g_BossConfigs[ps.chaId];
                time_t spawnTime = stateIt->second.deathTime + config.respawnSeconds;

                if (now < spawnTime)
                {
                    // Still on cooldown, skip
                    ++it;
                    continue;
                }

                // Timer expired - remove death state
                g_BossStates.erase(stateIt);
                g_StateDirty = true;
            }

            // Try to spawn boss on its map using saved position
            LG("BossTimer", "Timer expired for boss %u (%s) - attempting deferred spawn on %s at [%d, %d]\n",
               ps.chaId, GetBossName(ps.chaId), ps.mapName.c_str(), ps.posX, ps.posY);

            bool spawned = false;
            CMapRes* pMapRes = g_pGameApp->FindMapByName(ps.mapName.c_str());
            if (pMapRes && pMapRes->IsOpen())
            {
                SubMap* pSubMap = pMapRes->GetCopy(0);
                if (pSubMap)
                {
                    Point pos = { ps.posX, ps.posY };
                    CCharacter* pCha = pSubMap->ChaSpawn(
                        ps.chaId, enumCHACTRL_NONE, ps.angle, &pos);
                    if (pCha)
                    {
                        if (ps.resumeTimeMs > 0)
                            pCha->SetResumeTime(ps.resumeTimeMs);

                        LG("BossTimer", "Deferred spawn SUCCESS: boss %u (%s) spawned on %s at [%d, %d]\n",
                           ps.chaId, GetBossName(ps.chaId), ps.mapName.c_str(), ps.posX, ps.posY);
                        spawned = true;
                    }
                    else
                    {
                        LG("BossTimer", "Deferred spawn FAILED: ChaSpawn returned null for boss %u on %s\n",
                           ps.chaId, ps.mapName.c_str());
                    }
                }
            }
            else
            {
                LG("BossTimer", "WARNING: Map %s not found or not open for boss %u deferred spawn\n",
                   ps.mapName.c_str(), ps.chaId);
            }

            // Remove from pending list whether spawn succeeded or not
            it = g_PendingSpawns.erase(it);
        }
    }

}  // namespace BossTimer
