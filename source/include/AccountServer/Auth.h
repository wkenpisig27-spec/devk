#pragma once
/**
 * Auth.h - Unified header for the authentication system
 * 
 * Include this single header to get all auth system types and interfaces.
 */

// Core types
#include "auth/AuthTypes.h"

// Interfaces  
#include "auth/IAuthService.h"
#include "auth/IRateLimiter.h"

// Implementations
#include "auth/AuthServiceLegacy.h"

namespace auth {

/**
 * Load AuthConfig from an INI file
 * 
 * @param filename Path to the config file (e.g., "AccountServer.cfg")
 * @return Populated AuthConfig struct
 * 
 * Expected INI format:
 * [db]
 * dbserver=127.0.0.1
 * db=GameDB
 * userid=sa
 * passwd=password
 * 
 * [ratelimit]
 * enabled=1
 * max_attempts=5
 * interval_seconds=120
 * block_seconds=120
 * 
 * [auth]
 * login_timeout_ms=30000
 * saving_protection_seconds=15
 */
AuthConfig LoadAuthConfig(const std::string& filename);

/**
 * Create the default rate limiter instance
 */
std::shared_ptr<IRateLimiter> CreateDefaultRateLimiter(const RateLimitConfig& config);

/**
 * Create the legacy auth service (wraps existing AuthThread)
 */
std::shared_ptr<IAuthService> CreateLegacyAuthService(
    std::shared_ptr<IRateLimiter> rateLimiter,
    const AuthConfig& config);

} // namespace auth
