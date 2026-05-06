<?php
/**
 * PKO Website - Admin News Management
 */

require_once __DIR__ . '/../includes/config.php';

Security::setHeaders();

// Require login
Auth::require();

// Check admin access (GM level 99)
$currentUser = Auth::user();
$gmLevel = AccountModel::getGmLevel(Auth::id());
if ($gmLevel < 99) {
    flash('error', 'Access denied. You do not have permission to view this page.');
    redirect('/dashboard.php');
}

$action = $_GET['action'] ?? 'list';
$id = isset($_GET['id']) ? (int)$_GET['id'] : null;

$categories = NewsModel::getCategories();

// Handle form submissions BEFORE generating new CSRF token
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $csrfToken = $_POST['csrf_token'] ?? '';
    
    // Debug logging
    error_log("NEWS POST: action=" . ($_POST['action'] ?? 'none') . ", csrf_token=" . substr($csrfToken, 0, 10) . "...");
    
    if (!CSRF::verifyToken($csrfToken)) {
        error_log("NEWS POST: CSRF verification failed");
        flash('error', 'Invalid security token. Please try again.');
        redirect('/admin/news.php');
    }
    
    error_log("NEWS POST: CSRF passed, action=" . ($_POST['action'] ?? 'none'));
    $postAction = $_POST['action'] ?? '';
    
    if ($postAction === 'create' || $postAction === 'update') {
        $title = trim($_POST['title'] ?? '');
        $summary = trim($_POST['summary'] ?? '');
        $content = $_POST['content'] ?? '';
        $category = $_POST['category'] ?? 'update';
        $isPublished = isset($_POST['is_published']) ? 1 : 0;
        $isPinned = isset($_POST['is_pinned']) ? 1 : 0;
        
        // Handle image upload
        $imagePath = $_POST['existing_image'] ?? null;
        if (isset($_FILES['image']) && $_FILES['image']['error'] === UPLOAD_ERR_OK) {
            $uploadDir = __DIR__ . '/../assets/images/news/';
            if (!is_dir($uploadDir)) {
                mkdir($uploadDir, 0755, true);
            }
            
            $fileExt = strtolower(pathinfo($_FILES['image']['name'], PATHINFO_EXTENSION));
            $allowedExts = ['jpg', 'jpeg', 'png', 'gif', 'webp'];
            
            if (in_array($fileExt, $allowedExts)) {
                $fileName = 'news_' . time() . '_' . uniqid() . '.' . $fileExt;
                $targetPath = $uploadDir . $fileName;
                
                if (move_uploaded_file($_FILES['image']['tmp_name'], $targetPath)) {
                    $imagePath = '/assets/images/news/' . $fileName;
                }
            }
        }
        
        // Validation
        $errors = [];
        if (empty($title)) $errors[] = 'Title is required';
        if (empty($content)) $errors[] = 'Content is required';
        if (strlen($title) > 255) $errors[] = 'Title must be less than 255 characters';
        if (strlen($summary) > 500) $errors[] = 'Summary must be less than 500 characters';
        
        if (empty($errors)) {
            if ($postAction === 'create') {
                $slug = NewsModel::generateSlug($title);
                $newsId = NewsModel::create([
                    'title' => $title,
                    'slug' => $slug,
                    'summary' => $summary ?: null,
                    'content' => $content,
                    'image' => $imagePath,
                    'category' => $category,
                    'author' => $currentUser['name'],
                    'author_id' => Auth::id(),
                    'is_published' => $isPublished,
                    'is_pinned' => $isPinned
                ]);
                
                if ($newsId) {
                    flash('success', 'News article created successfully!');
                    redirect('/admin/news.php');
                } else {
                    flash('error', 'Failed to create news article.');
                }
            } else {
                // Update
                $editId = (int)$_POST['id'];
                $slug = NewsModel::generateSlug($title, $editId);
                
                $success = NewsModel::update($editId, [
                    'title' => $title,
                    'slug' => $slug,
                    'summary' => $summary ?: null,
                    'content' => $content,
                    'image' => $imagePath,
                    'category' => $category,
                    'is_published' => $isPublished,
                    'is_pinned' => $isPinned
                ]);
                
                if ($success) {
                    flash('success', 'News article updated successfully!');
                    redirect('/admin/news.php');
                } else {
                    flash('error', 'Failed to update news article.');
                }
            }
        } else {
            flash('error', implode('<br>', $errors));
        }
    } elseif ($postAction === 'delete') {
        $deleteId = (int)$_POST['id'];
        if (NewsModel::delete($deleteId)) {
            flash('success', 'News article deleted successfully!');
        } else {
            flash('error', 'Failed to delete news article.');
        }
        redirect('/admin/news.php');
    }
}

// Generate CSRF token for forms AFTER processing POST (so we don't overwrite the token being verified)
$pageToken = CSRF::generateToken();

// Get data for display
$newsData = [];
$editNews = null;

if ($action === 'list') {
    $page = isset($_GET['page']) ? max(1, (int)$_GET['page']) : 1;
    $newsData = NewsModel::getAll($page, 10, false);
} elseif ($action === 'edit' && $id) {
    $editNews = NewsModel::getById($id);
    if (!$editNews) {
        flash('error', 'News article not found.');
        redirect('/admin/news.php');
    }
}

$pageTitle = 'News Management';
require_once __DIR__ . '/../includes/header.php';
?>

<style>
    .news-editor {
        max-width: 900px;
        margin: 0 auto;
    }
    .form-group {
        margin-bottom: 1.5rem;
    }
    .form-label {
        display: block;
        margin-bottom: 0.5rem;
        font-weight: 500;
        color: #fff;
    }
    .form-input, .form-textarea, .form-select {
        width: 100%;
        padding: 12px 16px;
        background: rgba(255,255,255,0.05);
        border: 1px solid rgba(255,255,255,0.1);
        border-radius: 8px;
        color: #fff;
        font-size: 1rem;
        transition: border-color 0.2s;
    }
    .form-input:focus, .form-textarea:focus, .form-select:focus {
        outline: none;
        border-color: var(--gold);
    }
    .form-textarea {
        min-height: 300px;
        font-family: monospace;
        resize: vertical;
    }
    .form-select option {
        background: #1a1a2e;
        color: #fff;
    }
    .form-checkbox {
        display: flex;
        align-items: center;
        gap: 10px;
        cursor: pointer;
    }
    .form-checkbox input {
        width: 20px;
        height: 20px;
        accent-color: var(--gold);
    }
    .form-hint {
        font-size: 0.85rem;
        color: var(--text-muted);
        margin-top: 0.5rem;
    }
    .news-table {
        width: 100%;
        border-collapse: collapse;
    }
    .news-table th, .news-table td {
        padding: 1rem;
        text-align: left;
        border-bottom: 1px solid rgba(255,255,255,0.1);
    }
    .news-table th {
        color: var(--gold);
        font-weight: 600;
    }
    .news-table tr:hover {
        background: rgba(255,255,255,0.02);
    }
    .badge {
        display: inline-block;
        padding: 4px 10px;
        border-radius: 4px;
        font-size: 0.75rem;
        font-weight: 600;
    }
    .badge-success { background: #4CAF50; color: #fff; }
    .badge-warning { background: #FF9800; color: #000; }
    .badge-info { background: #2196F3; color: #fff; }
    .action-buttons {
        display: flex;
        gap: 8px;
    }
    .btn-sm {
        padding: 6px 12px;
        font-size: 0.85rem;
    }
    .image-preview {
        max-width: 200px;
        max-height: 150px;
        border-radius: 8px;
        margin-top: 0.5rem;
    }
    .category-badge {
        display: inline-flex;
        align-items: center;
        gap: 4px;
        padding: 4px 10px;
        border-radius: 12px;
        font-size: 0.8rem;
    }
    .pagination {
        display: flex;
        justify-content: center;
        gap: 8px;
        margin-top: 2rem;
    }
    .pagination a, .pagination span {
        padding: 8px 16px;
        background: rgba(255,255,255,0.05);
        border-radius: 4px;
        color: #fff;
        text-decoration: none;
    }
    .pagination a:hover {
        background: rgba(255,255,255,0.1);
    }
    .pagination .active {
        background: var(--gold);
        color: #000;
    }
</style>

<div class="container py-5">
    <?php $flash = getFlash(); if (!empty($flash) && isset($flash['type'])): ?>
    <div class="alert alert-<?= $flash['type'] === 'error' ? 'danger' : 'success' ?> mb-4" style="padding: 1rem; background: <?= $flash['type'] === 'error' ? 'rgba(244,67,54,0.2)' : 'rgba(76,175,80,0.2)' ?>; border-radius: 8px; border-left: 4px solid <?= $flash['type'] === 'error' ? '#f44336' : '#4CAF50' ?>;">
        <?= $flash['message'] ?? '' ?>
    </div>
    <?php endif; ?>

    <?php if ($action === 'list'): ?>
    <!-- News List -->
    <div class="d-flex justify-content-between align-items-center mb-4">
        <div>
            <h1 class="display-5 text-gradient">📰 News Management</h1>
            <p class="text-muted">Create and manage news articles</p>
        </div>
        <a href="/admin/news.php?action=create" class="btn btn-primary">
            ➕ Create New Article
        </a>
    </div>

    <div class="glass-panel p-4">
        <?php if (empty($newsData['news'])): ?>
        <p class="text-center text-muted py-4">No news articles yet. Create your first article!</p>
        <?php else: ?>
        <table class="news-table">
            <thead>
                <tr>
                    <th>Title</th>
                    <th>Category</th>
                    <th>Status</th>
                    <th>Views</th>
                    <th>Date</th>
                    <th>Actions</th>
                </tr>
            </thead>
            <tbody>
                <?php foreach ($newsData['news'] as $article): ?>
                <?php $cat = $categories[$article['category']] ?? ['name' => 'News', 'icon' => '📰', 'color' => '#888']; ?>
                <tr>
                    <td>
                        <a href="/news.php?slug=<?= e($article['slug']) ?>" target="_blank" style="color: #fff; text-decoration: none;">
                            <?php if ($article['is_pinned']): ?><span title="Pinned">📌</span><?php endif; ?>
                            <?= e($article['title']) ?>
                        </a>
                    </td>
                    <td>
                        <span class="category-badge" style="background: <?= $cat['color'] ?>20; color: <?= $cat['color'] ?>;">
                            <?= $cat['icon'] ?> <?= $cat['name'] ?>
                        </span>
                    </td>
                    <td>
                        <?php if ($article['is_published']): ?>
                        <span class="badge badge-success">Published</span>
                        <?php else: ?>
                        <span class="badge badge-warning">Draft</span>
                        <?php endif; ?>
                    </td>
                    <td><?= number_format($article['views']) ?></td>
                    <td><?= date('M j, Y', strtotime($article['created_at'])) ?></td>
                    <td>
                        <div class="action-buttons">
                            <a href="/admin/news.php?action=edit&id=<?= $article['id'] ?>" class="btn btn-ghost btn-sm">✏️ Edit</a>
                            <form method="POST" action="/admin/news.php" style="display:inline;" onsubmit="return confirm('Are you sure you want to delete this article?');">
                                <input type="hidden" name="csrf_token" value="<?= $pageToken ?>">
                                <input type="hidden" name="action" value="delete">
                                <input type="hidden" name="id" value="<?= $article['id'] ?>">
                                <button type="submit" class="btn btn-ghost btn-sm" style="color: #f44336;">🗑️ Delete</button>
                            </form>
                        </div>
                    </td>
                </tr>
                <?php endforeach; ?>
            </tbody>
        </table>

        <!-- Pagination -->
        <?php if ($newsData['totalPages'] > 1): ?>
        <div class="pagination">
            <?php for ($i = 1; $i <= $newsData['totalPages']; $i++): ?>
            <a href="/admin/news.php?page=<?= $i ?>" class="<?= $i === $newsData['page'] ? 'active' : '' ?>"><?= $i ?></a>
            <?php endfor; ?>
        </div>
        <?php endif; ?>
        <?php endif; ?>
    </div>

    <div class="mt-4">
        <a href="/admin/" class="btn btn-ghost">← Back to Admin Dashboard</a>
    </div>

    <?php elseif ($action === 'create' || $action === 'edit'): ?>
    <!-- Create/Edit Form -->
    <div class="news-editor">
        <div class="mb-4">
            <h1 class="display-5 text-gradient">
                <?= $action === 'create' ? '✍️ Create New Article' : '✏️ Edit Article' ?>
            </h1>
            <p class="text-muted">
                <?= $action === 'create' ? 'Write a new news article for players' : 'Update the news article' ?>
            </p>
        </div>

        <div class="glass-panel p-5">
            <form method="POST" enctype="multipart/form-data">
                <input type="hidden" name="csrf_token" value="<?= $pageToken ?>">
                <input type="hidden" name="action" value="<?= $action === 'create' ? 'create' : 'update' ?>">
                <?php if ($editNews): ?>
                <input type="hidden" name="id" value="<?= $editNews['id'] ?>">
                <input type="hidden" name="existing_image" value="<?= e($editNews['image'] ?? '') ?>">
                <?php endif; ?>

                <div class="form-group">
                    <label class="form-label">Title *</label>
                    <input type="text" name="title" class="form-input" required maxlength="255" 
                           value="<?= e($editNews['title'] ?? '') ?>" placeholder="Enter article title">
                </div>

                <div class="form-group">
                    <label class="form-label">Summary</label>
                    <input type="text" name="summary" class="form-input" maxlength="500"
                           value="<?= e($editNews['summary'] ?? '') ?>" placeholder="Brief summary (shown in news list)">
                    <p class="form-hint">Optional. A short description shown on the homepage.</p>
                </div>

                <div class="form-group">
                    <label class="form-label">Category</label>
                    <select name="category" class="form-select">
                        <?php foreach ($categories as $key => $cat): ?>
                        <option value="<?= $key ?>" <?= ($editNews['category'] ?? 'update') === $key ? 'selected' : '' ?>>
                            <?= $cat['icon'] ?> <?= $cat['name'] ?>
                        </option>
                        <?php endforeach; ?>
                    </select>
                </div>

                <div class="form-group">
                    <label class="form-label">Content * (HTML supported)</label>
                    <textarea name="content" class="form-textarea" required placeholder="Write your article content here. HTML tags are supported."><?= e($editNews['content'] ?? '') ?></textarea>
                    <p class="form-hint">Supported: &lt;p&gt;, &lt;h2&gt;, &lt;h3&gt;, &lt;ul&gt;, &lt;ol&gt;, &lt;li&gt;, &lt;strong&gt;, &lt;em&gt;, &lt;a&gt;, &lt;img&gt;</p>
                </div>

                <div class="form-group">
                    <label class="form-label">Featured Image</label>
                    <input type="file" name="image" class="form-input" accept="image/*">
                    <?php if (!empty($editNews['image'])): ?>
                    <img src="<?= e($editNews['image']) ?>" alt="Current image" class="image-preview">
                    <p class="form-hint">Current image will be kept if no new image is uploaded.</p>
                    <?php endif; ?>
                </div>

                <div class="form-group">
                    <label class="form-checkbox">
                        <input type="checkbox" name="is_published" value="1" <?= ($editNews['is_published'] ?? 1) ? 'checked' : '' ?>>
                        <span>Publish immediately</span>
                    </label>
                    <p class="form-hint">Uncheck to save as draft.</p>
                </div>

                <div class="form-group">
                    <label class="form-checkbox">
                        <input type="checkbox" name="is_pinned" value="1" <?= ($editNews['is_pinned'] ?? 0) ? 'checked' : '' ?>>
                        <span>📌 Pin to top</span>
                    </label>
                    <p class="form-hint">Pinned articles appear first in the news list.</p>
                </div>

                <div class="d-flex gap-3 mt-4">
                    <button type="submit" class="btn btn-primary">
                        <?= $action === 'create' ? '✅ Create Article' : '💾 Save Changes' ?>
                    </button>
                    <a href="/admin/news.php" class="btn btn-ghost">Cancel</a>
                </div>
            </form>
        </div>
    </div>
    <?php endif; ?>
</div>

<?php require_once __DIR__ . '/../includes/footer.php'; ?>
