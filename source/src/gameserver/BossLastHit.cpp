// BossLastHit.cpp - Boss last-hit drop ownership system
// Loads a simple list of ChaIDs from BossLastHit.txt
// Bosses in this list give drop ownership to the killer (last hitter)

#include "stdafx.h"
#include "BossLastHit.h"

#include <fstream>
#include <string>
#include <unordered_set>

namespace BossLastHit
{
    static std::unordered_set<unsigned int> g_LastHitBosses;
    static bool g_Initialized = false;

    static const char* CONFIG_FILE = "BossLastHit.txt";

    void Initialize()
    {
        if (g_Initialized)
            return;

        g_LastHitBosses.clear();

        std::ifstream file(CONFIG_FILE);
        if (!file.is_open())
        {
            LG("BossLastHit", "Warning: %s not found - no bosses will use last-hit drop\n", CONFIG_FILE);
            g_Initialized = true;
            return;
        }

        std::string line;
        int count = 0;
        while (std::getline(file, line))
        {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#' || line[0] == '/')
                continue;

            // Parse: ChaID (one per line)
            unsigned int chaId = 0;
            if (sscanf(line.c_str(), "%u", &chaId) == 1 && chaId > 0)
            {
                g_LastHitBosses.insert(chaId);
                count++;
            }
        }

        g_Initialized = true;
        LG("BossLastHit", "Loaded %d boss IDs from %s\n", count, CONFIG_FILE);
    }

    bool IsLastHitBoss(unsigned int chaId)
    {
        if (!g_Initialized)
            return false;

        return g_LastHitBosses.find(chaId) != g_LastHitBosses.end();
    }

}  // namespace BossLastHit
