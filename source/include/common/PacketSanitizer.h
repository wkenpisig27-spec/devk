#pragma once
/**
 * PacketSanitizer.h - Comprehensive packet validation and sanitization utilities
 * 
 * Purpose: Prevent exploits by validating ALL incoming packet data before processing.
 * 
 * Usage:
 *   #include "PacketSanitizer.h"
 *   
 *   // In packet handler:
 *   if (!PacketSanitizer::ValidatePacketSize(pk, 8)) return;  // Ensure 8 bytes available
 *   int itemId = READ_LONG(pk);
 *   if (!PacketSanitizer::ValidateItemID(itemId)) return;     // Validate item exists
 * 
 * Created: January 30, 2026
 */

#ifndef PACKET_SANITIZER_H
#define PACKET_SANITIZER_H

#include <string>
#include <cstring>
#include <algorithm>
#include <cctype>

namespace PacketSanitizer {

// ============================================================================
// CONSTANTS - Define safe limits
// ============================================================================

// String limits
constexpr size_t MAX_USERNAME_LENGTH = 32;
constexpr size_t MAX_PASSWORD_LENGTH = 64;
constexpr size_t MAX_CHAT_LENGTH = 256;
constexpr size_t MAX_STALL_NAME_LENGTH = 64;
constexpr size_t MAX_GUILD_NAME_LENGTH = 32;
constexpr size_t MAX_GUILD_MOTTO_LENGTH = 128;
constexpr size_t MAX_CHARACTER_NAME_LENGTH = 32;

// Numeric limits  
constexpr int MAX_ITEM_ID = 100000;          // Adjust based on your ItemInfo.txt
constexpr int MAX_SKILL_ID = 10000;          // Adjust based on your SkillInfo.txt
constexpr int MAX_MAP_ID = 1000;             // Adjust based on your maps
constexpr int MAX_NPC_ID = 100000;           // Adjust based on your NpcInfo.txt
constexpr int MAX_MONSTER_ID = 100000;       // Adjust based on your MonsterInfo.txt

// Position limits - game uses coordinates in hundreds of thousands (e.g. 215187, 277103)
constexpr int MIN_COORDINATE = 0;
constexpr int MAX_COORDINATE = 100000000;    // Map coordinate range (100 million max)

// Inventory/Trade limits
constexpr int MAX_KITBAG_SLOTS = 128;         // defMAX_KBITEM_NUM_PER_TYPE from CompCommand.h
constexpr int MAX_TRADE_SLOTS = 18;          // ROLE_MAXNUM_TRADEDATA
constexpr int MAX_STACK_COUNT = 999;         // ROLE_MAXNUM_ITEMTRADE
constexpr long long MAX_GOLD = 100000000000LL; // Max gold (100 billion, matches Init_Attr.lua ATTR_GD)
constexpr int MAX_IMPS = 999999999;          // Max IMP currency (adjusted for high-value trades)

// Movement limits
constexpr int MAX_MOVE_DISTANCE_PER_TICK = 50;  // Anti-speed hack
constexpr int MAX_WAYPOINTS = 32;               // stNetMoveInfo buffer size

// ============================================================================
// STRING VALIDATION
// ============================================================================

/**
 * Check if string contains SQL injection characters
 * @param str String to check
 * @return true if string is safe, false if it contains dangerous characters
 */
inline bool IsSafeString(const char* str) {
    if (str == nullptr) return false;
    
    // Dangerous SQL characters
    const char* dangerous = "'\";\\\r\n\t--/**/";
    
    while (*str) {
        if (strchr(dangerous, *str) != nullptr) {
            return false;
        }
        str++;
    }
    return true;
}

/**
 * Check if string contains only alphanumeric characters (for usernames, etc.)
 */
inline bool IsAlphanumeric(const char* str) {
    if (str == nullptr || *str == '\0') return false;
    
    while (*str) {
        if (!std::isalnum(static_cast<unsigned char>(*str)) && *str != '_' && *str != '-') {
            return false;
        }
        str++;
    }
    return true;
}

/**
 * Validate string with length and content checks
 * @param str String to validate
 * @param maxLen Maximum allowed length
 * @param allowSpaces Whether to allow space characters
 * @param requireAlphanumeric Whether to require alphanumeric only
 * @return true if valid
 */
inline bool ValidateString(const char* str, size_t maxLen, bool allowSpaces = false, bool requireAlphanumeric = false) {
    if (str == nullptr) return false;
    
    size_t len = strlen(str);
    if (len == 0 || len > maxLen) return false;
    
    // Check for null bytes in the middle
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '\0') return false;  // Embedded null
    }
    
    // Check for dangerous characters
    if (!IsSafeString(str)) return false;
    
    // Check content if required
    if (requireAlphanumeric && !IsAlphanumeric(str)) return false;
    
    // Check for spaces
    if (!allowSpaces && strchr(str, ' ') != nullptr) return false;
    
    return true;
}

/**
 * Validate username format
 */
inline bool ValidateUsername(const char* username) {
    return ValidateString(username, MAX_USERNAME_LENGTH, false, true);
}

/**
 * Validate character name format
 */
inline bool ValidateCharacterName(const char* name) {
    return ValidateString(name, MAX_CHARACTER_NAME_LENGTH, false, false);
}

/**
 * Validate chat message (allows spaces, checks length)
 */
inline bool ValidateChatMessage(const char* message) {
    return ValidateString(message, MAX_CHAT_LENGTH, true, false);
}

/**
 * Validate guild name
 */
inline bool ValidateGuildName(const char* name) {
    return ValidateString(name, MAX_GUILD_NAME_LENGTH, false, false);
}

/**
 * Validate stall name
 */
inline bool ValidateStallName(const char* name) {
    return ValidateString(name, MAX_STALL_NAME_LENGTH, true, false);
}

// ============================================================================
// NUMERIC VALIDATION
// ============================================================================

/**
 * Validate a number is within range (inclusive)
 */
template<typename T>
inline bool ValidateRange(T value, T minVal, T maxVal) {
    return value >= minVal && value <= maxVal;
}

/**
 * Validate item ID exists in valid range
 */
inline bool ValidateItemID(int itemId) {
    return ValidateRange(itemId, 1, MAX_ITEM_ID);
}

/**
 * Validate skill ID exists in valid range
 */
inline bool ValidateSkillID(int skillId) {
    return ValidateRange(skillId, 1, MAX_SKILL_ID);
}

/**
 * Validate map coordinates
 */
inline bool ValidateCoordinate(int coord) {
    return ValidateRange(coord, MIN_COORDINATE, MAX_COORDINATE);
}

/**
 * Validate position (x, y)
 */
inline bool ValidatePosition(int x, int y) {
    return ValidateCoordinate(x) && ValidateCoordinate(y);
}

/**
 * Validate inventory slot index
 */
inline bool ValidateKitbagSlot(int slot) {
    return ValidateRange(slot, 0, MAX_KITBAG_SLOTS - 1);
}

/**
 * Validate trade slot index
 */
inline bool ValidateTradeSlot(int slot) {
    return ValidateRange(slot, 0, MAX_TRADE_SLOTS - 1);
}

/**
 * Validate item stack count
 */
inline bool ValidateStackCount(int count) {
    return ValidateRange(count, 1, MAX_STACK_COUNT);
}

/**
 * Validate gold amount
 */
inline bool ValidateGold(long long amount) {
    return ValidateRange(amount, 0LL, MAX_GOLD);
}

/**
 * Validate IMP amount
 */
inline bool ValidateIMPs(int amount) {
    return ValidateRange(amount, 0, MAX_IMPS);
}

/**
 * Validate NPC ID
 */
inline bool ValidateNPCID(int npcId) {
    return ValidateRange(npcId, 1, MAX_NPC_ID);
}

// ============================================================================
// MOVEMENT VALIDATION
// ============================================================================

/**
 * Calculate squared distance between two points (avoids sqrt for speed)
 */
inline int DistanceSquared(int x1, int y1, int x2, int y2) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    return dx * dx + dy * dy;
}

/**
 * Validate movement distance (anti-speed hack)
 */
inline bool ValidateMoveDistance(int fromX, int fromY, int toX, int toY, int maxDistance = MAX_MOVE_DISTANCE_PER_TICK) {
    int distSq = DistanceSquared(fromX, fromY, toX, toY);
    return distSq <= (maxDistance * maxDistance);
}

/**
 * Validate waypoint count
 */
inline bool ValidateWaypointCount(int count) {
    return ValidateRange(count, 1, MAX_WAYPOINTS);
}

// ============================================================================
// PACKET SIZE VALIDATION
// ============================================================================

// NOTE: Packet size validation removed to avoid RPacket namespace conflicts.
// Use try-catch around packet reads or check values after reading instead.
// The server's RPacket implementation varies by project (dbc::RPacket vs RPacket).

// ============================================================================
// POINTER VALIDATION
// ============================================================================

/**
 * Validate a pointer is not null
 */
template<typename T>
inline bool ValidatePointer(T* ptr) {
    return ptr != nullptr;
}

/**
 * Validate character pointer and check if alive
 */
template<typename TCharacter>
inline bool ValidateCharacter(TCharacter* cha) {
    if (cha == nullptr) return false;
    // Add additional checks like IsAlive() if available
    return true;
}

// ============================================================================
// LOGGING HELPERS
// ============================================================================

/**
 * Log a sanitization failure
 * Use this for security auditing and detecting exploit attempts
 */
#define SANITIZE_LOG(category, format, ...) \
    LG("Security", "[%s] " format "\n", category, ##__VA_ARGS__)

#define SANITIZE_FAIL(category, reason, clientIP) \
    LG("Security", "[%s] Sanitization failed: %s from IP: %s\n", category, reason, clientIP)

// ============================================================================
// CONVENIENCE MACROS
// ============================================================================

/**
 * Validate and log on failure, returning from function
 * Usage: VALIDATE_OR_RETURN(ValidateItemID(itemId), "Trade", "Invalid item ID");
 */
#define VALIDATE_OR_RETURN(condition, category, message) \
    do { \
        if (!(condition)) { \
            LG("Security", "[%s] Validation failed: %s\n", category, message); \
            return; \
        } \
    } while(0)

/**
 * Validate and log on failure, returning false
 */
#define VALIDATE_OR_RETURN_FALSE(condition, category, message) \
    do { \
        if (!(condition)) { \
            LG("Security", "[%s] Validation failed: %s\n", category, message); \
            return false; \
        } \
    } while(0)

/**
 * Validate and log on failure, returning specified value
 */
#define VALIDATE_OR_RETURN_VAL(condition, category, message, retval) \
    do { \
        if (!(condition)) { \
            LG("Security", "[%s] Validation failed: %s\n", category, message); \
            return retval; \
        } \
    } while(0)

/**
 * Validate and break from switch case
 */
#define VALIDATE_OR_BREAK(condition, category, message) \
    if (!(condition)) { \
        LG("Security", "[%s] Validation failed: %s\n", category, message); \
        break; \
    }

} // namespace PacketSanitizer

// Short aliases for convenience
namespace PS = PacketSanitizer;

#endif // PACKET_SANITIZER_H
