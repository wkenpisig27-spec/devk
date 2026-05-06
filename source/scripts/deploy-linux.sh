#!/bin/bash
# ============================================================================
# PKO Server - Linux Deploy Script
# ============================================================================
# Copies compiled server binaries + config/resource to the deploy directory.
#
# Usage:
#   ./deploy-linux.sh                    Deploy to default dir
#   PKO_DEPLOY_DIR=/custom ./deploy-linux.sh   Deploy to custom dir
#
# Environment variables:
#   PKO_BUILD_DIR   - Build directory (default: source/out/linux)
#   PKO_DEPLOY_DIR  - Deploy directory (default: /home/$USER/pkodev-server)
# ============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
WORKSPACE_DIR="$(cd "$SOURCE_DIR/.." && pwd)"

BUILD_DIR="${PKO_BUILD_DIR:-$SOURCE_DIR/out/linux}"
DEPLOY_DIR="${PKO_DEPLOY_DIR:-/home/$USER/pkodev-server}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}============================================${NC}"
echo -e "${CYAN}  PKO Server - Linux Deploy${NC}"
echo -e "${CYAN}============================================${NC}"
echo -e "  Build dir:   ${BUILD_DIR}"
echo -e "  Deploy dir:  ${DEPLOY_DIR}"
echo -e "${CYAN}============================================${NC}"

# Verify build output exists
BIN_DIR="$BUILD_DIR/bin"
if [ ! -d "$BIN_DIR" ]; then
    echo -e "${RED}ERROR: Build output not found at $BIN_DIR${NC}"
    echo "Run build-linux.sh first."
    exit 1
fi

# Create deploy directory
mkdir -p "$DEPLOY_DIR"

# Copy server binaries
echo ""
echo -e "${YELLOW}Deploying server binaries...${NC}"
SERVERS=(AccountServer GateServer GroupServer GameServer)
for srv in "${SERVERS[@]}"; do
    if [ -f "$BIN_DIR/$srv" ]; then
        cp -f "$BIN_DIR/$srv" "$DEPLOY_DIR/"
        chmod +x "$DEPLOY_DIR/$srv"
        local_size=$(du -h "$BIN_DIR/$srv" | cut -f1)
        echo -e "  ${GREEN}✓${NC} $srv ($local_size)"
    else
        echo -e "  ${RED}✗${NC} $srv (not found in build)"
    fi
done

# Copy server configs (if not already present)
echo ""
echo -e "${YELLOW}Syncing server configs...${NC}"
SERVER_DIR="$WORKSPACE_DIR/server"
if [ -d "$SERVER_DIR" ]; then
    # Copy .cfg files (don't overwrite existing to preserve local settings)
    for cfg in "$SERVER_DIR"/*.cfg; do
        [ -f "$cfg" ] || continue
        cfgname=$(basename "$cfg")
        if [ ! -f "$DEPLOY_DIR/$cfgname" ]; then
            cp "$cfg" "$DEPLOY_DIR/"
            echo -e "  ${GREEN}+${NC} $cfgname (new)"
        else
            echo -e "  ${YELLOW}~${NC} $cfgname (exists, skipped)"
        fi
    done

    # Copy resource directory (always sync)
    if [ -d "$SERVER_DIR/resource" ]; then
        rsync -a --delete "$SERVER_DIR/resource/" "$DEPLOY_DIR/resource/" 2>/dev/null \
            || cp -r "$SERVER_DIR/resource" "$DEPLOY_DIR/"
        echo -e "  ${GREEN}✓${NC} resource/ synced"
    fi

    # Copy text data files
    for txt in "$SERVER_DIR"/*.txt; do
        [ -f "$txt" ] || continue
        cp "$txt" "$DEPLOY_DIR/"
    done
fi

echo ""
echo -e "${GREEN}============================================${NC}"
echo -e "${GREEN}  Deploy complete!${NC}"
echo -e "${GREEN}============================================${NC}"
echo -e "  Directory: ${DEPLOY_DIR}"
echo ""
echo "Next: Run start-servers.sh to launch the servers."
