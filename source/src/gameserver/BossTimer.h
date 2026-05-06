// BossTimer.h - Simple boss respawn timer system
// Tracks boss deaths and prevents respawn until timer expires
// Timer is REAL-TIME based - counts down even when server is offline

#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <ctime>

// Forward declarations
class CCharacter;
class SubMap;

namespace BossTimer
{
    // Boss configuration (from BossTracked.txt)
    struct BossConfig
    {
        unsigned int chaId;
        unsigned int respawnSeconds;  // How int until respawn (in seconds)
    };

    // Boss death state (saved to BossTimers.txt)
    struct BossState
    {
        unsigned int chaId;
        std::string mapName;
        time_t deathTime;             // Unix timestamp when boss died
    };

    // Pending spawn info - saved when a boss is blocked at startup
    // so we can spawn it later when the timer expires
    struct PendingSpawn
    {
        unsigned int chaId;
        std::string mapName;          // Map name where boss should spawn
        int posX, posY;              // Spawn position
        short angle;                  // Spawn angle
        int resumeTimeMs;            // Resume time in milliseconds (from spawn table)
    };

    // Data for client display
    struct BossDisplayInfo
    {
        unsigned int chaId;
        char name[64];
        unsigned char status;         // 0 = Alive, 1 = Dead (respawning)
        unsigned int remainingSeconds;
    };

    // Initialize the system - call once at server startup BEFORE map loading
    void Initialize();

    // Shutdown - saves state (called on server shutdown)
    void Shutdown();

    // Check if a boss spawn should be allowed
    // Returns true if spawn is allowed, false if still on cooldown
    // If blocked, records spawn info for deferred respawn
    // posX/posY/angle are saved so the boss can be spawned later when the timer expires
    bool CanSpawn(unsigned int chaId, const char* mapName,
                  int posX = 0, int posY = 0, short angle = 0, int resumeTimeMs = 0);

    // Record a boss death - call when a tracked boss dies
    void OnBossDeath(unsigned int chaId, const char* mapName);

    // Periodic update - call from main game loop
    // Checks if any pending boss timers have expired and spawns them
    void Update(unsigned int dwCurTime);

    // Get display info for all tracked bosses (for client UI)
    void GetDisplayInfo(std::vector<BossDisplayInfo>& outList);

    // Send boss timer data to a specific player
    void SendToPlayer(::CCharacter* pCha);

    // Check if a ChaID is a tracked boss
    bool IsTrackedBoss(unsigned int chaId);

    // Check if a tracked boss is currently on respawn cooldown
    // Returns true if the boss is dead and timer hasn't expired yet
    // If the timer has expired, clears the death state and returns false
    bool IsOnCooldown(unsigned int chaId);

    // Set resume time for a pending boss spawn (called by EntitySpawn when ChaSpawn is blocked)
    void SetPendingResumeTime(unsigned int chaId, int timeMs);

}  // namespace BossTimer
