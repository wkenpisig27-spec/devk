# Network Security Configuration for PKO Server

Your GateServer anti-DDoS protection works at the **application layer**, but real DDoS attacks often overwhelm the **network/OS layers** before reaching your application code.

## Why Your Anti-DDoS Didn't Trigger

The anti-DDoS checks in `ToClient::OnConnect()` run AFTER:
1. ✅ TCP 3-way handshake completes
2. ✅ OS allocates socket resources
3. ✅ Connection enters application's accept queue

**SYN flood attacks** and **connection floods** exhaust resources at steps 1-2, never reaching step 3.

## Multi-Layer Defense Strategy

### Layer 1: Network/Firewall (CRITICAL)

**Use a firewall with SYN cookie protection:**

#### Windows Firewall with Advanced Security

```powershell
# Enable SYN attack protection
netsh int tcp set security profiles=enabled

# Limit half-open connections
netsh int tcp set global chimney=disabled
netsh int tcp set global autotuninglevel=normal
netsh int tcp set global congestionprovider=ctcp
netsh int tcp set global ecncapability=disabled
netsh int tcp set global timestamps=disabled

# Set TCP parameters
netsh int tcp set global maxsynretransmissions=2
```

#### Linux (if using Linux VPS)

```bash
# Enable SYN cookies (prevents SYN flood)
sysctl -w net.ipv4.tcp_syncookies=1

# Reduce SYN-RECV timeout
sysctl -w net.ipv4.tcp_syn_retries=3
sysctl -w net.ipv4.tcp_synack_retries=3

# Increase connection queue backlog
sysctl -w net.core.somaxconn=4096
sysctl -w net.ipv4.tcp_max_syn_backlog=8192

# Make permanent
echo "net.ipv4.tcp_syncookies=1" >> /etc/sysctl.conf
echo "net.ipv4.tcp_max_syn_backlog=8192" >> /etc/sysctl.conf
sysctl -p
```

### Layer 2: Rate Limiting with iptables/nftables (Linux)

```bash
# Limit new connections per IP to 10/minute
iptables -A INPUT -p tcp --dport 1973 -m state --state NEW -m recent --set
iptables -A INPUT -p tcp --dport 1973 -m state --state NEW -m recent --update --seconds 60 --hitcount 10 -j DROP

# Limit packets per second
iptables -A INPUT -p tcp --dport 1973 -m limit --limit 100/s --limit-burst 200 -j ACCEPT
iptables -A INPUT -p tcp --dport 1973 -j DROP
```

### Layer 3: Cloud DDoS Protection (BEST for VPS)

**Use a DDoS protection service:**

- **Cloudflare Spectrum** - Layer 4 DDoS protection for game servers
- **AWS Shield** - If using AWS
- **OVH Game DDoS Protection** - Built-in for OVH VPS
- **Google Cloud Armor** - If using GCP

### Layer 4: Application-Level (Your Current Anti-DDoS)

Your GateServer.cfg settings are good for **legitimate user abuse**, but won't stop real network attacks.

**Current settings (already configured):**
```ini
[AntiDDoS]
Enabled = 1
ConnectionMinInterval = 10
MaxConnectionsPerSecond = 100
MaxPacketsPerSecond = 500
MaxLoginAttemptsPerMinute = 200
BanDurationMinutes = 1
MaxWarnings = 10
```

## VPS-Specific Recommendations

### For Windows VPS

1. **Enable Windows Firewall with strict rules**
2. **Install fail2ban for Windows** (ban IPs after repeated failures)
3. **Use CloudFlare Spectrum** in front of your server
4. **Limit concurrent connections in GateServer.cfg**: `MaxConnection = 500`

### For Linux VPS (Recommended)

1. **Configure kernel TCP settings** (see above)
2. **Install and configure fail2ban**:
   ```bash
   apt-get install fail2ban
   # Configure to monitor GateServer logs
   ```
3. **Use iptables rate limiting** (see above)
4. **Deploy behind a reverse proxy** with DDoS protection

## Testing Your Protection

### Safe Penetration Testing

**DO NOT flood your own VPS** - you'll get your IP banned by the hosting provider!

Instead, test application-level limits:

```python
# Test login rate limiting (safe test)
import socket
import time

for i in range(300):
    try:
        sock = socket.socket()
        sock.connect(('your-server', 1973))
        time.sleep(0.1)  # Small delay between attempts
        sock.close()
    except:
        print(f"Connection {i} failed")
```

### What You Experienced

Your test likely did:
- ❌ Massive SYN flood → OS exhausted before app sees connections
- ❌ Connection flood → Network saturated
- ❌ No rate limiting at OS/network layer

Result: VPS slowed down and disconnected (OS-level resource exhaustion)

## Production Deployment Checklist

- [ ] Enable SYN cookie protection at OS level
- [ ] Configure firewall with connection rate limits
- [ ] Use cloud DDoS protection service (Cloudflare/AWS/OVH)
- [ ] Set reasonable MaxConnection limit in GateServer.cfg
- [ ] Monitor server resources (CPU, memory, network)
- [ ] Have IP blacklist/whitelist capability
- [ ] Log all DDoS events for analysis
- [ ] Set up alerts for abnormal traffic patterns

## Summary

Your GateServer anti-DDoS is **working correctly** for application-level abuse. The issue is that **real DDoS attacks** happen at lower network layers that require OS-level and network-level protection.

**For production:** Use a VPS provider with built-in DDoS protection or put CloudFlare Spectrum in front of your game server.
