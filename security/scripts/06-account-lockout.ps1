<#
.SYNOPSIS
    Configures Windows account lockout policy to prevent brute-force attacks.

.DESCRIPTION
    Sets up:
    - Account lockout threshold (failed attempts before lockout)
    - Lockout duration (how long the account remains locked)
    - Lockout observation window (time window for counting attempts)

.PARAMETER Threshold
    Number of failed attempts before locking. Default: 3

.PARAMETER DurationMinutes
    How long to lock the account. Default: 30

.PARAMETER WindowMinutes
    Time window for counting attempts. Default: 30

.NOTES
    Run as Administrator
#>

param(
    [int]$Threshold = 3,
    [int]$DurationMinutes = 30,
    [int]$WindowMinutes = 30
)

#Requires -RunAsAdministrator

Write-Host "=== Configure Account Lockout Policy ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Settings:" -ForegroundColor White
Write-Host "  Lockout Threshold: $Threshold failed attempts" -ForegroundColor White
Write-Host "  Lockout Duration: $DurationMinutes minutes" -ForegroundColor White
Write-Host "  Observation Window: $WindowMinutes minutes" -ForegroundColor White
Write-Host ""

try {
    # Apply settings using net accounts
    $output = & net accounts /lockoutthreshold:$Threshold /lockoutduration:$DurationMinutes /lockoutwindow:$WindowMinutes 2>&1
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "========================================" -ForegroundColor Green
        Write-Host "  ACCOUNT LOCKOUT POLICY CONFIGURED!" -ForegroundColor Green
        Write-Host "========================================" -ForegroundColor Green
        Write-Host ""
        
        # Display current settings
        Write-Host "Current Policy:" -ForegroundColor Yellow
        & net accounts | Select-String -Pattern "Lockout"
        
        Write-Host ""
        Write-Host "How it works:" -ForegroundColor Yellow
        Write-Host "  - After $Threshold failed login attempts within $WindowMinutes minutes," -ForegroundColor White
        Write-Host "    the account will be locked for $DurationMinutes minutes." -ForegroundColor White
        Write-Host ""
    }
    else {
        Write-Host "ERROR: Failed to set account policy" -ForegroundColor Red
        Write-Host $output -ForegroundColor Red
    }

}
catch {
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
