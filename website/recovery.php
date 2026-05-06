<?php
/**
 * PKO Website Recovery Script
 * Run this once to recover from attack/downtime
 * 
 * USAGE: php recovery.php
 * Or visit: https://yoursite.com/recovery.php?key=YOUR_RECOVERY_KEY
 * 
 * DELETE THIS FILE AFTER USE!
 */

// Recovery key - change this to a random string before running!
define('RECOVERY_KEY', 'CHANGE_THIS_TO_RANDOM_STRING_12345');

// Allow CLI or web access with key
$isCli = php_sapi_name() === 'cli';
$hasValidKey = isset($_GET['key']) && hash_equals(RECOVERY_KEY, $_GET['key']);

if (!$isCli && !$hasValidKey) {
    http_response_code(403);
    die('Access denied. Use CLI or provide valid recovery key.');
}

// Output helper
function out($message, $type = 'info') {
    $colors = ['info' => '34', 'success' => '32', 'error' => '31', 'warning' => '33'];
    $color = $colors[$type] ?? '0';
    
    if (php_sapi_name() === 'cli') {
        echo "\033[{$color}m[" . strtoupper($type) . "]\033[0m $message\n";
    } else {
        $htmlColors = ['info' => 'blue', 'success' => 'green', 'error' => 'red', 'warning' => 'orange'];
        echo "<div style='color:{$htmlColors[$type]};font-family:monospace;'>[" . strtoupper($type) . "] $message</div>";
    }
}

if (!$isCli) {
    echo "<html><head><title>PKO Recovery</title></head><body style='background:#1a1a2e;color:#fff;padding:20px;'>";
    echo "<h1>🔧 PKO Website Recovery</h1><pre>";
}

out("PKO Website Recovery Script", 'info');
out("=============================", 'info');
echo "\n";

// 1. Clear rate limiting cache
out("Step 1: Clearing rate limiting cache...", 'info');
$rateLimitDir = sys_get_temp_dir() . '/pko_rate_limit';
if (is_dir($rateLimitDir)) {
    $files = glob($rateLimitDir . '/*.dat');
    $count = 0;
    foreach ($files as $file) {
        if (unlink($file)) $count++;
    }
    out("Cleared $count rate limit files", 'success');
} else {
    out("Rate limit directory not found (this is OK)", 'info');
}

// 2. Clear PHP sessions
out("Step 2: Clearing stale PHP sessions...", 'info');
$sessionPath = session_save_path() ?: sys_get_temp_dir();
if (is_dir($sessionPath)) {
    $oldSessions = 0;
    $threshold = time() - 86400; // 24 hours old
    foreach (glob($sessionPath . '/sess_*') as $file) {
        if (filemtime($file) < $threshold) {
            if (unlink($file)) $oldSessions++;
        }
    }
    out("Cleared $oldSessions stale session files", 'success');
} else {
    out("Session directory not accessible", 'warning');
}

// 3. Clear website cache
out("Step 3: Clearing website cache...", 'info');
$cacheDir = __DIR__ . '/cache';
if (is_dir($cacheDir)) {
    $cacheFiles = glob($cacheDir . '/*');
    $count = 0;
    foreach ($cacheFiles as $file) {
        if (is_file($file) && basename($file) !== '.gitkeep') {
            if (unlink($file)) $count++;
        }
    }
    out("Cleared $count cache files", 'success');
} else {
    out("Cache directory not found", 'info');
}

// 4. Test database connection
out("Step 4: Testing database connection...", 'info');
try {
    require_once __DIR__ . '/includes/config.php';
    
    // Test Account DB
    try {
        $db = Database::getAccountDb();
        $stmt = $db->query("SELECT 1 as test");
        out("AccountServer DB: Connected OK", 'success');
    } catch (Exception $e) {
        out("AccountServer DB: FAILED - " . $e->getMessage(), 'error');
    }
    
    // Test Game DB
    try {
        $db = Database::getGameDb();
        $stmt = $db->query("SELECT 1 as test");
        out("GameDB: Connected OK", 'success');
    } catch (Exception $e) {
        out("GameDB: FAILED - " . $e->getMessage(), 'error');
    }
    
    // Test Web DB
    try {
        $db = Database::getWebDb();
        $stmt = $db->query("SELECT 1 as test");
        out("WebsiteDB: Connected OK", 'success');
    } catch (Exception $e) {
        out("WebsiteDB: FAILED - " . $e->getMessage(), 'error');
    }
    
} catch (Exception $e) {
    out("Config load failed: " . $e->getMessage(), 'error');
}

// 5. Check PHP extensions
out("Step 5: Checking PHP extensions...", 'info');
$requiredExtensions = ['pdo', 'json', 'mbstring', 'openssl'];
$sqlExtensions = ['pdo_sqlsrv', 'sqlsrv', 'pdo_dblib', 'pdo_odbc', 'pdo_mysql'];

foreach ($requiredExtensions as $ext) {
    if (extension_loaded($ext)) {
        out("Extension $ext: Loaded", 'success');
    } else {
        out("Extension $ext: MISSING!", 'error');
    }
}

out("SQL Server drivers (need at least one):", 'info');
$hasSqlDriver = false;
foreach ($sqlExtensions as $ext) {
    if (extension_loaded($ext)) {
        out("  $ext: Loaded", 'success');
        $hasSqlDriver = true;
    }
}
if (!$hasSqlDriver) {
    out("  No SQL Server drivers found!", 'error');
    out("  Install with: sudo apt install php-sybase php-odbc", 'warning');
}

// 6. Check file permissions
out("Step 6: Checking file permissions...", 'info');
$checkDirs = [
    __DIR__ . '/cache' => 'Cache directory',
    __DIR__ . '/assets' => 'Assets directory',
];

foreach ($checkDirs as $dir => $name) {
    if (!is_dir($dir)) {
        out("$name: Not found", 'warning');
    } elseif (!is_writable($dir)) {
        out("$name: NOT WRITABLE - fix with: chmod 755 $dir", 'error');
    } else {
        out("$name: OK", 'success');
    }
}

// 7. Check .env file
out("Step 7: Checking .env configuration...", 'info');
$envFile = __DIR__ . '/.env';
if (!file_exists($envFile)) {
    out(".env file MISSING! Copy from .env.example", 'error');
} else {
    $envContent = file_get_contents($envFile);
    
    // Check for default/unsafe values
    if (strpos($envContent, 'CHANGE_THIS') !== false) {
        out("WARNING: .env contains default 'CHANGE_THIS' values!", 'warning');
    }
    if (strpos($envContent, 'JWT_SECRET=CHANGE') !== false) {
        out("CRITICAL: JWT_SECRET is using default value!", 'error');
    }
    if (strpos($envContent, 'DEBUG_MODE=true') !== false) {
        out("WARNING: DEBUG_MODE is enabled in production!", 'warning');
    }
    out(".env file exists", 'success');
}

// 8. Check error logs
out("Step 8: Recent PHP errors...", 'info');
$errorLog = ini_get('error_log');
if ($errorLog && file_exists($errorLog) && is_readable($errorLog)) {
    $lines = array_slice(file($errorLog), -10);
    foreach ($lines as $line) {
        out(trim($line), 'warning');
    }
} else {
    out("Cannot read PHP error log", 'info');
}

echo "\n";
out("=============================", 'info');
out("Recovery complete!", 'success');
out("", 'info');
out("NEXT STEPS:", 'info');
out("1. Restart PHP-FPM: sudo systemctl restart php8.1-fpm", 'info');
out("2. Restart web server: sudo systemctl restart nginx", 'info');
out("3. Check logs: sudo tail -f /var/log/nginx/error.log", 'info');
out("4. DELETE this recovery.php file when done!", 'warning');

if (!$isCli) {
    echo "</pre></body></html>";
}
