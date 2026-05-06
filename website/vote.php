<?php
/**
 * PKO Website - Vote for Rewards Page
 */

require_once __DIR__ . '/includes/config.php';

if (!VOTING_ENABLED) {
    redirect('/');
}

$user = Auth::user();
$userId = Auth::id();
$voteSites = VoteModel::getSites();

Security::setHeaders();
$stats = getServerStats();
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <?= metaTags('Vote for Rewards', 'Vote for ' . SERVER_NAME . ' and earn free credits to spend in the item shop!') ?>
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
                    <li><a href="/leaderboard.php" class="navbar-link">Leaderboard</a></li>
                    <li><a href="/download.php" class="navbar-link">Download</a></li>
                    <li><a href="/vote.php" class="navbar-link active">Vote</a></li>
                    <li><a href="/shop.php" class="navbar-link">Shop</a></li>
                    <li><a href="/news-list.php" class="navbar-link">News</a></li>
                    <li><a href="/donate.php" class="navbar-link">Item Mall</a></li>
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
                <h1 class="title-section text-center">Vote for Rewards</h1>
                <p class="text-secondary text-lg">Vote for us on these sites and earn free credits!</p>
            </div>

            <?php if (!$user): ?>
            <div class="alert alert-info mb-8" style="max-width: 600px; margin: 0 auto;">
                <svg class="alert-icon" fill="currentColor" viewBox="0 0 20 20">
                    <path fill-rule="evenodd" d="M18 10a8 8 0 11-16 0 8 8 0 0116 0zm-7-4a1 1 0 11-2 0 1 1 0 012 0zM9 9a1 1 0 000 2v3a1 1 0 001 1h1a1 1 0 100-2v-3a1 1 0 00-1-1H9z" clip-rule="evenodd"/>
                </svg>
                <div class="alert-content">
                    <div class="alert-title">Login Required</div>
                    <p>Please <a href="/login.php?redirect=/vote.php" class="font-semibold">log in</a> to vote and receive your rewards.</p>
                </div>
            </div>
            <?php endif; ?>

            <!-- How it works -->
            <div class="card mb-8" style="max-width: 800px; margin: 0 auto;">
                <h2 class="text-xl font-semibold mb-4">How It Works</h2>
                <div class="grid grid-cols-3 gap-6 text-center">
                    <div>
                        <div class="stat-icon mb-3" style="margin: 0 auto;">
                            <span class="text-xl">1</span>
                        </div>
                        <h3 class="font-semibold mb-1">Click Vote</h3>
                        <p class="text-muted text-sm">Click the vote button below</p>
                    </div>
                    <div>
                        <div class="stat-icon mb-3" style="margin: 0 auto;">
                            <span class="text-xl">2</span>
                        </div>
                        <h3 class="font-semibold mb-1">Complete Vote</h3>
                        <p class="text-muted text-sm">Vote on the external site</p>
                    </div>
                    <div>
                        <div class="stat-icon mb-3" style="margin: 0 auto;">
                            <span class="text-xl">3</span>
                        </div>
                        <h3 class="font-semibold mb-1">Get Rewards</h3>
                        <p class="text-muted text-sm">Credits added to your account!</p>
                    </div>
                </div>
            </div>

            <!-- Vote Sites -->
            <?php if (empty($voteSites)): ?>
            <div class="card text-center p-8">
                <div class="stat-icon mb-4" style="margin: 0 auto;">
                    <svg width="24" height="24" fill="currentColor" viewBox="0 0 20 20">
                        <path d="M9.049 2.927c.3-.921 1.603-.921 1.902 0l1.07 3.292a1 1 0 00.95.69h3.462c.969 0 1.371 1.24.588 1.81l-2.8 2.034a1 1 0 00-.364 1.118l1.07 3.292c.3.921-.755 1.688-1.54 1.118l-2.8-2.034a1 1 0 00-1.175 0l-2.8 2.034c-.784.57-1.838-.197-1.539-1.118l1.07-3.292a1 1 0 00-.364-1.118L2.98 8.72c-.783-.57-.38-1.81.588-1.81h3.461a1 1 0 00.951-.69l1.07-3.292z"/>
                    </svg>
                </div>
                <h3 class="text-xl font-semibold mb-2">No Vote Sites Available</h3>
                <p class="text-muted">Check back later for voting opportunities!</p>
            </div>
            <?php else: ?>
            <div class="grid grid-cols-2 gap-6" style="max-width: 900px; margin: 0 auto;">
                <?php foreach ($voteSites as $site): ?>
                <?php 
                    $canVote = $user ? VoteModel::canVote($userId, $site['id'], Security::getClientIp()) : false;
                ?>
                <div class="card">
                    <div class="flex items-center gap-4 mb-4">
                        <?php if ($site['image']): ?>
                        <img src="<?= e($site['image']) ?>" alt="<?= e($site['name']) ?>" class="rounded-lg" style="width: 60px; height: 60px; object-fit: cover;">
                        <?php else: ?>
                        <div class="rounded-lg flex items-center justify-center" style="width: 60px; height: 60px; background: var(--bg-tertiary);">
                            <svg width="24" height="24" fill="currentColor" viewBox="0 0 20 20" class="text-gold">
                                <path d="M9.049 2.927c.3-.921 1.603-.921 1.902 0l1.07 3.292a1 1 0 00.95.69h3.462c.969 0 1.371 1.24.588 1.81l-2.8 2.034a1 1 0 00-.364 1.118l1.07 3.292c.3.921-.755 1.688-1.54 1.118l-2.8-2.034a1 1 0 00-1.175 0l-2.8 2.034c-.784.57-1.838-.197-1.539-1.118l1.07-3.292a1 1 0 00-.364-1.118L2.98 8.72c-.783-.57-.38-1.81.588-1.81h3.461a1 1 0 00.951-.69l1.07-3.292z"/>
                            </svg>
                        </div>
                        <?php endif; ?>
                        <div class="flex-1">
                            <h3 class="font-semibold"><?= e(trim($site['name'])) ?></h3>
                            <p class="text-gold font-semibold">+<?= $site['prize'] ?> Credits</p>
                        </div>
                    </div>
                    
                    <?php if (!$user): ?>
                    <a href="/login.php?redirect=/vote.php" class="btn btn-secondary w-full">Login to Vote</a>
                    <?php elseif ($canVote): ?>
                    <button 
                        class="btn btn-primary w-full" 
                        data-vote-site="<?= $site['id'] ?>"
                        onclick="handleVote(<?= $site['id'] ?>)"
                    >
                        Vote Now
                    </button>
                    <?php else: ?>
                    <button class="btn btn-ghost w-full" disabled>
                        Please Wait...
                    </button>
                    <p class="text-center text-muted text-xs mt-2">Cooldown: <?= VOTE_COOLDOWN_HOURS ?>h between votes</p>
                    <?php endif; ?>
                </div>
                <?php endforeach; ?>
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
