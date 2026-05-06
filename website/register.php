<?php
/**
 * PKO Website - Registration Page
 */

require_once __DIR__ . '/includes/config.php';

// Redirect if already logged in
if (Auth::check()) {
    redirect('/dashboard.php');
}

Security::setHeaders();
$csrfToken = CSRF::generateToken();
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <?= metaTags('Register', 'Create a free ' . SERVER_NAME . ' account and start your pirate adventure today!') ?>
    <link rel="stylesheet" href="<?= asset('css/style.css') ?>">
    <link rel="icon" type="image/png" href="<?= asset('images/favicon.png') ?>">
</head>
<body class="auth-page">
    <div class="auth-card">
        <div class="auth-logo">
            <a href="/"><?= e(SERVER_NAME) ?></a>
        </div>
        
        <h1 class="auth-title">Join the Crew!</h1>
        <p class="auth-subtitle">Create your free account and set sail</p>
        
        <form id="register-form" method="POST" data-validate>
            <input type="hidden" name="csrf_token" value="<?= e($csrfToken) ?>">
            
            <div class="form-group">
                <label class="form-label" for="username">Username</label>
                <input 
                    type="text" 
                    id="username" 
                    name="username" 
                    class="form-input" 
                    placeholder="Choose a username (4-16 characters)"
                    required
                    minlength="4"
                    maxlength="16"
                    pattern="[a-zA-Z][a-zA-Z0-9_]*"
                    autocomplete="username"
                    autofocus
                >
                <p class="form-hint">Letters, numbers, and underscores only. Must start with a letter.</p>
            </div>
            
            <div class="form-group">
                <label class="form-label" for="email">Email Address</label>
                <input 
                    type="email" 
                    id="email" 
                    name="email" 
                    class="form-input" 
                    placeholder="Enter your email address"
                    required
                    autocomplete="email"
                >
            </div>
            
            <div class="form-group">
                <label class="form-label" for="password">Password</label>
                <input 
                    type="password" 
                    id="password" 
                    name="password" 
                    class="form-input" 
                    placeholder="Create a password (6-32 characters)"
                    required
                    minlength="6"
                    maxlength="32"
                    autocomplete="new-password"
                    data-strength="password-strength"
                >
                <div class="progress progress-sm mt-2">
                    <div class="progress-bar" id="password-strength" style="width: 0%"></div>
                </div>
                <p class="form-hint">Use letters and numbers for a stronger password.</p>
            </div>
            
            <div class="form-group">
                <label class="form-label" for="password_confirm">Confirm Password</label>
                <input 
                    type="password" 
                    id="password_confirm" 
                    name="password_confirm" 
                    class="form-input" 
                    placeholder="Re-enter your password"
                    required
                    autocomplete="new-password"
                >
            </div>
            
            <div class="form-group">
                <label class="form-checkbox">
                    <input type="checkbox" name="terms" required>
                    <span>I agree to the <a href="/terms.php" class="text-gold" target="_blank">Terms of Service</a> and <a href="/privacy.php" class="text-gold" target="_blank">Privacy Policy</a></span>
                </label>
            </div>
            
            <?php if (RECAPTCHA_ENABLED): ?>
            <div class="form-group">
                <div class="g-recaptcha" data-sitekey="<?= e(RECAPTCHA_SITE_KEY) ?>"></div>
            </div>
            <script src="https://www.google.com/recaptcha/api.js" async defer></script>
            <?php endif; ?>
            
            <button type="submit" class="btn btn-primary w-full btn-lg">
                Create Account
            </button>
        </form>
        
        <p class="auth-footer">
            Already have an account? <a href="/login.php" class="text-gold">Log in here</a>
        </p>
    </div>

    <script src="<?= asset('js/main.js') ?>"></script>
</body>
</html>
