<?php
/**
 * PKO Website - Player Dashboard
 */

require_once __DIR__ . '/includes/config.php';

// Require authentication
Auth::require();

$user = Auth::user();
$userId = Auth::id();

// Get user's characters
$characters = CharacterModel::getByAccountId($userId);

// Get account details
$details = AccountModel::getDetails($userId);
$credits = 0;

// Get credits from GameDB account table
try {
    $gameDb = Database::getGameDb();
    $stmt = $gameDb->prepare("SELECT credit FROM account WHERE act_id = ?");
    $stmt->execute([$userId]);
    $gameAccount = $stmt->fetch();
    $credits = (int)($gameAccount['credit'] ?? 0);
} catch (Exception $e) {
    error_log("Failed to get credits: " . $e->getMessage());
}

// Get GM level for admin access
$gmLevel = AccountModel::getGmLevel($userId);

Security::setHeaders();
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <?= metaTags('Dashboard', 'Manage your ' . SERVER_NAME . ' account, characters, and profile.') ?>
    <link rel="stylesheet" href="<?= asset('css/style.css') ?>">
    <link rel="icon" type="image/png" href="<?= asset('images/favicon.png') ?>">
</head>
<body>
    <!-- Navigation -->
    <nav class="navbar scrolled" role="navigation">
        <div class="container">
            <div class="navbar-content">
                <a href="/" class="navbar-logo"><?= e(SERVER_NAME) ?></a>
                
                <ul class="navbar-menu" id="navbar-menu">
                    <li><a href="/" class="navbar-link">Home</a></li>
                    <li><a href="/news-list.php" class="navbar-link">News</a></li>
                    <li><a href="/donate.php" class="navbar-link">Item Mall</a></li>
                    <li><a href="/download.php" class="navbar-link">Download</a></li>
                    <li><a href="/leaderboard.php" class="navbar-link">Leaderboard</a></li>
                    <li><a href="https://discord.com/invite/NHmFsuMpS5" target="_blank" class="navbar-link">Discord</a></li>
                </ul>
                
                <div class="navbar-actions">
                    <span class="text-gold font-medium"><?= formatNumber($credits) ?> CP</span>
                    <a href="#" class="btn btn-secondary" data-logout>Logout</a>
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

    <!-- Dashboard Header -->
    <section class="dashboard-header">
        <div class="container">
            <p class="dashboard-welcome">Welcome back,</p>
            <h1 class="dashboard-title"><?= e($user['name']) ?></h1>
            
            <div class="dashboard-stats">
                <div class="dashboard-stat">
                    <div class="dashboard-stat-value" data-credits><?= formatNumber($credits) ?></div>
                    <div class="dashboard-stat-label">Credits (CP)</div>
                </div>
                <div class="dashboard-stat">
                    <div class="dashboard-stat-value"><?= count($characters) ?></div>
                    <div class="dashboard-stat-label">Characters</div>
                </div>
                <div class="dashboard-stat">
                    <div class="dashboard-stat-value"><?= $user['login_status'] ? 'Online' : 'Offline' ?></div>
                    <div class="dashboard-stat-label">Status</div>
                </div>
            </div>
        </div>
    </section>

    <!-- Dashboard Content -->
    <section class="section">
        <div class="container">
            <div class="dashboard-layout">
                <!-- Sidebar -->
                <aside class="sidebar hide-mobile">
                    <ul class="sidebar-menu">
                        <li>
                            <a href="#characters" class="sidebar-link active">
                                <svg class="sidebar-link-icon" fill="currentColor" viewBox="0 0 20 20">
                                    <path d="M10 9a3 3 0 100-6 3 3 0 000 6zm-7 9a7 7 0 1114 0H3z"/>
                                </svg>
                                Characters
                            </a>
                        </li>
                        <li>
                            <a href="/settings.php" class="sidebar-link">
                                <svg class="sidebar-link-icon" fill="currentColor" viewBox="0 0 20 20">
                                    <path fill-rule="evenodd" d="M11.49 3.17c-.38-1.56-2.6-1.56-2.98 0a1.532 1.532 0 01-2.286.948c-1.372-.836-2.942.734-2.106 2.106.54.886.061 2.042-.947 2.287-1.561.379-1.561 2.6 0 2.978a1.532 1.532 0 01.947 2.287c-.836 1.372.734 2.942 2.106 2.106a1.532 1.532 0 012.287.947c.379 1.561 2.6 1.561 2.978 0a1.533 1.533 0 012.287-.947c1.372.836 2.942-.734 2.106-2.106a1.533 1.533 0 01.947-2.287c1.561-.379 1.561-2.6 0-2.978a1.532 1.532 0 01-.947-2.287c.836-1.372-.734-2.942-2.106-2.106a1.532 1.532 0 01-2.287-.947zM10 13a3 3 0 100-6 3 3 0 000 6z" clip-rule="evenodd"/>
                                </svg>
                                Settings
                            </a>
                        </li>
                        <li>
                            <a href="/download.php" class="sidebar-link">
                                <svg class="sidebar-link-icon" fill="currentColor" viewBox="0 0 20 20">
                                    <path fill-rule="evenodd" d="M3 17a1 1 0 011-1h12a1 1 0 110 2H4a1 1 0 01-1-1zm3.293-7.707a1 1 0 011.414 0L9 10.586V3a1 1 0 112 0v7.586l1.293-1.293a1 1 0 111.414 1.414l-3 3a1 1 0 01-1.414 0l-3-3a1 1 0 010-1.414z" clip-rule="evenodd"/>
                                </svg>
                                Download
                            </a>
                        </li>
                        <?php if ($gmLevel >= 99): ?>
                        <li>
                            <a href="/admin/news.php" class="sidebar-link">
                                <svg class="sidebar-link-icon" fill="currentColor" viewBox="0 0 20 20">
                                    <path d="M2 5a2 2 0 012-2h7a2 2 0 012 2v4a2 2 0 01-2 2H9l-3 3v-3H4a2 2 0 01-2-2V5z"/>
                                    <path d="M15 7v2a4 4 0 01-4 4H9.828l-1.766 1.767c.28.149.599.233.938.233h2l3 3v-3h2a2 2 0 002-2V9a2 2 0 00-2-2h-1z"/>
                                </svg>
                                Admin
                            </a>
                        </li>
                        <?php endif; ?>
                    </ul>
                </aside>

                <!-- Main Content -->
                <main>
                    <!-- Characters Section -->
                    <div id="characters">
                        <h2 class="title-section mb-6">Your Characters</h2>
                        
                        <?php if (empty($characters)): ?>
                        <div class="card text-center p-8">
                            <div class="stat-icon mb-4" style="margin: 0 auto;">
                                <svg width="24" height="24" fill="currentColor" viewBox="0 0 20 20">
                                    <path fill-rule="evenodd" d="M10 9a3 3 0 100-6 3 3 0 000 6zm-7 9a7 7 0 1114 0H3z" clip-rule="evenodd"/>
                                </svg>
                            </div>
                            <h3 class="text-xl font-semibold mb-2">No Characters Yet</h3>
                            <p class="text-muted mb-4">
                                Download the game client and create your first character to begin your adventure!
                            </p>
                            <a href="/download.php" class="btn btn-primary">Download Game</a>
                        </div>
                        <?php else: ?>
                        <div class="grid grid-cols-3">
                            <?php foreach ($characters as $char): ?>
                            <div class="character-card">
                                <div class="character-header">
                                    <span class="character-level">Lv.<?= $char['degree'] ?></span>
                                    <img 
                                        class="character-avatar" 
                                        src="<?= asset('images/classes/' . strtolower(str_replace(' ', '_', $char['job'] ?: 'newbie')) . '.svg') ?>" 
                                        alt="<?= e(getClassName($char['job'])) ?>"
                                        onerror="this.src='<?= asset('images/classes/newbie.svg') ?>'"
                                    >
                                    <h3 class="character-name"><?= e($char['cha_name']) ?></h3>
                                    <p class="character-class"><?= e(getClassName($char['job'])) ?></p>
                                </div>
                                <div class="character-stats">
                                    <div class="character-stat">
                                        <div class="character-stat-value"><?= formatNumber($char['hp']) ?></div>
                                        <div class="character-stat-label">HP</div>
                                    </div>
                                    <div class="character-stat">
                                        <div class="character-stat-value"><?= formatNumber($char['sp']) ?></div>
                                        <div class="character-stat-label">SP</div>
                                    </div>
                                    <div class="character-stat">
                                        <div class="character-stat-value"><?= formatNumber($char['gd']) ?></div>
                                        <div class="character-stat-label">Gold</div>
                                    </div>
                                </div>
                                <div class="p-4">
                                    <div class="grid grid-cols-3 gap-2 text-center text-sm">
                                        <div>
                                            <div class="font-semibold text-gold"><?= $char['str'] ?></div>
                                            <div class="text-muted text-xs">STR</div>
                                        </div>
                                        <div>
                                            <div class="font-semibold text-gold"><?= $char['dex'] ?></div>
                                            <div class="text-muted text-xs">DEX</div>
                                        </div>
                                        <div>
                                            <div class="font-semibold text-gold"><?= $char['agi'] ?></div>
                                            <div class="text-muted text-xs">AGI</div>
                                        </div>
                                        <div>
                                            <div class="font-semibold text-gold"><?= $char['con'] ?></div>
                                            <div class="text-muted text-xs">CON</div>
                                        </div>
                                        <div>
                                            <div class="font-semibold text-gold"><?= $char['sta'] ?></div>
                                            <div class="text-muted text-xs">STA</div>
                                        </div>
                                        <div>
                                            <div class="font-semibold text-gold"><?= $char['luk'] ?></div>
                                            <div class="text-muted text-xs">LUK</div>
                                        </div>
                                    </div>
                                    
                                    <?php if ($char['guild_id'] > 0): ?>
                                    <?php $guild = GuildModel::findById($char['guild_id']); ?>
                                    <?php if ($guild): ?>
                                    <div class="mt-4 pt-4 border-t border-light text-center">
                                        <span class="badge badge-gold"><?= e($guild['guild_name']) ?></span>
                                    </div>
                                    <?php endif; ?>
                                    <?php endif; ?>
                                    
                                    <div class="mt-4 text-center text-xs text-muted">
                                        📍 <?= e($char['map']) ?>
                                    </div>
                                </div>
                            </div>
                            <?php endforeach; ?>
                        </div>
                        <?php endif; ?>
                    </div>

                    <!-- Quick Actions -->
                    <div class="mt-8">
                        <h2 class="title-section mb-6">Quick Actions</h2>
                        
                        <div class="grid grid-cols-2 gap-4">
                            <a href="/leaderboard.php" class="card text-center p-6 transition hover:border-gold">
                                <div class="stat-icon mb-4" style="margin: 0 auto;">
                                    <svg width="24" height="24" fill="currentColor" viewBox="0 0 20 20">
                                        <path d="M5 3a2 2 0 00-2 2v2a2 2 0 002 2h2a2 2 0 002-2V5a2 2 0 00-2-2H5zM5 11a2 2 0 00-2 2v2a2 2 0 002 2h2a2 2 0 002-2v-2a2 2 0 00-2-2H5zM11 5a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2V5zM14 11a1 1 0 011 1v1h1a1 1 0 110 2h-1v1a1 1 0 11-2 0v-1h-1a1 1 0 110-2h1v-1a1 1 0 011-1z"/>
                                    </svg>
                                </div>
                                <h3 class="font-semibold">Leaderboard</h3>
                                <p class="text-muted text-sm">View rankings</p>
                            </a>
                            
                            <a href="/download.php" class="card text-center p-6 transition hover:border-gold">
                                <div class="stat-icon mb-4" style="margin: 0 auto;">
                                    <svg width="24" height="24" fill="currentColor" viewBox="0 0 20 20">
                                        <path fill-rule="evenodd" d="M3 17a1 1 0 011-1h12a1 1 0 110 2H4a1 1 0 01-1-1zm3.293-7.707a1 1 0 011.414 0L9 10.586V3a1 1 0 112 0v7.586l1.293-1.293a1 1 0 111.414 1.414l-3 3a1 1 0 01-1.414 0l-3-3a1 1 0 010-1.414z" clip-rule="evenodd"/>
                                    </svg>
                                </div>
                                <h3 class="font-semibold">Download</h3>
                                <p class="text-muted text-sm">Get game client</p>
                            </a>
                        </div>
                    </div>

                    <!-- Account Info -->
                    <div class="mt-8">
                        <h2 class="title-section mb-6">Account Information</h2>
                        
                        <div class="card">
                            <div class="grid grid-cols-2 gap-6">
                                <div>
                                    <p class="text-muted text-sm">Username</p>
                                    <p class="font-semibold"><?= e($user['name']) ?></p>
                                </div>
                                <div>
                                    <p class="text-muted text-sm">Email</p>
                                    <p class="font-semibold"><?= e($user['email'] ?? 'Not set') ?></p>
                                </div>
                                <div>
                                    <p class="text-muted text-sm">Last Login</p>
                                    <p class="font-semibold"><?= $user['last_login_time'] ? timeAgo($user['last_login_time']) : 'Never' ?></p>
                                </div>
                                <div>
                                    <p class="text-muted text-sm">Last IP</p>
                                    <p class="font-semibold"><?= e($user['last_login_ip'] ?? 'Unknown') ?></p>
                                </div>
                                <div>
                                    <p class="text-muted text-sm">Total Playtime</p>
                                    <p class="font-semibold"><?= formatDuration((int)($user['total_live_time'] ?? 0)) ?></p>
                                </div>
                                <div>
                                    <p class="text-muted text-sm">Account Status</p>
                                    <p class="font-semibold">
                                        <?php if ($user['ban']): ?>
                                        <span class="badge badge-danger">Banned</span>
                                        <?php else: ?>
                                        <span class="badge badge-success">Active</span>
                                        <?php endif; ?>
                                    </p>
                                </div>
                            </div>
                            
                            <div class="mt-6 pt-6 border-t border-light">
                                <a href="/settings.php" class="btn btn-ghost">Account Settings</a>
                                <a href="/change-password.php" class="btn btn-ghost">Change Password</a>
                            </div>
                        </div>
                    </div>
                </main>
            </div>
        </div>
    </section>

    <script src="<?= asset('js/main.js') ?>"></script>
</body>
</html>
