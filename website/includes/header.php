<?php
// Ensure config is loaded
if (!defined('ROOT_PATH')) {
    require_once __DIR__ . '/config.php';
}

$user = Auth::user();
$userId = Auth::id();
$credits = 0;
$gmLevel = 0;

if ($userId) {
    try {
        $gameDb = Database::getGameDb();
        $stmt = $gameDb->prepare("SELECT credit, gm FROM account WHERE act_id = ?");
        $stmt->execute([$userId]);
        $gameAccount = $stmt->fetch();
        $credits = (int)($gameAccount['credit'] ?? 0);
        $gmLevel = (int)($gameAccount['gm'] ?? 0);
    } catch (Exception $e) {
        // Silent fail
    }
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <?= metaTags($pageTitle ?? 'Dashboard', 'Manage your ' . SERVER_NAME . ' account.') ?>
    <link rel="stylesheet" href="<?= asset('css/style.css') ?>">
    <link rel="icon" type="image/png" href="<?= asset('images/favicon.png') ?>">
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css">
</head>
<body>
    <!-- Navigation -->
    <nav class="navbar scrolled" role="navigation">
        <div class="container">
            <div class="navbar-content">
                <a href="/" class="navbar-logo"><?= e(SERVER_NAME) ?></a>
                
                <ul class="navbar-menu" id="navbar-menu">
                    <li><a href="/" class="navbar-link">Home</a></li>
                    <li><a href="/leaderboard.php" class="navbar-link">Leaderboard</a></li>
                    <li><a href="/download.php" class="navbar-link">Download</a></li>
                    <li><a href="https://discord.com/invite/NHmFsuMpS5" target="_blank" class="navbar-link">Discord</a></li>
                    <li><a href="/news-list.php" class="navbar-link">News</a></li>
                    <li><a href="/donate.php" class="navbar-link">Donate</a></li>
                    <?php if ($user && $gmLevel >= 99): ?>
                    <li><a href="/admin/" class="navbar-link text-gold">Admin</a></li>
                    <?php endif; ?>
                </ul>
                
                <div class="navbar-actions">
                    <?php if ($user): ?>
                        <span class="text-gold font-medium"><?= formatNumber($credits) ?> CP</span>
                        <a href="/dashboard.php" class="btn btn-primary btn-sm">Dashboard</a>
                        <a href="#" class="btn btn-secondary btn-sm" data-logout>Logout</a>
                    <?php else: ?>
                        <a href="/login.php" class="btn btn-ghost">Login</a>
                        <a href="/register.php" class="btn btn-primary">Play Free</a>
                    <?php endif; ?>
                </div>
                
                <button class="navbar-toggle" aria-label="Toggle menu" aria-expanded="false">
                    <svg width="24" height="24" fill="none" stroke="currentColor" stroke-width="2">
                        <path d="M4 6h16M4 12h16M4 18h16"></path>
                    </svg>
                </button>
            </div>
        </div>
    </nav>

    <div style="padding-top: 80px;"></div>
