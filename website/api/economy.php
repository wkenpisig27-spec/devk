<?php
/**
 * PKO Website - Economy API Endpoint
 * 
 * Provides AJAX endpoints for economy/market data
 */

require_once __DIR__ . '/../includes/config.php';

// Set JSON response headers
header('Content-Type: application/json; charset=utf-8');
header('Cache-Control: public, max-age=60'); // Cache for 1 minute

$action = $_GET['action'] ?? '';

try {
    switch ($action) {
        case 'search':
            // Search items by name
            $query = trim($_GET['q'] ?? '');
            $limit = min(100, max(1, (int)($_GET['limit'] ?? 50)));
            
            if (strlen($query) < 2) {
                jsonError('Search query must be at least 2 characters', 400);
            }
            
            $results = searchItems($query, $limit);
            jsonSuccess(['items' => $results, 'count' => count($results)]);
            break;
            
        case 'item':
            // Get single item by ID
            $id = (int)($_GET['id'] ?? 0);
            
            if ($id <= 0) {
                jsonError('Invalid item ID', 400);
            }
            
            $item = getItemById($id);
            if (!$item) {
                jsonError('Item not found', 404);
            }
            
            jsonSuccess(['item' => $item]);
            break;
            
        case 'items-by-type':
            // Get items by type
            $type = (int)($_GET['type'] ?? 0);
            $limit = min(200, max(1, (int)($_GET['limit'] ?? 100)));
            
            $items = getItemsByType($type, $limit);
            jsonSuccess([
                'items' => $items, 
                'count' => count($items),
                'type_name' => getItemTypeName($type)
            ]);
            break;
            
        case 'stats':
            // Get economy statistics
            $stats = EconomyModel::getServerStats();
            jsonSuccess(['stats' => $stats]);
            break;
            
        case 'stalls':
            // Get active market stalls
            $limit = min(100, max(1, (int)($_GET['limit'] ?? 50)));
            $stalls = EconomyModel::getActiveStalls($limit);
            $stallsByMap = EconomyModel::getStallsByMap();
            
            jsonSuccess([
                'stalls' => $stalls,
                'by_map' => $stallsByMap,
                'total' => count($stalls)
            ]);
            break;
            
        case 'richest':
            // Get top richest players
            $limit = min(100, max(1, (int)($_GET['limit'] ?? 20)));
            $players = EconomyModel::getTopRichest($limit);
            jsonSuccess(['players' => $players]);
            break;
            
        case 'classes':
            // Get class distribution
            $distribution = EconomyModel::getClassDistribution();
            jsonSuccess(['distribution' => $distribution]);
            break;
            
        case 'levels':
            // Get level distribution
            $distribution = EconomyModel::getLevelDistribution();
            jsonSuccess(['distribution' => $distribution]);
            break;
            
        case 'activity':
            // Get recent activity
            $days = min(30, max(1, (int)($_GET['days'] ?? 7)));
            $activity = EconomyModel::getRecentActivity($days);
            jsonSuccess(['activity' => $activity]);
            break;
            
        case 'shop':
            // Get shop statistics
            $stats = EconomyModel::getShopStats();
            jsonSuccess(['shop' => $stats]);
            break;
            
        case 'types':
            // Get all item type names
            $types = [];
            for ($i = 0; $i <= 31; $i++) {
                $name = getItemTypeName($i);
                if ($name !== 'Unknown') {
                    $types[$i] = $name;
                }
            }
            jsonSuccess(['types' => $types]);
            break;
            
        case 'inventory':
            // Get a character's inventory
            $chaId = (int)($_GET['cha_id'] ?? 0);
            
            if ($chaId <= 0) {
                jsonError('Invalid character ID', 400);
            }
            
            $items = getCharacterInventory($chaId);
            jsonSuccess([
                'cha_id' => $chaId,
                'items' => $items,
                'count' => count($items),
                'total_items' => array_sum(array_column($items, 'quantity'))
            ]);
            break;
            
        case 'item-distribution':
            // Get most common items across all players
            $limit = min(200, max(1, (int)($_GET['limit'] ?? 50)));
            $items = getServerItemDistribution($limit);
            jsonSuccess([
                'items' => $items,
                'count' => count($items)
            ]);
            break;
            
        case 'rare-items':
            // Get rarest items
            $limit = min(100, max(1, (int)($_GET['limit'] ?? 20)));
            $items = getRarestItems($limit, 1);
            jsonSuccess([
                'items' => $items,
                'count' => count($items)
            ]);
            break;
            
        default:
            jsonError('Invalid action. Available actions: search, item, items-by-type, stats, stalls, richest, classes, levels, activity, shop, types, inventory, item-distribution, rare-items', 400);
    }
} catch (Exception $e) {
    error_log("Economy API error: " . $e->getMessage());
    jsonError('An error occurred processing your request', 500);
}
