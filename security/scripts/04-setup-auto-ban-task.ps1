<#
.SYNOPSIS
    Sets up a scheduled task to run the RDP brute-force blocker every 5 minutes.

.DESCRIPTION
    Creates a Windows Scheduled Task that runs the 03-rdp-bruteforce-blocker.ps1
    script automatically every 5 minutes.

.PARAMETER ScriptPath
    Path to the blocker script. Default: Same directory as this script.

.PARAMETER IntervalMinutes
    How often to run the task. Default: 5

.NOTES
    Run as Administrator
#>

param(
    [string]$ScriptPath,
    [int]$IntervalMinutes = 5
)

#Requires -RunAsAdministrator

$ErrorActionPreference = "Stop"

# Determine script path
if (-not $ScriptPath) {
    $ScriptPath = Join-Path (Split-Path $MyInvocation.MyCommand.Path -Parent) "03-rdp-bruteforce-blocker.ps1"
}

if (-not (Test-Path $ScriptPath)) {
    Write-Host "ERROR: Blocker script not found at: $ScriptPath" -ForegroundColor Red
    exit 1
}

$taskName = "RDP-BruteForce-Blocker"

Write-Host "=== Setup Auto-Ban Scheduled Task ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Script Path: $ScriptPath" -ForegroundColor White
Write-Host "Interval: Every $IntervalMinutes minutes" -ForegroundColor White
Write-Host ""

try {
    # Remove existing task if present
    $existingTask = Get-ScheduledTask -TaskName $taskName -ErrorAction SilentlyContinue
    if ($existingTask) {
        Write-Host "Removing existing scheduled task..." -ForegroundColor Yellow
        Unregister-ScheduledTask -TaskName $taskName -Confirm:$false
    }

    # Create action
    $action = New-ScheduledTaskAction -Execute "PowerShell.exe" `
        -Argument "-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File `"$ScriptPath`""

    # Create trigger (every N minutes, indefinitely)
    $trigger = New-ScheduledTaskTrigger -Once -At (Get-Date) `
        -RepetitionInterval (New-TimeSpan -Minutes $IntervalMinutes) `
        -RepetitionDuration ([TimeSpan]::MaxValue)

    # Create principal (run as SYSTEM with highest privileges)
    $principal = New-ScheduledTaskPrincipal -UserId "SYSTEM" `
        -LogonType ServiceAccount `
        -RunLevel Highest

    # Create settings
    $settings = New-ScheduledTaskSettingsSet `
        -AllowStartIfOnBatteries `
        -DontStopIfGoingOnBatteries `
        -StartWhenAvailable `
        -MultipleInstances IgnoreNew `
        -ExecutionTimeLimit (New-TimeSpan -Minutes 5)

    # Register task
    Register-ScheduledTask -TaskName $taskName `
        -Action $action `
        -Trigger $trigger `
        -Principal $principal `
        -Settings $settings `
        -Description "Automatically bans IPs with repeated failed RDP login attempts" | Out-Null

    # Start the task immediately
    Start-ScheduledTask -TaskName $taskName

    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "  SCHEDULED TASK CREATED!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Task Name: $taskName" -ForegroundColor White
    Write-Host "Runs: Every $IntervalMinutes minutes" -ForegroundColor White
    Write-Host "Log File: C:\logs\rdp-bans.log" -ForegroundColor White
    Write-Host ""
    Write-Host "Management Commands:" -ForegroundColor Yellow
    Write-Host "  View status:  Get-ScheduledTask -TaskName '$taskName'" -ForegroundColor White
    Write-Host "  Run now:      Start-ScheduledTask -TaskName '$taskName'" -ForegroundColor White
    Write-Host "  Stop:         Stop-ScheduledTask -TaskName '$taskName'" -ForegroundColor White
    Write-Host "  Disable:      Disable-ScheduledTask -TaskName '$taskName'" -ForegroundColor White
    Write-Host "  Remove:       Unregister-ScheduledTask -TaskName '$taskName'" -ForegroundColor White
    Write-Host ""

} catch {
    Write-Host ""
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
