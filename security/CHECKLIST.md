# 🔒 VPS Security Checklist

Use this checklist to ensure your VPS is properly secured. Complete items in order of priority.

---

## 🔴 Critical Priority (Do Immediately)

### RDP Security
- [ ] **Change RDP port from 3389 to a random high port (e.g., 41592)**
  - Script: `scripts/01-change-rdp-port.ps1`
  - Risk if not done: Constant brute-force attacks, server overload

- [ ] **Enable Network Level Authentication (NLA)**
  - Script: `scripts/02-enable-nla.ps1`
  - Risk if not done: Attackers can exhaust resources before authentication

- [ ] **Block default RDP port 3389 in Windows Firewall**
  - Done automatically by `01-change-rdp-port.ps1`

- [ ] **Configure OVH Network Firewall to block 3389**
  - Guide: `configs/ovh-firewall-rules.md`
  - Risk if not done: Attacks still reach your server, using bandwidth/CPU

### Account Security
- [ ] **Enable account lockout policy (3 attempts, 30 min lockout)**
  - Script: `scripts/06-account-lockout.ps1`
  - Prevents unlimited password guessing

- [ ] **Use strong passwords for all accounts**
  - At least 16 characters, mixed case, numbers, symbols
  - Consider using a password manager

- [ ] **Rename or disable the default Administrator account**
  ```powershell
  Rename-LocalUser -Name "Administrator" -NewName "MySecretAdmin"
  ```

---

## 🟠 High Priority (Do Within 24 Hours)

### Automated Protection
- [ ] **Set up RDP brute-force auto-ban script**
  - Script: `scripts/04-setup-auto-ban-task.ps1`
  - Automatically bans IPs with 5+ failed attempts

- [ ] **Verify auto-ban task is running**
  ```powershell
  Get-ScheduledTask -TaskName "RDP-BruteForce-Blocker"
  ```

### Network Firewall (OVH Dashboard)
- [ ] **Enable OVH Firewall on your IP**
- [ ] **Configure allow rules for:**
  - [ ] Your home/office IP → RDP custom port
  - [ ] 0.0.0.0/0 → Game server port (e.g., 7515)
  - [ ] 0.0.0.0/0 → Web ports 80/443 (if needed)
- [ ] **Configure deny rules for:**
  - [ ] 0.0.0.0/0 → Port 3389 (default RDP)
  - [ ] 0.0.0.0/0 → All other unused ports

### Game Server Protection
- [ ] **Enable GateServer AntiDDoS**
  - Config: `configs/gateserver-antiddos.cfg`
  - Copy settings to your `GateServer.cfg`

---

## 🟡 Medium Priority (Do Within 1 Week)

### IP Whitelisting
- [ ] **If you have a static IP, whitelist only that IP for RDP**
  - Script: `scripts/05-ip-whitelist.ps1`
  - Most secure option - blocks everyone except you

- [ ] **If dynamic IP, consider:**
  - [ ] Setting up a VPN with static IP
  - [ ] Using a jump box / bastion host
  - [ ] Using OVH's VPN service

### Monitoring & Logging
- [ ] **Run audit script to see attack patterns**
  - Script: `scripts/07-audit-connections.ps1`
  
- [ ] **Review Windows Security Event Log regularly**
  - Event ID 4625: Failed logins
  - Event ID 4624: Successful logins

- [ ] **Set up email alerts for critical events** (optional)

### Windows Updates
- [ ] **Enable automatic Windows Updates**
- [ ] **Verify latest security patches are installed**
  ```powershell
  Get-HotFix | Sort-Object InstalledOn -Descending | Select-Object -First 10
  ```

---

## 🟢 Low Priority (Do When Time Permits)

### Advanced Protection
- [ ] **Set up WireGuard VPN for secure RDP access**
  - Only allow RDP from VPN subnet
  - Strongest protection for remote access

- [ ] **Contact OVH for Game-specific Anti-DDoS**
  - They offer specialized protection for game servers

- [ ] **Enable Windows Defender Firewall logging**
  ```powershell
  Set-NetFirewallProfile -Profile Domain,Public,Private -LogBlocked True -LogAllowed False -LogFileName "%SystemRoot%\System32\LogFiles\Firewall\pfirewall.log"
  ```

### Backup & Recovery
- [ ] **Export current firewall rules**
  - Guide: `configs/windows-firewall-export.md`

- [ ] **Document your custom RDP port**
  - Store securely (password manager, encrypted file)

- [ ] **Test OVH KVM/VNC console access**
  - Verify you can access server if RDP fails

- [ ] **Create system restore point**
  ```powershell
  Checkpoint-Computer -Description "Pre-Security-Hardening"
  ```

---

## 📋 Verification Commands

After completing the checklist, verify your security:

```powershell
# 1. Check RDP port
Get-ItemProperty -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\Terminal Server\WinStations\RDP-Tcp' -Name 'PortNumber'

# 2. Check NLA status
Get-ItemProperty -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\Terminal Server\WinStations\RDP-Tcp' -Name 'UserAuthentication'

# 3. Check firewall rules for RDP
Get-NetFirewallRule -DisplayName "*RDP*" | Select-Object DisplayName, Enabled, Action

# 4. Check account lockout policy
net accounts

# 5. Check scheduled task is running
Get-ScheduledTask -TaskName "RDP-BruteForce-Blocker" | Select-Object TaskName, State

# 6. Check recent failed logins
Get-WinEvent -FilterHashtable @{LogName='Security';Id=4625} -MaxEvents 5 | Select-Object TimeCreated, Message
```

---

## 🚨 Signs You're Under Attack

Watch for these symptoms:

| Symptom | Likely Attack | Immediate Action |
|---------|--------------|------------------|
| Server unresponsive, OVH shows "active" | RDP flood | Change port, enable OVH firewall |
| Many Event 4625 in logs | Brute force | Run auto-ban script |
| High CPU from `svchost.exe` (TermService) | RDP saturation | Block 3389 at OVH level |
| Players can't connect, server OK | Game port flood | Enable GateServer AntiDDoS |
| High network I/O, low CPU | Bandwidth DDoS | Contact OVH support |

---

## ✅ Completion Status

| Category | Items | Completed | Date |
|----------|-------|-----------|------|
| Critical | 6 | [ ] / 6 | |
| High | 6 | [ ] / 6 | |
| Medium | 6 | [ ] / 6 | |
| Low | 6 | [ ] / 6 | |

**Last Review Date:** _______________

**Reviewed By:** _______________
