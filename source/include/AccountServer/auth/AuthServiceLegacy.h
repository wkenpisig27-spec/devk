#pragma once
/**
 * AuthServiceLegacy.h - Legacy wrapper implementation of IAuthService
 * 
 * This class wraps the existing AuthThread functionality to provide
 * the IAuthService interface while maintaining EXACT current behavior.
 * 
 * The implementation delegates to AuthThread methods where possible,
 * ensuring no behavioral changes.
 */

#include "auth/IAuthService.h"
#include "auth/IRateLimiter.h"
#include "AccountServer2.h"
#include <memory>

namespace auth {

/**
 * Legacy implementation that wraps existing AuthThread
 * 
 * This is a thin wrapper that:
 * 1. Uses existing AuthThread for actual authentication
 * 2. Uses LegacyRateLimiter for rate limiting
 * 3. Translates between old and new types
 * 
 * Behavior is identical to current system (Option A):
 * - Duplicate login kicks old session, new login must retry
 */
class AuthServiceLegacy : public IAuthService {
public:
    /**
     * Create with external rate limiter (for shared use)
     */
    explicit AuthServiceLegacy(
        std::shared_ptr<IRateLimiter> rateLimiter,
        const AuthConfig& config = AuthConfig());
    
    ~AuthServiceLegacy() override = default;
    
    // IAuthService implementation
    LoginResult Authenticate(const LoginRequest& request) override;
    bool Logout(int32_t accountId, int32_t sessionId) override;
    std::optional<AccountData> QueryAccount(const std::string& username) override;
    bool IsValidUsername(const std::string& username) override;
    const AuthConfig& GetConfig() const override { return m_config; }
    
private:
    std::shared_ptr<IRateLimiter> m_rateLimiter;
    AuthConfig m_config;
    
    // Helper to convert old status enum to new
    static AccountStatus ConvertStatus(int oldStatus);
    static BanStatus ConvertBanStatus(int banFlag);
};

} // namespace auth
