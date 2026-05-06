<?php
/**
 * PKO Website Configuration
 * 
 * This file loads environment variables and provides configuration constants
 * SECURITY: Never commit .env file to version control
 */

declare(strict_types=1);

// Error reporting based on environment
error_reporting(E_ALL);

// Load environment variables from .env file
$envFile = __DIR__ . '/../.env';
if (file_exists($envFile)) {
    $lines = file($envFile, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
    foreach ($lines as $line) {
        // Skip comments
        if (strpos(trim($line), '#') === 0) continue;
        
        // Parse key=value
        if (strpos($line, '=') !== false) {
            list($key, $value) = explode('=', $line, 2);
            $key = trim($key);
            $value = trim($value);
            
            // Remove surrounding quotes
            if (preg_match('/^(["\'])(.*)\\1$/', $value, $matches)) {
                $value = $matches[2];
            }
            
            putenv("$key=$value");
            $_ENV[$key] = $value;
        }
    }
}

// =============================================================================
// DATABASE CONFIGURATION
// =============================================================================

define('DB_ACCOUNT_HOST', getenv('DB_ACCOUNT_HOST') ?: 'localhost\\SQLExpress');
define('DB_ACCOUNT_NAME', getenv('DB_ACCOUNT_NAME') ?: 'AccountServer');
define('DB_ACCOUNT_USER', getenv('DB_ACCOUNT_USER') ?: 'pko_web');
define('DB_ACCOUNT_PASS', getenv('DB_ACCOUNT_PASS') ?: '');

define('DB_GAME_HOST', getenv('DB_GAME_HOST') ?: 'localhost\\SQLExpress');
define('DB_GAME_NAME', getenv('DB_GAME_NAME') ?: 'GameDB');
define('DB_GAME_USER', getenv('DB_GAME_USER') ?: 'pko_web');
define('DB_GAME_PASS', getenv('DB_GAME_PASS') ?: '');

define('DB_WEB_HOST', getenv('DB_WEB_HOST') ?: 'localhost\\SQLExpress');
define('DB_WEB_NAME', getenv('DB_WEB_NAME') ?: 'WebsiteDB');
define('DB_WEB_USER', getenv('DB_WEB_USER') ?: 'pko_web');
define('DB_WEB_PASS', getenv('DB_WEB_PASS') ?: '');

// =============================================================================
// SECURITY CONFIGURATION
// =============================================================================

define('JWT_SECRET', getenv('JWT_SECRET') ?: 'CHANGE_THIS_TO_A_SECURE_SECRET_KEY_AT_LEAST_64_CHARACTERS');
define('JWT_EXPIRATION', (int)(getenv('JWT_EXPIRATION') ?: 86400));
define('SESSION_NAME', getenv('SESSION_NAME') ?: 'pko_session');
define('SESSION_LIFETIME', (int)(getenv('SESSION_LIFETIME') ?: 86400));
define('CSRF_SECRET', getenv('CSRF_SECRET') ?: 'CHANGE_THIS_CSRF_SECRET_KEY');
define('BCRYPT_COST', (int)(getenv('BCRYPT_COST') ?: 10));

// =============================================================================
// RATE LIMITING
// =============================================================================

define('MAX_LOGIN_ATTEMPTS', (int)(getenv('MAX_LOGIN_ATTEMPTS') ?: 5));
define('LOCKOUT_DURATION', (int)(getenv('LOCKOUT_DURATION') ?: 15));
define('API_RATE_LIMIT', (int)(getenv('API_RATE_LIMIT') ?: 60));

// =============================================================================
// SERVER CONFIGURATION
// =============================================================================

define('SERVER_NAME', getenv('GAME_SERVER_NAME') ?: 'Slime Pirates Online');
define('GATE_SERVER_IP', getenv('GATE_SERVER_IP') ?: '127.0.0.1');
define('GATE_SERVER_PORT', (int)(getenv('GATE_SERVER_PORT') ?: 1973));
define('SITE_URL', getenv('SITE_URL') ?: 'http://localhost');
define('DEBUG_MODE', filter_var(getenv('DEBUG_MODE'), FILTER_VALIDATE_BOOLEAN));

// Set display errors based on debug mode
ini_set('display_errors', DEBUG_MODE ? '1' : '0');

// =============================================================================
// EMAIL CONFIGURATION
// =============================================================================

define('MAIL_ENABLED', filter_var(getenv('MAIL_ENABLED'), FILTER_VALIDATE_BOOLEAN));
define('SMTP_HOST', getenv('SMTP_HOST') ?: '');
define('SMTP_PORT', (int)(getenv('SMTP_PORT') ?: 587));
define('SMTP_USER', getenv('SMTP_USER') ?: '');
define('SMTP_PASS', getenv('SMTP_PASS') ?: '');
define('SMTP_FROM_NAME', getenv('SMTP_FROM_NAME') ?: SERVER_NAME);
define('SMTP_FROM_EMAIL', getenv('SMTP_FROM_EMAIL') ?: '');

// =============================================================================
// VOTING CONFIGURATION
// =============================================================================

define('VOTING_ENABLED', filter_var(getenv('VOTING_ENABLED'), FILTER_VALIDATE_BOOLEAN));
define('VOTE_COOLDOWN_HOURS', (int)(getenv('VOTE_COOLDOWN_HOURS') ?: 12));

// =============================================================================
// RECAPTCHA
// =============================================================================

define('RECAPTCHA_ENABLED', filter_var(getenv('RECAPTCHA_ENABLED'), FILTER_VALIDATE_BOOLEAN));
define('RECAPTCHA_SITE_KEY', getenv('RECAPTCHA_SITE_KEY') ?: '');
define('RECAPTCHA_SECRET_KEY', getenv('RECAPTCHA_SECRET_KEY') ?: '');

// =============================================================================
// MAINTENANCE MODE
// =============================================================================

define('MAINTENANCE_MODE', filter_var(getenv('MAINTENANCE_MODE'), FILTER_VALIDATE_BOOLEAN));
define('MAINTENANCE_MESSAGE', getenv('MAINTENANCE_MESSAGE') ?: 'Site under maintenance.');

// =============================================================================
// PATH CONSTANTS
// =============================================================================

define('ROOT_PATH', dirname(__DIR__));
define('INCLUDES_PATH', __DIR__);
define('ASSETS_PATH', ROOT_PATH . '/assets');
define('PAGES_PATH', ROOT_PATH . '/pages');
define('API_PATH', ROOT_PATH . '/api');

// =============================================================================
// CHARACTER CLASSES
// =============================================================================

define('CHARACTER_CLASSES', [
    'newbie' => ['name' => 'Newbie', 'icon' => 'newbie.png'],
    'swordsman' => ['name' => 'Swordsman', 'icon' => 'swordsman.png'],
    'hunter' => ['name' => 'Hunter', 'icon' => 'hunter.png'],
    'sailor' => ['name' => 'Sailor', 'icon' => 'sailor.png'],
    'explorer' => ['name' => 'Explorer', 'icon' => 'explorer.png'],
    'herbalist' => ['name' => 'Herbalist', 'icon' => 'herbalist.png'],
    'champion' => ['name' => 'Champion', 'icon' => 'champion.png'],
    'crusader' => ['name' => 'Crusader', 'icon' => 'crusader.png'],
    'sharpshooter' => ['name' => 'Sharpshooter', 'icon' => 'sharpshooter.png'],
    'cleric' => ['name' => 'Cleric', 'icon' => 'cleric.png'],
    'seal_master' => ['name' => 'Seal Master', 'icon' => 'sealmaster.png'],
    'voyager' => ['name' => 'Voyager', 'icon' => 'voyager.png'],
]);

// =============================================================================
// SESSION CONFIGURATION
// =============================================================================

if (session_status() === PHP_SESSION_NONE) {
    ini_set('session.cookie_httponly', '1');
    ini_set('session.cookie_secure', isset($_SERVER['HTTPS']) ? '1' : '0');
    ini_set('session.cookie_samesite', 'Strict');
    ini_set('session.gc_maxlifetime', (string)SESSION_LIFETIME);
    session_name(SESSION_NAME);
}

// =============================================================================
// TIMEZONE
// =============================================================================

date_default_timezone_set('UTC');

// =============================================================================
// AUTOLOAD INCLUDES
// =============================================================================

require_once INCLUDES_PATH . '/database.php';
require_once INCLUDES_PATH . '/security.php';
require_once INCLUDES_PATH . '/helpers.php';
