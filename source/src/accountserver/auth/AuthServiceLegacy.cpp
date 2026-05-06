/**
 * AuthServiceLegacy.cpp - Legacy wrapper implementation
 * 
 * This implementation wraps the existing AuthThread to provide the new 
 * IAuthService interface while maintaining EXACT current behavior (Option A).
 * 
 * Current behavior preserved:
 * - If account OFFLINE: login succeeds
 * - If account ONLINE: kick old session, return error (ERR_AP_ONLINE)
 * - If account SAVING (within 15 sec): return error (ERR_AP_SAVING)
 * - If account SAVING (after 15 sec): login succeeds
 * 
 * The new login does NOT automatically succeed after kick - client must retry.
 */

#include "stdafx.h"
#include "auth/AuthServiceLegacy.h"
#include "AccountServer2.h"
#include "conformity.h"
#include <botan/bcrypt.h>

// External references to existing global objects
extern AccountServer2* g_As2;
extern LoginTmpList tmpLogin;

namespace auth {

AuthServiceLegacy::AuthServiceLegacy(
    std::shared_ptr<IRateLimiter> rateLimiter,
    const AuthConfig& config)
    : m_rateLimiter(std::move(rateLimiter))
    , m_config(config) {
}

AccountStatus AuthServiceLegacy::ConvertStatus(int oldStatus) {
    // Convert from AuthThread's enum to our new enum
    // Values are the same, but this provides type safety
    switch (oldStatus) {
        case 0: return AccountStatus::Offline;  // ACCOUNT_OFFLINE
        case 1: return AccountStatus::Online;   // ACCOUNT_ONLINE
        case 2: return AccountStatus::Saving;   // ACCOUNT_SAVING
        default: return AccountStatus::Offline;
    }
}

BanStatus AuthServiceLegacy::ConvertBanStatus(int banFlag) {
    switch (banFlag) {
        case 0: return BanStatus::None;
        case 1: return BanStatus::PartialBan;
        case 2: return BanStatus::FullBan;
        case 3: return BanStatus::PermanentBan;
        default: return BanStatus::None;
    }
}

bool AuthServiceLegacy::IsValidUsername(const std::string& username) {
    // Use existing validation from conformity
    if (username.empty() || username.length() > 32) {
        return false;
    }
    
    return common::conformity::login::name::is_valid(
        username.c_str(), 
        static_cast<unsigned short>(username.length())
    );
}

std::optional<AccountData> AuthServiceLegacy::QueryAccount(const std::string& username) {
    // Note: This is a simplified version. The full implementation would
    // need to access the database through the existing db_mutator.
    // For now, this returns nullopt as it's not used in the main login flow.
    // The actual query happens inside Authenticate() via the existing code path.
    return std::nullopt;
}

LoginResult AuthServiceLegacy::Authenticate(const LoginRequest& request) {
    LoginResult result;
    
    // Step 1: Validate username format
    if (!IsValidUsername(request.username)) {
        result.code = LoginResultCode::InvalidData;
        result.errorMessage = "Invalid username format";
        return result;
    }
    
    // Step 2: Rate limiting check
    if (m_rateLimiter && !m_rateLimiter->IsAllowed(request.clientIP)) {
        result.code = LoginResultCode::RateLimited;
        result.errorMessage = "Too many login attempts, please wait";
        return result;
    }
    
    // Step 3: Record the attempt
    if (m_rateLimiter) {
        m_rateLimiter->RecordAttempt(request.clientIP);
    }
    
    // Step 4: Check if account is already being processed
    // (Prevents race condition with multiple simultaneous logins)
    if (!tmpLogin.Insert(request.username)) {
        result.code = LoginResultCode::AccountOnline;
        result.errorMessage = "Account login already in progress";
        return result;
    }
    
    // Note: At this point, the actual login logic continues in the existing
    // AuthThread::QueryAccount() and AuthThread::AccountLogin() methods.
    // This wrapper is designed to be called BEFORE those methods and provides
    // the rate limiting and initial validation.
    
    // For full integration, the existing AuthThread code would need to be
    // refactored to use this service. For now, this service handles:
    // - Rate limiting (moved from QueryAccount)
    // - Input validation (moved from QueryAccount)
    // - Temporary login list management
    
    // The actual database query and password verification remains in AuthThread
    // to ensure no behavioral changes during initial refactoring.
    
    result.code = LoginResultCode::Success;
    return result;
}

bool AuthServiceLegacy::Logout(int32_t accountId, int32_t sessionId) {
    // The actual logout is handled by existing CMD_PA_USER_LOGOUT handler
    // in AuthThread::AccountLogout(). This method is provided for interface
    // completeness and future refactoring.
    
    // For now, we just return true as the actual work happens elsewhere.
    // In a future refactoring phase, the AccountLogout logic would be moved here.
    return true;
}

} // namespace auth
