# PKO Server - Quick SYN Attack Mitigation
# Copy and paste these commands directly into PowerShell on your dedicated server

# ============================================
# STEP 1: IMMEDIATE - Block high-frequency IPs
# ============================================

# Check who's attacking (run this first to see the attackers)
Write-Host "=== Current connections to port 1973 ===" -ForegroundColor Yellow
netstat -an | findstr ":1973" | Group-Object { $_ -replace '.*\s+(\d+\.\d+\.\d+\.\d+):\d+\s+.*','$1' } | Sort-Object Count -Descending | Select-Object -First 20 | Format-Table Count, Name -AutoSize

# ============================================
# STEP 2: Block specific attacker IP (replace X.X.X.X)
# ============================================

# $attackerIP = "X.X.X.X"
# New-NetFirewallRule -DisplayName "Block Attacker $attackerIP" -Direction Inbound -Action Block -RemoteAddress $attackerIP -Protocol TCP -LocalPort 1973

# ============================================
# STEP 3: Rate limit using netsh (immediate effect)
# ============================================

netsh int tcp set global autotuninglevel=restricted
netsh int tcp set global chimney=disabled
netsh int tcp set global rss=disabled

# ============================================
# STEP 4: Quick registry hardening (no reboot needed for some)
# ============================================

$regPath = "HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters"
Set-ItemProperty -Path $regPath -Name "SynAttackProtect" -Value 1 -Type DWord -Force
Set-ItemProperty -Path $regPath -Name "TcpMaxConnectResponseRetransmissions" -Value 1 -Type DWord -Force

Write-Host ""
Write-Host "=== Quick mitigation applied ===" -ForegroundColor Green
Write-Host "For full protection, run HardenTcpStack.ps1 and reboot" -ForegroundColor Yellow
