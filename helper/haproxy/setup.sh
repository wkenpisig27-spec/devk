#!/bin/bash
#=============================================================================
# PKO HAProxy Quick Setup Script
# Run this on a fresh Ubuntu VPS to set up the proxy server
#=============================================================================
# Usage: 
#   1. Upload this folder to your VPS
#   2. chmod +x setup.sh
#   3. sudo ./setup.sh
#=============================================================================

set -e

echo "=============================================="
echo " PKO HAProxy Proxy Server Setup"
echo "=============================================="

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "Please run as root: sudo ./setup.sh"
    exit 1
fi

# Update system
echo "[1/6] Updating system packages..."
apt-get update
apt-get upgrade -y

# Install HAProxy
echo "[2/6] Installing HAProxy..."
apt-get install -y haproxy socat

# Copy configuration files
echo "[3/6] Installing configuration files..."
cp haproxy.cfg /etc/haproxy/haproxy.cfg
cp blocklist.acl /etc/haproxy/blocklist.acl

# Set permissions
chown root:root /etc/haproxy/haproxy.cfg
chown root:root /etc/haproxy/blocklist.acl
chmod 644 /etc/haproxy/haproxy.cfg
chmod 644 /etc/haproxy/blocklist.acl

# Enable kernel SYN flood protection
echo "[4/6] Enabling kernel SYN protection..."
cat >> /etc/sysctl.conf << 'EOF'

# PKO Proxy - SYN Flood Protection
net.ipv4.tcp_syncookies = 1
net.ipv4.tcp_max_syn_backlog = 65535
net.core.somaxconn = 65535
net.ipv4.tcp_fin_timeout = 30
net.ipv4.tcp_keepalive_time = 300
net.ipv4.tcp_tw_reuse = 1
EOF

sysctl -p

# Validate HAProxy config
echo "[5/6] Validating HAProxy configuration..."
haproxy -c -f /etc/haproxy/haproxy.cfg

# Enable and start HAProxy
echo "[6/6] Starting HAProxy..."
systemctl enable haproxy
systemctl restart haproxy

# Show status
echo ""
echo "=============================================="
echo " Setup Complete!"
echo "=============================================="
echo ""
echo " Proxy Status:"
systemctl status haproxy --no-pager | head -5
echo ""
echo " Stats Dashboard: http://$(hostname -I | awk '{print $1}'):8404/stats"
echo " Username: admin"
echo " Password: PKOProxy2026!"
echo ""
echo " Next steps:"
echo " 1. Update DNS A record to point to this server's IP"
echo " 2. Make sure your game server firewall allows this proxy IP"
echo ""
