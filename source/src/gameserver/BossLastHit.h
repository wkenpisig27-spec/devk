// BossLastHit.h - Boss last-hit drop ownership system
// Determines which bosses give drop ownership to the killer (last hitter)
// instead of using the default team-based drop system.
//
// Configuration is separate from BossTimer (BossTracked.txt).
// Boss list is loaded from BossLastHit.txt.

#pragma once

namespace BossLastHit
{
    // Initialize - loads boss list from BossLastHit.txt
    // Call once at server startup
    void Initialize();

    // Check if a ChaID is configured for last-hit drop ownership
    bool IsLastHitBoss(unsigned int chaId);

}  // namespace BossLastHit
