#!/bin/bash
# ============================================================================
# PKO Server - Linux Server Stopper
# ============================================================================
# Gracefully stops all running PKO servers.
#
# Usage:
#   ./stop-servers.sh
# ============================================================================

DEPLOY_DIR="${PKO_DEPLOY_DIR:-/home/$USER/pkodev-server}"
SESSION="pko"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}Stopping PKO servers...${NC}"

# Kill server processes
for srv in GameServer GateServer GroupServer AccountServer; do
    PID=$(pgrep -f "$DEPLOY_DIR/$srv" 2>/dev/null | head -1)
    if [ -n "$PID" ]; then
        kill "$PID" 2>/dev/null
        echo -e "  ${GREEN}✓${NC} $srv (PID $PID) stopped"
    else
        echo -e "  ${YELLOW}~${NC} $srv (not running)"
    fi
done

# Kill tmux session
tmux kill-session -t "$SESSION" 2>/dev/null && \
    echo -e "  ${GREEN}✓${NC} tmux session '$SESSION' killed" || \
    echo -e "  ${YELLOW}~${NC} tmux session '$SESSION' not found"

echo ""
echo -e "${GREEN}All servers stopped.${NC}"
