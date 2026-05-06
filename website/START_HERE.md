# ✅ PKO Website - Setup Complete!

Your PKO website has been configured for local development with nginx.

---

## 📦 What's Been Set Up

### ✅ Configuration Files Created

| File | Location | Purpose |
|------|----------|---------|
| `.env` | `website/.env` | Environment variables & database config |
| `pkodev.conf` | `C:\nginx\conf\sites\pkodev.conf` | nginx site configuration |
| `start-php-cgi.bat` | `C:\php\start-php-cgi.bat` | PHP FastCGI startup script |

### ✅ Helper Scripts Created

| Script | Purpose |
|--------|---------|
| `start-local-server.bat` | **ONE-CLICK START** - Starts PHP & nginx automatically |
| `stop-local-server.bat` | Stops all services cleanly |
| `setup.ps1` | PowerShell setup (run once as Administrator) |

### ✅ Documentation Created

| Document | Description |
|----------|-------------|
| `QUICKSTART.md` | **START HERE** - Quick 3-step guide |
| `SETUP_LOCAL.md` | Complete setup instructions with troubleshooting |
| `test.php` | PHP environment test page |

---

## 🚀 Quick Start Guide

### **Step 1: Install PHP 8.0+** (One-time setup)

If you don't have PHP installed:

```powershell
# 1. Download PHP 8.1+ Thread Safe (x64) from:
#    https://windows.php.net/download/

# 2. Extract to: C:\php\

# 3. Copy php.ini-development to php.ini

# 4. Edit C:\php\php.ini and enable these extensions (remove semicolon):
#    extension=openssl
#    extension=pdo_sqlsrv
#    extension=sqlsrv
#    extension=mbstring
#    extension=curl

# 5. Install SQL Server drivers:
#    https://docs.microsoft.com/en-us/sql/connect/php/download-drivers-php-sql-server
#    Copy DLLs to C:\php\ext\

# 6. Add PHP to PATH (as Administrator):
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\php", "Machine")
```

**See `SETUP_LOCAL.md` for detailed PHP installation guide.**

---

### **Step 2: Run Setup Script** (One-time setup)

```powershell
# Right-click PowerShell → "Run as Administrator"
cd C:\Users\pisig\Desktop\Github\pkodev\website
.\setup.ps1
```

This configures nginx to use your site configuration.

---

### **Step 3: Start Your Website**

**EASIEST WAY - Just double-click:**
```
📂 website\start-local-server.bat
```

**Or manually:**
```powershell
# Terminal 1: Start PHP
cd C:\php
.\start-php-cgi.bat

# Terminal 2: Start nginx  
cd C:\nginx
.\nginx.exe
```

---

## 🌐 Access Your Website

Once started:

| Page | URL |
|------|-----|
| **Homepage** | http://localhost:8080/ |
| **Test PHP** | http://localhost:8080/test.php |
| Login | http://localhost:8080/login.php |
| Register | http://localhost:8080/register.php |
| Dashboard | http://localhost:8080/dashboard.php |
| Leaderboard | http://localhost:8080/leaderboard.php |

---

## 📊 System Configuration

### nginx Setup
```
Port:          8080
Document Root: C:/Users/pisig/Desktop/Github/pkodev/website
PHP-FPM:       127.0.0.1:9000
Config:        C:\nginx\conf\sites\pkodev.conf
Logs:          C:\nginx\logs\pkodev-*.log
```

### Website (.env)
```
Environment:   Development
Debug Mode:    Enabled
Database:      SQL Server (localhost\SQLExpress)
Server Name:   PKO Dev Server
```

---

## 🗄️ Database Setup (Required)

Your website connects to SQL Server databases. You need to:

### 1. Create Database User

Run in **SQL Server Management Studio**:

```sql
-- Create login
CREATE LOGIN pko_web WITH PASSWORD = 'YourSecurePassword123!';

-- Grant permissions to AccountServer
USE AccountServer;
CREATE USER pko_web FOR LOGIN pko_web;
GRANT SELECT, INSERT, UPDATE ON SCHEMA::dbo TO pko_web;

-- Grant permissions to GameDB
USE GameDB;
CREATE USER pko_web FOR LOGIN pko_web;
GRANT SELECT ON SCHEMA::dbo TO pko_web;

-- Grant permissions to WebsiteDB
USE WebsiteDB;
CREATE USER pko_web FOR LOGIN pko_web;
GRANT SELECT, INSERT, UPDATE, DELETE ON SCHEMA::dbo TO pko_web;
```

### 2. Update .env File

Edit `website\.env` with your database credentials:

```env
DB_ACCOUNT_HOST=localhost\SQLExpress
DB_ACCOUNT_NAME=AccountServer
DB_ACCOUNT_USER=pko_web
DB_ACCOUNT_PASS=YourSecurePassword123!

DB_GAME_HOST=localhost\SQLExpress
DB_GAME_NAME=GameDB
DB_GAME_USER=pko_web
DB_GAME_PASS=YourSecurePassword123!
```

---

## 🧪 Testing Your Setup

### 1. Test PHP Installation

Visit: http://localhost:8080/test.php

This page will check:
- ✅ PHP version (8.0+ required)
- ✅ SQL Server PDO driver
- ✅ Required extensions
- ✅ .env configuration
- ✅ Database connection

### 2. Test Website Features

- **Registration:** Create a test account
- **Login:** Log in with test account
- **Dashboard:** View player dashboard
- **Leaderboard:** Check rankings display

---

## 🔧 Troubleshooting

### Issue: PHP not found

```powershell
# Add PHP to PATH (as Administrator)
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\php", "Machine")
# Restart PowerShell
php --version
```

### Issue: Port 8080 already in use

Edit `C:\nginx\conf\sites\pkodev.conf`:
```nginx
listen 8081;  # Change to available port
```
Then restart nginx:
```powershell
cd C:\nginx
.\nginx.exe -s stop
.\nginx.exe
```

### Issue: nginx configuration errors

```powershell
# Test configuration
cd C:\nginx
.\nginx.exe -t
```

### Issue: Can't connect to SQL Server

1. ✅ SQL Server is running
2. ✅ TCP/IP enabled in SQL Server Configuration Manager
3. ✅ Firewall allows SQL Server (port 1433)
4. ✅ Database user created (see Database Setup above)
5. ✅ Credentials in `.env` are correct

### Issue: PHP files download instead of executing

PHP-CGI is not running. Start it:
```powershell
cd C:\php
.\start-php-cgi.bat
```

### Issue: "No input file specified"

nginx document root is incorrect. Verify in `C:\nginx\conf\sites\pkodev.conf`:
```nginx
root "C:/Users/pisig/Desktop/Github/pkodev/website";
```

---

## 🛑 Stopping the Server

**Easy way:**
```
📂 website\stop-local-server.bat
```

**Manual way:**
```powershell
# Stop nginx
cd C:\nginx
.\nginx.exe -s stop

# Stop PHP-CGI (press Ctrl+C in its window, or:)
taskkill /F /IM php-cgi.exe
```

---

## 📝 Development Workflow

1. **Start services:** Run `start-local-server.bat`
2. **Edit code:** Make changes to PHP files
3. **Test:** Refresh browser (changes are immediate)
4. **Check errors:**
   - nginx: `C:\nginx\logs\pkodev-error.log`
   - Website: `website\logs\error.log`
5. **Stop services:** Run `stop-local-server.bat`

---

## 🔐 Security Notes

### Development vs Production

Your `.env` is configured for **development**:
- Debug mode is enabled (shows errors)
- Weak JWT/CSRF secrets (for testing only)

**Before deploying to production:**

1. ✅ Change `JWT_SECRET` to secure 64-character random string
2. ✅ Change `CSRF_SECRET` to secure random string
3. ✅ Set `APP_DEBUG=false`
4. ✅ Set `APP_ENV=production`
5. ✅ Use strong database passwords
6. ✅ Enable HTTPS (SSL certificate)
7. ✅ Delete `test.php`
8. ✅ Review security headers

---

## 📚 Documentation

| Document | Description |
|----------|-------------|
| **QUICKSTART.md** | This file - Quick start guide |
| **SETUP_LOCAL.md** | Detailed setup instructions with all steps |
| **README.md** | Website features and API documentation |
| `includes/config.php` | Configuration loader (technical reference) |
| `includes/security.php` | Security features (technical reference) |

---

## 🎯 What's Next?

### Immediate Tasks
1. ✅ Run `setup.ps1` (as Administrator) - one time only
2. ✅ Install PHP 8.0+ if not installed
3. ✅ Run `start-local-server.bat`
4. ✅ Visit http://localhost:8080/test.php
5. ✅ Configure database user and update `.env`

### Website Development
Once everything is running:
- 🔨 Customize design in `assets/css/style.css`
- 🔨 Modify pages (index.php, login.php, etc.)
- 🔨 Add features in `api/` directory
- 🔨 Configure shop items in database
- 🔨 Set up vote rewards

---

## ✅ Summary Checklist

**Setup (One-time):**
- [ ] PHP 8.0+ installed at `C:\php\`
- [ ] SQL Server drivers installed
- [ ] Ran `setup.ps1` as Administrator
- [ ] Database user created
- [ ] `.env` configured with database credentials

**Every Development Session:**
- [ ] Run `start-local-server.bat`
- [ ] Visit http://localhost:8080/test.php to verify
- [ ] Develop and test features
- [ ] Run `stop-local-server.bat` when done

---

## 💡 Tips

- **Auto-start on boot:** Create a shortcut to `start-local-server.bat` in your Startup folder
- **Different port:** Edit `pkodev.conf` to change from 8080 to any port
- **Multiple projects:** Create additional `.conf` files in `C:\nginx\conf\sites\`
- **VSCode:** Install PHP and nginx extensions for better development experience
- **Debugging:** Enable Xdebug in `php.ini` for step-through debugging

---

## 📞 Need Help?

1. **Check test.php:** http://localhost:8080/test.php shows diagnostic info
2. **Review logs:**
   - nginx errors: `C:\nginx\logs\pkodev-error.log`
   - nginx access: `C:\nginx\logs\pkodev-access.log`
   - Website errors: `website\logs\error.log`
3. **Read detailed guide:** `SETUP_LOCAL.md` has troubleshooting section
4. **Verify services running:**
   ```powershell
   netstat -ano | Select-String ":8080"  # nginx
   netstat -ano | Select-String ":9000"  # PHP-CGI
   ```

---

**Happy Coding! 🚀**

Your PKO website is ready for local development. Start with `start-local-server.bat` and visit http://localhost:8080/test.php to verify everything is working.
