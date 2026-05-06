#!/bin/bash
# ============================================================================
# PKO Server - Linux Server Launcher (tmux)
# ============================================================================
# Starts all 4 servers in a tmux session with separate windows.
#
# Usage:
#   ./start-servers.sh                          Start with defaults
#   PKO_DEPLOY_DIR=/custom ./start-servers.sh   Custom deploy dir
#
# tmux session: "pko"
#   Window 0: account  (AccountServer)
#   Window 1: group    (GroupServer)
#   Window 2: gate     (GateServer)
#   Window 3: game     (GameServer)
#
# To attach: tmux attach -t pko
# To stop:   ./stop-servers.sh
# ============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
DEPLOY_DIR="${PKO_DEPLOY_DIR:-/home/$USER/pkodev-server}"
SESSION="pko"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}============================================${NC}"
echo -e "${CYAN}  PKO Server - Starting Servers${NC}"
echo -e "${CYAN}============================================${NC}"
echo -e "  Deploy dir: ${DEPLOY_DIR}"
echo -e "  tmux:       ${SESSION}"
echo -e "${CYAN}============================================${NC}"

# Verify deploy directory
if [ ! -f "$DEPLOY_DIR/AccountServer" ]; then
    echo -e "${RED}ERROR: AccountServer not found in $DEPLOY_DIR${NC}"
    echo "Run deploy-linux.sh first."
    exit 1
fi

# Kill existing session
tmux kill-session -t "$SESSION" 2>/dev/null || true
sleep 1

echo ""
echo -e "${YELLOW}[1/4] Starting AccountServer...${NC}"
tmux new-session -d -s "$SESSION" -n account
tmux send-keys -t "$SESSION:account" "cd $DEPLOY_DIR && ./AccountServer" Enter
sleep 4

echo -e "${YELLOW}[2/4] Starting GroupServer...${NC}"
tmux new-window -t "$SESSION" -n group
tmux send-keys -t "$SESSION:group" "cd $DEPLOY_DIR && ./GroupServer" Enter
sleep 4

echo -e "${YELLOW}[3/4] Starting GateServer...${NC}"
tmux new-window -t "$SESSION" -n gate
tmux send-keys -t "$SESSION:gate" "cd $DEPLOY_DIR && ./GateServer" Enter
sleep 4

echo -e "${YELLOW}[4/4] Starting GameServer...${NC}"
tmux new-window -t "$SESSION" -n game
tmux send-keys -t "$SESSION:game" "cd $DEPLOY_DIR && ./GameServer" Enter
sleep 8

echo ""
echo -e "${GREEN}============================================${NC}"
echo -e "${GREEN}  Server Status${NC}"
echo -e "${GREEN}============================================${NC}"

# Check each server
for srv in AccountServer GroupServer GateServer GameServer; do
    if pgrep -f "$DEPLOY_DIR/$srv" > /dev/null 2>&1; then
        PID=$(pgrep -f "$DEPLOY_DIR/$srv" | head -1)
        echo -e "  ${GREEN}✓${NC} $srv (PID: $PID)"
    else
        echo -e "  ${RED}✗${NC} $srv (not running)"
    fi
done

echo ""
echo "WSL IP: $(hostname -I | awk '{print $1}')"
echo ""
echo "tmux attach -t $SESSION    # View server consoles"
echo "stop-servers.sh            # Stop all servers"
