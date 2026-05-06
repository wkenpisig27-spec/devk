# PKO Website - Local Development Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    YOUR DEVELOPMENT MACHINE                  │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Browser: http://localhost:8080/                     │   │
│  └──────────────────┬───────────────────────────────────┘   │
│                     │                                         │
│                     ↓                                         │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  nginx (Port 8080)                                   │   │
│  │  C:\nginx\                                           │   │
│  │  Config: conf/sites/pkodev.conf                     │   │
│  │                                                      │   │
│  │  • Serves static files (CSS, JS, images)           │   │
│  │  • Routes .php requests to PHP-CGI                 │   │
│  │  • Security headers & access control               │   │
│  └──────────────────┬───────────────────────────────────┘   │
│                     │                                         │
│                     ↓                                         │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  PHP-CGI FastCGI (Port 9000)                        │   │
│  │  C:\php\php-cgi.exe                                 │   │
│  │                                                      │   │
│  │  • Executes PHP code                                │   │
│  │  • Loads .env configuration                         │   │
│  │  • Connects to databases                            │   │
│  └──────────────────┬───────────────────────────────────┘   │
│                     │                                         │
│                     ↓                                         │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  PKO Website Files                                   │   │
│  │  C:\Users\pisig\Desktop\Github\pkodev\website\      │   │
│  │                                                      │   │
│  │  • index.php, login.php, register.php, etc.        │   │
│  │  • includes/ (config, database, security)          │   │
│  │  • assets/ (CSS, JS, images)                       │   │
│  │  • api/ (REST endpoints)                           │   │
│  └──────────────────┬───────────────────────────────────┘   │
│                     │                                         │
│                     ↓                                         │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Microsoft SQL Server                                │   │
│  │  localhost\SQLExpress                                │   │
│  │                                                      │   │
│  │  • AccountServer (user accounts, authentication)   │   │
│  │  • GameDB (characters, items, guilds)              │   │
│  │  • WebsiteDB (votes, shop purchases, logs)         │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

## Request Flow

```
1. User visits: http://localhost:8080/login.php

2. nginx receives request
   ├─ Checks: Is this a .php file?
   └─ Yes → Forward to PHP-CGI on port 9000

3. PHP-CGI processes login.php
   ├─ Loads .env configuration
   ├─ Loads includes/config.php
   ├─ Loads includes/security.php
   └─ Executes login form rendering

4. User submits login form
   ├─ POST to api/auth/login.php
   ├─ PHP validates credentials
   ├─ Queries AccountServer database
   ├─ Generates JWT token
   └─ Sets session cookie

5. User navigates to dashboard.php
   ├─ PHP checks JWT/session
   ├─ Queries GameDB for character data
   └─ Renders dashboard with player info
```

## File Structure

```
C:\nginx\
├── nginx.exe                       # Web server executable
├── conf\
│   ├── nginx.conf                 # Main config (includes sites/*.conf)
│   └── sites\
│       └── pkodev.conf           # ← YOUR SITE CONFIG
└── logs\
    ├── pkodev-access.log         # Request logs
    └── pkodev-error.log          # Error logs

C:\php\
├── php.exe                        # PHP CLI
├── php-cgi.exe                    # FastCGI processor
├── php.ini                        # PHP configuration
├── start-php-cgi.bat             # ← STARTUP SCRIPT
└── ext\
    ├── php_pdo_sqlsrv.dll        # SQL Server PDO driver
    └── php_sqlsrv.dll            # SQL Server driver

C:\Users\pisig\Desktop\Github\pkodev\website\
├── .env                           # ← CONFIGURATION
├── .env.example                   # Configuration template
├── index.php                      # Homepage
├── login.php                      # Login page
├── register.php                   # Registration
├── dashboard.php                  # Player dashboard
├── test.php                       # ← PHP TEST PAGE
│
├── includes\                      # Core PHP logic
│   ├── config.php                # Configuration loader
│   ├── database.php              # Database models
│   ├── security.php              # Authentication & security
│   └── helpers.php               # Utility functions
│
├── api\                           # REST API endpoints
│   ├── auth\                     # Login, register, logout
│   ├── shop\                     # Shop purchases
│   └── vote.php                  # Voting system
│
├── assets\                        # Static files
│   ├── css\style.css             # Stylesheet
│   ├── js\main.js                # JavaScript
│   └── images\                   # Images
│
├── logs\                          # Application logs
│   └── error.log                 # PHP errors
│
├── START_HERE.md                  # ← QUICK OVERVIEW
├── QUICKSTART.md                  # ← 3-STEP GUIDE
├── SETUP_LOCAL.md                 # ← DETAILED GUIDE
│
├── setup.ps1                      # ← SETUP SCRIPT (run as Admin)
├── start-local-server.bat        # ← START EVERYTHING
└── stop-local-server.bat         # ← STOP EVERYTHING
```

## Configuration Files

### .env (Environment Variables)
```
DB_ACCOUNT_HOST=localhost\SQLExpress
DB_ACCOUNT_NAME=AccountServer
DB_ACCOUNT_USER=pko_web
DB_ACCOUNT_PASS=YourPassword

JWT_SECRET=your_secret_key
APP_ENV=development
APP_DEBUG=true
```

### C:\nginx\conf\sites\pkodev.conf (nginx)
```nginx
server {
    listen 8080;
    root "C:/Users/pisig/Desktop/Github/pkodev/website";
    
    location ~ \.php$ {
        fastcgi_pass 127.0.0.1:9000;
        # ... FastCGI config
    }
}
```

### C:\php\php.ini (PHP Configuration)
```ini
extension=openssl
extension=pdo_sqlsrv
extension=sqlsrv
extension=mbstring
extension=curl
```

## Development Workflow

```
┌─────────────────────────────────────────────────────────┐
│  DEVELOPMENT CYCLE                                       │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  1. START SERVICES                                       │
│     • Double-click: start-local-server.bat              │
│     • Opens: PHP-CGI window (keep open)                 │
│     • Starts: nginx (background)                        │
│                                                          │
│  2. DEVELOP                                              │
│     • Edit files in: website/                           │
│     • Changes are IMMEDIATE (no rebuild)                │
│     • Refresh browser to see updates                    │
│                                                          │
│  3. TEST                                                 │
│     • Visit: http://localhost:8080/                     │
│     • Check: http://localhost:8080/test.php            │
│     • Test: Registration, login, features               │
│                                                          │
│  4. DEBUG                                                │
│     • Check nginx logs:                                 │
│       C:\nginx\logs\pkodev-error.log                    │
│     • Check PHP errors:                                 │
│       website\logs\error.log                            │
│     • Use browser DevTools (F12)                        │
│                                                          │
│  5. STOP SERVICES                                        │
│     • Double-click: stop-local-server.bat               │
│     • Or close PHP-CGI window                           │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

## Port Mapping

```
┌──────────┬─────────────────────┬──────────────────┐
│   Port   │      Service        │   Access From    │
├──────────┼─────────────────────┼──────────────────┤
│   8080   │  nginx (Web Server) │  Browser         │
│   9000   │  PHP-CGI (FastCGI)  │  nginx only      │
│   1433   │  SQL Server         │  PHP only        │
└──────────┴─────────────────────┴──────────────────┘
```

## Database Architecture

```
┌──────────────────────────────────────────────────────┐
│  SQL Server Instance: localhost\SQLExpress           │
├──────────────────────────────────────────────────────┤
│                                                       │
│  ┌─────────────────────────────────────────────┐    │
│  │  AccountServer Database                     │    │
│  │  • User accounts (actName, pass, email)     │    │
│  │  • Login tracking                           │    │
│  │  • Account status (banned, active)          │    │
│  └─────────────────────────────────────────────┘    │
│                                                       │
│  ┌─────────────────────────────────────────────┐    │
│  │  GameDB Database                            │    │
│  │  • Characters (stats, equipment, position)  │    │
│  │  • Guilds                                   │    │
│  │  • Items & inventory                        │    │
│  │  • NPCs & monsters                          │    │
│  └─────────────────────────────────────────────┘    │
│                                                       │
│  ┌─────────────────────────────────────────────┐    │
│  │  WebsiteDB Database                         │    │
│  │  • Vote logs                                │    │
│  │  • Shop purchase history                    │    │
│  │  • Web sessions                             │    │
│  │  • News/announcements                       │    │
│  └─────────────────────────────────────────────┘    │
│                                                       │
│  Database User: pko_web                              │
│  Password: (from .env)                               │
│                                                       │
└──────────────────────────────────────────────────────┘
```

## Security Layer

```
Browser Request
       ↓
  ┌─────────────────────────┐
  │  nginx Security Headers │
  │  • X-Frame-Options      │
  │  • X-Content-Type       │
  │  • X-XSS-Protection     │
  └────────┬────────────────┘
           ↓
  ┌─────────────────────────┐
  │  PHP Security Layer     │
  │  includes/security.php  │
  │                         │
  │  • CSRF Protection      │
  │  • JWT Authentication   │
  │  • Rate Limiting        │
  │  • Input Validation     │
  │  • SQL Injection Guard  │
  └────────┬────────────────┘
           ↓
  ┌─────────────────────────┐
  │  Database Layer         │
  │  Parameterized Queries  │
  └─────────────────────────┘
```

## Performance Optimization

```
Static Files (CSS, JS, Images)
    ↓
nginx caching (7 days)
    ↓
Browser cache
    ↓
Fast delivery ⚡

Dynamic PHP Files
    ↓
PHP-CGI execution
    ↓
Database query (with connection pooling)
    ↓
Response generation
    ↓
~50-200ms response time
```

## Common URLs

```
Homepage:       http://localhost:8080/
Test PHP:       http://localhost:8080/test.php
Login:          http://localhost:8080/login.php
Register:       http://localhost:8080/register.php
Dashboard:      http://localhost:8080/dashboard.php
Leaderboard:    http://localhost:8080/leaderboard.php
Shop:           http://localhost:8080/shop.php
Vote:           http://localhost:8080/vote.php

API Endpoints:
Login:          POST http://localhost:8080/api/auth/login.php
Register:       POST http://localhost:8080/api/auth/register.php
Shop Purchase:  POST http://localhost:8080/api/shop/purchase.php
Vote:           POST http://localhost:8080/api/vote.php
```

## Quick Commands Reference

```powershell
# Start everything (easiest)
.\start-local-server.bat

# Stop everything
.\stop-local-server.bat

# Check if nginx is running
netstat -ano | Select-String ":8080"

# Check if PHP-CGI is running
netstat -ano | Select-String ":9000"

# Test nginx config
cd C:\nginx
.\nginx.exe -t

# Reload nginx (after config changes)
cd C:\nginx
.\nginx.exe -s reload

# Stop nginx
cd C:\nginx
.\nginx.exe -s stop

# View nginx error log
Get-Content C:\nginx\logs\pkodev-error.log -Tail 20

# View website error log
Get-Content C:\Users\pisig\Desktop\Github\pkodev\website\logs\error.log -Tail 20
```

---

**Setup Complete! Ready to develop your PKO website locally. 🚀**

See [START_HERE.md](START_HERE.md) for getting started.
