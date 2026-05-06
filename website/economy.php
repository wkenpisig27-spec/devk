<?php
/**
 * PKO Website - Economy & Market Trends Page
 * 
 * Shows server economy statistics, market trends, and item database browser
 * RESTRICTED: Admin/GM access only
 */

require_once __DIR__ . '/includes/config.php';

// Require authentication
Auth::require();

// Check if user is admin or GM
$currentUser = Auth::user();
$userId = Auth::id();
$gmLevel = AccountModel::getGmLevel($userId);

// Allow access only if: username is 'admin' OR GM level >= 1
$isAdmin = strtolower($currentUser['name']) === 'admin';
$isGM = $gmLevel >= 1;

if (!$isAdmin && !$isGM) {
    flash('error', 'Access denied. This page is restricted to administrators and GMs only.');
    redirect('/dashboard.php');
}

// Get economy data with error handling for when DB is unavailable
$dbAvailable = true;
$economyStats = ['total_gold' => 0, 'average_gold' => 0, 'guild_treasury' => 0, 'wealth_distribution' => []];
$classDistribution = [];
$levelDistribution = [];
$topRichest = [];
$shopStats = ['top_items' => [], 'total_purchases' => 0];
$activeStalls = [];
$stallsByMap = [];
$recentActivity = [];

try {
    $economyStats = EconomyModel::getServerStats();
    $classDistribution = EconomyModel::getClassDistribution();
    $levelDistribution = EconomyModel::getLevelDistribution();
    $topRichest = EconomyModel::getTopRichest(10);
    $shopStats = EconomyModel::getShopStats();
    $activeStalls = EconomyModel::getActiveStalls(20);
    $stallsByMap = EconomyModel::getStallsByMap();
    $recentActivity = EconomyModel::getRecentActivity(7);
} catch (Exception $e) {
    $dbAvailable = false;
    error_log("Economy page DB error: " . $e->getMessage());
}

// Item search (doesn't require DB)
$searchQuery = trim($_GET['search'] ?? '');
$searchResults = [];
if ($searchQuery !== '') {
    $searchResults = searchItems($searchQuery, 50);
}

Security::setHeaders();
$stats = getServerStats();
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <?= metaTags('Statistics & Market', 'View server statistics, market trends, and browse the item database in ' . SERVER_NAME) ?>
    <link rel="stylesheet" href="<?= asset('css/style.css') ?>">
    <link rel="icon" type="image/png" href="<?= asset('images/favicon.png') ?>">
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        .economy-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
            gap: 1.5rem;
            margin-bottom: 2rem;
        }
        .stat-card {
            background: var(--card-bg);
            border: 1px solid var(--border-color);
            border-radius: 12px;
            padding: 1.5rem;
            text-align: center;
        }
        .stat-card .stat-icon {
            font-size: 2.5rem;
            margin-bottom: 0.5rem;
        }
        .stat-card .stat-value {
            font-size: 1.8rem;
            font-weight: 700;
            color: var(--gold);
            margin-bottom: 0.25rem;
        }
        .stat-card .stat-label {
            color: var(--text-muted);
            font-size: 0.9rem;
        }
        .chart-container {
            background: var(--card-bg);
            border: 1px solid var(--border-color);
            border-radius: 12px;
            padding: 1.5rem;
            margin-bottom: 1.5rem;
        }
        .chart-container h3 {
            margin-bottom: 1rem;
            font-size: 1.1rem;
            color: var(--text-primary);
        }
        .chart-wrapper {
            position: relative;
            height: 300px;
        }
        .item-search-box {
            background: var(--card-bg);
            border: 1px solid var(--border-color);
            border-radius: 12px;
            padding: 1.5rem;
            margin-bottom: 1.5rem;
        }
        .search-input-group {
            display: flex;
            gap: 0.5rem;
        }
        .search-input-group input {
            flex: 1;
            padding: 0.75rem 1rem;
            border: 1px solid var(--border-color);
            border-radius: 8px;
            background: var(--bg-primary);
            color: var(--text-primary);
            font-size: 1rem;
        }
        .search-input-group input:focus {
            outline: none;
            border-color: var(--primary);
        }
        .item-grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(250px, 1fr));
            gap: 1rem;
        }
        .item-card {
            background: var(--bg-secondary);
            border: 1px solid var(--border-color);
            border-radius: 8px;
            padding: 1rem;
            display: flex;
            align-items: center;
            gap: 1rem;
            transition: border-color 0.2s;
        }
        .item-card:hover {
            border-color: var(--primary);
        }
        .item-icon {
            width: 48px;
            height: 48px;
            background: var(--bg-primary);
            border-radius: 8px;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 1.5rem;
        }
        .item-info {
            flex: 1;
            min-width: 0;
        }
        .item-name {
            font-weight: 600;
            color: var(--text-primary);
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
        }
        .item-meta {
            font-size: 0.85rem;
            color: var(--text-muted);
        }
        .item-price {
            color: var(--gold);
            font-weight: 600;
        }
        .two-col-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 1.5rem;
        }
        @media (max-width: 768px) {
            .two-col-grid {
                grid-template-columns: 1fr;
            }
        }
        .market-listing {
            display: flex;
            align-items: center;
            gap: 1rem;
            padding: 0.75rem;
            background: var(--bg-secondary);
            border-radius: 8px;
            margin-bottom: 0.5rem;
        }
        .market-listing .seller-name {
            font-weight: 600;
            color: var(--text-primary);
        }
        .market-listing .stall-title {
            color: var(--text-muted);
            font-size: 0.9rem;
        }
        .market-listing .location {
            margin-left: auto;
            font-size: 0.85rem;
            color: var(--primary);
        }
        .wealth-bar {
            height: 24px;
            background: var(--bg-secondary);
            border-radius: 4px;
            overflow: hidden;
            margin-bottom: 0.5rem;
        }
        .wealth-bar-fill {
            height: 100%;
            background: linear-gradient(90deg, var(--gold), #f0c020);
            display: flex;
            align-items: center;
            padding-left: 8px;
            color: #000;
            font-size: 0.8rem;
            font-weight: 600;
        }
        .tab-container {
            display: flex;
            gap: 0.5rem;
            margin-bottom: 1.5rem;
            flex-wrap: wrap;
        }
        .tab-btn {
            padding: 0.5rem 1rem;
            border: 1px solid var(--border-color);
            background: var(--card-bg);
            border-radius: 8px;
            color: var(--text-primary);
            cursor: pointer;
            transition: all 0.2s;
        }
        .tab-btn:hover, .tab-btn.active {
            background: var(--primary);
            border-color: var(--primary);
            color: white;
        }
        .tab-content {
            display: none;
        }
        .tab-content.active {
            display: block;
        }
        
        /* Supply & Demand Tab Styles */
        .item-list {
            max-height: 300px;
            overflow-y: auto;
        }
        .item-list::-webkit-scrollbar {
            width: 6px;
        }
        .item-list::-webkit-scrollbar-track {
            background: rgba(255,255,255,0.05);
            border-radius: 3px;
        }
        .item-list::-webkit-scrollbar-thumb {
            background: var(--primary);
            border-radius: 3px;
        }
        .text-warning { color: #f97316; }
        .text-info { color: #3b82f6; }
        .text-danger { color: #ef4444; }
        .text-success { color: #22c55e; }
        .text-muted { color: #666; }
        
        /* Data table styling */
        .data-table {
            width: 100%;
            border-collapse: collapse;
        }
        .data-table th,
        .data-table td {
            padding: 0.75rem 1rem;
            text-align: left;
            border-bottom: 1px solid rgba(255,255,255,0.05);
        }
        .data-table th {
            background: rgba(255,255,255,0.02);
            font-weight: 600;
            font-size: 0.85rem;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            color: #888;
        }
        .data-table tr:hover {
            background: rgba(212, 175, 55, 0.05);
        }
        .table-responsive {
            overflow-x: auto;
            max-height: 600px;
            overflow-y: auto;
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
                    <li><a href="/leaderboard.php" class="navbar-link">Leaderboard</a></li>
                    <li><a href="/economy.php" class="navbar-link active">Statistics</a></li>
                    <li><a href="/download.php" class="navbar-link">Download</a></li>
                    <li><a href="https://discord.com/invite/NHmFsuMpS5" target="_blank" class="navbar-link">Discord</a></li>
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
                <h1 class="title-section text-center">📊 Server Statistics</h1>
                <p class="text-secondary text-lg">Track market trends and explore the item database</p>
            </div>

            <?php if (!$dbAvailable): ?>
            <!-- Database Unavailable Warning -->
            <div style="background: rgba(239, 68, 68, 0.1); border: 1px solid #ef4444; border-radius: 12px; padding: 1.5rem; margin-bottom: 2rem; text-align: center;">
                <div style="font-size: 2rem; margin-bottom: 0.5rem;">⚠️</div>
                <h3 style="color: #ef4444; margin-bottom: 0.5rem;">Database Unavailable</h3>
                <p class="text-muted">SQL Server is not running. Statistics are unavailable, but you can still browse the Item Database!</p>
            </div>
            <?php endif; ?>

            <!-- Economy Overview Cards -->
            <div class="economy-grid">
                <div class="stat-card">
                    <div class="stat-icon">💰</div>
                    <div class="stat-value"><?= formatNumber($economyStats['total_gold']) ?></div>
                    <div class="stat-label">Total Gold in Circulation</div>
                </div>
                <div class="stat-card">
                    <div class="stat-icon">👤</div>
                    <div class="stat-value"><?= formatNumber($economyStats['average_gold']) ?></div>
                    <div class="stat-label">Average Gold per Player</div>
                </div>
                <div class="stat-card">
                    <div class="stat-icon">🏦</div>
                    <div class="stat-value"><?= formatNumber($economyStats['guild_treasury']) ?></div>
                    <div class="stat-label">Guild Treasury Total</div>
                </div>
                <div class="stat-card">
                    <div class="stat-icon">🏪</div>
                    <div class="stat-value"><?= count($activeStalls) ?></div>
                    <div class="stat-label">Active Market Stalls</div>
                </div>
            </div>

            <!-- Tab Navigation -->
            <div class="tab-container">
                <button class="tab-btn active" data-tab="overview">📈 Overview</button>
                <button class="tab-btn" data-tab="supply-demand">📊 Supply & Demand</button>
                <button class="tab-btn" data-tab="market">🏪 Market Listings</button>
                <button class="tab-btn" data-tab="items">📦 Item Database</button>
                <button class="tab-btn" data-tab="shop">🛒 Shop Trends</button>
            </div>

            <!-- Overview Tab -->
            <div class="tab-content active" id="tab-overview">
                <div class="two-col-grid">
                    <!-- Level Distribution Chart -->
                    <div class="chart-container">
                        <h3>📊 Level Distribution</h3>
                        <div class="chart-wrapper">
                            <canvas id="levelChart"></canvas>
                        </div>
                    </div>

                    <!-- Class Distribution Chart -->
                    <div class="chart-container">
                        <h3>⚔️ Class Distribution</h3>
                        <div class="chart-wrapper">
                            <canvas id="classChart"></canvas>
                        </div>
                    </div>
                </div>

                <!-- Top Richest Players -->
                <div class="chart-container">
                    <h3>🏆 Top 10 Richest Players</h3>
                    <div class="table-container">
                        <table class="table">
                            <thead>
                                <tr>
                                    <th>Rank</th>
                                    <th>Character</th>
                                    <th>Class</th>
                                    <th>Level</th>
                                    <th>Gold</th>
                                </tr>
                            </thead>
                            <tbody>
                                <?php foreach ($topRichest as $index => $player): ?>
                                <tr>
                                    <td>
                                        <span class="leaderboard-rank <?= $index === 0 ? 'gold' : ($index === 1 ? 'silver' : ($index === 2 ? 'bronze' : '')) ?>">
                                            <?= $index + 1 ?>
                                        </span>
                                    </td>
                                    <td class="font-semibold"><?= e($player['cha_name']) ?></td>
                                    <td class="text-muted"><?= e(getClassName($player['job'] ?? '')) ?></td>
                                    <td><?= $player['degree'] ?></td>
                                    <td class="text-gold font-semibold"><?= number_format($player['gd']) ?></td>
                                </tr>
                                <?php endforeach; ?>
                            </tbody>
                        </table>
                    </div>
                </div>
            </div>

            <!-- Supply & Demand Analytics Tab -->
            <div class="tab-content" id="tab-supply-demand">
                <div class="two-col-grid">
                    <!-- Supply Trend Chart -->
                    <div class="chart-container">
                        <h3>📈 Item Supply Over Time</h3>
                        <div class="chart-wrapper" style="height: 300px;">
                            <canvas id="supplyTrendChart"></canvas>
                        </div>
                    </div>

                    <!-- Top Supplied Items -->
                    <div class="chart-container">
                        <h3>📦 Current Inventory Distribution</h3>
                        <div class="chart-wrapper" style="height: 300px;">
                            <canvas id="supplyDistributionChart"></canvas>
                        </div>
                    </div>
                </div>

                <!-- Item Supply/Demand Table -->
                <div class="chart-container">
                    <h3>📊 Supply & Demand Analysis</h3>
                    <p class="text-muted" style="margin-bottom: 1rem;">
                        Real-time analysis of item supply (player inventories) vs market demand (stall listings).
                        <strong>Scarcity Index:</strong> Higher = rarer item. <strong>Supply Ratio:</strong> Items in inventory vs items listed for sale.
                    </p>
                    <div id="supplyDemandTable" class="table-responsive">
                        <table class="data-table">
                            <thead>
                                <tr>
                                    <th>Item</th>
                                    <th>Total Supply</th>
                                    <th>Players Holding</th>
                                    <th>Listed for Sale</th>
                                    <th>Scarcity Index</th>
                                    <th>Status</th>
                                </tr>
                            </thead>
                            <tbody id="supplyDemandBody">
                                <tr>
                                    <td colspan="6" class="text-center text-muted">Loading analytics data...</td>
                                </tr>
                            </tbody>
                        </table>
                    </div>
                </div>

                <!-- Oversupply vs Undersupply -->
                <div class="two-col-grid">
                    <div class="chart-container">
                        <h3 style="color: var(--accent-success);">📈 Oversupplied Items</h3>
                        <p class="text-muted">Items with high supply relative to demand - potential price drops.</p>
                        <div id="oversupplyList" class="item-list">
                            <div class="text-muted text-center">Loading...</div>
                        </div>
                    </div>
                    <div class="chart-container">
                        <h3 style="color: var(--accent-danger);">📉 Undersupplied Items</h3>
                        <p class="text-muted">Rare items with low availability - potential value increase.</p>
                        <div id="undersupplyList" class="item-list">
                            <div class="text-muted text-center">Loading...</div>
                        </div>
                    </div>
                </div>
            </div>

            <!-- Market Listings Tab -->
            <div class="tab-content" id="tab-market">
                <?php if (empty($activeStalls)): ?>
                <div class="chart-container text-center">
                    <div style="padding: 3rem;">
                        <div style="font-size: 4rem; margin-bottom: 1rem;">🏪</div>
                        <h3>No Active Market Stalls</h3>
                        <p class="text-muted">There are currently no offline stalls active. Check back later!</p>
                    </div>
                </div>
                <?php else: ?>
                <div class="two-col-grid">
                    <div class="chart-container">
                        <h3>📍 Stalls by Location</h3>
                        <div class="chart-wrapper">
                            <canvas id="stallMapChart"></canvas>
                        </div>
                    </div>
                    <div class="chart-container">
                        <h3>🏪 Active Stalls (<?= count($activeStalls) ?>)</h3>
                        <div style="max-height: 300px; overflow-y: auto;">
                            <?php foreach ($activeStalls as $stall): ?>
                            <div class="market-listing">
                                <div>
                                    <div class="seller-name"><?= e($stall['cha_name']) ?></div>
                                    <div class="stall-title"><?= e($stall['stall_title']) ?> (<?= $stall['item_count'] ?> items)</div>
                                </div>
                                <div class="location">📍 <?= e($stall['map_name']) ?></div>
                            </div>
                            <?php endforeach; ?>
                        </div>
                    </div>
                </div>
                <?php endif; ?>
            </div>

            <!-- Item Database Tab -->
            <div class="tab-content" id="tab-items">
                <div class="item-search-box">
                    <h3 style="margin-bottom: 1rem;">🔍 Search Items</h3>
                    <form method="GET" action="" class="search-input-group">
                        <input type="hidden" name="tab" value="items">
                        <input 
                            type="text" 
                            name="search" 
                            placeholder="Search for items by name..." 
                            value="<?= e($searchQuery) ?>"
                            autocomplete="off"
                        >
                        <button type="submit" class="btn btn-primary">Search</button>
                    </form>
                </div>

                <?php if ($searchQuery !== ''): ?>
                <div class="chart-container">
                    <h3>Search Results for "<?= e($searchQuery) ?>" (<?= count($searchResults) ?> found)</h3>
                    <?php if (empty($searchResults)): ?>
                    <p class="text-muted">No items found matching your search.</p>
                    <?php else: ?>
                    <div class="item-grid">
                        <?php foreach ($searchResults as $item): ?>
                        <div class="item-card">
                            <div class="item-icon">
                                <?php 
                                $iconUrl = !empty($item['icon']) ? '/assets/icons/items/' . $item['icon'] . '.png' : '/assets/images/item-placeholder.svg';
                                ?>
                                <img src="<?= e($iconUrl) ?>" alt="<?= e($item['name']) ?>" style="width: 48px; height: 48px; object-fit: contain;" onerror="this.src='/assets/images/item-placeholder.svg'">
                            </div>
                            <div class="item-info">
                                <div class="item-name"><?= e($item['name']) ?></div>
                                <div class="item-meta">
                                    <?= e(getItemTypeName($item['type'])) ?> 
                                    <?php if ($item['level'] > 0): ?>
                                    • Lv.<?= $item['level'] ?>
                                    <?php endif; ?>
                                </div>
                                <?php if ($item['price'] > 0): ?>
                                <div class="item-price">💰 <?= number_format($item['price']) ?> gold</div>
                                <?php endif; ?>
                            </div>
                        </div>
                        <?php endforeach; ?>
                    </div>
                    <?php endif; ?>
                </div>
                <?php else: ?>
                <div class="chart-container">
                    <h3>Browse by Category</h3>
                    <p class="text-muted mb-4">Enter a search term above or click a category below to browse items.</p>
                    <div style="display: flex; flex-wrap: wrap; gap: 0.5rem;">
                        <?php 
                        $categories = ['Sword', 'Bow', 'Gun', 'Staff', 'Armor', 'Helm', 'Ring', 'Gem', 'Potion', 'Scroll'];
                        foreach ($categories as $cat): 
                        ?>
                        <a href="?search=<?= urlencode($cat) ?>#tab-items" class="btn btn-ghost"><?= $cat ?></a>
                        <?php endforeach; ?>
                    </div>
                </div>
                <?php endif; ?>
            </div>

            <!-- Shop Trends Tab -->
            <div class="tab-content" id="tab-shop">
                <div class="two-col-grid">
                    <div class="chart-container">
                        <h3>🛒 Most Popular Shop Items</h3>
                        <?php if (empty($shopStats['top_items'])): ?>
                        <p class="text-muted">No shop purchase data available yet.</p>
                        <?php else: ?>
                        <div class="table-container">
                            <table class="table">
                                <thead>
                                    <tr>
                                        <th>Item</th>
                                        <th>Price</th>
                                        <th>Purchases</th>
                                    </tr>
                                </thead>
                                <tbody>
                                    <?php foreach ($shopStats['top_items'] as $item): ?>
                                    <tr>
                                        <td class="font-semibold"><?= e($item['item_name']) ?></td>
                                        <td class="text-primary"><?= number_format($item['price']) ?> CP</td>
                                        <td><?= number_format($item['purchase_count']) ?></td>
                                    </tr>
                                    <?php endforeach; ?>
                                </tbody>
                            </table>
                        </div>
                        <?php endif; ?>
                    </div>
                    <div class="chart-container">
                        <h3>📈 Shop Statistics</h3>
                        <div class="stat-card" style="margin-bottom: 1rem;">
                            <div class="stat-icon">🛍️</div>
                            <div class="stat-value"><?= number_format($shopStats['total_purchases']) ?></div>
                            <div class="stat-label">Total Shop Purchases</div>
                        </div>
                        <p class="text-muted">Shop purchases are tracked when players buy items from the web shop using credits earned through voting.</p>
                    </div>
                </div>

                <!-- Recent Activity -->
                <div class="chart-container">
                    <h3>📅 New Characters (Last 7 Days)</h3>
                    <div class="chart-wrapper" style="height: 200px;">
                        <canvas id="activityChart"></canvas>
                    </div>
                </div>
            </div>
        </div>
    </section>

    <?php require_once __DIR__ . '/includes/footer.php'; ?>

    <script src="<?= asset('js/main.js') ?>"></script>
    <script>
        // Tab switching
        document.querySelectorAll('.tab-btn').forEach(btn => {
            btn.addEventListener('click', () => {
                // Remove active from all tabs
                document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
                document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
                
                // Add active to clicked tab
                btn.classList.add('active');
                document.getElementById('tab-' + btn.dataset.tab).classList.add('active');
                
                // Update URL without reload
                const url = new URL(window.location);
                url.hash = 'tab-' + btn.dataset.tab;
                history.pushState({}, '', url);
            });
        });

        // Handle hash on load
        if (window.location.hash) {
            const tabId = window.location.hash.replace('#tab-', '');
            const tabBtn = document.querySelector(`.tab-btn[data-tab="${tabId}"]`);
            if (tabBtn) tabBtn.click();
        }

        // Chart.js configuration
        Chart.defaults.color = '#a0aec0';
        Chart.defaults.borderColor = 'rgba(255,255,255,0.1)';

        try {
            // Level Distribution Chart
            const levelData = <?= json_encode($levelDistribution ?: []) ?>;
            if (levelData && levelData.length > 0 && document.getElementById('levelChart')) {
                new Chart(document.getElementById('levelChart'), {
                    type: 'bar',
                    data: {
                        labels: levelData.map(d => d.level_range),
                        datasets: [{
                            label: 'Players',
                            data: levelData.map(d => d.count),
                            backgroundColor: 'rgba(212, 175, 55, 0.8)',
                            borderColor: 'rgba(212, 175, 55, 1)',
                            borderWidth: 1
                        }]
                    },
                    options: {
                        responsive: true,
                        maintainAspectRatio: false,
                        plugins: {
                            legend: { display: false }
                        },
                        scales: {
                            y: {
                                beginAtZero: true,
                                grid: { color: 'rgba(255,255,255,0.05)' }
                            },
                            x: {
                                grid: { display: false }
                            }
                        }
                    }
                });
            }
        } catch (e) { console.error('Level chart error:', e); }

        try {
            // Class Distribution Chart
            const classData = <?= json_encode($classDistribution ?: []) ?>;
            if (classData && classData.length > 0 && document.getElementById('classChart')) {
                new Chart(document.getElementById('classChart'), {
                    type: 'doughnut',
                    data: {
                        labels: classData.map(d => d.job_class || 'Newbie'),
                        datasets: [{
                            data: classData.map(d => d.count),
                            backgroundColor: [
                                '#d4af37', '#3b82f6', '#22c55e', '#ef4444',
                                '#a855f7', '#f97316', '#06b6d4', '#ec4899',
                                '#84cc16', '#6366f1'
                            ]
                        }]
                    },
                    options: {
                        responsive: true,
                        maintainAspectRatio: false,
                        plugins: {
                            legend: {
                                position: 'right',
                                labels: { padding: 10 }
                            }
                        }
                    }
                });
            }
        } catch (e) { console.error('Class chart error:', e); }

        try {
            // Stalls by Map Chart (if data exists)
            const stallsData = <?= json_encode($stallsByMap ?: []) ?>;
            if (stallsData && stallsData.length > 0 && document.getElementById('stallMapChart')) {
                new Chart(document.getElementById('stallMapChart'), {
                    type: 'pie',
                    data: {
                        labels: stallsData.map(d => d.map_name),
                        datasets: [{
                            data: stallsData.map(d => d.stall_count),
                            backgroundColor: [
                                '#d4af37', '#3b82f6', '#22c55e', '#ef4444',
                                '#a855f7', '#f97316', '#06b6d4', '#ec4899'
                            ]
                        }]
                    },
                    options: {
                        responsive: true,
                        maintainAspectRatio: false
                    }
                });
            }
        } catch (e) { console.error('Stalls chart error:', e); }

        try {
            // Activity Chart
            const activityData = <?= json_encode($recentActivity ?: []) ?>;
            if (activityData && activityData.length > 0 && document.getElementById('activityChart')) {
                new Chart(document.getElementById('activityChart'), {
                    type: 'line',
                    data: {
                        labels: activityData.map(d => d.date).reverse(),
                        datasets: [{
                            label: 'New Characters',
                            data: activityData.map(d => d.new_characters).reverse(),
                            borderColor: '#22c55e',
                            backgroundColor: 'rgba(34, 197, 94, 0.1)',
                            fill: true,
                            tension: 0.3
                        }]
                    },
                    options: {
                        responsive: true,
                        maintainAspectRatio: false,
                        plugins: {
                            legend: { display: false }
                        },
                        scales: {
                            y: {
                                beginAtZero: true,
                                grid: { color: 'rgba(255,255,255,0.05)' }
                            },
                            x: {
                                grid: { display: false }
                            }
                        }
                    }
                });
            }
        } catch (e) { console.error('Activity chart error:', e); }


        // ========================================
        // Supply & Demand Analytics
        // ========================================
        
        let supplyTrendChart = null;
        let supplyDistributionChart = null;
        let supplyDemandLoaded = false;

        // Load Supply & Demand data when tab is clicked
        const supplyDemandTab = document.querySelector('[data-tab="supply-demand"]');
        if (supplyDemandTab) {
            supplyDemandTab.addEventListener('click', function() {
                if (!supplyDemandLoaded) {
                    loadSupplyDemandData();
                    supplyDemandLoaded = true;
                }
            });
        }
        
        // Also load if Supply & Demand tab is already active (e.g. from hash navigation)
        if (document.getElementById('tab-supply-demand')?.classList.contains('active')) {
            loadSupplyDemandData();
            supplyDemandLoaded = true;
        }

        async function loadSupplyDemandData() {
            try {
                // Fetch item distribution from API
                const response = await fetch('/api/economy.php?action=item-distribution');
                const data = await response.json();
                
                if (!data.success) {
                    console.error('Failed to load item distribution:', data.error);
                    return;
                }

                const items = data.items || [];
                
                // Calculate statistics
                const totalSupply = items.reduce((sum, item) => sum + parseInt(item.total_quantity || 0), 0);
                const uniqueTypes = items.length;
                const playersWithItems = [...new Set(items.map(i => i.player_count))].reduce((a, b) => Math.max(a, parseInt(b) || 0), 0);
                
                // Render charts
                renderSupplyDistributionChart(items.slice(0, 10));
                renderSupplyTrendChart(items);
                
                // Populate supply/demand table
                populateSupplyDemandTable(items);
                
                // Categorize items for oversupply/undersupply lists
                categorizeSupplyDemand(items);

            } catch (error) {
                console.error('Error loading supply/demand data:', error);
            }
        }

        function renderSupplyDistributionChart(topItems) {
            const ctx = document.getElementById('supplyDistributionChart');
            if (!ctx) return;

            if (supplyDistributionChart) {
                supplyDistributionChart.destroy();
            }

            supplyDistributionChart = new Chart(ctx, {
                type: 'bar',
                data: {
                    labels: topItems.map(i => i.name || `Item #${i.item_id}`),
                    datasets: [{
                        label: 'Total Quantity',
                        data: topItems.map(i => parseInt(i.total_quantity) || 0),
                        backgroundColor: [
                            '#d4af37', '#3b82f6', '#22c55e', '#ef4444', '#a855f7',
                            '#f97316', '#06b6d4', '#ec4899', '#84cc16', '#14b8a6'
                        ],
                        borderRadius: 4
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    indexAxis: 'y',
                    plugins: {
                        legend: { display: false }
                    },
                    scales: {
                        x: {
                            beginAtZero: true,
                            grid: { color: 'rgba(255,255,255,0.05)' }
                        },
                        y: {
                            grid: { display: false },
                            ticks: {
                                font: { size: 11 },
                                callback: function(value) {
                                    const label = this.getLabelForValue(value);
                                    return label.length > 15 ? label.substring(0, 15) + '...' : label;
                                }
                            }
                        }
                    }
                }
            });
        }

        function renderSupplyTrendChart(items) {
            const ctx = document.getElementById('supplyTrendChart');
            if (!ctx) return;

            if (supplyTrendChart) {
                supplyTrendChart.destroy();
            }

            // Group items by category based on quantity ranges
            const categories = {
                'Abundant (100+)': items.filter(i => parseInt(i.total_quantity) >= 100).length,
                'Common (50-99)': items.filter(i => parseInt(i.total_quantity) >= 50 && parseInt(i.total_quantity) < 100).length,
                'Uncommon (10-49)': items.filter(i => parseInt(i.total_quantity) >= 10 && parseInt(i.total_quantity) < 50).length,
                'Rare (2-9)': items.filter(i => parseInt(i.total_quantity) >= 2 && parseInt(i.total_quantity) < 10).length,
                'Unique (1)': items.filter(i => parseInt(i.total_quantity) === 1).length
            };

            supplyTrendChart = new Chart(ctx, {
                type: 'doughnut',
                data: {
                    labels: Object.keys(categories),
                    datasets: [{
                        data: Object.values(categories),
                        backgroundColor: [
                            '#22c55e', '#3b82f6', '#d4af37', '#f97316', '#ef4444'
                        ],
                        borderWidth: 2,
                        borderColor: '#1a1a2e'
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: {
                        legend: {
                            position: 'right',
                            labels: {
                                padding: 15,
                                usePointStyle: true
                            }
                        }
                    }
                }
            });
        }

        function populateSupplyDemandTable(items) {
            const tbody = document.getElementById('supplyDemandBody');
            if (!tbody) return;

            const maxQty = Math.max(...items.map(i => parseInt(i.total_quantity) || 1), 1);

            if (items.length === 0) {
                tbody.innerHTML = '<tr><td colspan="6" class="text-center text-muted">No item data available</td></tr>';
                return;
            }

            tbody.innerHTML = items.slice(0, 50).map(item => {
                const qty = parseInt(item.total_quantity) || 0;
                const scarcity = ((1 - qty / maxQty) * 100).toFixed(1);
                const playerCount = parseInt(item.player_count) || 0;
                const listedForSale = parseInt(item.listed_for_sale) || 0;
                const iconUrl = item.icon ? `/assets/icons/items/${item.icon}.png` : '/assets/images/item-placeholder.svg';
                
                // Determine status based on scarcity
                let statusClass, statusText;
                if (scarcity >= 80) {
                    statusClass = 'danger';
                    statusText = '🔴 Very Rare';
                } else if (scarcity >= 60) {
                    statusClass = 'warning';
                    statusText = '🟠 Rare';
                } else if (scarcity >= 40) {
                    statusClass = 'info';
                    statusText = '🟡 Uncommon';
                } else if (scarcity >= 20) {
                    statusClass = 'success';
                    statusText = '🟢 Common';
                } else {
                    statusClass = 'muted';
                    statusText = '⚪ Abundant';
                }

                return `
                    <tr>
                        <td>
                            <div style="display: flex; align-items: center; gap: 0.75rem;">
                                <img src="${iconUrl}" alt="${item.name || 'Item'}" class="item-icon" style="width: 32px; height: 32px; object-fit: contain; border-radius: 4px; background: rgba(0,0,0,0.2);" onerror="this.src='/assets/images/item-placeholder.svg'">
                                <span class="font-semibold">${item.name || 'Item #' + item.item_id}</span>
                            </div>
                        </td>
                        <td>${qty.toLocaleString()}</td>
                        <td>${playerCount.toLocaleString()}</td>
                        <td>${listedForSale > 0 ? listedForSale.toLocaleString() : '<span class="text-muted">-</span>'}</td>
                        <td>
                            <div style="display: flex; align-items: center; gap: 0.5rem;">
                                <div style="flex: 1; height: 6px; background: rgba(255,255,255,0.1); border-radius: 3px; overflow: hidden;">
                                    <div style="width: ${scarcity}%; height: 100%; background: ${scarcity >= 60 ? '#ef4444' : scarcity >= 30 ? '#d4af37' : '#22c55e'};"></div>
                                </div>
                                <span style="font-size: 0.85rem;">${scarcity}%</span>
                            </div>
                        </td>
                        <td><span class="text-${statusClass}">${statusText}</span></td>
                    </tr>
                `;
            }).join('');
        }

        function categorizeSupplyDemand(items) {
            const maxQty = Math.max(...items.map(i => parseInt(i.total_quantity) || 1), 1);
            
            // Items with scarcity < 30% are oversupplied (abundant)
            const oversupplied = items
                .filter(i => ((1 - (parseInt(i.total_quantity) || 0) / maxQty) * 100) < 30)
                .slice(0, 5);
            
            // Items with scarcity > 70% are undersupplied (rare)
            const undersupplied = items
                .filter(i => ((1 - (parseInt(i.total_quantity) || 0) / maxQty) * 100) > 70)
                .slice(0, 5);

            // Render oversupply list
            const oversupplyDiv = document.getElementById('oversupplyList');
            if (oversupplyDiv) {
                if (oversupplied.length === 0) {
                    oversupplyDiv.innerHTML = '<div class="text-muted text-center">No oversupplied items detected</div>';
                } else {
                    oversupplyDiv.innerHTML = oversupplied.map(item => {
                        const iconUrl = item.icon ? `/assets/icons/items/${item.icon}.png` : '/assets/images/item-placeholder.svg';
                        return `
                        <div class="item-row" style="display: flex; justify-content: space-between; align-items: center; padding: 0.75rem; border-bottom: 1px solid rgba(255,255,255,0.05);">
                            <div style="display: flex; align-items: center; gap: 0.75rem;">
                                <img src="${iconUrl}" alt="${item.name || 'Item'}" style="width: 28px; height: 28px; object-fit: contain; border-radius: 4px; background: rgba(0,0,0,0.2);" onerror="this.src='/assets/images/item-placeholder.svg'">
                                <span class="font-semibold">${item.name || 'Item #' + item.item_id}</span>
                            </div>
                            <span class="text-success">${parseInt(item.total_quantity).toLocaleString()} in circulation</span>
                        </div>
                    `}).join('');
                }
            }

            // Render undersupply list
            const undersupplyDiv = document.getElementById('undersupplyList');
            if (undersupplyDiv) {
                if (undersupplied.length === 0) {
                    undersupplyDiv.innerHTML = '<div class="text-muted text-center">No undersupplied items detected</div>';
                } else {
                    undersupplyDiv.innerHTML = undersupplied.map(item => {
                        const iconUrl = item.icon ? `/assets/icons/items/${item.icon}.png` : '/assets/images/item-placeholder.svg';
                        return `
                        <div class="item-row" style="display: flex; justify-content: space-between; align-items: center; padding: 0.75rem; border-bottom: 1px solid rgba(255,255,255,0.05);">
                            <div style="display: flex; align-items: center; gap: 0.75rem;">
                                <img src="${iconUrl}" alt="${item.name || 'Item'}" style="width: 28px; height: 28px; object-fit: contain; border-radius: 4px; background: rgba(0,0,0,0.2);" onerror="this.src='/assets/images/item-placeholder.svg'">
                                <span class="font-semibold">${item.name || 'Item #' + item.item_id}</span>
                            </div>
                            <span class="text-danger">Only ${parseInt(item.total_quantity).toLocaleString()} left!</span>
                        </div>
                    `}).join('');                }
            }
        }
    </script>
</body>
</html>