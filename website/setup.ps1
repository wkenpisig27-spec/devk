# =============================================================================
# PKO Website - Quick Setup Script
# =============================================================================
# Run this script as Administrator to configure nginx and create directories

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " PKO Website - Setup Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if running as administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "[ERROR] This script must be run as Administrator" -ForegroundColor Red
    Write-Host "Right-click PowerShell and select 'Run as Administrator'" -ForegroundColor Yellow
    Write-Host ""
    pause
    exit 1
}

Write-Host "[INFO] Running with Administrator privileges" -ForegroundColor Green
Write-Host ""

# =============================================================================
# Step 1: Check Prerequisites
# =============================================================================
Write-Host "Step 1: Checking Prerequisites..." -ForegroundColor Cyan
Write-Host ""

# Check nginx
if (Test-Path "C:\nginx\nginx.exe") {
    Write-Host "[OK] nginx found at C:\nginx\" -ForegroundColor Green
} else {
    Write-Host "[ERROR] nginx not found at C:\nginx\" -ForegroundColor Red
    Write-Host "Please install nginx first" -ForegroundColor Yellow
    pause
    exit 1
}

# Check PHP
$phpPath = Get-Command php -ErrorAction SilentlyContinue
if ($phpPath) {
    $phpVersion = php --version | Select-String -Pattern "PHP (\d+\.\d+)" | ForEach-Object { $_.Matches.Groups[1].Value }
    Write-Host "[OK] PHP $phpVersion found" -ForegroundColor Green
} else {
    Write-Host "[WARNING] PHP not found in PATH" -ForegroundColor Yellow
    Write-Host "You need to install PHP 8.0+ - see SETUP_LOCAL.md" -ForegroundColor Yellow
}

Write-Host ""

# =============================================================================
# Step 2: Create nginx sites directory
# =============================================================================
Write-Host "Step 2: Creating nginx sites directory..." -ForegroundColor Cyan
Write-Host ""

$sitesDir = "C:\nginx\conf\sites"
if (-not (Test-Path $sitesDir)) {
    New-Item -Path $sitesDir -ItemType Directory -Force | Out-Null
    Write-Host "[OK] Created $sitesDir" -ForegroundColor Green
} else {
    Write-Host "[INFO] Sites directory already exists" -ForegroundColor Yellow
}

Write-Host ""

# =============================================================================
# Step 3: Update nginx.conf to include sites
# =============================================================================
Write-Host "Step 3: Updating nginx.conf..." -ForegroundColor Cyan
Write-Host ""

$nginxConf = "C:\nginx\conf\nginx.conf"
$nginxContent = Get-Content $nginxConf -Raw

# Check if sites include already exists
if ($nginxContent -match "include\s+sites/\*\.conf") {
    Write-Host "[INFO] nginx.conf already includes sites/*.conf" -ForegroundColor Yellow
} else {
    # Backup original
    Copy-Item $nginxConf "$nginxConf.backup" -Force
    Write-Host "[INFO] Created backup: nginx.conf.backup" -ForegroundColor Cyan
    
    # Add include before the last closing brace of http block
    $nginxContent = $nginxContent -replace '(\s*)(}\s*#\s*End of http block\s*$|}\s*$)', "`n    # Include site configurations`n    include sites/*.conf;`n`$1`$2"
    
    Set-Content -Path $nginxConf -Value $nginxContent -NoNewline
    Write-Host "[OK] Updated nginx.conf to include sites/*.conf" -ForegroundColor Green
}

Write-Host ""

# =============================================================================
# Step 4: Verify site configuration
# =============================================================================
Write-Host "Step 4: Verifying site configuration..." -ForegroundColor Cyan
Write-Host ""

$siteConf = "C:\nginx\conf\sites\pkodev.conf"
if (Test-Path $siteConf) {
    Write-Host "[OK] Site configuration exists: pkodev.conf" -ForegroundColor Green
} else {
    Write-Host "[ERROR] Site configuration not found!" -ForegroundColor Red
    Write-Host "Expected: $siteConf" -ForegroundColor Yellow
}

Write-Host ""

# =============================================================================
# Step 5: Test nginx configuration
# =============================================================================
Write-Host "Step 5: Testing nginx configuration..." -ForegroundColor Cyan
Write-Host ""

Push-Location "C:\nginx"
$testResult = & .\nginx.exe -t 2>&1
Pop-Location

if ($testResult -match "syntax is ok" -and $testResult -match "successful") {
    Write-Host "[OK] nginx configuration is valid" -ForegroundColor Green
} else {
    Write-Host "[ERROR] nginx configuration has errors:" -ForegroundColor Red
    Write-Host $testResult -ForegroundColor Yellow
}

Write-Host ""

# =============================================================================
# Step 6: Create PHP directory (if needed)
# =============================================================================
Write-Host "Step 6: Checking PHP setup..." -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path "C:\php")) {
    Write-Host "[INFO] Creating C:\php directory" -ForegroundColor Cyan
    New-Item -Path "C:\php" -ItemType Directory -Force | Out-Null
    Write-Host "[OK] Created C:\php - Install PHP here" -ForegroundColor Green
} else {
    Write-Host "[OK] C:\php directory exists" -ForegroundColor Green
}

Write-Host ""

# =============================================================================
# Step 7: Create logs directory
# =============================================================================
Write-Host "Step 7: Creating website logs directory..." -ForegroundColor Cyan
Write-Host ""

$logsDir = "C:\Users\pisig\Desktop\Github\pkodev\website\logs"
if (-not (Test-Path $logsDir)) {
    New-Item -Path $logsDir -ItemType Directory -Force | Out-Null
    Write-Host "[OK] Created logs directory" -ForegroundColor Green
} else {
    Write-Host "[INFO] Logs directory already exists" -ForegroundColor Yellow
}

Write-Host ""

# =============================================================================
# Summary
# =============================================================================
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Setup Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "Next Steps:" -ForegroundColor Yellow
Write-Host ""
Write-Host "1. Install PHP 8.0+ if not installed:" -ForegroundColor White
Write-Host "   - Download from: https://windows.php.net/download/" -ForegroundColor Gray
Write-Host "   - Extract to C:\php\" -ForegroundColor Gray
Write-Host "   - See SETUP_LOCAL.md for detailed instructions" -ForegroundColor Gray
Write-Host ""
Write-Host "2. Configure .env file:" -ForegroundColor White
Write-Host "   - Edit: website\.env" -ForegroundColor Gray
Write-Host "   - Update database credentials" -ForegroundColor Gray
Write-Host ""
Write-Host "3. Start the server:" -ForegroundColor White
Write-Host "   - Run: website\start-local-server.bat" -ForegroundColor Gray
Write-Host ""
Write-Host "4. Visit your website:" -ForegroundColor White
Write-Host "   - URL: http://localhost:8080/" -ForegroundColor Gray
Write-Host "   - Test: http://localhost:8080/test.php" -ForegroundColor Gray
Write-Host ""

Write-Host "For detailed setup instructions, see:" -ForegroundColor Cyan
Write-Host "  - SETUP_LOCAL.md" -ForegroundColor White
Write-Host ""

pause
