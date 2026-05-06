<?php
/**
 * PKO Website API - Vote Endpoint
 */

require_once __DIR__ . '/../includes/config.php';

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

if (!VOTING_ENABLED) {
    jsonError('Voting is currently disabled');
}

// Get JSON input
$input = json_decode(file_get_contents('php://input'), true);
if (!$input || !isset($input['site_id'])) {
    jsonError('Site ID is required');
}

$siteId = (int)$input['site_id'];
$userId = Auth::id();
$user = Auth::user();
$ip = Security::getClientIp();

// Check if site exists
$site = VoteModel::getSiteById($siteId);
if (!$site) {
    jsonError('Vote site not found');
}

// Check if can vote
if (!VoteModel::canVote($userId, $siteId, $ip)) {
    jsonError('You have already voted on this site. Please wait for the cooldown.');
}

// Record vote
$success = VoteModel::recordVote($userId, $user['name'], $siteId, $ip);

if ($success) {
    // Calculate next vote time
    $nextVote = (time() + (VOTE_COOLDOWN_HOURS * 3600)) * 1000; // JavaScript timestamp
    
    jsonSuccess([
        'credits_earned' => $site['prize'],
        'vote_link' => trim($site['link']),
        'next_vote' => $nextVote
    ], 'Vote recorded! Thank you for supporting us.');
} else {
    jsonError('Failed to record vote. Please try again.');
}
