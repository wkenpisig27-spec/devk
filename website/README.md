# PKO Website Documentation

A complete, production-ready website for **Pirate King Online** private servers.

## 📁 Project Structure

```
website/
├── api/                    # REST API endpoints
│   ├── auth/
│   │   ├── login.php       # User login
│   │   ├── register.php    # User registration
│   │   └── logout.php      # User logout
│   ├── shop/
│   │   └── purchase.php    # Item shop purchases
│   ├── leaderboard.php     # Leaderboard data
│   ├── status.php          # Server status
│   └── vote.php            # Voting system
├── assets/
│   ├── css/
│   │   └── style.css       # Complete design system
│   ├── js/
│   │   └── main.js         # Client-side JavaScript
│   └── images/             # Website images
│       ├── classes/        # Character class icons
│       ├── items/          # Shop item icons
│       └── ...
├── includes/
│   ├── config.php          # Configuration loader
│   ├── database.php        # Database connection & models
│   ├── security.php        # Auth, JWT, CSRF, rate limiting
│   └── helpers.php         # Utility functions
├── pages/                  # Additional page partials
├── admin/                  # Admin panel (future)
├── .env.example            # Environment template
├── index.php               # Landing page
├── login.php               # Login page
├── register.php            # Registration page
├── dashboard.php           # Player dashboard
├── leaderboard.php         # Rankings page
├── vote.php                # Vote for rewards
├── shop.php                # Item shop
└── download.php            # Game download
```

## 🚀 Setup Instructions

### Prerequisites

- **PHP 8.0+** with extensions:
  - `pdo`
  - `pdo_sqlsrv` (SQL Server driver)
  - `json`
  - `openssl`
  - `mbstring`
- **Microsoft SQL Server** (the PKO databases)
- **Web Server**: Apache, Nginx, or IIS

### Installation Steps

1. **Copy website files** to your web server's document root

2. **Copy environment template**:
   ```bash
   cp .env.example .env
   ```

3. **Configure .env file** with your settings:
   ```env
   # Database
   DB_ACCOUNT_HOST=localhost\SQLExpress
   DB_ACCOUNT_NAME=AccountServer
   DB_ACCOUNT_USER=pko_web
   DB_ACCOUNT_PASS=your_secure_password

   DB_GAME_HOST=localhost\SQLExpress
   DB_GAME_NAME=GameDB
   DB_GAME_USER=pko_web
   DB_GAME_PASS=your_secure_password

   DB_WEB_HOST=localhost\SQLExpress
   DB_WEB_NAME=WebsiteDB
   DB_WEB_USER=pko_web
   DB_WEB_PASS=your_secure_password

   # Security - CHANGE THESE!
   JWT_SECRET=generate_a_64_character_random_string
   CSRF_SECRET=generate_another_random_string

   # Server info
   SERVER_NAME=My PKO Server
   SITE_URL=https://yoursite.com
   ```

4. **Create SQL user for the website**:
   ```sql
   -- Run in SQL Server Management Studio
   CREATE LOGIN pko_web WITH PASSWORD = 'your_secure_password';
   
   USE AccountServer;
   CREATE USER pko_web FOR LOGIN pko_web;
   GRANT SELECT, INSERT, UPDATE, EXECUTE TO pko_web;
   
   USE GameDB;
   CREATE USER pko_web FOR LOGIN pko_web;
   GRANT SELECT, UPDATE TO pko_web;
   
   USE WebsiteDB;
   CREATE USER pko_web FOR LOGIN pko_web;
   GRANT SELECT, INSERT, UPDATE TO pko_web;
   ```

5. **Set correct file permissions** (Linux):
   ```bash
   chmod 600 .env
   chmod 755 -R website/
   ```

6. **Configure web server URL rewriting** (optional, for clean URLs)

### Apache Configuration
```apache
<VirtualHost *:80>
    ServerName yoursite.com
    DocumentRoot /var/www/pko-website
    
    <Directory /var/www/pko-website>
        Options -Indexes +FollowSymLinks
        AllowOverride All
        Require all granted
    </Directory>
    
    # Security headers
    Header set X-Content-Type-Options "nosniff"
    Header set X-Frame-Options "SAMEORIGIN"
    Header set X-XSS-Protection "1; mode=block"
</VirtualHost>
```

### Nginx Configuration
```nginx
server {
    listen 80;
    server_name yoursite.com;
    root /var/www/pko-website;
    index index.php;
    
    location / {
        try_files $uri $uri/ /index.php?$query_string;
    }
    
    location ~ \.php$ {
        fastcgi_pass unix:/run/php/php8.0-fpm.sock;
        fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
        include fastcgi_params;
    }
    
    location ~ /\.env {
        deny all;
    }
}
```

## 🔒 Security Features

### Authentication
- **JWT tokens** for API authentication
- **bcrypt password hashing** (configurable cost factor)
- **CSRF protection** on all forms
- **Session security** with HTTP-only cookies

### Rate Limiting
- Login attempts limited (5 per 15 minutes by default)
- Registration limited (3 per hour per IP)
- API rate limiting (60 requests/minute)

### Input Validation
- Parameterized SQL queries (no SQL injection)
- Input sanitization and validation
- XSS prevention with output escaping

### Security Headers
All responses include:
- `X-Content-Type-Options: nosniff`
- `X-Frame-Options: SAMEORIGIN`
- `X-XSS-Protection: 1; mode=block`
- `Strict-Transport-Security` (when HTTPS)

## 📊 Database Integration

The website connects to three PKO databases:

### AccountServer
- `account_login` - User accounts
- `account_details` - VIP status, credits
- `user_log` - Login history

### GameDB
- `account` - Game account credits
- `character` - Player characters
- `guild` - Guild information
- `friends` - Friend relationships

### WebsiteDB
- `LarryEdit` - Shop items
- `LarryLaatikko` - Purchase delivery
- `vote` - Vote sites
- `vote_log` - Vote history

## 🎨 Design System

### Color Palette
- **Primary**: Ocean blue (#0080e6)
- **Secondary**: Treasure gold (#ffc61a)
- **Accent**: Pirate crimson (#e60000)
- **Background**: Deep dark (#0a0e17)

### Typography
- **Display**: Cinzel (headers)
- **Pirate**: Pirata One (logo)
- **Body**: Inter (content)

### Components
- Responsive grid system
- Glassmorphism cards
- Animated buttons
- Progress bars
- Leaderboard tables
- Toast notifications
- Modal dialogs

## 🔧 API Endpoints

### Authentication
```
POST /api/auth/login.php
POST /api/auth/register.php  
POST /api/auth/logout.php
```

### Data
```
GET  /api/status.php
GET  /api/leaderboard.php?type=level|pvp|guilds&limit=10
POST /api/vote.php
POST /api/shop/purchase.php
```

## 📝 Customization

### Adding Pages
1. Create new PHP file in root or `/pages/`
2. Include config: `require_once __DIR__ . '/includes/config.php';`
3. Use helper functions for rendering

### Modifying Styles
Edit `/assets/css/style.css` - CSS variables at the top control the theme:
```css
:root {
    --primary-500: #0080e6;
    --gold-400: #ffc61a;
    /* ... */
}
```

### Adding Shop Items
Insert into `WebsiteDB.LarryEdit`:
```sql
INSERT INTO LarryEdit (TuoNimi, TavaraHinta, TavaraTeksti, Quota, Icon, MyyniTyyppi, TavaraID, MontaTavaraa, category, bought)
VALUES ('Item Name', 100, 'Description', 999, 'icon_name', 'type', 12345, 1, 'category', 0);
```

### Adding Vote Sites
Insert into `WebsiteDB.vote`:
```sql
INSERT INTO vote (name, prize, link, image)
VALUES ('Site Name', 10, 'https://votesite.com/vote?id=123', 'https://example.com/banner.png');
```

## 🚀 Production Deployment

### Checklist
- [ ] Change all secrets in `.env`
- [ ] Set `DEBUG_MODE=false`
- [ ] Configure HTTPS
- [ ] Set up backup for databases
- [ ] Configure logging
- [ ] Enable reCAPTCHA for registration
- [ ] Test all functionality

### Performance Tips
- Enable PHP OPcache
- Use CDN for static assets
- Enable gzip compression
- Set appropriate cache headers

### Monitoring
- Check PHP error logs
- Monitor database connections
- Set up uptime monitoring
- Track API response times

## 📞 Support

For issues or questions:
1. Check troubleshooting in `/download.php`
2. Review server error logs
3. Join the community Discord

---

**Built with ❤️ for the PKO Community**
