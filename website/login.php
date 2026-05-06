<?php
/**
 * PKO Website - Login Page
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
    <?= metaTags('Login', 'Log in to your ' . SERVER_NAME . ' account to access your characters and profile.') ?>
    <link rel="stylesheet" href="<?= asset('css/style.css') ?>">
    <link rel="icon" type="image/png" href="<?= asset('images/favicon.png') ?>">
</head>
<body class="auth-page">
    <div class="auth-card">
        <div class="auth-logo">
            <a href="/"><?= e(SERVER_NAME) ?></a>
        </div>
        
        <h1 class="auth-title">Welcome Back, Pirate!</h1>
        <p class="auth-subtitle">Log in to continue your adventure</p>
        
        <form id="login-form" method="POST" data-validate>
            <input type="hidden" name="csrf_token" value="<?= e($csrfToken) ?>">
            
            <div class="form-group">
                <label class="form-label" for="username">Username</label>
                <input 
                    type="text" 
                    id="username" 
                    name="username" 
                    class="form-input" 
                    placeholder="Enter your username"
                    required
                    autocomplete="username"
                    autofocus
                >
            </div>
            
            <div class="form-group">
                <label class="form-label" for="password">Password</label>
                <input 
                    type="password" 
                    id="password" 
                    name="password" 
                    class="form-input" 
                    placeholder="Enter your password"
                    required
                    autocomplete="current-password"
                >
            </div>
            
            <div class="form-group flex justify-between items-center">
                <label class="form-checkbox">
                    <input type="checkbox" name="remember">
                    <span>Remember me</span>
                </label>
                <a href="/forgot-password.php" class="text-sm text-gold">Forgot password?</a>
            </div>
            
            <button type="submit" class="btn btn-primary w-full btn-lg">
                Log In
            </button>
        </form>
        
        <p class="auth-footer">
            Don't have an account? <a href="/register.php" class="text-gold">Create one for free</a>
        </p>
    </div>

    <script src="<?= asset('js/main.js') ?>"></script>
</body>
</html>
