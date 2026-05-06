<#
.SYNOPSIS
    Configures RDP access to only allow specific whitelisted IP addresses.

.DESCRIPTION
    This script:
    1. Removes existing RDP-related firewall rules
    2. Creates allow rules only for specified IP addresses
    3. Blocks all other IPs from accessing RDP

.PARAMETER WhitelistIPs
    Array of IP addresses to whitelist. Can be single IPs or CIDR ranges.

.PARAMETER RDPPort
    The RDP port to configure. Default: 41592

.PARAMETER Remove
    If specified, removes the whitelist rules and restores default access.

.EXAMPLE
    .\05-ip-whitelist.ps1 -WhitelistIPs "123.45.67.89", "98.76.54.0/24"

.EXAMPLE
    .\05-ip-whitelist.ps1 -Remove

.NOTES
    Run as Administrator
    CAUTION: If you set up a whitelist without including your current IP, you will be locked out!
#>

param(
    [Parameter(Mandatory=$false)]
    [string[]]$WhitelistIPs,

    [int]$RDPPort = 41592,

    [switch]$Remove
)

#Requires -RunAsAdministrator

$ErrorActionPreference = "Stop"

Write-Host "=== RDP IP Whitelist Configuration ===" -ForegroundColor Cyan
Write-Host ""

if ($Remove) {
    Write-Host "Removing IP whitelist rules..." -ForegroundColor Yellow
    
    # Remove whitelist rules
    Get-NetFirewallRule -DisplayName "RDP-Whitelist-*" -ErrorAction SilentlyContinue | Remove-NetFirewallRule
    Get-NetFirewallRule -DisplayName "RDP-Block-All-Others" -ErrorAction SilentlyContinue | Remove-NetFirewallRule
    
    # Re-enable default RDP rule
    $portRuleName = "RDP Custom Port $RDPPort"
    $existingRule = Get-NetFirewallRule -DisplayName $portRuleName -ErrorAction SilentlyContinue
    if (-not $existingRule) {
        New-NetFirewallRule -DisplayName $portRuleName `
            -Direction Inbound `
            -LocalPort $RDPPort `
            -Protocol TCP `
            -Action Allow `
            -Profile Any | Out-Null
    }
    
    Write-Host "IP whitelist removed. RDP is now open to all IPs on port $RDPPort." -ForegroundColor Green
    exit 0
}

if (-not $WhitelistIPs -or $WhitelistIPs.Count -eq 0) {
    Write-Host "ERROR: You must specify at least one IP address to whitelist." -ForegroundColor Red
    Write-Host ""
    Write-Host "Usage: .\05-ip-whitelist.ps1 -WhitelistIPs `"YOUR.IP.HERE`"" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "To find your IP, run: (Invoke-WebRequest -Uri 'https://ifconfig.me/ip').Content" -ForegroundColor White
    exit 1
}

# Get current RDP port
$currentPort = (Get-ItemProperty -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\Terminal Server\WinStations\RDP-Tcp' -Name 'PortNumber').PortNumber
if ($currentPort -ne $RDPPort) {
    Write-Host "Note: Current RDP port is $currentPort, but configuring for port $RDPPort" -ForegroundColor Yellow
}

Write-Host "Whitelisted IPs:" -ForegroundColor White
foreach ($ip in $WhitelistIPs) {
    Write-Host "  - $ip" -ForegroundColor Green
}
Write-Host ""
Write-Host "RDP Port: $RDPPort" -ForegroundColor White
Write-Host ""

# Warning
Write-Host "WARNING: This will ONLY allow the IPs listed above to connect via RDP!" -ForegroundColor Red
Write-Host "If your current IP is not in the list, you WILL be locked out!" -ForegroundColor Red
Write-Host ""

# Try to get current external IP
try {
    $currentIP = (Invoke-WebRequest -Uri 'https://ifconfig.me/ip' -TimeoutSec 5).Content.Trim()
    Write-Host "Your current external IP appears to be: $currentIP" -ForegroundColor Yellow
    
    $isWhitelisted = $false
    foreach ($wip in $WhitelistIPs) {
        if ($wip -eq $currentIP -or $wip.Split('/')[0] -eq $currentIP) {
            $isWhitelisted = $true
            break
        }
    }
    
    if (-not $isWhitelisted) {
        Write-Host "DANGER: Your current IP ($currentIP) is NOT in the whitelist!" -ForegroundColor Red
        Write-Host "You will be LOCKED OUT if you proceed!" -ForegroundColor Red
        Write-Host ""
    }
} catch {
    Write-Host "Could not determine your current IP. Proceed with caution." -ForegroundColor Yellow
}

$confirm = Read-Host "Type 'YES' to apply the whitelist"
if ($confirm -ne "YES") {
    Write-Host "Aborted." -ForegroundColor Yellow
    exit 0
}

try {
    # Remove existing generic RDP allow rules
    Write-Host "`nRemoving existing RDP allow rules..." -ForegroundColor Cyan
    Get-NetFirewallRule -DisplayName "RDP Custom Port*" -ErrorAction SilentlyContinue | Remove-NetFirewallRule
    Get-NetFirewallRule -DisplayName "RDP-Whitelist-*" -ErrorAction SilentlyContinue | Remove-NetFirewallRule
    Get-NetFirewallRule -DisplayName "RDP-Block-All-Others" -ErrorAction SilentlyContinue | Remove-NetFirewallRule

    # Create allow rules for each whitelisted IP
    Write-Host "Creating whitelist rules..." -ForegroundColor Cyan
    $index = 1
    foreach ($ip in $WhitelistIPs) {
        $ruleName = "RDP-Whitelist-$index-$($ip -replace '[/:]', '-')"
        New-NetFirewallRule -DisplayName $ruleName `
            -Direction Inbound `
            -LocalPort $RDPPort `
            -Protocol TCP `
            -RemoteAddress $ip `
            -Action Allow `
            -Profile Any `
            -Description "Whitelisted RDP access for $ip" | Out-Null
        Write-Host "  Created rule for $ip" -ForegroundColor Green
        $index++
    }

    # Create block rule for everyone else (lower priority)
    Write-Host "Creating block rule for other IPs..." -ForegroundColor Cyan
    New-NetFirewallRule -DisplayName "RDP-Block-All-Others" `
        -Direction Inbound `
        -LocalPort $RDPPort `
        -Protocol TCP `
        -Action Block `
        -Profile Any `
        -Description "Block all non-whitelisted RDP access" | Out-Null

    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "  IP WHITELIST CONFIGURED!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Only these IPs can now connect via RDP:" -ForegroundColor White
    foreach ($ip in $WhitelistIPs) {
        Write-Host "  - $ip" -ForegroundColor Green
    }
    Write-Host ""
    Write-Host "To remove the whitelist later:" -ForegroundColor Yellow
    Write-Host "  .\05-ip-whitelist.ps1 -Remove" -ForegroundColor White
    Write-Host ""

} catch {
    Write-Host ""
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
