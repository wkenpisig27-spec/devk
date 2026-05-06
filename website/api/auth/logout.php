<?php
/**
 * PKO Website API - Logout Endpoint
 */

require_once __DIR__ . '/../../includes/config.php';

header('Content-Type: application/json');
Security::setHeaders();

// Only accept POST
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    jsonError('Method not allowed', 405);
}

// Logout user
Auth::logout();

jsonSuccess([], 'Logged out successfully');
