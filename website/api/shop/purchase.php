<?php
/**
 * PKO Website API - Shop Purchase Endpoint
 */

require_once __DIR__ . '/../../includes/config.php';

header('Content-Type: application/json');
Security::setHeaders();

// Require authentication
if (!Auth::check()) {
    jsonError('Authentication required', 401);
}

// Only accept POST
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    jsonError('Method not allowed', 405);
}

// Get JSON input
$input = json_decode(file_get_contents('php://input'), true);
if (!$input) {
    jsonError('Invalid request body');
}

// Validate input
if (!isset($input['item_id']) || !isset($input['character_id'])) {
    jsonError('Item ID and Character ID are required');
}

$itemId = (int)$input['item_id'];
$characterId = (int)$input['character_id'];
$userId = Auth::id();

// Verify character belongs to user
$character = CharacterModel::findById($characterId);
if (!$character || $character['act_id'] != $userId) {
    jsonError('Invalid character');
}

// Process purchase
$result = ShopModel::purchase($userId, $itemId, $characterId);

if ($result['success']) {
    // Get new balance
    try {
        $gameDb = Database::getGameDb();
        $stmt = $gameDb->prepare("SELECT credit FROM account WHERE act_id = ?");
        $stmt->execute([$userId]);
        $account = $stmt->fetch();
        $newBalance = (int)($account['credit'] ?? 0);
    } catch (Exception $e) {
        $newBalance = null;
    }
    
    jsonSuccess([
        'new_balance' => $newBalance
    ], $result['message']);
} else {
    jsonError($result['error']);
}
