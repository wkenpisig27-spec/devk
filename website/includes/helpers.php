<?php
/**
 * PKO Website Helper Functions
 * 
 * Utility functions for views, formatting, and common operations
 */

declare(strict_types=1);

/**
 * JSON response helper for API endpoints
 */
function jsonResponse(array $data, int $statusCode = 200): void
{
    http_response_code($statusCode);
    header('Content-Type: application/json; charset=utf-8');
    echo json_encode($data, JSON_UNESCAPED_UNICODE);
    exit;
}

/**
 * Error JSON response
 */
function jsonError(string $message, int $statusCode = 400): void
{
    jsonResponse(['success' => false, 'error' => $message], $statusCode);
}

/**
 * Success JSON response
 */
function jsonSuccess(array $data = [], string $message = 'Success'): void
{
    jsonResponse(array_merge(['success' => true, 'message' => $message], $data));
}

/**
 * Redirect helper
 */
function redirect(string $url, int $statusCode = 302): void
{
    header("Location: $url", true, $statusCode);
    exit;
}

/**
 * Flash message helper
 */
function flash(string $type, string $message): void
{
    if (session_status() !== PHP_SESSION_ACTIVE) {
        session_start();
    }
    $_SESSION['flash'][$type][] = $message;
}

/**
 * Get and clear flash messages
 */
function getFlash(): array
{
    if (session_status() !== PHP_SESSION_ACTIVE) {
        session_start();
    }
    $flash = $_SESSION['flash'] ?? [];
    unset($_SESSION['flash']);
    return $flash;
}

/**
 * Check for flash messages
 */
function hasFlash(): bool
{
    if (session_status() !== PHP_SESSION_ACTIVE) {
        session_start();
    }
    return !empty($_SESSION['flash']);
}

/**
 * Format number with abbreviation (K, M, B)
 */
function formatNumber(int $number): string
{
    if ($number >= 1000000000) {
        return round($number / 1000000000, 1) . 'B';
    } elseif ($number >= 1000000) {
        return round($number / 1000000, 1) . 'M';
    } elseif ($number >= 1000) {
        return round($number / 1000, 1) . 'K';
    }
    return (string)$number;
}

/**
 * Format experience to percentage for next level
 */
function formatExpProgress(int $exp, int $level): array
{
    // PKO level exp formula (simplified)
    $expTable = getExpTable();
    $currentLevelExp = $expTable[$level] ?? 0;
    $nextLevelExp = $expTable[$level + 1] ?? $expTable[100] ?? PHP_INT_MAX;
    
    $progress = $nextLevelExp > $currentLevelExp
        ? (($exp - $currentLevelExp) / ($nextLevelExp - $currentLevelExp)) * 100
        : 100;
    
    return [
        'current' => $exp,
        'required' => $nextLevelExp,
        'percentage' => min(100, max(0, $progress))
    ];
}

/**
 * Get experience table (simplified PKO exp curve)
 */
function getExpTable(): array
{
    static $table = null;
    if ($table === null) {
        $table = [0 => 0];
        for ($i = 1; $i <= 100; $i++) {
            // Exponential growth formula
            $table[$i] = (int)($table[$i - 1] + pow($i, 2.5) * 50);
        }
    }
    return $table;
}

/**
 * Format time ago
 */
function timeAgo(string $datetime): string
{
    $timestamp = strtotime($datetime);
    $diff = time() - $timestamp;
    
    if ($diff < 60) {
        return 'Just now';
    } elseif ($diff < 3600) {
        $mins = floor($diff / 60);
        return "$mins minute" . ($mins > 1 ? 's' : '') . ' ago';
    } elseif ($diff < 86400) {
        $hours = floor($diff / 3600);
        return "$hours hour" . ($hours > 1 ? 's' : '') . ' ago';
    } elseif ($diff < 2592000) {
        $days = floor($diff / 86400);
        return "$days day" . ($days > 1 ? 's' : '') . ' ago';
    } else {
        return date('M j, Y', $timestamp);
    }
}

/**
 * Format duration (seconds to human readable)
 */
function formatDuration(int $seconds): string
{
    if ($seconds < 60) {
        return "$seconds second" . ($seconds !== 1 ? 's' : '');
    }
    
    $parts = [];
    
    $days = floor($seconds / 86400);
    if ($days > 0) {
        $parts[] = "$days day" . ($days > 1 ? 's' : '');
        $seconds %= 86400;
    }
    
    $hours = floor($seconds / 3600);
    if ($hours > 0) {
        $parts[] = "$hours hour" . ($hours > 1 ? 's' : '');
        $seconds %= 3600;
    }
    
    $minutes = floor($seconds / 60);
    if ($minutes > 0) {
        $parts[] = "$minutes minute" . ($minutes > 1 ? 's' : '');
    }
    
    return implode(', ', $parts);
}

/**
 * Get character class name from job string
 */
function getClassName(string $job): string
{
    $classes = [
        'newbie' => 'Newbie',
        'swordsman' => 'Swordsman',
        'hunter' => 'Hunter',
        'sailor' => 'Sailor',
        'explorer' => 'Explorer',
        'herbalist' => 'Herbalist',
        'champion' => 'Champion',
        'crusader' => 'Crusader',
        'sharpshooter' => 'Sharpshooter',
        'cleric' => 'Cleric',
        'seal_master' => 'Seal Master',
        'voyager' => 'Voyager',
    ];
    
    $jobLower = strtolower(trim($job));
    return $classes[$jobLower] ?? ucfirst($job);
}

/**
 * Get class icon path - returns SVG for styling flexibility
 */
function getClassIcon(string $job): string
{
    $job = strtolower(trim(str_replace(' ', '_', $job)));
    
    // Valid class names
    $validClasses = [
        'newbie', 'swordsman', 'hunter', 'sailor', 'explorer', 
        'herbalist', 'champion', 'crusader', 'sharpshooter', 
        'cleric', 'seal_master', 'voyager'
    ];
    
    // Fallback to newbie if unknown class
    if (!in_array($job, $validClasses)) {
        $job = 'newbie';
    }
    
    return "/assets/images/classes/{$job}.svg";
}

/**
 * Check if game server is online
 */
function isServerOnline(): bool
{
    static $status = null;
    
    if ($status !== null) {
        return $status;
    }
    
    $socket = @fsockopen(GATE_SERVER_IP, GATE_SERVER_PORT, $errno, $errstr, 2);
    
    if ($socket) {
        fclose($socket);
        $status = true;
    } else {
        $status = false;
    }
    
    return $status;
}

/**
 * Get server statistics
 */
function getServerStats(): array
{
    static $stats = null;
    
    if ($stats !== null) {
        return $stats;
    }
    
    try {
        $stats = [
            'online' => isServerOnline(),
            'players_online' => AccountModel::getOnlineCount(),
            'total_accounts' => AccountModel::getTotalAccounts(),
            'total_characters' => CharacterModel::getTotalCharacters(),
            'total_guilds' => GuildModel::getTotalGuilds(),
        ];
    } catch (Exception $e) {
        error_log("Failed to get server stats: " . $e->getMessage());
        $stats = [
            'online' => false,
            'players_online' => 0,
            'total_accounts' => 0,
            'total_characters' => 0,
            'total_guilds' => 0,
        ];
    }
    
    return $stats;
}

/**
 * Asset URL helper with cache busting
 */
function asset(string $path): string
{
    $fullPath = ROOT_PATH . '/assets/' . ltrim($path, '/');
    $version = file_exists($fullPath) ? filemtime($fullPath) : time();
    return '/assets/' . ltrim($path, '/') . '?v=' . $version;
}

/**
 * Include a template partial
 */
function partial(string $name, array $data = []): void
{
    extract($data);
    include PAGES_PATH . "/partials/{$name}.php";
}

/**
 * Safe output (escaped HTML)
 */
function e(string $value): string
{
    return htmlspecialchars($value, ENT_QUOTES | ENT_HTML5, 'UTF-8');
}

/**
 * Get current page name
 */
function getCurrentPage(): string
{
    $uri = $_SERVER['REQUEST_URI'] ?? '/';
    $path = parse_url($uri, PHP_URL_PATH);
    $page = basename($path, '.php');
    return $page === '' || $page === 'index' ? 'home' : $page;
}

/**
 * Check if current page matches
 */
function isPage(string $page): bool
{
    return getCurrentPage() === $page;
}

/**
 * Generate pagination HTML
 */
function pagination(int $current, int $total, int $perPage, string $baseUrl): string
{
    $totalPages = ceil($total / $perPage);
    if ($totalPages <= 1) return '';
    
    $html = '<nav class="pagination"><ul>';
    
    // Previous
    if ($current > 1) {
        $html .= '<li><a href="' . e($baseUrl . '?page=' . ($current - 1)) . '">&laquo;</a></li>';
    }
    
    // Page numbers
    $start = max(1, $current - 2);
    $end = min($totalPages, $current + 2);
    
    if ($start > 1) {
        $html .= '<li><a href="' . e($baseUrl . '?page=1') . '">1</a></li>';
        if ($start > 2) $html .= '<li class="ellipsis">...</li>';
    }
    
    for ($i = $start; $i <= $end; $i++) {
        $active = $i === $current ? ' class="active"' : '';
        $html .= '<li' . $active . '><a href="' . e($baseUrl . '?page=' . $i) . '">' . $i . '</a></li>';
    }
    
    if ($end < $totalPages) {
        if ($end < $totalPages - 1) $html .= '<li class="ellipsis">...</li>';
        $html .= '<li><a href="' . e($baseUrl . '?page=' . $totalPages) . '">' . $totalPages . '</a></li>';
    }
    
    // Next
    if ($current < $totalPages) {
        $html .= '<li><a href="' . e($baseUrl . '?page=' . ($current + 1)) . '">&raquo;</a></li>';
    }
    
    $html .= '</ul></nav>';
    return $html;
}

/**
 * Format gold/money display
 */
function formatGold(int $amount): string
{
    return number_format($amount) . ' <span class="gold-icon">G</span>';
}

/**
 * Format credits display
 */
function formatCredits(int $amount): string
{
    return number_format($amount) . ' <span class="credit-icon">CP</span>';
}

/**
 * Get item icon URL
 * 
 * @param string $iconName The icon name from ItemInfo (e.g., 'w0001')
 * @param int|null $itemId Optional item ID for fallback lookup
 * @return string URL to the icon image, or placeholder if not found
 */
function getItemIconUrl(string $iconName, ?int $itemId = null): string
{
    $basePath = '/assets/icons/items/';
    $placeholder = '/assets/images/item-placeholder.svg';
    
    // Try direct icon name
    if (!empty($iconName)) {
        $iconFile = $iconName . '.png';
        $fullPath = __DIR__ . '/../assets/icons/items/' . $iconFile;
        if (file_exists($fullPath)) {
            return $basePath . $iconFile;
        }
    }
    
    // Return placeholder
    return $placeholder;
}

/**
 * Render item icon HTML with fallback
 * 
 * @param string $iconName The icon name from ItemInfo
 * @param string $itemName Item name for alt text
 * @param string $size CSS size (default 32px)
 * @return string HTML img tag
 */
function renderItemIcon(string $iconName, string $itemName = 'Item', string $size = '32px'): string
{
    $url = getItemIconUrl($iconName);
    $alt = e($itemName);
    return sprintf(
        '<img src="%s" alt="%s" class="item-icon" style="width:%s;height:%s;object-fit:contain;" onerror="this.src=\'/assets/images/item-placeholder.png\'">',
        $url, $alt, $size, $size
    );
}

/**
 * Truncate text with ellipsis
 */
function truncate(string $text, int $length, string $suffix = '...'): string
{
    if (strlen($text) <= $length) {
        return $text;
    }
    return substr($text, 0, $length - strlen($suffix)) . $suffix;
}

/**
 * Generate meta tags
 */
function metaTags(string $title, string $description = '', string $image = ''): string
{
    $siteName = SERVER_NAME;
    $fullTitle = $title ? "{$title} | {$siteName}" : $siteName;
    $description = $description ?: "Join {$siteName} - An epic pirate MMORPG adventure awaits!";
    $image = $image ?: SITE_URL . '/assets/images/og-image.jpg';
    $url = SITE_URL . ($_SERVER['REQUEST_URI'] ?? '/');
    
    return <<<HTML
    <title>{$fullTitle}</title>
    <meta name="description" content="{$description}">
    <meta property="og:type" content="website">
    <meta property="og:title" content="{$fullTitle}">
    <meta property="og:description" content="{$description}">
    <meta property="og:image" content="{$image}">
    <meta property="og:url" content="{$url}">
    <meta property="og:site_name" content="{$siteName}">
    <meta name="twitter:card" content="summary_large_image">
    <meta name="twitter:title" content="{$fullTitle}">
    <meta name="twitter:description" content="{$description}">
    <meta name="twitter:image" content="{$image}">
HTML;
}

/**
 * ItemInfo Parser - Parses ItemInfo.txt from server resources
 * Returns a cached array of all items indexed by ID
 */
function getItemDatabase(): array
{
    static $items = null;
    
    if ($items !== null) {
        return $items;
    }
    
    $items = [];
    $cacheFile = __DIR__ . '/../cache/iteminfo.cache.php';
    $itemInfoPath = defined('ITEM_INFO_PATH') ? ITEM_INFO_PATH : __DIR__ . '/../../server/resource/ItemInfo.txt';
    
    // Check cache
    if (file_exists($cacheFile)) {
        $cacheTime = filemtime($cacheFile);
        $sourceTime = file_exists($itemInfoPath) ? filemtime($itemInfoPath) : 0;
        
        if ($cacheTime > $sourceTime) {
            $items = include $cacheFile;
            return $items;
        }
    }
    
    // Parse ItemInfo.txt
    if (!file_exists($itemInfoPath)) {
        return [];
    }
    
    $lines = file($itemInfoPath, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
    $headers = [];
    
    foreach ($lines as $lineNum => $line) {
        // Skip comment lines
        if (strpos(trim($line), '//') === 0) {
            if ($lineNum === 0) {
                // First comment line contains headers
                $headers = array_map('trim', explode("\t", substr(trim($line), 2)));
            }
            continue;
        }
        
        $fields = explode("\t", $line);
        if (count($fields) < 5) continue;
        
        $id = (int)$fields[0];
        if ($id <= 0) continue;
        
        $items[$id] = [
            'id' => $id,
            'name' => $fields[1] ?? '',
            'icon' => $fields[2] ?? '',
            'type' => (int)($fields[10] ?? 0),
            'level' => (int)($fields[24] ?? 0),
            'price' => (int)($fields[22] ?? 0),
            'tradeable' => ($fields[16] ?? '1') === '1',
            'stackable' => (int)($fields[21] ?? 1),
            'description' => $fields[count($fields) - 2] ?? '',
        ];
    }
    
    // Save cache
    $cacheDir = dirname($cacheFile);
    if (!is_dir($cacheDir)) {
        mkdir($cacheDir, 0755, true);
    }
    file_put_contents($cacheFile, '<?php return ' . var_export($items, true) . ';');
    
    return $items;
}

/**
 * Get item by ID
 */
function getItemById(int $id): ?array
{
    $items = getItemDatabase();
    return $items[$id] ?? null;
}

/**
 * Search items by name
 */
function searchItems(string $query, int $limit = 50): array
{
    $items = getItemDatabase();
    $results = [];
    $query = strtolower($query);
    
    foreach ($items as $item) {
        if (stripos($item['name'], $query) !== false) {
            $results[] = $item;
            if (count($results) >= $limit) break;
        }
    }
    
    return $results;
}

/**
 * Get items by type
 */
function getItemsByType(int $type, int $limit = 100): array
{
    $items = getItemDatabase();
    $results = [];
    
    foreach ($items as $item) {
        if ($item['type'] === $type) {
            $results[] = $item;
            if (count($results) >= $limit) break;
        }
    }
    
    return $results;
}

/**
 * PKO Kitbag Decryption Key (from Kitbag.cpp)
 */
define('PKO_KITBAG_KEY', '19800216');

/**
 * Decrypt PKO kitbag data
 * 
 * The kitbag data is encrypted using a simple XOR-like cipher:
 * Encrypt: buf[i] = data[i] + key[i % 8]
 * Decrypt: buf[i] = data[i] - key[i % 8]
 * 
 * @param string $encrypted The encrypted data portion (after the # separator)
 * @return string Decrypted data
 */
function decryptKitbagData(string $encrypted): string
{
    $key = PKO_KITBAG_KEY;
    $keyLen = strlen($key);
    $decrypted = '';
    
    for ($i = 0; $i < strlen($encrypted); $i++) {
        $decrypted .= chr(ord($encrypted[$i]) - ord($key[$i % $keyLen]));
    }
    
    return $decrypted;
}

/**
 * Parse PKO kitbag/resource content string
 * 
 * Format: [capacity]@[version]#[encrypted_data]
 * 
 * Encrypted data format (semicolon separated):
 * - First segment: number of item types (pages)
 * - For each page: useGridNum, then items...
 * - Each item: gridIndex,itemID,quantity,endure1,endure2,energy1,energy2,forgeLv,dbID,needLv,isLock,tradable,expiration,dbParam0,dbParam1,hasInstAttr,[instAttrs...]
 * - Last segment: checksum
 * 
 * @param string $content Raw content from Resource table
 * @return array Parsed items with item IDs and quantities
 */
function parseKitbagContent(string $content): array
{
    if (empty($content)) {
        return [];
    }
    
    $items = [];
    
    // Parse header: [capacity]@[version]#[encrypted_data]
    // Or old format: [version]#[encrypted_data]
    $capacity = 24; // Default capacity
    
    if (strpos($content, '@') !== false) {
        // New format with capacity
        list($capacityPart, $rest) = explode('@', $content, 2);
        $capacity = (int)$capacityPart;
    } else {
        $rest = $content;
    }
    
    // Split version and encrypted data
    if (strpos($rest, '#') === false) {
        // Old format without encryption
        $version = 0;
        $data = $rest;
    } else {
        list($versionStr, $encryptedData) = explode('#', $rest, 2);
        $version = (int)$versionStr;
        
        // Version 113 and 114 use encryption
        if ($version == 113 || $version == 114) {
            $data = decryptKitbagData($encryptedData);
        } else {
            $data = $versionStr; // Old format, version string IS the data
        }
    }
    
    if (empty($data)) {
        return [];
    }
    
    // Parse the semicolon-separated segments
    $segments = explode(';', $data);
    if (count($segments) < 2) {
        return [];
    }
    
    $segIndex = 0;
    $pageCount = (int)($segments[$segIndex++] ?? 0);
    
    if ($pageCount <= 0 || $pageCount > 10) {
        return []; // Invalid page count
    }
    
    // Calculate items per page
    $totalSegments = count($segments);
    // Last segment is checksum, first is pageCount, then each page has useGridNum + items
    
    for ($page = 0; $page < $pageCount && $segIndex < $totalSegments - 1; $page++) {
        // Skip useGridNum for this page
        $useGridNum = (int)($segments[$segIndex++] ?? 0);
        
        // Parse items for this page
        for ($i = 0; $i < $useGridNum && $segIndex < $totalSegments - 1; $i++) {
            $itemData = $segments[$segIndex++] ?? '';
            if (empty($itemData)) {
                continue;
            }
            
            $fields = explode(',', $itemData);
            if (count($fields) < 8) {
                continue; // Not enough fields
            }
            
            $gridIndex = (int)($fields[0] ?? 0);
            $itemId = (int)($fields[1] ?? 0);
            $quantity = (int)($fields[2] ?? 0);
            $endure1 = (int)($fields[3] ?? 0);
            $endure2 = (int)($fields[4] ?? 0);
            $energy1 = (int)($fields[5] ?? 0);
            $energy2 = (int)($fields[6] ?? 0);
            $forgeLv = (int)($fields[7] ?? 0);
            
            // Version 114 has additional fields
            $dbId = 0;
            $needLv = 0;
            $isLock = 0;
            $tradable = 1;
            $expiration = 0;
            
            if ($version == 114 && count($fields) >= 13) {
                $dbId = (int)($fields[8] ?? 0);
                $needLv = (int)($fields[9] ?? 0);
                $isLock = (int)($fields[10] ?? 0);
                $tradable = (int)($fields[11] ?? 1);
                $expiration = (int)($fields[12] ?? 0);
            }
            
            if ($itemId > 0 && $quantity > 0) {
                $items[] = [
                    'grid_index' => $gridIndex,
                    'item_id' => $itemId,
                    'quantity' => $quantity,
                    'endure' => [$endure1, $endure2],
                    'energy' => [$energy1, $energy2],
                    'forge_level' => $forgeLv,
                    'db_id' => $dbId,
                    'need_level' => $needLv,
                    'is_locked' => (bool)$isLock,
                    'tradable' => (bool)$tradable,
                    'expiration' => $expiration,
                    'page' => $page,
                ];
            }
        }
    }
    
    return $items;
}

/**
 * Get all items from a character's kitbag (inventory)
 * 
 * @param int $chaId Character ID
 * @return array Items with full details from ItemInfo
 */
function getCharacterInventory(int $chaId): array
{
    try {
        $db = Database::getGameDb();
        
        // Get kitbag resource (type_id = 1 for kitbag)
        $stmt = $db->prepare("
            SELECT content 
            FROM Resource 
            WHERE cha_id = ? AND type_id = 1
        ");
        $stmt->execute([$chaId]);
        $result = $stmt->fetch();
        
        if (!$result || empty($result['content'])) {
            return [];
        }
        
        // Parse the kitbag content
        $items = parseKitbagContent($result['content']);
        
        // Enrich with item info from ItemInfo.txt
        $itemDb = getItemDatabase();
        foreach ($items as &$item) {
            $itemInfo = $itemDb[$item['item_id']] ?? null;
            if ($itemInfo) {
                $item['name'] = $itemInfo['name'];
                $item['type'] = $itemInfo['type'];
                $item['type_name'] = getItemTypeName($itemInfo['type']);
                $item['level'] = $itemInfo['level'];
                $item['price'] = $itemInfo['price'];
                $item['icon'] = $itemInfo['icon'];
            } else {
                $item['name'] = 'Unknown Item #' . $item['item_id'];
                $item['type'] = 0;
                $item['type_name'] = 'Unknown';
                $item['level'] = 0;
                $item['price'] = 0;
                $item['icon'] = '';
            }
        }
        
        return $items;
    } catch (Exception $e) {
        error_log("Failed to get character inventory: " . $e->getMessage());
        return [];
    }
}

/**
 * Get item distribution across all players
 * 
 * @param int $limit Max number of items to return
 * @return array Item statistics sorted by total quantity
 */
function getServerItemDistribution(int $limit = 50): array
{
    try {
        $db = Database::getGameDb();
        
        // Get all kitbag resources
        $stmt = $db->query("
            SELECT r.cha_id, r.content, c.cha_name
            FROM Resource r
            INNER JOIN character c ON r.cha_id = c.cha_id
            WHERE r.type_id = 1 AND c.delflag = 0
        ");
        
        $itemCounts = [];
        $itemDb = getItemDatabase();
        
        while ($row = $stmt->fetch()) {
            $items = parseKitbagContent($row['content'] ?? '');
            
            foreach ($items as $item) {
                $itemId = $item['item_id'];
                if (!isset($itemCounts[$itemId])) {
                    $itemInfo = $itemDb[$itemId] ?? null;
                    $itemCounts[$itemId] = [
                        'item_id' => $itemId,
                        'name' => $itemInfo['name'] ?? 'Unknown #' . $itemId,
                        'icon' => $itemInfo['icon'] ?? '',
                        'type' => $itemInfo['type'] ?? 0,
                        'type_name' => getItemTypeName($itemInfo['type'] ?? 0),
                        'level' => $itemInfo['level'] ?? 0,
                        'price' => $itemInfo['price'] ?? 0,
                        'total_quantity' => 0,
                        'player_count' => 0,
                        'players' => [],
                    ];
                }
                
                $itemCounts[$itemId]['total_quantity'] += $item['quantity'];
                if (!in_array($row['cha_id'], $itemCounts[$itemId]['players'])) {
                    $itemCounts[$itemId]['players'][] = $row['cha_id'];
                    $itemCounts[$itemId]['player_count']++;
                }
            }
        }
        
        // Sort by total quantity descending
        usort($itemCounts, fn($a, $b) => $b['total_quantity'] <=> $a['total_quantity']);
        
        // Remove players array and limit results
        $result = array_slice($itemCounts, 0, $limit);
        foreach ($result as &$item) {
            unset($item['players']);
        }
        
        return $result;
    } catch (Exception $e) {
        error_log("Failed to get item distribution: " . $e->getMessage());
        return [];
    }
}

/**
 * Get rarest items (fewest total quantity across all players)
 * 
 * @param int $limit Max items to return
 * @param int $minPlayers Minimum players who own the item
 * @return array Rare items sorted by scarcity
 */
function getRarestItems(int $limit = 20, int $minPlayers = 1): array
{
    try {
        $distribution = getServerItemDistribution(1000); // Get more items
        
        // Filter by minimum players and sort by quantity ascending
        $rare = array_filter($distribution, fn($item) => $item['player_count'] >= $minPlayers);
        usort($rare, fn($a, $b) => $a['total_quantity'] <=> $b['total_quantity']);
        
        return array_slice($rare, 0, $limit);
    } catch (Exception $e) {
        error_log("Failed to get rarest items: " . $e->getMessage());
        return [];
    }
}

/**
 * Get item type name
 */
function getItemTypeName(int $type): string
{
    $types = [
        0 => 'None',
        1 => 'Sword (1H)',
        2 => 'Sword (2H)',
        3 => 'Bow',
        4 => 'Gun',
        5 => 'Claw',
        6 => 'Staff',
        7 => 'Shield',
        8 => 'Armor',
        9 => 'Helm',
        10 => 'Gloves',
        11 => 'Boots',
        12 => 'Ring',
        13 => 'Necklace',
        14 => 'Fairy',
        15 => 'Gem',
        16 => 'Material',
        17 => 'Quest Item',
        18 => 'Medicine',
        19 => 'Food',
        20 => 'Scroll',
        21 => 'Ammo',
        22 => 'Ship Part',
        23 => 'Ship Equip',
        24 => 'Blueprint',
        25 => 'Costume',
        26 => 'Voucher',
        27 => 'Mount',
        28 => 'Pet',
        29 => 'Event Item',
        30 => 'Gift Box',
        31 => 'Special',
    ];
    
    return $types[$type] ?? 'Unknown';
}
