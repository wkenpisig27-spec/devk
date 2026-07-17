#!/bin/bash
# ============================================================================
# PKO Server - Linux Deploy Script
# ============================================================================
# Copies compiled server binaries + config/resource to the deploy directory,
# then applies Linux-specific resource fixups (locale, case-sensitive names).
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
mkdir -p "$DEPLOY_DIR/LOG"/{AccountServer,GameServer,GateServer,GroupServer}
mkdir -p "$DEPLOY_DIR/PlayerData"

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

SERVER_DIR="$WORKSPACE_DIR/server"

# Copy server configs (if not already present)
echo ""
echo -e "${YELLOW}Syncing server configs...${NC}"
if [ -d "$SERVER_DIR" ]; then
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
            || { rm -rf "$DEPLOY_DIR/resource"; cp -r "$SERVER_DIR/resource" "$DEPLOY_DIR/"; }
        echo -e "  ${GREEN}✓${NC} resource/ synced"
    fi

    # Copy text data files
    for txt in "$SERVER_DIR"/*.txt; do
        [ -f "$txt" ] || continue
        cp "$txt" "$DEPLOY_DIR/"
    done

    # Addons (always sync)
    if [ -d "$SERVER_DIR/addons" ]; then
        rsync -a --delete "$SERVER_DIR/addons/" "$DEPLOY_DIR/addons/" 2>/dev/null \
            || { rm -rf "$DEPLOY_DIR/addons"; cp -r "$SERVER_DIR/addons" "$DEPLOY_DIR/"; }
        echo -e "  ${GREEN}✓${NC} addons/ synced"
    fi

    # License (overwrite so renewed keys deploy)
    if [ -f "$SERVER_DIR/license.lic" ]; then
        cp -f "$SERVER_DIR/license.lic" "$DEPLOY_DIR/"
        echo -e "  ${GREEN}✓${NC} license.lic"
    fi
fi

# ---------------------------------------------------------------------------
# Locale / ICU resources (often gitignored as *.res / *.loc)
# ---------------------------------------------------------------------------
echo ""
echo -e "${YELLOW}Syncing locale resources...${NC}"
copy_first_existing() {
    local dest="$1"
    shift
    for src in "$@"; do
        if [ -f "$src" ]; then
            cp -f "$src" "$dest"
            echo -e "  ${GREEN}✓${NC} $(basename "$dest") ← $src"
            return 0
        fi
    done
    return 1
}

if ! copy_first_existing "$DEPLOY_DIR/en_US.res" \
    "$SERVER_DIR/en_US.res" \
    "$WORKSPACE_DIR/helper/translation/server/en_US.res"; then
    echo -e "  ${RED}✗${NC} en_US.res not found (generate with genrb from helper/translation/server/en_US.txt)"
fi

if ! copy_first_existing "$DEPLOY_DIR/Locale.loc" \
    "$SERVER_DIR/Locale.loc" \
    "$WORKSPACE_DIR/helper/translation/server/Locale.loc" \
    "$WORKSPACE_DIR/server/Locale.loc.example"; then
    cat > "$DEPLOY_DIR/Locale.loc" << 'EOF'
[locale]
locale=en_US
path=.
log=0
EOF
    echo -e "  ${GREEN}+${NC} Locale.loc (generated default)"
fi

# ---------------------------------------------------------------------------
# Linux case-sensitivity safety net (harmless if code names already match)
# ---------------------------------------------------------------------------
echo ""
echo -e "${YELLOW}Applying Linux resource case fixups...${NC}"
RESOURCE_DIR="$DEPLOY_DIR/resource"
if [ -d "$RESOURCE_DIR" ]; then
    ensure_link() {
        local want="$1"   # desired name in resource/
        local have="$2"   # existing real file
        if [ -e "$RESOURCE_DIR/$want" ]; then
            return 0
        fi
        if [ -f "$RESOURCE_DIR/$have" ]; then
            ln -s "$have" "$RESOURCE_DIR/$want"
            echo -e "  ${GREEN}+${NC} $want → $have"
        fi
    }

    # Historical code names / alternate casings
    ensure_link "skillinfo.txt" "SkillInfo.txt"
    ensure_link "SkillInfo.txt" "skillinfo.txt"
    ensure_link "Hairs.txt" "hairs.txt"
    ensure_link "hairs.txt" "Hairs.txt"
    ensure_link "iteminfo.txt" "ItemInfo.txt"
    ensure_link "characterinfo.txt" "CharacterInfo.txt"

    # Map entity scripts: code tries both casings; symlink remaining mismatches
    while IFS= read -r -d '' ent; do
        dir=$(dirname "$ent")
        base=$(basename "$ent")
        map=$(basename "$dir")
        if [[ "$base" == "${map}Entity.lua" && ! -e "$dir/${map}entity.lua" ]]; then
            ln -s "$base" "$dir/${map}entity.lua"
            echo -e "  ${GREEN}+${NC} ${map}/${map}entity.lua → $base"
        elif [[ "$base" == "${map}entity.lua" && ! -e "$dir/${map}Entity.lua" ]]; then
            ln -s "$base" "$dir/${map}Entity.lua"
            echo -e "  ${GREEN}+${NC} ${map}/${map}Entity.lua → $base"
        fi
    done < <(find "$RESOURCE_DIR" -maxdepth 2 -type f \( -name '*Entity.lua' -o -name '*entity.lua' \) -print0 2>/dev/null)
fi

echo ""
echo -e "${GREEN}============================================${NC}"
echo -e "${GREEN}  Deploy complete!${NC}"
echo -e "${GREEN}============================================${NC}"
echo -e "  Directory: ${DEPLOY_DIR}"
echo ""
echo "Next: update DB settings in *.cfg if needed, then start servers."
