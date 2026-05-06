#pragma once
/**
 * IRateLimiter.h - Interface for login rate limiting
 * 
 * This interface abstracts the rate limiting functionality currently embedded
 * in AuthThread::QueryAccount(). The implementation will wrap existing code
 * in AccountServer2::attempt_info to maintain current behavior.
 */

#include <string>
#include <mutex>
#include <unordered_map>
#include "AuthTypes.h"

namespace auth {

/**
 * Interface for rate limiting login attempts
 * 
 * Current behavior (preserved):
 * - Tracks login attempts per IP address
 * - Blocks after 5 attempts within 2 minutes
 * - Block lasts for 2 minutes
 */
class IRateLimiter {
public:
    virtual ~IRateLimiter() = default;
    
    /**
     * Check if an IP address is allowed to make a login attempt
     * @param key Identifier for rate limiting (typically IP address)
     * @return true if allowed, false if rate limited
     */
    virtual bool IsAllowed(const std::string& key) = 0;
    
    /**
     * Record a login attempt from the given key
     * Call this AFTER checking IsAllowed() for accurate counting
     * @param key Identifier for rate limiting
     */
    virtual void RecordAttempt(const std::string& key) = 0;
    
    /**
     * Reset the attempt counter for a key (e.g., after successful login)
     * @param key Identifier to reset
     */
    virtual void Reset(const std::string& key) = 0;
    
    /**
     * Check if a key is currently blocked
     * @param key Identifier to check
     * @return true if currently blocked
     */
    virtual bool IsBlocked(const std::string& key) = 0;
    
    /**
     * Get the current configuration
     */
    virtual const RateLimitConfig& GetConfig() const = 0;
};

/**
 * Implementation that wraps the existing AccountServer2::attempt_info logic
 * This maintains exact current behavior while providing a clean interface
 */
class LegacyRateLimiter : public IRateLimiter {
public:
    explicit LegacyRateLimiter(const RateLimitConfig& config = RateLimitConfig());
    ~LegacyRateLimiter() override = default;
    
    bool IsAllowed(const std::string& key) override;
    void RecordAttempt(const std::string& key) override;
    void Reset(const std::string& key) override;
    bool IsBlocked(const std::string& key) override;
    const RateLimitConfig& GetConfig() const override { return m_config; }
    
    /**
     * Clean up expired entries periodically (call from maintenance thread)
     */
    void CleanupExpired();
    
private:
    struct AttemptInfo {
        size_t attemptCount = 0;
        uint64_t firstAttemptTick = 0;
        uint64_t blockStartTick = 0;
        bool isBlocked = false;
    };
    
    RateLimitConfig m_config;
    std::mutex m_mutex;
    std::unordered_map<std::string, AttemptInfo> m_attempts;
    
    uint64_t GetCurrentTick() const;
    bool IsBlockExpired(const AttemptInfo& info) const;
    bool IsIntervalExpired(const AttemptInfo& info) const;
};

} // namespace auth
