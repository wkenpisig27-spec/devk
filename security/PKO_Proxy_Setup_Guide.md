# PKO Smart Proxy v3 — Complete Setup Guide

**Purpose:** Comprehensive guide to deploy and manage the PKO Smart Proxy on a Linux VPS. Written to be followed step-by-step by an AI agent or human operator.

**Last Updated:** February 13, 2026  
**Proxy Version:** PKO Smart Proxy v3  
**Source File:** `helper/smartproxy/proxy_v3.js` (in the pkodev repository)  
**Config File:** `helper/smartproxy/proxy_config.json` (in the pkodev repository)

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [System Requirements](#2-system-requirements)
3. [Production Server Inventory](#3-production-server-inventory)
4. [Initial VPS Setup](#4-initial-vps-setup)
5. [Node.js Installation](#5-nodejs-installation)
6. [Proxy Application Setup](#6-proxy-application-setup)
7. [Configuration Reference](#7-configuration-reference)
8. [Systemd Service Configuration](#8-systemd-service-configuration)
9. [Firewall Rules (nftables)](#9-firewall-rules-nftables)
10. [Kernel / Sysctl Tuning](#10-kernel--sysctl-tuning)
11. [Log Rotation](#11-log-rotation)
12. [Fail2Ban (SSH Protection)](#12-fail2ban-ssh-protection)
13. [GateServer Integration](#13-gateserver-integration)
14. [OVH Edge Network Firewall](#14-ovh-edge-network-firewall)
15. [Game Client Configuration](#15-game-client-configuration)
16. [Admin Endpoints](#16-admin-endpoints)
17. [Management Commands](#17-management-commands)
18. [Multi-Server Operations](#18-multi-server-operations)
19. [Deploying Updates](#19-deploying-updates)
20. [Troubleshooting](#20-troubleshooting)
21. [How the Proxy Works (Technical Deep-Dive)](#21-how-the-proxy-works-technical-deep-dive)
22. [Quick Setup Checklist](#22-quick-setup-checklist)

---

## 1. Architecture Overview

```
Players (Game Client)
        │
        ▼ TCP :1973
┌─────────────────────────────────────────────────────┐
│  PKO Smart Proxy v3 (Node.js on Linux VPS)          │
│                                                     │
│  Layer 1: IP Blocklist + Temp Bans                  │ ← Instant reject
│  Layer 2: Connection Rate Limiting (adaptive)       │ ← Blocks floods
│  Layer 3: RSA Challenge-Response                    │ ← Blocks bots/scanners
│  Layer 4: Connection Fingerprinting                 │ ← Detects bot patterns
│  Layer 5: Post-Auth Packet Rate Limiting (300/s)    │ ← Blocks packet floods
│  Layer 6: Login Rate Limiting (20/min/IP)           │ ← Blocks login spam
│  Layer 7: Pending Login Queue (100 global max)      │ ← Protects GateServer
│  Layer 8: Attack Mode (auto-detect at 500 conn/s)  │ ← Halves all limits
│                                                     │
│  Authenticated traffic:                             │
│  Forward with PROXY Protocol v1 header              │
│  (preserves real client IP for GateServer)          │
└─────────────────────┬───────────────────────────────┘
                      │
                      ▼ TCP :1973 (with PROXY protocol v1 header)
┌─────────────────────────────────────────────────────┐
│  GateServer (Windows, on separate machine)          │
│  Parses PROXY protocol header → sees real client IP │
│  Routes to GameServer / AccountServer               │
└─────────────────────────────────────────────────────┘
```

### What Gets Blocked vs Forwarded

| Traffic Type | Action | Layer |
|-------------|--------|-------|
| Blocklisted IP | Instant drop, no response | Blocklist |
| Temp-banned IP | Instant drop, no response | Temp ban |
| Connection flood (>30/10s per IP) | Dropped | Rate limit |
| Too many concurrent connections (>20 per IP) | Dropped | Rate limit |
| Random TCP scan / port probe | Dropped — no valid RSA response | RSA challenge |
| Bot with scripted RSA response (<50ms) | Dropped — fingerprint flagged | Fingerprinting |
| Legitimate player | Forwarded to GateServer | Post-auth |
| Player spamming login button (>20/min) | Temp banned for 30s | Login rate |
| Packet flood after auth (>300 pkt/s) | Temp banned | Packet rate |
| DDoS >500 conn/s across all IPs | Attack mode — all limits halved | Adaptive |
| Volumetric DDoS (5+ Gbps) | **NOT blocked** — need upstream protection | N/A |

### Limitations

The proxy protects against application-layer attacks. It **cannot** stop:
- Volumetric bandwidth floods (need OVH Game DDoS protection / Cloudflare Spectrum / TCPShield)
- Attacks from thousands of unique IPs that each pass RSA legitimately
- Single-core Node.js event loop saturation under extreme load (~50k+ conn/s)

For volumetric protection, deploy the proxy on an OVH VPS (free hardware DDoS protection included).

---

## 2. System Requirements

| Spec | Minimum | Recommended |
|------|---------|-------------|
| **OS** | Ubuntu 22.04 LTS | Ubuntu 24.04 LTS |
| **CPU** | 1 vCPU | 2 vCPUs |
| **RAM** | 512 MB | 1 GB |
| **Network** | Low latency to GateServer | Same region as GateServer |
| **Node.js** | 18.x LTS | 20.x LTS |
| **Cost** | ~$4-6/month (DigitalOcean, Vultr, Hetzner) | |

---

## 3. Production Server Inventory

All proxy servers run identical code and configuration, pointing to the same GateServer.

### Server List

| Region | Proxy IP | Provider | User | SSH Key | DNS |
|--------|----------|----------|------|---------|-----|
| **Singapore (SG)** | `146.190.94.84` | DigitalOcean SGP1 | `root` | `id_pko_proxy_sg` | — |
| **Canada (CA)** | `158.69.211.60` | OVH Canada | `ubuntu` | `id_pko_proxy_ca` | — |
| **Europe (EU)** | `57.129.122.4` | OVH Europe | `ubuntu` | `id_pko_proxy_eu` | — |

### Common Settings (All Servers)

| Setting | Value |
|---------|-------|
| OS | Ubuntu 24.04 LTS |
| Node.js | v20.20.0 |
| Spec | 1 vCPU, 1 GB RAM |
| GateServer | `145.239.149.93:1973` |
| Proxy Code | `proxy_v3.js` (SHA256: `c5b6c6a2...`) |
| Log File | `/var/log/pko-proxy.log` |
| Service | `smartproxy.service` |

### Quick SSH Commands

```powershell
# Singapore
ssh -i "$env:USERPROFILE\.ssh\id_pko_proxy_sg" root@146.190.94.84

# Canada
ssh -i "$env:USERPROFILE\.ssh\id_pko_proxy_ca" ubuntu@158.69.211.60

# Europe
ssh -i "$env:USERPROFILE\.ssh\id_pko_proxy_eu" ubuntu@57.129.122.4
```

### GateServer Details

| Setting | Value |
|---------|-------|
| IP | `145.239.149.93` |
| Port | 1973 |
| Provider | OVH |
| Edge Firewall | Enabled (only proxy IPs allowed on port 1973) |
| SQL Access | `148.113.196.82` on port 1433 |

---

## 4. Initial VPS Setup

### 4.1 Update System

```bash
apt update && apt upgrade -y
```

### 4.2 Setup SSH Key Authentication

On your **local Windows machine** (PowerShell):

```powershell
# Generate a dedicated SSH key per region (replace <region> with sg, ca, or eu)
ssh-keygen -t ed25519 -f "$env:USERPROFILE\.ssh\id_pko_proxy_<region>" -C "pko-proxy-<region>"

# Copy public key to the server (replace USER and VPS_IP)
type "$env:USERPROFILE\.ssh\id_pko_proxy_<region>.pub" | ssh USER@VPS_IP "mkdir -p ~/.ssh && cat >> ~/.ssh/authorized_keys"
```

**SSH Key Naming Convention:**

| Region | Key File | User |
|--------|----------|------|
| Singapore | `id_pko_proxy_sg` | `root` |
| Canada | `id_pko_proxy_ca` | `ubuntu` |
| Europe | `id_pko_proxy_eu` | `ubuntu` |

### 4.3 Disable Password Authentication

On the **VPS**:

```bash
sed -i 's/#PasswordAuthentication yes/PasswordAuthentication no/' /etc/ssh/sshd_config
sed -i 's/PasswordAuthentication yes/PasswordAuthentication no/' /etc/ssh/sshd_config
systemctl restart sshd
```

### 4.4 Verify SSH Key Access

From **Windows**:

```powershell
# Should connect without password prompt (example for SG)
ssh -i "$env:USERPROFILE\.ssh\id_pko_proxy_sg" root@146.190.94.84 "echo 'SSH key auth working'"
```

**IMPORTANT:** All subsequent SSH commands in this guide use region-specific keys. The full SSH command pattern is:
```powershell
ssh -i "$env:USERPROFILE\.ssh\id_pko_proxy_<region>" USER@VPS_IP
```

---

## 5. Node.js Installation

```bash
curl -fsSL https://deb.nodesource.com/setup_20.x | bash -
apt install -y nodejs

# Verify
node --version    # Should output v20.x.x
npm --version     # Should output 10.x.x
```

---

## 6. Proxy Application Setup

### 6.1 Create Directory

```bash
mkdir -p /opt/pko-proxy
```

### 6.2 Upload Proxy Script

From **Windows** (PowerShell):

```powershell
# Replace <region>, USER, and VPS_IP for your target server
scp -i "$env:USERPROFILE\.ssh\id_pko_proxy_<region>" "helper\smartproxy\proxy_v3.js" USER@VPS_IP:/opt/pko-proxy/proxy_v3.js
```

> **Source file location:** `helper/smartproxy/proxy_v3.js` in the pkodev repository.  
> **Rule:** NEVER edit proxy code directly on the server. Always edit locally in the repository, then upload.

### 6.3 Create Configuration File

```bash
cat > /opt/pko-proxy/config.json << 'EOF'
{
  "listenPort": 1973,
  "healthPort": 8080,
  "gateServerHost": "145.239.149.93",
  "gateServerPort": 1973,
  "maxConnectionsPerIP": 20,
  "connectionRateLimit": 30,
  "connectionRateWindow": 10000,
  "tempBanDuration": 30000,
  "packetRateLimit": 300,
  "loginRateLimit": 20,
  "maxPendingLoginsPerIP": 8,
  "maxPendingLoginsGlobal": 100,
  "pendingLoginTimeout": 15000
}
EOF
```

**Note:** The `gateServerHost` is set to the current production GateServer. Change this only if your GateServer IP changes.

### 6.4 Create Blocklist File

```bash
cat > /opt/pko-proxy/blocklist.txt << 'EOF'
# PKO Smart Proxy — IP Blocklist
# One IP per line. Lines starting with # are comments.
# Auto-reloaded every 60 seconds — no restart needed.
EOF
```

### 6.5 Verify Files

```bash
ls -la /opt/pko-proxy/
# Expected: proxy_v3.js  config.json  blocklist.txt
```

---

## 7. Configuration Reference

### Full Configuration Options

The proxy merges `config.json` values on top of built-in defaults. You only need to specify values you want to override.

| Key | Default | Recommended | Description |
|-----|---------|-------------|-------------|
| **Server** | | | |
| `listenPort` | `1973` | `1973` | Port players connect to |
| `listenHost` | `0.0.0.0` | `0.0.0.0` | Listen address (all interfaces) |
| `regionName` | `default` | `default` | Region label (shown in logs/health) |
| `regionId` | `default` | `default` | Region identifier |
| **Backend** | | | |
| `gateServerHost` | — | **YOUR IP** | GateServer IP address (**MUST SET**) |
| `gateServerPort` | `1973` | `1973` | GateServer port |
| `gateServerConnectTimeout` | `15000` | `15000` | TCP connect timeout to GateServer (ms) |
| **Health Check** | | | |
| `healthCheckPort` | `8405` | `8080` | HTTP health endpoint port (localhost only) |
| `healthCheckEnabled` | `true` | `true` | Enable health HTTP server |
| **Anti-DDoS** | | | |
| `authTimeout` | `8000` | `8000` | Time for client to answer RSA challenge (ms) |
| `maxConnectionsPerIP` | `15` | `20` | Max simultaneous connections per IP |
| `connectionRateLimit` | `20` | `30` | Max new connections per IP per window |
| `connectionRateWindow` | `10000` | `10000` | Rate limit window (ms) |
| `tempBanDuration` | `300000` | `30000` | Temp ban duration (ms). 30000 = 30s |
| **Adaptive Mode** | | | |
| `attackThreshold` | `500` | `500` | Connections/sec to trigger attack mode |
| `attackModeMultiplier` | `0.5` | `0.5` | Multiply all limits by this during attacks |
| `normalModeDelay` | `60000` | `60000` | Wait time before leaving attack mode (ms) |
| **Fingerprinting** | | | |
| `fingerprintEnabled` | `true` | `true` | Enable bot fingerprinting |
| `minResponseTimeMs` | `50` | `50` | Responses faster than this are suspicious |
| `maxResponseTimeMs` | `15000` | `15000` | Response time upper bound |
| **Post-Auth Protection** | | | |
| `packetRateLimit` | `300` | `300` | Max packets/sec per IP after authentication |
| `packetRateWindow` | `1000` | `1000` | Packet rate measurement window (ms) |
| `loginRateLimit` | `15` | `20` | Max login attempts per IP per window |
| `loginRateWindow` | `60000` | `60000` | Login rate window (ms) |
| `maxPendingLoginsPerIP` | `2` | `8` | Max concurrent pending logins per IP |
| `maxPendingLoginsGlobal` | `50` | `100` | Max concurrent pending logins globally |
| `pendingLoginTimeout` | `30000` | `15000` | Auto-release pending login slot (ms) |
| **RSA Caching** | | | |
| `rsaRefreshInterval` | `60000` | `60000` | Normal RSA refresh interval (ms) |
| `rsaFetchCooldown` | `5000` | `5000` | Min time between RSA fetch attempts (ms) |
| **PKO Protocol** | | | |
| `cmdLoginId` | `431` | `431` | CMD_CM_LOGIN command ID (client→server) |
| `cmdLoginResponseId` | `931` | `931` | CMD_MC_LOGIN command ID (server→client) |
| **Logging** | | | |
| `logLevel` | `info` | `info` | Log level: `debug`, `info`, `warn`, `error` |
| `statsInterval` | `30000` | `60000` | Stats print interval (ms) |
| **Files** | | | |
| `blocklistFile` | `./blocklist.txt` | `/opt/pko-proxy/blocklist.txt` | Path to IP blocklist |

### Config Field Aliases

The proxy accepts alternate field names for backward compatibility:

| Alias | Canonical Name |
|-------|---------------|
| `healthPort` | `healthCheckPort` |
| `maxConnPerIP` | `maxConnectionsPerIP` |

### Hot-Reload

The config is automatically reloaded when the file changes (checked every 30 seconds). You can also trigger immediate reload:

```bash
# Signal-based reload (instant)
kill -SIGHUP $(pgrep -f proxy_v3.js)

# Or just edit config.json — auto-detected within 30 seconds
```

### Multi-Region Support

Run multiple proxy instances on the same or different servers with `--config`:

```bash
node proxy_v3.js --config config.us.json
node proxy_v3.js --config config.eu.json
```

Each config can have a different `regionName`, `regionId`, `listenPort`, and `gateServerHost`.

---

## 8. Systemd Service Configuration

### 8.1 Create Service File

```bash
cat > /etc/systemd/system/smartproxy.service << 'EOF'
[Unit]
Description=PKO Smart Proxy v3 - DDoS Protection
After=network.target

[Service]
Type=simple
WorkingDirectory=/opt/pko-proxy
ExecStart=/usr/bin/node /opt/pko-proxy/proxy_v3.js
Restart=always
RestartSec=3
StandardOutput=append:/var/log/pko-proxy.log
StandardError=append:/var/log/pko-proxy.log
Environment=NODE_ENV=production

[Install]
WantedBy=multi-user.target
EOF
```

**Key settings explained:**
- `Restart=always` — Auto-restart if proxy crashes or is killed
- `RestartSec=3` — Wait 3 seconds before restart
- `StandardOutput=append:` — All logs go to `/var/log/pko-proxy.log`
- `WorkingDirectory=/opt/pko-proxy` — Config.json resolved relative to this

### 8.2 Enable and Start

```bash
systemctl daemon-reload
systemctl enable smartproxy     # Start on boot
systemctl start smartproxy      # Start now

# Verify it's running
systemctl status smartproxy
```

### 8.3 Verify Startup

```bash
tail -20 /var/log/pko-proxy.log
```

Expected output:

```
PKO Smart Proxy v3 starting...
Loaded configuration from config.json
Region: default (default)
Backend: 145.239.149.93:1973
Max connections per IP: 10 (5 during attack)
Fetching RSA handshake...
RSA handshake cached successfully
Health check endpoint: http://127.0.0.1:8080/health (localhost only)
Proxy listening on 0.0.0.0:1973
Auth timeout: 8000ms | Rate limit: 30/10s
Attack threshold: 500/s | Fingerprinting: ON
```

If RSA handshake fails, verify:
1. Your GateServer is running
2. The `gateServerHost` in config.json is correct
3. Your GateServer's firewall allows connections from the proxy VPS IP

**IMPORTANT:** The proxy retries RSA fetch 5 times on startup with exponential backoff (2s, 4s, 6s, 8s, 10s). If all fail, it starts anyway and retries every 15 seconds until successful.

### 8.4 Check for Orphan Processes

After setting up systemd, ensure no old manually-started proxy processes are still running:

```bash
# List all proxy processes
ps aux | grep proxy_v3 | grep -v grep

# Should show exactly ONE process (the systemd-managed one)
# If you see multiple, kill the orphans:
# kill <orphan_pid>

# Verify only the systemd process owns port 1973
ss -tlnp | grep 1973
```

**Why this matters:** If an old manually-started proxy is still running, you'll have TWO proxies — one with old code writing status logs, one with new code handling traffic. This causes confusing log output (alternating uptime values), wastes resources, and the old process may have unfixed bugs. Always ensure only one process is running.

---

## 9. Firewall Rules (nftables)

Ubuntu 24.04 uses **nftables** by default (not iptables).

The firewall uses a **DROP** default policy — only explicitly allowed traffic gets through. SSH is restricted to known admin IPs.

### 9.1 Create Firewall Configuration

```bash
cat > /etc/nftables.conf << 'NFTCONF'
#!/usr/sbin/nft -f

flush ruleset

table inet filter {
    chain input {
        type filter hook input priority 0; policy drop;

        # Allow loopback
        iifname lo accept

        # Allow established/related connections
        ct state established,related accept

        # SSH — restricted to admin IPs only
        tcp dport 22 ip saddr { YOUR_ADMIN_IP, YOUR_GATESERVER_IP } accept

        # Game proxy port — open to all (proxy handles rate limiting)
        tcp dport 1973 accept

        # Health check — localhost only
        ip saddr 127.0.0.1 tcp dport 8080 accept

        # ICMP ping — rate limited
        ip protocol icmp icmp type echo-request limit rate 1/second burst 5 packets accept
    }

    chain forward {
        type filter hook forward priority 0; policy drop;
    }

    chain output {
        type filter hook output priority 0; policy accept;
    }
}
NFTCONF
```

**CRITICAL:** Replace `YOUR_ADMIN_IP` with your public IP and `YOUR_GATESERVER_IP` with `145.239.149.93` (so the GateServer can reach the proxy if needed).

### 9.2 Apply Rules

```bash
nft -f /etc/nftables.conf
systemctl enable nftables
systemctl restart nftables
```

### 9.3 Verify

```bash
nft list ruleset
```

Expected: policy `drop`, SSH restricted, port 1973 open, health on localhost only.

**Note:** If fail2ban is installed, you'll also see a `table inet f2b-table` with SSH ban rules. This is normal and managed by fail2ban — do not modify it.

---

## 10. Kernel / Sysctl Tuning

### 10.1 Apply Network Tuning

```bash
cat > /etc/sysctl.d/99-pko-proxy.conf << 'EOF'

# === PKO Proxy Network Tuning ===
# SYN flood protection
net.ipv4.tcp_syncookies = 1
net.ipv4.tcp_max_syn_backlog = 65535

# Socket listen backlog
net.core.somaxconn = 65535

# Reduce swap usage (prefer RAM)
vm.swappiness = 10

# Disable ICMP redirects (prevent MITM)
net.ipv4.conf.all.accept_redirects = 0
net.ipv4.conf.default.accept_redirects = 0
net.ipv4.conf.all.send_redirects = 0

# Spoof protection (reverse path filtering)
net.ipv4.conf.all.rp_filter = 1
net.ipv4.conf.default.rp_filter = 1
EOF

sysctl --system
```

### 10.2 Verify

```bash
sysctl net.ipv4.tcp_syncookies net.ipv4.tcp_max_syn_backlog net.core.somaxconn vm.swappiness
```

Expected:

```
net.ipv4.tcp_syncookies = 1
net.ipv4.tcp_max_syn_backlog = 65535
net.core.somaxconn = 65535
vm.swappiness = 10
```

---

## 11. Log Rotation

### 11.1 Create Logrotate Config

```bash
cat > /etc/logrotate.d/pko-proxy << 'EOF'
/var/log/pko-proxy.log {
    daily
    rotate 7
    compress
    delaycompress
    missingok
    notifempty
    size 50M
    copytruncate
}
EOF
```

**Settings:** Rotates daily or when file exceeds 50MB, keeps 7 days of compressed logs. Uses `copytruncate` so the proxy doesn't need to be restarted after rotation.

### 11.2 Test Rotation

```bash
logrotate -f /etc/logrotate.d/pko-proxy
```

---

## 12. Fail2Ban (SSH Protection)

### 12.1 Install and Configure

```bash
apt install -y fail2ban

cat > /etc/fail2ban/jail.local << 'EOF'
[sshd]
enabled = true
bantime = 604800
maxretry = 3
findtime = 600
EOF

systemctl enable fail2ban
systemctl restart fail2ban
```

**Settings:** Bans IP for 7 days after 3 failed SSH attempts within 10 minutes.

### 12.2 Verify

```bash
fail2ban-client status sshd
```

---

## 13. GateServer Integration

The proxy sends a **PROXY Protocol v1** header before each player's traffic. Your GateServer must be configured to parse this header to see real client IPs.

### 13.1 GateServer Configuration

In your **GateServer.cfg**, enable PROXY Protocol:

```ini
[AntiDDoS]
ProxyProtocol = 1
```

### 13.2 Trusted IPs

Create/update `TrustedIPs.txt` in your GateServer directory with **all** proxy VPS IPs:

```
146.190.94.84
158.69.211.60
57.129.122.4
```

Only connections from trusted IPs are allowed to send PROXY Protocol headers. All other connections are treated as direct connections (their own IP is used).

### 13.3 How PROXY Protocol Works

When a player connects through the proxy, the following happens:

1. Client TCP connects to proxy:1973
2. Proxy sends RSA challenge to client (cached from GateServer)
3. Client responds with RSA answer (proves it's a real game client)
4. Proxy opens a new TCP connection to GateServer:1973
5. Proxy sends: `PROXY TCP4 <real_client_ip> <gate_ip> <client_port> <gate_port>\r\n`
6. Proxy forwards the client's RSA response to GateServer
7. GateServer parses PROXY header → extracts real client IP
8. All subsequent data is transparently forwarded in both directions

The GateServer's `ParseProxyProtocol()` function (in `source/src/gateserver/ToClient.cpp`) parses the `PROXY TCP4` header and extracts the real client IP, so all logging, anti-cheat, bans, and IP-based features use the player's actual IP, not the proxy's IP.

### 13.4 GateServer Firewall (Recommended)

Your GateServer should only accept connections to port 1973 from the proxy IPs. See [Section 14: OVH Edge Network Firewall](#14-ovh-edge-network-firewall) for the recommended setup.

---

## 14. OVH Edge Network Firewall

If your GateServer is hosted on OVH, use the **Edge Network Firewall** to restrict port 1973 to proxy IPs only. This prevents players from bypassing the proxy by connecting directly to the GateServer.

### 14.1 Configuration (OVH Manager)

Navigate to: **Bare Metal Cloud** → **IP** → Select GateServer IP → **Configure the edge Network Firewall**

| Priority | Action | Protocol | Source IP | Source Port | Dest Port | Purpose |
|----------|--------|----------|-----------|-------------|-----------|----------|
| 0 | Allow | TCP | `57.129.122.4/32` | — | 1973 | EU proxy |
| 1 | Allow | TCP | `158.69.211.60/32` | — | 1973 | CA proxy |
| 2 | Allow | TCP | `146.190.94.84/32` | — | 1973 | SG proxy |
| 3 | Refuse | TCP | Any | — | 1973 | Block all other |
| 19 | Allow | TCP | `148.113.196.82/32` | — | 1433 | SQL access |

**Important:** Rules are evaluated in priority order (lowest number first). The "Refuse all on 1973" rule (priority 3) must come AFTER all proxy Allow rules.

### 14.2 Verification

From each proxy server, verify connectivity:

```bash
nc -zv 145.239.149.93 1973
```

Expected: `Connection to 145.239.149.93 1973 port [tcp/*] succeeded!`

From any other IP, port 1973 should be unreachable.

---

## 15. Game Client Configuration

Update the game client to connect to the **proxy's IP** instead of the GateServer IP directly.

### 15.1 Client Server List

In the client's server configuration (typically Lua scripts in `scripts/` or a server list file), set:

```
Server IP: YOUR_PROXY_VPS_IP
Server Port: 1973
```

### 15.2 DNS (Optional but Recommended)

Point your game domain to the proxy:

```
play.yourgame.com → YOUR_PROXY_VPS_IP
```

This lets you swap proxy servers by updating DNS without patching game clients.

---

## 16. Admin Endpoints

The proxy includes HTTP admin endpoints on the health server (port 8080, localhost only) for manual IP management.

### 16.1 Check IP Status

View fingerprint score, temp ban status, and block status for a specific IP:

```bash
# On the proxy server
curl -s "http://127.0.0.1:8080/check-ip?ip=1.2.3.4" | python3 -m json.tool
```

Response:

```json
{
  "ip": "1.2.3.4",
  "fingerprint": { "score": 30, "connections": 5, "validResponses": 3 },
  "tempBanned": false,
  "blocked": false
}
```

### 16.2 Reset IP

Instantly clear all tracking data for an IP (fingerprint, temp bans, rate limits):

```bash
# On the proxy server
curl -s "http://127.0.0.1:8080/reset-ip?ip=1.2.3.4" | python3 -m json.tool
```

Response:

```json
{
  "ip": "1.2.3.4",
  "reset": true,
  "cleared": ["fingerprint", "tempBan", "rateTracking"]
}
```

### 16.3 Remote Commands (from Windows)

```powershell
# Check an IP on Singapore
ssh -i "$env:USERPROFILE\.ssh\id_pko_proxy_sg" root@146.190.94.84 'curl -s "http://127.0.0.1:8080/check-ip?ip=1.2.3.4"'

# Reset an IP on all 3 servers
$servers = @(
    @{Key="id_pko_proxy_sg"; User="root"; IP="146.190.94.84"},
    @{Key="id_pko_proxy_ca"; User="ubuntu"; IP="158.69.211.60"},
    @{Key="id_pko_proxy_eu"; User="ubuntu"; IP="57.129.122.4"}
)
foreach ($s in $servers) {
    Write-Host "Resetting on $($s.IP)..."
    ssh -i "$env:USERPROFILE\.ssh\$($s.Key)" "$($s.User)@$($s.IP)" 'curl -s "http://127.0.0.1:8080/reset-ip?ip=PLAYER_IP"'
}
```

**Note:** These endpoints are only accessible from localhost (127.0.0.1). The firewall blocks external access to port 8080.

---

## 17. Management Commands

### Service Management

```bash
# Check status
systemctl status smartproxy

# Start / Stop / Restart
systemctl start smartproxy
systemctl stop smartproxy
systemctl restart smartproxy

# View live logs (Ctrl+C to stop)
tail -f /var/log/pko-proxy.log

# View last 50 lines
tail -50 /var/log/pko-proxy.log

# Check systemd journal (alternative log source)
journalctl -u smartproxy -n 50 --no-pager
```

### From Windows (Remote)

```powershell
# Per-region variables
$SG = @{ Key="$env:USERPROFILE\.ssh\id_pko_proxy_sg"; User="root"; IP="146.190.94.84" }
$CA = @{ Key="$env:USERPROFILE\.ssh\id_pko_proxy_ca"; User="ubuntu"; IP="158.69.211.60" }
$EU = @{ Key="$env:USERPROFILE\.ssh\id_pko_proxy_eu"; User="ubuntu"; IP="57.129.122.4" }

# Check status (example: SG)
ssh -i $SG.Key "$($SG.User)@$($SG.IP)" "systemctl status smartproxy --no-pager"

# View logs
ssh -i $SG.Key "$($SG.User)@$($SG.IP)" "tail -50 /var/log/pko-proxy.log"

# Restart
ssh -i $SG.Key "$($SG.User)@$($SG.IP)" "systemctl restart smartproxy"

# Live tail (Ctrl+C to stop)
ssh -i $SG.Key "$($SG.User)@$($SG.IP)" "tail -f /var/log/pko-proxy.log"
```

### Connection Monitoring

```bash
# Count active connections to proxy
ss -tn state established '( sport = :1973 )' | wc -l

# Connections per IP (top 20)
ss -tn state established '( sport = :1973 )' | awk '{print $5}' | cut -d: -f1 | sort | uniq -c | sort -rn | head -20

# Count unique connected IPs
ss -tn state established '( sport = :1973 )' | awk '{print $5}' | cut -d: -f1 | sort -u | wc -l

# Check proxy process (should show exactly 1)
ps aux | grep proxy_v3 | grep -v grep

# Verify process owns the port
ss -tlnp | grep 1973
```

### Health Check (Local on VPS)

```bash
# Full JSON health report (port 8080 is blocked externally — localhost only)
curl -s http://127.0.0.1:8080/health | python3 -m json.tool

# Simple uptime check
curl -s http://127.0.0.1:8080/health/simple
```

Health endpoint response example:

```json
{
  "status": "ok",
  "region": "default",
  "uptime": 3600,
  "mode": "normal",
  "backend": "145.239.149.93:1973",
  "rsaCached": true,
  "rsaAge": 45,
  "connections": {
    "active": 5,
    "total": 1234,
    "authenticated": 1100,
    "perSecond": 2
  },
  "blocked": {
    "blocklist": 0,
    "rateLimit": 50,
    "authTimeout": 30,
    "invalidPacket": 100,
    "fingerprint": 5,
    "packetRate": 2,
    "loginRate": 1,
    "pendingLogin": 0,
    "total": 188
  },
  "pendingLogins": {
    "global": 3,
    "maxGlobal": 100,
    "maxPerIP": 8
  }
}
```

### Blocklist Management

```bash
# Add an IP to blocklist
echo "1.2.3.4" >> /opt/pko-proxy/blocklist.txt

# Add with comment
echo "# Attacker spotted 2026-02-12" >> /opt/pko-proxy/blocklist.txt
echo "1.2.3.4" >> /opt/pko-proxy/blocklist.txt

# Remove an IP
sed -i '/1.2.3.4/d' /opt/pko-proxy/blocklist.txt

# View blocklist
cat /opt/pko-proxy/blocklist.txt

# Blocklist auto-reloads every 60 seconds — NO restart needed
```

### Emergency Log Truncation

```bash
# If log file grows too large and fills disk
truncate -s 0 /var/log/pko-proxy.log
```

---

## 18. Multi-Server Operations

### Deploy to All Servers

```powershell
# Define all servers
$servers = @(
    @{Key="id_pko_proxy_sg"; User="root"; IP="146.190.94.84"; Name="SG"},
    @{Key="id_pko_proxy_ca"; User="ubuntu"; IP="158.69.211.60"; Name="CA"},
    @{Key="id_pko_proxy_eu"; User="ubuntu"; IP="57.129.122.4"; Name="EU"}
)

# Deploy proxy code to all servers
foreach ($s in $servers) {
    Write-Host "Deploying to $($s.Name) ($($s.IP))..."
    scp -i "$env:USERPROFILE\.ssh\$($s.Key)" "helper\smartproxy\proxy_v3.js" "$($s.User)@$($s.IP):/opt/pko-proxy/proxy_v3.js"
    ssh -i "$env:USERPROFILE\.ssh\$($s.Key)" "$($s.User)@$($s.IP)" "sudo systemctl restart smartproxy"
}
```

### Deploy Config to All Servers

```powershell
foreach ($s in $servers) {
    Write-Host "Deploying config to $($s.Name)..."
    scp -i "$env:USERPROFILE\.ssh\$($s.Key)" "helper\smartproxy\config.json" "$($s.User)@$($s.IP):/opt/pko-proxy/config.json"
    ssh -i "$env:USERPROFILE\.ssh\$($s.Key)" "$($s.User)@$($s.IP)" "sudo systemctl restart smartproxy"
}
```

### Status Check All Servers

```powershell
foreach ($s in $servers) {
    Write-Host "`n=== $($s.Name) ($($s.IP)) ==="
    ssh -i "$env:USERPROFILE\.ssh\$($s.Key)" "$($s.User)@$($s.IP)" "systemctl is-active smartproxy; node --version; sha256sum /opt/pko-proxy/proxy_v3.js"
}
```

### Verify Identical Code

```powershell
# Compare proxy_v3.js hashes across all servers (should all match)
foreach ($s in $servers) {
    $hash = ssh -i "$env:USERPROFILE\.ssh\$($s.Key)" "$($s.User)@$($s.IP)" "sha256sum /opt/pko-proxy/proxy_v3.js"
    Write-Host "$($s.Name): $hash"
}
```

---

## 19. Deploying Updates

### Upload New Proxy Code

From **Windows** (PowerShell):

```powershell
# Single server example (SG)
scp -i "$env:USERPROFILE\.ssh\id_pko_proxy_sg" "helper\smartproxy\proxy_v3.js" root@146.190.94.84:/opt/pko-proxy/proxy_v3.js

# Then restart
ssh -i "$env:USERPROFILE\.ssh\id_pko_proxy_sg" root@146.190.94.84 "systemctl restart smartproxy; sleep 2; systemctl status smartproxy --no-pager"
```

For deploying to all servers at once, see [Section 18: Multi-Server Operations](#18-multi-server-operations).

### Upload New Config

```powershell
# Single server example (SG)
scp -i "$env:USERPROFILE\.ssh\id_pko_proxy_sg" "helper\smartproxy\config.json" root@146.190.94.84:/opt/pko-proxy/config.json

# Config auto-reloads within 30 seconds, OR force immediate reload:
ssh -i "$env:USERPROFILE\.ssh\id_pko_proxy_sg" root@146.190.94.84 "kill -SIGHUP \$(pgrep -f proxy_v3.js)"
```

### Post-Deploy Verification Checklist

After every deploy, verify:

1. **Service running:** `systemctl status smartproxy`
2. **RSA cached:** Look for `RSA handshake cached successfully` in logs
3. **Listening:** `ss -tlnp | grep 1973`
4. **One process:** `ps aux | grep proxy_v3 | grep -v grep` (exactly 1 result)
5. **Code hash matches:** `sha256sum /opt/pko-proxy/proxy_v3.js`
6. **Test login:** Connect with game client and log in successfully

---

## 20. Troubleshooting

### Proxy Won't Start

```bash
# Check systemd journal for errors
journalctl -u smartproxy -n 50 --no-pager

# Run manually to see errors directly (stop service first)
systemctl stop smartproxy
cd /opt/pko-proxy && node proxy_v3.js
# Ctrl+C to stop, then restart service:
systemctl start smartproxy
```

Common causes:
- Node.js not installed (`node: command not found`)
- Syntax error in proxy_v3.js (corrupted upload)
- Port 1973 already in use by another process
- Invalid JSON in config.json (check with `python3 -m json.tool < config.json`)

### RSA Handshake Fails

The proxy fetches the RSA handshake from your GateServer on startup and refreshes every 60 seconds.

```bash
# Test GateServer connectivity from the proxy VPS
nc -zv 145.239.149.93 1973

# Check config
cat /opt/pko-proxy/config.json | grep gateServer
```

Common causes:
- GateServer not running
- Wrong `gateServerHost` in config.json
- GateServer firewall blocking the proxy VPS IP (check OVH Edge Firewall rules)
- GateServer doesn't have PROXY Protocol enabled (`ProxyProtocol = 1`)

**RSA Auto-Recovery Behavior:**
- When GateServer goes down, the proxy invalidates the cached RSA handshake
- It retries fetching RSA every 15 seconds until successful
- Once GateServer comes back up, the proxy auto-recovers within 15 seconds
- **No manual restart needed** for RSA recovery in normal circumstances
- On first startup, it retries 5 times with exponential backoff before giving up

### Players Get "Connection Failed"

Diagnostic steps (run these on the **proxy VPS**):

```bash
# 1. Is proxy running?
systemctl status smartproxy
ss -tlnp | grep 1973

# 2. Is GateServer reachable from proxy?
nc -zv 145.239.149.93 1973

# 3. Is player's IP being blocked? (replace PLAYER_IP)
grep "PLAYER_IP" /var/log/pko-proxy.log | tail -20

# 4. Check IP status with admin endpoint
curl -s "http://127.0.0.1:8080/check-ip?ip=PLAYER_IP" | python3 -m json.tool

# 5. Reset the player's IP if blocked by fingerprinting
curl -s "http://127.0.0.1:8080/reset-ip?ip=PLAYER_IP" | python3 -m json.tool

# 6. Is the pending login queue full?
curl -s http://127.0.0.1:8080/health | python3 -c "import sys,json; d=json.load(sys.stdin); print(f'Pending: {d[\"pendingLogins\"][\"global\"]}/{d[\"pendingLogins\"][\"maxGlobal\"]}')"

# 7. Is RSA handshake cached?
curl -s http://127.0.0.1:8080/health | python3 -c "import sys,json; d=json.load(sys.stdin); print(f'RSA cached: {d[\"rsaCached\"]}, age: {d[\"rsaAge\"]}s')"
```

### Players Can Login But Get Kicked Immediately

This is NOT a proxy issue. The proxy is forwarding traffic correctly. Check:
- GameServer status (is it running/crashing?)
- GateServer logs for "GameServer trouble/malfunction" messages
- GameServer logs for crash dumps or errors

### Connection Issues After GateServer Restart

The proxy automatically invalidates and re-fetches the RSA handshake when:
- A GateServer connection fails (ECONNRESET, ECONNREFUSED, etc.)
- A GateServer connection times out
- The periodic refresh fails

Self-healing happens within 15 seconds. If it doesn't, restart:

```bash
systemctl restart smartproxy
```

### Two Proxy Processes Running (Orphan Detection)

Symptom: Log shows alternating uptime values (e.g., "Uptime: 5h" then "Uptime: 0h 10m").

```bash
# Find all proxy processes
ps aux | grep proxy_v3 | grep -v grep

# Should show exactly ONE. The systemd PID is shown in:
systemctl status smartproxy

# Kill any other PIDs (orphans from before systemd was set up)
kill <orphan_pid>

# Verify
ps aux | grep proxy_v3 | grep -v grep
ss -tlnp | grep 1973
```

### High Memory / CPU

```bash
# Process stats
top -p $(pgrep -f "node proxy")

# Active connection count
ss -tn state established '( sport = :1973 )' | wc -l

# Log size (can cause disk pressure if rotation isn't working)
ls -lh /var/log/pko-proxy.log

# Emergency truncate if needed
truncate -s 0 /var/log/pko-proxy.log
```

### Port 1973 Not Listening

```bash
ss -tlnp | grep 1973
```

If empty:
- Check startup errors: `tail -50 /var/log/pko-proxy.log`
- Check if another process has the port: `ss -tlnp | grep 1973`
- Try manual start: `systemctl stop smartproxy && cd /opt/pko-proxy && node proxy_v3.js`

### Config Not Loading

```bash
# Validate JSON syntax
python3 -m json.tool < /opt/pko-proxy/config.json

# Check proxy log for config reload messages
grep "Config" /var/log/pko-proxy.log | tail -10
```

**WARNING about PowerShell + JSON:** When creating config.json by piping PowerShell output through SSH, special characters can get mangled. Always upload config using SCP.

---

## 21. How the Proxy Works (Technical Deep-Dive)

This section explains the proxy internals for advanced maintenance and debugging.

### Connection Lifecycle

```
1. Client TCP connects to proxy:1973
2. Proxy checks:
   a. IP in blocklist? → DROP (no response sent)
   b. IP temp-banned? → DROP (no response sent)
   c. IP fingerprint suspicious? → DROP (no response sent)
   d. Connection rate exceeded? → DROP (no response sent)
   e. Too many concurrent connections? → DROP (no response sent)
3. Proxy sends cached RSA handshake packet to client
4. Start 8-second auth timer
5. Client sends RSA response packet
6. Proxy validates response:
   a. Valid PKO packet format? (2-byte BE length header, 8+ bytes min)
   b. Valid command ID? (< 500, client→server range)
   c. Response time reasonable? (>50ms, <15000ms)
7. If valid → Client AUTHENTICATED
8. Proxy opens new TCP connection to GateServer
9. Proxy sends PROXY protocol v1 header:
   "PROXY TCP4 <client_ip> <gate_ip> <client_port> <gate_port>\r\n"
10. Proxy forwards client's RSA response to GateServer
11. Phase transitions to FORWARDING — bidirectional transparent relay begins
12. Post-auth monitoring on client→gate data:
    - Count packets per TCP segment (PKO framing: 2-byte BE length)
    - Check against packet rate limit (300/s)
    - Detect CMD_CM_LOGIN (command ID 431) → apply login rate limit
    - Track pending login slots for lobby queueing
13. Post-auth monitoring on gate→client data:
    - Detect CMD_MC_LOGIN (command ID 931) → release pending login slot
14. On disconnect from either side → cleanup both sockets + release all slots
```

### RSA Handshake Caching

The proxy pre-fetches the RSA handshake from the GateServer and caches it in memory. This avoids creating a GateServer connection for every client connection attempt (which would exhaust GateServer threads during DDoS).

**Normal operation:**
- Refreshes cached RSA every 60 seconds (`rsaRefreshInterval`)
- Uses the last valid cached handshake for all client connections
- The RSA handshake is the first packet GateServer sends on connect (636 bytes typically)

**When GateServer goes down:**
- GateServer connections fail → proxy sets `cachedRsaHandshake = null`
- Retry interval drops to every 15 seconds
- New client connections fail with "Failed to get RSA handshake"
- Once GateServer comes back, next retry succeeds and caching resumes

**Startup behavior:**
- Tries 5 times with exponential backoff: 2s, 4s, 6s, 8s, 10s delays
- If all 5 fail, proxy starts anyway and retries every 15s in background
- Proxy can serve clients as soon as first RSA fetch succeeds

### Attack Mode

When total connection rate exceeds `attackThreshold` (default: 500 connections/sec across all IPs), the proxy enters attack mode:

| Setting | Normal | Attack Mode |
|---------|--------|-------------|
| `maxConnectionsPerIP` | 20 | 10 (×0.5) |
| `connectionRateLimit` | 30/10s | 15/10s (×0.5) |
| `tempBanDuration` | 30s | 60s (×2) |

- Attack mode activates once rate exceeds threshold
- Stays active for 60 seconds (`normalModeDelay`) after rate drops below threshold
- Logged as `ATTACK MODE ACTIVATED` / `Attack mode deactivated`

### PKO Packet Format

```
Offset  Size  Field
0       2     Packet length (uint16 BE, includes this header)
2       4     Session ID (uint32 BE, RPC layer)
6       2     Command ID (uint16 BE)
8+      var   Payload data
```

The proxy parses this format to:
1. Validate client RSA responses (checks command ID < 500)
2. Count packets per TCP segment for rate limiting
3. Detect login commands: CMD_CM_LOGIN (431), CMD_MC_LOGIN (931)

### Connection Fingerprinting

Each IP builds a fingerprint score from -100 to +100:

| Event | Score Change |
|-------|-------------|
| Valid RSA response | +10 |
| Invalid RSA response | -5 |
| Suspiciously fast response (<50ms) | -3 |
| Normal response time | No change |

An IP is blocked by fingerprinting when:
- 10+ connections with 0 valid responses
- Score drops below -80
- More than 80% of responses are suspiciously fast (5+ connections)

Fingerprint data ages out after 30 minutes (positive scores) or 24 hours (negative scores).

### State Maps (Memory)

| Map | Key | Purpose | Cleanup Interval |
|-----|-----|---------|-----------------|
| `connections` | Socket | Connection state (phase, gate socket, timers) | On disconnect |
| `ipTracker` | IP string | Connection rate + concurrent count | 10 min |
| `tempBlacklist` | IP string | Temp bans with expiry timestamp | 10 min |
| `blocklist` | IP string | Permanent blocklist from file | On file reload (60s) |
| `fingerprints` | IP string | Bot detection scores per IP | 10 min (30min/1min age) |
| `packetRates` | IP string | Post-auth packet count per window | 10 min |
| `loginRates` | IP string | Login attempt timestamps | 10 min |
| `pendingLogins` | IP string | Concurrent pending login count | On response/timeout |

### Log Format

```
[2026-02-12T08:52:04.798Z] [default] [INFO] Authenticated {"connId":"136.158.102.175:57837","responseTime":"39ms"}
```

Format: `[ISO_TIMESTAMP] [REGION] [LEVEL] MESSAGE {JSON_DATA}`

### Key Log Messages

| Message | Meaning | Action |
|---------|---------|--------|
| `Authenticated` | Player passed RSA challenge | Normal — player connected |
| `Auth timeout` | Client didn't respond in 8s | Scanner or very slow connection |
| `Invalid packet` | Non-PKO traffic hit port | Port scanner or corruption |
| `ATTACK MODE ACTIVATED` | >500 conn/s detected | Rate limits automatically halved |
| `Attack mode deactivated` | Rate returned to normal | Limits restored |
| `Packet rate exceeded` | Auth'd player sending >300 pkt/s | Player temp-banned, possible bot |
| `Login rate exceeded` | >20 login attempts/min from IP | IP temp-banned |
| `Pending login queue full` | All login slots used | Queue overloaded — increase limits? |
| `GateServer connection failed` | Can't connect to backend | GateServer down? Check backend |
| `RSA handshake cached` | Successfully refreshed RSA | Normal periodic refresh |
| `Failed to get RSA handshake` | Can't reach GateServer for RSA | GateServer unreachable |
| `Config reloaded` | config.json changed and loaded | Hot-reload successful |
| `Cleanup cycle` | Old tracking data cleaned up | Normal maintenance (every 10 min) |

---

## 22. Quick Setup Checklist

Copy this checklist for setting up a new proxy server:

```
[ ] 1.  Provision VPS (Ubuntu 24.04 LTS, 1+ vCPU, 1+ GB RAM)
[ ] 2.  apt update && apt upgrade -y
[ ] 3.  Setup SSH key (id_pko_proxy_<region>) and disable password auth
[ ] 4.  Install Node.js 20.x
[ ] 5.  mkdir -p /opt/pko-proxy
[ ] 6.  Upload proxy_v3.js from helper/smartproxy/proxy_v3.js in repo
[ ] 7.  Create config.json — SET gateServerHost to 145.239.149.93
[ ] 8.  Create empty blocklist.txt
[ ] 9.  Create systemd service file (smartproxy.service) — log to /var/log/pko-proxy.log
[ ] 10. systemctl daemon-reload && systemctl enable smartproxy && systemctl start smartproxy
[ ] 11. Verify logs: tail -20 /var/log/pko-proxy.log (RSA cached + listening)
[ ] 12. Verify port: ss -tlnp | grep 1973
[ ] 13. Verify one process: ps aux | grep proxy_v3 | grep -v grep
[ ] 14. Setup nftables firewall (DROP policy, SSH restricted to admin IPs)
[ ] 15. Apply sysctl tuning (/etc/sysctl.d/99-pko-proxy.conf)
[ ] 16. Create logrotate config (/etc/logrotate.d/pko-proxy, copytruncate)
[ ] 17. Install fail2ban
[ ] 18. Configure GateServer.cfg: ProxyProtocol = 1 under [AntiDDoS]
[ ] 19. Add proxy VPS IP to GateServer's TrustedIPs.txt
[ ] 20. Add proxy VPS IP to OVH Edge Network Firewall (Allow TCP on port 1973)
[ ] 21. Update game client server list to connect to proxy IP:1973
[ ] 22. Test: Login through proxy and verify it works
[ ] 23. Verify GateServer sees real client IP (not proxy IP) in its logs
[ ] 24. Verify admin endpoints: curl http://127.0.0.1:8080/check-ip?ip=127.0.0.1
[ ] 25. Verify code hash matches other servers: sha256sum /opt/pko-proxy/proxy_v3.js
```

### Values to Change Per Deployment

| What | Where | Set To |
|------|-------|--------|
| GateServer IP | `config.json` → `gateServerHost` | `145.239.149.93` |
| Proxy VPS IP | Game client server list | This proxy server's public IP |
| Proxy VPS IP | GateServer `TrustedIPs.txt` | This proxy server's public IP |
| Proxy VPS IP | OVH Edge Firewall rules | Allow TCP on port 1973 |
| SSH key name | `$env:USERPROFILE\.ssh\id_pko_proxy_<region>` | One key per region (sg, ca, eu) |

### Files Reference

| Local (Repository) | Remote (VPS) | Purpose |
|---------------------|-------------|---------|
| `helper/smartproxy/proxy_v3.js` | `/opt/pko-proxy/proxy_v3.js` | Proxy application code |
| `helper/smartproxy/config.json` | `/opt/pko-proxy/config.json` | Proxy configuration |
| — | `/opt/pko-proxy/blocklist.txt` | IP blocklist (created on server) |
| — | `/var/log/pko-proxy.log` | Runtime log file (auto-created) |
| — | `/etc/systemd/system/smartproxy.service` | Systemd service unit |
| — | `/etc/logrotate.d/pko-proxy` | Log rotation config |
| — | `/etc/nftables.conf` | Firewall rules |
| — | `/etc/sysctl.d/99-pko-proxy.conf` | Kernel tuning |

---

**Document Version:** 4.0  
**Last Updated:** February 13, 2026  
**Based on:** Ubuntu 24.04 LTS, Node.js v20.20.0, PKO Smart Proxy v3
