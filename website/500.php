<?php
/**
 * PKO Website - 500 Error Page
 */

// Don't load config here as it might be the cause of the error
http_response_code(500);
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Server Error | Pirate King Online</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #0a0e17;
            color: #f1f5f9;
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            text-align: center;
            padding: 2rem;
        }
        h1 { font-size: 8rem; color: #ffc61a; font-weight: 700; line-height: 1; }
        h2 { font-size: 1.5rem; margin: 1rem 0 0.5rem; }
        p { color: #94a3b8; margin-bottom: 2rem; max-width: 400px; }
        a {
            display: inline-block;
            padding: 0.75rem 1.5rem;
            background: linear-gradient(135deg, #ffc61a 0%, #e6ac00 100%);
            color: #0a0e17;
            text-decoration: none;
            border-radius: 0.5rem;
            font-weight: 600;
        }
        a:hover { transform: translateY(-2px); }
    </style>
</head>
<body>
    <div>
        <h1>500</h1>
        <h2>Server Hit an Iceberg!</h2>
        <p>Something went wrong on our end. Our crew is working to fix it. Please try again later.</p>
        <a href="/">Return to Safety</a>
    </div>
</body>
</html>
