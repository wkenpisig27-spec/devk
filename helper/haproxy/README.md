# PKO HAProxy Proxy Server Backup

Quick deployment package for setting up a DDoS protection proxy.

## Files Included

| File | Purpose |
|------|---------|
| `haproxy.cfg` | Main HAProxy configuration |
| `blocklist.acl` | Blocked attack subnets |
| `setup.sh` | One-command installation script |

## Quick Setup (New Proxy Server)

### 1. Order a VPS
- Provider: Any (OVH, Hetzner, DigitalOcean, Vultr, etc.)
- OS: Ubuntu 22.04 or 24.04
- Size: Smallest tier ($5/month is fine)
- Location: Close to your players

### 2. Upload Files
```bash
# From your local machine (run in this folder)
scp haproxy.cfg blocklist.acl setup.sh root@NEW_VPS_IP:/root/
```

### 3. Configure (IMPORTANT!)
Edit `haproxy.cfg` before running setup:
```bash
ssh root@NEW_VPS_IP
nano /root/haproxy.cfg

# Change line 75: Your game server IP
# server gameserver YOUR_GAMESERVER_IP:1973 ...
```

### 4. Run Setup
```bash
chmod +x setup.sh
sudo ./setup.sh
```

### 5. Update DNS
Point your game DNS to the new proxy IP:
```
play.yourdomain.com → NEW_VPS_IP
```

### 6. Whitelist Proxy on Game Server
Make sure your game server firewall allows the new proxy IP.

---

## Configuration Reference

### What to Change

| Setting | Location | Current Value |
|---------|----------|---------------|
| Game server IP | haproxy.cfg line 75 | 51.75.251.180 |
| Stats password | haproxy.cfg line 47 | PKOProxy2026! |
| Blocked subnets | blocklist.acl | 19 subnets |

### Rate Limits

| Limit | Value | Purpose |
|-------|-------|---------|
| conn_rate | 5/10s | Max 5 connections per 10 seconds per IP |
| conn_cur | 3 | Max 3 concurrent connections per IP |
| maxconn | 2000 | Queue starts after 2000 connections |
| timeout queue | 30s | How long players wait in queue |

### Ports

| Port | Service |
|------|---------|
| 1973 | GateServer (players connect here) |
| 8404 | Stats dashboard (HTTP) |

---

## Management Commands

```bash
# Check status
sudo systemctl status haproxy

# View logs
sudo journalctl -u haproxy -f

# Reload config after changes
sudo systemctl reload haproxy

# View connected IPs
echo "show table ft_gateserver" | sudo socat stdio /run/haproxy/admin.sock

# Block an IP
echo "1.2.3.4" | sudo tee -a /etc/haproxy/blocklist.acl
sudo systemctl reload haproxy
```

---

## Failover Procedure

If current proxy is dead/overwhelmed:

1. **Spin up new VPS** (5 min)
2. **Upload files + run setup.sh** (10 min)
3. **Update DNS A record** to new IP
4. **Wait for DNS propagation** (5-30 min)
5. Players reconnect automatically

---

## Current Proxy Info

- **IP**: 57.129.122.4
- **Stats**: http://57.129.122.4:8404/stats
- **Login**: admin / PKOProxy2026!
