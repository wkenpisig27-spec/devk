#pragma once

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>

#include "gameapp.h"
#include "scene.h"
#include "character.h"
#include "MapSet.h"
#include "MonsterSet.h"
#include "FindPath.h"
#include "EffectObj.h"
#include "NPCHelper.h"
#include <characterrecord.h>

// Centralized auto-path entry point used by UI modules.
enum class EAutoPathError {
	None = 0,
	InvalidLink,
	SceneUnavailable,
	InBoatMode,
	DifferentMap,
	InvalidCoordinates,
};

struct SAutoPathResult {
	bool success = false;
	EAutoPathError error = EAutoPathError::None;
	std::string message;
	std::string resolvedMap;
	int resolvedX = 0;
	int resolvedY = 0;
};

class CAutoPathService {
public:
	static SAutoPathResult NavigateFromText(const std::string& clickedText) {
		std::string text = TrimCopy(clickedText);
		if (text.empty()) {
			return Fail(EAutoPathError::InvalidLink, "Empty route link");
		}

		std::string navPayload;
		if (TryExtractNavPayload(text, navPayload)) {
			return NavigateFromPayload(navPayload);
		}

		std::string targetMapName;
		int tx = -1;
		int ty = -1;

		if (TryResolveNpcLocation(text, targetMapName, tx, ty)) {
			return NavigateTo(targetMapName, tx, ty);
		}

		if (TryResolveMonsterLocationByName(text, targetMapName, tx, ty)) {
			return NavigateTo(targetMapName, tx, ty);
		}

		size_t leftParenPos = std::string::npos;
		if (!TryExtractCoordinatePair(text, tx, ty, leftParenPos)) {
			return Fail(EAutoPathError::InvalidLink, "No valid route payload found");
		}

		std::string mapOrNpc = StripTrailingAt(text.substr(0, leftParenPos));
		if (!TryResolveMapName(mapOrNpc, targetMapName)) {
			if (CGameScene* pScene = dynamic_cast<CGameScene*>(CGameApp::GetCurScene())) {
				if (auto* pMapInfo = pScene->GetCurMapInfo()) {
					targetMapName = pMapInfo->szName;
				}
			}
		}

		return NavigateTo(targetMapName, tx, ty);
	}

	static SAutoPathResult NavigateFromPayload(const std::string& payloadText) {
		std::string payload = TrimCopy(payloadText);
		if (payload.empty()) {
			return Fail(EAutoPathError::InvalidLink, "Empty route payload");
		}

		if (_strnicmp(payload.c_str(), "nav:", 4) == 0) {
			payload = payload.substr(4);
		}

		// Support author-friendly format: <nav:NPC Name>
		if (payload.find(':') == std::string::npos) {
			std::string targetMap;
			int tx = 0;
			int ty = 0;
			if (!TryResolveNpcLocation(payload, targetMap, tx, ty)) {
				if (!TryResolveMonsterLocationByName(payload, targetMap, tx, ty)) {
					return Fail(EAutoPathError::InvalidLink, "NPC/Monster not found");
				}
			}
			return NavigateTo(targetMap, tx, ty);
		}

		std::vector<std::string> tokens = Split(payload, ':');
		if (tokens.empty()) {
			return Fail(EAutoPathError::InvalidLink, "Invalid route payload");
		}

		if (_stricmp(tokens[0].c_str(), "coord") == 0) {
			if (tokens.size() < 3) {
				return Fail(EAutoPathError::InvalidLink, "Invalid coord payload");
			}
			int tx = 0;
			int ty = 0;
			if (!TryParseInt(tokens[1], tx) || !TryParseInt(tokens[2], ty)) {
				return Fail(EAutoPathError::InvalidLink, "Invalid coord values");
			}
			return NavigateTo("", tx, ty);
		}

		if (_stricmp(tokens[0].c_str(), "npc") == 0) {
			if (tokens.size() < 2) {
				return Fail(EAutoPathError::InvalidLink, "Invalid npc payload");
			}
			int npcId = 0;
			if (!TryParseInt(tokens[1], npcId)) {
				return Fail(EAutoPathError::InvalidLink, "Invalid npc id");
			}

			std::string targetMap;
			int tx = 0;
			int ty = 0;
			if (!TryResolveNpcLocationById(npcId, targetMap, tx, ty)) {
				return Fail(EAutoPathError::InvalidLink, "NPC not found");
			}
			return NavigateTo(targetMap, tx, ty);
		}

		if (_stricmp(tokens[0].c_str(), "monster") == 0) {
			if (tokens.size() < 2) {
				return Fail(EAutoPathError::InvalidLink, "Invalid monster payload");
			}

			int monsterId = 0;
			if (!TryParseInt(tokens[1], monsterId)) {
				return Fail(EAutoPathError::InvalidLink, "Invalid monster id");
			}

			std::string targetMap;
			int tx = 0;
			int ty = 0;
			if (!TryResolveMonsterLocationById(monsterId, targetMap, tx, ty)) {
				return Fail(EAutoPathError::InvalidLink, "Monster location not found");
			}

			return NavigateTo(targetMap, tx, ty);
		}

		if (_stricmp(tokens[0].c_str(), "monstername") == 0) {
			if (tokens.size() < 2) {
				return Fail(EAutoPathError::InvalidLink, "Invalid monstername payload");
			}

			std::string monsterName = Join(tokens, 1, tokens.size(), ":");
			std::string targetMap;
			int tx = 0;
			int ty = 0;
			if (!TryResolveMonsterLocationByName(monsterName, targetMap, tx, ty)) {
				return Fail(EAutoPathError::InvalidLink, "Monster location not found");
			}

			return NavigateTo(targetMap, tx, ty);
		}

		if (_stricmp(tokens[0].c_str(), "map") == 0) {
			if (tokens.size() < 4) {
				return Fail(EAutoPathError::InvalidLink, "Invalid map payload");
			}

			// Support map names that may contain ':' by taking the last two tokens as coordinates.
			int tx = 0;
			int ty = 0;
			if (!TryParseInt(tokens[tokens.size() - 2], tx) || !TryParseInt(tokens[tokens.size() - 1], ty)) {
				return Fail(EAutoPathError::InvalidLink, "Invalid map payload coordinates");
			}

			std::string mapName = Join(tokens, 1, tokens.size() - 2, ":");
			return NavigateTo(mapName, tx, ty);
		}

		return Fail(EAutoPathError::InvalidLink, "Unsupported route payload");
	}

	static SAutoPathResult NavigateTo(const std::string& targetMapName, int tx, int ty) {
		CGameScene* pScene = dynamic_cast<CGameScene*>(CGameApp::GetCurScene());
		if (!pScene || !pScene->GetMainCha() || !pScene->GetCurMapInfo()) {
			return Fail(EAutoPathError::SceneUnavailable, "Scene unavailable");
		}

		if (pScene->GetMainCha()->IsBoat()) {
			return Fail(EAutoPathError::InBoatMode, "No routing on sea");
		}

		if (tx < 0 || tx > 4096 || ty < 0 || ty > 4096) {
			return Fail(EAutoPathError::InvalidCoordinates, "Invalid destination coordinates");
		}

		const char* curmap = pScene->GetCurMapInfo()->szName;
		if (!targetMapName.empty() && !IsSameMapName(targetMapName, curmap) && !IsSameMapName(targetMapName, pScene->GetTerrainName())) {
			return Fail(EAutoPathError::DifferentMap, "Not on the same map");
		}

		const int cx = static_cast<int>(pScene->GetMainCha()->GetPos().x);
		const int cy = static_cast<int>(pScene->GetMainCha()->GetPos().y);

		g_cFindPathEx.Reset();
		g_cFindPathEx.ClearDestDirection();
		g_cFindPathEx.SetDestDirection(cx, cy, tx, ty);
		g_cFindPathEx.SetTarget(cx, cy, tx, ty);

		D3DXVECTOR3 target(static_cast<float>(tx), static_cast<float>(ty), 0);
		CNavigationBar::g_cNaviBar.SetTarget("", target);
		CNavigationBar::g_cNaviBar.Show(true);

		SAutoPathResult result;
		result.success = true;
		result.error = EAutoPathError::None;
		result.message = "OK";
		result.resolvedMap = targetMapName.empty() ? std::string(curmap) : targetMapName;
		result.resolvedX = tx;
		result.resolvedY = ty;
		return result;
	}

private:
	static SAutoPathResult Fail(EAutoPathError error, const char* message) {
		SAutoPathResult result;
		result.success = false;
		result.error = error;
		result.message = message ? message : "";
		return result;
	}

	static bool TryExtractNavPayload(const std::string& text, std::string& outPayload) {
		if (text.empty()) {
			return false;
		}

		if (_strnicmp(text.c_str(), "nav:", 4) == 0) {
			size_t pipePos = text.find('|');
			outPayload = (pipePos == std::string::npos) ? text : text.substr(0, pipePos);
			return true;
		}

		if (_strnicmp(text.c_str(), "<nav:", 5) == 0 && text.back() == '>') {
			std::string raw = text.substr(1, text.size() - 2);
			size_t pipePos = raw.find('|');
			outPayload = (pipePos == std::string::npos) ? raw : raw.substr(0, pipePos);
			return true;
		}

		return false;
	}

	static bool TryParseInt(const std::string& text, int& outValue) {
		try {
			outValue = std::stoi(text);
			return true;
		} catch (...) {
			return false;
		}
	}

	static std::vector<std::string> Split(const std::string& text, char delimiter) {
		std::vector<std::string> result;
		std::string current;
		for (size_t i = 0; i < text.size(); ++i) {
			if (text[i] == delimiter) {
				result.push_back(current);
				current.clear();
			} else {
				current.push_back(text[i]);
			}
		}
		result.push_back(current);
		return result;
	}

	static std::string Join(const std::vector<std::string>& parts, size_t begin, size_t end, const char* delimiter) {
		std::string result;
		for (size_t i = begin; i < end; ++i) {
			if (!result.empty()) {
				result += delimiter;
			}
			result += parts[i];
		}
		return result;
	}

	static std::string TrimCopy(const std::string& value) {
		std::string result = value;
		result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](unsigned char ch) {
			return !std::isspace(ch);
		}));
		result.erase(std::find_if(result.rbegin(), result.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
		}).base(), result.end());
		return result;
	}

	static bool EndsWithWordAt(const std::string& value) {
		if (value.size() < 2) {
			return false;
		}
		if (value.size() == 2) {
			return _stricmp(value.c_str(), "at") == 0;
		}
		return _stricmp(value.c_str() + value.size() - 2, "at") == 0 && std::isspace(static_cast<unsigned char>(value[value.size() - 3]));
	}

	static std::string StripTrailingAt(std::string value) {
		value = TrimCopy(value);
		if (EndsWithWordAt(value)) {
			value = TrimCopy(value.substr(0, value.size() - 2));
		}
		return value;
	}

	static bool TryExtractCoordinatePair(const std::string& input, int& outX, int& outY, size_t& outLeftParenPos) {
		for (size_t leftParen = input.find('('); leftParen != std::string::npos; leftParen = input.find('(', leftParen + 1)) {
			size_t comma = input.find(',', leftParen + 1);
			if (comma == std::string::npos) {
				continue;
			}

			size_t rightParen = input.find(')', comma + 1);
			if (rightParen == std::string::npos) {
				continue;
			}

			std::string xPart = TrimCopy(input.substr(leftParen + 1, comma - leftParen - 1));
			std::string yPart = TrimCopy(input.substr(comma + 1, rightParen - comma - 1));
			if (xPart.empty() || yPart.empty()) {
				continue;
			}

			try {
				outX = std::stoi(xPart);
				outY = std::stoi(yPart);
				outLeftParenPos = leftParen;
				return true;
			} catch (...) {
				continue;
			}
		}

		return false;
	}

	static bool TryResolveNpcLocation(const std::string& npcName, std::string& outMapName, int& outX, int& outY) {
		if (npcName.empty() || !NPCHelper::I()) {
			return false;
		}

		std::string candidate = NormalizeNpcNameCandidate(npcName);
		if (candidate.empty()) {
			return false;
		}

		std::vector<std::string> candidateNames;
		candidateNames.push_back(candidate);

		size_t possessivePos = candidate.find("'s ");
		if (possessivePos != std::string::npos && possessivePos + 3 < candidate.size()) {
			candidateNames.push_back(TrimCopy(candidate.substr(possessivePos + 3)));
		}

		size_t dashPos = candidate.rfind('-');
		if (dashPos != std::string::npos && dashPos + 1 < candidate.size()) {
			std::string trailingName = TrimCopy(candidate.substr(dashPos + 1));
			if (!trailingName.empty()) {
				candidateNames.push_back(trailingName);
			}
		}

		int nTotalIndex = NPCHelper::I()->GetLastID() + 1;
		for (size_t c = 0; c < candidateNames.size(); ++c) {
			const std::string& currentCandidate = candidateNames[c];
			if (currentCandidate.empty()) {
				continue;
			}

			for (int i = 0; i < nTotalIndex; ++i) {
				NPCData* pData = GetNPCDataInfo(i);
				if (!pData) {
					continue;
				}

				if (_stricmp(pData->szName, currentCandidate.c_str()) != 0) {
					continue;
				}

				outMapName = (_stricmp(pData->szName, "jialebi") == 0) ? "Pirate's Base" : SelectNpcMapName(pData);
				outX = static_cast<int>(pData->dwxPos0);
				outY = static_cast<int>(pData->dwyPos0);
				return true;
			}
		}

		for (size_t c = 0; c < candidateNames.size(); ++c) {
			const std::string& currentCandidate = candidateNames[c];
			const std::string candidateKey = BuildNpcMatchKey(currentCandidate);
			if (candidateKey.empty()) {
				continue;
			}

			for (int i = 0; i < nTotalIndex; ++i) {
				NPCData* pData = GetNPCDataInfo(i);
				if (!pData) {
					continue;
				}

				const std::string npcKey = BuildNpcMatchKey(pData->szName);
				if (npcKey.empty()) {
					continue;
				}

				if (candidateKey == npcKey || EndsWithKey(candidateKey, npcKey) || EndsWithKey(npcKey, candidateKey)) {
					outMapName = (_stricmp(pData->szName, "jialebi") == 0) ? "Pirate's Base" : SelectNpcMapName(pData);
					outX = static_cast<int>(pData->dwxPos0);
					outY = static_cast<int>(pData->dwyPos0);
					return true;
				}
			}
		}

		return false;
	}

	static bool EndsWithKey(const std::string& value, const std::string& suffix) {
		if (value.size() < suffix.size()) {
			return false;
		}

		return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
	}

	static bool EqualsIgnoreCase(const std::string& a, const std::string& b) {
		return _stricmp(a.c_str(), b.c_str()) == 0;
	}

	static bool IsLikelyWorldAreaName(const std::string& value) {
		return EqualsIgnoreCase(value, "Ascaron") ||
		       EqualsIgnoreCase(value, "Magical Ocean") ||
		       EqualsIgnoreCase(value, "Deep Blue") ||
		       EqualsIgnoreCase(value, "Winter Island") ||
		       EqualsIgnoreCase(value, "DreamIsland") ||
		       EqualsIgnoreCase(value, "Dream Island");
	}

	static std::string GetCurrentMapName() {
		CGameScene* pScene = dynamic_cast<CGameScene*>(CGameApp::GetCurScene());
		if (!pScene || !pScene->GetCurMapInfo()) {
			return "";
		}

		return pScene->GetCurMapInfo()->szName;
	}

	static std::string GetCurrentTerrainName() {
		CGameScene* pScene = dynamic_cast<CGameScene*>(CGameApp::GetCurScene());
		if (!pScene) {
			return "";
		}

		return pScene->GetTerrainName();
	}

	static std::string BuildMapMatchKey(const std::string& value) {
		std::string key;
		key.reserve(value.size());

		for (size_t i = 0; i < value.size(); ++i) {
			unsigned char ch = static_cast<unsigned char>(value[i]);
			if (std::isalnum(ch)) {
				key.push_back(static_cast<char>(std::tolower(ch)));
			}
		}

		if (key == "garner" || key == "ascaron") {
			return "garner";
		}

		if (key == "magicsea" || key == "magicalocean") {
			return "magicsea";
		}

		if (key == "darkblue" || key == "deepblue") {
			return "darkblue";
		}

		return key;
	}

	static bool IsSameMapName(const std::string& left, const std::string& right) {
		const std::string leftKey = BuildMapMatchKey(TrimCopy(left));
		const std::string rightKey = BuildMapMatchKey(TrimCopy(right));

		if (leftKey.empty() || rightKey.empty()) {
			return false;
		}

		return leftKey == rightKey;
	}

	static bool IsCurrentMapName(const std::string& mapName) {
		const std::string currentMap = GetCurrentMapName();
		if (IsSameMapName(mapName, currentMap)) {
			return true;
		}

		const std::string currentTerrain = GetCurrentTerrainName();
		return IsSameMapName(mapName, currentTerrain);
	}

	static std::string SelectNpcMapName(const NPCData* pData) {
		if (!pData) {
			return "";
		}

		const std::string primary = TrimCopy(pData->szMapName);
		const std::string secondary = TrimCopy(pData->szArea);
		const std::string currentMap = GetCurrentMapName();

		if (!currentMap.empty()) {
			if (!primary.empty() && EqualsIgnoreCase(primary, currentMap)) {
				return primary;
			}
			if (!secondary.empty() && EqualsIgnoreCase(secondary, currentMap)) {
				return secondary;
			}
		}

		if (primary.empty()) {
			return secondary;
		}
		if (secondary.empty()) {
			return primary;
		}

		const bool primaryWorld = IsLikelyWorldAreaName(primary);
		const bool secondaryWorld = IsLikelyWorldAreaName(secondary);

		if (primaryWorld && !secondaryWorld) {
			return secondary;
		}
		if (secondaryWorld && !primaryWorld) {
			return primary;
		}

		return primary;
	}

	static std::string BuildNpcMatchKey(const std::string& value) {
		std::string key;
		key.reserve(value.size());

		for (size_t i = 0; i < value.size(); ++i) {
			unsigned char ch = static_cast<unsigned char>(value[i]);
			if (std::isalnum(ch)) {
				key.push_back(static_cast<char>(std::tolower(ch)));
			}
		}

		return key;
	}

	static void BuildNameKeyVariants(const std::string& key, std::vector<std::string>& outVariants) {
		outVariants.clear();
		if (key.empty()) {
			return;
		}

		outVariants.push_back(key);

		if (key.size() > 3 && key.compare(key.size() - 3, 3, "ies") == 0) {
			outVariants.push_back(key.substr(0, key.size() - 3) + "y");
		}

		if (key.size() > 2 && key.compare(key.size() - 2, 2, "es") == 0) {
			outVariants.push_back(key.substr(0, key.size() - 2));
		}

		if (key.size() > 1 && key[key.size() - 1] == 's') {
			outVariants.push_back(key.substr(0, key.size() - 1));
		}
	}

	static bool IsNameKeyMatch(const std::string& left, const std::string& right) {
		if (left.empty() || right.empty()) {
			return false;
		}

		if (left == right) {
			return true;
		}

		std::vector<std::string> leftVariants;
		std::vector<std::string> rightVariants;
		BuildNameKeyVariants(left, leftVariants);
		BuildNameKeyVariants(right, rightVariants);

		for (size_t i = 0; i < leftVariants.size(); ++i) {
			for (size_t j = 0; j < rightVariants.size(); ++j) {
				if (leftVariants[i] == rightVariants[j]) {
					return true;
				}
			}
		}

		return false;
	}

	static std::string NormalizeNpcNameCandidate(std::string value) {
		value = TrimCopy(value);

		// Remove simple wrappers users may include in quest text.
		while (!value.empty() && (value.front() == '"' || value.front() == '\'' || value.front() == '(' || value.front() == '[')) {
			value.erase(value.begin());
		}
		while (!value.empty() && (value.back() == '"' || value.back() == '\'' || value.back() == ')' || value.back() == ']')) {
			value.pop_back();
		}

		// Ignore trailing punctuation commonly found at sentence boundaries.
		while (!value.empty()) {
			char c = value.back();
			if (c == '.' || c == ',' || c == ';' || c == ':' || c == '!' || c == '?') {
				value.pop_back();
				continue;
			}
			break;
		}

		return TrimCopy(value);
	}

	static bool TryResolveNpcLocationById(int npcId, std::string& outMapName, int& outX, int& outY) {
		if (npcId < 0 || !NPCHelper::I()) {
			return false;
		}

		NPCData* pData = GetNPCDataInfo(npcId);
		if (!pData) {
			return false;
		}

		outMapName = (_stricmp(pData->szName, "jialebi") == 0) ? "Pirate's Base" : SelectNpcMapName(pData);
		outX = static_cast<int>(pData->dwxPos0);
		outY = static_cast<int>(pData->dwyPos0);
		return true;
	}

	static bool TryResolveMonsterLocationById(int monsterId, std::string& outMapName, int& outX, int& outY) {
		if (monsterId <= 0 || !CMonsterSet::I()) {
			return false;
		}

		int nLastId = CMonsterSet::I()->GetLastID();
		std::string currentMap = GetCurrentMapName();
		std::string currentTerrain = GetCurrentTerrainName();

		for (int mapId = 1; mapId <= nLastId; ++mapId) {
			CMonsterInfo* pInfo = GetMonsterInfo(mapId);
			if (!pInfo || !pInfo->bExist) {
				continue;
			}

			for (int i = 0; i < MONSTER_LIST_MAX; ++i) {
				if (pInfo->nMonsterList[i] != monsterId) {
					continue;
				}

				std::string mapName = TrimCopy(pInfo->szArea);
				if (mapName.empty()) {
					mapName = currentMap.empty() ? currentTerrain : currentMap;
				}

				if (!currentMap.empty() && !currentTerrain.empty() && !IsSameMapName(mapName, currentMap) && !IsSameMapName(mapName, currentTerrain)) {
					continue;
				}

				outMapName = currentMap.empty() ? mapName : currentMap;
				if (outMapName.empty()) {
					outMapName = currentTerrain;
				}
				outX = (pInfo->ptStart.x + pInfo->ptEnd.x) / 2;
				outY = (pInfo->ptStart.y + pInfo->ptEnd.y) / 2;
				return true;
			}
		}

		// Fallback: first known spawn region if current map filtering didn't match.
		for (int mapId = 1; mapId <= nLastId; ++mapId) {
			CMonsterInfo* pInfo = GetMonsterInfo(mapId);
			if (!pInfo || !pInfo->bExist) {
				continue;
			}

			for (int i = 0; i < MONSTER_LIST_MAX; ++i) {
				if (pInfo->nMonsterList[i] == monsterId) {
					outMapName = TrimCopy(pInfo->szArea);
					if (IsCurrentMapName(outMapName)) {
						outMapName = currentMap.empty() ? outMapName : currentMap;
					}
					outX = (pInfo->ptStart.x + pInfo->ptEnd.x) / 2;
					outY = (pInfo->ptStart.y + pInfo->ptEnd.y) / 2;
					return true;
				}
			}
		}

		return false;
	}

	static bool TryResolveMonsterLocationByName(const std::string& monsterName, std::string& outMapName, int& outX, int& outY) {
		if (monsterName.empty() || !CMonsterSet::I()) {
			return false;
		}

		std::string candidate = NormalizeNpcNameCandidate(monsterName);
		if (candidate.empty()) {
			return false;
		}
		const std::string candidateKey = BuildNpcMatchKey(candidate);

		const std::string currentMap = GetCurrentMapName();
		const std::string currentTerrain = GetCurrentTerrainName();
		int nLastId = CMonsterSet::I()->GetLastID();

		for (int mapId = 1; mapId <= nLastId; ++mapId) {
			CMonsterInfo* pInfo = GetMonsterInfo(mapId);
			if (!pInfo || !pInfo->bExist) {
				continue;
			}

			std::string area = TrimCopy(pInfo->szArea);
			if (!currentMap.empty() && !currentTerrain.empty() && !area.empty() && !IsSameMapName(area, currentMap) && !IsSameMapName(area, currentTerrain)) {
				continue;
			}

			for (int i = 0; i < MONSTER_LIST_MAX; ++i) {
				int monsterId = pInfo->nMonsterList[i];
				if (monsterId <= 0) {
					continue;
				}

				CChaRecord* pMonster = GetChaRecordInfo(monsterId);
				if (!pMonster) {
					continue;
				}

				const std::string monsterKey = BuildNpcMatchKey(pMonster->szName);
				if (_stricmp(pMonster->szName, candidate.c_str()) == 0 || IsNameKeyMatch(candidateKey, monsterKey)) {
					outMapName = currentMap.empty() ? area : currentMap;
					if (outMapName.empty()) {
						outMapName = currentTerrain;
					}
					outX = (pInfo->ptStart.x + pInfo->ptEnd.x) / 2;
					outY = (pInfo->ptStart.y + pInfo->ptEnd.y) / 2;
					return true;
				}
			}
		}

		for (int mapId = 1; mapId <= nLastId; ++mapId) {
			CMonsterInfo* pInfo = GetMonsterInfo(mapId);
			if (!pInfo || !pInfo->bExist) {
				continue;
			}

			for (int i = 0; i < MONSTER_LIST_MAX; ++i) {
				int monsterId = pInfo->nMonsterList[i];
				if (monsterId <= 0) {
					continue;
				}

				CChaRecord* pMonster = GetChaRecordInfo(monsterId);
				if (!pMonster) {
					continue;
				}

				const std::string monsterKey = BuildNpcMatchKey(pMonster->szName);
				if (_stricmp(pMonster->szName, candidate.c_str()) == 0 || IsNameKeyMatch(candidateKey, monsterKey)) {
					outMapName = TrimCopy(pInfo->szArea);
					if (IsCurrentMapName(outMapName)) {
						outMapName = currentMap.empty() ? outMapName : currentMap;
					}
					outX = (pInfo->ptStart.x + pInfo->ptEnd.x) / 2;
					outY = (pInfo->ptStart.y + pInfo->ptEnd.y) / 2;
					return true;
				}
			}
		}

		return false;
	}

	static bool TryResolveMapName(const std::string& mapOrNpcName, std::string& outMapName) {
		if (mapOrNpcName.empty()) {
			return false;
		}

		if (_stricmp(mapOrNpcName.c_str(), RES_STRING(CL_LANGUAGE_MATCH_56)) == 0) {
			outMapName = RES_STRING(CL_LANGUAGE_MATCH_56);
			return true;
		}

		if (_stricmp(mapOrNpcName.c_str(), RES_STRING(CL_LANGUAGE_MATCH_57)) == 0) {
			outMapName = RES_STRING(CL_LANGUAGE_MATCH_57);
			return true;
		}

		if (_stricmp(mapOrNpcName.c_str(), RES_STRING(CL_LANGUAGE_MATCH_58)) == 0) {
			outMapName = RES_STRING(CL_LANGUAGE_MATCH_58);
			return true;
		}

		if (_stricmp(mapOrNpcName.c_str(), "Winter Isle Archipelago") == 0) {
			outMapName = "Winter Isle Archipelago";
			return true;
		}

		int unusedX = 0;
		int unusedY = 0;
		return TryResolveNpcLocation(mapOrNpcName, outMapName, unusedX, unusedY);
	}
};
