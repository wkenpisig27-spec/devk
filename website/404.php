<?php
/**
 * PKO Website - 404 Error Page
 */

require_once __DIR__ . '/includes/config.php';

http_response_code(404);
Security::setHeaders();
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <?= metaTags('Page Not Found', 'The page you are looking for could not be found.') ?>
    <link rel="stylesheet" href="<?= asset('css/style.css') ?>">
    <link rel="icon" type="image/png" href="<?= asset('images/favicon.png') ?>">
</head>
<body class="auth-page">
    <div class="text-center" style="max-width: 500px;">
        <div class="title-hero" style="font-size: 8rem; margin-bottom: 0;">404</div>
        <h1 class="text-3xl font-semibold mb-4">Ship Lost at Sea!</h1>
        <p class="text-secondary text-lg mb-8">
            The page you're looking for has drifted off course or never existed.
        </p>
        <div class="flex justify-center gap-4">
            <a href="/" class="btn btn-primary">Return Home</a>
            <a href="javascript:history.back()" class="btn btn-secondary">Go Back</a>
        </div>
    </div>
</body>
</html>
