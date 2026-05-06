#pragma once
/**
 * AuthTypes.h - Common types for the authentication system
 * 
 * This file defines enums and structs used across the authentication layer.
 * These types maintain compatibility with the existing system while providing
 * cleaner abstractions for the refactored code.
 */

#include <string>
#include <cstdint>

namespace auth {

/**
 * Account status in the database
 * Matches existing AuthThread enum values for backward compatibility
 */
enum class AccountStatus : int {
    Offline = 0,   // ACCOUNT_OFFLINE - Account is not logged in
    Online = 1,    // ACCOUNT_ONLINE - Account is currently playing
    Saving = 2     // ACCOUNT_SAVING - Account is in saving/transition state
};

/**
 * Ban status from database
 */
enum class BanStatus : int {
    None = 0,           // Not banned
    PartialBan = 1,     // Partial ban (ERR_AP_PBANUSER)
    FullBan = 2,        // Full ban (ERR_AP_BANUSER)  
    PermanentBan = 3    // Permanent ban
};

/**
 * Result of an authentication attempt
 */
enum class LoginResultCode : int {
    Success = 0,
    InvalidUser,            // Account doesn't exist
    InvalidPassword,        // Wrong password
    AccountBanned,          // Full ban
    AccountPartialBan,      // Partial ban
    AccountOnline,          // Already logged in (will trigger kick)
    AccountSaving,          // Account in saving state, try again later
    RateLimited,            // Too many login attempts
    NetworkError,           // Communication error
    DatabaseError,          // Database query/update failed
    InvalidData,            // Malformed packet data
    ServerFull,             // Too many players online
    VersionMismatch,        // Client version doesn't match
    UnknownError            // Catch-all for unexpected errors
};

/**
 * Request data for login attempt
 */
struct LoginRequest {
    std::string username;
    std::string password;           // Plaintext from decrypted packet
    std::string clientIP;
    std::string macAddress;
    std::string passport;           // Bill passport (usually "nobill")
    std::string groupServerName;    // Which GroupServer is requesting
    
    LoginRequest() = default;
    LoginRequest(const std::string& user, const std::string& pass, 
                 const std::string& ip, const std::string& mac)
        : username(user), password(pass), clientIP(ip), macAddress(mac) {}
};

/**
 * Result of a login attempt
 */
struct LoginResult {
    LoginResultCode code = LoginResultCode::UnknownError;
    int32_t accountId = 0;
    int32_t sessionId = 0;
    int32_t kickAccountId = 0;      // If code == AccountOnline, this is the account to kick
    std::string errorMessage;
    
    bool IsSuccess() const { return code == LoginResultCode::Success; }
    bool RequiresKick() const { return code == LoginResultCode::AccountOnline && kickAccountId > 0; }
};

/**
 * Account information retrieved from database
 */
struct AccountData {
    int32_t id = 0;
    std::string name;
    std::string passwordHash;       // bcrypt hash
    AccountStatus status = AccountStatus::Offline;
    BanStatus banStatus = BanStatus::None;
    int32_t protectTime = 0;        // Seconds until login allowed after saving
    std::string loginGroup;         // Which GroupServer account is on
    
    bool Exists() const { return id > 0; }
    bool IsBanned() const { return banStatus != BanStatus::None; }
    bool IsOnline() const { return status == AccountStatus::Online; }
    bool IsSaving() const { return status == AccountStatus::Saving; }
};

/**
 * Configuration for rate limiting
 */
struct RateLimitConfig {
    uint32_t maxAttemptsPerInterval = 5;        // Max login attempts before block
    uint32_t intervalSeconds = 120;              // Time window for counting attempts (2 min)
    uint32_t blockDurationSeconds = 120;         // How int to block after exceeding limit
    bool enabled = true;
    
    // Default constructor uses same values as current magic numbers
    RateLimitConfig() = default;
};

/**
 * Configuration for authentication timeouts
 */
struct AuthTimeoutConfig {
    uint32_t loginTimeoutMs = 30000;            // Timeout for login SyncCall (30 sec)
    uint32_t savingProtectionSeconds = 15;      // SAVING_TIME - wait before allowing relogin
    uint32_t reloginDelaySeconds = 15;          // RELOGIN_TIME
    
    AuthTimeoutConfig() = default;
};

/**
 * Complete authentication configuration
 */
struct AuthConfig {
    RateLimitConfig rateLimit;
    AuthTimeoutConfig timeouts;
    std::string accountTableName = "account_login";
    
    // Database connection info (loaded from config file)
    std::string dbServer;
    std::string dbName;
    std::string dbUserId;
    std::string dbPassword;
    
    AuthConfig() = default;
};

} // namespace auth
