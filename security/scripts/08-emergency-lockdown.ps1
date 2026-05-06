<#
.SYNOPSIS
    Emergency script to lock down RDP or unlock it if stuck.

.DESCRIPTION
    Two modes:
    1. LOCKDOWN: Blocks ALL RDP connections immediately. Use when under severe attack.
    2. UNLOCK: Resets firewall to allow RDP. Use via OVH Console if locked out.

.PARAMETER Unlock
    If specified, unlocks RDP instead of locking it.

.PARAMETER RDPPort
    Port to unlock. Default: 41592.

.NOTES
    Run as Administrator
    LOCKDOWN mode will disconnect you!
#>

param(
    [switch]$Unlock,
    [int]$RDPPort = 41592
)

#Requires -RunAsAdministrator

$ErrorActionPreference = "Stop"

if ($Unlock) {
    Write-Host "=== EMERGENCY UNLOCK ===" -ForegroundColor Green
    Write-Host "Attempting to restore RDP access on port $RDPPort..."
    
    try {
        # Validating input
        if ($RDPPort -lt 1024 -or $RDPPort -gt 65535) {
            # Fallback to check registry if user didn't specify
            $regPort = (Get-ItemProperty -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\Terminal Server\WinStations\RDP-Tcp' -Name 'PortNumber' -ErrorAction SilentlyContinue).PortNumber
            if ($regPort) { 
                $RDPPort = $regPort
                Write-Host "Detected RDP port from registry: $RDPPort" -ForegroundColor Yellow
            }
            else {
                $RDPPort = 3389
                Write-Host "Defaulting to 3389" -ForegroundColor Yellow
            }
        }
        
        # Remove any blocking rules for RDP
        Get-NetFirewallRule -DisplayName "RDP-Block-All-Others" -ErrorAction SilentlyContinue | Remove-NetFirewallRule
        Get-NetFirewallRule -DisplayName "Block RDP Port 3389" -ErrorAction SilentlyContinue | Remove-NetFirewallRule
        
        # Ensure allow rule exists
        $ruleName = "Emergency Access $RDPPort"
        New-NetFirewallRule -DisplayName $ruleName `
            -Direction Inbound `
            -LocalPort $RDPPort `
            -Protocol TCP `
            -Action Allow `
            -Profile Any `
            -Description "Created by Emergency Unlock Script" -ErrorAction SilentlyContinue | Out-Null
            
        Write-Host "Success! You should now be able to connect on port $RDPPort" -ForegroundColor Green
    }
    catch {
        Write-Host "Error during unlock: $($_.Exception.Message)" -ForegroundColor Red
        Write-Host "Try running: Set-NetFirewallProfile -Profile Domain,Public,Private -Enabled False" -ForegroundColor Yellow
    }
}
else {
    Write-Host "=== EMERGENCY LOCKDOWN ===" -ForegroundColor Red
    Write-Host "WARNING: This will block ALL RDP connections immediately!"
    Write-Host "You will be disconnected."
    Write-Host "To recover, you must use the OVH KVM/VNC Console."
    Write-Host ""
    $confirm = Read-Host "Type 'LOCKDOWN' to confirm"
    
    if ($confirm -eq "LOCKDOWN") {
        try {
            # Create high priority block rule
            New-NetFirewallRule -DisplayName "EMERGENCY RDP LOCKDOWN" `
                -Direction Inbound `
                -LocalPort @(3389, $RDPPort) `
                -Protocol TCP `
                -Action Block `
                -Profile Any `
                -Description "Emergency Lockdown Active" | Out-Null
                
            Write-Host "Server is now locked down." -ForegroundColor Red
        }
        catch {
            Write-Host "Failed to apply lockdown: $($_.Exception.Message)" -ForegroundColor Red
        }
    }
    else {
        Write-Host "Aborted." -ForegroundColor Yellow
    }
}
