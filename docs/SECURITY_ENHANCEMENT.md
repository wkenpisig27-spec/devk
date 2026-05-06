# PKO Security Enhancement Architecture

## Overview

This document describes a comprehensive security enhancement across all three layers:
- **Proxy Server** (PKO Smart Proxy v3)
- **GateServer** (Connection handler)
- **Client** (Game.exe)

## Quick Reference

| Component | File | Key Enhancement |
|-----------|------|-----------------|
| Proxy | `helper/smartproxy/proxy_v3.js` | Cryptographic token, fingerprinting |
| GateServer | `include/common/ProxyToken.h` | Token verification |
| Client | `ClientSecurityEnhancements.md` | HMAC signing, nonce verification |

---

## Layer 1: PKO Smart Proxy v3

### New Features

1. **Proxy Token System**
   - After RSA validation, proxy generates cryptographic token
   - Token format: `timestamp:clientIP:nonce:hmac`
   - HMAC uses shared secret between proxy and GateServer
   - Token valid for 30 seconds (configurable)

2. **Connection Fingerprinting**
   - Tracks response time patterns per IP
   - Detects bot-like behavior (too fast, no valid responses)
   - Builds reputation score over time

3. **Adaptive Rate Limiting**
   - Detects attack mode (>500 conn/sec)
   - Halves rate limits during attacks
   - Trusted IPs (positive fingerprint) get relaxed limits

4. **Enhanced Logging**
   - Token generation tracking
   - Fingerprint-based blocks
   - Response time analysis

### Configuration

```javascript
const CONFIG = {
    // Shared secret - MUST match GateServer
    proxySecret: process.env.PROXY_SECRET || 'YOUR_SECRET_HERE',
    tokenValidityMs: 30000,
    
    // Attack detection
    attackThreshold: 500,         // conn/sec to trigger
    attackModeMultiplier: 0.5,    // halve limits
    
    // Fingerprinting
    fingerprintEnabled: true,
    minResponseTimeMs: 50,        // faster = suspicious
};
```

### Deployment

```bash
# On proxy server (57.129.122.4)
cd /opt/pko-proxy
export PROXY_SECRET="YourSharedSecret2026!"
node proxy_v3.js
```

---

## Layer 2: GateServer Integration

### ProxyToken.h

Add to `source/include/common/ProxyToken.h`:
- HMAC-SHA256 verification using Windows BCrypt
- Token parsing and validation
- Clock skew tolerance (5 seconds)

### Configuration

Add to `GateServer.cfg`:

```ini
[AntiDDoS]
ProxyProtocol = 1
ProxyTokenEnabled = 1
ProxyTokenSecret = YourSharedSecret2026!
TrustedProxyIPs = 57.129.122.4
```

### Integration Points

1. **ParseProxyProtocol()** - Extract token from header
2. **OnConnect()** - Verify token before accepting
3. **VerifyDDoS()** - Use token verification status for trust

### Code Changes Required

In `ToClient.cpp`, after parsing PROXY protocol header:

```cpp
#include "common/ProxyToken.h"

// In ParseProxyProtocol(), after extracting srcIP:
if (m_proxyTokenEnabled) {
    std::string token = ProxyToken::ExtractTokenFromHeader(buffer);
    ProxyToken::VerifyResult result = ProxyToken::Verify(token, srcIP);
    
    if (!result.valid) {
        LG("GateServer", "[ProxyToken] Invalid: %s from %s\n", 
            result.reason.c_str(), srcIP);
        return false;
    }
}
```

---

## Layer 3: Client Enhancements

### Immediate (No client update required)

The proxy token system works transparently - clients don't need modification
because the proxy handles validation and token generation.

### Future Enhancements (Requires client update)

1. **Handshake Nonce**
   - Server includes 16-byte nonce in RSA handshake
   - Client must echo nonce in response
   - Prevents replay attacks

2. **Packet Signing (HMAC)**
   - Critical packets signed with session key
   - Server rejects unsigned/invalid signatures
   - Prevents packet manipulation

3. **Client Integrity**
   - Hash critical DLLs
   - Report hash during handshake
   - Flag suspicious clients

---

## Security Flow Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                         SECURITY FLOW                                │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  Client                  Proxy (v3)               GateServer         │
│    │                        │                         │              │
│    │─── TCP Connect ────────│                         │              │
│    │                        │                         │              │
│    │◄── RSA Challenge ──────│ (cached from GateServer)│              │
│    │                        │                         │              │
│    │    [Wait for RSA response, track response time]  │              │
│    │                        │                         │              │
│    │─── RSA Response ───────│                         │              │
│    │                        │                         │              │
│    │         │              │                         │              │
│    │         │  ┌───────────┴───────────┐             │              │
│    │         │  │ Validate:             │             │              │
│    │         │  │ - Packet format       │             │              │
│    │         │  │ - Response time       │             │              │
│    │         │  │ - Fingerprint score   │             │              │
│    │         │  │                       │             │              │
│    │         │  │ If valid:             │             │              │
│    │         │  │ - Generate token      │             │              │
│    │         │  │ - Update fingerprint  │             │              │
│    │         │  └───────────┬───────────┘             │              │
│    │         │              │                         │              │
│    │         │              │─── PROXY PROTOCOL ──────│              │
│    │         │              │    + RSA Response       │              │
│    │         │              │                         │              │
│    │         │              │              ┌──────────┴──────────┐   │
│    │         │              │              │ Verify:             │   │
│    │         │              │              │ - Token HMAC        │   │
│    │         │              │              │ - Token timestamp   │   │
│    │         │              │              │ - IP match          │   │
│    │         │              │              │ - Behavioral check  │   │
│    │         │              │              └──────────┬──────────┘   │
│    │         │              │                         │              │
│    │◄────────┼──────────────┼──── Login Proceed ──────│              │
│    │         │              │                         │              │
│    │═══════ ENCRYPTED SESSION ESTABLISHED ═══════════│              │
│    │                        │                         │              │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Attack Mitigation Matrix

| Attack Type | Layer | Mitigation |
|-------------|-------|------------|
| Connection Flood | Proxy | Rate limiting + fingerprinting |
| RSA Replay | Proxy + GateServer | Nonce + token timestamp |
| Direct GateServer | GateServer | Token verification required |
| Packet Injection | GateServer | HMAC signing (future) |
| Botnet DDoS | Proxy | Adaptive limits + behavioral analysis |
| Credential Stuffing | GateServer | Login rate limits + account lockout |
| Session Hijacking | Client | Session token binding |

---

## Deployment Checklist

### Proxy Server
- [ ] Upload `proxy_v3.js` to `/opt/pko-proxy/`
- [ ] Set `PROXY_SECRET` environment variable
- [ ] Stop old proxy, start v3
- [ ] Verify token generation in logs

### GateServer
- [ ] Add `ProxyToken.h` to include/common/
- [ ] Update `GateServer.cfg` with token settings
- [ ] Modify `ToClient.cpp` to verify tokens
- [ ] Rebuild GateServer
- [ ] Test with proxy token validation

### Testing
- [ ] Connect through proxy → should get token → should work
- [ ] Connect directly to GateServer → should fail (no token)
- [ ] Multiple rapid connections → should trigger fingerprint tracking
- [ ] Attack simulation → should activate adaptive mode

---

## Monitoring Commands

### Proxy Server
```bash
# Watch authenticated clients
journalctl -u pko-proxy -f | grep "Authenticated"

# Check fingerprint blocking
journalctl -u pko-proxy -f | grep "fingerprint"

# Token generation rate
journalctl -u pko-proxy -f | grep "tokensGenerated"
```

### GateServer
```powershell
# Watch token verification
Select-String -Path "server\LOG\*.log" -Pattern "ProxyToken" | Select-Object -Last 50

# Check behavioral tracking
Select-String -Path "server\LOG\*.log" -Pattern "behavioral|bot-like" | Select-Object -Last 50
```

---

## Shared Secret Management

**CRITICAL**: The proxy secret must match between proxy and GateServer.

### Generation
```bash
# Generate a secure random secret
openssl rand -base64 32
# Example: Xk7Gm2pN9qR4sT6vW8yZ1aB3cD5eF7hJ
```

### Distribution
1. Set in proxy: `export PROXY_SECRET="YourSecret"`
2. Set in GateServer.cfg: `ProxyTokenSecret = YourSecret`
3. **Never commit secrets to git**
4. Use environment variables or separate config files

### Rotation
1. Generate new secret
2. Update proxy first (it will accept both during transition)
3. Update GateServer
4. Remove old secret from proxy

---

## Future Roadmap

### Phase 1 (Current) ✓
- Proxy token system
- Connection fingerprinting
- Adaptive rate limiting

### Phase 2 (Next)
- Client nonce verification
- HMAC packet signing for critical packets
- Enhanced session management

### Phase 3 (Future)
- Machine learning bot detection
- Geographic anomaly detection
- Client integrity verification
- Hardware fingerprinting
