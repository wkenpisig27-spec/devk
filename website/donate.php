<?php
/**
 * PKO Website - Donation Page
 */

require_once __DIR__ . '/includes/config.php';

Security::setHeaders();

// Initialize session and check auth before any output
$isLoggedIn = Auth::check();
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <?= metaTags('Donate', 'Support ' . SERVER_NAME . ' by donating - Get Donation Coins!') ?>
    <link rel="stylesheet" href="<?= asset('css/style.css') ?>">
    <link rel="icon" type="image/png" href="<?= asset('images/favicon.png') ?>">
    <style>
        .donation-section {
            margin-bottom: 2rem;
        }
        .donation-header {
            display: flex;
            align-items: center;
            gap: 12px;
            margin-bottom: 1.5rem;
            padding-bottom: 1rem;
            border-bottom: 2px solid var(--gold);
        }
        .donation-header h2 {
            margin: 0;
            color: var(--gold);
        }
        .donation-icon {
            width: 40px;
            height: 40px;
            display: flex;
            align-items: center;
            justify-content: center;
            background: rgba(212, 175, 55, 0.1);
            border-radius: 8px;
            font-size: 1.5rem;
        }
        .rate-badge {
            display: flex;
            align-items: center;
            gap: 12px;
            background: linear-gradient(135deg, rgba(212, 175, 55, 0.15), rgba(184, 134, 11, 0.1));
            border: 1px solid var(--gold);
            border-left: 4px solid var(--gold);
            color: #fff;
            padding: 16px 20px;
            border-radius: 8px;
            font-weight: 600;
            font-size: 1.1rem;
            margin-bottom: 1.5rem;
        }
        .rate-badge::before {
            content: "💰";
            font-size: 1.5rem;
        }
        .step-list {
            list-style: none;
            padding: 0;
            margin: 0;
        }
        .step-item {
            display: flex;
            gap: 16px;
            margin-bottom: 1rem;
            padding: 1rem;
            background: rgba(255, 255, 255, 0.02);
            border-radius: 8px;
            border-left: 3px solid var(--gold);
            color: #fff;
        }
        .step-number {
            flex-shrink: 0;
            width: 32px;
            height: 32px;
            display: flex;
            align-items: center;
            justify-content: center;
            background: var(--gold);
            color: #fff;
            border-radius: 50%;
            font-weight: 700;
            font-size: 0.9rem;
        }
        .step-content {
            flex: 1;
            color: #fff;
        }
        .step-content strong {
            color: #fff;
        }
        .wallet-address {
            background: rgba(0, 0, 0, 0.3);
            border: 1px solid rgba(212, 175, 55, 0.3);
            border-radius: 8px;
            padding: 12px 16px;
            margin: 1rem 0;
            font-family: monospace;
            font-size: 0.95rem;
            word-break: break-all;
            color: var(--gold);
        }
        .after-payment {
            background: rgba(76, 175, 80, 0.1);
            border: 1px solid rgba(76, 175, 80, 0.3);
            border-radius: 8px;
            padding: 1.5rem;
            margin-top: 1.5rem;
        }
        .after-payment h3 {
            color: #4CAF50;
            margin-top: 0;
            display: flex;
            align-items: center;
            gap: 8px;
        }
        .important-notice {
            background: rgba(244, 67, 54, 0.1);
            border: 1px solid rgba(244, 67, 54, 0.3);
            border-radius: 8px;
            padding: 1.5rem;
            margin-top: 1.5rem;
        }
        .important-notice h3 {
            color: #f44336;
            margin-top: 0;
            display: flex;
            align-items: center;
            gap: 8px;
        }
        .important-notice ul {
            margin-bottom: 0;
        }
        .payment-tabs {
            display: flex;
            gap: 8px;
            margin-bottom: 2rem;
            flex-wrap: wrap;
        }
        .payment-tab {
            padding: 12px 24px;
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 8px;
            cursor: pointer;
            transition: all 0.3s ease;
            color: var(--text-muted);
            font-weight: 500;
        }
        .payment-tab:hover {
            background: rgba(212, 175, 55, 0.1);
            border-color: rgba(212, 175, 55, 0.3);
        }
        .payment-tab.active {
            background: rgba(212, 175, 55, 0.2);
            border-color: var(--gold);
            color: var(--gold);
        }
        .payment-content {
            display: none;
        }
        .payment-content.active {
            display: block;
        }
        .email-link {
            color: var(--gold);
            text-decoration: none;
        }
        .email-link:hover {
            text-decoration: underline;
        }
        
        /* Item Mall Styles */
        .item-category-title {
            color: var(--gold);
            font-size: 1.25rem;
            margin: 2rem 0 1rem;
            padding-bottom: 0.5rem;
            border-bottom: 1px solid rgba(212, 175, 55, 0.3);
        }
        .item-category-title:first-of-type {
            margin-top: 1rem;
        }
        .item-mall-grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
            gap: 1rem;
        }
        .mall-item {
            background: rgba(0, 0, 0, 0.3);
            border: 1px solid rgba(212, 175, 55, 0.2);
            border-radius: 8px;
            padding: 1rem;
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 0.5rem;
            transition: all 0.3s ease;
        }
        .mall-item:hover {
            border-color: var(--gold);
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(212, 175, 55, 0.2);
        }
        .mall-item-rare {
            border-color: rgba(138, 43, 226, 0.4);
            background: linear-gradient(135deg, rgba(138, 43, 226, 0.1) 0%, rgba(0, 0, 0, 0.3) 100%);
        }
        .mall-item-rare:hover {
            border-color: rgb(138, 43, 226);
            box-shadow: 0 4px 12px rgba(138, 43, 226, 0.3);
        }
        .mall-item-legendary {
            border-color: rgba(255, 165, 0, 0.5);
            background: linear-gradient(135deg, rgba(255, 165, 0, 0.15) 0%, rgba(0, 0, 0, 0.3) 100%);
        }
        .mall-item-legendary:hover {
            border-color: rgb(255, 165, 0);
            box-shadow: 0 4px 12px rgba(255, 165, 0, 0.4);
        }
        .mall-item-icon {
            width: 48px;
            height: 48px;
            object-fit: contain;
            image-rendering: pixelated;
        }
        .mall-item-info {
            text-align: center;
            display: flex;
            flex-direction: column;
            gap: 0.25rem;
        }
        .mall-item-name {
            font-size: 0.9rem;
            color: #fff;
            font-weight: 500;
        }
        .mall-item-qty {
            font-size: 0.8rem;
            color: var(--text-muted);
        }
        .mall-item-price {
            background: #d4af37;
            background: var(--gold, #d4af37);
            color: #000;
            padding: 0.35rem 0.85rem;
            border-radius: 4px;
            font-weight: 600;
            font-size: 0.85rem;
            display: inline-block;
        }
        .mall-item-legendary .mall-item-price {
            background: linear-gradient(135deg, #ffd700 0%, #ff8c00 100%);
        }
        .mall-item-rare .mall-item-price {
            background: linear-gradient(135deg, #9f7aea 0%, #805ad5 100%);
            color: #fff;
        }
        
        @media (max-width: 768px) {
            .item-mall-grid {
                grid-template-columns: repeat(2, 1fr);
            }
        }
    </style>
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
                    <?php if ($isLoggedIn): ?>
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
        <div class="container" style="max-width: 900px;">
            <h1 class="title-section mb-6">💎 Support the Server</h1>
            <p class="text-muted mb-8">Your donations help keep the server running and allow us to develop new features. Thank you for your support!</p>
            
            <!-- Payment Method Tabs -->
            <div class="payment-tabs">
                <button class="payment-tab" data-tab="itemmall">🛒 Item Mall</button>
                <button class="payment-tab active" data-tab="gcash">📱 GCash</button>
                <button class="payment-tab" data-tab="paypal">💳 PayPal</button>
                <button class="payment-tab" data-tab="crypto">🪙 Crypto (SOL)</button>
            </div>

            <!-- Item Mall Section -->
            <div class="payment-content" id="itemmall">
                <div class="card p-8">
                    <div class="donation-section">
                        <div class="donation-header">
                            <div class="donation-icon">🛒</div>
                            <h2>Item Mall</h2>
                        </div>
                        
                        <p class="text-muted mb-6">Exchange your Donation Coins for these exclusive items in-game. Contact an admin after donating to receive your items.</p>
                        
                        <!-- Utility Items -->
                        <h3 class="item-category-title">⚙️ Utility Items</h3>
                        <div class="item-mall-grid">
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n0170.png" alt="Inventory Slots Pack" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Inventory Slots Pack</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">1 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n0321.png" alt="Reforge Card" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Reforge Card</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">15 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n0507.png" alt="Sand Bag Party!" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Sand Bag Party!</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">5 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1698.png" alt="Sand Bag Lv6" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Sand Bag Lv6</span>
                                    <span class="mall-item-qty">x15</span>
                                </div>
                                <div class="mall-item-price">5 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n2108.png" alt="Great Fairy Ration" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Great Fairy Ration</span>
                                    <span class="mall-item-qty">x99</span>
                                </div>
                                <div class="mall-item-price">1 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1397.png" alt="3.5x Amplifier of Strive" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">3.5x Amplifier of Strive</span>
                                    <span class="mall-item-qty">x5</span>
                                </div>
                                <div class="mall-item-price">1 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1187.png" alt="Book of Skill Reset" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Book of Skill Reset</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">10 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/g0386.png" alt="Change Name Card" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Change Name Card</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">5 DC</div>
                            </div>
                        </div>
                        
                        <!-- Virgo Equipment -->
                        <h3 class="item-category-title">⚔️ Virgo Equipment</h3>
                        <div class="item-mall-grid">
                            <div class="mall-item mall-item-rare">
                                <img src="/patcher/texture/icon/e1281.png" alt="Virgo's Lust" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Virgo's Lust</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">25 DC</div>
                            </div>
                            <div class="mall-item mall-item-rare">
                                <img src="/patcher/texture/icon/e1281.png" alt="Virgo's Speed" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Virgo's Speed</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">25 DC</div>
                            </div>
                            <div class="mall-item mall-item-rare">
                                <img src="/patcher/texture/icon/e1285.png" alt="Virgo's Tank" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Virgo's Tank</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">25 DC</div>
                            </div>
                            <div class="mall-item mall-item-rare">
                                <img src="/patcher/texture/icon/e1289.png" alt="Virgo's Sea" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Virgo's Sea</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">25 DC</div>
                            </div>
                            <div class="mall-item mall-item-rare">
                                <img src="/patcher/texture/icon/e1289.png" alt="Virgo's Buffs" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Virgo's Buffs</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">25 DC</div>
                            </div>
                            <div class="mall-item mall-item-rare">
                                <img src="/patcher/texture/icon/e1293.png" alt="Virgo's Rage" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Virgo's Rage</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">25 DC</div>
                            </div>
                        </div>
                        
                        <!-- Mount Boxes -->
                        <h3 class="item-category-title">🐴 Mount Boxes</h3>
                        <div class="item-mall-grid">
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1779.png" alt="Black Dragon Mount Box" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Black Dragon Mount</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">30 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1779.png" alt="Lamb Mount Box" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Lamb Mount</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">30 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1779.png" alt="Broomstick Mount Box" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Broomstick Mount</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">30 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1779.png" alt="Ostrich Mount Box" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Ostrich Mount</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">30 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1779.png" alt="Lycan Mount Box" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Lycan Mount</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">30 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1779.png" alt="Cat Mount Box" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Cat Mount</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">30 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1779.png" alt="Lion Mount Box" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Lion Mount</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">30 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1779.png" alt="Tortoise Mount Box" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Tortoise Mount</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">30 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1779.png" alt="T-Rex Mount Box" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">T-Rex Mount</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">30 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1779.png" alt="Rat Mount Box" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Rat Mount</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">30 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1779.png" alt="Deer Mount Box" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Deer Mount</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">30 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1779.png" alt="Wolf Mount Box" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Wolf Mount</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">30 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1779.png" alt="Birdocopter Mount Box" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Birdocopter Mount</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">30 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1779.png" alt="Elephant Mount Box" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Elephant Mount</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">30 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1779.png" alt="Rockets Mount Box" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Rockets Mount</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">30 DC</div>
                            </div>
                        </div>
                        
                        <!-- Pet Boxes -->
                        <h3 class="item-category-title">🐾 Pet Boxes</h3>
                        <div class="item-mall-grid">
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1216.png" alt="August Pet 1 Box" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">August Pet 1 Box</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">30 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1216.png" alt="August Pet 2 Box" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">August Pet 2 Box</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">30 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1216.png" alt="August Pet 3 Box" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">August Pet 3 Box</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">30 DC</div>
                            </div>
                            <div class="mall-item">
                                <img src="/patcher/texture/icon/n1216.png" alt="August Pet 4 Box" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">August Pet 4 Box</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">30 DC</div>
                            </div>
                        </div>
                        
                        <!-- Special Items -->
                        <h3 class="item-category-title">✨ Special Items</h3>
                        <div class="item-mall-grid">
                            <div class="mall-item mall-item-legendary">
                                <img src="/patcher/texture/icon/n1754.png" alt="Rebirth Stone" class="mall-item-icon" onerror="this.src='/assets/images/placeholder-item.png'">
                                <div class="mall-item-info">
                                    <span class="mall-item-name">Rebirth Stone</span>
                                    <span class="mall-item-qty">x1</span>
                                </div>
                                <div class="mall-item-price">50 DC</div>
                            </div>
                        </div>
                        
                        <div class="after-payment" style="margin-top: 2rem;">
                            <h3>📝 How to Purchase</h3>
                            <ol class="step-list">
                                <li class="step-item">
                                    <span class="step-number">1</span>
                                    <div class="step-content">
                                        <strong>Donate</strong> using GCash, PayPal, or Crypto to receive Donation Coins.
                                    </div>
                                </li>
                                <li class="step-item">
                                    <span class="step-number">2</span>
                                    <div class="step-content">
                                        <strong>Contact an Admin</strong> in-game or via Discord with your receipt.
                                    </div>
                                </li>
                                <li class="step-item">
                                    <span class="step-number">3</span>
                                    <div class="step-content">
                                        <strong>Request your items</strong> and they will be delivered to your character.
                                    </div>
                                </li>
                            </ol>
                        </div>
                    </div>
                </div>
            </div>

            <!-- GCash Section -->
            <div class="payment-content active" id="gcash">
                <div class="card p-8">
                    <div class="donation-section">
                        <div class="donation-header">
                            <div class="donation-icon">📱</div>
                            <h2>GCash Payment</h2>
                        </div>
                        
                        <div class="rate-badge">$1 USD = 1 Donation Coin</div>
                        
                        <!-- GCash QR Code -->
                        <div style="text-align: center; margin: 2rem 0;">
                            <div style="background: #007DFE; padding: 20px; border-radius: 16px; display: inline-block;">
                                <img src="<?= asset('images/gcash-qr.jpg') ?>" alt="GCash QR Code" style="max-width: 280px; border-radius: 12px;">
                            </div>
                            <p class="text-muted mt-4">Scan this QR code with your GCash app</p>
                        </div>
                        
                        <h3>How to Pay Using GCash</h3>
                        
                        <ol class="step-list">
                            <li class="step-item">
                                <span class="step-number">1</span>
                                <div class="step-content">
                                    <strong>Open the GCash app</strong> on your phone.
                                </div>
                            </li>
                            <li class="step-item">
                                <span class="step-number">2</span>
                                <div class="step-content">
                                    <strong>Tap "Scan QR"</strong> from the main menu.
                                </div>
                            </li>
                            <li class="step-item">
                                <span class="step-number">3</span>
                                <div class="step-content">
                                    <strong>Scan the GCash QR code</strong> shown above.
                                </div>
                            </li>
                            <li class="step-item">
                                <span class="step-number">4</span>
                                <div class="step-content">
                                    <strong>Verify the recipient name</strong> to make sure it matches the official server payment account.
                                </div>
                            </li>
                            <li class="step-item">
                                <span class="step-number">5</span>
                                <div class="step-content">
                                    <strong>Enter the exact amount</strong> you wish to donate.
                                </div>
                            </li>
                            <li class="step-item">
                                <span class="step-number">6</span>
                                <div class="step-content">
                                    <strong>Tap "Pay"</strong> and confirm the transaction.
                                </div>
                            </li>
                        </ol>
                        
                        <div class="after-payment">
                            <h3>📸 After Payment</h3>
                            <ol class="step-list">
                                <li class="step-item">
                                    <span class="step-number">7</span>
                                    <div class="step-content">
                                        <strong>Take a screenshot</strong> of the successful payment receipt.
                                    </div>
                                </li>
                                <li class="step-item">
                                    <span class="step-number">8</span>
                                    <div class="step-content">
                                        <strong>Send the screenshot</strong> together with your <strong>in-game name</strong> via direct message on Discord to <strong>popout0360</strong>, or send it directly to the <strong>Slime Pirates Online Facebook page</strong>.
                                    </div>
                                </li>
                            </ol>
                        </div>
                    </div>
                </div>
            </div>

            <!-- PayPal Section -->
            <div class="payment-content" id="paypal">
                <div class="card p-8">
                    <div class="donation-section">
                        <div class="donation-header">
                            <div class="donation-icon">💳</div>
                            <h2>PayPal Payment</h2>
                        </div>
                        
                        <div class="rate-badge">$1 USD = 1 Donation Coin</div>
                        
                        <h3>How to Pay Using PayPal</h3>
                        
                        <ol class="step-list">
                            <li class="step-item">
                                <span class="step-number">1</span>
                                <div class="step-content">
                                    <strong>Open PayPal</strong> (app or website) and log in to your account.
                                </div>
                            </li>
                            <li class="step-item">
                                <span class="step-number">2</span>
                                <div class="step-content">
                                    <strong>Tap "Send & Request"</strong> (or "Send Money").
                                </div>
                            </li>
                            <li class="step-item">
                                <span class="step-number">3</span>
                                <div class="step-content">
                                    <strong>Enter this PayPal email</strong> as the recipient:
                                    <div class="wallet-address">
                                        <a href="mailto:eloisa.m.delacruz@gmail.com" class="email-link">eloisa.m.delacruz@gmail.com</a>
                                    </div>
                                </div>
                            </li>
                            <li class="step-item">
                                <span class="step-number">4</span>
                                <div class="step-content">
                                    <strong>Enter the exact amount</strong> you want to purchase.
                                </div>
                            </li>
                            <li class="step-item">
                                <span class="step-number">5</span>
                                <div class="step-content">
                                    <strong>Make sure the payment is set to:</strong>
                                    <br><span class="text-gold font-semibold">Friends and Family</span>
                                </div>
                            </li>
                            <li class="step-item">
                                <span class="step-number">6</span>
                                <div class="step-content">
                                    <strong>Confirm and complete</strong> the payment.
                                </div>
                            </li>
                        </ol>
                        
                        <div class="after-payment">
                            <h3>📸 After Payment</h3>
                            <p class="mb-4">Take a screenshot of the PayPal receipt showing:</p>
                            <ul class="mb-4">
                                <li>Transaction ID</li>
                                <li>Amount</li>
                                <li>Payment status (Completed)</li>
                            </ul>
                            <ol class="step-list">
                                <li class="step-item">
                                    <span class="step-number">7</span>
                                    <div class="step-content">
                                        <strong>Send the screenshot</strong> together with your <strong>in-game name</strong> via direct message on Discord to <strong>popout0360</strong>, or send it directly to the <strong>Slime Pirates Online Facebook page</strong>.
                                    </div>
                                </li>
                                <li class="step-item">
                                    <span class="step-number">8</span>
                                    <div class="step-content">
                                        <strong>Wait for confirmation.</strong> Donation Coins will be delivered after payment verification.
                                    </div>
                                </li>
                            </ol>
                        </div>
                        
                        <div class="important-notice">
                            <h3>🔒 Important Purchase Terms</h3>
                            <ul>
                                <li>This purchase is for digital in-game virtual goods delivered to the account name you provide</li>
                                <li>Items are <strong>non-refundable</strong> once delivered and confirmed</li>
                                <li>Delivery is logged for verification purposes</li>
                                <li>Payments sent without following the instructions or with incorrect details may delay delivery</li>
                                <li>Fraudulent disputes or chargebacks will result in <strong>permanent in-game account bans</strong> and submission of delivery proof to PayPal during investigation</li>
                            </ul>
                        </div>
                    </div>
                </div>
            </div>

            <!-- Crypto Section -->
            <div class="payment-content" id="crypto">
                <div class="card p-8">
                    <div class="donation-section">
                        <div class="donation-header">
                            <div class="donation-icon">🪙</div>
                            <h2>Crypto Payment (SOL)</h2>
                        </div>
                        
                        <div class="rate-badge">$1 USD = 1 Donation Coin</div>
                        
                        <h3>How to Pay Using SOL (Solana)</h3>
                        
                        <ol class="step-list">
                            <li class="step-item">
                                <span class="step-number">1</span>
                                <div class="step-content">
                                    <strong>Open your crypto wallet</strong> (Phantom, Solflare, Trust Wallet, etc.)
                                </div>
                            </li>
                            <li class="step-item">
                                <span class="step-number">2</span>
                                <div class="step-content">
                                    <strong>Choose SOL (Solana)</strong> as the currency.
                                </div>
                            </li>
                            <li class="step-item">
                                <span class="step-number">3</span>
                                <div class="step-content">
                                    <strong>Copy this wallet address:</strong>
                                    <div class="wallet-address">
                                        4NRFaeirqUorrwFu6ExFmvHHiuNzkUD1Dt6hQgikkg1T
                                    </div>
                                    <p class="text-muted text-sm">⚠️ Do NOT type it manually - always copy and paste!</p>
                                </div>
                            </li>
                            <li class="step-item">
                                <span class="step-number">4</span>
                                <div class="step-content">
                                    <strong>Paste the address</strong> into the "Send" field.
                                </div>
                            </li>
                            <li class="step-item">
                                <span class="step-number">5</span>
                                <div class="step-content">
                                    <strong>Enter the exact amount</strong> you wish to donate.
                                </div>
                            </li>
                            <li class="step-item">
                                <span class="step-number">6</span>
                                <div class="step-content">
                                    <strong>Make sure the network is set to:</strong>
                                    <br><span class="text-gold font-semibold">Solana (SOL)</span>
                                </div>
                            </li>
                            <li class="step-item">
                                <span class="step-number">7</span>
                                <div class="step-content">
                                    <strong>Send the payment.</strong>
                                </div>
                            </li>
                        </ol>
                        
                        <div class="after-payment">
                            <h3>📸 After Payment</h3>
                            <ol class="step-list">
                                <li class="step-item">
                                    <span class="step-number">8</span>
                                    <div class="step-content">
                                        <strong>Copy the Transaction Hash (TXID)</strong> from your wallet.
                                    </div>
                                </li>
                                <li class="step-item">
                                    <span class="step-number">9</span>
                                    <div class="step-content">
                                        <strong>Take a screenshot</strong> of the completed transaction.
                                    </div>
                                </li>
                                <li class="step-item">
                                    <span class="step-number">10</span>
                                    <div class="step-content">
                                        <strong>Send the TXID + screenshot</strong> together with your <strong>in-game name</strong> via direct message on Discord to <strong>popout0360</strong>, or send it directly to the <strong>Slime Pirates Online Facebook page</strong>.
                                    </div>
                                </li>
                            </ol>
                            
                            <div class="mt-4 p-4" style="background: rgba(76, 175, 80, 0.2); border-radius: 8px;">
                                <h4 style="color: #4CAF50; margin-top: 0;">✅ Confirmation</h4>
                                <ul class="mb-0">
                                    <li>Wait for verification</li>
                                    <li>Once confirmed, your purchase will be processed</li>
                                    <li>You will be notified when done</li>
                                </ul>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <!-- Contact Card -->
            <div class="card p-6 text-center mt-8">
                <h3 class="font-semibold mb-2">Need Help with Donations?</h3>
                <p class="text-muted mb-4">Contact our staff team through Discord for assistance with donations or payment issues.</p>
                <a href="https://discord.com/invite/NHmFsuMpS5" target="_blank" class="btn btn-primary">
                    <svg width="20" height="20" fill="currentColor" viewBox="0 0 24 24" style="margin-right: 8px;"><path d="M20.317 4.37a19.791 19.791 0 0 0-4.885-1.515.074.074 0 0 0-.079.037c-.21.375-.444.864-.608 1.25a18.27 18.27 0 0 0-5.487 0 12.64 12.64 0 0 0-.617-1.25.077.077 0 0 0-.079-.037A19.736 19.736 0 0 0 3.677 4.37a.07.07 0 0 0-.032.027C.533 9.046-.32 13.58.099 18.057a.082.082 0 0 0 .031.057 19.9 19.9 0 0 0 5.993 3.03.078.078 0 0 0 .084-.028 14.09 14.09 0 0 0 1.226-1.994.076.076 0 0 0-.041-.106 13.107 13.107 0 0 1-1.872-.892.077.077 0 0 1-.008-.128 10.2 10.2 0 0 0 .372-.292.074.074 0 0 1 .077-.01c3.928 1.793 8.18 1.793 12.062 0a.074.074 0 0 1 .078.01c.12.098.246.198.373.292a.077.077 0 0 1-.006.127 12.299 12.299 0 0 1-1.873.892.077.077 0 0 0-.041.107c.36.698.772 1.362 1.225 1.993a.076.076 0 0 0 .084.028 19.839 19.839 0 0 0 6.002-3.03.077.077 0 0 0 .032-.054c.5-5.177-.838-9.674-3.549-13.66a.061.061 0 0 0-.031-.03zM8.02 15.33c-1.183 0-2.157-1.085-2.157-2.419 0-1.333.956-2.419 2.157-2.419 1.21 0 2.176 1.096 2.157 2.42 0 1.333-.956 2.418-2.157 2.418zm7.975 0c-1.183 0-2.157-1.085-2.157-2.419 0-1.333.955-2.419 2.157-2.419 1.21 0 2.176 1.096 2.157 2.42 0 1.333-.946 2.418-2.157 2.418z"/></svg>
                    Join Discord
                </a>
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
                    </ul>
                </div>
            </div>
            
            <div class="footer-bottom">
                <p>&copy; <?= date('Y') ?> <?= e(SERVER_NAME) ?>. All rights reserved.</p>
            </div>
        </div>
    </footer>

    <script src="<?= asset('js/main.js') ?>"></script>
    <script>
        // Payment method tabs functionality
        document.querySelectorAll('.payment-tab').forEach(tab => {
            tab.addEventListener('click', function() {
                // Remove active class from all tabs and content
                document.querySelectorAll('.payment-tab').forEach(t => t.classList.remove('active'));
                document.querySelectorAll('.payment-content').forEach(c => c.classList.remove('active'));
                
                // Add active class to clicked tab
                this.classList.add('active');
                
                // Show corresponding content
                const tabId = this.getAttribute('data-tab');
                document.getElementById(tabId).classList.add('active');
            });
        });
    </script>
</body>
</html>
