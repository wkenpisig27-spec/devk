<#
.SYNOPSIS
    Automatically bans IPs with repeated failed RDP login attempts.

.DESCRIPTION
    This script:
    1. Scans Windows Security Event Log for Event ID 4625 (Failed Logon)
    2. Counts attempts per IP address in the last N minutes
    3. Creates Windows Firewall rules to block offending IPs
    4. Optionally removes old ban rules after expiry

.PARAMETER Threshold
    Number of failed attempts before banning. Default: 5

.PARAMETER TimeWindowMinutes
    Time window to count attempts. Default: 10

.PARAMETER BanDurationMinutes
    How long to ban IPs (0 = permanent). Default: 60

.PARAMETER LogFile
    Path to log file. Default: C:\logs\rdp-bans.log

.NOTES
    Run as Administrator
    Should be scheduled to run every 5 minutes
#>

param(
    [int]$Threshold = 5,
    [int]$TimeWindowMinutes = 10,
    [int]$BanDurationMinutes = 60,
    [string]$LogFile = "C:\logs\rdp-bans.log"
)

#Requires -RunAsAdministrator

# Ensure log directory exists
$logDir = Split-Path $LogFile -Parent
if (-not (Test-Path $logDir)) {
    New-Item -ItemType Directory -Path $logDir -Force | Out-Null
}

function Write-Log {
    param([string]$Message)
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logMessage = "[$timestamp] $Message"
    Add-Content -Path $LogFile -Value $logMessage
    Write-Output $logMessage
}

# Get failed logon events
try {
    $events = Get-WinEvent -FilterHashtable @{
        LogName = 'Security'
        Id = 4625
        StartTime = (Get-Date).AddMinutes(-$TimeWindowMinutes)
    } -ErrorAction SilentlyContinue
} catch {
    # No events found, exit silently
    exit 0
}

if (-not $events -or $events.Count -eq 0) {
    exit 0
}

# Count attempts per IP
$ipCounts = @{}
foreach ($event in $events) {
    try {
        $xml = [xml]$event.ToXml()
        $ipData = $xml.Event.EventData.Data | Where-Object { $_.Name -eq 'IpAddress' }
        $ip = $ipData.'#text'
        
        # Skip local and empty IPs
        if ($ip -and $ip -ne '-' -and $ip -ne '::1' -and $ip -ne '127.0.0.1' -and $ip -ne '') {
            if ($ipCounts.ContainsKey($ip)) {
                $ipCounts[$ip]++
            } else {
                $ipCounts[$ip] = 1
            }
        }
    } catch {
        continue
    }
}

# Ban IPs exceeding threshold
$bannedCount = 0
foreach ($ip in $ipCounts.Keys) {
    if ($ipCounts[$ip] -ge $Threshold) {
        $ruleName = "RDP-BAN-$ip"
        
        # Check if already banned
        $existing = Get-NetFirewallRule -DisplayName $ruleName -ErrorAction SilentlyContinue
        if (-not $existing) {
            try {
                New-NetFirewallRule -DisplayName $ruleName `
                    -Direction Inbound `
                    -RemoteAddress $ip `
                    -Action Block `
                    -Profile Any `
                    -Description "Auto-banned for $($ipCounts[$ip]) failed RDP attempts at $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" | Out-Null
                
                Write-Log "BANNED: $ip (${($ipCounts[$ip])} failed attempts)"
                $bannedCount++
            } catch {
                Write-Log "ERROR: Failed to ban $ip - $($_.Exception.Message)"
            }
        }
    }
}

# Clean up expired bans
if ($BanDurationMinutes -gt 0) {
    $banRules = Get-NetFirewallRule -DisplayName "RDP-BAN-*" -ErrorAction SilentlyContinue
    $expiryCutoff = (Get-Date).AddMinutes(-$BanDurationMinutes)
    
    foreach ($rule in $banRules) {
        # Parse datetime from description
        if ($rule.Description -match 'at (\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})') {
            $banTime = [datetime]::ParseExact($matches[1], 'yyyy-MM-dd HH:mm:ss', $null)
            if ($banTime -lt $expiryCutoff) {
                try {
                    Remove-NetFirewallRule -DisplayName $rule.DisplayName
                    $ip = $rule.DisplayName -replace 'RDP-BAN-', ''
                    Write-Log "UNBANNED: $ip (ban expired)"
                } catch {
                    Write-Log "ERROR: Failed to unban $($rule.DisplayName) - $($_.Exception.Message)"
                }
            }
        }
    }
}

if ($bannedCount -gt 0) {
    Write-Log "Session complete: $bannedCount new IPs banned"
}
