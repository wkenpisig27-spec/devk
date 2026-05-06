/**
 * AuthConfig.cpp - Configuration loading for auth system
 * 
 * Loads authentication configuration from INI files, maintaining
 * compatibility with existing AccountServer.cfg format.
 */

#include "stdafx.h"
#include "Auth.h"
#include "inifile.h"
#include "GlobalVariable.h"
#include <iostream>

namespace auth {

// Helper to safely read an optional INI key
static std::string SafeGetKey(IniSection& section, const char* key, const std::string& defaultVal = "") {
    try {
        return section[key];
    } catch (...) {
        return defaultVal;
    }
}

static int SafeGetInt(IniSection& section, const char* key, int defaultVal) {
    try {
        std::string val = section[key];
        if (val.empty()) return defaultVal;
        return std::stoi(val);
    } catch (...) {
        return defaultVal;
    }
}

AuthConfig LoadAuthConfig(const std::string& filename) {
    AuthConfig config;
    
    try {
        IniFile inf(filename.c_str());
        
        // Database settings (same as existing AuthThread::LoadConfig)
        IniSection& db = inf["db"];
        config.dbServer = db["dbserver"];
        config.dbName = db["db"];
        config.dbUserId = db["userid"];
        config.dbPassword = db["passwd"];
        
    } catch (std::exception& e) {
        std::cerr << "[AuthConfig] Error loading db config: " << e.what() << std::endl;
        // Continue with defaults for other settings
    }
    
    // Try to load optional rate limit settings
    try {
        IniFile inf(filename.c_str());
        IniSection& rl = inf["ratelimit"];
        
        int enabled = SafeGetInt(rl, "enabled", 1);
        config.rateLimit.enabled = (enabled != 0);
        config.rateLimit.maxAttemptsPerInterval = SafeGetInt(rl, "max_attempts", 5);
        config.rateLimit.intervalSeconds = SafeGetInt(rl, "interval_seconds", 120);
        config.rateLimit.blockDurationSeconds = SafeGetInt(rl, "block_seconds", 120);
    } catch (...) {
        // Section doesn't exist - use defaults (matching current magic numbers)
    }
    
    // Try to load optional auth timeout settings
    try {
        IniFile inf(filename.c_str());
        IniSection& auth = inf["auth"];
        
        config.timeouts.loginTimeoutMs = SafeGetInt(auth, "login_timeout_ms", 30000);
        config.timeouts.savingProtectionSeconds = SafeGetInt(auth, "saving_protection_seconds", 15);
        config.timeouts.reloginDelaySeconds = SafeGetInt(auth, "relogin_delay_seconds", 15);
    } catch (...) {
        // Section doesn't exist - use defaults
    }
    
    // Try to load optional database table settings
    try {
        IniFile inf(filename.c_str());
        IniSection& database = inf["database"];
        
        std::string tableName = SafeGetKey(database, "account_table", "account_login");
        if (!tableName.empty()) {
            config.accountTableName = tableName;
        }
    } catch (...) {
        // Section doesn't exist - use default
    }
    
    return config;
}

std::shared_ptr<IRateLimiter> CreateDefaultRateLimiter(const RateLimitConfig& config) {
    return std::make_shared<LegacyRateLimiter>(config);
}

std::shared_ptr<IAuthService> CreateLegacyAuthService(
    std::shared_ptr<IRateLimiter> rateLimiter,
    const AuthConfig& config) {
    return std::make_shared<AuthServiceLegacy>(rateLimiter, config);
}

} // namespace auth
