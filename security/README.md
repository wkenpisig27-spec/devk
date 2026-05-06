# 🛡️ VPS Security Guide for Game Server Hosting

This directory contains security scripts, configurations, and documentation to protect your Windows VPS from attacks, particularly RDP flooding and DDoS attacks.

## 📁 Directory Structure

```
security/
├── README.md                    # This file - Overview and quick start
├── CHECKLIST.md                 # Step-by-step security checklist
├── scripts/
│   ├── 01-change-rdp-port.ps1   # Change RDP from 3389 to custom port
│   ├── 02-enable-nla.ps1        # Enable Network Level Authentication
│   ├── 03-rdp-bruteforce-blocker.ps1  # Auto-ban failed login IPs
│   ├── 04-setup-auto-ban-task.ps1     # Schedule the auto-ban script
│   ├── 05-ip-whitelist.ps1      # Whitelist specific IPs for RDP
│   ├── 06-account-lockout.ps1   # Enable account lockout policy
│   ├── 07-audit-connections.ps1 # Diagnostic - see who's attacking
│   ├── 08-emergency-lockdown.ps1     # Emergency - block all RDP
│   └── 09-full-security-setup.ps1    # All-in-one setup script
└── configs/
    ├── ovh-firewall-rules.md    # OVH Dashboard firewall configuration
    ├── gateserver-antiddos.cfg  # Recommended GateServer AntiDDoS settings
    └── windows-firewall-export.md  # How to export/import firewall rules
```

## 🚀 Quick Start

### Option 1: Run the All-in-One Setup (Recommended)
```powershell
# Run as Administrator
cd C:\path\to\security\scripts
.\09-full-security-setup.ps1
```

### Option 2: Step-by-Step Setup
1. **Change RDP Port** (Most Important!)
   ```powershell
   .\01-change-rdp-port.ps1 -NewPort 41592
   ```

2. **Enable Network Level Authentication**
   ```powershell
   .\02-enable-nla.ps1
   ```

3. **Set up Auto-Ban for Brute Force**
   ```powershell
   .\04-setup-auto-ban-task.ps1
   ```

4. **Configure OVH Firewall** - Follow `configs/ovh-firewall-rules.md`

## ⚠️ Important Notes

- **Always test in a safe environment first**
- **Keep a backup RDP session open when making changes**
- **Note down your new RDP port - you'll need it to connect!**
- **Have OVH console access ready in case you lock yourself out**

## 🔥 Common Attack Patterns & Solutions

| Attack Type | Symptoms | Solution |
|------------|----------|----------|
| RDP Brute Force | Many Event ID 4625 in Security log | Auto-ban script + IP whitelist |
| RDP Port Flood | Server unresponsive, high CPU | Change port + OVH Firewall |
| SYN Flood | Many SYN_RECEIVED connections | OVH Anti-DDoS + rate limiting |
| Game Port Flood | Players can't connect | GateServer AntiDDoS settings |

## 📞 Emergency Recovery

If you've locked yourself out:
1. Go to OVH Dashboard
2. Use **KVM/VNC Console** to access server
3. Run: `.\scripts\08-emergency-lockdown.ps1 -Unlock`
4. Reconfigure your firewall rules

## 📊 Monitoring

Run diagnostic scripts to see attack patterns:
```powershell
# See failed login attempts by IP
.\scripts\07-audit-connections.ps1 -FailedLogins

# See current connections
.\scripts\07-audit-connections.ps1 -CurrentConnections

# Check for SYN flood
.\scripts\07-audit-connections.ps1 -SynFloodCheck
```

## 🔗 Related Resources

- [OVH Anti-DDoS Documentation](https://docs.ovh.com/gb/en/dedicated/anti-ddos/)
- [Microsoft RDP Security Best Practices](https://docs.microsoft.com/en-us/windows-server/remote/remote-desktop-services/rds-security)
- [Windows Firewall with Advanced Security](https://docs.microsoft.com/en-us/windows/security/threat-protection/windows-firewall/)
