<?php
/**
 * PKO Economy Snapshot Collector
 * 
 * This script collects economy data and saves snapshots for trend analysis.
 * Should be run periodically via cron/task scheduler (hourly recommended).
 * 
 * Usage: php collect_economy_snapshot.php
 * 
 * Windows Task Scheduler:
 *   Action: C:\php\php.exe
 *   Arguments: C:\path\to\website\cron\collect_economy_snapshot.php
 *   Schedule: Every hour
 */

declare(strict_types=1);

// Ensure running from CLI
if (php_sapi_name() !== 'cli') {
    die("This script must be run from command line.\n");
}

require_once __DIR__ . '/../includes/config.php';
require_once __DIR__ . '/../includes/database.php';
require_once __DIR__ . '/../includes/helpers.php';

echo "=== PKO Economy Snapshot Collector ===\n";
echo "Time: " . date('Y-m-d H:i:s') . "\n\n";

try {
    $db = Database::getGameDb();
    
    // ========================================
    // Step 1: Calculate overall economy metrics
    // ========================================
    echo "Collecting economy metrics...\n";
    
    // Gold statistics
    $goldStmt = $db->query("
        SELECT 
            SUM(CAST(gd AS BIGINT)) as total_gold,
            AVG(CAST(gd AS BIGINT)) as avg_gold,
            COUNT(*) as total_characters
        FROM character 
        WHERE delflag = 0
    ");
    $goldStats = $goldStmt->fetch();
    
    // Guild gold
    $guildStmt = $db->query("
        SELECT SUM(CAST(gold AS BIGINT)) as total_guild_gold
        FROM guild 
        WHERE guild_name NOT LIKE 'Pirate Guild %'
    ");
    $guildStats = $guildStmt->fetch();
    
    // Active stalls (if table exists)
    $activeStalls = 0;
    $totalStallItems = 0;
    try {
        $stallStmt = $db->query("
            SELECT COUNT(*) as stall_count, ISNULL(SUM(item_count), 0) as item_count
            FROM offline_stalls 
            WHERE is_active = 1 AND expire_time > GETDATE()
        ");
        $stallStats = $stallStmt->fetch();
        $activeStalls = (int)$stallStats['stall_count'];
        $totalStallItems = (int)$stallStats['item_count'];
    } catch (Exception $e) {
        echo "  Note: offline_stalls table not found\n";
    }
    
    // Calculate Gini coefficient (wealth inequality)
    $giniStmt = $db->query("
        SELECT gd FROM character WHERE delflag = 0 ORDER BY gd ASC
    ");
    $goldValues = $giniStmt->fetchAll(PDO::FETCH_COLUMN);
    $gini = calculateGiniCoefficient($goldValues);
    
    echo "  Total Gold: " . number_format($goldStats['total_gold'] ?? 0) . "\n";
    echo "  Guild Gold: " . number_format($guildStats['total_guild_gold'] ?? 0) . "\n";
    echo "  Characters: " . number_format($goldStats['total_characters'] ?? 0) . "\n";
    echo "  Active Stalls: $activeStalls\n";
    echo "  Gini Coefficient: " . round($gini, 4) . "\n\n";
    
    // ========================================
    // Step 2: Insert economy snapshot
    // ========================================
    echo "Creating economy snapshot...\n";
    
    // Check if table exists, create if not
    try {
        $db->query("SELECT TOP 1 snapshot_id FROM economy_snapshots");
    } catch (Exception $e) {
        echo "  Creating economy_snapshots table...\n";
        createEconomyTables($db);
    }
    
    $insertStmt = $db->prepare("
        INSERT INTO economy_snapshots (
            snapshot_time, total_gold, total_guild_gold, avg_player_gold,
            total_characters, active_stalls, total_stall_items, gini_coefficient
        ) VALUES (
            GETDATE(), ?, ?, ?, ?, ?, ?, ?
        )
    ");
    $insertStmt->execute([
        $goldStats['total_gold'] ?? 0,
        $guildStats['total_guild_gold'] ?? 0,
        $goldStats['avg_gold'] ?? 0,
        $goldStats['total_characters'] ?? 0,
        $activeStalls,
        $totalStallItems,
        $gini
    ]);
    
    // Get the snapshot ID
    $snapshotId = (int)$db->lastInsertId();
    echo "  Snapshot ID: $snapshotId\n\n";
    
    // ========================================
    // Step 3: Collect per-item statistics
    // ========================================
    echo "Collecting item statistics...\n";
    
    $itemDistribution = getServerItemDistribution(500); // Get top 500 items
    $itemCount = count($itemDistribution);
    echo "  Found $itemCount unique items in player inventories\n";
    
    // Get stall listings for demand data (if available)
    $stallListings = [];
    // TODO: Parse stall item_data to get listed items and prices
    
    // Insert item snapshots
    $itemStmt = $db->prepare("
        INSERT INTO economy_item_snapshots (
            snapshot_id, item_id, total_supply, player_count, avg_per_player,
            listed_for_sale, stall_count, scarcity_index, supply_demand_ratio
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    ");
    
    $totalPlayers = max(1, (int)($goldStats['total_characters'] ?? 1));
    $inserted = 0;
    
    foreach ($itemDistribution as $item) {
        $avgPerPlayer = $item['player_count'] > 0 
            ? $item['total_quantity'] / $item['player_count'] 
            : 0;
        
        // Scarcity index: lower player_count and total_quantity = higher scarcity
        // Normalized to 0-100 scale
        $scarcity = calculateScarcityIndex(
            $item['total_quantity'], 
            $item['player_count'], 
            $totalPlayers
        );
        
        // Supply/demand ratio (placeholder - needs stall data)
        $listedForSale = $stallListings[$item['item_id']]['quantity'] ?? 0;
        $supplyDemandRatio = $listedForSale > 0 
            ? $item['total_quantity'] / $listedForSale 
            : null;
        
        $itemStmt->execute([
            $snapshotId,
            $item['item_id'],
            $item['total_quantity'],
            $item['player_count'],
            round($avgPerPlayer, 2),
            $listedForSale,
            $stallListings[$item['item_id']]['stall_count'] ?? 0,
            round($scarcity, 4),
            $supplyDemandRatio ? round($supplyDemandRatio, 4) : null
        ]);
        $inserted++;
    }
    
    echo "  Inserted $inserted item snapshots\n\n";
    
    // ========================================
    // Step 4: Cleanup old data (keep 90 days)
    // ========================================
    echo "Cleaning up old snapshots...\n";
    $cleanupStmt = $db->query("
        DELETE FROM economy_snapshots 
        WHERE snapshot_time < DATEADD(DAY, -90, GETDATE())
    ");
    $deleted = $cleanupStmt->rowCount();
    echo "  Deleted $deleted old snapshots\n\n";
    
    echo "=== Snapshot Complete ===\n";
    
} catch (Exception $e) {
    echo "ERROR: " . $e->getMessage() . "\n";
    exit(1);
}

// ========================================
// Helper Functions
// ========================================

/**
 * Calculate Gini coefficient for wealth distribution
 * 0 = perfect equality, 1 = perfect inequality
 */
function calculateGiniCoefficient(array $values): float
{
    $n = count($values);
    if ($n < 2) return 0;
    
    sort($values);
    $sum = array_sum($values);
    if ($sum == 0) return 0;
    
    $giniSum = 0;
    foreach ($values as $i => $value) {
        $giniSum += ($i + 1) * $value;
    }
    
    $gini = (2 * $giniSum) / ($n * $sum) - ($n + 1) / $n;
    return max(0, min(1, $gini));
}

/**
 * Calculate scarcity index for an item
 * Higher value = rarer item
 */
function calculateScarcityIndex(int $totalQuantity, int $playerCount, int $totalPlayers): float
{
    if ($totalQuantity <= 0) return 100;
    if ($totalPlayers <= 0) return 0;
    
    // Ownership ratio (what % of players have this item)
    $ownershipRatio = $playerCount / $totalPlayers;
    
    // Quantity factor (log scale to handle large numbers)
    $quantityFactor = 1 / (1 + log10(1 + $totalQuantity));
    
    // Combined scarcity (0-100 scale)
    $scarcity = (1 - $ownershipRatio) * 50 + $quantityFactor * 50;
    
    return min(100, max(0, $scarcity));
}

/**
 * Create economy tables if they don't exist
 */
function createEconomyTables(PDO $db): void
{
    $db->exec("
        IF OBJECT_ID('dbo.economy_snapshots', 'U') IS NULL
        CREATE TABLE [dbo].[economy_snapshots] (
            [snapshot_id]       INT IDENTITY(1,1) NOT NULL PRIMARY KEY,
            [snapshot_time]     DATETIME NOT NULL DEFAULT GETDATE(),
            [total_gold]        BIGINT NOT NULL DEFAULT 0,
            [total_guild_gold]  BIGINT NOT NULL DEFAULT 0,
            [avg_player_gold]   BIGINT NOT NULL DEFAULT 0,
            [total_characters]  INT NOT NULL DEFAULT 0,
            [active_stalls]     INT NOT NULL DEFAULT 0,
            [total_stall_items] INT NOT NULL DEFAULT 0,
            [gini_coefficient]  DECIMAL(5,4) NULL
        )
    ");
    
    $db->exec("
        IF OBJECT_ID('dbo.economy_item_snapshots', 'U') IS NULL
        CREATE TABLE [dbo].[economy_item_snapshots] (
            [id]                INT IDENTITY(1,1) NOT NULL PRIMARY KEY,
            [snapshot_id]       INT NOT NULL,
            [item_id]           INT NOT NULL,
            [total_supply]      INT NOT NULL DEFAULT 0,
            [player_count]      INT NOT NULL DEFAULT 0,
            [avg_per_player]    DECIMAL(10,2) NULL,
            [listed_for_sale]   INT NOT NULL DEFAULT 0,
            [stall_count]       INT NOT NULL DEFAULT 0,
            [avg_asking_price]  BIGINT NULL,
            [scarcity_index]    DECIMAL(10,4) NULL,
            [supply_demand_ratio] DECIMAL(10,4) NULL
        )
    ");
}
