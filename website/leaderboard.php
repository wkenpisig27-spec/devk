<?php
/**
 * PKO Website - Leaderboard Page
 */

require_once __DIR__ . '/includes/config.php';

$tab = $_GET['tab'] ?? 'players';
$page = max(1, (int)($_GET['page'] ?? 1));
$perPage = 20;

// Get data based on tab with error handling
$topPlayers = [];
$topGuilds = [];
$topEarners = [];

try {
    switch ($tab) {
        case 'guilds':
            $topGuilds = GuildModel::getTopByMembers(50);
            break;
        case 'earners':
            $topEarners = CharacterModel::getTopByGold(50);
            break;
        default:
            $tab = 'players';
            $topPlayers = CharacterModel::getTopByLevel(50);
            break;
    }
} catch (Exception $e) {
    error_log("Leaderboard page DB error: " . $e->getMessage());
}

Security::setHeaders();
$stats = getServerStats();
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <?= metaTags('Leaderboard', 'View the top players, guilds, and PvP rankings in ' . SERVER_NAME) ?>
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
                    <?php if (Auth::check()): ?>
                        <a href="/dashboard.php" class="btn btn-ghost">Dashboard</a>
                        <a href="#" class="btn btn-primary" data-logout>Logout</a>
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

    <div style="padding-top: 100px;"></div>

    <section class="section">
        <div class="container">
            <div class="text-center mb-8">
                <h1 class="title-section text-center">Leaderboard</h1>
                <p class="text-secondary text-lg">See who rules the seven seas</p>
            </div>

            <!-- Tabs -->
            <div class="flex justify-center gap-2 mb-8">
                <a href="?tab=players" class="btn <?= $tab === 'players' ? 'btn-primary' : 'btn-ghost' ?>">
                    🏆 Top Players
                </a>
                <a href="?tab=earners" class="btn <?= $tab === 'earners' ? 'btn-primary' : 'btn-ghost' ?>">
                    💰 Top Earners
                </a>
                <a href="?tab=guilds" class="btn <?= $tab === 'guilds' ? 'btn-primary' : 'btn-ghost' ?>">
                    🏴 Top Guilds
                </a>
            </div>

            <!-- Players Tab -->
            <?php if ($tab === 'players'): ?>
            <div class="card">
                <div class="table-container">
                    <table class="table">
                        <thead>
                            <tr>
                                <th style="width: 60px;">Rank</th>
                                <th>Character</th>
                                <th>Class</th>
                                <th>Guild</th>
                                <th style="width: 100px;">Level</th>
                            </tr>
                        </thead>
                        <tbody>
                            <?php foreach ($topPlayers as $index => $player): ?>
                            <tr>
                                <td>
                                    <span class="leaderboard-rank <?= $index === 0 ? 'gold' : ($index === 1 ? 'silver' : ($index === 2 ? 'bronze' : '')) ?>">
                                        <?= $index + 1 ?>
                                    </span>
                                </td>
                                <td>
                                    <div class="flex items-center gap-3">
                                        <img 
                                            src="<?= asset('images/classes/' . strtolower(str_replace(' ', '_', $player['job'] ?: 'newbie')) . '.svg') ?>" 
                                            alt="" 
                                            class="rounded-lg" 
                                            style="width: 32px; height: 32px;"
                                            onerror="this.src='<?= asset('images/classes/newbie.svg') ?>'"
                                        >
                                        <span class="font-semibold"><?= e($player['cha_name']) ?></span>
                                    </div>
                                </td>
                                <td class="text-muted"><?= e(getClassName($player['job'])) ?></td>
                                <td class="text-muted"><?= e($player['guild_name'] ?? '-') ?></td>
                                <td class="font-semibold text-gold"><?= $player['degree'] ?></td>
                            </tr>
                            <?php endforeach; ?>
                            <?php if (empty($topPlayers)): ?>
                            <tr>
                                <td colspan="5" class="text-center text-muted">No players found</td>
                            </tr>
                            <?php endif; ?>
                        </tbody>
                    </table>
                </div>
            </div>
            <?php endif; ?>

            <!-- Earners Tab -->
            <?php if ($tab === 'earners'): ?>
            <div class="card">
                <div class="table-container">
                    <table class="table">
                        <thead>
                            <tr>
                                <th style="width: 60px;">Rank</th>
                                <th>Character</th>
                                <th>Class</th>
                                <th>Guild</th>
                                <th style="width: 100px;">Level</th>
                                <th style="width: 140px;">Gold</th>
                            </tr>
                        </thead>
                        <tbody>
                            <?php foreach ($topEarners as $index => $player): ?>
                            <tr>
                                <td>
                                    <span class="leaderboard-rank <?= $index === 0 ? 'gold' : ($index === 1 ? 'silver' : ($index === 2 ? 'bronze' : '')) ?>">
                                        <?= $index + 1 ?>
                                    </span>
                                </td>
                                <td>
                                    <div class="flex items-center gap-3">
                                        <img 
                                            src="<?= asset('images/classes/' . strtolower(str_replace(' ', '_', $player['job'] ?: 'newbie')) . '.svg') ?>" 
                                            alt="" 
                                            class="rounded-lg" 
                                            style="width: 32px; height: 32px;"
                                            onerror="this.src='<?= asset('images/classes/newbie.svg') ?>'"
                                        >
                                        <span class="font-semibold"><?= e($player['cha_name']) ?></span>
                                    </div>
                                </td>
                                <td class="text-muted"><?= e(getClassName($player['job'])) ?></td>
                                <td class="text-muted"><?= e($player['guild_name'] ?? '-') ?></td>
                                <td class="text-gold"><?= $player['degree'] ?></td>
                                <td class="font-semibold text-gold"><?= formatNumber($player['gd']) ?></td>
                            </tr>
                            <?php endforeach; ?>
                            <?php if (empty($topEarners)): ?>
                            <tr>
                                <td colspan="6" class="text-center text-muted">No earner data found</td>
                            </tr>
                            <?php endif; ?>
                        </tbody>
                    </table>
                </div>
            </div>
            <?php endif; ?>

            <!-- Guilds Tab -->
            <?php if ($tab === 'guilds'): ?>
            <div class="card">
                <div class="table-container">
                    <table class="table">
                        <thead>
                            <tr>
                                <th style="width: 60px;">Rank</th>
                                <th>Guild</th>
                                <th>Leader</th>
                                <th style="width: 100px;">Members</th>
                                <th style="width: 100px;">Level</th>
                                <th style="width: 120px;">Experience</th>
                            </tr>
                        </thead>
                        <tbody>
                            <?php foreach ($topGuilds as $index => $guild): ?>
                            <tr>
                                <td>
                                    <span class="leaderboard-rank <?= $index === 0 ? 'gold' : ($index === 1 ? 'silver' : ($index === 2 ? 'bronze' : '')) ?>">
                                        <?= $index + 1 ?>
                                    </span>
                                </td>
                                <td>
                                    <div class="flex items-center gap-3">
                                        <div class="rounded-lg flex items-center justify-center text-gold font-bold" style="width: 32px; height: 32px; background: var(--bg-tertiary);">
                                            <?= strtoupper(substr($guild['guild_name'], 0, 2)) ?>
                                        </div>
                                        <div>
                                            <span class="font-semibold"><?= e($guild['guild_name']) ?></span>
                                            <?php if ($guild['motto']): ?>
                                            <div class="text-xs text-muted"><?= e(truncate($guild['motto'], 40)) ?></div>
                                            <?php endif; ?>
                                        </div>
                                    </div>
                                </td>
                                <td class="text-muted"><?= e($guild['leader_name'] ?? 'Unknown') ?></td>
                                <td class="font-semibold text-gold"><?= $guild['member_total'] ?></td>
                                <td class="text-muted"><?= $guild['level'] ?></td>
                                <td class="text-muted"><?= formatNumber($guild['exp']) ?></td>
                            </tr>
                            <?php endforeach; ?>
                            <?php if (empty($topGuilds)): ?>
                            <tr>
                                <td colspan="6" class="text-center text-muted">No guilds found</td>
                            </tr>
                            <?php endif; ?>
                        </tbody>
                    </table>
                </div>
            </div>
            <?php endif; ?>
        </div>
    </section>

    <!-- Footer -->
    <footer class="footer">
        <div class="container">
            <div class="footer-grid">
                <div class="footer-brand">
                    <a href="/" class="footer-logo"><?= e(SERVER_NAME) ?></a>
                    <p class="footer-description">
                        Embark on the ultimate pirate MMORPG adventure. Sail the seas, battle enemies, and become a legend!
                    </p>
                    <div class="footer-social">
                        <a href="https://discord.com/invite/NHmFsuMpS5" target="_blank" aria-label="Discord">
                            <svg width="20" height="20" fill="currentColor" viewBox="0 0 24 24"><path d="M20.317 4.37a19.791 19.791 0 0 0-4.885-1.515.074.074 0 0 0-.079.037c-.21.375-.444.864-.608 1.25a18.27 18.27 0 0 0-5.487 0 12.64 12.64 0 0 0-.617-1.25.077.077 0 0 0-.079-.037A19.736 19.736 0 0 0 3.677 4.37a.07.07 0 0 0-.032.027C.533 9.046-.32 13.58.099 18.057a.082.082 0 0 0 .031.057 19.9 19.9 0 0 0 5.993 3.03.078.078 0 0 0 .084-.028 14.09 14.09 0 0 0 1.226-1.994.076.076 0 0 0-.041-.106 13.107 13.107 0 0 1-1.872-.892.077.077 0 0 1-.008-.128 10.2 10.2 0 0 0 .372-.292.074.074 0 0 1 .077-.01c3.928 1.793 8.18 1.793 12.062 0a.074.074 0 0 1 .078.01c.12.098.246.198.373.292a.077.077 0 0 1-.006.127 12.299 12.299 0 0 1-1.873.892.077.077 0 0 0-.041.107c.36.698.772 1.362 1.225 1.993a.076.076 0 0 0 .084.028 19.839 19.839 0 0 0 6.002-3.03.077.077 0 0 0 .032-.054c.5-5.177-.838-9.674-3.549-13.66a.061.061 0 0 0-.031-.03zM8.02 15.33c-1.183 0-2.157-1.085-2.157-2.419 0-1.333.956-2.419 2.157-2.419 1.21 0 2.176 1.096 2.157 2.42 0 1.333-.956 2.418-2.157 2.418zm7.975 0c-1.183 0-2.157-1.085-2.157-2.419 0-1.333.955-2.419 2.157-2.419 1.21 0 2.176 1.096 2.157 2.42 0 1.333-.946 2.418-2.157 2.418z"/></svg>
                        </a>
                        <a href="https://www.facebook.com/share/1AL6yMoj2g/?mibextid=wwXIfr" target="_blank" aria-label="Facebook">
                            <svg width="20" height="20" fill="currentColor" viewBox="0 0 24 24"><path d="M24 12.073c0-6.627-5.373-12-12-12s-12 5.373-12 12c0 5.99 4.388 10.954 10.125 11.854v-8.385H7.078v-3.47h3.047V9.43c0-3.007 1.792-4.669 4.533-4.669 1.312 0 2.686.235 2.686.235v2.953H15.83c-1.491 0-1.956.925-1.956 1.874v2.25h3.328l-.532 3.47h-2.796v8.385C19.612 23.027 24 18.062 24 12.073z"/></svg>
                        </a>
                        <a href="https://www.instagram.com/slime.piratesonline" target="_blank" aria-label="Instagram">
                            <svg width="20" height="20" fill="currentColor" viewBox="0 0 24 24"><path d="M12 2.163c3.204 0 3.584.012 4.85.07 3.252.148 4.771 1.691 4.919 4.919.058 1.265.069 1.645.069 4.849 0 3.205-.012 3.584-.069 4.849-.149 3.225-1.664 4.771-4.919 4.919-1.266.058-1.644.07-4.85.07-3.204 0-3.584-.012-4.849-.07-3.26-.149-4.771-1.699-4.919-4.92-.058-1.265-.07-1.644-.07-4.849 0-3.204.013-3.583.07-4.849.149-3.227 1.664-4.771 4.919-4.919 1.266-.057 1.645-.069 4.849-.069zM12 0C8.741 0 8.333.014 7.053.072 2.695.272.273 2.69.073 7.052.014 8.333 0 8.741 0 12c0 3.259.014 3.668.072 4.948.2 4.358 2.618 6.78 6.98 6.98C8.333 23.986 8.741 24 12 24c3.259 0 3.668-.014 4.948-.072 4.354-.2 6.782-2.618 6.979-6.98.059-1.28.073-1.689.073-4.948 0-3.259-.014-3.667-.072-4.947-.196-4.354-2.617-6.78-6.979-6.98C15.668.014 15.259 0 12 0zm0 5.838a6.162 6.162 0 100 12.324 6.162 6.162 0 000-12.324zM12 16a4 4 0 110-8 4 4 0 010 8zm6.406-11.845a1.44 1.44 0 100 2.881 1.44 1.44 0 000-2.881z"/></svg>
                        </a>
                        <a href="https://www.tiktok.com/@slime.pirates.onl" target="_blank" aria-label="TikTok">
                            <svg width="20" height="20" fill="currentColor" viewBox="0 0 24 24"><path d="M19.59 6.69a4.83 4.83 0 01-3.77-4.25V2h-3.45v13.67a2.89 2.89 0 01-5.2 1.74 2.89 2.89 0 012.31-4.64 2.93 2.93 0 01.88.13V9.4a6.84 6.84 0 00-1-.05A6.33 6.33 0 005 20.1a6.34 6.34 0 0010.86-4.43v-7a8.16 8.16 0 004.77 1.52v-3.4a4.85 4.85 0 01-1-.1z"/></svg>
                        </a>
                        <a href="https://youtube.com/@slimepiratesonline?si=ksWNk6Fe6ZSTx5nt" target="_blank" aria-label="YouTube">
                            <svg width="20" height="20" fill="currentColor" viewBox="0 0 24 24"><path d="M23.498 6.186a3.016 3.016 0 0 0-2.122-2.136C19.505 3.545 12 3.545 12 3.545s-7.505 0-9.377.505A3.017 3.017 0 0 0 .502 6.186C0 8.07 0 12 0 12s0 3.93.502 5.814a3.016 3.016 0 0 0 2.122 2.136c1.871.505 9.376.505 9.376.505s7.505 0 9.377-.505a3.015 3.015 0 0 0 2.122-2.136C24 15.93 24 12 24 12s0-3.93-.502-5.814zM9.545 15.568V8.432L15.818 12l-6.273 3.568z"/></svg>
                        </a>
                    </div>
                </div>
                
                <div>
                    <h4 class="footer-title">Game</h4>
                    <ul class="footer-links">
                        <li><a href="/download.php">Download</a></li>
                        <li><a href="/register.php">Register</a></li>
                        <li><a href="/leaderboard.php">Leaderboard</a></li>
                    </ul>
                </div>
                
                <div>
                    <h4 class="footer-title">Community</h4>
                    <ul class="footer-links">
                        <li><a href="https://discord.com/invite/NHmFsuMpS5" target="_blank">Discord</a></li>
                        <li><a href="https://discord.com/invite/NHmFsuMpS5" target="_blank">Support</a></li>
                    </ul>
                </div>
                
                <div>
                    <h4 class="footer-title">Legal</h4>
                    <ul class="footer-links">
                        <li><a href="/terms.php">Terms of Service</a></li>
                        <li><a href="/privacy.php">Privacy Policy</a></li>
                        <li><a href="/rules.php">Game Rules</a></li>
                        <li><a href="/refund.php">Refund Policy</a></li>
                        <li><a href="/donate.php">Item Mall</a></li>
                    </ul>
                </div>
            </div>
            
            <div class="footer-bottom">
                <p>&copy; <?= date('Y') ?> <?= e(SERVER_NAME) ?>. All rights reserved.</p>
            </div>
        </div>
    </footer>

    <script src="<?= asset('js/main.js') ?>"></script>
</body>
</html>
