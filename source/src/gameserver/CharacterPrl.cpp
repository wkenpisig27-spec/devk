#include "stdafx.h"
#include "SubMap.h"
#include "GameApp.h"
#include "GameAppNet.h"
#include "CharTrade.h"
#include "Parser.h"
#include "NPC.h"
#include "WorldEudemon.h"
#include "Player.h"
#include "LevelRecord.h"
#include "CharForge.h"
#include "HairRecord.h"
#include "gamedb.h"

#include "Birthplace.h"
#include "CharBoat.h"
#include "Guild.h"
#include "CharStall.h"

#include "Auction.h"
#include <cctype>

static bool IsEconomyBlockedByDB(CCharacter& cha) {
	if (!g_bDBDegraded)
		return false;
	cha.SystemNotice("Server database is temporarily unavailable. Economy actions are disabled.");
	return true;
}
#include <cerrno>
#include <climits>
#include <conformity.h>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>
#include "PacketSanitizer.h"  // Packet validation utilities
#include "BossTimer.h"        // Boss respawn timer system
#include "common/OpcodeHandlerRegistry.h"
#include "common/OpcodeIngress.h"
#include "common/OpcodeMeta.h"
#include "common/PacketReader.h"
#include <stdexcept>

_DBC_USING

const short g_sLiveSkillNeedItemNum[4] = {6, 4, 6, 6};
extern std::string g_strLogName;

namespace {

struct SChestPreviewEntry {
	int itemID;
	int quantity;
	int weight;
};

struct SChestPreviewTable {
	std::vector<SChestPreviewEntry> entries;
	int totalWeight = 0;
};

using SChestPreviewMap = std::unordered_map<int, SChestPreviewTable>;

bool g_chestPreviewLoaded = false;
DWORD g_chestPreviewLastLoadAttempt = 0;
SChestPreviewMap g_chestPreviewTables;

constexpr DWORD kChestPreviewRequestCooldownMs = 500;
constexpr DWORD kChestPreviewLoadRetryCooldownMs = 5000;

bool TryParseInt(const std::string& text, int& value) {
	if (text.empty()) {
		return false;
	}

	errno = 0;
	char* end = nullptr;
	const long parsed = std::strtol(text.c_str(), &end, 10);
	if (end == text.c_str() || errno == ERANGE || parsed < INT_MIN || parsed > INT_MAX) {
		return false;
	}

	while (*end != '\0') {
		if (!std::isspace(static_cast<unsigned char>(*end))) {
			return false;
		}
		++end;
	}

	value = static_cast<int>(parsed);
	return true;
}

bool ParseChestTableMappings(const std::string& itemEffectPath, std::unordered_map<int, std::string>& chestToLuaTable) {
	std::ifstream input(itemEffectPath);
	if (!input.is_open()) {
		return false;
	}

	const std::regex mappingRegex(R"(rItem\[(\d+)\]\s*=\s*(\w+))");
	std::string line;
	while (std::getline(input, line)) {
		std::smatch match;
		if (!std::regex_search(line, match, mappingRegex)) {
			continue;
		}

		int chestID = 0;
		if (!TryParseInt(match[1].str(), chestID)) {
			continue;
		}
		chestToLuaTable[chestID] = match[2].str();
	}

	return !chestToLuaTable.empty();
}

bool ParseChestEntries(const std::string& variablePath, const std::unordered_map<int, std::string>& chestToLuaTable, SChestPreviewMap& outTables) {
	if (chestToLuaTable.empty()) {
		return false;
	}

	std::unordered_map<std::string, int> luaTableToChest;
	for (const auto& entry : chestToLuaTable) {
		luaTableToChest[entry.second] = entry.first;
		outTables[entry.first] = SChestPreviewTable{};
	}

	std::ifstream input(variablePath);
	if (!input.is_open()) {
		return false;
	}

	const std::regex entryRegex(R"((\w+)\[\s*\d+\s*\]\s*=\s*\{[^}]*Active\s*=\s*(\d+)[^}]*ItemID\s*=\s*(\d+)[^}]*Quantity\s*=\s*(\d+)[^}]*Rad\s*=\s*(\d+))");
	std::string line;
	while (std::getline(input, line)) {
		std::smatch match;
		if (!std::regex_search(line, match, entryRegex)) {
			continue;
		}

		auto chestIt = luaTableToChest.find(match[1].str());
		if (chestIt == luaTableToChest.end()) {
			continue;
		}

		int active = 0, itemID = 0, quantity = 0, weight = 0;
		if (!TryParseInt(match[2].str(), active) || !TryParseInt(match[3].str(), itemID) ||
			!TryParseInt(match[4].str(), quantity) || !TryParseInt(match[5].str(), weight) || active != 1) {
			continue;
		}

		auto& table = outTables[chestIt->second];
		table.entries.push_back({itemID, quantity, weight});
		table.totalWeight += weight;
	}

	for (auto it = outTables.begin(); it != outTables.end();) {
		if (it->second.entries.empty() || it->second.totalWeight <= 0) {
			it = outTables.erase(it);
		} else {
			++it;
		}
	}

	return !outTables.empty();
}

void LoadChestPreviewTables() {
	if (g_chestPreviewLoaded) {
		return;
	}

	const DWORD now = GetTickCount();
	if (now - g_chestPreviewLastLoadAttempt < kChestPreviewLoadRetryCooldownMs) {
		return;
	}
	g_chestPreviewLastLoadAttempt = now;

	g_chestPreviewTables.clear();

	const char* itemEffectCandidates[] = {
		"resource/script/calculate/ItemEffect.lua",
		"server/resource/script/calculate/ItemEffect.lua",
		"../server/resource/script/calculate/ItemEffect.lua",
	};
	const char* variableCandidates[] = {
		"resource/script/calculate/variable.lua",
		"server/resource/script/calculate/variable.lua",
		"../server/resource/script/calculate/variable.lua",
	};

	try {
		std::unordered_map<int, std::string> chestToLuaTable;
		for (const char* path : itemEffectCandidates) {
			if (ParseChestTableMappings(path, chestToLuaTable)) {
				break;
			}
		}

		if (!chestToLuaTable.empty()) {
			for (const char* path : variableCandidates) {
				if (ParseChestEntries(path, chestToLuaTable, g_chestPreviewTables)) {
					break;
				}
			}
		}

		g_chestPreviewLoaded = !g_chestPreviewTables.empty();
		if (!g_chestPreviewLoaded) {
			LG("Security", "[ChestPreview] Preview tables not loaded (no valid entries found), retrying later\n");
		}
	} catch (const std::exception& ex) {
		LG("Security", "[ChestPreview] Failed to load preview tables: %s\n", ex.what());
		g_chestPreviewLoaded = false;
		g_chestPreviewTables.clear();
	} catch (...) {
		LG("Security", "[ChestPreview] Failed to load preview tables: unknown exception\n");
		g_chestPreviewLoaded = false;
		g_chestPreviewTables.clear();
	}
}

bool CanCharacterPreviewChest(CCharacter* pCha, int chestItemID) {
	if (!pCha || chestItemID <= 0 || chestItemID > USHRT_MAX) {
		return false;
	}

	if (!GetItemRecordInfo(chestItemID)) {
		return false;
	}

	// Require ownership to avoid exposing hidden/event chest drop tables.
	return pCha->HasItem(static_cast<USHORT>(chestItemID), 1) == TRUE;
}

void SendChestPreviewPacket(CCharacter* pCha, int chestItemID) {
	if (!pCha || chestItemID <= 0) {
		return;
	}

	LoadChestPreviewTables();

	WPACKET packet = GETWPACKET();
	WRITE_CMD(packet, CMD_MC_CHEST_PREVIEW);
	packet.WriteLong(chestItemID);

	if (!CanCharacterPreviewChest(pCha, chestItemID)) {
		packet.WriteShort(0);
		packet.WriteLong(0);
		pCha->ReflectINFof(pCha, packet);
		return;
	}

	auto tableIt = g_chestPreviewTables.find(chestItemID);
	if (tableIt == g_chestPreviewTables.end()) {
		packet.WriteShort(0);
		packet.WriteLong(0);
		pCha->ReflectINFof(pCha, packet);
		return;
	}

	const SChestPreviewTable& table = tableIt->second;
	int safeCount = static_cast<int>(table.entries.size());
	if (safeCount > 32767) {
		safeCount = 32767;
	}
	short entryCount = static_cast<short>(safeCount);
	packet.WriteShort(entryCount);
	packet.WriteLong(table.totalWeight);

	for (short i = 0; i < entryCount; ++i) {
		const SChestPreviewEntry& entry = table.entries[i];
		packet.WriteLong(entry.itemID);
		packet.WriteLong(entry.quantity);
		packet.WriteLong(entry.weight);
	}

	pCha->ReflectINFof(pCha, packet);
}

} // namespace

namespace {

bool CharTradeRateLimitOk(CCharacter& cha) {
	if (IsEconomyBlockedByDB(cha))
		return false;
	DWORD dwNow = (DWORD)GetTickCount64();
	if (dwNow - cha.m_dwLastTradePacketTime < 200)
		return false;
	cha.m_dwLastTradePacketTime = dwNow;
	return true;
}

void HandleStoreOperate(CCharacter* cha, RPacket& pk, uint16_t usCmd) {
	CCharacter* pMainCha = cha->GetPlyMainCha();
	if (!pMainCha->IsStoreEnable()) {
		return;
	}
	lua_getglobal(g_pLuaState, "operateIGS");
	if (!lua_isfunction(g_pLuaState, -1)) {
		lua_pop(g_pLuaState, 1);
		return;
	}

	lua_pushlightuserdata(g_pLuaState, (void*)cha);
	lua_pushlightuserdata(g_pLuaState, (void*)&pk);
	int nStatus = lua_pcall(g_pLuaState, 2, 0, 0);
	lua_settop(g_pLuaState, 0);

	if (usCmd == CMD_CM_STORE_CLOSE) {
		pMainCha->SetStoreEnable(false);
		pMainCha->ForgeAction(false);
	}
}

void HandlePmGuildBank(CCharacter* cha, RPacket& pk) {
	net::PacketReader reader(pk);
	uChar bankTypeRaw = 0;
	if (!reader.Char(bankTypeRaw)) {
		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MP_GUILDBANK);
		WRITE_LONG(WtPk, cha->GetGuildID());
		cha->ReflectINFof(cha, WtPk);
		return;
	}
	Char bankType = static_cast<Char>(bankTypeRaw);

	// Rate limit guild bank DB operations (1s cooldown)
	// NOTE: must NOT break here - ack must always be sent or GroupServer queue gets permanently stuck
	bool bRateLimited = false;
	{
		DWORD dwNow = (DWORD)GetTickCount64();
		if (dwNow - cha->m_dwLastGuildBankTime < 1000) {
			bRateLimited = true;
		} else {
			cha->m_dwLastGuildBankTime = dwNow;
		}
	}

	// Check if player is in a safezone (enumAREA_TYPE_NOT_FIGHT = 0x0002)
	if (bRateLimited) {
		// silently skip processing but still fall through to send ack
	} else if (IsEconomyBlockedByDB(*cha)) {
		// notice already shown
	} else if (!cha->IsLiveing()) {
		cha->SystemNotice("Dead pirates are unable to trade.");
	} else if (!cha->IsInArea(enumAREA_TYPE_NOT_FIGHT)) {
		cha->SystemNotice("Must be in a safe zone to use the guild bank.");
	} else if (!cha->IsGuildBankOpen()) {
		cha->SystemNotice("You must open the guild bank interface first.");
	} else {
		switch (bankType) {

		case 0: { // bankoper
			uChar chSrcTypeRaw = 0;
			uChar chTarTypeRaw = 0;
			uShort sSrcGrid = 0;
			uShort sSrcNum = 0;
			uShort sTarGrid = 0;
			if (!reader.Char(chSrcTypeRaw) || !reader.Short(sSrcGrid) || !reader.Short(sSrcNum) ||
				!reader.Char(chTarTypeRaw) || !reader.Short(sTarGrid)) {
				break;
			}
			Char chSrcType = static_cast<Char>(chSrcTypeRaw);
			Short sSrcGridS = static_cast<Short>(sSrcGrid);
			Short sSrcNumS = static_cast<Short>(sSrcNum);
			Char chTarType = static_cast<Char>(chTarTypeRaw);
			Short sTarGridS = static_cast<Short>(sTarGrid);
			Short sRet;
			int guildID = cha->GetGuildID();
			std::vector<CTableGuild::BankLog> logs = game_db.GetGuildLog(guildID);

			if (chTarType != chSrcType) {

				CTableGuild::BankLog l;
				CKitbag bag;

				l.time = time(0);
				l.quantity = sSrcNumS;
				l.userID = cha->GetPlyMainCha()->m_ID;

				if (chTarType == 0) {
					game_db.GetGuildBank(guildID, &bag);
					l.type = 2;
				} else if (chTarType == 1) {
					bag = cha->GetPlyMainCha()->m_CKitbag;
					l.type = 3;
				}
				l.parameter = bag.GetID(sSrcGridS);
				logs.push_back(l);
			}
			sRet = cha->Cmd_GuildBankOper(chSrcType, sSrcGridS, sSrcNumS, chTarType, sTarGridS);
			if (sRet != enumITEMOPT_SUCCESS || !game_db.SetGuildLog(logs, guildID)) {
				cha->ItemOprateFailed(sRet);
			}

			break;
		}

		case 1: { // withdraw/deposit gold
			uChar actionRaw = 0;
			unsigned long long goldRaw = 0;
			if (!reader.Char(actionRaw) || !reader.LongLong(goldRaw)) {
				break;
			}
			Char action = static_cast<Char>(actionRaw);
			long long gold = static_cast<long long>(goldRaw);

			int guildID = cha->GetGuildID();

			// SANITIZE: Validate gold amount before processing
			if (gold < 0) {
				LG("Security", "[GuildBank] Negative gold %lld from character %s (ID:%d) - exploit attempt blocked\n",
					gold, cha->GetName(), cha->m_ID);
				break;
			}
			if (!PS::ValidateGold(gold)) {
				LG("Security", "[GuildBank] Invalid gold amount %lld from character %s (ID:%d)\n",
					gold, cha->GetName(), cha->m_ID);
				break;
			}
			// SANITIZE: Validate action type
			if (action != 0 && action != 1) {
				LG("Security", "[GuildBank] Invalid action %d from character %s\n", action, cha->GetName());
				break;
			}

			long long originalGold = gold;  // Store original request for messages
			__int64 currentgold = cha->getAttr(ATTR_GD);
			unsigned long long guildGold = game_db.GetGuildBankGold(guildID);

			unsigned long long maxGuildGold = 100000000000LL;  // 100 billion cap
			__int64 maxCharGold = 100000000000LL;
			std::vector<CTableGuild::BankLog> logs = game_db.GetGuildLog(guildID);

			int canTake = (emGldPermTakeBank & cha->guildPermission);
			int canGive = (emGldPermDepoBank & cha->guildPermission);

			CTableGuild::BankLog l;

			if (action == 0 && canTake == emGldPermTakeBank) { // withdraw
				l.type = 0;									   // Withdraw

				// Check if character is already at max gold
				if (currentgold >= maxCharGold) {
					cha->SystemNotice("Unable to withdraw: Your inventory gold is at the maximum limit (100 billion).");
					break;
				}
				// make sure we dont cause gold overflow.
				if (gold + currentgold > maxCharGold) {
					gold = maxCharGold - currentgold;
					char msg[256];
					_snprintf_s(msg, sizeof(msg), _TRUNCATE,
						"Withdrawal capped: You can only receive %lld gold (inventory would exceed 100 billion limit).", gold);
					cha->SystemNotice(msg);
				}
				// make sure we cant withdraw more than is in bank.
				if (gold > (__int64)guildGold) {
					gold = guildGold;
					if (gold < originalGold) {
						char msg[256];
						_snprintf_s(msg, sizeof(msg), _TRUNCATE,
							"Withdrawal adjusted: Guild vault only has %lld gold available.", gold);
						cha->SystemNotice(msg);
					}
				}
				// we dont want to do redundant transactions.
				if (gold < 1) {
					cha->SystemNotice("Unable to withdraw: No gold available to withdraw.");
					break;
				}
			} else if (action == 1 && canGive == emGldPermDepoBank) { // deposit
				l.type = 1;											  // deposit

				// Check if guild vault is already at max
				if (guildGold >= maxGuildGold) {
					cha->SystemNotice("Unable to deposit: Guild vault is at the maximum limit (100 billion).");
					break;
				}
				// check player has that much gold
				// if not, then set gold to whatever they have.
				if (gold > currentgold) {
					gold = currentgold;
					if (gold < originalGold && gold > 0) {
						char msg[256];
						_snprintf_s(msg, sizeof(msg), _TRUNCATE,
							"Deposit adjusted: You only have %lld gold available.", gold);
						cha->SystemNotice(msg);
					}
				}
				// check to see if guild is at max gold already.
				// make sure we dont cause gold overflow.
				if (gold + (__int64)guildGold > (__int64)maxGuildGold) {
					gold = maxGuildGold - guildGold;
					char msg[256];
					_snprintf_s(msg, sizeof(msg), _TRUNCATE,
						"Deposit capped: Guild vault can only accept %lld more gold (would exceed 100 billion limit).", gold);
					cha->SystemNotice(msg);
				}

				// we dont want to do redundant transactions.
				if (gold < 1) {
					cha->SystemNotice("Unable to deposit: No gold to deposit or vault is full.");
					break;
				}
				gold = 0 - gold;
			} else {
				break;
			}

			// TRANSACTION: Wrap gold transfer in transaction for atomicity
			game_db.BeginTran();

			if (game_db.UpdateGuildBankGold(guildID, -gold)) {
				l.time = time(0);
				l.parameter = gold > 0 ? gold : -gold;
				l.quantity = 0;
				l.userID = cha->GetPlyMainCha()->m_ID;

				logs.push_back(l);
				if (game_db.SetGuildLog(logs, guildID)) {
					cha->setAttr(ATTR_GD, currentgold + gold);

					// Save player gold immediately to ensure atomicity
					if (cha->GetPlyMainCha()->SaveAssets()) {
						game_db.CommitTran();

						cha->SynAttr(enumATTRSYN_TRADE);
						cha->SyncBoatAttr(enumATTRSYN_TRADE);

						// send update packet to let other members of guild see the update.
						WPACKET WtPk = GETWPACKET();
						WRITE_CMD(WtPk, CMD_MM_UPDATEGUILDBANKGOLD);
						WRITE_LONG(WtPk, cha->GetPlyMainCha()->m_ID);
						WRITE_LONG(WtPk, cha->GetPlyMainCha()->GetGuildID());
						cha->ReflectINFof(cha, WtPk);
					} else {
						// Rollback and restore player gold
						game_db.RollBack();
						cha->setAttr(ATTR_GD, currentgold);
						LG("bank_error", "Failed to save player assets during gold transfer: player=%s guild=%d gold=%lld\n",
						   cha->GetPlyMainCha()->GetName(), guildID, gold);
					}
				} else {
					game_db.RollBack();
					LG("bank_error", "Failed to set guild log during gold transfer: player=%s guild=%d\n",
					   cha->GetPlyMainCha()->GetName(), guildID);
				}
			} else {
				game_db.RollBack();
				LG("bank_error", "Failed to update guild bank gold: player=%s guild=%d gold=%lld\n",
				   cha->GetPlyMainCha()->GetName(), guildID, gold);
			}
			break;
		}
		}
	}

	// let group know we have finished, so the next guild bank packet can be processed.
	WPACKET WtPk = GETWPACKET();
	WRITE_CMD(WtPk, CMD_MP_GUILDBANK);
	WRITE_LONG(WtPk, cha->GetGuildID());
	cha->ReflectINFof(cha, WtPk);
}

void RegisterAllCharacterOpcodeHandlers() {
	std::vector<OpcodeHandlerEntry> entries;
	entries.reserve(112);

	auto add = [&](uint16_t op, OpcodeHandlerFn fn, const char* name, uint16_t minPayload = 0) {
		entries.push_back({op, fn, name, minPayload});
	};

	add(CMD_CM_BOSSTIMER_REQUEST, &CCharacter::OpcodeHandle_CmBossTimerRequest, OpcodeName(CMD_CM_BOSSTIMER_REQUEST));
	add(CMD_CM_RANK, &CCharacter::OpcodeHandle_CmRank, OpcodeName(CMD_CM_RANK));
	add(CMD_CM_CANCELEXIT, &CCharacter::OpcodeHandle_CmCancelExit, OpcodeName(CMD_CM_CANCELEXIT));
	add(CMD_CM_CHECK_PING, &CCharacter::OpcodeHandle_CmCheckPing, OpcodeName(CMD_CM_CHECK_PING), 0);
	add(CMD_CM_ENDACTION, &CCharacter::OpcodeHandle_CmEndAction, OpcodeName(CMD_CM_ENDACTION), 0);
	add(CMD_CM_DIE_RETURN, &CCharacter::OpcodeHandle_CmDieReturn, OpcodeName(CMD_CM_DIE_RETURN), 1);
	add(CMD_CM_MISLOG, &CCharacter::OpcodeHandle_CmMisLog, OpcodeName(CMD_CM_MISLOG));
	add(CMD_CM_MISLOGINFO, &CCharacter::OpcodeHandle_CmMisLogInfo, OpcodeName(CMD_CM_MISLOGINFO), 2);
	add(CMD_CM_MISLOG_CLEAR, &CCharacter::OpcodeHandle_CmMisLogClear, OpcodeName(CMD_CM_MISLOG_CLEAR), 2);
	add(CMD_CM_MAP_MASK, &CCharacter::OpcodeHandle_CmMapMask, OpcodeName(CMD_CM_MAP_MASK));
	add(CMD_CM_SAY, &CCharacter::OpcodeHandle_CmSay, OpcodeName(CMD_CM_SAY));
	add(CMD_CM_STALLSEARCH, &CCharacter::OpcodeHandle_CmStallSearch, OpcodeName(CMD_CM_STALLSEARCH), 4);
	add(CMD_CM_SYNATTR, &CCharacter::OpcodeHandle_CmSynAttr, OpcodeName(CMD_CM_SYNATTR), 0);
	add(CMD_CM_REFRESH_DATA, &CCharacter::OpcodeHandle_CmRefreshData, OpcodeName(CMD_CM_REFRESH_DATA), 8);
	add(CMD_CM_READBOOK_START, &CCharacter::OpcodeHandle_CmReadbookStart, OpcodeName(CMD_CM_READBOOK_START));
	add(CMD_CM_READBOOK_CLOSE, &CCharacter::OpcodeHandle_CmReadbookClose, OpcodeName(CMD_CM_READBOOK_CLOSE));
	add(CMD_CM_KITBAG_CHECK, &CCharacter::OpcodeHandle_CmKitbagCheck, OpcodeName(CMD_CM_KITBAG_CHECK), 0);
	add(CMD_CM_KITBAG_UNLOCK, &CCharacter::OpcodeHandle_CmKitbagUnlock, OpcodeName(CMD_CM_KITBAG_UNLOCK), 0);
	add(CMD_CM_BOAT_GETINFO, &CCharacter::OpcodeHandle_CmBoatGetinfo, OpcodeName(CMD_CM_BOAT_GETINFO), 0);
	add(CMD_CM_STALL_ALLDATA, &CCharacter::OpcodeHandle_CmStallAlldata, OpcodeName(CMD_CM_STALL_ALLDATA), 0);
	add(CMD_CM_BEGINACTION, &CCharacter::OpcodeHandle_CmBeginAction, OpcodeName(CMD_CM_BEGINACTION), 5);
	add(CMD_CM_FORGE, &CCharacter::OpcodeHandle_CmForge, OpcodeName(CMD_CM_FORGE), 1);
	add(CMD_CM_BOAT_CANCEL, &CCharacter::OpcodeHandle_CmBoatCancel, OpcodeName(CMD_CM_BOAT_CANCEL));
	add(CMD_CM_CREATE_BOAT, &CCharacter::OpcodeHandle_CmCreateBoat, OpcodeName(CMD_CM_CREATE_BOAT));
	add(CMD_CM_UPDATEBOAT_PART, &CCharacter::OpcodeHandle_CmUpdateboatPart, OpcodeName(CMD_CM_UPDATEBOAT_PART));
	add(CMD_CM_STALL_OPEN, &CCharacter::OpcodeHandle_CmStallOpen, OpcodeName(CMD_CM_STALL_OPEN), 4);
	add(CMD_CM_STALL_CLOSE, &CCharacter::OpcodeHandle_CmStallClose, OpcodeName(CMD_CM_STALL_CLOSE));
	add(CMD_CM_KITBAG_AUTOLOCK, &CCharacter::OpcodeHandle_CmKitbagAutolock, OpcodeName(CMD_CM_KITBAG_AUTOLOCK), 1);
	add(CMD_CM_KITBAG_LOCK, &CCharacter::OpcodeHandle_CmKitbagLock, OpcodeName(CMD_CM_KITBAG_LOCK), 0);
	add(CMD_CM_UPDATEHAIR, &CCharacter::OpcodeHandle_CmUpdatehair, OpcodeName(CMD_CM_UPDATEHAIR), 2);
	add(CMD_CM_SKILLUPGRADE, &CCharacter::OpcodeHandle_CmSkillupgrade, OpcodeName(CMD_CM_SKILLUPGRADE), 3);
	add(CMD_CM_TEAM_FIGHT_ASK, &CCharacter::OpcodeHandle_CmTeamFightAsk, OpcodeName(CMD_CM_TEAM_FIGHT_ASK), 9);
	add(CMD_CM_TEAM_FIGHT_ASR, &CCharacter::OpcodeHandle_CmTeamFightAsr, OpcodeName(CMD_CM_TEAM_FIGHT_ASR), 1);
	add(CMD_CM_ITEM_REPAIR_ASK, &CCharacter::OpcodeHandle_CmItemRepairAsk, OpcodeName(CMD_CM_ITEM_REPAIR_ASK), 10);
	add(CMD_CM_ITEM_REPAIR_ASR, &CCharacter::OpcodeHandle_CmItemRepairAsr, OpcodeName(CMD_CM_ITEM_REPAIR_ASR), 1);
	add(CMD_CM_STALL_BUY, &CCharacter::OpcodeHandle_CmStallBuy, OpcodeName(CMD_CM_STALL_BUY), 7);
	add(CMD_CM_BOAT_LUANCH, &CCharacter::OpcodeHandle_CmBoatLuanch, OpcodeName(CMD_CM_BOAT_LUANCH), 5);
	add(CMD_CM_BOAT_BAGSEL, &CCharacter::OpcodeHandle_CmBoatBagsel, OpcodeName(CMD_CM_BOAT_BAGSEL), 5);
	add(CMD_CM_BOAT_SELECT, &CCharacter::OpcodeHandle_CmBoatSelect, OpcodeName(CMD_CM_BOAT_SELECT), 6);
	add(CMD_CM_ENTITY_EVENT, &CCharacter::OpcodeHandle_CmEntityEvent, OpcodeName(CMD_CM_ENTITY_EVENT), 4);
	add(CMD_CM_ITEM_FORGE_CANACTION, &CCharacter::OpcodeHandle_CmItemForgeCanaction, OpcodeName(CMD_CM_ITEM_FORGE_CANACTION), 1);
	add(CMD_CM_VALIDATE_SLOT_ITEM, &CCharacter::OpcodeHandle_CmValidateSlotItem, OpcodeName(CMD_CM_VALIDATE_SLOT_ITEM), 4);
	add(CMD_CM_ITEM_FORGE_ASK, &CCharacter::OpcodeHandle_CmItemForgeAsk, OpcodeName(CMD_CM_ITEM_FORGE_ASK), 1);
	add(CMD_CM_ITEM_FORGE_ASR, &CCharacter::OpcodeHandle_CmItemForgeAsr, OpcodeName(CMD_CM_ITEM_FORGE_ASR), 1);
	add(CMD_CM_ITEM_LOTTERY_ASK, &CCharacter::OpcodeHandle_CmItemLotteryAsk, OpcodeName(CMD_CM_ITEM_LOTTERY_ASK), 1);
	add(CMD_CM_LIFESKILL_ASK, &CCharacter::OpcodeHandle_CmLifeskillAsk, OpcodeName(CMD_CM_LIFESKILL_ASK), 8);
	add(CMD_CM_LIFESKILL_ASR, &CCharacter::OpcodeHandle_CmLifeskillAsr, OpcodeName(CMD_CM_LIFESKILL_ASR), 8);
	add(CMD_CM_KITBAG_EXPAND, &CCharacter::OpcodeHandle_CmKitbagExpand, OpcodeName(CMD_CM_KITBAG_EXPAND));
	add(CMD_CM_PING, &CCharacter::OpcodeHandle_CmPing, OpcodeName(CMD_CM_PING), 28);
	add(CMD_CM_TIGER_START, &CCharacter::OpcodeHandle_CmTigerStart, OpcodeName(CMD_CM_TIGER_START), 10);
	add(CMD_CM_TIGER_STOP, &CCharacter::OpcodeHandle_CmTigerStop, OpcodeName(CMD_CM_TIGER_STOP), 6);
	add(CMD_CM_KITBAGTEMP_SYNC, &CCharacter::OpcodeHandle_CmKitbagtempSync, OpcodeName(CMD_CM_KITBAGTEMP_SYNC), 0);
	add(CMD_CM_STORE_OPEN_ASK, &CCharacter::OpcodeHandle_CmStoreOpenAsk, OpcodeName(CMD_CM_STORE_OPEN_ASK), 0);
	add(CMD_CM_STORE_LIST_ASK, &CCharacter::OpcodeHandle_CmStoreListAsk, OpcodeName(CMD_CM_STORE_LIST_ASK), 0);
	add(CMD_CM_STORE_BUY_ASK, &CCharacter::OpcodeHandle_CmStoreBuyAsk, OpcodeName(CMD_CM_STORE_BUY_ASK), 0);
	add(CMD_CM_STORE_CHANGE_ASK, &CCharacter::OpcodeHandle_CmStoreChangeAsk, OpcodeName(CMD_CM_STORE_CHANGE_ASK), 0);
	add(CMD_CM_STORE_QUERY, &CCharacter::OpcodeHandle_CmStoreQuery, OpcodeName(CMD_CM_STORE_QUERY), 0);
	add(CMD_CM_STORE_VIP, &CCharacter::OpcodeHandle_CmStoreVip, OpcodeName(CMD_CM_STORE_VIP), 0);
	add(CMD_CM_STORE_CLOSE, &CCharacter::OpcodeHandle_CmStoreClose, OpcodeName(CMD_CM_STORE_CLOSE), 0);
	add(CMD_CM_REQUESTTALK, &CCharacter::OpcodeHandle_CmRequestTalkOrTrade, OpcodeName(CMD_CM_REQUESTTALK), 4);
	add(CMD_CM_REQUESTTRADE, &CCharacter::OpcodeHandle_CmRequestTalkOrTrade, OpcodeName(CMD_CM_REQUESTTRADE), 4);
	add(CMD_CM_ITEM_LOCK_ASK, &CCharacter::OpcodeHandle_CmItemLockAsk, OpcodeName(CMD_CM_ITEM_LOCK_ASK), 1);
	add(CMD_CM_ITEM_UNLOCK_ASK, &CCharacter::OpcodeHandle_CmItemUnlockAsk, OpcodeName(CMD_CM_ITEM_UNLOCK_ASK), 0);
	add(CMD_CM_GAME_REQUEST_PIN, &CCharacter::OpcodeHandle_CmGameRequestPin, OpcodeName(CMD_CM_GAME_REQUEST_PIN), 0);
	add(CMD_CM_CHARTRADE_REQUEST, &CCharacter::OpcodeHandle_CmChartradeRequest, OpcodeName(CMD_CM_CHARTRADE_REQUEST), 5);
	add(CMD_CM_CHARTRADE_ACCEPT, &CCharacter::OpcodeHandle_CmChartradeAccept, OpcodeName(CMD_CM_CHARTRADE_ACCEPT), 5);
	add(CMD_CM_CHARTRADE_REJECT, &CCharacter::OpcodeHandle_CmChartradeReject, OpcodeName(CMD_CM_CHARTRADE_REJECT), 0);
	add(CMD_CM_CHARTRADE_CANCEL, &CCharacter::OpcodeHandle_CmChartradeCancel, OpcodeName(CMD_CM_CHARTRADE_CANCEL), 5);
	add(CMD_CM_CHARTRADE_ITEM, &CCharacter::OpcodeHandle_CmChartradeItem, OpcodeName(CMD_CM_CHARTRADE_ITEM), 9);
	add(CMD_CM_CHARTRADE_MONEY, &CCharacter::OpcodeHandle_CmChartradeMoney, OpcodeName(CMD_CM_CHARTRADE_MONEY), 7);
	add(CMD_CM_CHARTRADE_VALIDATEDATA, &CCharacter::OpcodeHandle_CmChartradeValidatedata, OpcodeName(CMD_CM_CHARTRADE_VALIDATEDATA), 5);
	add(CMD_CM_CHARTRADE_VALIDATE, &CCharacter::OpcodeHandle_CmChartradeValidate, OpcodeName(CMD_CM_CHARTRADE_VALIDATE), 5);
	add(CMD_CM_VOLUNTER_OPEN, &CCharacter::OpcodeHandle_CmVolunterOpen, OpcodeName(CMD_CM_VOLUNTER_OPEN), 2);
	add(CMD_CM_VOLUNTER_LIST, &CCharacter::OpcodeHandle_CmVolunterList, OpcodeName(CMD_CM_VOLUNTER_LIST), 4);
	add(CMD_CM_VOLUNTER_ADD, &CCharacter::OpcodeHandle_CmVolunterAdd, OpcodeName(CMD_CM_VOLUNTER_ADD), 0);
	add(CMD_CM_VOLUNTER_DEL, &CCharacter::OpcodeHandle_CmVolunterDel, OpcodeName(CMD_CM_VOLUNTER_DEL), 0);
	add(CMD_CM_VOLUNTER_SEL, &CCharacter::OpcodeHandle_CmVolunterSel, OpcodeName(CMD_CM_VOLUNTER_SEL), 0);
	add(CMD_CM_VOLUNTER_ASR, &CCharacter::OpcodeHandle_CmVolunterAsr, OpcodeName(CMD_CM_VOLUNTER_ASR), 2);
	add(CMD_CM_MASTER_INVITE, &CCharacter::OpcodeHandle_CmMasterInvite, OpcodeName(CMD_CM_MASTER_INVITE), 4);
	add(CMD_CM_MASTER_ASR, &CCharacter::OpcodeHandle_CmMasterAsr, OpcodeName(CMD_CM_MASTER_ASR), 2);
	add(CMD_CM_MASTER_DEL, &CCharacter::OpcodeHandle_CmMasterDel, OpcodeName(CMD_CM_MASTER_DEL), 4);
	add(CMD_CM_PRENTICE_DEL, &CCharacter::OpcodeHandle_CmPrenticeDel, OpcodeName(CMD_CM_PRENTICE_DEL), 4);
	add(CMD_CM_PRENTICE_INVITE, &CCharacter::OpcodeHandle_CmPrenticeInvite, OpcodeName(CMD_CM_PRENTICE_INVITE), 4);
	add(CMD_CM_PRENTICE_ASR, &CCharacter::OpcodeHandle_CmPrenticeAsr, OpcodeName(CMD_CM_PRENTICE_ASR), 2);
	add(CMD_PM_GUILDBANK, &CCharacter::OpcodeHandle_PmGuildbank, OpcodeName(CMD_PM_GUILDBANK), 1);
	add(CMD_PM_PUSHTOGUILDBANK, &CCharacter::OpcodeHandle_PmPushToGuildbank, OpcodeName(CMD_PM_PUSHTOGUILDBANK), 0);
	add(CMD_TM_CHANGE_PERSONINFO, &CCharacter::OpcodeHandle_TmChangePersoninfo, OpcodeName(CMD_TM_CHANGE_PERSONINFO), 2);
	add(CMD_CM_GUILD_PERM, &CCharacter::OpcodeHandle_CmGuildPerm, OpcodeName(CMD_CM_GUILD_PERM), 8);
	add(CMD_CM_GUILD_PUTNAME, &CCharacter::OpcodeHandle_CmGuildPutname, OpcodeName(CMD_CM_GUILD_PUTNAME), 1);
	add(CMD_CM_GUILD_TRYFOR, &CCharacter::OpcodeHandle_CmGuildTryfor, OpcodeName(CMD_CM_GUILD_TRYFOR), 4);
	add(CMD_CM_GUILD_TRYFORCFM, &CCharacter::OpcodeHandle_CmGuildTryforcfm, OpcodeName(CMD_CM_GUILD_TRYFORCFM), 1);
	add(CMD_CM_GUILD_LISTTRYPLAYER, &CCharacter::OpcodeHandle_CmGuildListtryplayer, OpcodeName(CMD_CM_GUILD_LISTTRYPLAYER));
	add(CMD_CM_GUILD_APPROVE, &CCharacter::OpcodeHandle_CmGuildApprove, OpcodeName(CMD_CM_GUILD_APPROVE), 4);
	add(CMD_CM_GUILD_REJECT, &CCharacter::OpcodeHandle_CmGuildReject, OpcodeName(CMD_CM_GUILD_REJECT), 4);
	add(CMD_CM_GUILD_KICK, &CCharacter::OpcodeHandle_CmGuildKick, OpcodeName(CMD_CM_GUILD_KICK), 4);
	add(CMD_CM_GUILD_LEAVE, &CCharacter::OpcodeHandle_CmGuildLeave, OpcodeName(CMD_CM_GUILD_LEAVE));
	add(CMD_CM_GUILD_DISBAND, &CCharacter::OpcodeHandle_CmGuildDisband, OpcodeName(CMD_CM_GUILD_DISBAND), 0);
	add(CMD_CM_GUILD_MOTTO, &CCharacter::OpcodeHandle_CmGuildMotto, OpcodeName(CMD_CM_GUILD_MOTTO), 0);
	add(CMD_PM_GUILD_DISBAND, &CCharacter::OpcodeHandle_PmGuildDisband, OpcodeName(CMD_PM_GUILD_DISBAND));
	add(CMD_CM_GUILD_CHALLENGE, &CCharacter::OpcodeHandle_CmGuildChallenge, OpcodeName(CMD_CM_GUILD_CHALLENGE), 5);
	add(CMD_CM_GUILD_LEIZHU, &CCharacter::OpcodeHandle_CmGuildLeizhu, OpcodeName(CMD_CM_GUILD_LEIZHU), 5);
	add(CMD_CM_SAY2CAMP, &CCharacter::OpcodeHandle_CmSay2camp, OpcodeName(CMD_CM_SAY2CAMP), 0);
	add(CMD_CM_GM_SEND, &CCharacter::OpcodeHandle_CmGmSend, OpcodeName(CMD_CM_GM_SEND), 4);
	add(CMD_CM_GM_RECV, &CCharacter::OpcodeHandle_CmGmRecv, OpcodeName(CMD_CM_GM_RECV), 4);
	add(CMD_CM_PK_CTRL, &CCharacter::OpcodeHandle_CmPkCtrl, OpcodeName(CMD_CM_PK_CTRL), 1);
	add(CMD_CM_CHEAT_CHECK, &CCharacter::OpcodeHandle_CmCheatCheck, OpcodeName(CMD_CM_CHEAT_CHECK), 0);
	add(CMD_CM_BIDUP, &CCharacter::OpcodeHandle_CmBidup, OpcodeName(CMD_CM_BIDUP), 0);
	add(CMD_CM_ANTIINDULGENCE, &CCharacter::OpcodeHandle_CmAntiindulgence, OpcodeName(CMD_CM_ANTIINDULGENCE));
	add(CMD_CM_REQUEST_DROP_RATE, &CCharacter::OpcodeHandle_CmRequestDropRate, OpcodeName(CMD_CM_REQUEST_DROP_RATE));
	add(CMD_CM_REQUEST_EXP_RATE, &CCharacter::OpcodeHandle_CmRequestExpRate, OpcodeName(CMD_CM_REQUEST_EXP_RATE));
	add(CMD_CM_GET_PLAYER_BATTLE_POINT, &CCharacter::OpcodeHandle_CmGetPlayerBattlePoint, OpcodeName(CMD_CM_GET_PLAYER_BATTLE_POINT));
	add(CMD_CM_REQUEST_CHEST_PREVIEW, &CCharacter::OpcodeHandle_CmRequestChestPreview, OpcodeName(CMD_CM_REQUEST_CHEST_PREVIEW), 4);

	if (!RegisterOpcodeHandlers(OpcodeDispatchDomain::GameCharacter, entries.data(), entries.size())) {
		throw std::runtime_error("RegisterAllCharacterOpcodeHandlers failed");
	}

	printf("Character opcode registry: %zu handlers\n", OpcodeHandlerCount(OpcodeDispatchDomain::GameCharacter));
}

} // namespace

void RegisterCharacterOpcodeHandlers() {
	static bool registered = false;
	if (registered) {
		return;
	}
	RegisterAllCharacterOpcodeHandlers();
	registered = true;
}

bool CCharacter::OpcodeHandle_CmBossTimerRequest(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	BossTimer::SendToPlayer(cha);
	return true;
}

bool CCharacter::OpcodeHandle_CmRank(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	const DWORD cooldown = GetTickCount();
	if (cha->ShowRankColD > cooldown) {
		cha->BickerNotice("Please Calm Down Don't Spam! ");
		return true;
	}
	game_db.ShowExpRank(cha->GetPlyMainCha(), 50);
	return true;
}

bool CCharacter::OpcodeHandle_CmCancelExit(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	static_cast<CCharacter*>(ctx)->CancelExit();
	return true;
}

bool CCharacter::OpcodeHandle_CmCheckPing(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	const DWORD dwPing = GetTickCount() - cha->m_dwPingSendTick;
	cha->m_dwPing = dwPing;
	cha->SendPreMoveTime();
	return true;
}

bool CCharacter::OpcodeHandle_CmEndAction(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	static_cast<CCharacter*>(ctx)->EndAction(pk);
	return true;
}

bool CCharacter::OpcodeHandle_CmDieReturn(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uChar relive = 0;
	if (!reader.Char(relive)) {
		return true;
	}
	cha->m_chSelRelive = relive;
	cha->GetPlyMainCha()->ResetChaRelive();
	if (cha->m_chSelRelive == enumEPLAYER_RELIVE_NORIGIN) {
		cha->SetRelive(enumEPLAYER_RELIVE_ORIGIN, 0);
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmMisLog(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	static_cast<CCharacter*>(ctx)->MisLog();
	return true;
}

bool CCharacter::OpcodeHandle_CmMisLogInfo(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	net::PacketReader reader(pk);
	uShort wMisID = 0;
	if (!reader.Short(wMisID)) {
		return true;
	}
	static_cast<CCharacter*>(ctx)->MisLogInfo(wMisID);
	return true;
}

bool CCharacter::OpcodeHandle_CmMisLogClear(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	net::PacketReader reader(pk);
	uShort wMisID = 0;
	if (!reader.Short(wMisID)) {
		return true;
	}
	static_cast<CCharacter*>(ctx)->MisLogClear(wMisID);
	return true;
}

bool CCharacter::OpcodeHandle_CmMapMask(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	if (!cha->GetSubMap()) {
		return true;
	}

	int lDataLen;
	BYTE* pData = cha->GetPlayer()->GetMapMask(lDataLen);
	WPACKET wpk = GETWPACKET();
	WRITE_CMD(wpk, CMD_MC_MAP_MASK);
	WRITE_LONG(wpk, cha->m_ID);
	if (!pData) {
		WRITE_CHAR(wpk, 0);
	} else {
		WRITE_CHAR(wpk, 1);
		WRITE_SEQ(wpk, (cChar*)pData, (uShort)lDataLen);
	}
	cha->ReflectINFof(cha, wpk);
	return true;
}

bool CCharacter::OpcodeHandle_CmSay(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	DWORD dwNowTick = GetTickCount();
	if (dwNowTick - cha->_dwLastSayTick < (DWORD)g_Config.m_lSayInterval) {
		cha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00001));
		return true;
	}
	cha->_dwLastSayTick = dwNowTick;

	if (!cha->GetSubMap()) {
		LG("dialog error", "when character%s is dialog,the map is null!\n", cha->m_CLog.GetLogName());
		return true;
	}
	uShort l_retlen = 0;
	net::PacketReader reader(pk);
	cChar* l_content = reader.Raw().ReadSequence(l_retlen);
	if (!l_content)
		return true;

	if (l_retlen == 0 || l_retlen > PS::MAX_CHAT_LENGTH) {
		LG("Security", "[Chat] Invalid message length %d from character %s\n", l_retlen, cha->GetName());
		return true;
	} else if (*l_content == '&') {
		Char chGMLv = cha->GetPlayer()->GetGMLev();
		if (chGMLv == 0 || chGMLv > 150)
			cha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00002));
		else
			cha->DoCommand(l_content + 1, l_retlen - 1);
	} else if (*l_content == '$' && *(l_content + 1) == '$') {
		cha->DoCommand_CheckStatus(l_content + 3, l_retlen - 2);
	} else {
		g_CParser.DoString("HandleChat", enumSCRIPT_RETURN_NUMBER, 1, enumSCRIPT_PARAM_LIGHTUSERDATA, 1, cha, enumSCRIPT_PARAM_STRING, 1, l_content, DOSTRING_PARAM_END);
		if (!g_CParser.GetReturnNumber(0))
			return true;
		if (g_Config.m_bBlindChaos && cha->IsPlayerCha() && cha->IsPKSilver()) {
			cha->SystemNotice("Unable to chat in this map!");
			return true;
		}

		WPACKET wpk = GETWPACKET();
		WRITE_CMD(wpk, CMD_MC_SAY);
		WRITE_LONG(wpk, cha->m_ID);
		WRITE_SEQ(wpk, l_content, l_retlen);
		WRITE_LONG(wpk, cha->chatColour);
		cha->NotiChgToEyeshot(wpk);
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmStallSearch(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	DWORD dwNow = GetTickCount();
	if (dwNow - cha->m_dwLastStallSearchTime < 2000) {
		cha->SystemNotice("Please wait before searching again.");
		return true;
	}
	cha->m_dwLastStallSearchTime = dwNow;

	net::PacketReader reader(pk);
	uLong itemID = 0;
	if (!reader.Long(itemID)) {
		return true;
	}
	g_StallSystem.SearchItem(*cha, itemID);
	return true;
}

bool CCharacter::OpcodeHandle_CmSynAttr(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	cha->GetPlayer()->GetMainCha()->Cmd_ReassignAttr(pk);
	return true;
}

bool CCharacter::OpcodeHandle_CmRefreshData(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uLong lWorldID = 0;
	uLong lHandle = 0;
	if (!reader.Long(lWorldID) || !reader.Long(lHandle)) {
		return true;
	}
	Entity* pCEnt = g_pGameApp->IsLiveingEntity(lWorldID, lHandle);
	if (pCEnt) {
		CCharacter* pCCha = pCEnt->IsCharacter();
		if (pCCha && pCCha->GetPlayer() == cha->GetPlayer()) {
			pCCha->SynAttr(enumATTRSYN_ITEM_EQUIP);
		}
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmReadbookStart(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();
	if (!cha->IsBoat()) {
		pMainCha->SetReadBookState(true);
		pMainCha->ForgeAction(true);
		pMainCha->m_CKitbag.Lock();
	} else {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00004));
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmReadbookClose(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();
	if (!cha->IsBoat()) {
		pMainCha->SetReadBookState(false);
		pMainCha->ForgeAction(false);
		pMainCha->m_CKitbag.UnLock();
	} else {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00005));
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmKitbagCheck(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	static_cast<CCharacter*>(ctx)->GetPlyMainCha()->Cmd_CheckKitbagState();
	return true;
}

bool CCharacter::OpcodeHandle_CmKitbagUnlock(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	cChar* szPwd = nullptr;
	if (!reader.String(szPwd)) {
		return true;
	}

	cha->GetPlyMainCha()->Cmd_UnlockKitbag(szPwd);
	return true;
}

bool CCharacter::OpcodeHandle_CmBoatGetinfo(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	if (cha->GetPlayer()->IsLuanchOut()) {
		g_CharBoat.GetBoatInfo(*cha, cha->GetPlayer()->GetLuanchID());
	} else {
		cha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00003));
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmStallAlldata(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	g_StallSystem.StartStall(*static_cast<CCharacter*>(ctx), pk);
	return true;
}

bool CCharacter::OpcodeHandle_CmBeginAction(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uLong ulWorldID = 0;
	if (!reader.Long(ulWorldID)) {
		return true;
	}

	if (cha->GetPlayer()) {
		if (cha->GetPlayer()->GetCtrlCha() && ulWorldID == cha->GetPlayer()->GetCtrlCha()->GetID())
			cha->GetPlayer()->GetCtrlCha()->BeginAction(reader.Raw());
		else if (cha->GetPlayer()->GetMainCha() && ulWorldID == cha->GetPlayer()->GetMainCha()->GetID())
			cha->GetPlayer()->GetMainCha()->BeginAction(reader.Raw());
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmForge(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uChar byIndex = 0;
	if (!reader.Char(byIndex)) {
		return true;
	}
	g_ForgeSystem.ForgeItem(*cha, byIndex);
	return true;
}

bool CCharacter::OpcodeHandle_CmBoatCancel(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	g_CharBoat.Cancel(*static_cast<CCharacter*>(ctx));
	return true;
}

bool CCharacter::OpcodeHandle_CmCreateBoat(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	g_CharBoat.MakeBoat(*static_cast<CCharacter*>(ctx), pk);
	return true;
}

bool CCharacter::OpcodeHandle_CmUpdateboatPart(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	g_CharBoat.Update(*static_cast<CCharacter*>(ctx), pk);
	return true;
}

bool CCharacter::OpcodeHandle_CmStallOpen(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	g_StallSystem.OpenStall(*static_cast<CCharacter*>(ctx), pk);
	return true;
}

bool CCharacter::OpcodeHandle_CmStallClose(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	g_StallSystem.CloseStall(*static_cast<CCharacter*>(ctx));
	return true;
}

bool CCharacter::OpcodeHandle_CmKitbagAutolock(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uChar cAutoLock = 0;
	if (!reader.Char(cAutoLock)) {
		return true;
	}
	cha->GetPlyMainCha()->Cmd_SetKitbagAutoLock(cAutoLock);
	return true;
}

bool CCharacter::OpcodeHandle_CmKitbagLock(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	static_cast<CCharacter*>(ctx)->GetPlyMainCha()->Cmd_LockKitbag();
	return true;
}

bool CCharacter::OpcodeHandle_CmUpdatehair(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	if (!cha->GetSubMap())
		return true;
	cha->Cmd_ChangeHair(pk);
	return true;
}

bool CCharacter::OpcodeHandle_CmSkillupgrade(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uShort sSkillID = 0;
	uChar chAddGrade = 0;
	if (!reader.Short(sSkillID) || !reader.Char(chAddGrade)) {
		return true;
	}

	if (!PS::ValidateRange(static_cast<int>(sSkillID), 1, PS::MAX_SKILL_ID)) {
		LG("Security", "[Skill] Invalid skill ID %d from character %s\n", sSkillID, cha->GetName());
		return true;
	}

	chAddGrade = 1;

	char chSkillLv = 0;
	CCharacter* pMainCha = cha->GetPlyMainCha();
	SSkillGrid* pSkill = pMainCha->m_CSkillBag.GetSkillContByID(sSkillID);
	if (pSkill)
		chSkillLv = pSkill->chLv;

	if (chSkillLv <= 0) {
		cha->SystemNotice("Unable to upgrade skill without learning!");
		return true;
	}

	auto validation = [&]() -> bool {
		auto pCSkill = GetSkillRecordInfo(sSkillID);
		if (!pCSkill->IsShow())
			return false;
		if (sSkillID >= 25 && sSkillID <= 38)
			return false;
		if (sSkillID >= 329 && sSkillID <= 444)
			return false;
		if (sSkillID >= 321 && sSkillID <= 324)
			return false;
		if (453 <= sSkillID && sSkillID <= 459)
			return false;
		if (0467 == sSkillID || 280 == sSkillID || 311 == sSkillID)
			return false;
		return true;
	}();

	if (!validation) {
		cha->SystemNotice("You have been caught exploiting! This will lead to account suspension or deletion.");
		LG("Upgrade Exploit", "Player %s tried to force upgrade on SkillID: %d\n", pMainCha->GetName(), sSkillID);
		return true;
	}
	cha->GetPlayer()->GetMainCha()->LearnSkill(sSkillID, chAddGrade, false);
	return true;
}

bool CCharacter::OpcodeHandle_CmTeamFightAsk(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uChar chType = 0;
	uLong lID = 0;
	uLong lHandle = 0;
	if (!reader.Char(chType) || !reader.Long(lID) || !reader.Long(lHandle)) {
		return true;
	}
	cha->Cmd_FightAsk(static_cast<Char>(chType), static_cast<Long>(lID), static_cast<Long>(lHandle));
	return true;
}

bool CCharacter::OpcodeHandle_CmTeamFightAsr(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uChar chAnswer = 0;
	if (!reader.Char(chAnswer)) {
		return true;
	}
	cha->Cmd_FightAnswer(chAnswer != 0);
	return true;
}

bool CCharacter::OpcodeHandle_CmItemRepairAsk(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uLong npcId1 = 0;
	uLong npcId2 = 0;
	uChar chPosType = 0;
	uChar chPosID = 0;
	if (!reader.Long(npcId1) || !reader.Long(npcId2) || !reader.Char(chPosType) || !reader.Char(chPosID)) {
		return true;
	}
	(void)npcId1;
	(void)npcId2;
	cha->Cmd_ItemRepairAsk(static_cast<Char>(chPosType), static_cast<Char>(chPosID));
	return true;
}

bool CCharacter::OpcodeHandle_CmItemRepairAsr(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uChar answer = 0;
	if (!reader.Char(answer)) {
		return true;
	}
	cha->Cmd_ItemRepairAnswer(answer != 0);
	return true;
}

bool CCharacter::OpcodeHandle_CmStallBuy(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	if (IsEconomyBlockedByDB(*cha))
		return true;
	g_StallSystem.BuyGoods(*cha, pk);
	return true;
}

bool CCharacter::OpcodeHandle_CmBoatLuanch(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uLong dwNpcID = 0;
	if (!reader.Long(dwNpcID)) {
		return true;
	}
	CCharacter* pCha = cha->m_submap->FindCharacter(dwNpcID, cha->GetShape().centre);
	if (pCha == nullptr) {
		return true;
	} else if (cha->GetPlayer()->GetBankNpc()) {
		return true;
	} else if (g_CParser.DoString("IsSailNpc", enumSCRIPT_RETURN_NUMBER, 1, enumSCRIPT_PARAM_LIGHTUSERDATA, 1, cha, enumSCRIPT_PARAM_LIGHTUSERDATA, 1, pCha, DOSTRING_PARAM_END)) {
		if (!g_CParser.GetReturnNumber(0)) {
			return true;
		}
	}

	uChar byIndex = 0;
	if (!reader.Char(byIndex)) {
		return true;
	}
	cha->BoatSelLuanch(static_cast<BYTE>(byIndex));
	return true;
}

bool CCharacter::OpcodeHandle_CmBoatSelect(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uLong dwNpcID = 0;
	if (!reader.Long(dwNpcID)) {
		return true;
	}
	CCharacter* pCha = cha->m_submap->FindCharacter(dwNpcID, cha->GetShape().centre);
	if (pCha == nullptr) {
		return true;
	}
	if (g_CParser.DoString("IsSailBoatNpc", enumSCRIPT_RETURN_NUMBER, 1, enumSCRIPT_PARAM_LIGHTUSERDATA, 1, cha, enumSCRIPT_PARAM_LIGHTUSERDATA, 1, pCha, DOSTRING_PARAM_END)) {
		if (!g_CParser.GetReturnNumber(0)) {
			return true;
		}
	}
	uChar byType = 0;
	uChar byIndex = 0;
	if (!reader.Char(byType) || !reader.Char(byIndex)) {
		return true;
	}
	cha->BoatSelected(static_cast<BYTE>(byType), static_cast<BYTE>(byIndex));
	return true;
}

bool CCharacter::OpcodeHandle_CmBoatBagsel(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uLong dwNpcID = 0;
	if (!reader.Long(dwNpcID)) {
		return true;
	}
	if (dwNpcID) {
		CCharacter* pCha = cha->m_submap->FindCharacter(dwNpcID, cha->GetShape().centre);
		if (pCha == nullptr)
			return true;
	}

	uChar byIndex = 0;
	if (!reader.Char(byIndex)) {
		return true;
	}
	cha->BoatPackBag(static_cast<BYTE>(byIndex));
	return true;
}

bool CCharacter::OpcodeHandle_CmEntityEvent(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uLong dwEntityID = 0;
	if (!reader.Long(dwEntityID)) {
		return true;
	}
	CCharacter* pCha = cha->m_submap->FindCharacter(dwEntityID, cha->GetShape().centre);
	if (pCha == nullptr)
		return true;
	mission::CEventEntity* pEntity = pCha->IsEvent();
	if (pEntity) {
		pEntity->MsgProc(*cha, reader.Raw());
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmItemForgeCanaction(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uChar canaction = 0;
	if (!reader.Char(canaction)) {
		return true;
	}
	cha->ForgeAction(canaction != 0);
	return true;
}

bool CCharacter::OpcodeHandle_CmValidateSlotItem(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uChar chFormType = 0;
	uChar chSlotIndex = 0;
	uShort sGridID = 0;
	if (!reader.Char(chFormType) || !reader.Char(chSlotIndex) || !reader.Short(sGridID)) {
		return true;
	}

	if (!PS::ValidateRange(static_cast<int>(chFormType), 1, 6)) {
		LG("Security", "[SlotItem] Invalid form type %d from character %s\n", chFormType, cha->GetName());
		return true;
	}
	if (!PS::ValidateRange(static_cast<int>(chSlotIndex), 0, 9)) {
		LG("Security", "[SlotItem] Invalid slot index %d from character %s\n", chSlotIndex, cha->GetName());
		return true;
	}
	if (!PS::ValidateKitbagSlot(static_cast<int>(sGridID)) && sGridID != -1) {
		LG("Security", "[SlotItem] Invalid grid ID %d from character %s\n", sGridID, cha->GetName());
		return true;
	}

	cha->Cmd_ValidateSlotItem(chFormType, chSlotIndex, sGridID);
	return true;
}

bool CCharacter::OpcodeHandle_CmItemForgeAsk(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uChar first = 0;
	if (!reader.Char(first)) {
		return true;
	}
	if (first == 0) {
		cha->ForgeAction(false);
		return true;
	}
	uChar chType = 0;
	if (!reader.Char(chType)) {
		cha->ForgeAction(false);
		return true;
	}

	if (!PS::ValidateRange(static_cast<int>(chType), 0, 10)) {
		LG("Security", "[Forge] Invalid forge type %d from character %s\n", chType, cha->GetName());
		cha->ForgeAction(false);
		return true;
	}

	SForgeItem SFgeItem;
	bool validForge = true;
	for (int i = 0; i < defMAX_ITEM_FORGE_GROUP && validForge; i++) {
		uShort sGridNum = 0;
		if (!reader.Short(sGridNum)) {
			cha->ForgeAction(false);
			return true;
		}
		SFgeItem.SGroup[i].sGridNum = static_cast<short>(sGridNum);
		if (SFgeItem.SGroup[i].sGridNum < 0 || SFgeItem.SGroup[i].sGridNum > defMAX_KBITEM_NUM_PER_TYPE) {
			cha->ForgeAction(false);
			validForge = false;
			break;
		}
		for (short j = 0; j < SFgeItem.SGroup[i].sGridNum; j++) {
			uShort sGridID = 0;
			uShort sItemNum = 0;
			if (!reader.Short(sGridID) || !reader.Short(sItemNum)) {
				cha->ForgeAction(false);
				return true;
			}
			SFgeItem.SGroup[i].SGrid[j].sGridID = static_cast<short>(sGridID);
			SFgeItem.SGroup[i].SGrid[j].sItemNum = static_cast<short>(sItemNum);

			if (!PS::ValidateKitbagSlot(static_cast<int>(SFgeItem.SGroup[i].SGrid[j].sGridID))) {
				LG("Security", "[Forge] Invalid grid ID %d from character %s\n", SFgeItem.SGroup[i].SGrid[j].sGridID, cha->GetName());
				validForge = false;
				break;
			}
			if (SFgeItem.SGroup[i].SGrid[j].sItemNum < 0 || SFgeItem.SGroup[i].SGrid[j].sItemNum > PS::MAX_STACK_COUNT) {
				LG("Security", "[Forge] Invalid item count %d from character %s\n", SFgeItem.SGroup[i].SGrid[j].sItemNum, cha->GetName());
				validForge = false;
				break;
			}
		}
	}
	if (!validForge) {
		cha->ForgeAction(false);
		return true;
	}
	cha->Cmd_ItemForgeAsk(static_cast<Char>(chType), &SFgeItem);
	return true;
}

bool CCharacter::OpcodeHandle_CmItemLotteryAsk(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uChar first = 0;
	if (!reader.Char(first)) {
		return true;
	}
	if (first == 0) {
		cha->ForgeAction(false);
		return true;
	}

	SLotteryItem SLtrItem;
	bool validLottery = true;
	for (int i = 0; i < defMAX_ITEM_LOTTERY_GROUP && validLottery; i++) {
		uShort sGridNum = 0;
		if (!reader.Short(sGridNum)) {
			cha->ForgeAction(false);
			return true;
		}
		SLtrItem.SGroup[i].sGridNum = static_cast<short>(sGridNum);
		if (SLtrItem.SGroup[i].sGridNum < 0 || SLtrItem.SGroup[i].sGridNum > defMAX_KBITEM_NUM_PER_TYPE) {
			validLottery = false;
			break;
		}
		for (short j = 0; j < SLtrItem.SGroup[i].sGridNum; j++) {
			uShort sGridID = 0;
			uShort sItemNum = 0;
			if (!reader.Short(sGridID) || !reader.Short(sItemNum)) {
				cha->ForgeAction(false);
				return true;
			}
			SLtrItem.SGroup[i].SGrid[j].sGridID = static_cast<short>(sGridID);
			SLtrItem.SGroup[i].SGrid[j].sItemNum = static_cast<short>(sItemNum);

			if (!PS::ValidateKitbagSlot(static_cast<int>(SLtrItem.SGroup[i].SGrid[j].sGridID))) {
				LG("Security", "[Lottery] Invalid grid ID %d from character %s\n", SLtrItem.SGroup[i].SGrid[j].sGridID, cha->GetName());
				validLottery = false;
				break;
			}
			if (SLtrItem.SGroup[i].SGrid[j].sItemNum < 0 || SLtrItem.SGroup[i].SGrid[j].sItemNum > PS::MAX_STACK_COUNT) {
				LG("Security", "[Lottery] Invalid item count %d from character %s\n", SLtrItem.SGroup[i].SGrid[j].sItemNum, cha->GetName());
				validLottery = false;
				break;
			}
		}
	}
	if (!validLottery) {
		cha->ForgeAction(false);
		return true;
	}
	cha->Cmd_ItemLotteryAsk(&SLtrItem);
	return true;
}

bool CCharacter::OpcodeHandle_CmItemForgeAsr(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uChar answer = 0;
	if (!reader.Char(answer)) {
		return true;
	}
	cha->Cmd_ItemForgeAnswer(answer != 0 ? true : false);
	return true;
}

bool CCharacter::OpcodeHandle_CmLifeskillAsk(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uLong type = 0;
	uLong dwNpcID = 0;
	if (!reader.Long(type) || !reader.Long(dwNpcID)) {
		return true;
	}
	(void)dwNpcID;
	if (type >= 4) {
		return true;
	}

	SLifeSkillItem LifeSkillItem;
	LifeSkillItem.sbagCount = g_sLiveSkillNeedItemNum[type];
	for (int i = 0; i < LifeSkillItem.sbagCount; i++) {
		uShort gridId = 0;
		if (!reader.Short(gridId)) {
			return true;
		}
		LifeSkillItem.sGridID[i] = gridId;
	}
	switch (type) {
	case 0: {
		LifeSkillItem.sReturn = atoi(cha->GetPlayer()->GetLifeSkillinfo().c_str());
		break;
	}
	case 1: {
		string strVer[2];
		Util_ResolveTextLine(cha->GetPlayer()->GetLifeSkillinfo().c_str(), strVer, 2, ',');
		if (atoi(strVer[0].c_str()) > atoi(strVer[1].c_str()))
			LifeSkillItem.sReturn = 1;
		else
			LifeSkillItem.sReturn = 0;
		break;
	}
	case 2: {
		uShort sret = 0;
		if (!reader.Short(sret)) {
			return true;
		}
		string strVer[3];
		Util_ResolveTextLine(cha->GetPlayer()->GetLifeSkillinfo().c_str(), strVer, 3, ',');
		int count = atoi(strVer[0].c_str()) + atoi(strVer[1].c_str()) + atoi(strVer[2].c_str());
		count -= 9;
		if (count > 0)
			count = 1;
		else
			count = 0;
		if (count == sret)
			LifeSkillItem.sReturn = 1;
		else
			LifeSkillItem.sReturn = 0;
		break;
	}
	case 3: {
		uShort sReturn = 0;
		if (!reader.Short(sReturn)) {
			return true;
		}
		LifeSkillItem.sReturn = sReturn;
		break;
	}
	}
	cha->Cmd_LifeSkillItemAsk(static_cast<int>(type), &LifeSkillItem);
	return true;
}

bool CCharacter::OpcodeHandle_CmLifeskillAsr(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uLong type = 0;
	uLong dwNpcID = 0;
	if (!reader.Long(type) || !reader.Long(dwNpcID)) {
		return true;
	}
	(void)dwNpcID;
	if (type >= 4) {
		return true;
	}

	SLifeSkillItem LifeSkillItem;
	LifeSkillItem.sbagCount = g_sLiveSkillNeedItemNum[type];
	for (int i = 0; i < LifeSkillItem.sbagCount; i++) {
		uShort gridId = 0;
		if (!reader.Short(gridId)) {
			return true;
		}
		LifeSkillItem.sGridID[i] = gridId;
	}

	switch (type) {
	case 0: {
		cChar* pchar = nullptr;
		if (!reader.String(pchar)) {
			return true;
		}
		(void)pchar;
		LifeSkillItem.sReturn = 1;
	}
	case 1: {
		LifeSkillItem.sReturn = 0;
	}
	case 2: {
		uShort sReturn = 0;
		if (!reader.Short(sReturn)) {
			return true;
		}
		LifeSkillItem.sReturn = sReturn;
		break;
	}
	case 3: {
		uShort sReturn = 0;
		if (!reader.Short(sReturn)) {
			return true;
		}
		LifeSkillItem.sReturn = sReturn;
		break;
	}
	}

	cha->Cmd_LifeSkillItemAsR(static_cast<int>(type), &LifeSkillItem);
	return true;
}

bool CCharacter::OpcodeHandle_CmKitbagExpand(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();
	if (!pMainCha)
		return true;

	const int EXPAND_COST = 100;
	const short EXPAND_AMOUNT = 6;

	short currentCap = pMainCha->m_CKitbag.GetCapacity();
	if (currentCap >= defMAX_KBITEM_NUM_PER_TYPE) {
		pMainCha->SystemNotice("Your inventory is already at maximum capacity.");
		return true;
	}

	short actualExpand = EXPAND_AMOUNT;
	if (currentCap + actualExpand > defMAX_KBITEM_NUM_PER_TYPE)
		actualExpand = defMAX_KBITEM_NUM_PER_TYPE - currentCap;

	int currentIMP = pMainCha->GetIMP();
	if (currentIMP < EXPAND_COST) {
		pMainCha->SystemNotice("Not enough IMP. You need 100 IMP to expand your inventory.");
		return true;
	}

	pMainCha->SetIMP(currentIMP - EXPAND_COST, true);

	if (!pMainCha->AddKitbagCapacity(actualExpand)) {
		pMainCha->SetIMP(currentIMP, true);
		pMainCha->SystemNotice("Failed to expand inventory.");
		return true;
	}

	char szMsg[128];
	sprintf(szMsg, "Inventory expanded by %d slots! New capacity: %d.",
		actualExpand, pMainCha->m_CKitbag.GetCapacity());
	pMainCha->SystemNotice(szMsg);
	return true;
}

bool CCharacter::OpcodeHandle_CmPing(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uLong ulPing = 0;
	unsigned long long lGateSvr = 0;
	uLong lSrcID = 0;
	uLong lGatePlayerID = 0;
	unsigned long long lGatePlayerAddr = 0;
	if (!reader.Long(ulPing) || !reader.LongLong(lGateSvr) || !reader.Long(lSrcID)
		|| !reader.Long(lGatePlayerID) || !reader.LongLong(lGatePlayerAddr)) {
		return true;
	}
	ulPing = GetTickCount() - ulPing;

	BEGINGETGATE();
	GateServer* pNoGate;
	GateServer* pGate = 0;
	while (pNoGate = GETNEXTGATE()) {
		if (MakeULong(pNoGate) == lGateSvr) {
			pGate = pNoGate;
			break;
		}
	}
	if (!pGate)
		return true;

	WPACKET WtPk = GETWPACKET();
	WRITE_CMD(WtPk, CMD_MC_QUERY_CHAPING);
	WRITE_LONG(WtPk, lSrcID);
	WRITE_STRING(WtPk, cha->GetName());
	WRITE_STRING(WtPk, cha->GetSubMap()->GetName());
	WRITE_LONG(WtPk, ulPing);
	WRITE_LONG(WtPk, lGatePlayerID);
	WRITE_LONGLONG(WtPk, lGatePlayerAddr);
	WRITE_SHORT(WtPk, 1);
	pGate->SendData(WtPk);
	return true;
}

bool CCharacter::OpcodeHandle_CmTigerStart(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uLong dwNpcID = 0;
	if (!reader.Long(dwNpcID)) {
		return true;
	}

	for (int i = 0; i < 3; i++) {
		uShort sTigerSel = 0;
		if (!reader.Short(sTigerSel)) {
			return true;
		}
		cha->m_sTigerSel[i] = (sTigerSel > 0) ? 1 : 0;
	}

	CCharacter* pCha = cha->m_submap->FindCharacter(dwNpcID, cha->GetShape().centre);
	if (pCha == nullptr)
		return true;

	CCharacter* pMainCha = cha->GetPlyMainCha();
	pMainCha->DoTigerScript("TigerStart");
	return true;
}

bool CCharacter::OpcodeHandle_CmTigerStop(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uLong dwNpcID = 0;
	if (!reader.Long(dwNpcID)) {
		return true;
	}
	CCharacter* pCha = cha->m_submap->FindCharacter(dwNpcID, cha->GetShape().centre);
	if (pCha == nullptr)
		return true;

	CCharacter* pMainCha = cha->GetPlyMainCha();
	uShort sNum = 0;
	if (!reader.Short(sNum)) {
		return true;
	}

	if (sNum < 1 || sNum > 3) {
		pMainCha->ForgeAction(false);
		memset(cha->m_sTigerItemID, 0, sizeof(cha->m_sTigerItemID));
		memset(cha->m_sTigerSel, 0, sizeof(cha->m_sTigerSel));
		return true;
	}

	short sIndex = 3 * (sNum - 1);
	bool bSucc = true;
	WPACKET wpk = GETWPACKET();
	WRITE_CMD(wpk, CMD_MC_TIGER_ITEM_ID);
	WRITE_SHORT(wpk, sNum);
	for (int i = 0; i < 3; i++) {
		if (pMainCha->m_sTigerItemID[sIndex] <= 0) {
			bSucc = false;
		}
		WRITE_SHORT(wpk, pMainCha->m_sTigerItemID[sIndex++]);
	}
	cha->ReflectINFof(cha, wpk);

	if (bSucc) {
		if (sNum == 3) {
			pMainCha->DoTigerScript("TigerStop");
			memset(cha->m_sTigerItemID, 0, sizeof(cha->m_sTigerItemID));
			memset(cha->m_sTigerSel, 0, sizeof(cha->m_sTigerSel));
		}
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmStoreOpenAsk(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	cChar* szPwd = nullptr;
	if (!reader.String(szPwd) || !szPwd) {
		return true;
	}
	CCharacter* pMainCha = cha->GetPlyMainCha();
	if (pMainCha->IsReadBook()) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00008));
		return true;
	}

	if (pMainCha->IsStoreEnable()) {
		return true;
	}

	if (!pMainCha->CheckStoreTime(1000)) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00009));
		return true;
	}
	pMainCha->ResetStoreTime();

	CPlayer* pCply = pMainCha->GetPlayer();
	cChar* szPwd2 = pCply->GetPassword();

	if ((szPwd2[0] == 0) || (!strcmp(szPwd, szPwd2)) || g_Config.m_bInstantIGS) {
		pMainCha->SetStoreEnable(true);
		HandleStoreOperate(cha, reader.Raw(), CMD_CM_STORE_OPEN_ASK);
	} else {
		pMainCha->PopupNotice(RES_STRING(GM_CHARACTERPRL_CPP_00010));
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmStoreListAsk(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	HandleStoreOperate(static_cast<CCharacter*>(ctx), pk, CMD_CM_STORE_LIST_ASK);
	return true;
}

bool CCharacter::OpcodeHandle_CmStoreBuyAsk(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	HandleStoreOperate(static_cast<CCharacter*>(ctx), pk, CMD_CM_STORE_BUY_ASK);
	return true;
}

bool CCharacter::OpcodeHandle_CmStoreChangeAsk(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	HandleStoreOperate(static_cast<CCharacter*>(ctx), pk, CMD_CM_STORE_CHANGE_ASK);
	return true;
}

bool CCharacter::OpcodeHandle_CmStoreQuery(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	HandleStoreOperate(static_cast<CCharacter*>(ctx), pk, CMD_CM_STORE_QUERY);
	return true;
}

bool CCharacter::OpcodeHandle_CmStoreVip(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	HandleStoreOperate(static_cast<CCharacter*>(ctx), pk, CMD_CM_STORE_VIP);
	return true;
}

bool CCharacter::OpcodeHandle_CmStoreClose(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	HandleStoreOperate(static_cast<CCharacter*>(ctx), pk, CMD_CM_STORE_CLOSE);
	return true;
}

bool CCharacter::OpcodeHandle_CmRequestTalkOrTrade(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	if (cha->GetTradeData() || cha->GetBoat() || cha->GetStallData() || !cha->GetActControl(enumACTCONTROL_TALKTO_NPC) || cha->m_CKitbag.IsLock() || !cha->GetActControl(enumACTCONTROL_ITEM_OPT)) {
		return true;
	}

	{
		DWORD dwNow = GetTickCount();
		if (dwNow - cha->m_dwLastNpcInteractTime < 300)
			return true;
		cha->m_dwLastNpcInteractTime = dwNow;
	}

	net::PacketReader reader(pk);
	uLong ulID = 0;
	if (!reader.Long(ulID)) {
		return true;
	}
	if (ulID == mission::g_WorldEudemon.GetID()) {
		mission::g_WorldEudemon.MsgProc(*cha, reader.Raw());
		return true;
	}

	CCharacter* pCha = cha->m_submap->FindCharacter(ulID, cha->GetShape().centre);
	if (pCha == nullptr) {
		return true;
	}

	mission::CNpc* pNpc = pCha->IsNpc();
	if (pNpc) {
		pNpc->MsgProc(*cha, reader.Raw());
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmKitbagtempSync(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();

	if (!pMainCha->m_pCKitbagTmp) {
		return true;
	}

	WPACKET pkret = GETWPACKET();
	WRITE_CMD(pkret, CMD_MC_KITBAGTEMP_SYNC);
	pMainCha->WriteKitbag(*(pMainCha->m_pCKitbagTmp), pkret, enumSYN_KITBAG_INIT);
	pMainCha->ReflectINFof(pMainCha, pkret);

	int lStoreItemID = pMainCha->GetStoreItemID();
	if (lStoreItemID > 0) {
		if (g_StoreSystem.Accept(pMainCha, lStoreItemID)) {
			pMainCha->SetStoreItemID(0);
		}
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmItemLockAsk(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	WPACKET rpk = GETWPACKET();
	WRITE_CMD(rpk, CMD_CM_ITEM_LOCK_ASR);
	CCharacter* pMainCha = cha->GetPlyMainCha();
	CPlayer* pCPly = cha->GetPlayer();

	if (pMainCha) {

		if (pMainCha->m_CKitbag.IsLock() || pMainCha->m_CKitbag.IsPwdLocked() || pCPly->GetStallData() || pCPly->GetMainCha()->GetTradeData()) {
			cha->SystemNotice("Bag is currently locked.");
			return true;
		}

		uChar chPosTypeRaw = 0;
		net::PacketReader reader(pk);
		if (!reader.Char(chPosTypeRaw)) {
			return true;
		}
		dbc::Char chPosType = static_cast<dbc::Char>(chPosTypeRaw);

		if (!PS::ValidateKitbagSlot(chPosType)) {
			LG("Security", "[ItemLock] Invalid slot %d from character %s\n", chPosType, cha->GetName());
			return true;
		}

		SItemGrid* item = pMainCha->m_CKitbag.GetGridContByID(chPosType);
		if (item) {
			CItemRecord* pCItemRec = GetItemRecordInfo(item->sID);
			if (pCItemRec) {
				CPlayer* pPlayer = pMainCha->GetPlayer();
				if (pPlayer) {
					WRITE_CHAR(rpk, 1);
					item->dwDBID = 1;
					cha->m_CKitbag.SetChangeFlag();
					cha->SynKitbagNew(enumSYN_KITBAG_SWITCH);
					cha->ReflectINFof(pMainCha, rpk);
					return true;
				};
			};
		};
	};
	WRITE_CHAR(rpk, 0);
	pMainCha->ReflectINFof(pMainCha, rpk);
	return true;
}

bool CCharacter::OpcodeHandle_CmItemUnlockAsk(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	cChar* input_password = nullptr;
	if (!reader.String(input_password)) {
		return true;
	}
	uChar chPosTypeRaw = 0;
	if (!reader.Char(chPosTypeRaw)) {
		return true;
	}
	if (CCharacter* pMainCha = cha->GetPlyMainCha(); pMainCha) {
		pMainCha->Cmd_UnlockItem(static_cast<Char>(chPosTypeRaw), input_password);
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmGameRequestPin(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();
	if (!pMainCha)
		return true;

	if (cha->requestType == 0)
		return true;

	if (!cha->IsReqPosEqualRealPos()) {
		cha->requestType = 0;
		return true;
	}

	const char* szPwd = nullptr;
	net::PacketReader reader(pk);
	if (!reader.String(szPwd) || !szPwd) {
		return true;
	}

	CPlayer* pCply = pMainCha->GetPlayer();
	cChar* szPwd2 = pCply->GetPassword();
	if ((szPwd2[0] == 0) || (!strcmp(szPwd, szPwd2))) {
		g_CParser.DoString("HandlePinRequest", enumSCRIPT_RETURN_NUMBER, 1, enumSCRIPT_PARAM_LIGHTUSERDATA, 1, cha, enumSCRIPT_PARAM_NUMBER, 1, cha->requestType, DOSTRING_PARAM_END);
		if (!g_CParser.GetReturnNumber(0))
			return true;
	} else {
		pMainCha->PopupNotice(RES_STRING(GM_CHARACTERPRL_CPP_00010));
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmChartradeRequest(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	if (!CharTradeRateLimitOk(*cha))
		return true;
	net::PacketReader reader(pk);
	uChar byType = 0;
	uLong dwCharID = 0;
	if (!reader.Char(byType) || !reader.Long(dwCharID)) {
		return true;
	}
	g_TradeSystem.Request(byType, *cha, dwCharID);
	return true;
}

bool CCharacter::OpcodeHandle_CmChartradeAccept(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	if (!CharTradeRateLimitOk(*cha))
		return true;
	net::PacketReader reader(pk);
	uChar byType = 0;
	uLong dwCharID = 0;
	if (!reader.Char(byType) || !reader.Long(dwCharID)) {
		return true;
	}
	g_TradeSystem.Accept(byType, *cha, dwCharID);
	return true;
}

bool CCharacter::OpcodeHandle_CmChartradeReject(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	(void)ctx;
	return true;
}

bool CCharacter::OpcodeHandle_CmChartradeCancel(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	if (!CharTradeRateLimitOk(*cha))
		return true;
	net::PacketReader reader(pk);
	uChar byType = 0;
	uLong dwCharID = 0;
	if (!reader.Char(byType) || !reader.Long(dwCharID)) {
		return true;
	}
	g_TradeSystem.Cancel(byType, *cha, dwCharID);
	return true;
}

bool CCharacter::OpcodeHandle_CmChartradeItem(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	if (!CharTradeRateLimitOk(*cha))
		return true;
	net::PacketReader reader(pk);
	uChar byType = 0;
	uLong dwCharID = 0;
	uChar byOpType = 0;
	uChar byIndex = 0;
	uChar byItemIndex = 0;
	uChar byCount = 0;
	if (!reader.Char(byType) || !reader.Long(dwCharID) || !reader.Char(byOpType) ||
		!reader.Char(byIndex) || !reader.Char(byItemIndex) || !reader.Char(byCount)) {
		return true;
	}

	if (!PS::ValidateTradeSlot(byIndex)) {
		LG("Security", "[Trade] Invalid trade slot %d from character %s\n", byIndex, cha->GetName());
		return true;
	}
	if (!PS::ValidateKitbagSlot(byItemIndex)) {
		LG("Security", "[Trade] Invalid kitbag slot %d from character %s\n", byItemIndex, cha->GetName());
		return true;
	}
	if (!PS::ValidateStackCount(byCount) && byCount != 0) {
		LG("Security", "[Trade] Invalid stack count %d from character %s\n", byCount, cha->GetName());
		return true;
	}

	g_TradeSystem.AddItem(byType, *cha, dwCharID, byOpType, byIndex, byItemIndex, byCount);
	return true;
}

bool CCharacter::OpcodeHandle_CmChartradeMoney(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	if (!CharTradeRateLimitOk(*cha))
		return true;
	net::PacketReader reader(pk);
	uChar byType = 0;
	uLong dwCharID = 0;
	uChar byOpType = 0;
	uChar currency = 0;
	if (!reader.Char(byType) || !reader.Long(dwCharID) || !reader.Char(byOpType) || !reader.Char(currency)) {
		return true;
	}
	long long llMoney = 0;
	if (currency == 0) {
		unsigned long long money = 0;
		if (!reader.LongLong(money)) {
			return true;
		}
		llMoney = static_cast<long long>(money);
	} else if (currency == 1) {
		uLong money = 0;
		if (!reader.Long(money)) {
			return true;
		}
		llMoney = static_cast<long long>(money);
	}

	if (currency != 0 && currency != 1) {
		LG("Security", "[Trade] Invalid currency type %d from character %s\n", currency, cha->GetName());
		return true;
	}
	if (llMoney < 0) {
		LG("Security", "[Trade] Negative money %lld from character %s - possible exploit attempt\n", llMoney, cha->GetName());
		return true;
	}
	if (currency == 0 && !PS::ValidateGold(llMoney)) {
		LG("Security", "[Trade] Gold amount %lld exceeds maximum from character %s\n", llMoney, cha->GetName());
		return true;
	}
	if (currency == 1 && !PS::ValidateIMPs((int)llMoney)) {
		LG("Security", "[Trade] IMP amount %lld exceeds maximum from character %s\n", llMoney, cha->GetName());
		return true;
	}

	if (currency == 0) {
		g_TradeSystem.AddMoney(byType, *cha, dwCharID, byOpType, llMoney);
	} else if (currency == 1) {
		g_TradeSystem.AddIMP(byType, *cha, dwCharID, byOpType, (DWORD)llMoney);
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmChartradeValidatedata(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	if (!CharTradeRateLimitOk(*cha))
		return true;
	net::PacketReader reader(pk);
	uChar byType = 0;
	uLong dwCharID = 0;
	if (!reader.Char(byType) || !reader.Long(dwCharID)) {
		return true;
	}
	g_TradeSystem.ValidateItemData(byType, *cha, dwCharID);
	return true;
}

bool CCharacter::OpcodeHandle_CmChartradeValidate(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	if (!CharTradeRateLimitOk(*cha))
		return true;
	net::PacketReader reader(pk);
	uChar byType = 0;
	uLong dwCharID = 0;
	if (!reader.Char(byType) || !reader.Long(dwCharID)) {
		return true;
	}
	g_TradeSystem.ValidateTrade(byType, *cha, dwCharID);
	return true;
}

bool CCharacter::OpcodeHandle_CmVolunterOpen(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();
	net::PacketReader reader(pk);
	uShort sNum = 0;
	if (!reader.Short(sNum)) {
		return true;
	}

	int nVolNum = g_pGameApp->GetVolNum();
	int nStart = 0;
	short sRetNum = (nVolNum - nStart < sNum) ? (nVolNum - nStart) : sNum;
	if (sRetNum < 0)
		sRetNum = 0;
	short sPageNum = (nVolNum % sNum == 0) ? (nVolNum / sNum) : (nVolNum / sNum + 1);

	char chState = (pMainCha->IsVolunteer() ? 1 : 0);
	WPACKET packet = GETWPACKET();
	WRITE_CMD(packet, CMD_MC_VOLUNTER_OPEN);
	WRITE_CHAR(packet, chState);
	WRITE_SHORT(packet, sPageNum);
	WRITE_SHORT(packet, sRetNum);
	for (int i = 0; i < sRetNum; i++) {
		SVolunteer* pVolunteer = g_pGameApp->GetVolInfo(nStart + i);
		WRITE_STRING(packet, pVolunteer->szName);
		WRITE_LONG(packet, pVolunteer->lLevel);
		WRITE_LONG(packet, pVolunteer->lJob);
		WRITE_STRING(packet, pVolunteer->szMapName);
	}
	cha->ReflectINFof(cha, packet);
	return true;
}

bool CCharacter::OpcodeHandle_CmVolunterList(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uShort sPage = 0;
	uShort sNum = 0;
	if (!reader.Short(sPage) || !reader.Short(sNum)) {
		return true;
	}

	int nVolNum = g_pGameApp->GetVolNum();
	int nStart = (sPage - 1) * sNum;
	short sRetNum = (nVolNum - nStart < sNum) ? (nVolNum - nStart) : sNum;
	if (sRetNum < 0)
		sRetNum = 0;
	short sPageNum = (nVolNum % sNum == 0) ? (nVolNum / sNum) : (nVolNum / sNum + 1);

	WPACKET packet = GETWPACKET();
	WRITE_CMD(packet, CMD_MC_VOLUNTER_LIST);
	WRITE_SHORT(packet, sPageNum);
	WRITE_SHORT(packet, sPage);
	WRITE_SHORT(packet, sRetNum);
	for (int i = 0; i < sRetNum; i++) {
		SVolunteer* pVolunteer = g_pGameApp->GetVolInfo(nStart + i);
		WRITE_STRING(packet, pVolunteer->szName);
		WRITE_LONG(packet, pVolunteer->lLevel);
		WRITE_LONG(packet, pVolunteer->lJob);
		WRITE_STRING(packet, pVolunteer->szMapName);
	}
	cha->ReflectINFof(cha, packet);
	return true;
}

bool CCharacter::OpcodeHandle_CmVolunterAdd(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();
	pMainCha->Cmd_AddVolunteer();
	pMainCha->SynVolunteerState(pMainCha->IsVolunteer());
	return true;
}

bool CCharacter::OpcodeHandle_CmVolunterDel(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();
	pMainCha->Cmd_DelVolunteer();
	pMainCha->SynVolunteerState(pMainCha->IsVolunteer());
	return true;
}

bool CCharacter::OpcodeHandle_CmVolunterSel(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();
	if (pMainCha->GetLevel() < 8) {
		pMainCha->PopupNotice("Only players lv8 and above can request party!");
		return true;
	}

	cChar* szName = nullptr;
	net::PacketReader reader(pk);
	if (!reader.String(szName) || !szName) {
		return true;
	}
	CCharacter* pTarCha = cha->FindVolunteer(szName);
	if (!pTarCha) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00012), szName);
		return true;
	}

	if (pTarCha == pMainCha) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00013));
		return true;
	}

	if (strcmp(pTarCha->GetPlyCtrlCha()->GetSubMap()->GetName(), cha->GetPlyCtrlCha()->GetSubMap()->GetName())) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00014));
		return true;
	}

	if (!(cha->GetPlyCtrlCha()->GetSubMap()->GetMapRes()->CanTeam())) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00015));
		return true;
	}

	pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00016));

	WPACKET packet = GETWPACKET();
	WRITE_CMD(packet, CMD_MC_VOLUNTER_ASK);
	WRITE_STRING(packet, pMainCha->GetName());
	pTarCha->ReflectINFof(pTarCha, packet);
	return true;
}

bool CCharacter::OpcodeHandle_CmVolunterAsr(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();
	net::PacketReader reader(pk);
	uShort sRet = 0;
	if (!reader.Short(sRet)) {
		return true;
	}
	cChar* szName = nullptr;
	if (!reader.String(szName) || !szName) {
		return true;
	}
	CCharacter* pSrcCha = g_pGameApp->FindChaByName(szName);
	if (!pSrcCha) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00012), szName);
		return true;
	}

	if (sRet == 0) {
		pSrcCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00018), pMainCha->GetName());
		return true;
	}

	WPacket l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MP_TEAM_CREATE);
	WRITE_STRING(l_wpk, pSrcCha->GetName());
	WRITE_STRING(l_wpk, pMainCha->GetName());
	pMainCha->ReflectINFof(pMainCha, l_wpk);
	return true;
}

bool CCharacter::OpcodeHandle_CmMasterInvite(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();
	net::PacketReader reader(pk);
	cChar* szName = nullptr;
	uLong dwCharID = 0;
	if (!reader.String(szName) || !reader.Long(dwCharID)) {
		return true;
	}

	if (!szName || strlen(szName) == 0 || strlen(szName) > PS::MAX_CHARACTER_NAME_LENGTH) {
		LG("Security", "[Master] Invalid name from character %s\n", cha->GetName());
		return true;
	}

	if (cha->IsBoat()) {
		cha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00019));
		return true;
	}

	CCharacter* pTarCha = pMainCha->GetSubMap()->FindCharacter(dwCharID, pMainCha->GetShape().centre);
	if (!pTarCha) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00012), szName);
		return true;
	}

	if (pTarCha->IsOfflineStallNPC()) {
		pMainCha->SystemNotice("You cannot send a disciple request to an offline stall.");
		return true;
	}

	if (pTarCha->GetLevel() < 41) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00017));
		return true;
	}

	if (pMainCha->GetLevel() > 40) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00020));
		return true;
	}

	if (pMainCha->GetMasterDBID() != 0) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00021));
		return true;
	}

	if (pTarCha->IsInvited()) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00022));
		return true;
	}
	if (!pTarCha->GetPlayer() || !pTarCha->GetPlayer()->CanReceiveRequests()) {
		pMainCha->SystemNotice("%s is currently offline. Unable to send request!", pMainCha->GetName());
		return true;
	}

	pTarCha->SetInvited(true);

	WPACKET packet = GETWPACKET();
	WRITE_CMD(packet, CMD_MC_MASTER_ASK);
	WRITE_STRING(packet, pMainCha->GetName());
	WRITE_LONG(packet, pMainCha->GetID());
	pTarCha->ReflectINFof(pTarCha, packet);
	return true;
}

bool CCharacter::OpcodeHandle_CmMasterAsr(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();
	net::PacketReader reader(pk);
	uShort sRet = 0;
	cChar* szName = nullptr;
	uLong dwCharID = 0;
	if (!reader.Short(sRet) || !reader.String(szName) || !reader.Long(dwCharID)) {
		return true;
	}

	pMainCha->SetInvited(false);

	if (cha->IsBoat()) {
		cha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00023));
		return true;
	}

	CCharacter* pSrcCha = pMainCha->GetSubMap()->FindCharacter(dwCharID, pMainCha->GetShape().centre);
	if (!pSrcCha) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00012), szName);
		return true;
	}

	if (pMainCha->GetLevel() < 41) {
		pSrcCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00017));
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00024));
		return true;
	}

	if (pSrcCha->GetLevel() > 40) {
		pSrcCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00020));
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00025));
		return true;
	}

	if (sRet == 0) {
		pSrcCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00026), pMainCha->GetName());
		return true;
	}

	WPacket l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MP_MASTER_CREATE);
	WRITE_STRING(l_wpk, pSrcCha->GetName());
	WRITE_LONG(l_wpk, pSrcCha->GetPlayer()->GetDBChaId());
	WRITE_STRING(l_wpk, pMainCha->GetName());
	WRITE_LONG(l_wpk, pMainCha->GetPlayer()->GetDBChaId());
	pMainCha->ReflectINFof(pMainCha, l_wpk);
	return true;
}

bool CCharacter::OpcodeHandle_CmMasterDel(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();
	net::PacketReader reader(pk);
	cChar* szName = nullptr;
	uLong ulChaID = 0;
	if (!reader.String(szName) || !reader.Long(ulChaID)) {
		return true;
	}

	if (pMainCha->GetLevel() > 40) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00027));
		return true;
	}

	int lDelMoney = 0;
	if (!pMainCha->HasMoney(lDelMoney)) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00028));
		return true;
	}
	pMainCha->SystemNotice("Your Mentor Deleted Successfully ");

	WPacket l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MP_MASTER_DEL);
	WRITE_STRING(l_wpk, pMainCha->GetName());
	WRITE_LONG(l_wpk, pMainCha->GetPlayer()->GetDBChaId());
	WRITE_STRING(l_wpk, szName);
	WRITE_LONG(l_wpk, ulChaID);
	pMainCha->ReflectINFof(pMainCha, l_wpk);
	return true;
}

bool CCharacter::OpcodeHandle_CmPrenticeDel(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();
	net::PacketReader reader(pk);
	cChar* szName = nullptr;
	uLong ulChaID = 0;
	if (!reader.String(szName) || !reader.Long(ulChaID)) {
		return true;
	}

	int lCredit = (int)pMainCha->GetCredit();
	if (lCredit < 0) {
		lCredit = 0;
	}
	pMainCha->SetCredit(lCredit);
	pMainCha->SynAttr(enumATTRSYN_TASK);
	pMainCha->SystemNotice("Your Disciple Deleted Successfully ");

	WPacket l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MP_MASTER_DEL);
	WRITE_STRING(l_wpk, szName);
	WRITE_LONG(l_wpk, ulChaID);
	WRITE_STRING(l_wpk, pMainCha->GetName());
	WRITE_LONG(l_wpk, pMainCha->GetPlayer()->GetDBChaId());
	pMainCha->ReflectINFof(pMainCha, l_wpk);
	return true;
}

bool CCharacter::OpcodeHandle_CmPrenticeInvite(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();
	net::PacketReader reader(pk);
	cChar* szName = nullptr;
	uLong dwCharID = 0;
	if (!reader.String(szName) || !reader.Long(dwCharID)) {
		return true;
	}

	if (cha->IsBoat()) {
		cha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00023));
		return true;
	}

	CCharacter* pTarCha = pMainCha->GetSubMap()->FindCharacter(dwCharID, pMainCha->GetShape().centre);
	if (!pTarCha) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00012), szName);
		return true;
	}

	if (pTarCha->IsOfflineStallNPC()) {
		pMainCha->SystemNotice("You cannot send a disciple request to an offline stall.");
		return true;
	}

	if (pMainCha->GetLevel() < 41) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00024));
		return true;
	}

	if (pTarCha->GetLevel() > 40) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00025));
		return true;
	}

	if (pTarCha->IsInvited()) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00022));
		return true;
	}
	if (!pTarCha->GetPlayer() || !pTarCha->GetPlayer()->CanReceiveRequests()) {
		pMainCha->SystemNotice("%s is currently offline. Unable to send request!", pMainCha->GetName());
		return true;
	}
	pTarCha->SetInvited(true);

	WPACKET packet = GETWPACKET();
	WRITE_CMD(packet, CMD_MC_PRENTICE_ASK);
	WRITE_STRING(packet, pMainCha->GetName());
	WRITE_LONG(packet, pMainCha->GetID());
	pTarCha->ReflectINFof(pTarCha, packet);
	return true;
}

bool CCharacter::OpcodeHandle_CmPrenticeAsr(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();
	net::PacketReader reader(pk);
	uShort sRet = 0;
	cChar* szName = nullptr;
	uLong dwCharID = 0;
	if (!reader.Short(sRet) || !reader.String(szName) || !reader.Long(dwCharID)) {
		return true;
	}

	pMainCha->SetInvited(false);

	if (cha->IsBoat()) {
		cha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00019));
		return true;
	}

	CCharacter* pSrcCha = pMainCha->GetSubMap()->FindCharacter(dwCharID, pMainCha->GetShape().centre);
	if (!pSrcCha) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00012), szName);
		return true;
	}

	if (pSrcCha->GetLevel() < 41) {
		pSrcCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00024));
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00017));
		return true;
	}

	if (pMainCha->GetLevel() > 40) {
		pSrcCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00025));
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00020));
		return true;
	}

	if (sRet == 0) {
		pSrcCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00030), pMainCha->GetName());
		return true;
	}

	WPacket l_wpk = GETWPACKET();
	WRITE_CMD(l_wpk, CMD_MP_MASTER_CREATE);
	WRITE_STRING(l_wpk, pMainCha->GetName());
	WRITE_LONG(l_wpk, pMainCha->GetPlayer()->GetDBChaId());
	WRITE_STRING(l_wpk, pSrcCha->GetName());
	WRITE_LONG(l_wpk, pSrcCha->GetPlayer()->GetDBChaId());
	pMainCha->ReflectINFof(pMainCha, l_wpk);
	return true;
}

bool CCharacter::OpcodeHandle_PmGuildbank(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	HandlePmGuildBank(static_cast<CCharacter*>(ctx), pk);
	return true;
}

bool CCharacter::OpcodeHandle_PmPushToGuildbank(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	int guildID = cha->GetGuildID();
	if (guildID == 0) {
		return true;
	}
	CKitbag pCSrcBag;
	game_db.GetGuildBank(guildID, &pCSrcBag);
	pCSrcBag.SetChangeFlag(false);

	SItemGrid SPopItem;
	net::PacketReader reader(pk);
	cChar* strItem = nullptr;
	if (!reader.String(strItem)) {
		return true;
	}
	String2Item(strItem, &SPopItem);

	short sSrcGridID = defKITBAG_DEFPUSH_POS;
	if (pCSrcBag.Push(&SPopItem, sSrcGridID) == enumKBACT_ERROR_FULL) {
		// drop item next to player?
	} else {
		cha->GetPlayer()->SynGuildBank(&pCSrcBag, enumSYN_KITBAG_BANK);
		cha->GetPlayer()->SetBankSaveFlag(0);
		game_db.UpdateGuildBank(guildID, &pCSrcBag);
	}
	// let group know we have finished, so the next guild bank packet can be processed.
	WPACKET WtPk = GETWPACKET();
	WRITE_CMD(WtPk, CMD_MP_GUILDBANK);
	WRITE_LONG(WtPk, guildID);
	cha->ReflectINFof(cha, WtPk);
	return true;
}

bool CCharacter::OpcodeHandle_TmChangePersoninfo(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	cChar* motto = nullptr;
	if (!reader.String(motto)) {
		return true;
	}
	if (motto) {
		cha->SetMotto(motto);
	}
	uShort sIconID = 0;
	if (!reader.Short(sIconID)) {
		return true;
	}

	// SANITIZE: Validate icon ID range (valid icons are 1-100, 0 = no icon)
	if (sIconID < 0 || sIconID > 100) {
		LG("Security", "[PersonInfo] Invalid icon ID %d from character %s\n", sIconID, cha->GetName());
		sIconID = 1;  // Reset to default icon
	}
	cha->SetIcon(sIconID);
	return true;
}

bool CCharacter::OpcodeHandle_CmGuildPerm(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uLong targetID = 0;
	uLong permission = 0;
	if (!reader.Long(targetID) || !reader.Long(permission)) {
		return true;
	}

	// SANITIZE: Validate permission value (must be valid bitmask)
	const unsigned int VALID_PERM_MASK = 0xFF;  // All valid permission bits
	if (permission & ~VALID_PERM_MASK) {
		LG("Security", "[Guild] Invalid permission mask 0x%08X from character %s\n", permission, cha->GetName());
		return true;
	}

	int guild_id = cha->GetPlyMainCha()->GetGuildID();
	if (guild_id == 0 || !emGldPermMgr & cha->GetPlyMainCha()->guildPermission || game_db.GetGuildLeaderID(guild_id) == targetID) {
		cha->GetPlyMainCha()->SystemNotice("You do not have permission to do this.");
		return true;
	}

	// update in DB
	if (!game_db.SetGuildPermission(targetID, permission, guild_id)) {
		cha->GetPlyMainCha()->SystemNotice("Player not found");
		return true;
	}

	// update in game
	CPlayer* targetPly = g_pGameApp->GetPlayerByDBID(targetID);
	if (targetPly) {
		targetPly->GetMainCha()->guildPermission = permission;
	}

	// update for group (sends to players)
	WPACKET wpk = GETWPACKET();
	WRITE_CMD(wpk, CMD_MP_GUILD_PERM);
	WRITE_LONG(wpk, targetID);
	WRITE_LONG(wpk, permission);
	cha->ReflectINFof(cha, wpk);
	return true;
}

bool CCharacter::OpcodeHandle_CmGuildPutname(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uChar confirm = 0;
	cChar* l_guildname = nullptr;
	cChar* l_passwd = nullptr;
	if (!reader.Char(confirm) || !reader.String(l_guildname) || !reader.String(l_passwd)) {
		return true;
	}
	bool l_confirm = confirm != 0;

	// SANITIZE: Validate guild name and password
	if (!l_guildname || !l_passwd) {
		cha->GetPlyMainCha()->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00006));
		return true;
	}
	size_t guildNameLen = strlen(l_guildname);
	size_t passwdLen = strlen(l_passwd);
	if (guildNameLen == 0 || guildNameLen > 16 || passwdLen == 0 || passwdLen > 32) {
		LG("Security", "[Guild] Invalid guild name/password length from character %s\n", cha->GetName());
		cha->GetPlyMainCha()->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00006));
		return true;
	}
	if (!PS::IsSafeString(l_guildname) || !PS::IsSafeString(l_passwd)) {
		LG("Security", "[Guild] Dangerous characters in guild name/password from character %s\n", cha->GetName());
		cha->GetPlyMainCha()->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00006));
		return true;
	}

	if (Guild::IsValidGuildName(l_guildname, uShort(strlen(l_guildname)))) {
		Guild::cmd_CreateGuild(cha->GetPlyMainCha(), l_confirm, l_guildname, l_passwd);
	} else {
		cha->GetPlyMainCha()->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00006));
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmGuildTryfor(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	net::PacketReader reader(pk);
	uLong guildId = 0;
	if (!reader.Long(guildId)) {
		return true;
	}
	Guild::cmd_GuildTryFor(static_cast<CCharacter*>(ctx)->GetPlyMainCha(), guildId);
	return true;
}

bool CCharacter::OpcodeHandle_CmGuildTryforcfm(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	net::PacketReader reader(pk);
	uChar confirm = 0;
	if (!reader.Char(confirm)) {
		return true;
	}
	Guild::cmd_GuildTryForComfirm(static_cast<CCharacter*>(ctx)->GetPlyMainCha(), static_cast<Char>(confirm));
	return true;
}

bool CCharacter::OpcodeHandle_CmGuildListtryplayer(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	Guild::cmd_GuildListTryPlayer(static_cast<CCharacter*>(ctx)->GetPlyMainCha());
	return true;
}

bool CCharacter::OpcodeHandle_CmGuildApprove(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	net::PacketReader reader(pk);
	uLong chaId = 0;
	if (!reader.Long(chaId)) {
		return true;
	}
	Guild::cmd_GuildApprove(static_cast<CCharacter*>(ctx)->GetPlyMainCha(), chaId);
	return true;
}

bool CCharacter::OpcodeHandle_CmGuildReject(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	net::PacketReader reader(pk);
	uLong chaId = 0;
	if (!reader.Long(chaId)) {
		return true;
	}
	Guild::cmd_GuildReject(static_cast<CCharacter*>(ctx)->GetPlyMainCha(), chaId);
	return true;
}

bool CCharacter::OpcodeHandle_CmGuildKick(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	net::PacketReader reader(pk);
	uLong chaId = 0;
	if (!reader.Long(chaId)) {
		return true;
	}
	Guild::cmd_GuildKick(static_cast<CCharacter*>(ctx)->GetPlyMainCha(), chaId);
	return true;
}

bool CCharacter::OpcodeHandle_CmGuildLeave(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	if (!(cha->GetPlyCtrlCha()->GetSubMap()->GetMapRes()->CanGuild())) {
		cha->GetPlyMainCha()->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00007));
		return true;
	}

	Guild::cmd_GuildLeave(cha->GetPlyMainCha());
	return true;
}

bool CCharacter::OpcodeHandle_CmGuildDisband(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	cChar* l_passwd = nullptr;
	if (!reader.String(l_passwd)) {
		return true;
	}
	int canDisband = (cha->GetPlyMainCha()->guildPermission & emGldPermDisband);
	if (canDisband == emGldPermDisband) {
		if (l_passwd && !strchr(l_passwd, '\'')) {
			Guild::cmd_GuildDisband(cha->GetPlyMainCha(), l_passwd);
		}
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmGuildMotto(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uShort len = 0;
	cChar* l_motto = nullptr;
	if (!reader.String(l_motto, len)) {
		return true;
	}

	if (len > 0 && !common::conformity::guild::motto::is_valid(l_motto, len))
		return true;

	// Allow empty motto for removal or valid non-empty motto
	if (l_motto && (len == 0 || (strlen(l_motto) < 50 && !strchr(l_motto, '\'')))) {
		int canMotto = (cha->GetPlyMainCha()->guildPermission & emGldPermMotto);
		if (canMotto == emGldPermMotto) {
			Guild::cmd_GuildMotto(cha->GetPlyMainCha(), l_motto);
		}
	}
	return true;
}

bool CCharacter::OpcodeHandle_PmGuildDisband(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	Guild::cmd_PMDisband(static_cast<CCharacter*>(ctx)->GetPlyMainCha());
	return true;
}

bool CCharacter::OpcodeHandle_CmGuildChallenge(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uChar byLevel = 0;
	uLong dwMoney = 0;
	if (!reader.Char(byLevel) || !reader.Long(dwMoney)) {
		return true;
	}
	Guild::cmd_GuildChallenge(cha->GetPlyMainCha(), byLevel, dwMoney);
	return true;
}

bool CCharacter::OpcodeHandle_CmGuildLeizhu(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uChar byLevel = 0;
	uLong dwMoney = 0;
	if (!reader.Char(byLevel) || !reader.Long(dwMoney)) {
		return true;
	}
	Guild::cmd_GuildLeizhu(cha->GetPlyMainCha(), byLevel, dwMoney);
	return true;
}

bool CCharacter::OpcodeHandle_CmSay2camp(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();
	net::PacketReader reader(pk);
	cChar* szContent = nullptr;
	if (!reader.String(szContent)) {
		return true;
	}
	CCharacter* pCha = nullptr;
	SubMap* pSubMap = cha->GetPlyCtrlCha()->GetSubMap();

	// Rate limit: reuse the same say interval as normal chat
	DWORD dwNowTick = GetTickCount();
	if (dwNowTick - cha->_dwLastSayTick < (DWORD)g_Config.m_lSayInterval) {
		cha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00001));
		return true;
	}
	cha->_dwLastSayTick = dwNowTick;

	// Validate chat content
	if (!szContent || !PS::ValidateChatMessage(szContent)) {
		LG("Security", "[CampChat] Invalid message from character %s\n", pMainCha->GetName());
		return true;
	}

	short mapType = pSubMap->GetMapRes()->GetType();
	bool bIsCTF = (mapType == 17);  // CTF map type
	bool bIsGuildWar = pSubMap->GetMapRes()->CanGuildWar();

	if (bIsCTF) {
		// CTF maps: send to all players on the same side (SideID)
		Long lSideID = pMainCha->GetSideID();
		if (lSideID == 0) {
			cha->SystemNotice("You are not assigned to a team.");
			return true;
		}

		pSubMap->BeginGetPlyCha();
		while (pCha = pSubMap->GetNextPlyCha()) {
			if (pCha->GetSideID() == lSideID) {
				WPacket l_wpk = GETWPACKET();
				WRITE_CMD(l_wpk, CMD_MC_SAY2CAMP);
				WRITE_STRING(l_wpk, pMainCha->GetName());
				WRITE_STRING(l_wpk, szContent);
				WRITE_LONG(l_wpk, cha->chatColour);
				pCha->ReflectINFof(pCha, l_wpk);
			}
		}
	} else if (bIsGuildWar) {
		// Guild war maps: send to all players with the same guild type
		BOOL bHasGuild = pMainCha->HasGuild();
		if (!bHasGuild) {
			cha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00031));
			return true;
		}

		DWORD dwGuildID = pMainCha->GetGuildID();

		pSubMap->BeginGetPlyCha();
		while (pCha = pSubMap->GetNextPlyCha()) {
			if (pCha->HasGuild() && pCha->GetGuildID() == dwGuildID) {
				WPacket l_wpk = GETWPACKET();
				WRITE_CMD(l_wpk, CMD_MC_SAY2CAMP);
				WRITE_STRING(l_wpk, pMainCha->GetName());
				WRITE_STRING(l_wpk, szContent);
				WRITE_LONG(l_wpk, cha->chatColour);
				pCha->ReflectINFof(pCha, l_wpk);
			}
		}
	} else {
		cha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00032));
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmGmSend(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();

	net::PacketReader reader(pk);
	uLong dwNpcID = 0;
	if (!reader.Long(dwNpcID)) {
		return true;
	}
	CCharacter* pCha = cha->m_submap->FindCharacter(dwNpcID, cha->GetShape().centre);
	if (pCha == nullptr)
		return true;

	cChar* szTitle = nullptr;
	cChar* szContent = nullptr;
	if (!reader.String(szTitle) || !reader.String(szContent)) {
		return true;
	}
	// SANITIZE: null check before strlen to prevent crash on malformed packet
	if (!szTitle || !szContent) {
		LG("Security", "[GMSend] Null title or content from character %s\n", cha->GetName());
		return true;
	}
	if (strlen(szTitle) > 32 || strlen(szContent) > 512) {
		pMainCha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00033));
		return true;
	}
	g_StoreSystem.RequestGMSend(pMainCha, szTitle, szContent);
	return true;
}

bool CCharacter::OpcodeHandle_CmGmRecv(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();

	net::PacketReader reader(pk);
	uLong dwNpcID = 0;
	if (!reader.Long(dwNpcID)) {
		return true;
	}
	CCharacter* pCha = cha->m_submap->FindCharacter(dwNpcID, cha->GetShape().centre);
	if (pCha == nullptr)
		return true;

	g_StoreSystem.RequestGMRecv(pMainCha);
	return true;
}

bool CCharacter::OpcodeHandle_CmPkCtrl(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	net::PacketReader reader(pk);
	uChar pkMode = 0;
	if (!reader.Char(pkMode)) {
		return true;
	}

	if (pkMode)
		cha->Cmd_SetInPK();
	else
		cha->Cmd_SetInPK(false);
	cha->SynPKCtrl();
	return true;
}

bool CCharacter::OpcodeHandle_CmCheatCheck(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();

	net::PacketReader reader(pk);
	cChar* answer = nullptr;
	if (!reader.String(answer)) {
		return true;
	}
	pMainCha->CheatCheck(answer);
	return true;
}

bool CCharacter::OpcodeHandle_CmBidup(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pMainCha = cha->GetPlyMainCha();
	if (g_CParser.DoString("YORN", enumSCRIPT_RETURN_NUMBER, 1, enumSCRIPT_PARAM_LIGHTUSERDATA, 1, pMainCha, DOSTRING_PARAM_END)) {
		if (g_CParser.GetReturnNumber(0)) {
			net::PacketReader reader(pk);
			uLong dwNpcID = 0;
			uShort sItemID = 0;
			uLong price = 0;
			if (!reader.Long(dwNpcID)) {
				return true;
			}
			CCharacter* pNpc = cha->m_submap->FindCharacter(dwNpcID, cha->GetShape().centre);
			if (pNpc == nullptr) {
				cha->SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00034), dwNpcID);
				return true;
			}
			if (!reader.Short(sItemID) || !reader.Long(price)) {
				return true;
			}
			g_AuctionSystem.BidUp(pMainCha, static_cast<short>(sItemID), static_cast<uInt>(price));
			g_AuctionSystem.NotifyAuction(cha, pNpc);
		}
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmAntiindulgence(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	static_cast<CCharacter*>(ctx)->GetPlyMainCha()->SetScaleFlag();
	return true;
}

bool CCharacter::OpcodeHandle_CmRequestDropRate(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pCha = cha->GetPlyCtrlCha();
	if (pCha) {
		WPACKET pk = GETWPACKET();
		WRITE_CMD(pk, CMD_MC_REQUEST_DROP_RATE);
		pk.WriteFloat(pCha->GetDropRate());
		pCha->ReflectINFof(pCha, pk);
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmRequestExpRate(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pCha = cha->GetPlyMainCha();
	if (pCha) {
		WPACKET pk = GETWPACKET();
		WRITE_CMD(pk, CMD_MC_REQUEST_EXP_RATE);
		pk.WriteFloat(pCha->GetExpRate());
		pCha->ReflectINFof(pCha, pk);
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmGetPlayerBattlePoint(void* ctx, DataSocket* /*sock*/, RPacket& /*pk*/) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pCha = cha->GetPlyMainCha();
	if (pCha) {
		WPACKET pk = GETWPACKET();
		WRITE_CMD(pk, CMD_MC_GET_PLAYER_BATTLE_POINT);
		pk.WriteLong(pCha->GetBattlePower());
		pCha->ReflectINFof(pCha, pk);
	}
	return true;
}

bool CCharacter::OpcodeHandle_CmRequestChestPreview(void* ctx, DataSocket* /*sock*/, RPacket& pk) {
	CCharacter* cha = static_cast<CCharacter*>(ctx);
	CCharacter* pCha = cha->GetPlyMainCha();
	if (pCha) {
		DWORD dwNow = GetTickCount();
		if (dwNow - cha->m_dwLastChestPreviewTime < kChestPreviewRequestCooldownMs) {
			return true;
		}
		cha->m_dwLastChestPreviewTime = dwNow;

		net::PacketReader reader(pk);
		uLong chestItemID = 0;
		if (!reader.Long(chestItemID)) {
			return true;
		}
		if (chestItemID <= 0) {
			return true;
		}

		SendChestPreviewPacket(pCha, static_cast<int>(chestItemID));
	}
	return true;
}

//----------------------------------------------------------
//                    ?????????
//----------------------------------------------------------
void CCharacter::ProcessPacket(unsigned short usCmd, RPACKET pk) {
	if (!ValidateGameCharacterOpcode(usCmd, pk, GetName())) {
		return;
	}
	if (DispatchOpcodeHandler(OpcodeDispatchDomain::GameCharacter, usCmd, this, nullptr, pk)) {
		return;
	}
}

void CCharacter::BeginAction(RPACKET pk) {
	T_B const int clPing = 300;

	if (!IsLiveing()) {

		m_CLog.Log("$$$PacketID:\t%u\n", m_ulPacketID);
		// m_CLog.Log("??????(?????)\n\n");
		m_CLog.Log("refuse action request(self inexistent)\n\n");
		return;
	}
	if (GetPlayer()->GetCtrlCha() == this && !GetSubMap()) {

		m_CLog.Log("$$$PacketID:\t%u\n", m_ulPacketID);
		// m_CLog.Log("??????(????)\n\n");
		m_CLog.Log("refuse action request(map is null)\n\n");
		return;
	}

	net::PacketReader reader(pk);
	uLong ulPacketId = 0;
#ifdef defPROTOCOL_HAVE_PACKETID
	if (!reader.Long(ulPacketId)) {
		return;
	}
#endif
	uChar chActionTypeRaw = 0;
	if (!reader.Char(chActionTypeRaw)) {
		return;
	}
	Char chActionType = static_cast<Char>(chActionTypeRaw);

	m_CLog.Log("Begin Action: \t%d\tPacketID: %u\n", chActionType, ulPacketId);

	m_ulPacketID = ulPacketId;
	switch (chActionType) {
	case enumACTION_MOVE: {
		{
			DWORD dwNow = (DWORD)GetTickCount64();
			if (dwNow - m_dwLastMovePacketTime < 50)
				break;
			m_dwLastMovePacketTime = dwNow;
		}

		if (!GetSubMap()) {

			m_CLog.Log("$$$PacketID:\t%u\n", m_ulPacketID);
			// m_CLog.Log("??????(????)\n\n");
			m_CLog.Log("refuse action request(map is null)\n\n");
			return;
		}

		if (m_CAction.GetCurActionNo() >= 0) // ?????????
		{

			FailedActionNoti(enumACTION_MOVE, enumFACTION_EXISTACT);
			// SystemNotice("????????(?????????)\n");
			SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00035));
			// m_CLog.Log("????????(?????????)[PacketID: %u]\n", ulPacketId);
			m_CLog.Log("irregular action request(foregone action hasn't finish)[PacketID: %u]\n", ulPacketId);
			break;
		}

		if (m_sPoseState == enumPoseSeat) {

			FailedActionNoti(enumACTION_MOVE, enumFACTION_EXISTACT);
			// SystemNotice("????????(?????????)\n");
			SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00035));
			// m_CLog.Log("????????(?????????)[PacketID: %u]\n", ulPacketId);
			m_CLog.Log("irregular action request(foregone action hasn't finish)[PacketID: %u]\n", ulPacketId);
			break;
		}
		ResetPosState();

		uShort ulTurnNum = 0;
		cChar* pData = reader.Raw().ReadSequence(ulTurnNum);
		Point Path[defMOVE_INFLEXION_NUM];
		Char chPointNum;
		if (!pData) {

			FailedActionNoti(enumACTION_MOVE, enumFACTION_MOVEPATH);
			// SystemNotice("??????,???????\n");
			SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00036));
			// m_CLog.Log("??????,???????\n");
			m_CLog.Log("move path error,don't have move sequence point\n");
			break;
		}
		if ((chPointNum = Char(ulTurnNum / sizeof(Point))) > defMOVE_INFLEXION_NUM) {

			FailedActionNoti(enumACTION_MOVE, enumFACTION_MOVEPATH);
			// SystemNotice("??????(???:%d,?????:%d)\n", ulTurnNum / sizeof(Point), defMOVE_INFLEXION_NUM);
			SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00037), ulTurnNum / sizeof(Point), defMOVE_INFLEXION_NUM);
			// m_CLog.Log("??????(???:%d,?????:%d)[PacketID: %u]\n", ulTurnNum / sizeof(Point), defMOVE_INFLEXION_NUM, ulPacketId);
			m_CLog.Log("move path error(inflexion number:%d,max inflexion number:%d)[PacketID: %u]\n", ulTurnNum / sizeof(Point), defMOVE_INFLEXION_NUM, ulPacketId);
			break;
		}
		memcpy(Path, pData, chPointNum * sizeof(Point));

		Cmd_BeginMove((Short)m_dwPing, Path, chPointNum);
	} break;
	case enumACTION_SKILL: {
		{
			DWORD dwNow = (DWORD)GetTickCount64();
			if (dwNow - m_dwLastSkillPacketTime < 100)
				break;
			m_dwLastSkillPacketTime = dwNow;
		}

		if (GetPlyMainCha()->m_CKitbag.IsLock()) {
			// SystemNotice("????????,??????!\n");
			SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00038));
			FailedActionNoti(enumACTION_SKILL, enumFACTION_ACTFORBID);
			break;
		}

		if (!GetSubMap()) {
			m_CLog.Log("$$$PacketID:\t%u\n", m_ulPacketID);
			// m_CLog.Log("??????(????)\n\n");
			m_CLog.Log("refuse action request(map is null)\n\n");
			return;
		}

		if (GetPlayer()->GetBankNpc()) {
			// SystemNotice("??????!\n");
			SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00039));
			FailedActionNoti(enumACTION_SKILL, enumFACTION_ACTFORBID);
			break;
		}

		if (m_CAction.GetCurActionNo() >= 0) // ?????????
		{
			FailedActionNoti(enumACTION_SKILL, enumFACTION_EXISTACT);
			// SystemNotice("????????(?????????)\n");
			SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00035));
			// m_CLog.Log("????????(?????????)[PacketID: %u]\n", ulPacketId);
			m_CLog.Log("irregular action request(foregone action hasn't finish)[PacketID: %u]\n", ulPacketId);
			break;
		}

		if (m_sPoseState == enumPoseSeat) {
			FailedActionNoti(enumACTION_SKILL, enumFACTION_EXISTACT);
			// SystemNotice("????????(?????????)\n");
			SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00035));
			// m_CLog.Log("????????(?????????)[PacketID: %u]\n", ulPacketId);
			m_CLog.Log("irregular action request(foregone action hasn't finish)[PacketID: %u]\n", ulPacketId);
			break;
		}
		ResetPosState();

		uChar chMoveRaw = 0;
		if (!reader.Char(chMoveRaw)) {
			return;
		}
		char chMove = static_cast<char>(chMoveRaw);
		if (chMove == 2) // ????????????
		{
			uChar chFightIDRaw = 0;
			if (!reader.Char(chFightIDRaw)) {
				return;
			}
			Char chFightID = static_cast<Char>(chFightIDRaw);
			// ???
			Point Path[defMOVE_INFLEXION_NUM];
			Char chPointNum;
			uShort ulTurnNum = 0;
			cChar* pData = reader.Raw().ReadSequence(ulTurnNum);
			if (!pData) {
				FailedActionNoti(enumACTION_SKILL, enumFACTION_MOVEPATH);
				// SystemNotice("??????,???????\n");
				SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00036));
				// m_CLog.Log("??????,???????\n");
				m_CLog.Log("move path error,don't have move sequence point\n");
				break;
			}

			if ((chPointNum = Char(ulTurnNum / sizeof(Point))) > defMOVE_INFLEXION_NUM) {
				FailedActionNoti(enumACTION_SKILL, enumFACTION_MOVEPATH);
				// SystemNotice("??????(???:%d,?????:%d)\n", ulTurnNum / sizeof(Point), defMOVE_INFLEXION_NUM);
				SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00037), ulTurnNum / sizeof(Point), defMOVE_INFLEXION_NUM);
				// m_CLog.Log("??????(???:%d,?????:%d)[PacketID: %u]\n", ulTurnNum / sizeof(Point), defMOVE_INFLEXION_NUM, ulPacketId);
				m_CLog.Log("move path error(inflexion number:%d,max inflexion number:%d)[PacketID: %u]\n", ulTurnNum / sizeof(Point), defMOVE_INFLEXION_NUM, ulPacketId);
				break;
			}
			// m_CLog.Log("????(ulTurnNum: %d)[PacketID: %u]\n", ulTurnNum, ulPacketId);
			m_CLog.Log("move path(ulTurnNum: %d)[PacketID: %u]\n", ulTurnNum, ulPacketId);
			memcpy(Path, pData, chPointNum * sizeof(Point));
			// ???
			uLong ulSkillIDRaw = 0;
			uLong lTarInfo1Raw = 0;
			uLong lTarInfo2Raw = 0;
			if (!reader.Long(ulSkillIDRaw)) {
				return;
			}
			if (!reader.Long(lTarInfo1Raw)) {
				return;
			}
			if (!reader.Long(lTarInfo2Raw)) {
				return;
			}
			dbc::uLong ulSkillID = ulSkillIDRaw;
			Long lTarInfo1 = static_cast<Long>(lTarInfo1Raw);
			Long lTarInfo2 = static_cast<Long>(lTarInfo2Raw);
			
			// SANITIZE: Validate skill ID range
			if (!PS::ValidateRange(static_cast<int>(ulSkillID), 1, PS::MAX_SKILL_ID)) {
				LG("Security", "[Skill] Invalid skill ID %u from character %s\n", ulSkillID, GetName());
				FailedActionNoti(enumACTION_SKILL, enumFACTION_NOSKILL);
				break;
			}

			CSkillRecord* pRec = GetSkillRecordInfo(ulSkillID);
			if (!pRec) {
				// LG( "?????", "??�%s�1?????(????: %d)[PacketID: %u]\n", GetName(), ulSkillID, ulPacketId);
				LG("skill inexistence", "character�%s�1skill inexistence(skill number: %d)[PacketID: %u]\n", GetName(), ulSkillID, ulPacketId);
				FailedActionNoti(enumACTION_SKILL, enumFACTION_NOSKILL);
				// LG( "?????", "??�%s�2?????(????: %d)[PacketID: %u]\n", GetName(), ulSkillID, ulPacketId);
				LG("skill inexistence", "character�%s�2skill inexistence(skill number: %d)[PacketID: %u]\n", GetName(), ulSkillID, ulPacketId);
				// SystemNotice("?????(????: %d)\n", ulSkillID);
				SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00040), ulSkillID);
				// m_CLog.Log("?????(????: %d)[PacketID: %u]\n", ulSkillID, ulPacketId);
				m_CLog.Log("skill inexistence(skill number: %d)[PacketID: %u]\n", ulSkillID, ulPacketId);
				break;
			}
			Cmd_BeginSkill((Short)m_dwPing, Path, chPointNum, pRec, 1, lTarInfo1, lTarInfo2);
		} else {
			// SystemNotice("?????(??????)????");
			SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00041));
			// m_CLog.Log("?????(??????)????[PacketID: %u]\n", ulPacketId);
			m_CLog.Log("the action type(directness use skills)has been cancellation[PacketID: %u]\n", ulPacketId);
			break;
		}
	} break;
	case enumACTION_STOP_STATE: {
		if (!GetSubMap()) {
			m_CLog.Log("$$$PacketID:\t%u\n", m_ulPacketID);
			// m_CLog.Log("??????(????)\n\n");
			m_CLog.Log("refuse action request(map is null)\n\n");
			return;
		}

		uShort sStateIDRaw = 0;
		if (!reader.Short(sStateIDRaw)) {
			return;
		}
		Short sStateID = static_cast<Short>(sStateIDRaw);

		CSkillStateRecord* pSSkillState = GetCSkillStateRecordInfo((uChar)sStateID);
		if (!pSSkillState)
			break;
		if (!pSSkillState->bCanCancel)
			break;
		DelSkillState((uChar)sStateID);
	} break;
	case enumACTION_LEAN: // ??
	{
		if (!GetSubMap()) {
			m_CLog.Log("$$$PacketID:\t%u\n", m_ulPacketID);
			// m_CLog.Log("??????(????)\n\n");
			m_CLog.Log("refuse action request(map is null)\n\n");
			return;
		}

		// Rate limit broadcast actions to prevent network saturation
		{
			DWORD dwNow = GetTickCount();
			if (dwNow - m_dwLastBroadcastTime < 200) break;
			m_dwLastBroadcastTime = dwNow;
		}

		m_sPoseState = enumPoseLean;
		m_SSeat.chIsSeat = 0;

		m_SLean.ulPacketID = ulPacketId;
		uLong lPoseRaw = 0;
		uLong lAngleRaw = 0;
		uLong lPosXRaw = 0;
		uLong lPosYRaw = 0;
		uLong lHeightRaw = 0;
		if (!reader.Long(lPoseRaw)) {
			return;
		}
		if (!reader.Long(lAngleRaw)) {
			return;
		}
		if (!reader.Long(lPosXRaw)) {
			return;
		}
		if (!reader.Long(lPosYRaw)) {
			return;
		}
		if (!reader.Long(lHeightRaw)) {
			return;
		}
		m_SLean.lPose = static_cast<Long>(lPoseRaw);
		m_SLean.lAngle = static_cast<Long>(lAngleRaw);
		m_SLean.lPosX = static_cast<Long>(lPosXRaw);
		m_SLean.lPosY = static_cast<Long>(lPosYRaw);
		m_SLean.lHeight = static_cast<Long>(lHeightRaw);
		m_SLean.chState = 0;

		// ??
		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_NOTIACTION); // ????
		WRITE_LONG(WtPk, m_ID);
		WRITE_LONG(WtPk, m_SLean.ulPacketID);
		WRITE_CHAR(WtPk, enumACTION_LEAN);
		WRITE_CHAR(WtPk, m_SLean.chState);
		WRITE_LONG(WtPk, m_SLean.lPose);
		WRITE_LONG(WtPk, m_SLean.lAngle);
		WRITE_LONG(WtPk, m_SLean.lPosX);
		WRITE_LONG(WtPk, m_SLean.lPosY);
		WRITE_LONG(WtPk, m_SLean.lHeight);
		NotiChgToEyeshot(WtPk); // ??
		//

		// log
		m_CLog.Log("$$$PacketID:\t%u\n", m_SLean.ulPacketID);
		m_CLog.Log("===Recieve(Lean):\tTick %u\n", GetTickCount());
		m_CLog.Log("\n");
		m_CLog.Log("$$$PacketID:\t%u\n", m_SLean.ulPacketID);
		m_CLog.Log("###Send(Lean):\tTick %u\n", GetTickCount());
		m_CLog.Log("\n");
		//
	} break;
		/// item picks
	case enumACTION_ITEM_PICK: // ???
	{
		uLong lWorldIDRaw = 0;
		uLong lHandleRaw = 0;
		if (!reader.Long(lWorldIDRaw)) {
			return;
		}
		if (!reader.Long(lHandleRaw)) {
			return;
		}
		Long lWorldID = static_cast<Long>(lWorldIDRaw);
		Long lHandle = static_cast<Long>(lHandleRaw);
		
		// Rate limit pickup: allow burst of up to 30 per second (Ctrl+A loot-all),
		// then throttle sustained floods that could saturate GateServer/GroupServer.
		DWORD dwNow = GetTickCount();
		if (dwNow - m_dwLastPickupTime >= 1000) {
			// New window - reset counter
			m_dwLastPickupTime = dwNow;
			m_nPickupCount = 0;
		}
		m_nPickupCount++;
		if (m_nPickupCount > 30) {
			break; // Over burst limit - drop to prevent flood
		}

		// Note: WorldID and Handle are validated by IsLiveingEntity() in Cmd_PickupItem
		// We don't need extra validation here since invalid IDs simply won't find an entity

		Short sRet = Cmd_PickupItem(lWorldID, lHandle);
		if (sRet != enumITEMOPT_SUCCESS)
			ItemOprateFailed(sRet);
	} break;
	case enumACTION_ITEM_THROW: // ???(????????)
	{
		uShort sGridIDRaw = 0;
		uShort lNumRaw = 0;
		uLong lPosXRaw = 0;
		uLong lPosYRaw = 0;
		if (!reader.Short(sGridIDRaw)) {
			return;
		}
		if (!reader.Short(lNumRaw)) {
			return;
		}
		if (!reader.Long(lPosXRaw)) {
			return;
		}
		if (!reader.Long(lPosYRaw)) {
			return;
		}
		Short sGridID = static_cast<Short>(sGridIDRaw);
		Short lNum = static_cast<Short>(lNumRaw);
		Long lPosX = static_cast<Long>(lPosXRaw);
		Long lPosY = static_cast<Long>(lPosYRaw);
		
		// SANITIZE: Validate grid, count, and position
		if (!PS::ValidateKitbagSlot(static_cast<int>(sGridID))) {
			LG("Security", "[Throw] Invalid grid %d from character %s\n", sGridID, GetName());
			break;
		}
		if (!PS::ValidateStackCount(static_cast<int>(lNum))) {
			LG("Security", "[Throw] Invalid count %d from character %s\n", lNum, GetName());
			break;
		}
		if (!PS::ValidatePosition(static_cast<int>(lPosX), static_cast<int>(lPosY))) {
			LG("Security", "[Throw] Invalid position (%d,%d) from character %s\n", lPosX, lPosY, GetName());
			break;
		}

		Short sRet = Cmd_ThrowItem(0, sGridID, &lNum, lPosX, lPosY);
		if (sRet != enumITEMOPT_SUCCESS)
			ItemOprateFailed(sRet);
	} break;
	case enumACTION_ITEM_LOCK: {
		uShort sGridIDRaw = 0;
		if (!reader.Short(sGridIDRaw)) {
			return;
		}
		Short sGridID = static_cast<Short>(sGridIDRaw);
		Short sRet = Cmd_LockItem(sGridID);
		if (sRet != enumITEMOPT_SUCCESS)
			ItemOprateFailed(sRet);
	} break;
	case enumACTION_ITEM_UNLOCK: {
		uShort sGridIDRaw = 0;
		if (!reader.Short(sGridIDRaw)) {
			return;
		}
		Short sGridID = static_cast<Short>(sGridIDRaw);
		cChar* szPwd = nullptr;
		if (!reader.String(szPwd)) {
			return;
		}
		Short sRet = Cmd_UnlockItem(sGridID, szPwd);
		if (sRet != enumITEMOPT_SUCCESS)
			ItemOprateFailed(sRet);
	} break;
	case enumACTION_ITEM_USE: // ????
	{
		uShort sFromGridIDRaw = 0;
		uShort sToGridIDRaw = 0;
		if (!reader.Short(sFromGridIDRaw)) {
			return;
		}
		if (!reader.Short(sToGridIDRaw)) {
			return;
		}
		Short sFromGridID = static_cast<Short>(sFromGridIDRaw);
		Short sToGridID = static_cast<Short>(sToGridIDRaw);
		
		// SANITIZE: Validate grid IDs
		if (!PS::ValidateKitbagSlot(static_cast<int>(sFromGridID))) {
			LG("Security", "[UseItem] Invalid from grid %d from character %s\n", sFromGridID, GetName());
			break;
		}
		// ToGridID can be -1 for self-use items, -2 for right-click use, -3 for equipped mount target
		if (sToGridID != -1 && sToGridID != -2 && sToGridID != -3 && !PS::ValidateKitbagSlot(static_cast<int>(sToGridID))) {
			LG("Security", "[UseItem] Invalid to grid %d from character %s\n", sToGridID, GetName());
			break;
		}

		Short sRet = Cmd_UseItem(0, sFromGridID, 0, sToGridID);
		if (sRet != enumITEMOPT_SUCCESS)
			ItemOprateFailed(sRet);
	} break;
	case enumACTION_ITEM_UNFIX: // ????
	{
		m_CChaAttr.ResetChangeFlag();

		Char chDir;
		Long lParam1, lParam2;

		Char chLinkID;
		Short sGridID;
		uChar chLinkIDRaw = 0;
		uShort sGridIDRaw = 0;
		if (!reader.Char(chLinkIDRaw)) {
			return;
		}
		if (!reader.Short(sGridIDRaw)) {
			return;
		}
		chLinkID = static_cast<Char>(chLinkIDRaw);
		sGridID = static_cast<Short>(sGridIDRaw);
		if (sGridID == -2) // ????
		{
			chDir = 0;
			uLong lParam1Raw = 0;
			uLong lParam2Raw = 0;
			if (!reader.Long(lParam1Raw)) {
				return;
			}
			if (!reader.Long(lParam2Raw)) {
				return;
			}
			lParam1 = static_cast<Long>(lParam1Raw);
			lParam2 = static_cast<Long>(lParam2Raw);
		} else if (sGridID == -1) // ?????,????
		{
			chDir = 1;
			lParam1 = 0;
			lParam2 = -1;
		} else if (sGridID >= 0) // ?????,????
		{
			chDir = 1;
			lParam1 = 0;
			lParam2 = sGridID;
		}

		// printf("chLinkID: %d\n", chLinkID);
		// printf("sGridID: %d\n", sGridID);

		Short sUnfixNum = 0;
		Short sRet = Cmd_UnfixItem(chLinkID, &sUnfixNum, chDir, lParam1, lParam2);
		if (sRet != enumITEMOPT_SUCCESS)
			ItemOprateFailed(sRet);
	} break;
	case enumACTION_ITEM_POS: // ??????
	{
		uShort sSrcGridRaw = 0;
		uShort sSrcNumRaw = 0;
		uShort sTarGridRaw = 0;
		if (!reader.Short(sSrcGridRaw)) {
			return;
		}
		if (!reader.Short(sSrcNumRaw)) {
			return;
		}
		if (!reader.Short(sTarGridRaw)) {
			return;
		}
		Short sSrcGrid = static_cast<Short>(sSrcGridRaw);
		Short sSrcNum = static_cast<Short>(sSrcNumRaw);
		Short sTarGrid = static_cast<Short>(sTarGridRaw);
		
		// SANITIZE: Validate grid IDs and count
		if (!PS::ValidateKitbagSlot(static_cast<int>(sSrcGrid)) || 
		    !PS::ValidateKitbagSlot(static_cast<int>(sTarGrid))) {
			LG("Security", "[ItemPos] Invalid grid src=%d tar=%d from character %s\n", sSrcGrid, sTarGrid, GetName());
			break;
		}
		if (sSrcNum < 0 || sSrcNum > PS::MAX_STACK_COUNT) {
			LG("Security", "[ItemPos] Invalid count %d from character %s\n", sSrcNum, GetName());
			break;
		}

		Short sRet = Cmd_ItemSwitchPos(0, sSrcGrid, sSrcNum, sTarGrid);
		if (sRet != enumITEMOPT_SUCCESS)
			ItemOprateFailed(sRet);
	} break;
	case enumACTION_KITBAGTMP_DRAG: // ??????
	{
		uShort sSrcGridRaw = 0;
		uShort sSrcNumRaw = 0;
		uShort sTarGridRaw = 0;
		if (!reader.Short(sSrcGridRaw)) {
			return;
		}
		if (!reader.Short(sSrcNumRaw)) {
			return;
		}
		if (!reader.Short(sTarGridRaw)) {
			return;
		}
		Short sSrcGrid = static_cast<Short>(sSrcGridRaw);
		Short sSrcNum = static_cast<Short>(sSrcNumRaw);
		Short sTarGrid = static_cast<Short>(sTarGridRaw);
		
		// SANITIZE: Validate grid IDs and count
		if (!PS::ValidateKitbagSlot(static_cast<int>(sSrcGrid)) || 
		    !PS::ValidateKitbagSlot(static_cast<int>(sTarGrid))) {
			LG("Security", "[DragItem] Invalid grid src=%d tar=%d from character %s\n", sSrcGrid, sTarGrid, GetName());
			break;
		}
		if (sSrcNum < 0 || sSrcNum > PS::MAX_STACK_COUNT) {
			LG("Security", "[DragItem] Invalid count %d from character %s\n", sSrcNum, GetName());
			break;
		}

		Short sRet = Cmd_DragItem(sSrcGrid, sSrcNum, sTarGrid);
		if (sRet != enumITEMOPT_SUCCESS)
			ItemOprateFailed(sRet);
	} break;
	case enumACTION_ITEM_DELETE: // ????
	{
		uShort sFromGridIDRaw = 0;
		if (!reader.Short(sFromGridIDRaw)) {
			return;
		}
		Short sFromGridID = static_cast<Short>(sFromGridIDRaw);
		
		// SANITIZE: Validate grid ID
		if (!PS::ValidateKitbagSlot(static_cast<int>(sFromGridID))) {
			LG("Security", "[DeleteItem] Invalid grid %d from character %s\n", sFromGridID, GetName());
			break;
		}
		
		Short sOptNum = 0;
		Short sRet = Cmd_DelItem(0, sFromGridID, &sOptNum);
		if (sRet != enumITEMOPT_SUCCESS)
			ItemOprateFailed(sRet);
	} break;
	case enumACTION_ITEM_INFO: // ????
	{
		ViewItemInfo(reader.Raw());
	} break;
	case enumACTION_BANK:

	{
		if (IsEconomyBlockedByDB(*this))
			break;
		uChar chSrcTypeRaw = 0;
		uShort sSrcGridRaw = 0;
		uShort sSrcNumRaw = 0;
		uChar chTarTypeRaw = 0;
		uShort sTarGridRaw = 0;
		if (!reader.Char(chSrcTypeRaw)) {
			return;
		}
		if (!reader.Short(sSrcGridRaw)) {
			return;
		}
		if (!reader.Short(sSrcNumRaw)) {
			return;
		}
		if (!reader.Char(chTarTypeRaw)) {
			return;
		}
		if (!reader.Short(sTarGridRaw)) {
			return;
		}
		Char chSrcType = static_cast<Char>(chSrcTypeRaw);
		Short sSrcGrid = static_cast<Short>(sSrcGridRaw);
		Short sSrcNum = static_cast<Short>(sSrcNumRaw);
		Char chTarType = static_cast<Char>(chTarTypeRaw);
		Short sTarGrid = static_cast<Short>(sTarGridRaw);
		Short sRet;
		
		// SANITIZE: Validate bank operation parameters
		// Type: 0=kitbag, 1=bank, 2=temp kitbag
		if (!PS::ValidateRange(static_cast<int>(chSrcType), 0, 2) || 
		    !PS::ValidateRange(static_cast<int>(chTarType), 0, 2)) {
			LG("Security", "[Bank] Invalid type src=%d tar=%d from character %s\n", chSrcType, chTarType, GetName());
			break;
		}
		if (sSrcGrid < -1 || sSrcGrid > PS::MAX_KITBAG_SLOTS ||
		    sTarGrid < -1 || sTarGrid > PS::MAX_KITBAG_SLOTS) {
			LG("Security", "[Bank] Invalid grid src=%d tar=%d from character %s\n", sSrcGrid, sTarGrid, GetName());
			break;
		}
		if (sSrcNum < 0 || sSrcNum > PS::MAX_STACK_COUNT) {
			LG("Security", "[Bank] Invalid count %d from character %s\n", sSrcNum, GetName());
			break;
		}

		sRet = Cmd_BankOper(chSrcType, sSrcGrid, sSrcNum, chTarType, sTarGrid);

		if (sRet != enumITEMOPT_SUCCESS)
			ItemOprateFailed(sRet);
	} break;
	case enumACTION_CLOSE_BANK: {
		GetPlayer()->CloseBank();
	} break;
	case enumACTION_REQUESTGUILDBANK: {
		if (GetGuildID() == 0) {
			return;
		}
		
		// Check if player is in a safezone (enumAREA_TYPE_NOT_FIGHT = 0x0002)
		if (!IsInArea(enumAREA_TYPE_NOT_FIGHT)) {
			SystemNotice("Must be in a safe zone to use the guild bank.");
			return;
		}

		// Rate limit guild bank DB operations (1s cooldown)
		{
			DWORD dwNow = GetTickCount();
			if (dwNow - m_dwLastGuildBankTime < 1000) break;
			m_dwLastGuildBankTime = dwNow;
		}
		
		GetPlayer()->OpenGuildBank();
		GetPlayer()->GetGuildGold();
		break;
	}
	case enumACTION_UPDATEGUILDLOGS: {
		int guildID = GetGuildID();
		if (guildID == 0) {
			return;
		}

		// Rate limit guild log DB queries (1s cooldown)
		{
			DWORD dwNow = GetTickCount();
			if (dwNow - m_dwLastGuildBankTime < 1000) break;
			m_dwLastGuildBankTime = dwNow;
		}

		std::vector<CTableGuild::BankLog> logs = game_db.GetGuildLog(guildID);
		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_NOTIACTION);
		WRITE_LONG(WtPk, m_ID);
		WRITE_LONG(WtPk, ulPacketId);
		WRITE_CHAR(WtPk, enumACTION_UPDATEGUILDLOGS);
		WRITE_SHORT(WtPk, logs.size());
		// WRITE_SHORT(WtPk, oldsize);
		//  User is clicking the tab, fetch only the latest 13 operations
		for (int i = 1; i <= 13; i++) {
			if (i > logs.size()) { // We reached the end, send signal to stop
				WRITE_SHORT(WtPk, 9);
				break;
			}
			WRITE_SHORT(WtPk, logs.at(logs.size() - i).type);
			WRITE_LONGLONG(WtPk, logs.at(logs.size() - i).time);
			WRITE_LONGLONG(WtPk, logs.at(logs.size() - i).parameter);
			WRITE_SHORT(WtPk, logs.at(logs.size() - i).quantity);
			WRITE_SHORT(WtPk, logs.at(logs.size() - i).userID);
		}

		ReflectINFof(this, WtPk);
		break;
	}
	case enumACTION_REQUESTGUILDLOGS: {
		int guildID = GetGuildID();
		if (guildID == 0) {
			return;
		}

		// Rate limit guild log DB queries (1s cooldown)
		{
			DWORD dwNow = GetTickCount();
			if (dwNow - m_dwLastGuildBankTime < 1000) break;
			m_dwLastGuildBankTime = dwNow;
		}

		std::vector<CTableGuild::BankLog> logs = game_db.GetGuildLog(guildID);

		uShort curSizeRaw = 0;
		if (!reader.Short(curSizeRaw)) {
			return;
		}
		uShort curSize = curSizeRaw;

		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_NOTIACTION);
		WRITE_LONG(WtPk, m_ID);
		WRITE_LONG(WtPk, ulPacketId);
		WRITE_CHAR(WtPk, enumACTION_REQUESTGUILDLOGS);

		for (int i = 1; i <= 13; i++) {
			// Send latest 13 logs to client
			if ((int)(curSize + i) > logs.size()) { // We reached the end of logs before fetching those 13 logs, send stop parameter
				WRITE_SHORT(WtPk, 9);
				break;
			}
			WRITE_SHORT(WtPk, logs.at(logs.size() - curSize - i).type);
			WRITE_LONGLONG(WtPk, logs.at(logs.size() - curSize - i).time);
			WRITE_LONGLONG(WtPk, logs.at(logs.size() - curSize - i).parameter);
			WRITE_SHORT(WtPk, logs.at(logs.size() - curSize - i).quantity);
			WRITE_SHORT(WtPk, logs.at(logs.size() - curSize - i).userID);
		}

		ReflectINFof(this, WtPk);
		break;
	}
	case enumACTION_SHORTCUT: {
		uChar chIndexRaw = 0;
		uChar chTypeRaw = 0;
		uShort sGridRaw = 0;
		if (!reader.Char(chIndexRaw)) {
			return;
		}
		if (!reader.Char(chTypeRaw)) {
			return;
		}
		if (!reader.Short(sGridRaw)) {
			return;
		}
		char chIndex = static_cast<char>(chIndexRaw);
		char chType = static_cast<char>(chTypeRaw);
		short sGrid = static_cast<short>(sGridRaw);

		if (chIndex < 0 || chIndex >= SHORT_CUT_NUM)
			break;
		m_CShortcut.chType[chIndex] = chType;
		m_CShortcut.byGridID[chIndex] = sGrid;
	} break;
	case enumACTION_LOOK: {
		// m_SChaPart.sTypeID = READ_SHORT(pk);
		// for (int i = 0; i < enumEQUIP_NUM; i++)
		//	m_SChaPart.SLink[i].sID = READ_SHORT(pk);

		//// ??
		// WPACKET WtPk	=GETWPACKET();
		// WRITE_CMD(WtPk, CMD_MC_NOTIACTION);	//????
		// WRITE_LONG(WtPk, m_ID);
		// WRITE_LONG(WtPk, ulPacketId);
		// WRITE_CHAR(WtPk, enumACTION_LOOK);
		// WRITE_SHORT(WtPk, m_SChaPart.sTypeID);
		// for (int i = 0; i < enumEQUIP_NUM; i++)
		//	WRITE_SHORT(WtPk, m_SChaPart.sLink[i]);
		// NotiChgToEyeshot(WtPk);//??
	} break;
	case enumACTION_TEMP: {
		// Rate limit broadcast actions to prevent network saturation
		{
			DWORD dwNow = GetTickCount();
			if (dwNow - m_dwLastBroadcastTime < 200) break;
			m_dwLastBroadcastTime = dwNow;
		}

		uLong tempItemIDRaw = 0;
		uLong tempPartIDRaw = 0;
		if (!reader.Long(tempItemIDRaw)) {
			return;
		}
		if (!reader.Long(tempPartIDRaw)) {
			return;
		}
		short tempItemID = static_cast<short>(tempItemIDRaw);
		short tempPartID = static_cast<short>(tempPartIDRaw);
		
		// Security validation: Check that item ID exists in ItemInfo
		// This prevents attackers from broadcasting invalid IDs that crash other clients
		if (tempItemID != 0) {
			CItemRecord* pItemRec = GetItemRecordInfo(tempItemID);
			if (!pItemRec) {
				LG("security", "[ALERT] Character '%s' sent invalid temp item ID %d - blocking broadcast\n", 
					GetName(), tempItemID);
				break;  // Don't broadcast invalid data
			}
		}
		
		// Validate part ID is within reasonable range
		if (tempPartID < 0 || tempPartID > 10000) {
			LG("security", "[ALERT] Character '%s' sent invalid temp part ID %d - blocking broadcast\n", 
				GetName(), tempPartID);
			break;  // Don't broadcast invalid data
		}
		
		// Block temp appearance on blind chaos maps — would reveal real gear identity
		if (g_Config.m_bBlindChaos && IsPlayerCha() && IsPKSilver())
			break;
		
		m_STempChaPart.sItemID = tempItemID;
		m_STempChaPart.sPartID = tempPartID;

		// ??
		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_NOTIACTION); // ????
		WRITE_LONG(WtPk, m_ID);
		WRITE_LONG(WtPk, ulPacketId);
		WRITE_CHAR(WtPk, enumACTION_TEMP);
		WRITE_LONG(WtPk, m_STempChaPart.sItemID);
		WRITE_LONG(WtPk, m_STempChaPart.sPartID);

		NotiChgToEyeshot(WtPk); // ??
	} break;
	case enumACTION_EVENT: {
		// Rate limit NPC/event interactions to prevent Lua VM saturation
		{
			DWORD dwNow = GetTickCount();
			if (dwNow - m_dwLastNpcInteractTime < 300) break;
			m_dwLastNpcInteractTime = dwNow;
		}

		uLong lIDRaw = 0;
		uLong lHandleRaw = 0;
		if (!reader.Long(lIDRaw)) {
			return;
		}
		if (!reader.Long(lHandleRaw)) {
			return;
		}
		Long lID = static_cast<Long>(lIDRaw);
		Long lHandle = static_cast<Long>(lHandleRaw);
		Entity* pCObj = g_pGameApp->IsLiveingEntity(lID, lHandle);
		if (!pCObj) {
			// m_CLog.Log("?????????\n");
			m_CLog.Log("it inexistent this entity in this map");
			break;
		}
		uShort usEventIDRaw = 0;
		if (!reader.Short(usEventIDRaw)) {
			return;
		}
		uShort usEventID = usEventIDRaw;
		ExecuteEvent(pCObj, usEventID);
	} break;
	case enumACTION_FACE: {
		// Rate limit broadcast actions to prevent network saturation
		{
			DWORD dwNow = GetTickCount();
			if (dwNow - m_dwLastBroadcastTime < 200) break;
			m_dwLastBroadcastTime = dwNow;
		}

		uShort sAngleRaw = 0;
		uShort sPoseRaw = 0;
		if (!reader.Short(sAngleRaw)) {
			return;
		}
		if (!reader.Short(sPoseRaw)) {
			return;
		}
		Short sAngle = static_cast<Short>(sAngleRaw);
		Short sPose = static_cast<Short>(sPoseRaw);

		// ??
		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_NOTIACTION); // ????
		WRITE_LONG(WtPk, m_ID);
		WRITE_LONG(WtPk, ulPacketId);
		WRITE_CHAR(WtPk, enumACTION_FACE);
		WRITE_SHORT(WtPk, sAngle);
		WRITE_SHORT(WtPk, sPose);
		NotiChgToEyeshot(WtPk); // ??
	} break;
	case enumACTION_SKILL_POSE: {
		if (!GetSubMap()) {
			m_CLog.Log("$$$PacketID:\t%u\n", m_ulPacketID);
			// m_CLog.Log("??????(????)\n\n");
			m_CLog.Log("refuse action request(map is null)\n\n");
			return;
		}

		// Rate limit broadcast actions to prevent network saturation
		{
			DWORD dwNow = GetTickCount();
			if (dwNow - m_dwLastBroadcastTime < 200) break;
			m_dwLastBroadcastTime = dwNow;
		}

		if (IsBoat())
			break;
		if (GetMoveState() == enumMSTATE_ON || GetFightState() == enumFSTATE_ON || !GetActControl(enumACTCONTROL_MOVE))
			break;

		uShort sAngleRaw = 0;
		uShort sPoseRaw = 0;
		if (!reader.Short(sAngleRaw)) {
			return;
		}
		if (!reader.Short(sPoseRaw)) {
			return;
		}
		Short sAngle = static_cast<Short>(sAngleRaw);
		Short sPose = static_cast<Short>(sPoseRaw);

		// ??
		WPACKET WtPk = GETWPACKET();
		WRITE_CMD(WtPk, CMD_MC_NOTIACTION); // ????
		WRITE_LONG(WtPk, m_ID);
		WRITE_LONG(WtPk, ulPacketId);
		WRITE_CHAR(WtPk, enumACTION_SKILL_POSE);
		WRITE_SHORT(WtPk, sAngle);
		WRITE_SHORT(WtPk, sPose);
		NotiChgToEyeshot(WtPk); // ??

		bool bToSeat = g_IsSeatPose(sPose);
		if ((bToSeat && m_SSeat.chIsSeat) || (!bToSeat && !m_SSeat.chIsSeat))
			break;

		// ????(??????)
		dbc::uLong ulSkillID = 202;
		CSkillRecord* pCSkill = GetSkillRecordInfo(ulSkillID);
		if (!pCSkill) {
			// m_CLog.Log("?????(????: %d)\n", ulSkillID);
			m_CLog.Log("skills inexistence(skills number: %d)\n", ulSkillID);
			break;
		}

		if (bToSeat) // ??
		{
			m_SSeat.chIsSeat = 1;
			m_SSeat.sAngle = sAngle;
			m_SSeat.sPose = sPose;
			g_CParser.DoString(pCSkill->szActive, enumSCRIPT_RETURN_NONE, 0, enumSCRIPT_PARAM_LIGHTUSERDATA, 1, this, enumSCRIPT_PARAM_NUMBER, 1, 1, DOSTRING_PARAM_END);
		} else // ??
		{
			m_SSeat.chIsSeat = 0;
			g_CParser.DoString(pCSkill->szInactive, enumSCRIPT_RETURN_NONE, 0, enumSCRIPT_PARAM_LIGHTUSERDATA, 1, this, enumSCRIPT_PARAM_NUMBER, 1, 1, DOSTRING_PARAM_END);
		}
		if (bToSeat)
			m_sPoseState = enumPoseSeat;
		else
			m_sPoseState = enumPoseStand;
	} break;
	case enumACTION_PK_CTRL: {
		uChar pkModeRaw = 0;
		if (!reader.Char(pkModeRaw)) {
			return;
		}
		if (pkModeRaw)
			Cmd_SetInPK();
		else
			Cmd_SetInPK(false);
		SynPKCtrl();
	} break;
	default:
		break;
	}
	T_E
}

// ?? : ?????????
void CCharacter::Cmd_ChangeHair(RPACKET& pk) {
	T_B char szRes[128];

	net::PacketReader reader(pk);
	uShort sScriptIDRaw = 0;
	if (!reader.Short(sScriptIDRaw)) {
		return;
	}
	short sScriptID = static_cast<short>(sScriptIDRaw);

	TradeAction(false); // ??????, ????????
	HairAction(false);	// ??????

	if (sScriptID == 0) // ??????
	{
		return;
	}

	if (m_CKitbag.IsPwdLocked()) {
		// sprintf(szRes, "????????, ?????");
		sprintf(szRes, RES_STRING(GM_CHARACTERPRL_CPP_00042));
		Prl_ChangeHairResult(0, szRes);
		return;
	}

	CHairRecord* pHair = GetHairRecordInfo(sScriptID);
	if (!pHair) {
		// sprintf(szRes, "????????, ????ID = %d", sScriptID);
		sprintf(szRes, RES_STRING(GM_CHARACTERPRL_CPP_00043), sScriptID);
		Prl_ChangeHairResult(0, szRes);
		return;
	}

	short sValidCnt = 0;
	short sValidGrid[defHAIR_MAX_ITEM][3];

	for (short i = 0; i < defHAIR_MAX_ITEM; i++) {
		short sNeedItemID = (short)(pHair->dwNeedItem[i][0]);
		if (sNeedItemID > 0) {
			BOOL bOK = TRUE;
			uShort sGridLocRaw = 0;
			if (!reader.Short(sGridLocRaw)) {
				sprintf(szRes, RES_STRING(GM_CHARACTERPRL_CPP_00044));
				Prl_ChangeHairResult(0, szRes);
				return;
			}
			short sGridLoc = static_cast<short>(sGridLocRaw);
			if (sGridLoc == -1)
				bOK = FALSE;

			if (bOK) {
				// ??????????????
				short sNowItemID = m_CKitbag.GetID(sGridLoc);
				if (sNowItemID != sNeedItemID) {
					bOK = FALSE;
				}
			}

			if (!bOK) {
				// sprintf(szRes, "??????, ???????");
				sprintf(szRes, RES_STRING(GM_CHARACTERPRL_CPP_00044));
				Prl_ChangeHairResult(0, szRes);
				return;
			}
			sValidGrid[sValidCnt][0] = sGridLoc;
			sValidGrid[sValidCnt][1] = sNeedItemID;
			sValidGrid[sValidCnt][2] = (short)(pHair->dwNeedItem[i][1]); // ????
			sValidCnt++;
		}
	}

	// ???????, ????
	m_CKitbag.SetChangeFlag(false);
	/*if(!TakeMoney("???", pHair->dwMoney))
	{
		SystemNotice("??????, ????!");
		return;
	}*/
	if (!TakeMoney(RES_STRING(GM_CHARACTERPRL_CPP_00045), pHair->dwMoney)) {
		SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00046));
		return;
	}

	SItemGrid item;
	for (short i = 0; i < sValidCnt; i++) {
		item.sID = sValidGrid[i][1];
		item.sNum = sValidGrid[i][2];

		short sRet = KbPopItem(true, false, &item, sValidGrid[i][0]);
		if (sRet != enumKBACT_SUCCESS) {
			// SystemNotice("??????, ????????????????!");

			SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00047));
			return;
		}
	}

	// ????????
	SynKitbagNew(enumSYN_KITBAG_FROM_NPC);

	// ??????, ????????

	SetLookChangeFlag(true);
	// 10%??????????
	if (rand() % 100 < 10 && pHair->GetFailItemNum() > 0) {
		int nRandFail = rand() % pHair->GetFailItemNum();
		short sFailHair = (short)(pHair->dwFailItemID[nRandFail]);
		m_SChaPart.sHairID = sFailHair;
		// SystemNotice("???????, ?????!");
		SystemNotice(RES_STRING(GM_CHARACTERPRL_CPP_00048));
		Prl_ChangeHairResult(sScriptID, "fail", true);
	} else {
		// ??????, ??????
		m_SChaPart.sHairID = (short)(pHair->dwItemID); // ????
		Prl_ChangeHairResult(sScriptID, "ok", true);
	}

	// ?????????
	if (g_Config.m_bBlindChaos && IsPlayerCha() && IsPKSilver())
		SynLook(LOOK_SELF, true); // sync to self (changing hair)
	else
		SynLook();
	T_E
}

// ???????
// ??1 : ??ID, ????0
// ??2 : ????????
void CCharacter::Prl_ChangeHairResult(int nScriptID, const char* szReason, BOOL bNoticeAll) {
	T_B
		WPACKET wpk = GETWPACKET();
	WRITE_CMD(wpk, CMD_MC_UPDATEHAIR_RES);
	WRITE_LONG(wpk, GetID());
	WRITE_SHORT(wpk, nScriptID);
	WRITE_STRING(wpk, szReason);
	if (bNoticeAll) {
		NotiChgToEyeshot(wpk); // ??
	} else {
		ReflectINFof(this, wpk);
	}
	T_E
}

// ???????????
void CCharacter::Prl_OpenHair() {
	T_B
		HairAction(true);

	WPACKET wpk = GETWPACKET();
	WRITE_CMD(wpk, CMD_MC_OPENHAIR);
	ReflectINFof(this, wpk);

	T_E
}
