<?php
/**
 * PKO Website Security Module
 * 
 * Handles JWT authentication, CSRF protection, rate limiting,
 * input validation, and other security measures
 */

declare(strict_types=1);

/**
 * IP Blocklist Checker - Call this early in request lifecycle
 */
class IPBlocker
{
    private static ?array $blocklist = null;
    
    /**
     * Check if current IP is blocked and die if so
     */
    public static function check(): void
    {
        $ip = self::getClientIp();
        
        if (self::isBlocked($ip)) {
            http_response_code(403);
            die('Access denied.');
        }
    }
    
    /**
     * Check if IP is in blocklist
     */
    public static function isBlocked(string $ip): bool
    {
        $blocklist = self::getBlocklist();
        
        foreach ($blocklist as $blocked) {
            $blocked = trim($blocked);
            if (empty($blocked) || strpos($blocked, '#') === 0) continue;
            
            // Check exact match
            if ($ip === $blocked) return true;
            
            // Check CIDR range
            if (strpos($blocked, '/') !== false && self::ipInRange($ip, $blocked)) {
                return true;
            }
        }
        
        return false;
    }
    
    /**
     * Check if IP is within CIDR range
     */
    private static function ipInRange(string $ip, string $cidr): bool
    {
        list($subnet, $bits) = explode('/', $cidr);
        $ip = ip2long($ip);
        $subnet = ip2long($subnet);
        $mask = -1 << (32 - (int)$bits);
        $subnet &= $mask;
        return ($ip & $mask) === $subnet;
    }
    
    /**
     * Get client IP address
     */
    private static function getClientIp(): string
    {
        $headers = ['HTTP_CF_CONNECTING_IP', 'HTTP_X_FORWARDED_FOR', 'HTTP_X_REAL_IP', 'REMOTE_ADDR'];
        
        foreach ($headers as $header) {
            if (!empty($_SERVER[$header])) {
                $ip = $_SERVER[$header];
                if (strpos($ip, ',') !== false) {
                    $ip = trim(explode(',', $ip)[0]);
                }
                if (filter_var($ip, FILTER_VALIDATE_IP)) {
                    return $ip;
                }
            }
        }
        return '0.0.0.0';
    }
    
    /**
     * Load blocklist from file
     */
    private static function getBlocklist(): array
    {
        if (self::$blocklist === null) {
            $file = __DIR__ . '/blocklist.php';
            self::$blocklist = file_exists($file) ? (include $file) : [];
        }
        return self::$blocklist;
    }
    
    /**
     * Add IP to blocklist file
     */
    public static function addToBlocklist(string $ip): bool
    {
        $file = __DIR__ . '/blocklist.php';
        $blocklist = file_exists($file) ? (include $file) : [];
        
        if (!in_array($ip, $blocklist)) {
            $blocklist[] = $ip;
            $content = "<?php\nreturn " . var_export($blocklist, true) . ";\n";
            return file_put_contents($file, $content, LOCK_EX) !== false;
        }
        return true;
    }
}

// Check blocklist immediately when this file is loaded
IPBlocker::check();

/**
 * JWT Token Handler
 */
class JWT
{
    /**
     * Generate a JWT token
     */
    public static function generate(array $payload): string
    {
        $header = [
            'alg' => 'HS256',
            'typ' => 'JWT'
        ];
        
        $payload['iat'] = time();
        $payload['exp'] = time() + JWT_EXPIRATION;
        
        $headerEncoded = self::base64UrlEncode(json_encode($header));
        $payloadEncoded = self::base64UrlEncode(json_encode($payload));
        
        $signature = hash_hmac('sha256', "$headerEncoded.$payloadEncoded", JWT_SECRET, true);
        $signatureEncoded = self::base64UrlEncode($signature);
        
        return "$headerEncoded.$payloadEncoded.$signatureEncoded";
    }
    
    /**
     * Verify and decode a JWT token
     */
    public static function verify(string $token): ?array
    {
        $parts = explode('.', $token);
        if (count($parts) !== 3) {
            return null;
        }
        
        list($headerEncoded, $payloadEncoded, $signatureEncoded) = $parts;
        
        // Verify signature
        $signature = self::base64UrlDecode($signatureEncoded);
        $expectedSignature = hash_hmac('sha256', "$headerEncoded.$payloadEncoded", JWT_SECRET, true);
        
        if (!hash_equals($expectedSignature, $signature)) {
            return null;
        }
        
        // Decode payload
        $payload = json_decode(self::base64UrlDecode($payloadEncoded), true);
        if (!$payload) {
            return null;
        }
        
        // Check expiration
        if (isset($payload['exp']) && $payload['exp'] < time()) {
            return null;
        }
        
        return $payload;
    }
    
    private static function base64UrlEncode(string $data): string
    {
        return rtrim(strtr(base64_encode($data), '+/', '-_'), '=');
    }
    
    private static function base64UrlDecode(string $data): string
    {
        return base64_decode(strtr($data, '-_', '+/'));
    }
}

/**
 * CSRF Protection
 */
class CSRF
{
    /**
     * Generate CSRF token
     */
    public static function generateToken(): string
    {
        if (session_status() !== PHP_SESSION_ACTIVE) {
            session_start();
        }
        
        $token = bin2hex(random_bytes(32));
        $_SESSION['csrf_token'] = $token;
        $_SESSION['csrf_token_time'] = time();
        
        return $token;
    }
    
    /**
     * Verify CSRF token
     */
    public static function verifyToken(string $token): bool
    {
        if (session_status() !== PHP_SESSION_ACTIVE) {
            session_start();
        }
        
        if (!isset($_SESSION['csrf_token']) || !isset($_SESSION['csrf_token_time'])) {
            return false;
        }
        
        // Token expires after 1 hour
        if (time() - $_SESSION['csrf_token_time'] > 3600) {
            unset($_SESSION['csrf_token'], $_SESSION['csrf_token_time']);
            return false;
        }
        
        return hash_equals($_SESSION['csrf_token'], $token);
    }
    
    /**
     * Generate hidden input field
     */
    public static function field(): string
    {
        $token = self::generateToken();
        return '<input type="hidden" name="csrf_token" value="' . htmlspecialchars($token) . '">';
    }
}

/**
 * Rate Limiter
 */
class RateLimiter
{
    private static string $cacheDir = '';
    
    private static function getCacheDir(): string
    {
        if (empty(self::$cacheDir)) {
            self::$cacheDir = sys_get_temp_dir() . '/pko_rate_limit';
            if (!is_dir(self::$cacheDir)) {
                mkdir(self::$cacheDir, 0755, true);
            }
        }
        return self::$cacheDir;
    }
    
    /**
     * Check if action is rate limited
     */
    public static function isLimited(string $action, string $identifier, int $maxAttempts, int $windowSeconds): bool
    {
        $key = md5("{$action}:{$identifier}");
        $file = self::getCacheDir() . "/{$key}.dat";
        
        $data = self::loadData($file);
        $now = time();
        
        // Clean old entries
        $data = array_filter($data, fn($time) => $now - $time < $windowSeconds);
        
        return count($data) >= $maxAttempts;
    }
    
    /**
     * Record an attempt
     */
    public static function recordAttempt(string $action, string $identifier, int $windowSeconds = 60): void
    {
        $key = md5("{$action}:{$identifier}");
        $file = self::getCacheDir() . "/{$key}.dat";
        
        $data = self::loadData($file);
        $now = time();
        
        // Clean old entries
        $data = array_filter($data, fn($time) => $now - $time < $windowSeconds);
        $data[] = $now;
        
        self::saveData($file, $data);
    }
    
    /**
     * Get remaining cooldown time
     */
    public static function getCooldownRemaining(string $action, string $identifier, int $windowSeconds): int
    {
        $key = md5("{$action}:{$identifier}");
        $file = self::getCacheDir() . "/{$key}.dat";
        
        $data = self::loadData($file);
        if (empty($data)) return 0;
        
        $oldest = min($data);
        $remaining = ($oldest + $windowSeconds) - time();
        
        return max(0, $remaining);
    }
    
    /**
     * Clear rate limit for identifier
     */
    public static function clear(string $action, string $identifier): void
    {
        $key = md5("{$action}:{$identifier}");
        $file = self::getCacheDir() . "/{$key}.dat";
        
        if (file_exists($file)) {
            unlink($file);
        }
    }
    
    private static function loadData(string $file): array
    {
        if (!file_exists($file)) return [];
        $content = file_get_contents($file);
        return $content ? json_decode($content, true) ?? [] : [];
    }
    
    private static function saveData(string $file, array $data): void
    {
        file_put_contents($file, json_encode($data), LOCK_EX);
    }
}

/**
 * Input Validator
 */
class Validator
{
    private array $errors = [];
    private array $data = [];
    
    public function __construct(array $data)
    {
        $this->data = $data;
    }
    
    /**
     * Validate username
     */
    public function username(string $field, string $label = 'Username'): self
    {
        $value = trim($this->data[$field] ?? '');
        
        if (empty($value)) {
            $this->errors[$field] = "$label is required";
        } elseif (strlen($value) < 4 || strlen($value) > 16) {
            $this->errors[$field] = "$label must be 4-16 characters";
        } elseif (!preg_match('/^[a-zA-Z][a-zA-Z0-9_]*$/', $value)) {
            $this->errors[$field] = "$label must start with a letter and contain only letters, numbers, and underscores";
        }
        
        return $this;
    }
    
    /**
     * Validate password
     */
    public function password(string $field, string $label = 'Password'): self
    {
        $value = $this->data[$field] ?? '';
        
        if (empty($value)) {
            $this->errors[$field] = "$label is required";
        } elseif (strlen($value) < 6 || strlen($value) > 32) {
            $this->errors[$field] = "$label must be 6-32 characters";
        } elseif (!preg_match('/[A-Za-z]/', $value) || !preg_match('/[0-9]/', $value)) {
            $this->errors[$field] = "$label must contain both letters and numbers";
        }
        
        return $this;
    }
    
    /**
     * Validate password confirmation
     */
    public function passwordConfirm(string $field, string $passwordField, string $label = 'Password confirmation'): self
    {
        $value = $this->data[$field] ?? '';
        $password = $this->data[$passwordField] ?? '';
        
        if ($value !== $password) {
            $this->errors[$field] = "Passwords do not match";
        }
        
        return $this;
    }
    
    /**
     * Validate email
     */
    public function email(string $field, string $label = 'Email'): self
    {
        $value = trim($this->data[$field] ?? '');
        
        if (empty($value)) {
            $this->errors[$field] = "$label is required";
        } elseif (!filter_var($value, FILTER_VALIDATE_EMAIL)) {
            $this->errors[$field] = "Invalid email format";
        } elseif (strlen($value) > 100) {
            $this->errors[$field] = "$label is too long";
        }
        
        return $this;
    }
    
    /**
     * Validate required field
     */
    public function required(string $field, string $label): self
    {
        $value = trim($this->data[$field] ?? '');
        
        if (empty($value)) {
            $this->errors[$field] = "$label is required";
        }
        
        return $this;
    }
    
    /**
     * Validate integer
     */
    public function integer(string $field, string $label, ?int $min = null, ?int $max = null): self
    {
        $value = $this->data[$field] ?? '';
        
        if (!is_numeric($value) || (int)$value != $value) {
            $this->errors[$field] = "$label must be a whole number";
        } else {
            $intValue = (int)$value;
            if ($min !== null && $intValue < $min) {
                $this->errors[$field] = "$label must be at least $min";
            }
            if ($max !== null && $intValue > $max) {
                $this->errors[$field] = "$label must be at most $max";
            }
        }
        
        return $this;
    }
    
    /**
     * Custom validation
     */
    public function custom(string $field, callable $callback, string $errorMessage): self
    {
        $value = $this->data[$field] ?? '';
        
        if (!$callback($value, $this->data)) {
            $this->errors[$field] = $errorMessage;
        }
        
        return $this;
    }
    
    /**
     * Check if validation passed
     */
    public function passes(): bool
    {
        return empty($this->errors);
    }
    
    /**
     * Check if validation failed
     */
    public function fails(): bool
    {
        return !empty($this->errors);
    }
    
    /**
     * Get all errors
     */
    public function errors(): array
    {
        return $this->errors;
    }
    
    /**
     * Get first error
     */
    public function firstError(): ?string
    {
        return $this->errors ? reset($this->errors) : null;
    }
    
    /**
     * Get sanitized value
     */
    public function getValue(string $field, $default = null)
    {
        return $this->data[$field] ?? $default;
    }
}

/**
 * Security Utilities
 */
class Security
{
    /**
     * Sanitize string for HTML output
     */
    public static function escape(string $value): string
    {
        return htmlspecialchars($value, ENT_QUOTES | ENT_HTML5, 'UTF-8');
    }
    
    /**
     * Sanitize string for JavaScript
     */
    public static function escapeJs(string $value): string
    {
        return json_encode($value, JSON_HEX_TAG | JSON_HEX_QUOT | JSON_HEX_AMP | JSON_HEX_APOS);
    }
    
    /**
     * Get client IP address
     */
    public static function getClientIp(): string
    {
        $headers = [
            'HTTP_CF_CONNECTING_IP',  // Cloudflare
            'HTTP_X_FORWARDED_FOR',
            'HTTP_X_REAL_IP',
            'REMOTE_ADDR'
        ];
        
        foreach ($headers as $header) {
            if (!empty($_SERVER[$header])) {
                $ip = $_SERVER[$header];
                // X-Forwarded-For may contain multiple IPs
                if (strpos($ip, ',') !== false) {
                    $ip = trim(explode(',', $ip)[0]);
                }
                if (filter_var($ip, FILTER_VALIDATE_IP)) {
                    return $ip;
                }
            }
        }
        
        return '0.0.0.0';
    }
    
    /**
     * Generate secure random token
     */
    public static function generateToken(int $length = 32): string
    {
        return bin2hex(random_bytes($length));
    }
    
    /**
     * Verify reCAPTCHA (if enabled)
     */
    public static function verifyRecaptcha(string $response): bool
    {
        if (!RECAPTCHA_ENABLED || empty(RECAPTCHA_SECRET_KEY)) {
            return true;
        }
        
        $data = [
            'secret' => RECAPTCHA_SECRET_KEY,
            'response' => $response,
            'remoteip' => self::getClientIp()
        ];
        
        $options = [
            'http' => [
                'method' => 'POST',
                'header' => 'Content-Type: application/x-www-form-urlencoded',
                'content' => http_build_query($data)
            ]
        ];
        
        $context = stream_context_create($options);
        $result = file_get_contents('https://www.google.com/recaptcha/api/siteverify', false, $context);
        
        if ($result === false) {
            error_log("reCAPTCHA verification failed: could not contact Google");
            return false;
        }
        
        $json = json_decode($result, true);
        return $json['success'] ?? false;
    }
    
    /**
     * Set security headers
     */
    public static function setHeaders(): void
    {
        header('X-Content-Type-Options: nosniff');
        header('X-Frame-Options: SAMEORIGIN');
        header('X-XSS-Protection: 1; mode=block');
        header('Referrer-Policy: strict-origin-when-cross-origin');
        
        if (isset($_SERVER['HTTPS']) && $_SERVER['HTTPS'] === 'on') {
            header('Strict-Transport-Security: max-age=31536000; includeSubDomains');
        }
    }
}

/**
 * Authentication Handler
 */
class Auth
{
    private static ?array $user = null;
    private static ?string $cachedSessionId = null;
    
    /**
     * Reset static cache - call at beginning of each request
     */
    public static function resetCache(): void
    {
        self::$user = null;
        self::$cachedSessionId = null;
    }
    
    /**
     * Attempt to authenticate user
     */
    public static function attempt(string $username, string $password): array
    {
        $ip = Security::getClientIp();
        
        // Check rate limiting
        if (RateLimiter::isLimited('login', $ip, MAX_LOGIN_ATTEMPTS, LOCKOUT_DURATION * 60)) {
            $remaining = RateLimiter::getCooldownRemaining('login', $ip, LOCKOUT_DURATION * 60);
            $minutes = ceil($remaining / 60);
            return [
                'success' => false,
                'error' => "Too many login attempts. Please try again in $minutes minute(s)."
            ];
        }
        
        // Find user
        $account = AccountModel::findByUsername($username);
        
        if (!$account) {
            RateLimiter::recordAttempt('login', $ip, LOCKOUT_DURATION * 60);
            return ['success' => false, 'error' => 'Invalid username or password'];
        }
        
        // Check if banned
        if ($account['ban']) {
            return ['success' => false, 'error' => 'This account has been banned'];
        }
        
        // Check enable_login_time
        if (!empty($account['enable_login_time'])) {
            $enableTime = strtotime($account['enable_login_time']);
            if ($enableTime > time()) {
                $until = date('Y-m-d H:i:s', $enableTime);
                return ['success' => false, 'error' => "Account suspended until $until"];
            }
        }
        
        // Verify password
        if (!AccountModel::verifyPassword($account, $password)) {
            RateLimiter::recordAttempt('login', $ip, LOCKOUT_DURATION * 60);
            return ['success' => false, 'error' => 'Invalid username or password'];
        }
        
        // Clear rate limit on successful login
        RateLimiter::clear('login', $ip);
        
        // Update login info
        AccountModel::updateLoginInfo((int)$account['id'], $ip);
        
        // Generate JWT token
        $token = JWT::generate([
            'sub' => $account['id'],
            'username' => $account['name'],
            'email' => $account['email']
        ]);
        
        // Start session
        if (session_status() !== PHP_SESSION_ACTIVE) {
            session_start();
        }
        $_SESSION['user_id'] = $account['id'];
        $_SESSION['username'] = $account['name'];
        $_SESSION['login_time'] = time();
        
        // Get GM Level
        $gmLevel = AccountModel::getGmLevel((int)$account['id']);

        return [
            'success' => true,
            'token' => $token,
            'user' => [
                'id' => $account['id'],
                'username' => $account['name'],
                'email' => $account['email'],
                'gm_level' => $gmLevel
            ]
        ];
    }
    
    /**
     * Get current authenticated user
     */
    public static function user(): ?array
    {
        // Start session first to get session ID
        if (session_status() !== PHP_SESSION_ACTIVE) {
            session_start();
        }
        
        $currentSessionId = session_id();
        
        // Only use cache if session ID matches (prevents cross-user caching in PHP-FPM)
        if (self::$user !== null && self::$cachedSessionId === $currentSessionId) {
            return self::$user;
        }
        
        // Clear cache if session changed
        self::$user = null;
        self::$cachedSessionId = $currentSessionId;
        
        if (isset($_SESSION['user_id'])) {
            self::$user = AccountModel::findById((int)$_SESSION['user_id']);
            return self::$user;
        }
        
        // Try JWT from Authorization header
        $authHeader = $_SERVER['HTTP_AUTHORIZATION'] ?? '';
        if (preg_match('/Bearer\s+(.+)/', $authHeader, $matches)) {
            $payload = JWT::verify($matches[1]);
            if ($payload && isset($payload['sub'])) {
                self::$user = AccountModel::findById((int)$payload['sub']);
                return self::$user;
            }
        }
        
        return null;
    }
    
    /**
     * Check if user is authenticated
     */
    public static function check(): bool
    {
        return self::user() !== null;
    }
    
    /**
     * Get user ID
     */
    public static function id(): ?int
    {
        $user = self::user();
        return $user ? (int)$user['id'] : null;
    }
    
    /**
     * Logout
     */
    public static function logout(): void
    {
        self::$user = null;
        
        if (session_status() !== PHP_SESSION_ACTIVE) {
            session_start();
        }
        
        // Clear session
        $_SESSION = [];
        
        // Destroy session cookie
        if (ini_get('session.use_cookies')) {
            $params = session_get_cookie_params();
            setcookie(
                session_name(),
                '',
                time() - 42000,
                $params['path'],
                $params['domain'],
                $params['secure'],
                $params['httponly']
            );
        }
        
        session_destroy();
    }
    
    /**
     * Require authentication (redirect if not logged in)
     */
    public static function require(): void
    {
        if (!self::check()) {
            if (self::isApiRequest()) {
                http_response_code(401);
                header('Content-Type: application/json');
                echo json_encode(['error' => 'Authentication required']);
                exit;
            }
            
            header('Location: /login.php?redirect=' . urlencode($_SERVER['REQUEST_URI']));
            exit;
        }
    }
    
    /**
     * Check if this is an API request
     */
    private static function isApiRequest(): bool
    {
        return strpos($_SERVER['REQUEST_URI'] ?? '', '/api/') === 0
            || (isset($_SERVER['HTTP_ACCEPT']) && strpos($_SERVER['HTTP_ACCEPT'], 'application/json') !== false);
    }
}
