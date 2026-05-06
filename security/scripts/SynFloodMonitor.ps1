#Requires -RunAsAdministrator
<#
.SYNOPSIS
    SYN Flood Attack Monitor and Mitigator for PKO GateServer
.DESCRIPTION
    Monitors port 1973 for SYN flood attacks and automatically blocks attacking IPs
.NOTES
    Run as Administrator on your dedicated server
#>

param(
    [int]$Port = 1973,
    [int]$Threshold = 50,           # Connections per IP to trigger block
    [int]$MonitorInterval = 5,      # Seconds between checks
    [int]$BlockDuration = 3600,     # Seconds to keep IP blocked (1 hour)
    [switch]$DryRun                 # Don't actually block, just report
)

# Configuration
$LogFile = ".\LOG\SynFloodMonitor.log"
$BlockedIPsFile = ".\LOG\BlockedIPs.json"
$WhitelistIPs = @(
    "127.0.0.1",
    "::1"
    # Add your trusted IPs here
)

# Initialize
$BlockedIPs = @{}
if (Test-Path $BlockedIPsFile) {
    $BlockedIPs = Get-Content $BlockedIPsFile | ConvertFrom-Json -AsHashtable
}

function Write-Log {
    param([string]$Message, [string]$Level = "INFO")
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logEntry = "[$timestamp] [$Level] $Message"
    Write-Host $logEntry -ForegroundColor $(switch($Level) {
        "WARN" { "Yellow" }
        "ERROR" { "Red" }
        "ATTACK" { "Magenta" }
        "BLOCK" { "Red" }
        default { "White" }
    })
    Add-Content -Path $LogFile -Value $logEntry -ErrorAction SilentlyContinue
}

function Get-ConnectionsByIP {
    param([int]$Port)
    
    $connections = netstat -an | Select-String ":$Port\s" | ForEach-Object {
        if ($_ -match '(\d+\.\d+\.\d+\.\d+):.*\s+(\d+\.\d+\.\d+\.\d+):(\d+)\s+(\w+)') {
            [PSCustomObject]@{
                LocalIP = $Matches[1]
                RemoteIP = $Matches[2]
                RemotePort = $Matches[3]
                State = $Matches[4]
            }
        }
    }
    
    return $connections | Group-Object RemoteIP | Select-Object Name, Count, @{
        Name = 'States'
        Expression = { ($_.Group | Group-Object State | ForEach-Object { "$($_.Name):$($_.Count)" }) -join ", " }
    } | Sort-Object Count -Descending
}

function Block-AttackingIP {
    param([string]$IP, [string]$Reason)
    
    if ($WhitelistIPs -contains $IP) {
        Write-Log "Skipping whitelisted IP: $IP" "WARN"
        return
    }
    
    if ($BlockedIPs.ContainsKey($IP)) {
        Write-Log "IP already blocked: $IP" "INFO"
        return
    }
    
    $ruleName = "PKO-AutoBlock-$($IP -replace '\.', '-')"
    
    if ($DryRun) {
        Write-Log "[DRY RUN] Would block IP: $IP - Reason: $Reason" "BLOCK"
        return
    }
    
    try {
        # Create firewall rule to block
        New-NetFirewallRule -DisplayName $ruleName `
            -Direction Inbound `
            -Action Block `
            -RemoteAddress $IP `
            -Protocol TCP `
            -LocalPort $Port `
            -Profile Any `
            -ErrorAction Stop | Out-Null
        
        $BlockedIPs[$IP] = @{
            BlockedAt = (Get-Date).ToString("o")
            Reason = $Reason
            RuleName = $ruleName
            ExpiresAt = (Get-Date).AddSeconds($BlockDuration).ToString("o")
        }
        
        # Save blocked IPs
        $BlockedIPs | ConvertTo-Json | Set-Content $BlockedIPsFile
        
        Write-Log "BLOCKED IP: $IP - Reason: $Reason - Rule: $ruleName" "BLOCK"
    }
    catch {
        Write-Log "Failed to block IP $IP : $_" "ERROR"
    }
}

function Unblock-ExpiredIPs {
    $now = Get-Date
    $toRemove = @()
    
    foreach ($ip in $BlockedIPs.Keys) {
        $expiry = [DateTime]::Parse($BlockedIPs[$ip].ExpiresAt)
        if ($now -gt $expiry) {
            $ruleName = $BlockedIPs[$ip].RuleName
            
            if (-not $DryRun) {
                try {
                    Remove-NetFirewallRule -DisplayName $ruleName -ErrorAction SilentlyContinue
                    Write-Log "Unblocked expired IP: $ip" "INFO"
                }
                catch {
                    Write-Log "Failed to unblock IP $ip : $_" "ERROR"
                }
            }
            $toRemove += $ip
        }
    }
    
    foreach ($ip in $toRemove) {
        $BlockedIPs.Remove($ip)
    }
    
    if ($toRemove.Count -gt 0) {
        $BlockedIPs | ConvertTo-Json | Set-Content $BlockedIPsFile
    }
}

function Show-Stats {
    param($Connections)
    
    Clear-Host
    Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
    Write-Host "  PKO GateServer SYN Flood Monitor - Port $Port" -ForegroundColor Cyan
    Write-Host "  Threshold: $Threshold connections | Block Duration: $($BlockDuration/60) min" -ForegroundColor Gray
    Write-Host "  Blocked IPs: $($BlockedIPs.Count) | $(Get-Date -Format 'HH:mm:ss')" -ForegroundColor Gray
    Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
    Write-Host ""
    
    $totalConnections = ($Connections | Measure-Object -Property Count -Sum).Sum
    $synWait = $Connections | Where-Object { $_.States -match "SYN" }
    
    if ($totalConnections -gt 0) {
        Write-Host "  Total Connections: $totalConnections" -ForegroundColor White
        Write-Host "  Unique IPs: $($Connections.Count)" -ForegroundColor White
        Write-Host ""
        
        Write-Host "  Top Connections by IP:" -ForegroundColor Yellow
        Write-Host "  ─────────────────────────────────────────────────────────────" -ForegroundColor DarkGray
        
        $Connections | Select-Object -First 15 | ForEach-Object {
            $color = if ($_.Count -ge $Threshold) { "Red" } 
                     elseif ($_.Count -ge ($Threshold / 2)) { "Yellow" } 
                     else { "Green" }
            
            $blocked = if ($BlockedIPs.ContainsKey($_.Name)) { " [BLOCKED]" } else { "" }
            Write-Host ("  {0,-18} {1,5} connections  {2}" -f $_.Name, $_.Count, $_.States) -ForegroundColor $color
            if ($blocked) { Write-Host $blocked -ForegroundColor Red -NoNewline; Write-Host "" }
        }
    }
    else {
        Write-Host "  No active connections on port $Port" -ForegroundColor Green
    }
    
    Write-Host ""
    Write-Host "  Press Ctrl+C to stop monitoring" -ForegroundColor DarkGray
    Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
}

# Main monitoring loop
Write-Log "Starting SYN Flood Monitor on port $Port (Threshold: $Threshold)" "INFO"

try {
    while ($true) {
        # Get current connections
        $connections = Get-ConnectionsByIP -Port $Port
        
        # Check for attacks
        foreach ($conn in $connections) {
            if ($conn.Count -ge $Threshold) {
                Write-Log "ATTACK DETECTED from $($conn.Name): $($conn.Count) connections ($($conn.States))" "ATTACK"
                Block-AttackingIP -IP $conn.Name -Reason "SYN flood: $($conn.Count) connections"
            }
        }
        
        # Unblock expired IPs
        Unblock-ExpiredIPs
        
        # Display stats
        Show-Stats -Connections $connections
        
        Start-Sleep -Seconds $MonitorInterval
    }
}
catch {
    Write-Log "Monitor stopped: $_" "ERROR"
}
finally {
    Write-Log "SYN Flood Monitor stopped" "INFO"
}
