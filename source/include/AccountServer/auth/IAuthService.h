#pragma once
/**
 * IAuthService.h - Interface for the authentication service
 * 
 * This interface abstracts the login/logout logic currently in AuthThread.
 * The implementation wraps existing AuthThread::AccountLogin() and 
 * AuthThread::AccountLogout() to maintain exact current behavior.
 */

#include <string>
#include <optional>
#include "AuthTypes.h"

namespace auth {

/**
 * Interface for authentication operations
 * 
 * Current behavior (Option A - preserved exactly):
 * - If account OFFLINE: login succeeds
 * - If account ONLINE: kick old session, return error (client must retry)
 * - If account SAVING (within 15 sec): return error
 * - If account SAVING (after 15 sec): login succeeds
 */
class IAuthService {
public:
    virtual ~IAuthService() = default;
    
    /**
     * Authenticate a user login request
     * 
     * @param request Login credentials and metadata
     * @return LoginResult with success/error code and account info
     * 
     * Behavior matches existing AuthThread::AccountLogin():
     * - Validates credentials via bcrypt
     * - Handles account status (OFFLINE/ONLINE/SAVING)
     * - Updates database on successful login
     * - Returns kick command info if duplicate login detected
     */
    virtual LoginResult Authenticate(const LoginRequest& request) = 0;
    
    /**
     * Log out an account
     * 
     * @param accountId The account ID to log out
     * @param sessionId The session ID (for validation)
     * @return true if logout was successful
     */
    virtual bool Logout(int32_t accountId, int32_t sessionId) = 0;
    
    /**
     * Query account information without logging in
     * 
     * @param username Account name to query
     * @return AccountData if found, nullopt otherwise
     */
    virtual std::optional<AccountData> QueryAccount(const std::string& username) = 0;
    
    /**
     * Validate a username format
     * 
     * @param username Name to validate
     * @return true if valid format
     */
    virtual bool IsValidUsername(const std::string& username) = 0;
    
    /**
     * Get the current configuration
     */
    virtual const AuthConfig& GetConfig() const = 0;
};

/**
 * Map LoginResultCode to existing error codes for packet responses
 * This maintains wire protocol compatibility
 */
inline uint16_t ToLegacyErrorCode(LoginResultCode code) {
    // These values match existing ERR_AP_* and ERR_MC_* constants
    switch (code) {
        case LoginResultCode::Success:          return 0;  // ERR_SUCCESS
        case LoginResultCode::InvalidUser:      return 1;  // ERR_AP_INVALIDUSER
        case LoginResultCode::InvalidPassword:  return 2;  // ERR_AP_INVALIDPWD
        case LoginResultCode::AccountBanned:    return 3;  // ERR_AP_BANUSER
        case LoginResultCode::AccountPartialBan:return 4;  // ERR_AP_PBANUSER
        case LoginResultCode::AccountOnline:    return 5;  // ERR_AP_ONLINE
        case LoginResultCode::AccountSaving:    return 6;  // ERR_AP_SAVING
        case LoginResultCode::RateLimited:      return 7;  // ERR_AP_DISCONN (reused)
        case LoginResultCode::NetworkError:     return 100;// ERR_AP_NETEXCP
        case LoginResultCode::DatabaseError:    return 101;// ERR_AP_UNKNOWN
        case LoginResultCode::InvalidData:      return 102;// ERR_AP_DISCONN
        case LoginResultCode::ServerFull:       return 200;// ERR_MC_TOOMANYPLY
        case LoginResultCode::VersionMismatch:  return 201;// ERR_MC_VER_ERROR
        default:                                return 255;// ERR_AP_UNKNOWN
    }
}

} // namespace auth
