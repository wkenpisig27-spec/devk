<?php
/**
 * PKO Website API - Registration Endpoint
 */

require_once __DIR__ . '/../../includes/config.php';

header('Content-Type: application/json');
Security::setHeaders();

// Only accept POST
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    jsonError('Method not allowed', 405);
}

// Get JSON input
$input = json_decode(file_get_contents('php://input'), true);
if (!$input) {
    jsonError('Invalid request body');
}

// Verify CSRF token if provided via session
if (isset($input['csrf_token']) && !CSRF::verifyToken($input['csrf_token'])) {
    jsonError('Invalid security token. Please refresh the page and try again.');
}

// Rate limiting
$ip = Security::getClientIp();
if (RateLimiter::isLimited('register', $ip, 3, 3600)) {
    jsonError('Too many registration attempts. Please try again later.');
}

// Validate input
$validator = new Validator($input);
$validator
    ->username('username')
    ->email('email')
    ->password('password')
    ->passwordConfirm('password_confirm', 'password');

if ($validator->fails()) {
    jsonError($validator->firstError());
}

$username = trim($input['username']);
$email = trim($input['email']);
$password = $input['password'];

// Check if username exists
if (AccountModel::usernameExists($username)) {
    RateLimiter::recordAttempt('register', $ip, 3600);
    jsonError('This username is already taken');
}

// Check if email exists
if (AccountModel::emailExists($email)) {
    RateLimiter::recordAttempt('register', $ip, 3600);
    jsonError('This email is already registered');
}

// Verify reCAPTCHA if enabled
if (RECAPTCHA_ENABLED && !empty($input['g-recaptcha-response'])) {
    if (!Security::verifyRecaptcha($input['g-recaptcha-response'])) {
        jsonError('reCAPTCHA verification failed');
    }
}

// Create account
$accountId = AccountModel::create($username, $password, $email);

if ($accountId) {
    // Log the registration
    error_log("New account registered: $username (ID: $accountId) from IP: $ip");
    
    jsonSuccess([
        'account_id' => $accountId
    ], 'Account created successfully! You can now log in.');
} else {
    RateLimiter::recordAttempt('register', $ip, 3600);
    jsonError('Failed to create account. Please try again.');
}
