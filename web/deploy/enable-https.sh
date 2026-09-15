#!/usr/bin/env bash
# Enable HTTPS once a domain's A record points at this VPS.
# Usage: sudo bash enable-https.sh your.domain.com
set -euo pipefail

DOMAIN="${1:-}"
if [ -z "$DOMAIN" ]; then
  echo "Usage: $0 your.domain.com"
  exit 1
fi

export DEBIAN_FRONTEND=noninteractive
apt-get update -y
apt-get install -y certbot python3-certbot-nginx

# Point nginx server_name at the domain before certbot
sed -i "s/server_name _;/server_name ${DOMAIN} www.${DOMAIN};/" /etc/nginx/sites-available/abyss-sea
nginx -t
systemctl reload nginx

certbot --nginx -d "$DOMAIN" -d "www.$DOMAIN" --non-interactive --agree-tos --register-unsafely-without-email --redirect || \
certbot --nginx -d "$DOMAIN" --non-interactive --agree-tos --register-unsafely-without-email --redirect

# Tell Next.js the public URL
mkdir -p /etc/abyss-sea
cat >/etc/abyss-sea/web.env <<EOF
NEXT_PUBLIC_SITE_URL=https://${DOMAIN}
DATA_DIR=/var/lib/abyss-sea
EOF

# Restart app with new env
if ! grep -q EnvironmentFile /etc/systemd/system/abyss-sea-web.service; then
  sed -i '/\[Service\]/a EnvironmentFile=/etc/abyss-sea/web.env' /etc/systemd/system/abyss-sea-web.service
fi
systemctl daemon-reload
systemctl restart abyss-sea-web nginx

echo "HTTPS enabled for https://${DOMAIN}"
