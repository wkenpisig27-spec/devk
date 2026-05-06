<?php
require_once __DIR__ . '/../includes/config.php';

// Require login
Auth::require();

// Check admin access (GM level 99)
$currentUser = Auth::user();
$gmLevel = AccountModel::getGmLevel(Auth::id());
if ($gmLevel < 99) {
    flash('error', 'Access denied. You do not have permission to view this page.');
    redirect('/dashboard.php');
}

$pageTitle = 'Admin Dashboard';
require_once __DIR__ . '/../includes/header.php';

// Get Stats
$onlineCount = AccountModel::getOnlineCount();
$totalAccounts = AccountModel::getTotalAccounts();
$totalChars = CharacterModel::getTotalCharacters();
$totalGuilds = GuildModel::getTotalGuilds();
?>

<div class="container py-5">
    <div class="row mb-4">
        <div class="col-12">
            <h1 class="display-4 text-gradient">Admin Dashboard</h1>
            <p class="lead text-muted">Server Management & Statistics</p>
        </div>
    </div>

    <!-- Stats Cards -->
    <div class="grid grid-cols-4 gap-4 mb-5">
        <div class="glass-panel p-4 text-center h-100">
            <div class="text-warning mb-2"><i class="fas fa-users fa-2x"></i></div>
            <h3 class="fw-bold mb-0"><?= number_format($onlineCount) ?></h3>
            <div class="text-muted small">Players Online</div>
        </div>
        <div class="glass-panel p-4 text-center h-100">
            <div class="text-info mb-2"><i class="fas fa-id-card fa-2x"></i></div>
            <h3 class="fw-bold mb-0"><?= number_format($totalAccounts) ?></h3>
            <div class="text-muted small">Total Accounts</div>
        </div>
        <div class="glass-panel p-4 text-center h-100">
            <div class="text-success mb-2"><i class="fas fa-user-shield fa-2x"></i></div>
            <h3 class="fw-bold mb-0"><?= number_format($totalChars) ?></h3>
            <div class="text-muted small">Characters Created</div>
        </div>
        <div class="glass-panel p-4 text-center h-100">
            <div class="text-danger mb-2"><i class="fas fa-flag fa-2x"></i></div>
            <h3 class="fw-bold mb-0"><?= number_format($totalGuilds) ?></h3>
            <div class="text-muted small">Guilds Established</div>
        </div>
    </div>

    <!-- Quick Actions -->
    <div class="grid grid-cols-2 gap-4">
        <div class="glass-panel p-4 h-100">
            <h4 class="mb-4"><i class="fas fa-cogs me-2"></i>Server Actions</h4>
            <div class="d-grid gap-3">
                <a href="/admin/news.php" class="btn btn-primary w-100 mb-2">
                    <i class="fas fa-newspaper me-2"></i>Manage News
                </a>
                <button class="btn btn-outline-warning w-100 mb-2" onclick="alert('Feature coming soon!')">
                    <i class="fas fa-gift me-2"></i>Send Global Gift
                </button>
                <a href="/" class="btn btn-outline-light w-100">
                    <i class="fas fa-home me-2"></i>Back to Website
                </a>
            </div>
        </div>
        
        <div class="glass-panel p-4 h-100">
            <h4 class="mb-4"><i class="fas fa-server me-2"></i>System Status</h4>
            <div class="d-flex justify-content-between align-items-center mb-3">
                <span>Web Server</span>
                <span class="badge bg-success">Online</span>
            </div>
            <div class="d-flex justify-content-between align-items-center mb-3">
                <span>Database Connection</span>
                <span class="badge bg-success">Connected</span>
            </div>
            <div class="d-flex justify-content-between align-items-center">
                <span>Game Server</span>
                <?php if (ServerStatus::check()): ?>
                    <span class="badge bg-success">Online</span>
                <?php else: ?>
                    <span class="badge bg-danger">Offline</span>
                <?php endif; ?>
            </div>
        </div>
    </div>
</div>

<?php require_once __DIR__ . '/../includes/footer.php'; ?>
