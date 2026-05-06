#Requires -RunAsAdministrator
<#
.SYNOPSIS
    Apply Windows TCP/IP hardening against SYN flood attacks
.DESCRIPTION
    Configures registry settings and netsh to protect against SYN floods
.NOTES
    Run ONCE as Administrator, then REBOOT the server
#>

Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "  PKO Server - SYN Flood Protection Hardening Script" -ForegroundColor Cyan
Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""

$registryPath = "HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters"

# Ensure registry path exists
if (-not (Test-Path $registryPath)) {
    New-Item -Path $registryPath -Force | Out-Null
}

Write-Host "[1/7] Enabling SYN Attack Protection..." -ForegroundColor Yellow
Set-ItemProperty -Path $registryPath -Name "SynAttackProtect" -Value 1 -Type DWord -Force
Write-Host "      SynAttackProtect = 1 (Enabled)" -ForegroundColor Green

Write-Host "[2/7] Reducing SYN-ACK Retransmissions..." -ForegroundColor Yellow
Set-ItemProperty -Path $registryPath -Name "TcpMaxConnectResponseRetransmissions" -Value 1 -Type DWord -Force
Write-Host "      TcpMaxConnectResponseRetransmissions = 1 (was 2)" -ForegroundColor Green

Write-Host "[3/7] Limiting Half-Open Connections..." -ForegroundColor Yellow
Set-ItemProperty -Path $registryPath -Name "TcpMaxHalfOpen" -Value 100 -Type DWord -Force
Set-ItemProperty -Path $registryPath -Name "TcpMaxHalfOpenRetried" -Value 80 -Type DWord -Force
Write-Host "      TcpMaxHalfOpen = 100" -ForegroundColor Green
Write-Host "      TcpMaxHalfOpenRetried = 80" -ForegroundColor Green

Write-Host "[4/7] Enabling TCP Timestamps (RFC 1323)..." -ForegroundColor Yellow
Set-ItemProperty -Path $registryPath -Name "Tcp1323Opts" -Value 1 -Type DWord -Force
Write-Host "      Tcp1323Opts = 1 (Enabled)" -ForegroundColor Green

Write-Host "[5/7] Enabling Dead Gateway Detection..." -ForegroundColor Yellow
Set-ItemProperty -Path $registryPath -Name "EnableDeadGWDetect" -Value 1 -Type DWord -Force
Write-Host "      EnableDeadGWDetect = 1" -ForegroundColor Green

Write-Host "[6/7] Configuring TCP Auto-Tuning..." -ForegroundColor Yellow
netsh int tcp set global autotuninglevel=restricted 2>$null
netsh int tcp set global congestionprovider=ctcp 2>$null
Write-Host "      Auto-tuning = Restricted" -ForegroundColor Green
Write-Host "      Congestion Provider = CTCP" -ForegroundColor Green

Write-Host "[7/7] Enabling Windows Firewall Stealth Mode..." -ForegroundColor Yellow
netsh advfirewall set allprofiles firewallpolicy blockinbound,allowoutbound 2>$null
Write-Host "      Default inbound = Block" -ForegroundColor Green

Write-Host ""
Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "  HARDENING COMPLETE - REBOOT REQUIRED" -ForegroundColor Green
Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""
Write-Host "Additional recommendations:" -ForegroundColor Yellow
Write-Host "  1. Contact OVH to enable Game DDoS Protection" -ForegroundColor White
Write-Host "  2. Run SynFloodMonitor.ps1 to auto-block attackers" -ForegroundColor White
Write-Host "  3. Consider rate-limiting in GateServer code" -ForegroundColor White
Write-Host ""

$reboot = Read-Host "Reboot now? (y/n)"
if ($reboot -eq 'y') {
    Restart-Computer -Force
}
