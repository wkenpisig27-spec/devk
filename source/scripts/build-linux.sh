#!/bin/bash
# ============================================================================
# PKO Server - Linux Build Script
# ============================================================================
# Usage:
#   ./build-linux.sh              # Full build (configure + compile)
#   ./build-linux.sh --build-only # Skip cmake configure, just make
#   ./build-linux.sh --clean      # Clean build directory
#   ./build-linux.sh --rebuild    # Clean + full build
#
# Environment variables:
#   PKO_BUILD_DIR   - Build output directory (default: source/out/linux)
#   PKO_BUILD_TYPE  - Release or Debug (default: Release)
#   PKO_JOBS        - Parallel jobs (default: $(nproc))
# ============================================================================

set -e
trap '' INT  # Prevent accidental Ctrl+C from killing build

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Configurable settings
BUILD_DIR="${PKO_BUILD_DIR:-$SOURCE_DIR/out/linux}"
BUILD_TYPE="${PKO_BUILD_TYPE:-Release}"
JOBS="${PKO_JOBS:-$(nproc)}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

print_header() {
    echo -e "${CYAN}============================================${NC}"
    echo -e "${CYAN}  PKO Server - Linux Build${NC}"
    echo -e "${CYAN}============================================${NC}"
    echo -e "  Source:     ${SOURCE_DIR}"
    echo -e "  Build dir:  ${BUILD_DIR}"
    echo -e "  Build type: ${BUILD_TYPE}"
    echo -e "  Jobs:       ${JOBS}"
    echo -e "${CYAN}============================================${NC}"
}

do_clean() {
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
    echo -e "${GREEN}Clean complete.${NC}"
}

do_configure() {
    echo -e "${YELLOW}Configuring CMake...${NC}"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake "$SOURCE_DIR" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        2>&1 | tee "$BUILD_DIR/cmake.log"
    echo -e "${GREEN}Configure complete.${NC}"
}

do_build() {
    echo -e "${YELLOW}Building with ${JOBS} parallel jobs...${NC}"
    cd "$BUILD_DIR"

    local LOG_FILE="$BUILD_DIR/build.log"
    make -j"$JOBS" > "$LOG_FILE" 2>&1
    local EXIT_CODE=$?

    local ERROR_COUNT=$(grep -c 'error:' "$LOG_FILE" 2>/dev/null || echo 0)

    if [ $EXIT_CODE -eq 0 ] && [ "$ERROR_COUNT" -eq 0 ]; then
        echo -e "${GREEN}============================================${NC}"
        echo -e "${GREEN}  BUILD SUCCESSFUL${NC}"
        echo -e "${GREEN}============================================${NC}"

        echo -e "\nBinaries:"
        for bin in AccountServer GateServer GroupServer GameServer; do
            if [ -f "$BUILD_DIR/bin/$bin" ]; then
                local SIZE=$(du -h "$BUILD_DIR/bin/$bin" | cut -f1)
                echo -e "  ${GREEN}✓${NC} $bin ($SIZE)"
            else
                echo -e "  ${RED}✗${NC} $bin (not found)"
            fi
        done
    else
        echo -e "${RED}============================================${NC}"
        echo -e "${RED}  BUILD FAILED (exit=$EXIT_CODE, errors=$ERROR_COUNT)${NC}"
        echo -e "${RED}============================================${NC}"
        echo ""
        grep 'error:' "$LOG_FILE" | grep -v 'Interrupt' | head -20
        echo ""
        echo "Full log: $LOG_FILE"
        exit 1
    fi
}

# Parse arguments
case "${1:-}" in
    --clean)
        print_header
        do_clean
        ;;
    --build-only)
        print_header
        do_build
        ;;
    --rebuild)
        print_header
        do_clean
        do_configure
        do_build
        ;;
    --help|-h)
        echo "Usage: $0 [--build-only|--clean|--rebuild|--help]"
        echo ""
        echo "Options:"
        echo "  (none)        Configure + build"
        echo "  --build-only  Skip cmake configure, just make"
        echo "  --clean       Remove build directory"
        echo "  --rebuild     Clean + configure + build"
        echo ""
        echo "Environment:"
        echo "  PKO_BUILD_DIR=$BUILD_DIR"
        echo "  PKO_BUILD_TYPE=$BUILD_TYPE"
        echo "  PKO_JOBS=$JOBS"
        ;;
    *)
        print_header
        do_configure
        do_build
        ;;
esac
