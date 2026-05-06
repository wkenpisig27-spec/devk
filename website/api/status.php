<?php
/**
 * PKO Website API - Server Status Endpoint
 */

require_once __DIR__ . '/../includes/config.php';

header('Content-Type: application/json');
header('Cache-Control: public, max-age=30'); // Cache for 30 seconds
Security::setHeaders();

try {
    $stats = getServerStats();
    
    jsonSuccess([
        'online' => $stats['online'],
        'players_online' => $stats['players_online'],
        'total_accounts' => $stats['total_accounts'],
        'total_characters' => $stats['total_characters'],
        'total_guilds' => $stats['total_guilds'],
        'timestamp' => time()
    ]);
} catch (Exception $e) {
    error_log("Status API error: " . $e->getMessage());
    jsonSuccess([
        'online' => false,
        'players_online' => 0,
        'total_accounts' => 0,
        'total_characters' => 0,
        'total_guilds' => 0,
        'timestamp' => time()
    ]);
}
