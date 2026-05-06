<#
.SYNOPSIS
    Enables Network Level Authentication (NLA) for RDP.

.DESCRIPTION
    NLA requires users to authenticate before a full RDP session is established.
    This prevents attackers from exhausting server resources with connection attempts.

.NOTES
    Run as Administrator
#>

#Requires -RunAsAdministrator

$ErrorActionPreference = "Stop"

Write-Host "=== Enable Network Level Authentication ===" -ForegroundColor Cyan
Write-Host ""

# Check current status
$currentNLA = (Get-ItemProperty -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\Terminal Server\WinStations\RDP-Tcp' -Name 'UserAuthentication').UserAuthentication

if ($currentNLA -eq 1) {
    Write-Host "NLA is already enabled." -ForegroundColor Green
    exit 0
}

Write-Host "Current NLA Status: Disabled" -ForegroundColor Yellow
Write-Host ""

try {
    # Enable NLA
    Write-Host "Enabling Network Level Authentication..." -ForegroundColor Cyan
    Set-ItemProperty -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\Terminal Server\WinStations\RDP-Tcp' -Name 'UserAuthentication' -Value 1
    
    # Also set via Group Policy registry key
    Set-ItemProperty -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\Terminal Server\WinStations\RDP-Tcp' -Name 'SecurityLayer' -Value 2
    
    # Verify
    $newNLA = (Get-ItemProperty -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\Terminal Server\WinStations\RDP-Tcp' -Name 'UserAuthentication').UserAuthentication
    
    if ($newNLA -eq 1) {
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Green
        Write-Host "  NLA ENABLED SUCCESSFULLY!" -ForegroundColor Green
        Write-Host "========================================" -ForegroundColor Green
        Write-Host ""
        Write-Host "Benefits:" -ForegroundColor Yellow
        Write-Host "  - Users must authenticate before session starts" -ForegroundColor White
        Write-Host "  - Prevents resource exhaustion attacks" -ForegroundColor White
        Write-Host "  - Protects against pre-authentication vulnerabilities" -ForegroundColor White
        Write-Host ""
    } else {
        Write-Host "Warning: NLA may not have been enabled correctly" -ForegroundColor Red
    }

} catch {
    Write-Host ""
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
