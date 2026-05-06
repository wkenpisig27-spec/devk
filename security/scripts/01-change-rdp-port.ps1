<#
.SYNOPSIS
    Changes the Windows RDP port from default 3389 to a custom port.

.DESCRIPTION
    This script:
    1. Changes the RDP listening port in the registry
    2. Creates a firewall rule for the new port
    3. Optionally blocks the old port 3389
    4. Restarts the RDP service

.PARAMETER NewPort
    The new port number for RDP. Should be between 1024-65535.
    Default: 41592

.PARAMETER BlockOldPort
    If specified, creates a firewall rule to block port 3389.
    Default: $true

.EXAMPLE
    .\01-change-rdp-port.ps1 -NewPort 41592

.NOTES
    Run as Administrator
    Keep your current RDP session open until you verify the new port works!
    Connect using: mstsc /v:your-server-ip:41592
#>

param(
    [Parameter(Mandatory=$false)]
    [ValidateRange(1024, 65535)]
    [int]$NewPort = 41592,

    [Parameter(Mandatory=$false)]
    [bool]$BlockOldPort = $true
)

# Requires elevation
#Requires -RunAsAdministrator

$ErrorActionPreference = "Stop"

Write-Host "=== RDP Port Change Script ===" -ForegroundColor Cyan
Write-Host ""

# Get current port
$currentPort = (Get-ItemProperty -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\Terminal Server\WinStations\RDP-Tcp' -Name 'PortNumber').PortNumber
Write-Host "Current RDP Port: $currentPort" -ForegroundColor Yellow
Write-Host "New RDP Port: $NewPort" -ForegroundColor Green
Write-Host ""

# Confirmation
Write-Host "WARNING: This will change your RDP port!" -ForegroundColor Red
Write-Host "Make sure you:"
Write-Host "  1. Keep this RDP session open until you verify the new port works"
Write-Host "  2. Note down the new port: $NewPort"
Write-Host "  3. Have OVH console access ready in case of issues"
Write-Host ""
$confirm = Read-Host "Type 'YES' to continue"

if ($confirm -ne "YES") {
    Write-Host "Aborted." -ForegroundColor Yellow
    exit 0
}

try {
    # Step 1: Change registry
    Write-Host "`n[1/5] Changing RDP port in registry..." -ForegroundColor Cyan
    Set-ItemProperty -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\Terminal Server\WinStations\RDP-Tcp' -Name 'PortNumber' -Value $NewPort
    Write-Host "  Registry updated successfully" -ForegroundColor Green

    # Step 2: Create firewall rule for new port
    Write-Host "`n[2/5] Creating firewall rule for new port..." -ForegroundColor Cyan
    $ruleName = "RDP Custom Port $NewPort"
    
    # Remove existing rule if it exists
    $existingRule = Get-NetFirewallRule -DisplayName $ruleName -ErrorAction SilentlyContinue
    if ($existingRule) {
        Remove-NetFirewallRule -DisplayName $ruleName
    }
    
    New-NetFirewallRule -DisplayName $ruleName `
        -Direction Inbound `
        -LocalPort $NewPort `
        -Protocol TCP `
        -Action Allow `
        -Profile Any `
        -Description "RDP access on custom port (created by security script)" | Out-Null
    Write-Host "  Firewall rule created for port $NewPort" -ForegroundColor Green

    # Step 3: Block old port if requested
    if ($BlockOldPort -and $currentPort -eq 3389) {
        Write-Host "`n[3/5] Blocking default RDP port 3389..." -ForegroundColor Cyan
        $blockRuleName = "Block RDP Port 3389"
        
        $existingBlock = Get-NetFirewallRule -DisplayName $blockRuleName -ErrorAction SilentlyContinue
        if ($existingBlock) {
            Remove-NetFirewallRule -DisplayName $blockRuleName
        }
        
        New-NetFirewallRule -DisplayName $blockRuleName `
            -Direction Inbound `
            -LocalPort 3389 `
            -Protocol TCP `
            -Action Block `
            -Profile Any `
            -Description "Block default RDP port to prevent attacks" | Out-Null
        Write-Host "  Port 3389 blocked" -ForegroundColor Green
    } else {
        Write-Host "`n[3/5] Skipping port 3389 block (already using custom port or disabled)" -ForegroundColor Yellow
    }

    # Step 4: Restart RDP service
    Write-Host "`n[4/5] Restarting Remote Desktop Service..." -ForegroundColor Cyan
    Restart-Service -Name TermService -Force
    Start-Sleep -Seconds 3
    Write-Host "  Service restarted" -ForegroundColor Green

    # Step 5: Verify
    Write-Host "`n[5/5] Verifying changes..." -ForegroundColor Cyan
    $newPortCheck = (Get-ItemProperty -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\Terminal Server\WinStations\RDP-Tcp' -Name 'PortNumber').PortNumber
    
    if ($newPortCheck -eq $NewPort) {
        Write-Host "  Port successfully changed to $NewPort" -ForegroundColor Green
    } else {
        Write-Host "  Warning: Port may not have changed correctly" -ForegroundColor Red
    }

    # Final instructions
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "  RDP PORT CHANGE COMPLETE!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "IMPORTANT:" -ForegroundColor Yellow
    Write-Host "  1. Open a NEW RDP connection to test: mstsc /v:YOUR-IP:$NewPort" -ForegroundColor White
    Write-Host "  2. Do NOT close this session until the new connection works" -ForegroundColor White
    Write-Host "  3. Update OVH Firewall to allow port $NewPort" -ForegroundColor White
    Write-Host ""
    Write-Host "If you get locked out:" -ForegroundColor Yellow
    Write-Host "  - Use OVH KVM/VNC console" -ForegroundColor White
    Write-Host "  - Run: netsh advfirewall set allprofiles state off" -ForegroundColor White
    Write-Host "  - Then reconfigure firewall" -ForegroundColor White
    Write-Host ""

} catch {
    Write-Host ""
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "No changes were made or changes may be incomplete." -ForegroundColor Red
    Write-Host ""
    exit 1
}
