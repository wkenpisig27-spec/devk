/**
 * RateLimiter.cpp - Implementation of rate limiting for login attempts
 * 
 * This implementation maintains exact behavior of the existing code in
 * AuthThread::QueryAccount() and AccountServer2::attempt_info.
 * 
 * Current behavior preserved:
 * - MAGIC_NUMBER_BLOCKINTERVAL = 120 * 1000 (2 minutes)
 * - MAGIC_NUMBER_BLOCKNUM = 5 (max attempts)
 * - Block resets after interval expires
 */

#include "stdafx.h"
#include <mutex>
#include <unordered_map>
#include "auth/IRateLimiter.h"
#ifdef PKO_PLATFORM_WINDOWS
#include <Windows.h>  // For GetTickCount64
#else
#include "serversdk/platform_compat.h"
#endif

namespace auth {

LegacyRateLimiter::LegacyRateLimiter(const RateLimitConfig& config)
    : m_config(config) {
}

uint64_t LegacyRateLimiter::GetCurrentTick() const {
    return GetTickCount64();
}

bool LegacyRateLimiter::IsBlockExpired(const AttemptInfo& info) const {
    if (!info.isBlocked) return true;
    
    uint64_t blockDurationMs = static_cast<uint64_t>(m_config.blockDurationSeconds) * 1000;
    return (GetCurrentTick() - info.blockStartTick) > blockDurationMs;
}

bool LegacyRateLimiter::IsIntervalExpired(const AttemptInfo& info) const {
    uint64_t intervalMs = static_cast<uint64_t>(m_config.intervalSeconds) * 1000;
    return (GetCurrentTick() - info.firstAttemptTick) > intervalMs;
}

bool LegacyRateLimiter::IsAllowed(const std::string& key) {
    if (!m_config.enabled) {
        return true;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_attempts.find(key);
    if (it == m_attempts.end()) {
        // First attempt from this key - always allowed
        return true;
    }
    
    AttemptInfo& info = it->second;
    
    // Check if block has expired
    if (info.isBlocked) {
        if (IsBlockExpired(info)) {
            // Block expired - reset and allow
            info.isBlocked = false;
            info.attemptCount = 0;
            info.firstAttemptTick = GetCurrentTick();
            return true;
        }
        // Still blocked
        return false;
    }
    
    // Check if interval has expired (reset counter)
    if (IsIntervalExpired(info)) {
        info.attemptCount = 0;
        info.firstAttemptTick = GetCurrentTick();
        return true;
    }
    
    // Within interval - check count
    return info.attemptCount < m_config.maxAttemptsPerInterval;
}

void LegacyRateLimiter::RecordAttempt(const std::string& key) {
    if (!m_config.enabled) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto& info = m_attempts[key];
    
    // Initialize on first attempt
    if (info.firstAttemptTick == 0) {
        info.firstAttemptTick = GetCurrentTick();
    }
    
    // Reset if interval expired
    if (IsIntervalExpired(info)) {
        info.attemptCount = 0;
        info.firstAttemptTick = GetCurrentTick();
    }
    
    // Increment counter
    info.attemptCount++;
    
    // Check if should block
    if (info.attemptCount > m_config.maxAttemptsPerInterval) {
        info.isBlocked = true;
        info.blockStartTick = GetCurrentTick();
    }
}

void LegacyRateLimiter::Reset(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_attempts.erase(key);
}

bool LegacyRateLimiter::IsBlocked(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_attempts.find(key);
    if (it == m_attempts.end()) {
        return false;
    }
    
    const AttemptInfo& info = it->second;
    return info.isBlocked && !IsBlockExpired(info);
}

void LegacyRateLimiter::CleanupExpired() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_attempts.begin();
    while (it != m_attempts.end()) {
        const AttemptInfo& info = it->second;
        
        // Remove entries that are not blocked and interval has expired
        if (!info.isBlocked && IsIntervalExpired(info)) {
            it = m_attempts.erase(it);
        } 
        // Remove blocked entries where block has expired
        else if (info.isBlocked && IsBlockExpired(info)) {
            it = m_attempts.erase(it);
        }
        else {
            ++it;
        }
    }
}

} // namespace auth
