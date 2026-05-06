<#
.SYNOPSIS
    Runs the full security setup sequence.

.DESCRIPTION
    Interactive script that guides the user through:
    1. Changing RDP Port
    2. Enabling NLA
    3. Configuring Account Lockout
    4. Setting up Auto-Ban Task

.NOTES
    Run as Administrator
#>

#Requires -RunAsAdministrator

$scriptDir = Split-Path $MyInvocation.MyCommand.Path
function Run-Script {
    param([string]$Name, [string]$Command)
    Write-Host "`n=== STEP: $Name ===" -ForegroundColor Cyan
    $choice = Read-Host "Do you want to run this step? (Y/n)"
    if ($choice -eq '' -or $choice -eq 'Y' -or $choice -eq 'y') {
        Invoke-Expression "& '$scriptDir\$Command'"
    }
    else {
        Write-Host "Skipping $Name..." -ForegroundColor Yellow
    }
}

Write-Host "=== Full Security Setup Wizard ===" -ForegroundColor Cyan
Write-Host "This will guide you through hardening your VPS."
Write-Host ""

# Step 1: Account Lockout (Safe, high impact)
Run-Script "Configure Account Lockout Policy" "06-account-lockout.ps1"

# Step 2: NLA (Safe, high impact)
Run-Script "Enable Network Level Authentication" "02-enable-nla.ps1"

# Step 3: Auto Ban (Safe, high impact)
Run-Script "Setup Auto-Ban for Brute Force" "04-setup-auto-ban-task.ps1"

# Step 4: Change Port (Riskiest, do last)
Write-Host "`n=== STEP: Change RDP Port ===" -ForegroundColor Cyan
Write-Host "CRITICAL: This will change how you connect to the server."
Write-Host "Default suggestion: 41592"
$doPort = Read-Host "Do you want to change the RDP port now? (Y/n)"
if ($doPort -eq '' -or $doPort -eq 'Y' -or $doPort -eq 'y') {
    $port = Read-Host "Enter new port number (default 41592)"
    if ($port -eq '') { $port = "41592" }
    
    Invoke-Expression "& '$scriptDir\01-change-rdp-port.ps1' -NewPort $port"
}

Write-Host "`n=== Setup Wizard Complete ===" -ForegroundColor Green
Write-Host "Don't forget to configure the OVH Firewall!" -ForegroundColor White
Write-Host "See configs\ovh-firewall-rules.md for details." -ForegroundColor White
