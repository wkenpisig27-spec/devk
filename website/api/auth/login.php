<?php
/**
 * PKO Website API - Login Endpoint
 */

require_once __DIR__ . '/../../includes/config.php';

header('Content-Type: application/json');
Security::setHeaders();

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
$validator = new Validator($input);
$validator
    ->required('username', 'Username')
    ->required('password', 'Password');

if ($validator->fails()) {
    jsonError($validator->firstError());
}

// Attempt login
$result = Auth::attempt(
    trim($input['username']),
    $input['password']
);

if ($result['success']) {
    jsonSuccess([
        'token' => $result['token'],
        'user' => $result['user']
    ], 'Login successful');
} else {
    jsonError($result['error'], 401);
}
