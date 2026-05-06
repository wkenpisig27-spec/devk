# PKO Website Setup Checklist

Use this checklist to track your setup progress.

## ✅ Prerequisites (One-Time Setup)

### Step 1: Install PHP 8.0+
- [ ] Downloaded PHP 8.1+ Thread Safe (x64) from https://windows.php.net/download/
- [ ] Extracted to `C:\php\`
- [ ] Copied `php.ini-development` to `php.ini`
- [ ] Edited `php.ini` and enabled extensions:
  - [ ] `extension=openssl`
  - [ ] `extension=pdo_sqlsrv`
  - [ ] `extension=sqlsrv`
  - [ ] `extension=mbstring`
  - [ ] `extension=curl`
  - [ ] `extension=fileinfo`
- [ ] Added PHP to PATH (as Administrator):
  ```powershell
  [Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\php", "Machine")
  ```
- [ ] Verified installation: `php --version`

### Step 2: Install SQL Server PHP Drivers
- [ ] Downloaded Microsoft Drivers for PHP for SQL Server
- [ ] Copied appropriate DLLs to `C:\php\ext\`:
  - [ ] `php_pdo_sqlsrv_81_ts_x64.dll` (or your PHP version)
  - [ ] `php_sqlsrv_81_ts_x64.dll` (or your PHP version)
- [ ] Verified drivers loaded: `php -m | Select-String "sqlsrv"`

### Step 3: Configure nginx
- [ ] Ran `setup.ps1` as Administrator
- [ ] Verified `C:\nginx\conf\sites\pkodev.conf` exists
- [ ] Tested nginx configuration: `nginx.exe -t`

### Step 4: Database Setup
- [ ] SQL Server is installed and running
- [ ] Created database user `pko_web`:
  ```sql
  CREATE LOGIN pko_web WITH PASSWORD = 'YourPassword';
  ```
- [ ] Granted permissions to AccountServer database
- [ ] Granted permissions to GameDB database
- [ ] Granted permissions to WebsiteDB database
- [ ] Updated `.env` file with database credentials

---

## 🚀 Starting Your Website (Every Development Session)

### Quick Start Method
- [ ] Double-clicked `start-local-server.bat`
- [ ] PHP-CGI window opened (keep it open)
- [ ] nginx started in background

### Manual Start Method (Alternative)
- [ ] Terminal 1: Started PHP-CGI
  ```powershell
  cd C:\php
  .\start-php-cgi.bat
  ```
- [ ] Terminal 2: Started nginx
  ```powershell
  cd C:\nginx
  .\nginx.exe
  ```

---

## 🧪 Testing Your Setup

### Basic Tests
- [ ] Opened browser and visited: http://localhost:8080/
- [ ] Website homepage loads
- [ ] Visited test page: http://localhost:8080/test.php
- [ ] PHP version shows 8.0+
- [ ] SQL Server PDO driver is loaded
- [ ] All required extensions are loaded
- [ ] .env configuration is loaded
- [ ] Database connection successful

### Feature Tests
- [ ] Visited login page: http://localhost:8080/login.php
- [ ] Visited register page: http://localhost:8080/register.php
- [ ] Visited dashboard: http://localhost:8080/dashboard.php
- [ ] Created test account (if database is set up)
- [ ] Logged in successfully
- [ ] Dashboard displays correctly

---

## 📝 Development Workflow

### During Development
- [ ] Edited PHP files in `website/` directory
- [ ] Refreshed browser to see changes (no rebuild needed)
- [ ] Checked nginx error log if issues: `C:\nginx\logs\pkodev-error.log`
- [ ] Checked website error log: `website\logs\error.log`

### When Finished
- [ ] Stopped services: `stop-local-server.bat`
  Or:
- [ ] Closed PHP-CGI window
- [ ] Stopped nginx: `nginx.exe -s stop`

---

## 🔧 Troubleshooting

### If PHP Not Found
- [ ] Added PHP to PATH (see Step 1)
- [ ] Restarted PowerShell/Command Prompt
- [ ] Verified: `php --version`

### If Port 8080 Already in Use
- [ ] Changed port in `C:\nginx\conf\sites\pkodev.conf`
- [ ] Updated line: `listen 8081;` (or another port)
- [ ] Reloaded nginx: `nginx.exe -s reload`

### If PHP Files Download Instead of Execute
- [ ] Verified PHP-CGI is running on port 9000
- [ ] Checked: `netstat -ano | Select-String ":9000"`
- [ ] Restarted PHP-CGI if needed

### If Can't Connect to Database
- [ ] Verified SQL Server is running
- [ ] Enabled TCP/IP in SQL Server Configuration Manager
- [ ] Checked database credentials in `.env` are correct
- [ ] Verified database user has proper permissions
- [ ] Tested connection with SQL Server Management Studio

### If nginx Won't Start
- [ ] Tested configuration: `nginx.exe -t`
- [ ] Checked for port conflicts: `netstat -ano | Select-String ":8080"`
- [ ] Checked nginx error log: `C:\nginx\logs\error.log`
- [ ] Killed existing nginx processes: `taskkill /F /IM nginx.exe`

---

## 🎯 Next Steps After Setup

### Website Customization
- [ ] Reviewed website features in `README.md`
- [ ] Customized design in `assets/css/style.css`
- [ ] Updated server name in `.env`: `SERVER_NAME=My Server`
- [ ] Configured shop items in database
- [ ] Set up vote rewards: `VOTE_REWARD_POINTS=10`

### Security Configuration
- [ ] Changed `JWT_SECRET` to secure random string (64+ characters)
- [ ] Changed `CSRF_SECRET` to secure random string
- [ ] Used strong database passwords
- [ ] Reviewed security settings in `.env`

### Production Preparation (When Ready)
- [ ] Set `APP_ENV=production` in `.env`
- [ ] Set `APP_DEBUG=false` in `.env`
- [ ] Enabled HTTPS (SSL certificate)
- [ ] Configured firewall rules
- [ ] Deleted `test.php` file
- [ ] Reviewed security headers in nginx config
- [ ] Set up regular backups
- [ ] Configured monitoring

---

## 📚 Documentation Reference

- **START_HERE.md** - Complete overview with all details
- **QUICKSTART.md** - 3-step quick start guide
- **SETUP_LOCAL.md** - Detailed setup instructions with troubleshooting
- **ARCHITECTURE.md** - System architecture and request flow
- **README.md** - Website features and API documentation

---

## ✨ Success Criteria

Your setup is complete when:
- [x] PHP 8.0+ is installed and in PATH
- [x] SQL Server PHP drivers are loaded
- [x] nginx is configured with site config
- [x] Database user is created with proper permissions
- [x] `.env` is configured with correct credentials
- [x] `start-local-server.bat` starts both services
- [x] http://localhost:8080/ loads the homepage
- [x] http://localhost:8080/test.php shows all green checkmarks
- [x] Can register and login to test account

**Congratulations! Your PKO website is ready for development! 🎉**

---

## 💡 Quick Commands Reference

```powershell
# Start everything
.\start-local-server.bat

# Stop everything
.\stop-local-server.bat

# Check if services are running
netstat -ano | Select-String ":8080"  # nginx
netstat -ano | Select-String ":9000"  # PHP-CGI

# Reload nginx after config changes
cd C:\nginx
.\nginx.exe -s reload

# View recent errors
Get-Content C:\nginx\logs\pkodev-error.log -Tail 20
Get-Content website\logs\error.log -Tail 20
```

---

**Date Started:** _________________

**Date Completed:** _________________

**Notes:**
```
_____________________________________________________________________________

_____________________________________________________________________________

_____________________________________________________________________________
```
