# Local Development Setup Guide

This guide will help you set up the PKO website locally on Windows with nginx.

## 📋 Prerequisites Checklist

- [x] nginx installed at `C:\nginx\`
- [ ] PHP 8.0+ installed
- [ ] Microsoft SQL Server (for PKO databases)
- [ ] SQL Server PDO driver for PHP

---

## 🔧 Step 1: Install PHP 8.0+

### Download PHP

1. Visit: https://windows.php.net/download/
2. Download **PHP 8.1+ (x64 Thread Safe)** ZIP
3. Extract to `C:\php\`

### Configure PHP

1. Copy `C:\php\php.ini-development` to `C:\php\php.ini`

2. Edit `C:\php\php.ini` and enable these extensions (remove the `;` at start of line):
   ```ini
   extension=openssl
   extension=pdo_sqlsrv
   extension=sqlsrv
   extension=mbstring
   extension=curl
   extension=fileinfo
   extension=gd
   ```

3. Add PHP to PATH:
   ```powershell
   # Run as Administrator
   [Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\php", "Machine")
   ```

4. Verify PHP installation:
   ```powershell
   php --version
   php -m  # List loaded modules
   ```

---

## 🔌 Step 2: Install SQL Server PHP Drivers

### Download Microsoft Drivers for PHP for SQL Server

1. Visit: https://docs.microsoft.com/en-us/sql/connect/php/download-drivers-php-sql-server
2. Download the latest version (5.10+)
3. Extract the ZIP file

### Install Drivers

1. Copy the appropriate driver DLLs to `C:\php\ext\`:
   - For PHP 8.1: `php_pdo_sqlsrv_81_ts_x64.dll` and `php_sqlsrv_81_ts_x64.dll`
   - For PHP 8.0: `php_pdo_sqlsrv_80_ts_x64.dll` and `php_sqlsrv_80_ts_x64.dll`

2. Already enabled in php.ini (from Step 1):
   ```ini
   extension=pdo_sqlsrv
   extension=sqlsrv
   ```

3. Verify drivers loaded:
   ```powershell
   php -m | Select-String -Pattern "sqlsrv"
   ```

---

## 🌐 Step 3: Configure nginx

### Update nginx.conf

1. Edit `C:\nginx\conf\nginx.conf`

2. Add this line inside the `http` block (near the end, before the last `}`):
   ```nginx
   include sites/*.conf;
   ```

3. Create sites directory:
   ```powershell
   New-Item -Path "C:\nginx\conf\sites" -ItemType Directory -Force
   ```

### nginx Configuration Already Created

The file `C:\nginx\conf\sites\pkodev.conf` has been created with:
- Listen on port **8080**
- Document root pointing to your website folder
- PHP-FPM configuration
- Security headers and access controls

---

## 🚀 Step 4: Setup PHP-FPM (FastCGI Process Manager)

### Download PHP-FPM for Windows

Since PHP for Windows doesn't include php-fpm by default, we'll use **php-cgi**:

1. No additional downloads needed - php-cgi.exe comes with PHP

2. Create a PHP startup script at `C:\php\start-php-cgi.bat`:
   ```batch
   @echo off
   echo Starting PHP FastCGI on port 9000...
   C:\php\php-cgi.exe -b 127.0.0.1:9000
   ```

---

## 🗄️ Step 5: Configure Database

### Create SQL Server User for Website

Run this in **SQL Server Management Studio**:

```sql
-- Create login for website
CREATE LOGIN pko_web WITH PASSWORD = 'YourSecurePassword123!';

-- Grant permissions to AccountServer
USE AccountServer;
CREATE USER pko_web FOR LOGIN pko_web;
GRANT SELECT, INSERT, UPDATE ON SCHEMA::dbo TO pko_web;

-- Grant permissions to GameDB
USE GameDB;
CREATE USER pko_web FOR LOGIN pko_web;
GRANT SELECT ON SCHEMA::dbo TO pko_web;

-- Grant permissions to WebsiteDB (if exists)
USE WebsiteDB;
CREATE USER pko_web FOR LOGIN pko_web;
GRANT SELECT, INSERT, UPDATE, DELETE ON SCHEMA::dbo TO pko_web;
```

### Verify .env Configuration

The `.env` file has been created with default development settings. Update these values if needed:

```env
DB_ACCOUNT_HOST=localhost\SQLExpress
DB_ACCOUNT_NAME=AccountServer
DB_ACCOUNT_USER=pko_web
DB_ACCOUNT_PASS=YourSecurePassword123!
```

---

## ▶️ Step 6: Start Services

### Start PHP FastCGI

```powershell
# In a new PowerShell window:
cd C:\php
.\start-php-cgi.bat
```

Keep this window open - PHP FastCGI is running.

### Start nginx

```powershell
# In another PowerShell window:
cd C:\nginx
.\nginx.exe
```

### Verify Services Running

```powershell
# Check nginx
netstat -ano | Select-String ":8080"

# Check PHP-CGI
netstat -ano | Select-String ":9000"
```

Both should show LISTENING status.

---

## 🧪 Step 7: Test Your Website

### Access the Website

Open your browser and visit:
- **Main Page:** http://localhost:8080/
- **Login:** http://localhost:8080/login.php
- **Register:** http://localhost:8080/register.php
- **Dashboard:** http://localhost:8080/dashboard.php

### Test PHP Configuration

1. Create test file `C:\Users\pisig\Desktop\Github\pkodev\website\phpinfo.php`:
   ```php
   <?php phpinfo(); ?>
   ```

2. Visit: http://localhost:8080/phpinfo.php

3. Check for:
   - PHP Version 8.0+
   - pdo_sqlsrv extension loaded
   - sqlsrv extension loaded

4. **Delete phpinfo.php after testing** (security!)

---

## 🛠️ Common Issues & Solutions

### Issue: "No input file specified"

**Solution:** Check that nginx document root path is correct:
```nginx
root "C:/Users/pisig/Desktop/Github/pkodev/website";
```

### Issue: PHP files download instead of executing

**Solution:** Verify PHP-FPM is running on port 9000:
```powershell
netstat -ano | Select-String ":9000"
```

### Issue: "Unable to connect to SQL Server"

**Solutions:**
1. Verify SQL Server is running
2. Check SQL Server is listening on TCP/IP (SQL Server Configuration Manager)
3. Verify firewall allows SQL Server connections
4. Test connection with SQL Server Management Studio first
5. Ensure pdo_sqlsrv extension is loaded: `php -m | Select-String "sqlsrv"`

### Issue: nginx won't start

**Solution:** Check for port conflicts:
```powershell
netstat -ano | Select-String ":8080"
```

If port 8080 is in use, edit `C:\nginx\conf\sites\pkodev.conf` and change:
```nginx
listen 8081;  # Or another available port
```

---

## 🔄 Restart Services

### Reload nginx (after config changes)

```powershell
cd C:\nginx
.\nginx.exe -s reload
```

### Stop nginx

```powershell
cd C:\nginx
.\nginx.exe -s stop
```

### Restart PHP-CGI

Press `Ctrl+C` in the PHP-CGI window, then run the batch file again.

---

## 📝 Development Workflow

1. **Edit code** in your IDE/editor
2. **Refresh browser** to see changes
3. **Check nginx error log** if issues occur:
   - `C:\nginx\logs\pkodev-error.log`
4. **Check PHP errors** in website:
   - `C:\Users\pisig\Desktop\Github\pkodev\website\logs\error.log`

---

## 🎯 Next Steps

Once everything is running:

1. ✅ Test user registration
2. ✅ Test user login
3. ✅ Verify database connections
4. ✅ Test shop functionality
5. ✅ Test leaderboard display
6. ✅ Configure vote rewards

---

## 🔒 Security Reminders

**Before deploying to production:**

1. Change JWT_SECRET and CSRF_SECRET in `.env`
2. Set `APP_DEBUG=false` in production
3. Use strong database passwords
4. Enable HTTPS (SSL certificate)
5. Configure firewall rules
6. Regular security updates

---

## 📞 Need Help?

Check the main README.md for more information or review the error logs:
- nginx: `C:\nginx\logs\pkodev-error.log`
- Website: Check includes/config.php for error handling
