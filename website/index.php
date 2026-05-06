<?php
/**
 * PKO Website - Landing Page
 */

require_once __DIR__ . '/includes/config.php';

// Get server stats with error handling
$stats = getServerStats();
$topPlayers = [];
$topGuilds = [];
$latestNews = [];

try {
    $topPlayers = CharacterModel::getTopByLevel(5);
    $topGuilds = GuildModel::getTopByMembers(5);
    $latestNews = NewsModel::getLatest(4);
} catch (Exception $e) {
    error_log("Index page DB error: " . $e->getMessage());
}

$newsCategories = NewsModel::getCategories();

Security::setHeaders();
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <?= metaTags('', 'Embark on an epic pirate adventure! Join thousands of players in the ultimate MMORPG experience.') ?>
    <link rel="stylesheet" href="<?= asset('css/style.css') ?>">
    <link rel="icon" type="image/png" href="<?= asset('images/favicon.png') ?>">
</head>
<body>
    <!-- Navigation -->
    <nav class="navbar" role="navigation">
        <div class="container">
            <div class="navbar-content">
                <a href="/" class="navbar-logo"><?= e(SERVER_NAME) ?></a>
                
                <ul class="navbar-menu" id="navbar-menu">
                    <li><a href="/" class="navbar-link active">Home</a></li>
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

    <!-- Hero Section -->
    <section class="hero">
        <div class="hero-bg" style="background-image: url('<?= asset('images/hero-bg.jpg') ?>')"></div>
        <div class="hero-particles"></div>
        
        <div class="hero-content" style="padding-top: 40px;">
            <p class="hero-subtitle">Set Sail for Adventure</p>
            <h1 class="title-hero hero-title"><?= e(SERVER_NAME) ?></h1>
            <p class="hero-description">
                Set sail with Slime. Build your crew, chase legendary loot, and rule the seas in a classic pirate MMORPG reborn.
            </p>
            
            <div class="hero-actions">
                <a href="/register.php" class="btn btn-primary btn-lg">
                    <svg width="20" height="20" fill="currentColor" viewBox="0 0 20 20">
                        <path fill-rule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zM9.555 7.168A1 1 0 008 8v4a1 1 0 001.555.832l3-2a1 1 0 000-1.664l-3-2z" clip-rule="evenodd"/>
                    </svg>
                    Start Playing Free
                </a>
                <a href="/download.php" class="btn btn-secondary btn-lg">
                    <svg width="20" height="20" fill="currentColor" viewBox="0 0 20 20">
                        <path fill-rule="evenodd" d="M3 17a1 1 0 011-1h12a1 1 0 110 2H4a1 1 0 01-1-1zm3.293-7.707a1 1 0 011.414 0L9 10.586V3a1 1 0 112 0v7.586l1.293-1.293a1 1 0 111.414 1.414l-3 3a1 1 0 01-1.414 0l-3-3a1 1 0 010-1.414z" clip-rule="evenodd"/>
                    </svg>
                    Download Client
                </a>
            </div>
            
            <div class="hero-stats">
                <div class="hero-stat">
                    <div class="server-status" data-server-status>
                        <span class="server-status-dot <?= $stats['online'] ? 'online' : 'offline' ?>"></span>
                        <span><?= $stats['online'] ? 'Online • ' . formatNumber($stats['players_online']) . ' Players' : 'Offline' ?></span>
                    </div>
                </div>
            </div>
            
            <!-- YouTube Video Showcase -->
            <div class="youtube-showcase">
                <h3 class="showcase-title">🎬 Game Preview</h3>
                <div class="video-carousel-container">
                    <button class="carousel-btn carousel-prev" aria-label="Previous videos">
                        <svg width="24" height="24" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M12.707 5.293a1 1 0 010 1.414L9.414 10l3.293 3.293a1 1 0 01-1.414 1.414l-4-4a1 1 0 010-1.414l4-4a1 1 0 011.414 0z" clip-rule="evenodd"/></svg>
                    </button>
                    <div class="video-carousel" id="videoCarousel">
                        <?php
                        // Add your YouTube video IDs here
                        $youtubeVideos = [
                            ['id' => 'YaLdP-Rw-TQ', 'title' => 'Slime Pirates Gameplay'],
                            ['id' => 'KwXlRU6B4Vs', 'title' => 'Adventure Highlights'],
                            ['id' => 'nUl85y3Cpf4', 'title' => 'Epic Battles'],
                            ['id' => 'zw4RHt2x8wA', 'title' => 'Busy Market'],
                            ['id' => 'R-Sw87Z2uzI', 'title' => 'Boss Raid Event'],
                            ['id' => 'w1jg5nlV_Ic', 'title' => 'Guild War'],
                            ['id' => 'oyKhgQC-BjY', 'title' => 'Character Showcase'],
                            ['id' => '7mxDNgVknrU', 'title' => 'PvP Action'],
                            ['id' => 'PRLHg8egGnc', 'title' => 'Event Highlights'],
                            ['id' => '366Swwo7tfo', 'title' => 'World Exploration'],
                        ];
                        foreach ($youtubeVideos as $video): ?>
                        <a href="https://www.youtube.com/watch?v=<?= $video['id'] ?>" target="_blank" class="video-thumbnail" title="<?= htmlspecialchars($video['title']) ?>">
                            <img src="https://img.youtube.com/vi/<?= $video['id'] ?>/mqdefault.jpg" alt="<?= htmlspecialchars($video['title']) ?>" loading="lazy">
                            <div class="video-play-icon">
                                <svg width="48" height="48" fill="currentColor" viewBox="0 0 24 24"><path d="M8 5v14l11-7z"/></svg>
                            </div>
                            <span class="video-title"><?= htmlspecialchars($video['title']) ?></span>
                        </a>
                        <?php endforeach; ?>
                    </div>
                    <button class="carousel-btn carousel-next" aria-label="Next videos">
                        <svg width="24" height="24" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M7.293 14.707a1 1 0 010-1.414L10.586 10 7.293 6.707a1 1 0 011.414-1.414l4 4a1 1 0 010 1.414l-4 4a1 1 0 01-1.414 0z" clip-rule="evenodd"/></svg>
                    </button>
                </div>
                <a href="https://www.youtube.com/@slimepiratesonline" target="_blank" class="view-channel-btn">
                    <svg width="20" height="20" fill="currentColor" viewBox="0 0 24 24"><path d="M23.498 6.186a3.016 3.016 0 0 0-2.122-2.136C19.505 3.545 12 3.545 12 3.545s-7.505 0-9.377.505A3.017 3.017 0 0 0 .502 6.186C0 8.07 0 12 0 12s0 3.93.502 5.814a3.016 3.016 0 0 0 2.122 2.136c1.871.505 9.376.505 9.376.505s7.505 0 9.377-.505a3.015 3.015 0 0 0 2.122-2.136C24 15.93 24 12 24 12s0-3.93-.502-5.814zM9.545 15.568V8.432L15.818 12l-6.273 3.568z"/></svg>
                    View All Videos
                </a>
            </div>
        </div>
    </section>

    <!-- Stats Section -->
    <section class="section" style="margin-top: -80px; position: relative; z-index: 10;">
        <div class="container">
            <div class="stats-grid">
                <div class="stat-card" data-animate>
                    <div class="stat-icon">
                        <svg width="24" height="24" fill="currentColor" viewBox="0 0 20 20">
                            <path d="M9 6a3 3 0 11-6 0 3 3 0 016 0zM17 6a3 3 0 11-6 0 3 3 0 016 0zM12.93 17c.046-.327.07-.66.07-1a6.97 6.97 0 00-1.5-4.33A5 5 0 0119 16v1h-6.07zM6 11a5 5 0 015 5v1H1v-1a5 5 0 015-5z"/>
                        </svg>
                    </div>
                    <div class="stat-value" data-counter="<?= $stats['total_accounts'] ?>"><?= formatNumber($stats['total_accounts']) ?></div>
                    <div class="stat-label">Registered Players</div>
                </div>
                
                <div class="stat-card" data-animate>
                    <div class="stat-icon">
                        <svg width="24" height="24" fill="currentColor" viewBox="0 0 20 20">
                            <path fill-rule="evenodd" d="M10 9a3 3 0 100-6 3 3 0 000 6zm-7 9a7 7 0 1114 0H3z" clip-rule="evenodd"/>
                        </svg>
                    </div>
                    <div class="stat-value" data-counter="<?= $stats['total_characters'] ?>"><?= formatNumber($stats['total_characters']) ?></div>
                    <div class="stat-label">Characters Created</div>
                </div>
                
                <div class="stat-card" data-animate>
                    <div class="stat-icon">
                        <svg width="24" height="24" fill="currentColor" viewBox="0 0 20 20">
                            <path d="M13 6a3 3 0 11-6 0 3 3 0 016 0zM18 8a2 2 0 11-4 0 2 2 0 014 0zM14 15a4 4 0 00-8 0v3h8v-3zM6 8a2 2 0 11-4 0 2 2 0 014 0zM16 18v-3a5.972 5.972 0 00-.75-2.906A3.005 3.005 0 0119 15v3h-3zM4.75 12.094A5.973 5.973 0 004 15v3H1v-3a3 3 0 013.75-2.906z"/>
                        </svg>
                    </div>
                    <div class="stat-value" data-counter="<?= $stats['total_guilds'] ?>"><?= formatNumber($stats['total_guilds']) ?></div>
                    <div class="stat-label">Active Guilds</div>
                </div>
                
                <div class="stat-card" data-animate>
                    <div class="stat-icon">
                        <svg width="24" height="24" fill="currentColor" viewBox="0 0 20 20">
                            <path fill-rule="evenodd" d="M3.172 5.172a4 4 0 015.656 0L10 6.343l1.172-1.171a4 4 0 115.656 5.656L10 17.657l-6.828-6.829a4 4 0 010-5.656z" clip-rule="evenodd"/>
                        </svg>
                    </div>
                    <div class="stat-value" data-counter="<?= $stats['players_online'] ?>"><?= formatNumber($stats['players_online']) ?></div>
                    <div class="stat-label">Online Now</div>
                </div>
            </div>
        </div>
    </section>

    <!-- News Section -->
    <section class="section section-lg">
        <div class="container">
            <div class="text-center mb-8">
                <h2 class="title-section text-center" data-animate>📰 Latest News</h2>
                <p class="text-secondary text-lg" data-animate>Stay updated with the latest announcements and events</p>
            </div>
            
            <?php if (!empty($latestNews)): ?>
            <div class="grid grid-cols-2" style="gap: 1.5rem;">
                <?php foreach ($latestNews as $index => $article): ?>
                <?php $cat = $newsCategories[$article['category']] ?? ['name' => 'News', 'icon' => '📰', 'color' => '#888']; ?>
                <a href="/news.php?slug=<?= e($article['slug']) ?>" class="news-card" data-animate style="text-decoration: none; color: inherit;">
                    <div class="card card-highlight" style="height: 100%; display: flex; flex-direction: column;">
                        <?php if ($article['image']): ?>
                        <div class="news-card-image" style="height: 180px; background: url('<?= e($article['image']) ?>') center/cover; border-radius: 8px 8px 0 0; margin: -1.5rem -1.5rem 1rem -1.5rem;"></div>
                        <?php endif; ?>
                        <div class="news-card-content" style="flex: 1; display: flex; flex-direction: column;">
                            <div style="display: flex; align-items: center; gap: 8px; margin-bottom: 0.75rem;">
                                <span class="category-badge" style="background: <?= $cat['color'] ?>20; color: <?= $cat['color'] ?>; padding: 4px 10px; border-radius: 12px; font-size: 0.75rem;">
                                    <?= $cat['icon'] ?> <?= $cat['name'] ?>
                                </span>
                                <?php if ($article['is_pinned']): ?>
                                <span style="font-size: 0.75rem;" title="Pinned">📌</span>
                                <?php endif; ?>
                            </div>
                            <h3 class="card-title" style="font-size: 1.1rem; margin-bottom: 0.5rem; line-height: 1.4;"><?= e($article['title']) ?></h3>
                            <?php if ($article['summary']): ?>
                            <p class="text-secondary" style="font-size: 0.9rem; margin-bottom: 1rem; flex: 1; line-height: 1.5;"><?= e(substr($article['summary'], 0, 120)) ?><?= strlen($article['summary']) > 120 ? '...' : '' ?></p>
                            <?php endif; ?>
                            <div class="news-card-meta" style="margin-top: auto; font-size: 0.8rem; color: var(--text-muted); display: flex; justify-content: space-between;">
                                <span><?= date('M j, Y', strtotime($article['created_at'])) ?></span>
                                <span>👁 <?= number_format($article['views']) ?> views</span>
                            </div>
                        </div>
                    </div>
                </a>
                <?php endforeach; ?>
            </div>
            
            <div class="text-center mt-6">
                <a href="/news-list.php" class="btn btn-ghost">View All News →</a>
            </div>
            <?php else: ?>
            <div class="text-center py-8">
                <p class="text-muted">No news articles yet. Check back soon!</p>
            </div>
            <?php endif; ?>
        </div>
    </section>

    <!-- Features Section -->
    <section class="section section-lg">
        <div class="container">
            <div class="text-center mb-8">
                <h2 class="title-section text-center" data-animate>Why Choose Us?</h2>
                <p class="text-secondary text-lg" data-animate>Experience the ultimate pirate MMORPG with these amazing features</p>
            </div>
            
            <div class="grid grid-cols-3">
                <div class="card card-highlight" data-animate>
                    <div class="stat-icon mb-4">
                        <svg width="24" height="24" fill="currentColor" viewBox="0 0 20 20">
                            <path fill-rule="evenodd" d="M11.3 1.046A1 1 0 0112 2v5h4a1 1 0 01.82 1.573l-7 10A1 1 0 018 18v-5H4a1 1 0 01-.82-1.573l7-10a1 1 0 011.12-.38z" clip-rule="evenodd"/>
                        </svg>
                    </div>
                    <h3 class="card-title mb-2">Slime Battle Zone</h3>
                    <p class="text-secondary">Crew up, throw hands, and crush enemies in fast, chaotic fights across every map.</p>
                </div>
                
                <div class="card card-highlight" data-animate>
                    <div class="stat-icon mb-4">
                        <svg width="24" height="24" fill="currentColor" viewBox="0 0 20 20">
                            <path d="M9 4.804A7.968 7.968 0 005.5 4c-1.255 0-2.443.29-3.5.804v10A7.969 7.969 0 015.5 14c1.669 0 3.218.51 4.5 1.385A7.962 7.962 0 0114.5 14c1.255 0 2.443.29 3.5.804v-10A7.968 7.968 0 0014.5 4c-1.255 0-2.443.29-3.5.804V12a1 1 0 11-2 0V4.804z"/>
                        </svg>
                    </div>
                    <h3 class="card-title mb-2">Slime Missions & Loot</h3>
                    <p class="text-secondary">Take on wild missions, discover hidden rewards, and stack loot like a true Slime legend.</p>
                </div>
                
                <div class="card card-highlight" data-animate>
                    <div class="stat-icon mb-4">
                        <svg width="24" height="24" fill="currentColor" viewBox="0 0 20 20">
                            <path d="M13 6a3 3 0 11-6 0 3 3 0 016 0zM18 8a2 2 0 11-4 0 2 2 0 014 0zM14 15a4 4 0 00-8 0v3h8v-3zM6 8a2 2 0 11-4 0 2 2 0 014 0zM16 18v-3a5.972 5.972 0 00-.75-2.906A3.005 3.005 0 0119 15v3h-3zM4.75 12.094A5.973 5.973 0 004 15v3H1v-3a3 3 0 013.75-2.906z"/>
                        </svg>
                    </div>
                    <h3 class="card-title mb-2">Active Community</h3>
                    <p class="text-secondary">Join a vibrant community of pirates! Form guilds, participate in events, and make friends from around the world.</p>
                </div>
                
                <div class="card card-highlight" data-animate>
                    <div class="stat-icon mb-4">
                        <svg width="24" height="24" fill="currentColor" viewBox="0 0 20 20">
                            <path fill-rule="evenodd" d="M6.267 3.455a3.066 3.066 0 001.745-.723 3.066 3.066 0 013.976 0 3.066 3.066 0 001.745.723 3.066 3.066 0 012.812 2.812c.051.643.304 1.254.723 1.745a3.066 3.066 0 010 3.976 3.066 3.066 0 00-.723 1.745 3.066 3.066 0 01-2.812 2.812 3.066 3.066 0 00-1.745.723 3.066 3.066 0 01-3.976 0 3.066 3.066 0 00-1.745-.723 3.066 3.066 0 01-2.812-2.812 3.066 3.066 0 00-.723-1.745 3.066 3.066 0 010-3.976 3.066 3.066 0 00.723-1.745 3.066 3.066 0 012.812-2.812zm7.44 5.252a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clip-rule="evenodd"/>
                        </svg>
                    </div>
                    <h3 class="card-title mb-2">Fair Gameplay</h3>
                    <p class="text-secondary">Anti-cheat protection and balanced gameplay ensure everyone has a fair and enjoyable experience.</p>
                </div>
                
                <div class="card card-highlight" data-animate>
                    <div class="stat-icon mb-4">
                        <svg width="24" height="24" fill="currentColor" viewBox="0 0 20 20">
                            <path fill-rule="evenodd" d="M5 2a1 1 0 011 1v1h1a1 1 0 010 2H6v1a1 1 0 01-2 0V6H3a1 1 0 010-2h1V3a1 1 0 011-1zm0 10a1 1 0 011 1v1h1a1 1 0 110 2H6v1a1 1 0 11-2 0v-1H3a1 1 0 110-2h1v-1a1 1 0 011-1zM12 2a1 1 0 01.967.744L14.146 7.2 17.5 9.134a1 1 0 010 1.732l-3.354 1.935-1.18 4.455a1 1 0 01-1.933 0L9.854 12.8 6.5 10.866a1 1 0 010-1.732l3.354-1.935 1.18-4.455A1 1 0 0112 2z" clip-rule="evenodd"/>
                        </svg>
                    </div>
                    <h3 class="card-title mb-2">Regular Updates</h3>
                    <p class="text-secondary">New content, events, and features are added regularly to keep your adventure fresh and exciting.</p>
                </div>
                
                <div class="card card-highlight" data-animate>
                    <div class="stat-icon mb-4">
                        <svg width="24" height="24" fill="currentColor" viewBox="0 0 20 20">
                            <path fill-rule="evenodd" d="M2.166 4.999A11.954 11.954 0 0010 1.944 11.954 11.954 0 0017.834 5c.11.65.166 1.32.166 2.001 0 5.225-3.34 9.67-8 11.317C5.34 16.67 2 12.225 2 7c0-.682.057-1.35.166-2.001zm11.541 3.708a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clip-rule="evenodd"/>
                        </svg>
                    </div>
                    <h3 class="card-title mb-2">Secure & Stable</h3>
                    <p class="text-secondary">Enterprise-grade security with DDoS protection and reliable servers for uninterrupted gameplay.</p>
                </div>
            </div>
        </div>
    </section>

    <!-- Leaderboard Section -->
    <section class="section">
        <div class="container">
            <div class="text-center mb-8">
                <h2 class="title-section text-center" data-animate>Top Pirates</h2>
                <p class="text-secondary text-lg" data-animate>Meet the legends of the seven seas</p>
            </div>
            
            <div class="grid grid-cols-2">
                <!-- Top Players -->
                <div class="leaderboard" data-animate>
                    <div class="leaderboard-header">
                        <h3 class="leaderboard-title">🏆 Top Players</h3>
                        <a href="/leaderboard.php" class="btn btn-sm btn-ghost">View All</a>
                    </div>
                    <ul class="leaderboard-list">
                        <?php foreach ($topPlayers as $index => $player): ?>
                        <li class="leaderboard-item">
                            <span class="leaderboard-rank <?= $index === 0 ? 'gold' : ($index === 1 ? 'silver' : ($index === 2 ? 'bronze' : '')) ?>">
                                <?= $index + 1 ?>
                            </span>
                            <img class="leaderboard-avatar" src="<?= asset('images/classes/' . strtolower(str_replace(' ', '_', $player['job'] ?: 'newbie')) . '.svg') ?>" alt="" onerror="this.src='<?= asset('images/classes/newbie.svg') ?>'">
                            <div class="leaderboard-info">
                                <div class="leaderboard-name"><?= e($player['cha_name']) ?></div>
                                <div class="leaderboard-meta"><?= e($player['guild_name'] ?? 'No Guild') ?> • Lv.<?= $player['degree'] ?></div>
                            </div>
                            <span class="leaderboard-value">Lv.<?= $player['degree'] ?></span>
                        </li>
                        <?php endforeach; ?>
                        <?php if (empty($topPlayers)): ?>
                        <li class="leaderboard-item text-center text-muted">No players yet. Be the first!</li>
                        <?php endif; ?>
                    </ul>
                </div>
                
                <!-- Top Guilds -->
                <div class="leaderboard" data-animate>
                    <div class="leaderboard-header">
                        <h3 class="leaderboard-title">⚔️ Top Guilds</h3>
                        <a href="/leaderboard.php?tab=guilds" class="btn btn-sm btn-ghost">View All</a>
                    </div>
                    <ul class="leaderboard-list">
                        <?php foreach ($topGuilds as $index => $guild): ?>
                        <li class="leaderboard-item">
                            <span class="leaderboard-rank <?= $index === 0 ? 'gold' : ($index === 1 ? 'silver' : ($index === 2 ? 'bronze' : '')) ?>">
                                <?= $index + 1 ?>
                            </span>
                            <div class="leaderboard-avatar flex items-center justify-center text-gold font-bold">
                                <?= strtoupper(substr($guild['guild_name'], 0, 2)) ?>
                            </div>
                            <div class="leaderboard-info">
                                <div class="leaderboard-name"><?= e($guild['guild_name']) ?></div>
                                <div class="leaderboard-meta">Leader: <?= e($guild['leader_name'] ?? 'Unknown') ?> • <?= $guild['member_total'] ?> members</div>
                            </div>
                            <span class="leaderboard-value"><?= $guild['member_total'] ?> 👥</span>
                        </li>
                        <?php endforeach; ?>
                        <?php if (empty($topGuilds)): ?>
                        <li class="leaderboard-item text-center text-muted">No guilds yet. Create one!</li>
                        <?php endif; ?>
                    </ul>
                </div>
            </div>
        </div>
    </section>

    <!-- CTA Section -->
    <section class="section section-lg" style="background: var(--gradient-glow);">
        <div class="container">
            <div class="text-center" data-animate>
                <h2 class="title-section text-center">Ready to Set Sail?</h2>
                <p class="text-secondary text-xl mb-8" style="max-width: 600px; margin-left: auto; margin-right: auto;">
                    Join thousands of players and begin your epic pirate adventure today. It's free to play!
                </p>
                <div class="flex justify-center gap-4">
                    <a href="/register.php" class="btn btn-primary btn-lg">Create Free Account</a>
                    <a href="/download.php" class="btn btn-secondary btn-lg">Download Game</a>
                </div>
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
                <div class="server-status" data-server-status>
                    <span class="server-status-dot <?= $stats['online'] ? 'online' : 'offline' ?>"></span>
                    <span>Server <?= $stats['online'] ? 'Online' : 'Offline' ?></span>
                </div>
            </div>
        </div>
    </footer>

    <style>
        /* YouTube Video Showcase */
        .youtube-showcase {
            margin-top: 2.5rem;
            text-align: center;
        }
        .showcase-title {
            color: var(--gold, #d4af37);
            font-size: 1.1rem;
            margin-bottom: 1rem;
            font-weight: 500;
            letter-spacing: 1px;
        }
        .video-carousel-container {
            position: relative;
            display: flex;
            align-items: center;
            gap: 0.5rem;
            max-width: 1200px;
            margin: 0 auto;
        }
        .video-carousel {
            display: flex;
            gap: 1rem;
            overflow-x: auto;
            scroll-behavior: smooth;
            scrollbar-width: none;
            -ms-overflow-style: none;
            padding: 0.5rem 0;
        }
        .video-carousel::-webkit-scrollbar {
            display: none;
        }
        .video-thumbnail {
            flex-shrink: 0;
            width: 280px;
            position: relative;
            border-radius: 8px;
            overflow: hidden;
            border: 2px solid transparent;
            transition: all 0.3s ease;
            text-decoration: none;
        }
        .video-thumbnail:hover {
            border-color: var(--gold, #d4af37);
            transform: translateY(-4px);
            box-shadow: 0 8px 20px rgba(212, 175, 55, 0.3);
        }
        .video-thumbnail img {
            width: 100%;
            height: 158px;
            object-fit: cover;
            display: block;
        }
        .video-play-icon {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -60%);
            width: 48px;
            height: 48px;
            background: rgba(0, 0, 0, 0.7);
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            opacity: 0;
            transition: opacity 0.3s ease;
        }
        .video-play-icon svg {
            fill: #fff;
            margin-left: 4px;
        }
        .video-thumbnail:hover .video-play-icon {
            opacity: 1;
        }
        .video-title {
            display: block;
            background: rgba(0, 0, 0, 0.85);
            color: #fff;
            font-size: 0.75rem;
            padding: 0.4rem 0.5rem;
            text-align: center;
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
        }
        .carousel-btn {
            flex-shrink: 0;
            width: 36px;
            height: 36px;
            border-radius: 50%;
            background: rgba(212, 175, 55, 0.2);
            border: 1px solid var(--gold, #d4af37);
            color: var(--gold, #d4af37);
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: all 0.3s ease;
        }
        .carousel-btn:hover {
            background: var(--gold, #d4af37);
            color: #000;
        }
        .view-channel-btn {
            display: inline-flex;
            align-items: center;
            gap: 0.5rem;
            margin-top: 1rem;
            padding: 0.5rem 1rem;
            background: transparent;
            border: 1px solid rgba(255, 255, 255, 0.2);
            border-radius: 20px;
            color: #fff;
            text-decoration: none;
            font-size: 0.85rem;
            transition: all 0.3s ease;
        }
        .view-channel-btn:hover {
            background: rgba(255, 0, 0, 0.1);
            border-color: #ff0000;
            color: #ff0000;
        }
        .view-channel-btn svg {
            fill: currentColor;
        }
        @media (max-width: 768px) {
            .video-thumbnail {
                width: 200px;
            }
            .video-thumbnail img {
                height: 112px;
            }
            .carousel-btn {
                display: none;
            }
        }
    </style>
    
    <script>
        // YouTube Video Carousel with Auto-scroll
        document.addEventListener('DOMContentLoaded', function() {
            const carousel = document.getElementById('videoCarousel');
            const prevBtn = document.querySelector('.carousel-prev');
            const nextBtn = document.querySelector('.carousel-next');
            
            if (carousel && prevBtn && nextBtn) {
                const scrollAmount = 296; // thumbnail width + gap
                let autoScrollInterval;
                
                // Manual scroll buttons
                prevBtn.addEventListener('click', () => {
                    carousel.scrollBy({ left: -scrollAmount, behavior: 'smooth' });
                });
                
                nextBtn.addEventListener('click', () => {
                    carousel.scrollBy({ left: scrollAmount, behavior: 'smooth' });
                });
                
                // Auto-scroll function
                function autoScroll() {
                    const maxScrollLeft = carousel.scrollWidth - carousel.clientWidth;
                    if (carousel.scrollLeft >= maxScrollLeft - 10) {
                        // Reset to beginning
                        carousel.scrollTo({ left: 0, behavior: 'smooth' });
                    } else {
                        carousel.scrollBy({ left: scrollAmount, behavior: 'smooth' });
                    }
                }
                
                // Start auto-scroll every 3 seconds
                autoScrollInterval = setInterval(autoScroll, 3000);
                
                // Pause on hover
                carousel.addEventListener('mouseenter', () => {
                    clearInterval(autoScrollInterval);
                });
                
                // Resume on mouse leave
                carousel.addEventListener('mouseleave', () => {
                    autoScrollInterval = setInterval(autoScroll, 3000);
                });
                
                // Pause when manually clicking buttons
                prevBtn.addEventListener('click', () => {
                    clearInterval(autoScrollInterval);
                    autoScrollInterval = setInterval(autoScroll, 3000);
                });
                
                nextBtn.addEventListener('click', () => {
                    clearInterval(autoScrollInterval);
                    autoScrollInterval = setInterval(autoScroll, 3000);
                });
            }
        });
    </script>
    <script src="<?= asset('js/main.js') ?>"></script>
</body>
</html>
