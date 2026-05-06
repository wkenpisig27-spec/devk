# PKO Website - Local Development Quick Start

## ✅ Files Created

Your website has been configured for local development. Here are the files created:

### Configuration Files
- ✅ `.env` - Environment configuration (already exists)
- ✅ `C:\nginx\conf\sites\pkodev.conf` - nginx site configuration
- ✅ `C:\php\start-php-cgi.bat` - PHP FastCGI startup script

### Helper Scripts
- ✅ `start-local-server.bat` - One-click server startup
- ✅ `stop-local-server.bat` - Stop all services
- ✅ `setup.ps1` - PowerShell setup script (run once as Administrator)
- ✅ `test.php` - PHP environment test page

### Documentation
- ✅ `SETUP_LOCAL.md` - Complete setup guide with troubleshooting

---

## 🚀 Quick Start (3 Steps)

### **Step 1: Install PHP** (if not installed)

```powershell
# Download PHP 8.1+ from: https://windows.php.net/download/
# Choose "VS16 x64 Thread Safe" ZIP

# Extract to C:\php\
# Copy php.ini-development to php.ini
# Edit php.ini and enable extensions (see SETUP_LOCAL.md)
```

### **Step 2: Run Setup Script** (Administrator)

```powershell
# Right-click PowerShell and "Run as Administrator"
cd C:\Users\pisig\Desktop\Github\pkodev\website
.\setup.ps1
```

This script will:
- Create nginx sites directory
- Update nginx.conf to include site configs
- Verify configurations
- Create required directories

### **Step 3: Start the Server**

```powershell
# Double-click this file:
start-local-server.bat
```

Or manually:
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

Once started, visit:
- **Homepage:** http://localhost:8080/
- **Test Page:** http://localhost:8080/test.php (check PHP config)
- **Login:** http://localhost:8080/login.php
- **Register:** http://localhost:8080/register.php

---

## 📋 Configuration Summary

### nginx Configuration
- **Port:** 8080
- **Document Root:** `C:/Users/pisig/Desktop/Github/pkodev/website`
- **PHP-FPM:** 127.0.0.1:9000
- **Config File:** `C:\nginx\conf\sites\pkodev.conf`

### Website Configuration (.env)
- **Environment:** Development
- **Debug Mode:** Enabled
- **Database:** SQL Server (localhost\SQLExpress)
- **Session:** pko_session

---

## 🔧 Troubleshooting

### PHP not found?
```powershell
# Add PHP to PATH (as Administrator)
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\php", "Machine")
# Restart PowerShell
```

### Port 8080 already in use?
Edit `C:\nginx\conf\sites\pkodev.conf` and change:
```nginx
listen 8081;  # Or another port
```

### nginx won't start?
```powershell
# Check configuration
cd C:\nginx
.\nginx.exe -t

# Check for running nginx processes
tasklist | findstr nginx
# Kill if needed
taskkill /F /IM nginx.exe
```

### Can't connect to SQL Server?
1. Ensure SQL Server is running
2. Enable TCP/IP in SQL Server Configuration Manager
3. Create database user (see SETUP_LOCAL.md Step 5)
4. Test connection with SQL Server Management Studio first

---

## 📝 Development Workflow

1. **Make changes** to PHP files in `website/` directory
2. **Refresh browser** - changes are immediate (no rebuild needed)
3. **Check errors:**
   - nginx: `C:\nginx\logs\pkodev-error.log`
   - Website: `website\logs\error.log`
4. **Stop services:** Run `stop-local-server.bat`

---

## 🔒 Before Production

When deploying to production:

1. **Update .env:**
   - Change `JWT_SECRET` and `CSRF_SECRET` to secure random strings
   - Set `APP_DEBUG=false`
   - Set `APP_ENV=production`
   - Use strong database passwords

2. **Security:**
   - Enable HTTPS (SSL certificate)
   - Configure firewall rules
   - Remove test.php file
   - Review security headers in nginx config

3. **Performance:**
   - Enable PHP OPcache
   - Configure nginx caching
   - Enable gzip compression
   - Use production database

---

## 📚 Additional Resources

- **Full Setup Guide:** `SETUP_LOCAL.md`
- **Website Documentation:** `README.md`
- **nginx Docs:** https://nginx.org/en/docs/
- **PHP Manual:** https://www.php.net/manual/

---

## 🎯 Next Steps

Once your server is running:

1. ✅ Visit http://localhost:8080/test.php to verify PHP setup
2. ✅ Configure database credentials in `.env`
3. ✅ Create SQL Server database user (see SETUP_LOCAL.md)
4. ✅ Test user registration and login
5. ✅ Explore the website features

---

Need help? Check `SETUP_LOCAL.md` for detailed troubleshooting and setup instructions.
