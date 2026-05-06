#!/bin/bash
# PKO Website Server Recovery Script
# Run on Ubuntu server: sudo bash server-recovery.sh
#
# This script:
# 1. Clears attack artifacts
# 2. Restarts services
# 3. Applies security hardening

set -e

echo "============================================"
echo "PKO Website Server Recovery"
echo "============================================"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root: sudo bash server-recovery.sh"
    exit 1
fi

# Detect PHP version
PHP_VERSION=$(php -v 2>/dev/null | head -n1 | cut -d' ' -f2 | cut -d'.' -f1,2)
if [ -z "$PHP_VERSION" ]; then
    PHP_VERSION="8.1"
fi
echo "[INFO] Detected PHP version: $PHP_VERSION"

# 1. Clear rate limiting cache
echo ""
echo "[STEP 1] Clearing rate limit cache..."
rm -rf /tmp/pko_rate_limit 2>/dev/null || true
echo "  Done."

# 2. Clear PHP sessions older than 24 hours
echo ""
echo "[STEP 2] Clearing stale PHP sessions..."
find /var/lib/php/sessions -name 'sess_*' -mtime +1 -delete 2>/dev/null || true
find /tmp -name 'sess_*' -mtime +1 -delete 2>/dev/null || true
echo "  Done."

# 3. Clear opcache
echo ""
echo "[STEP 3] Clearing PHP opcache..."
if command -v php &> /dev/null; then
    php -r "opcache_reset();" 2>/dev/null || true
fi
echo "  Done."

# 4. Check and fix permissions
echo ""
echo "[STEP 4] Fixing file permissions..."
WEBROOT="/var/www/pko/website"
if [ -d "$WEBROOT" ]; then
    chown -R www-data:www-data "$WEBROOT"
    find "$WEBROOT" -type d -exec chmod 755 {} \;
    find "$WEBROOT" -type f -exec chmod 644 {} \;
    chmod 600 "$WEBROOT/.env" 2>/dev/null || true
    echo "  Fixed permissions for $WEBROOT"
else
    echo "  Warning: $WEBROOT not found. Update WEBROOT variable in this script."
fi

# 5. Restart PHP-FPM
echo ""
echo "[STEP 5] Restarting PHP-FPM..."
systemctl restart php${PHP_VERSION}-fpm 2>/dev/null || systemctl restart php-fpm 2>/dev/null || true
echo "  Done."

# 6. Restart web server
echo ""
echo "[STEP 6] Restarting web server..."
if systemctl is-active --quiet nginx; then
    nginx -t && systemctl restart nginx
    echo "  Nginx restarted."
elif systemctl is-active --quiet apache2; then
    apache2ctl configtest && systemctl restart apache2
    echo "  Apache restarted."
else
    echo "  Warning: No web server detected running."
fi

# 7. Check for attacking IPs in logs
echo ""
echo "[STEP 7] Checking for attacking IPs (last 1000 requests)..."
LOGFILE=""
if [ -f /var/log/nginx/access.log ]; then
    LOGFILE="/var/log/nginx/access.log"
elif [ -f /var/log/apache2/access.log ]; then
    LOGFILE="/var/log/apache2/access.log"
fi

if [ -n "$LOGFILE" ]; then
    echo "  Top 10 IPs by request count:"
    tail -1000 "$LOGFILE" | awk '{print $1}' | sort | uniq -c | sort -rn | head -10
    echo ""
    echo "  IPs with 403/404 errors:"
    tail -1000 "$LOGFILE" | awk '$9 ~ /^(403|404)/ {print $1}' | sort | uniq -c | sort -rn | head -10
fi

# 8. Check PHP error log
echo ""
echo "[STEP 8] Recent PHP errors:"
if [ -f /var/log/php${PHP_VERSION}-fpm.log ]; then
    tail -20 /var/log/php${PHP_VERSION}-fpm.log
elif [ -f /var/log/php-fpm/error.log ]; then
    tail -20 /var/log/php-fpm/error.log
fi

# 9. Test website
echo ""
echo "[STEP 9] Testing website..."
HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" http://localhost/ 2>/dev/null || echo "000")
if [ "$HTTP_CODE" = "200" ]; then
    echo "  Website is responding (HTTP $HTTP_CODE)"
else
    echo "  Warning: Website returned HTTP $HTTP_CODE"
fi

# 10. Show service status
echo ""
echo "[STEP 10] Service status:"
systemctl status php${PHP_VERSION}-fpm --no-pager -l 2>/dev/null | head -5 || true
systemctl status nginx --no-pager -l 2>/dev/null | head -5 || systemctl status apache2 --no-pager -l 2>/dev/null | head -5 || true

echo ""
echo "============================================"
echo "Recovery Complete!"
echo "============================================"
echo ""
echo "NEXT STEPS:"
echo "1. Deploy updated website files"
echo "2. Run recovery.php: curl 'http://localhost/recovery.php?key=YOUR_KEY'"
echo "3. Monitor logs: tail -f /var/log/nginx/error.log"
echo "4. Consider enabling Cloudflare or fail2ban"
echo ""
echo "To block an attacking IP:"
echo "  iptables -A INPUT -s ATTACKER_IP -j DROP"
echo "  # Or add to website/includes/blocklist.php"
echo ""
