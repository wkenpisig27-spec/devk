<?php
/**
 * PKO Website - Item Shop Page
 */

require_once __DIR__ . '/includes/config.php';

$user = Auth::user();
$userId = Auth::id();
$characters = $userId ? CharacterModel::getByAccountId($userId) : [];

// Get user credits
$credits = 0;
if ($userId) {
    try {
        $gameDb = Database::getGameDb();
        $stmt = $gameDb->prepare("SELECT credit FROM account WHERE act_id = ?");
        $stmt->execute([$userId]);
        $gameAccount = $stmt->fetch();
        $credits = (int)($gameAccount['credit'] ?? 0);
    } catch (Exception $e) {
        error_log("Failed to get credits: " . $e->getMessage());
    }
}

// Get shop items
$category = $_GET['category'] ?? null;
$categories = ShopModel::getCategories();
$items = ShopModel::getItems($category);

Security::setHeaders();
$stats = getServerStats();
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <?= metaTags('Item Shop', 'Browse and purchase items for your ' . SERVER_NAME . ' characters using credits.') ?>
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
                    <li><a href="/vote.php" class="navbar-link">Vote</a></li>
                    <li><a href="/shop.php" class="navbar-link active">Shop</a></li>
                    <li><a href="/news-list.php" class="navbar-link">News</a></li>
                    <li><a href="/donate.php" class="navbar-link">Item Mall</a></li>
                </ul>
                
                <div class="navbar-actions">
                    <?php if ($user): ?>
                        <span class="text-gold font-medium" data-credits><?= formatNumber($credits) ?> CP</span>
                        <a href="/dashboard.php" class="btn btn-ghost">Dashboard</a>
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
                <h1 class="title-section text-center">Item Shop</h1>
                <p class="text-secondary text-lg">Purchase items and gear for your characters</p>
                <?php if ($user): ?>
                <p class="text-gold text-xl font-semibold mt-2">Your Balance: <?= formatNumber($credits) ?> Credits</p>
                <?php endif; ?>
            </div>

            <?php if (!$user): ?>
            <div class="alert alert-info mb-8" style="max-width: 600px; margin: 0 auto;">
                <svg class="alert-icon" fill="currentColor" viewBox="0 0 20 20">
                    <path fill-rule="evenodd" d="M18 10a8 8 0 11-16 0 8 8 0 0116 0zm-7-4a1 1 0 11-2 0 1 1 0 012 0zM9 9a1 1 0 000 2v3a1 1 0 001 1h1a1 1 0 100-2v-3a1 1 0 00-1-1H9z" clip-rule="evenodd"/>
                </svg>
                <div class="alert-content">
                    <div class="alert-title">Login Required</div>
                    <p>Please <a href="/login.php?redirect=/shop.php" class="font-semibold">log in</a> to purchase items.</p>
                </div>
            </div>
            <?php elseif (empty($characters)): ?>
            <div class="alert alert-warning mb-8" style="max-width: 600px; margin: 0 auto;">
                <svg class="alert-icon" fill="currentColor" viewBox="0 0 20 20">
                    <path fill-rule="evenodd" d="M8.257 3.099c.765-1.36 2.722-1.36 3.486 0l5.58 9.92c.75 1.334-.213 2.98-1.742 2.98H4.42c-1.53 0-2.493-1.646-1.743-2.98l5.58-9.92zM11 13a1 1 0 11-2 0 1 1 0 012 0zm-1-8a1 1 0 00-1 1v3a1 1 0 002 0V6a1 1 0 00-1-1z" clip-rule="evenodd"/>
                </svg>
                <div class="alert-content">
                    <div class="alert-title">No Characters</div>
                    <p>You need to create a character in-game before you can purchase items.</p>
                </div>
            </div>
            <?php else: ?>
            <!-- Character Selector -->
            <div class="card mb-6" style="max-width: 600px; margin: 0 auto;">
                <h3 class="font-semibold mb-4">Select Character to Receive Items</h3>
                <input type="hidden" name="character_id" value="<?= $characters[0]['cha_id'] ?? '' ?>">
                <div class="grid grid-cols-3 gap-3" data-character-selector>
                    <?php foreach ($characters as $index => $char): ?>
                    <div 
                        class="card p-3 text-center cursor-pointer transition <?= $index === 0 ? 'selected' : '' ?>" 
                        data-character-id="<?= $char['cha_id'] ?>"
                        style="<?= $index === 0 ? 'border-color: var(--gold-400);' : '' ?>"
                    >
                        <img 
                            src="<?= asset('images/classes/' . strtolower(str_replace(' ', '_', $char['job'] ?: 'newbie')) . '.svg') ?>" 
                            alt="" 
                            class="rounded-lg mb-2" 
                            style="width: 40px; height: 40px; margin: 0 auto;"
                            onerror="this.src='<?= asset('images/classes/newbie.svg') ?>'"
                        >
                        <div class="font-semibold text-sm"><?= e($char['cha_name']) ?></div>
                        <div class="text-xs text-muted">Lv.<?= $char['degree'] ?></div>
                    </div>
                    <?php endforeach; ?>
                </div>
            </div>
            <?php endif; ?>

            <!-- Category Filter -->
            <?php if (!empty($categories)): ?>
            <div class="flex justify-center gap-2 mb-6 flex-wrap">
                <a href="/shop.php" class="btn <?= !$category ? 'btn-primary' : 'btn-ghost' ?>">All</a>
                <?php foreach ($categories as $cat): ?>
                <a href="/shop.php?category=<?= urlencode($cat) ?>" class="btn <?= $category === $cat ? 'btn-primary' : 'btn-ghost' ?>">
                    <?= e(ucfirst(trim($cat))) ?>
                </a>
                <?php endforeach; ?>
            </div>
            <?php endif; ?>

            <!-- Shop Items -->
            <?php if (empty($items)): ?>
            <div class="card text-center p-8">
                <div class="stat-icon mb-4" style="margin: 0 auto;">
                    <svg width="24" height="24" fill="currentColor" viewBox="0 0 20 20">
                        <path d="M3 1a1 1 0 000 2h1.22l.305 1.222a.997.997 0 00.01.042l1.358 5.43-.893.892C3.74 11.846 4.632 14 6.414 14H15a1 1 0 000-2H6.414l1-1H14a1 1 0 00.894-.553l3-6A1 1 0 0017 3H6.28l-.31-1.243A1 1 0 005 1H3z"/>
                    </svg>
                </div>
                <h3 class="text-xl font-semibold mb-2">No Items Available</h3>
                <p class="text-muted">Check back later for new items!</p>
            </div>
            <?php else: ?>
            <div class="grid grid-cols-4 gap-4">
                <?php foreach ($items as $item): ?>
                <?php 
                    $stock = $item['Quota'] - $item['bought'];
                    $canAfford = $credits >= $item['TavaraHinta'];
                ?>
                <div class="card">
                    <div class="text-center mb-4">
                        <?php if ($item['Icon']): ?>
                        <img 
                            src="<?= asset('images/items/' . trim($item['Icon']) . '.png') ?>" 
                            alt="<?= e(trim($item['TuoNimi'])) ?>" 
                            class="rounded-lg mb-2" 
                            style="width: 48px; height: 48px; margin: 0 auto;"
                            onerror="this.style.display='none'"
                        >
                        <?php endif; ?>
                        <h3 class="font-semibold"><?= e(trim($item['TuoNimi'])) ?></h3>
                        <?php if ($item['MontaTavaraa'] > 1): ?>
                        <span class="badge badge-gold">x<?= $item['MontaTavaraa'] ?></span>
                        <?php endif; ?>
                    </div>
                    
                    <?php if ($item['TavaraTeksti']): ?>
                    <p class="text-sm text-muted mb-4"><?= e(truncate(trim($item['TavaraTeksti']), 100)) ?></p>
                    <?php endif; ?>
                    
                    <div class="flex justify-between items-center mb-4">
                        <span class="text-gold font-bold text-lg"><?= formatNumber($item['TavaraHinta']) ?> CP</span>
                        <span class="text-muted text-sm"><?= $stock ?> left</span>
                    </div>
                    
                    <?php if (!$user): ?>
                    <a href="/login.php?redirect=/shop.php" class="btn btn-secondary btn-sm w-full">Login to Buy</a>
                    <?php elseif (empty($characters)): ?>
                    <button class="btn btn-ghost btn-sm w-full" disabled>No Character</button>
                    <?php elseif (!$canAfford): ?>
                    <button class="btn btn-ghost btn-sm w-full" disabled>Not Enough Credits</button>
                    <?php elseif ($stock <= 0): ?>
                    <button class="btn btn-ghost btn-sm w-full" disabled>Out of Stock</button>
                    <?php else: ?>
                    <button 
                        class="btn btn-primary btn-sm w-full" 
                        onclick="handlePurchase(<?= $item['TavaraListaID'] ?>)"
                    >
                        Buy Now
                    </button>
                    <?php endif; ?>
                </div>
                <?php endforeach; ?>
            </div>
            <?php endif; ?>
            
            <!-- How to get credits -->
            <div class="mt-12 text-center">
                <h2 class="text-xl font-semibold mb-4">Need More Credits?</h2>
                <p class="text-muted mb-4">Earn free credits by voting for our server!</p>
                <a href="/vote.php" class="btn btn-gold">Vote for Credits</a>
            </div>
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
    <style>
        .card.selected, .card[data-character-id]:hover {
            border-color: var(--gold-400) !important;
        }
    </style>
</body>
</html>
