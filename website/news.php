<?php
/**
 * PKO Website - News Article Page
 */

require_once __DIR__ . '/includes/config.php';

Security::setHeaders();

// Initialize session before any output
$isLoggedIn = Auth::check();
$currentUser = $isLoggedIn ? Auth::user() : null;

// Get news by slug or ID
$slug = $_GET['slug'] ?? null;
$id = isset($_GET['id']) ? (int)$_GET['id'] : null;

$news = null;

if ($slug) {
    $news = NewsModel::getBySlug($slug);
} elseif ($id) {
    $news = NewsModel::getById($id);
}

// 404 if not found or not published
if (!$news || (!$news['is_published'] && (!$currentUser || strtolower($currentUser['name']) !== 'admin'))) {
    http_response_code(404);
    header('Location: /404.php');
    exit;
}

// Increment view count
NewsModel::incrementViews($news['id']);

// Get categories for display
$categories = NewsModel::getCategories();
$categoryInfo = $categories[$news['category']] ?? ['name' => 'News', 'icon' => '📰', 'color' => '#888'];

// Format date
$createdDate = new DateTime($news['created_at']);
$formattedDate = $createdDate->format('F j, Y');
$timeAgo = getTimeAgo($news['created_at']);

function getTimeAgo($datetime) {
    $now = new DateTime();
    $ago = new DateTime($datetime);
    $diff = $now->diff($ago);
    
    if ($diff->y > 0) return $diff->y . ' year' . ($diff->y > 1 ? 's' : '') . ' ago';
    if ($diff->m > 0) return $diff->m . ' month' . ($diff->m > 1 ? 's' : '') . ' ago';
    if ($diff->d > 0) return $diff->d . ' day' . ($diff->d > 1 ? 's' : '') . ' ago';
    if ($diff->h > 0) return $diff->h . ' hour' . ($diff->h > 1 ? 's' : '') . ' ago';
    if ($diff->i > 0) return $diff->i . ' minute' . ($diff->i > 1 ? 's' : '') . ' ago';
    return 'Just now';
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <?= metaTags($news['title'], $news['summary'] ?? 'Read the latest news from ' . SERVER_NAME) ?>
    <link rel="stylesheet" href="<?= asset('css/style.css') ?>">
    <link rel="icon" type="image/png" href="<?= asset('images/favicon.png') ?>">
    <style>
        .news-article {
            max-width: 800px;
            margin: 0 auto;
        }
        .news-header {
            margin-bottom: 2rem;
        }
        .news-category {
            display: inline-flex;
            align-items: center;
            gap: 6px;
            padding: 6px 12px;
            border-radius: 20px;
            font-size: 0.85rem;
            font-weight: 600;
            margin-bottom: 1rem;
        }
        .news-title {
            font-size: 2.5rem;
            font-weight: 700;
            line-height: 1.2;
            margin-bottom: 1rem;
            color: #fff;
        }
        .news-meta {
            display: flex;
            flex-wrap: wrap;
            gap: 1.5rem;
            color: var(--text-muted);
            font-size: 0.9rem;
        }
        .news-meta-item {
            display: flex;
            align-items: center;
            gap: 6px;
        }
        .news-image {
            width: 100%;
            max-height: 400px;
            object-fit: cover;
            border-radius: 12px;
            margin-bottom: 2rem;
        }
        .news-content {
            color: #e0e0e0;
            font-size: 1.1rem;
            line-height: 1.8;
        }
        .news-content h2, .news-content h3 {
            color: var(--gold);
            margin-top: 2rem;
            margin-bottom: 1rem;
        }
        .news-content p {
            margin-bottom: 1.5rem;
        }
        .news-content ul, .news-content ol {
            margin-bottom: 1.5rem;
            padding-left: 1.5rem;
        }
        .news-content li {
            margin-bottom: 0.5rem;
        }
        .news-content a {
            color: var(--gold);
            text-decoration: underline;
        }
        .news-content img {
            max-width: 100%;
            border-radius: 8px;
            margin: 1rem 0;
        }
        .news-footer {
            margin-top: 3rem;
            padding-top: 2rem;
            border-top: 1px solid rgba(255,255,255,0.1);
        }
        .back-link {
            display: inline-flex;
            align-items: center;
            gap: 8px;
            color: var(--gold);
            text-decoration: none;
            transition: opacity 0.2s;
        }
        .back-link:hover {
            opacity: 0.8;
        }
        .pinned-badge {
            display: inline-flex;
            align-items: center;
            gap: 4px;
            background: var(--gold);
            color: #000;
            padding: 4px 10px;
            border-radius: 4px;
            font-size: 0.75rem;
            font-weight: 600;
            margin-left: 8px;
        }
        .admin-actions {
            display: flex;
            gap: 1rem;
            margin-bottom: 1rem;
        }
        @media (max-width: 768px) {
            .news-title {
                font-size: 1.8rem;
            }
            .news-meta {
                gap: 1rem;
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
        <div class="container">
            <article class="news-article">
                <!-- Admin Actions -->
                <?php if ($currentUser && strtolower($currentUser['name']) === 'admin'): ?>
                <div class="admin-actions">
                    <a href="/admin/news.php?action=edit&id=<?= $news['id'] ?>" class="btn btn-ghost btn-sm">
                        ✏️ Edit Article
                    </a>
                    <?php if (!$news['is_published']): ?>
                    <span class="badge" style="background: #f44336; padding: 8px 12px;">📝 Draft - Not Published</span>
                    <?php endif; ?>
                </div>
                <?php endif; ?>

                <header class="news-header">
                    <div>
                        <span class="news-category" style="background: <?= $categoryInfo['color'] ?>20; color: <?= $categoryInfo['color'] ?>;">
                            <?= $categoryInfo['icon'] ?> <?= $categoryInfo['name'] ?>
                        </span>
                        <?php if ($news['is_pinned']): ?>
                        <span class="pinned-badge">📌 Pinned</span>
                        <?php endif; ?>
                    </div>
                    
                    <h1 class="news-title"><?= e($news['title']) ?></h1>
                    
                    <div class="news-meta">
                        <span class="news-meta-item">
                            <svg width="16" height="16" fill="currentColor" viewBox="0 0 16 16"><path d="M8 8a3 3 0 1 0 0-6 3 3 0 0 0 0 6zm2-3a2 2 0 1 1-4 0 2 2 0 0 1 4 0zm4 8c0 1-1 1-1 1H3s-1 0-1-1 1-4 6-4 6 3 6 4zm-1-.004c-.001-.246-.154-.986-.832-1.664C11.516 10.68 10.289 10 8 10c-2.29 0-3.516.68-4.168 1.332-.678.678-.83 1.418-.832 1.664h10z"/></svg>
                            <?= e($news['author']) ?>
                        </span>
                        <span class="news-meta-item">
                            <svg width="16" height="16" fill="currentColor" viewBox="0 0 16 16"><path d="M11 6.5a.5.5 0 0 1 .5-.5h1a.5.5 0 0 1 .5.5v1a.5.5 0 0 1-.5.5h-1a.5.5 0 0 1-.5-.5v-1zm-3 0a.5.5 0 0 1 .5-.5h1a.5.5 0 0 1 .5.5v1a.5.5 0 0 1-.5.5h-1a.5.5 0 0 1-.5-.5v-1zm-5 3a.5.5 0 0 1 .5-.5h1a.5.5 0 0 1 .5.5v1a.5.5 0 0 1-.5.5h-1a.5.5 0 0 1-.5-.5v-1zm3 0a.5.5 0 0 1 .5-.5h1a.5.5 0 0 1 .5.5v1a.5.5 0 0 1-.5.5h-1a.5.5 0 0 1-.5-.5v-1z"/><path d="M3.5 0a.5.5 0 0 1 .5.5V1h8V.5a.5.5 0 0 1 1 0V1h1a2 2 0 0 1 2 2v11a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V3a2 2 0 0 1 2-2h1V.5a.5.5 0 0 1 .5-.5zM1 4v10a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V4H1z"/></svg>
                            <?= $formattedDate ?>
                        </span>
                        <span class="news-meta-item">
                            <svg width="16" height="16" fill="currentColor" viewBox="0 0 16 16"><path d="M16 8s-3-5.5-8-5.5S0 8 0 8s3 5.5 8 5.5S16 8 16 8zM1.173 8a13.133 13.133 0 0 1 1.66-2.043C4.12 4.668 5.88 3.5 8 3.5c2.12 0 3.879 1.168 5.168 2.457A13.133 13.133 0 0 1 14.828 8c-.058.087-.122.183-.195.288-.335.48-.83 1.12-1.465 1.755C11.879 11.332 10.119 12.5 8 12.5c-2.12 0-3.879-1.168-5.168-2.457A13.134 13.134 0 0 1 1.172 8z"/><path d="M8 5.5a2.5 2.5 0 1 0 0 5 2.5 2.5 0 0 0 0-5zM4.5 8a3.5 3.5 0 1 1 7 0 3.5 3.5 0 0 1-7 0z"/></svg>
                            <?= number_format($news['views']) ?> views
                        </span>
                        <span class="news-meta-item text-muted">
                            <?= $timeAgo ?>
                        </span>
                    </div>
                </header>

                <?php if ($news['image']): ?>
                <img src="<?= e($news['image']) ?>" alt="<?= e($news['title']) ?>" class="news-image">
                <?php endif; ?>

                <div class="news-content">
                    <?= $news['content'] ?>
                </div>

                <footer class="news-footer">
                    <a href="/" class="back-link">
                        <svg width="20" height="20" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24">
                            <path stroke-linecap="round" stroke-linejoin="round" d="M10 19l-7-7m0 0l7-7m-7 7h18"></path>
                        </svg>
                        Back to Home
                    </a>
                </footer>
            </article>
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
