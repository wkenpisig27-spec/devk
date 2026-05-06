# PKO Smart Proxy v3 - DDoS Protection System

> **Last Updated:** February 10, 2026  
> **Status:** Production Ready ✅

## Overview

A Node.js-based smart proxy that filters DDoS attack traffic using RSA handshake verification. Only legitimate game clients that complete the RSA handshake are forwarded to the actual GateServer. Supports **multi-region deployment** for lower latency worldwide.

## Quick Reference

| Component | Value |
|-----------|-------|
| **EU Proxy IP** | `57.129.122.4` |
| **Proxy Port** | `1973` |
| **Health Check** | `http://<proxy-ip>:8405/health` |
| **GateServer IP** | `145.239.149.93` |
| **GateServer Port** | `1973` |
| **SSH Access** | `ssh -i ~/.ssh/id_rsa ubuntu@57.129.122.4` |

## Architecture

```
                    ┌─────────────────────────────────────┐
                    │         INTERNET                    │
                    │  (DDoS attacks + legitimate users)  │
                    └──────────────┬──────────────────────┘
                                   │
              ┌────────────────────┼────────────────────┐
              ▼                    ▼                    ▼
   ┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐
   │  EU Proxy (main) │ │  US Proxy        │ │  Asia Proxy      │
   │  57.129.122.4    │ │  <us-vps-ip>     │ │  <asia-vps-ip>   │
   │  :1973           │ │  :1973           │ │  :1973           │
   │  Smart Proxy v3  │ │  Smart Proxy v3  │ │  Smart Proxy v3  │
   │  RSA challenge   │ │  RSA challenge   │ │  RSA challenge   │
   │  ✅/❌ filter    │ │  ✅/❌ filter    │ │  ✅/❌ filter    │
   └────────┬─────────┘ └────────┬─────────┘ └────────┬─────────┘
            │                    │                     │
            │    Only authenticated connections        │
            └────────────────────┼─────────────────────┘
                                 ▼
                    ┌─────────────────────────────────────┐
                    │      Game Server (Windows)          │
                    │      145.239.149.93:1973            │
                    │                                     │
                    │      GateServer.exe                 │
                    └─────────────────────────────────────┘
```

## How RSA Verification Works

```
1. Proxy caches RSA handshake from GateServer (every 60s)
2. Client connects to proxy
3. Proxy sends cached RSA challenge to client
4. Client responds with encrypted handshake
5. Proxy compares:
   ├─ ✅ Match → Forward to GateServer (real player)
   └─ ❌ No match → Block (bot/attack)
```

### Why Bots Can't Pass

- RSA private key is generated at GateServer startup
- Private key is NEVER transmitted over the network
- Breaking 1024-bit RSA would take trillions of years
- Bots can only send garbage data → instant rejection

## File Locations

### On Proxy Server (Ubuntu)

| File | Path |
|------|------|
| Proxy Script | `/opt/pko-proxy/proxy_v3.js` |
| Proxy Log | `/opt/pko-proxy/proxy.log` |
| Blocklist | `/opt/pko-proxy/blocklist.txt` |
| Systemd Service | `/etc/systemd/system/pko-proxy.service` |
| Logrotate Config | `/etc/logrotate.d/pko-proxy` |
| Logrotate Cron | `/etc/cron.d/pko-proxy-logrotate` |

### Local Development

| File | Path |
|------|------|
| Source Code | `helper/smartproxy/proxy_v3.js` |
| Documentation | `helper/smartproxy/README.md` |

## Installation

### On Ubuntu Proxy Server

```bash
# Install Node.js 18+
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt-get install -y nodejs

# Create directory
sudo mkdir -p /opt/pko-proxy
sudo chown ubuntu:ubuntu /opt/pko-proxy

# Upload proxy_v3.js
scp -i ~/.ssh/id_rsa helper/smartproxy/proxy_v3.js ubuntu@57.129.122.4:/opt/pko-proxy/

# Create empty blocklist
touch /opt/pko-proxy/blocklist.txt
```

### Systemd Service

```bash
sudo tee /etc/systemd/system/pko-proxy.service > /dev/null << 'EOF'
[Unit]
Description=PKO Smart Proxy v3
After=network.target

[Service]
Type=simple
User=ubuntu
WorkingDirectory=/opt/pko-proxy
ExecStart=/usr/bin/node proxy_v3.js
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable pko-proxy
sudo systemctl start pko-proxy
```

### Log Rotation Setup

```bash
# Logrotate config
sudo tee /etc/logrotate.d/pko-proxy > /dev/null << 'EOF'
/opt/pko-proxy/proxy.log {
    size 100M
    rotate 3
    compress
    missingok
    notifempty
    copytruncate
}
EOF

# Cron job (every 5 minutes)
echo '*/5 * * * * root /usr/sbin/logrotate /etc/logrotate.d/pko-proxy' | sudo tee /etc/cron.d/pko-proxy-logrotate
```

## Configuration

Settings are loaded from `config.json` (or a custom file via `--config`). Only values you want to override need to be in the config file — defaults are built into `proxy_v3.js`.

```json
{
    "regionName": "Europe",
    "regionId": "eu",
    "healthCheckPort": 8405,
    "healthCheckEnabled": true,
    "maxConnectionsPerIP": 20,
    "connectionRateLimit": 30,
    "authTimeout": 15000,
    "tempBanDuration": 120000
}
```

See `config.eu.json`, `config.us.json`, `config.asia.json` for regional templates.

## Common Operations

### Check Status
```bash
ssh -i ~/.ssh/id_rsa ubuntu@57.129.122.4 "tail -30 /opt/pko-proxy/proxy.log"
```

### Restart Proxy
```bash
ssh -i ~/.ssh/id_rsa ubuntu@57.129.122.4 "sudo systemctl restart pko-proxy"
```

### View Service Status
```bash
ssh -i ~/.ssh/id_rsa ubuntu@57.129.122.4 "sudo systemctl status pko-proxy"
```

### Check Resource Usage
```bash
ssh -i ~/.ssh/id_rsa ubuntu@57.129.122.4 "ps aux | grep proxy_v3 | grep -v grep"
```

### Upload New Version (from Windows PowerShell)
```powershell
scp -i "$env:USERPROFILE\.ssh\id_rsa" "helper\smartproxy\proxy_v3.js" ubuntu@57.129.122.4:/opt/pko-proxy/proxy_v3.js
ssh -i "$env:USERPROFILE\.ssh\id_rsa" ubuntu@57.129.122.4 "sudo systemctl restart pko-proxy"
```

### Clear Log (Emergency)
```bash
ssh -i ~/.ssh/id_rsa ubuntu@57.129.122.4 "sudo truncate -s 0 /opt/pko-proxy/proxy.log"
```

## Understanding Stats

```
=== PKO Smart Proxy v3 Stats ===
Uptime: 0h 43m | Mode: NORMAL
Active: 2916 | Total: 1348460 | Auth: 158
Rate: 122/s | Blocked: 1124374
Forwarded: 15.28 MB
================================
```

| Metric | Description |
|--------|-------------|
| **Uptime** | Time since proxy started |
| **Mode** | `NORMAL` or `ATTACK` (>500 conn/s) |
| **Active** | Currently open connections (mostly bots timing out) |
| **Total** | All connection attempts since startup |
| **Auth** | Players that passed RSA (real clients) |
| **Rate** | Current connections per second |
| **Blocked** | Failed RSA verification (attacks) |
| **Forwarded** | Data sent to GateServer |

## Attack Mode

| Setting | Value |
|---------|-------|
| **Threshold** | 500 connections/second |
| **Activation** | Immediate when exceeded |
| **Deactivation** | 60 seconds after rate drops |
| **Behavior** | Stricter timeouts, aggressive filtering |

## Connection Lifecycle

```
Bot connects → Active +1
  ↓
Proxy sends RSA challenge
  ↓
Bot sends garbage (or nothing)
  ↓
Timeout (30s) → Blocked +1, Active -1, Connection closed

---

Player connects → Active +1
  ↓
Proxy sends RSA challenge
  ↓
Client responds with valid RSA
  ↓
Auth +1 → Forward to GateServer
```

## Security Layers

1. **RSA Verification** - Only real game clients can pass
2. **UFW Firewall** - Only ports 22, 1973 open
3. **Fail2Ban** - Blocks SSH brute force attempts
4. **Connection Timeout** - 30s max for handshake
5. **Attack Mode** - Stricter filtering during attacks

## Troubleshooting

### Proxy Not Starting
```bash
sudo journalctl -u pko-proxy -n 50 --no-pager
```

### GateServer Unreachable
```bash
# Test connectivity from proxy
nc -zv 145.239.149.93 1973
```

### RSA Handshake Errors
- Check if GateServer is running
- Verify GateServer IP/port in config
- Check firewall allows proxy IP

### High Memory Usage
```bash
# Restart to clear memory
sudo systemctl restart pko-proxy
```

### Log File Too Large
```bash
sudo truncate -s 0 /opt/pko-proxy/proxy.log
sudo logrotate -f /etc/logrotate.d/pko-proxy
```

## Performance Expectations

| Metric | Normal Range |
|--------|--------------|
| **Memory** | 50-150 MB |
| **CPU** | 5-15% |
| **Block Rate** | 80-95% |
| **Active Connections** | 2,000-5,000 (mostly bots timing out) |

## Production Results (Feb 1, 2026)

After 43 minutes during active DDoS attack:

| Metric | Result |
|--------|--------|
| Total Connections | 1,348,460 |
| Attacks Blocked | 1,124,374 (83.4%) |
| Players Authenticated | 158 |
| Data Forwarded | 15.28 MB |
| Memory Usage | 130 MB |
| CPU Usage | 9.1% |
| Crashes | 0 |

## Why RSA Can't Be Broken

- RSA private key generated at GateServer startup
- Private key NEVER leaves server memory
- Breaking 1024-bit RSA: ~300 trillion years
- If someone could break RSA, they'd empty banks, not DDoS games

---

*For detailed memory/instructions, see `.agents/memory.instruction.md`*

## Multi-Region Deployment

### Architecture

```
EU Players ──→ EU Proxy (57.129.122.4:1973) ──┐
                                               │
US Players ──→ US Proxy (us-vps-ip:1973)   ──┤──→ GateServer (145.239.149.93:1973)
                                               │
Asia Players → Asia Proxy (asia-vps-ip:1973)──┘
```

Each regional proxy:
- Runs the **same** `proxy_v3.js` code
- Has its own `config.json` with region name
- Caches the RSA handshake from GateServer
- Handles DDoS filtering locally
- Forwards only authenticated connections
- Exposes a `/health` endpoint for monitoring

### Setting Up a New Regional Proxy

**1. Provision VPS** in the target region (Ubuntu 22.04+ recommended, 1 vCPU / 1GB RAM minimum)

**2. Install Node.js & upload files:**
```bash
# On the new VPS
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt-get install -y nodejs
sudo mkdir -p /opt/pko-proxy
sudo chown ubuntu:ubuntu /opt/pko-proxy
```

```powershell
# From your Windows machine - upload proxy + config
scp -i "$env:USERPROFILE\.ssh\id_rsa" "helper\smartproxy\proxy_v3.js" ubuntu@<NEW-VPS-IP>:/opt/pko-proxy/
scp -i "$env:USERPROFILE\.ssh\id_rsa" "helper\smartproxy\config.us.json" ubuntu@<NEW-VPS-IP>:/opt/pko-proxy/config.json
```

**3. Create systemd service:**
```bash
sudo tee /etc/systemd/system/pko-proxy.service > /dev/null << 'EOF'
[Unit]
Description=PKO Smart Proxy v3
After=network.target

[Service]
Type=simple
User=ubuntu
WorkingDirectory=/opt/pko-proxy
ExecStart=/usr/bin/node proxy_v3.js
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable pko-proxy
sudo systemctl start pko-proxy
```

**4. Configure firewall:**
```bash
sudo ufw allow 22/tcp
sudo ufw allow 1973/tcp
sudo ufw allow 8405/tcp   # Health check endpoint
sudo ufw enable
```

**5. Add proxy IP to GateServer whitelist** (on game server):

Edit `GateServer.cfg` → `[AntiDDoS]` section:
```ini
Whitelist = 127.0.0.1,57.129.122.4,<NEW-VPS-IP>
```
Also add to `TrustedIPs.txt`:
```
<NEW-VPS-IP>
```

**6. Restart GateServer** to pick up the new whitelist.

**7. Update DNS** to point players in that region to the new proxy IP, or give them the IP directly.

### Using CLI Config

Instead of renaming config files, you can specify which config to use:
```bash
node proxy_v3.js --config config.us.json
```

Update systemd service accordingly:
```ini
ExecStart=/usr/bin/node proxy_v3.js --config config.us.json
```

### Config Differences Per Region

Distant regions need more generous timeouts since packets travel further:

| Setting | EU (close) | US (medium) | Asia (far) |
|---------|-----------|-------------|------------|
| `authTimeout` | 15000 | 20000 | 25000 |
| `maxResponseTimeMs` | 20000 | 25000 | 30000 |
| `gateServerConnectTimeout` | 15000 | 15000 | 20000 |

### Monitoring All Regions

Each proxy exposes a JSON health endpoint:

```bash
# Check EU proxy
curl http://57.129.122.4:8405/health

# Check US proxy
curl http://<US-VPS-IP>:8405/health

# Simple uptime check (for monitoring tools)
curl http://<PROXY-IP>:8405/health/simple
```

Example health response:
```json
{
  "status": "ok",
  "region": "US East",
  "regionId": "us-east",
  "uptime": 3600,
  "mode": "normal",
  "connections": {
    "active": 12,
    "total": 5400,
    "authenticated": 340,
    "perSecond": 3
  },
  "blocked": { "total": 120 },
  "traffic": { "mbForwarded": 45.2 }
}
```

### Example Config Files

| File | Region | Description |
|------|--------|-------------|
| `config.json` | EU (main) | Current production config |
| `config.eu.json` | Europe | Template for EU proxy |
| `config.us.json` | US East | Template for US proxy |
| `config.asia.json` | Asia | Template for Asia proxy (highest timeouts) |
