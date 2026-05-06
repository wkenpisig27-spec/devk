#pragma once
#include "Player.h"
#include <deque>

// Maximum number of guilds supported (guild IDs 1 to MAX_GUILD_ID)
constexpr int MAX_GUILD_ID = 1000;

struct GuildBankMsg {
	Player* player;
	WPacket msg;
	DWORD playerChaID;  // Store character ID for validation
};

std::deque<GuildBankMsg> guildBankMsgQueue[MAX_GUILD_ID + 1];

// Helper to remove queued messages for a disconnected player
inline void RemovePlayerFromGuildBankQueue(Player* player) {
	if (!player) return;
	for (int guildID = 1; guildID <= MAX_GUILD_ID; ++guildID) {
		auto& queue = guildBankMsgQueue[guildID];
		for (auto it = queue.begin(); it != queue.end(); ) {
			if (it->player == player) {
				it = queue.erase(it);
			} else {
				++it;
			}
		}
	}
}
