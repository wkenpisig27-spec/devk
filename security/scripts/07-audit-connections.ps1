<#
.SYNOPSIS
    Diagnostic script to audit connections and detect attacks.

.DESCRIPTION
    Provides various views into current and recent connection activity:
    - Failed login attempts by IP
    - Current connections to specific ports
    - SYN flood detection
    - Banned IP list

.PARAMETER FailedLogins
    Show recent failed login attempts grouped by IP.

.PARAMETER CurrentConnections
    Show current network connections.

.PARAMETER SynFloodCheck
    Check for signs of SYN flood attack.

.PARAMETER BannedIPs
    Show currently banned IP addresses.

.PARAMETER Port
    Port to filter for CurrentConnections. Default: all ports.

.PARAMETER Hours
    Hours of history to check for FailedLogins. Default: 24.

.EXAMPLE
    .\07-audit-connections.ps1 -FailedLogins -Hours 1

.EXAMPLE
    .\07-audit-connections.ps1 -CurrentConnections -Port 3389

.EXAMPLE
    .\07-audit-connections.ps1 -SynFloodCheck
#>

param(
    [switch]$FailedLogins,
    [switch]$CurrentConnections,
    [switch]$SynFloodCheck,
    [switch]$BannedIPs,
    [int]$Port,
    [int]$Hours = 24
)

Write-Host "=== VPS Security Audit ===" -ForegroundColor Cyan
Write-Host "Time: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" -ForegroundColor White
Write-Host ""

# If no specific option, show a summary
if (-not $FailedLogins -and -not $CurrentConnections -and -not $SynFloodCheck -and -not $BannedIPs) {
    $FailedLogins = $true
    $CurrentConnections = $true
    $SynFloodCheck = $true
    $BannedIPs = $true
}

# Failed Logins
if ($FailedLogins) {
    Write-Host "=== Failed Login Attempts (Last $Hours hours) ===" -ForegroundColor Yellow
    Write-Host ""
    
    try {
        $events = Get-WinEvent -FilterHashtable @{
            LogName   = 'Security'
            Id        = 4625
            StartTime = (Get-Date).AddHours(-$Hours)
        } -ErrorAction SilentlyContinue

        if ($events -and $events.Count -gt 0) {
            $ipStats = @{}
            foreach ($event in $events) {
                try {
                    $xml = [xml]$event.ToXml()
                    $ip = ($xml.Event.EventData.Data | Where-Object { $_.Name -eq 'IpAddress' }).'#text'
                    $user = ($xml.Event.EventData.Data | Where-Object { $_.Name -eq 'TargetUserName' }).'#text'
                    
                    if ($ip -and $ip -ne '-' -and $ip -ne '::1') {
                        if (-not $ipStats.ContainsKey($ip)) {
                            $ipStats[$ip] = @{Count = 0; Users = @(); LastAttempt = $null }
                        }
                        $ipStats[$ip].Count++
                        if ($user -and $user -notin $ipStats[$ip].Users) {
                            $ipStats[$ip].Users += $user
                        }
                        if (-not $ipStats[$ip].LastAttempt -or $event.TimeCreated -gt $ipStats[$ip].LastAttempt) {
                            $ipStats[$ip].LastAttempt = $event.TimeCreated
                        }
                    }
                }
                catch { continue }
            }

            # Sort by count
            $sorted = $ipStats.GetEnumerator() | Sort-Object { $_.Value.Count } -Descending

            Write-Host "IP Address           Attempts  Last Attempt          Target Users" -ForegroundColor Cyan
            Write-Host "-------------------- --------- -------------------- --------------------------------" -ForegroundColor Cyan
            
            foreach ($item in $sorted) {
                $ip = $item.Key.PadRight(20)
                $count = $item.Value.Count.ToString().PadRight(9)
                $lastAttempt = $item.Value.LastAttempt.ToString("yyyy-MM-dd HH:mm:ss")
                $users = ($item.Value.Users -join ", ")
                if ($users.Length -gt 30) { $users = $users.Substring(0, 27) + "..." }
                
                $color = if ($item.Value.Count -ge 10) { "Red" } elseif ($item.Value.Count -ge 5) { "Yellow" } else { "White" }
                Write-Host "$ip $count $lastAttempt $users" -ForegroundColor $color
            }
            
            Write-Host ""
            Write-Host "Total: $($events.Count) failed attempts from $($ipStats.Count) unique IPs" -ForegroundColor White
        }
        else {
            Write-Host "No failed login attempts in the last $Hours hours." -ForegroundColor Green
        }
    }
    catch {
        Write-Host "Could not read security log: $($_.Exception.Message)" -ForegroundColor Red
    }
    Write-Host ""
}

# Current Connections
if ($CurrentConnections) {
    Write-Host "=== Current Network Connections ===" -ForegroundColor Yellow
    Write-Host ""
    
    $rdpPort = (Get-ItemProperty -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\Terminal Server\WinStations\RDP-Tcp' -Name 'PortNumber' -ErrorAction SilentlyContinue).PortNumber
    if (-not $rdpPort) { $rdpPort = 3389 }
    
    Write-Host "RDP Port: $rdpPort" -ForegroundColor White
    Write-Host ""
    
    $connections = Get-NetTCPConnection -State Established, SynReceived, TimeWait -ErrorAction SilentlyContinue
    
    if ($Port) {
        $connections = $connections | Where-Object { $_.LocalPort -eq $Port }
    }
    
    # Group by local port
    $portGroups = $connections | Group-Object LocalPort | Sort-Object Count -Descending | Select-Object -First 10
    
    Write-Host "Top Ports by Connection Count:" -ForegroundColor Cyan
    foreach ($group in $portGroups) {
        $portName = switch ($group.Name) {
            "3389" { "RDP (default)" }
            "$rdpPort" { "RDP (custom)" }
            "80" { "HTTP" }
            "443" { "HTTPS" }
            default { "" }
        }
        Write-Host "  Port $($group.Name): $($group.Count) connections $portName" -ForegroundColor White
    }
    
    Write-Host ""
    
    # Show RDP connections specifically
    $rdpConns = $connections | Where-Object { $_.LocalPort -eq $rdpPort -or $_.LocalPort -eq 3389 }
    if ($rdpConns) {
        Write-Host "RDP Connections:" -ForegroundColor Cyan
        $rdpConns | Select-Object RemoteAddress, RemotePort, State | Format-Table -AutoSize
    }
    Write-Host ""
}

# SYN Flood Check
if ($SynFloodCheck) {
    Write-Host "=== SYN Flood Check ===" -ForegroundColor Yellow
    Write-Host ""
    
    $synReceived = (Get-NetTCPConnection -State SynReceived -ErrorAction SilentlyContinue | Measure-Object).Count
    $timeWait = (Get-NetTCPConnection -State TimeWait -ErrorAction SilentlyContinue | Measure-Object).Count
    $established = (Get-NetTCPConnection -State Established -ErrorAction SilentlyContinue | Measure-Object).Count
    
    Write-Host "Connection States:" -ForegroundColor Cyan
    Write-Host "  SYN_RECEIVED: $synReceived" -ForegroundColor $(if ($synReceived -gt 50) { "Red" } elseif ($synReceived -gt 20) { "Yellow" } else { "Green" })
    Write-Host "  TIME_WAIT: $timeWait" -ForegroundColor $(if ($timeWait -gt 500) { "Red" } elseif ($timeWait -gt 200) { "Yellow" } else { "Green" })
    Write-Host "  ESTABLISHED: $established" -ForegroundColor White
    Write-Host ""
    
    if ($synReceived -gt 50) {
        Write-Host "WARNING: High SYN_RECEIVED count may indicate SYN flood attack!" -ForegroundColor Red
    }
    elseif ($synReceived -gt 20) {
        Write-Host "CAUTION: Elevated SYN_RECEIVED count. Monitor closely." -ForegroundColor Yellow
    }
    else {
        Write-Host "Connection states look normal." -ForegroundColor Green
    }
    Write-Host ""
}

# Banned IPs
if ($BannedIPs) {
    Write-Host "=== Currently Banned IPs ===" -ForegroundColor Yellow
    Write-Host ""
    
    $banRules = Get-NetFirewallRule -DisplayName "RDP-BAN-*" -ErrorAction SilentlyContinue
    
    if ($banRules -and $banRules.Count -gt 0) {
        Write-Host "IP Address           Banned At                Description" -ForegroundColor Cyan
        Write-Host "-------------------- ---------------------- -----------------------------" -ForegroundColor Cyan
        
        foreach ($rule in $banRules) {
            $ip = ($rule.DisplayName -replace 'RDP-BAN-', '').PadRight(20)
            $time = if ($rule.Description -match 'at (\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})') { $matches[1] } else { "Unknown" }
            $desc = $rule.Description
            if ($desc.Length -gt 40) { $desc = $desc.Substring(0, 37) + "..." }
            Write-Host "$ip $time $desc" -ForegroundColor White
        }
        
        Write-Host ""
        Write-Host "Total: $($banRules.Count) IPs currently banned" -ForegroundColor White
    }
    else {
        Write-Host "No IPs currently banned." -ForegroundColor Green
    }
    Write-Host ""
}

Write-Host "=== Audit Complete ===" -ForegroundColor Cyan
