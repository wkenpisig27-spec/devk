<?php
/**
 * PKO Website - News Archive
 */

require_once __DIR__ . '/includes/config.php';

Security::setHeaders();

$isLoggedIn = Auth::check();

$page = isset($_GET['page']) ? max(1, (int)$_GET['page']) : 1;
$category = isset($_GET['category']) ? $_GET['category'] : null;

// Get news with pagination
$newsData = NewsModel::getAll($page, 12, true, $category);
$categories = NewsModel::getCategories();

$pageTitle = 'News & Updates';
require_once __DIR__ . '/includes/header.php';
?>

<style>
    .news-hero {
        background: linear-gradient(135deg, rgba(30, 58, 95, 0.9), rgba(15, 23, 42, 0.95));
        padding: 5rem 0 4rem;
        text-align: center;
        border-bottom: 1px solid rgba(255,255,255,0.1);
        margin-top: 80px;
    }
    .news-hero h1 {
        font-size: 2.5rem;
        margin-bottom: 0.5rem;
    }
    .category-filters {
        display: flex;
        flex-wrap: wrap;
        justify-content: center;
        gap: 8px;
        margin-bottom: 2rem;
    }
    .category-filter {
        display: inline-flex;
        align-items: center;
        gap: 6px;
        padding: 8px 16px;
        background: rgba(255,255,255,0.05);
        border: 1px solid rgba(255,255,255,0.1);
        border-radius: 20px;
        color: #fff;
        text-decoration: none;
        font-size: 0.9rem;
        transition: all 0.2s;
    }
    .category-filter:hover, .category-filter.active {
        background: var(--gold);
        color: #000;
        border-color: var(--gold);
    }
    .news-grid {
        display: grid;
        grid-template-columns: repeat(auto-fill, minmax(320px, 1fr));
        gap: 1.5rem;
    }
    .news-card {
        background: rgba(255,255,255,0.03);
        border: 1px solid rgba(255,255,255,0.08);
        border-radius: 12px;
        overflow: hidden;
        transition: transform 0.2s, box-shadow 0.2s;
        text-decoration: none;
        color: inherit;
        display: flex;
        flex-direction: column;
    }
    .news-card:hover {
        transform: translateY(-4px);
        box-shadow: 0 12px 40px rgba(0,0,0,0.3);
        border-color: rgba(255,255,255,0.15);
    }
    .news-card-image {
        height: 180px;
        background-color: rgba(255,255,255,0.05);
        background-size: cover;
        background-position: center;
    }
    .news-card-image.no-image {
        display: flex;
        align-items: center;
        justify-content: center;
        font-size: 3rem;
    }
    .news-card-body {
        padding: 1.25rem;
        flex: 1;
        display: flex;
        flex-direction: column;
    }
    .news-card-category {
        display: inline-flex;
        align-items: center;
        gap: 4px;
        padding: 4px 10px;
        border-radius: 12px;
        font-size: 0.75rem;
        width: fit-content;
        margin-bottom: 0.75rem;
    }
    .news-card-title {
        font-size: 1.1rem;
        font-weight: 600;
        margin-bottom: 0.5rem;
        line-height: 1.4;
        color: #fff;
    }
    .news-card-summary {
        font-size: 0.9rem;
        color: var(--text-muted);
        line-height: 1.5;
        flex: 1;
    }
    .news-card-meta {
        display: flex;
        justify-content: space-between;
        align-items: center;
        font-size: 0.8rem;
        color: var(--text-muted);
        margin-top: 1rem;
        padding-top: 1rem;
        border-top: 1px solid rgba(255,255,255,0.05);
    }
    .pinned-badge {
        color: var(--gold);
    }
    .pagination {
        display: flex;
        justify-content: center;
        gap: 8px;
        margin-top: 3rem;
    }
    .pagination a, .pagination span {
        padding: 10px 18px;
        background: rgba(255,255,255,0.05);
        border-radius: 6px;
        color: #fff;
        text-decoration: none;
        transition: all 0.2s;
    }
    .pagination a:hover {
        background: rgba(255,255,255,0.1);
    }
    .pagination .active {
        background: var(--gold);
        color: #000;
    }
    .pagination .disabled {
        opacity: 0.5;
        pointer-events: none;
    }
    .empty-state {
        text-align: center;
        padding: 4rem 2rem;
        color: var(--text-muted);
    }
    .empty-state-icon {
        font-size: 4rem;
        margin-bottom: 1rem;
    }
</style>

<div class="news-hero">
    <div class="container">
        <h1 class="text-gradient">📰 News & Updates</h1>
        <p class="text-muted">Stay informed about server updates, events, and announcements</p>
    </div>
</div>

<div class="container" style="padding: 3rem 1rem 5rem;">
    <!-- Category Filters -->
    <div class="category-filters">
        <a href="/news-list.php" class="category-filter <?= !$category ? 'active' : '' ?>">
            📋 All
        </a>
        <?php foreach ($categories as $key => $cat): ?>
        <a href="/news-list.php?category=<?= $key ?>" class="category-filter <?= $category === $key ? 'active' : '' ?>">
            <?= $cat['icon'] ?> <?= $cat['name'] ?>
        </a>
        <?php endforeach; ?>
    </div>

    <?php if (!empty($newsData['news'])): ?>
    <!-- News Grid -->
    <div class="news-grid">
        <?php foreach ($newsData['news'] as $article): ?>
        <?php $cat = $categories[$article['category']] ?? ['name' => 'News', 'icon' => '📰', 'color' => '#888']; ?>
        <a href="/news.php?slug=<?= e($article['slug']) ?>" class="news-card">
            <?php if ($article['image']): ?>
            <div class="news-card-image" style="background-image: url('<?= e($article['image']) ?>');"></div>
            <?php else: ?>
            <div class="news-card-image no-image"><?= $cat['icon'] ?></div>
            <?php endif; ?>
            
            <div class="news-card-body">
                <div style="display: flex; align-items: center; gap: 8px;">
                    <span class="news-card-category" style="background: <?= $cat['color'] ?>20; color: <?= $cat['color'] ?>;">
                        <?= $cat['icon'] ?> <?= $cat['name'] ?>
                    </span>
                    <?php if ($article['is_pinned']): ?>
                    <span class="pinned-badge" title="Pinned">📌</span>
                    <?php endif; ?>
                </div>
                
                <h3 class="news-card-title"><?= e($article['title']) ?></h3>
                
                <?php if ($article['summary']): ?>
                <p class="news-card-summary"><?= e(substr($article['summary'], 0, 150)) ?><?= strlen($article['summary']) > 150 ? '...' : '' ?></p>
                <?php endif; ?>
                
                <div class="news-card-meta">
                    <span><?= date('M j, Y', strtotime($article['created_at'])) ?></span>
                    <span>👁 <?= number_format($article['views']) ?></span>
                </div>
            </div>
        </a>
        <?php endforeach; ?>
    </div>

    <!-- Pagination -->
    <?php if ($newsData['totalPages'] > 1): ?>
    <div class="pagination">
        <?php if ($page > 1): ?>
        <a href="/news-list.php?page=<?= $page - 1 ?><?= $category ? '&category=' . $category : '' ?>">← Prev</a>
        <?php else: ?>
        <span class="disabled">← Prev</span>
        <?php endif; ?>
        
        <?php 
        $start = max(1, $page - 2);
        $end = min($newsData['totalPages'], $page + 2);
        ?>
        
        <?php if ($start > 1): ?>
        <a href="/news-list.php?page=1<?= $category ? '&category=' . $category : '' ?>">1</a>
        <?php if ($start > 2): ?><span style="padding: 10px 4px;">...</span><?php endif; ?>
        <?php endif; ?>
        
        <?php for ($i = $start; $i <= $end; $i++): ?>
        <a href="/news-list.php?page=<?= $i ?><?= $category ? '&category=' . $category : '' ?>" class="<?= $i === $page ? 'active' : '' ?>"><?= $i ?></a>
        <?php endfor; ?>
        
        <?php if ($end < $newsData['totalPages']): ?>
        <?php if ($end < $newsData['totalPages'] - 1): ?><span style="padding: 10px 4px;">...</span><?php endif; ?>
        <a href="/news-list.php?page=<?= $newsData['totalPages'] ?><?= $category ? '&category=' . $category : '' ?>"><?= $newsData['totalPages'] ?></a>
        <?php endif; ?>
        
        <?php if ($page < $newsData['totalPages']): ?>
        <a href="/news-list.php?page=<?= $page + 1 ?><?= $category ? '&category=' . $category : '' ?>">Next →</a>
        <?php else: ?>
        <span class="disabled">Next →</span>
        <?php endif; ?>
    </div>
    <?php endif; ?>

    <?php else: ?>
    <!-- Empty State -->
    <div class="empty-state">
        <div class="empty-state-icon">📭</div>
        <h3>No News Found</h3>
        <p>There are no news articles <?= $category ? 'in this category ' : '' ?>yet. Check back soon!</p>
        <?php if ($category): ?>
        <a href="/news-list.php" class="btn btn-ghost mt-4">View All News</a>
        <?php endif; ?>
    </div>
    <?php endif; ?>
</div>

<?php require_once __DIR__ . '/includes/footer.php'; ?>
