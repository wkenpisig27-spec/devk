<?php
/**
 * PKO Website API - Leaderboard Endpoint
 */

require_once __DIR__ . '/../includes/config.php';

header('Content-Type: application/json');
header('Cache-Control: public, max-age=60'); // Cache for 1 minute
Security::setHeaders();

$type = $_GET['type'] ?? 'level';
$limit = min(50, max(1, (int)($_GET['limit'] ?? 10)));

try {
    switch ($type) {
        case 'pvp':
        case 'battle_power':
            $data = CharacterModel::getTopByBattlePower($limit);
            break;
        case 'guilds':
            $data = GuildModel::getTopByLevel($limit);
            break;
        case 'level':
        default:
            $data = CharacterModel::getTopByLevel($limit);
            break;
    }
    
    jsonSuccess([
        'type' => $type,
        'data' => $data
    ]);
} catch (Exception $e) {
    error_log("Leaderboard API error: " . $e->getMessage());
    jsonError('Failed to load leaderboard data', 500);
}
