<?php
/**
 * PKO Website - Download Page
 */

require_once __DIR__ . '/includes/config.php';

Security::setHeaders();
$stats = getServerStats();
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <?= metaTags('Download', 'Download the ' . SERVER_NAME . ' game client and start your pirate adventure!') ?>
    <link rel="stylesheet" href="<?= asset('css/style.css') ?>">
    <link rel="icon" type="image/png" href="<?= asset('images/favicon.png') ?>">
</head>
<body>
    <!-- Navigation -->
    <nav class="navbar scrolled" role="navigation">
        <div class="container">
            <div class="navbar-content">
                <a href="/" class="navbar-logo"><?= e(SERVER_NAME) ?></a>
                
                <ul class="navbar-menu" id="navbar-menu">
                    <li><a href="/" class="navbar-link">Home</a></li>
                    <li><a href="/news-list.php" class="navbar-link">News</a></li>
                    <li><a href="/donate.php" class="navbar-link">Item Mall</a></li>
                    <li><a href="/download.php" class="navbar-link">Download</a></li>
                    <li><a href="/leaderboard.php" class="navbar-link">Leaderboard</a></li>
                    <li><a href="https://discord.com/invite/NHmFsuMpS5" target="_blank" class="navbar-link">Discord</a></li>
                </ul>
                
                <div class="navbar-actions">
                    <?php if (Auth::check()): ?>
                        <a href="/dashboard.php" class="btn btn-ghost">Dashboard</a>
                        <a href="#" class="btn btn-primary" data-logout>Logout</a>
                    <?php else: ?>
                        <a href="/login.php" class="btn btn-ghost">Login</a>
                        <a href="/register.php" class="btn btn-primary">Play Free</a>
                    <?php endif; ?>
                </div>
                
                <button class="navbar-toggle" aria-label="Toggle menu" aria-expanded="false">
                    <svg width="24" height="24" fill="none" stroke="currentColor" stroke-width="2">
                        <path d="M4 6h16M4 12h16M4 18h16"></path>
                    </svg>
                </button>
            </div>
        </div>
    </nav>

    <div style="padding-top: 100px;"></div>

    <section class="section">
        <div class="container" style="max-width: 900px;">
            <div class="text-center mb-8">
                <h1 class="title-section text-center">Download Game</h1>
                <p class="text-secondary text-lg">Get the <?= e(SERVER_NAME) ?> client and start playing!</p>
            </div>

            <!-- Main Download -->
            <div class="card card-highlight text-center p-8 mb-8">
                <div class="stat-icon mb-4" style="margin: 0 auto; width: 80px; height: 80px;">
                    <svg width="40" height="40" fill="currentColor" viewBox="0 0 20 20">
                        <path fill-rule="evenodd" d="M3 17a1 1 0 011-1h12a1 1 0 110 2H4a1 1 0 01-1-1zm3.293-7.707a1 1 0 011.414 0L9 10.586V3a1 1 0 112 0v7.586l1.293-1.293a1 1 0 111.414 1.414l-3 3a1 1 0 01-1.414 0l-3-3a1 1 0 010-1.414z" clip-rule="evenodd"/>
                    </svg>
                </div>
                <h2 class="text-2xl font-semibold mb-2"><?= e(SERVER_NAME) ?> Full Client</h2>
                <p class="text-muted mb-6">Complete game installation with all necessary files</p>
                
                <div class="flex justify-center gap-4 flex-wrap mb-6">
                    <a href="https://drive.google.com/file/d/1cWf0TcvSWSfsrWKNbieny5IW2dg7KIfm/view" target="_blank" class="btn btn-primary btn-lg">
                        <svg width="20" height="20" fill="currentColor" viewBox="0 0 20 20">
                            <path fill-rule="evenodd" d="M3 17a1 1 0 011-1h12a1 1 0 110 2H4a1 1 0 01-1-1zm3.293-7.707a1 1 0 011.414 0L9 10.586V3a1 1 0 112 0v7.586l1.293-1.293a1 1 0 111.414 1.414l-3 3a1 1 0 01-1.414 0l-3-3a1 1 0 010-1.414z" clip-rule="evenodd"/>
                        </svg>
                        Download (Google Drive)
                    </a>
                    <a href="https://mega.nz/file/7cVX2T4b#Hns_rKQ7T7L2YB43U7AsqjAFBfrj4F-IJnB2vAhDgvY" target="_blank" class="btn btn-secondary btn-lg">
                        <svg width="20" height="20" fill="currentColor" viewBox="0 0 20 20">
                            <path fill-rule="evenodd" d="M3 17a1 1 0 011-1h12a1 1 0 110 2H4a1 1 0 01-1-1zm3.293-7.707a1 1 0 011.414 0L9 10.586V3a1 1 0 112 0v7.586l1.293-1.293a1 1 0 111.414 1.414l-3 3a1 1 0 01-1.414 0l-3-3a1 1 0 010-1.414z" clip-rule="evenodd"/>
                        </svg>
                        Download (MEGA)
                    </a>
                </div>
                
                <div class="grid grid-cols-3 gap-4 text-center">
                    <div>
                        <div class="text-gold font-semibold">~2 GB</div>
                        <div class="text-muted text-sm">File Size</div>
                    </div>
                    <div>
                        <div class="text-gold font-semibold">Windows</div>
                        <div class="text-muted text-sm">Platform</div>
                    </div>
                    <div>
                        <div class="text-gold font-semibold">v1.0</div>
                        <div class="text-muted text-sm">Version</div>
                    </div>
                </div>
            </div>

            <!-- Manual Patches -->
            <div class="card mb-8">
                <h3 class="text-lg font-semibold mb-4">
                    <svg width="20" height="20" fill="currentColor" viewBox="0 0 20 20" style="display: inline-block; vertical-align: middle; margin-right: 8px;">
                        <path fill-rule="evenodd" d="M4 4a2 2 0 012-2h4.586A2 2 0 0112 2.586L15.414 6A2 2 0 0116 7.414V16a2 2 0 01-2 2H6a2 2 0 01-2-2V4zm2 6a1 1 0 011-1h6a1 1 0 110 2H7a1 1 0 01-1-1zm1 3a1 1 0 100 2h6a1 1 0 100-2H7z" clip-rule="evenodd"/>
                    </svg>
                    Manual Patches
                </h3>
                <p class="text-muted mb-4">If the auto-updater doesn't work or you prefer manual updates, download patches here:</p>
                
                <div class="patch-list">
                    <div class="patch-item flex justify-between items-center py-3">
                        <div>
                            <span class="font-medium">Latest Manual Patch</span>
                            <span class="text-muted text-sm ml-2">— Contains all recent updates and fixes</span>
                        </div>
                        <div class="flex gap-2">
                            <a href="https://drive.google.com/file/d/1yWoZlSXMDha6r0UpqiqCg5YoB_N9wopQ/view" target="_blank" class="btn btn-sm btn-primary">
                                <svg width="16" height="16" fill="currentColor" viewBox="0 0 20 20">
                                    <path fill-rule="evenodd" d="M3 17a1 1 0 011-1h12a1 1 0 110 2H4a1 1 0 01-1-1zm3.293-7.707a1 1 0 011.414 0L9 10.586V3a1 1 0 112 0v7.586l1.293-1.293a1 1 0 111.414 1.414l-3 3a1 1 0 01-1.414 0l-3-3a1 1 0 010-1.414z" clip-rule="evenodd"/>
                                </svg>
                                Download
                            </a>
                        </div>
                    </div>
                </div>
                
                <div class="alert alert-info mt-4">
                    <svg class="alert-icon" fill="currentColor" viewBox="0 0 20 20">
                        <path fill-rule="evenodd" d="M18 10a8 8 0 11-16 0 8 8 0 0116 0zm-7-4a1 1 0 11-2 0 1 1 0 012 0zM9 9a1 1 0 000 2v3a1 1 0 001 1h1a1 1 0 100-2v-3a1 1 0 00-1-1H9z" clip-rule="evenodd"/>
                    </svg>
                    <div class="alert-content">
                        <div class="alert-title">How to Install Patches</div>
                        <ol style="margin: 0.5rem 0 0 1.25rem; padding: 0;">
                            <li>Download the latest manual patch</li>
                            <li>Extract so you will get the actual folder/files to patch</li>
                            <li>Copy and paste in your client files/</li>
                            <li>Make sure it will ask you to replace existing files</li>
                            <li>Run start.bat</li>
                        </ol>
                    </div>
                </div>
            </div>

            <!-- System Requirements -->
            <div class="grid grid-cols-2 gap-6 mb-8">
                <div class="card">
                    <h3 class="text-lg font-semibold mb-4">Minimum Requirements</h3>
                    <ul style="list-style: none;">
                        <li class="flex justify-between py-2 border-b border-light">
                            <span class="text-muted">OS</span>
                            <span>Windows 7/8/10/11</span>
                        </li>
                        <li class="flex justify-between py-2 border-b border-light">
                            <span class="text-muted">CPU</span>
                            <span>Intel Core 2 Duo</span>
                        </li>
                        <li class="flex justify-between py-2 border-b border-light">
                            <span class="text-muted">RAM</span>
                            <span>2 GB</span>
                        </li>
                        <li class="flex justify-between py-2 border-b border-light">
                            <span class="text-muted">Graphics</span>
                            <span>DirectX 9.0c compatible</span>
                        </li>
                        <li class="flex justify-between py-2">
                            <span class="text-muted">Storage</span>
                            <span>5 GB free space</span>
                        </li>
                    </ul>
                </div>
                
                <div class="card">
                    <h3 class="text-lg font-semibold mb-4">Recommended</h3>
                    <ul style="list-style: none;">
                        <li class="flex justify-between py-2 border-b border-light">
                            <span class="text-muted">OS</span>
                            <span>Windows 10/11</span>
                        </li>
                        <li class="flex justify-between py-2 border-b border-light">
                            <span class="text-muted">CPU</span>
                            <span>Intel Core i3 or better</span>
                        </li>
                        <li class="flex justify-between py-2 border-b border-light">
                            <span class="text-muted">RAM</span>
                            <span>4 GB</span>
                        </li>
                        <li class="flex justify-between py-2 border-b border-light">
                            <span class="text-muted">Graphics</span>
                            <span>DirectX 11 compatible</span>
                        </li>
                        <li class="flex justify-between py-2">
                            <span class="text-muted">Storage</span>
                            <span>10 GB free space (SSD)</span>
                        </li>
                    </ul>
                </div>
            </div>

            <!-- Installation Guide -->
            <div class="card mb-8">
                <h3 class="text-lg font-semibold mb-4">Installation Guide</h3>
                <ol class="text-secondary" style="list-style: decimal; padding-left: 1.5rem;">
                    <li class="mb-3">
                        <strong class="text-primary">Download the client</strong> using one of the links above
                    </li>
                    <li class="mb-3">
                        <strong class="text-primary">Extract the ZIP file</strong> to a folder of your choice (e.g., C:\Games\PKO)
                    </li>
                    <li class="mb-3">
                        <strong class="text-primary">Run the launcher</strong> by double-clicking <code class="bg-tertiary px-2 py-1 rounded">Slime Pirates.exe</code>
                    </li>
                    <li class="mb-3">
                        <strong class="text-primary">Create an account</strong> using the Register button above, or log in with existing credentials
                    </li>
                    <li>
                        <strong class="text-primary">Enjoy your adventure!</strong> Create your character and start playing
                    </li>
                </ol>
                
                <div class="alert alert-warning mt-6">
                    <svg class="alert-icon" fill="currentColor" viewBox="0 0 20 20">
                        <path fill-rule="evenodd" d="M8.257 3.099c.765-1.36 2.722-1.36 3.486 0l5.58 9.92c.75 1.334-.213 2.98-1.742 2.98H4.42c-1.53 0-2.493-1.646-1.743-2.98l5.58-9.92zM11 13a1 1 0 11-2 0 1 1 0 012 0zm-1-8a1 1 0 00-1 1v3a1 1 0 002 0V6a1 1 0 00-1-1z" clip-rule="evenodd"/>
                    </svg>
                    <div class="alert-content">
                        <div class="alert-title">Important</div>
                        <p>Some antivirus software may flag the client. This is a false positive - please add an exception for the game folder.</p>
                    </div>
                </div>
            </div>

            <!-- Troubleshooting -->
            <div class="card">
                <h3 class="text-lg font-semibold mb-4">Troubleshooting</h3>
                
                <details class="mb-4">
                    <summary class="cursor-pointer font-medium py-2">Game won't start / DirectX error</summary>
                    <div class="text-muted pt-2 pl-4">
                        Install the latest DirectX End-User Runtime from Microsoft's website. Also ensure your graphics drivers are up to date.
                    </div>
                </details>
                
                <details class="mb-4">
                    <summary class="cursor-pointer font-medium py-2">Cannot connect to server</summary>
                    <div class="text-muted pt-2 pl-4">
                        Check if the server is online (status shown at the bottom of this page). Ensure your firewall isn't blocking the game.
                    </div>
                </details>
                
                <details class="mb-4">
                    <summary class="cursor-pointer font-medium py-2">Game crashes on character creation</summary>
                    <div class="text-muted pt-2 pl-4">
                        Try running the game as Administrator. Right-click Slime Pirates.exe → Properties → Compatibility → Run as administrator.
                    </div>
                </details>
                
                <details>
                    <summary class="cursor-pointer font-medium py-2">Other issues</summary>
                    <div class="text-muted pt-2 pl-4">
                        Join our Discord server for community support and announcements. Our team and players are happy to help!
                    </div>
                </details>
            </div>
        </div>
    </section>

    <!-- Footer -->
    <footer class="footer">
        <div class="container">
            <div class="footer-grid">
                <div class="footer-brand">
                    <a href="/" class="footer-logo"><?= e(SERVER_NAME) ?></a>
                    <p class="footer-description">
                        Embark on the ultimate pirate MMORPG adventure. Sail the seas, battle enemies, and become a legend!
                    </p>
                    <div class="footer-social">
                        <a href="https://discord.com/invite/NHmFsuMpS5" target="_blank" aria-label="Discord">
                            <svg width="20" height="20" fill="currentColor" viewBox="0 0 24 24"><path d="M20.317 4.37a19.791 19.791 0 0 0-4.885-1.515.074.074 0 0 0-.079.037c-.21.375-.444.864-.608 1.25a18.27 18.27 0 0 0-5.487 0 12.64 12.64 0 0 0-.617-1.25.077.077 0 0 0-.079-.037A19.736 19.736 0 0 0 3.677 4.37a.07.07 0 0 0-.032.027C.533 9.046-.32 13.58.099 18.057a.082.082 0 0 0 .031.057 19.9 19.9 0 0 0 5.993 3.03.078.078 0 0 0 .084-.028 14.09 14.09 0 0 0 1.226-1.994.076.076 0 0 0-.041-.106 13.107 13.107 0 0 1-1.872-.892.077.077 0 0 1-.008-.128 10.2 10.2 0 0 0 .372-.292.074.074 0 0 1 .077-.01c3.928 1.793 8.18 1.793 12.062 0a.074.074 0 0 1 .078.01c.12.098.246.198.373.292a.077.077 0 0 1-.006.127 12.299 12.299 0 0 1-1.873.892.077.077 0 0 0-.041.107c.36.698.772 1.362 1.225 1.993a.076.076 0 0 0 .084.028 19.839 19.839 0 0 0 6.002-3.03.077.077 0 0 0 .032-.054c.5-5.177-.838-9.674-3.549-13.66a.061.061 0 0 0-.031-.03zM8.02 15.33c-1.183 0-2.157-1.085-2.157-2.419 0-1.333.956-2.419 2.157-2.419 1.21 0 2.176 1.096 2.157 2.42 0 1.333-.956 2.418-2.157 2.418zm7.975 0c-1.183 0-2.157-1.085-2.157-2.419 0-1.333.955-2.419 2.157-2.419 1.21 0 2.176 1.096 2.157 2.42 0 1.333-.946 2.418-2.157 2.418z"/></svg>
                        </a>
                        <a href="https://www.facebook.com/share/1AL6yMoj2g/?mibextid=wwXIfr" target="_blank" aria-label="Facebook">
                            <svg width="20" height="20" fill="currentColor" viewBox="0 0 24 24"><path d="M24 12.073c0-6.627-5.373-12-12-12s-12 5.373-12 12c0 5.99 4.388 10.954 10.125 11.854v-8.385H7.078v-3.47h3.047V9.43c0-3.007 1.792-4.669 4.533-4.669 1.312 0 2.686.235 2.686.235v2.953H15.83c-1.491 0-1.956.925-1.956 1.874v2.25h3.328l-.532 3.47h-2.796v8.385C19.612 23.027 24 18.062 24 12.073z"/></svg>
                        </a>
                        <a href="https://www.instagram.com/slime.piratesonline" target="_blank" aria-label="Instagram">
                            <svg width="20" height="20" fill="currentColor" viewBox="0 0 24 24"><path d="M12 2.163c3.204 0 3.584.012 4.85.07 3.252.148 4.771 1.691 4.919 4.919.058 1.265.069 1.645.069 4.849 0 3.205-.012 3.584-.069 4.849-.149 3.225-1.664 4.771-4.919 4.919-1.266.058-1.644.07-4.85.07-3.204 0-3.584-.012-4.849-.07-3.26-.149-4.771-1.699-4.919-4.92-.058-1.265-.07-1.644-.07-4.849 0-3.204.013-3.583.07-4.849.149-3.227 1.664-4.771 4.919-4.919 1.266-.057 1.645-.069 4.849-.069zM12 0C8.741 0 8.333.014 7.053.072 2.695.272.273 2.69.073 7.052.014 8.333 0 8.741 0 12c0 3.259.014 3.668.072 4.948.2 4.358 2.618 6.78 6.98 6.98C8.333 23.986 8.741 24 12 24c3.259 0 3.668-.014 4.948-.072 4.354-.2 6.782-2.618 6.979-6.98.059-1.28.073-1.689.073-4.948 0-3.259-.014-3.667-.072-4.947-.196-4.354-2.617-6.78-6.979-6.98C15.668.014 15.259 0 12 0zm0 5.838a6.162 6.162 0 100 12.324 6.162 6.162 0 000-12.324zM12 16a4 4 0 110-8 4 4 0 010 8zm6.406-11.845a1.44 1.44 0 100 2.881 1.44 1.44 0 000-2.881z"/></svg>
                        </a>
                        <a href="https://www.tiktok.com/@slime.pirates.onl" target="_blank" aria-label="TikTok">
                            <svg width="20" height="20" fill="currentColor" viewBox="0 0 24 24"><path d="M19.59 6.69a4.83 4.83 0 01-3.77-4.25V2h-3.45v13.67a2.89 2.89 0 01-5.2 1.74 2.89 2.89 0 012.31-4.64 2.93 2.93 0 01.88.13V9.4a6.84 6.84 0 00-1-.05A6.33 6.33 0 005 20.1a6.34 6.34 0 0010.86-4.43v-7a8.16 8.16 0 004.77 1.52v-3.4a4.85 4.85 0 01-1-.1z"/></svg>
                        </a>
                        <a href="https://youtube.com/@slimepiratesonline?si=ksWNk6Fe6ZSTx5nt" target="_blank" aria-label="YouTube">
                            <svg width="20" height="20" fill="currentColor" viewBox="0 0 24 24"><path d="M23.498 6.186a3.016 3.016 0 0 0-2.122-2.136C19.505 3.545 12 3.545 12 3.545s-7.505 0-9.377.505A3.017 3.017 0 0 0 .502 6.186C0 8.07 0 12 0 12s0 3.93.502 5.814a3.016 3.016 0 0 0 2.122 2.136c1.871.505 9.376.505 9.376.505s7.505 0 9.377-.505a3.015 3.015 0 0 0 2.122-2.136C24 15.93 24 12 24 12s0-3.93-.502-5.814zM9.545 15.568V8.432L15.818 12l-6.273 3.568z"/></svg>
                        </a>
                    </div>
                </div>
                
                <div>
                    <h4 class="footer-title">Game</h4>
                    <ul class="footer-links">
                        <li><a href="/download.php">Download</a></li>
                        <li><a href="/register.php">Register</a></li>
                        <li><a href="/leaderboard.php">Leaderboard</a></li>
                    </ul>
                </div>
                
                <div>
                    <h4 class="footer-title">Community</h4>
                    <ul class="footer-links">
                        <li><a href="https://discord.com/invite/NHmFsuMpS5" target="_blank">Discord</a></li>
                        <li><a href="https://discord.com/invite/NHmFsuMpS5" target="_blank">Support</a></li>
                    </ul>
                </div>
                
                <div>
                    <h4 class="footer-title">Legal</h4>
                    <ul class="footer-links">
                        <li><a href="/terms.php">Terms of Service</a></li>
                        <li><a href="/privacy.php">Privacy Policy</a></li>
                        <li><a href="/rules.php">Game Rules</a></li>
                        <li><a href="/refund.php">Refund Policy</a></li>
                        <li><a href="/donate.php">Item Mall</a></li>
                    </ul>
                </div>
            </div>
            
            <div class="footer-bottom">
                <p>&copy; <?= date('Y') ?> <?= e(SERVER_NAME) ?>. All rights reserved.</p>
            </div>
        </div>
    </footer>

    <script src="<?= asset('js/main.js') ?>"></script>
    <style>
        details summary::-webkit-details-marker { display: none; }
        details summary::before { content: '▸ '; color: var(--gold-400); }
        details[open] summary::before { content: '▾ '; }
        code { font-family: monospace; }
        .bg-tertiary { background: var(--bg-tertiary); }
        .patch-item:last-child { border-bottom: none; }
        .patch-item:hover { background: var(--bg-secondary); margin: 0 -1rem; padding-left: 1rem; padding-right: 1rem; border-radius: 0.5rem; }
        .items-center { align-items: center; }
    </style>
</body>
</html>
