#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Build script for Jettison State RX with enforced quality checks

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# Parse arguments
CLEAN=false
NO_CHECKS=false
JOBS=$(nproc)

print_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --clean         Clean build directory before building"
    echo "  --no-checks     Skip format and lint checks"
    echo "  -j, --jobs N    Number of parallel build jobs (default: $(nproc))"
    echo "  -h, --help      Show this help message"
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN=true
            shift
            ;;
        --no-checks)
            NO_CHECKS=true
            shift
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        -h|--help)
            print_usage
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            print_usage
            exit 1
            ;;
    esac
done

# Print header
echo -e "${BLUE}============================================${NC}"
echo -e "${BLUE}Jettison State RX - Build Script${NC}"
echo -e "${BLUE}============================================${NC}"
echo ""

# Check for required tools
echo -e "${BLUE}[1/6] Checking required tools...${NC}"
for tool in cmake clang clang-format clang-tidy git; do
    if ! command -v $tool &> /dev/null; then
        echo -e "${RED}ERROR: $tool is not installed${NC}"
        exit 1
    fi
    echo -e "  ${GREEN}✓${NC} $tool found"
done
echo ""

# Check git submodules
echo -e "${BLUE}[2/6] Checking git submodules...${NC}"
if [ ! -f "${SCRIPT_DIR}/jettison_proto_cpp/jon_shared_data.pb.h" ]; then
    echo -e "${YELLOW}  Submodule not initialized, initializing now...${NC}"
    git submodule update --init --recursive
fi
echo -e "  ${GREEN}✓${NC} Submodules ready"
echo ""

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo -e "${BLUE}[3/6] Cleaning build directory...${NC}"
    rm -rf "${BUILD_DIR}"
    echo -e "  ${GREEN}✓${NC} Build directory cleaned"
    echo ""
else
    echo -e "${BLUE}[3/6] Skipping clean (use --clean to clean)${NC}"
    echo ""
fi

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Configure CMake
echo -e "${BLUE}[4/6] Configuring CMake...${NC}"
if [ "$NO_CHECKS" = true ]; then
    cmake -DCMAKE_BUILD_TYPE=Release -DENFORCE_CHECKS=OFF ..
    echo -e "  ${YELLOW}⚠${NC}  Code quality checks DISABLED"
else
    cmake -DCMAKE_BUILD_TYPE=Release -DENFORCE_CHECKS=ON ..
    echo -e "  ${GREEN}✓${NC} Code quality checks ENABLED"
fi
echo ""

# Run format and lint (if checks are enabled)
if [ "$NO_CHECKS" = false ]; then
    echo -e "${BLUE}[5/6] Running code quality checks...${NC}"

    echo -e "  ${BLUE}→${NC} Running clang-format..."
    if ! cmake --build . --target format; then
        echo -e "${RED}ERROR: clang-format failed${NC}"
        exit 1
    fi
    echo -e "  ${GREEN}✓${NC} Code formatted"

    echo -e "  ${BLUE}→${NC} Running clang-tidy (strict mode)..."
    if ! cmake --build . --target lint-strict 2>&1 | tee lint-output.log; then
        echo -e "${RED}ERROR: clang-tidy found issues${NC}"
        echo -e "${YELLOW}See lint-output.log for details${NC}"
        exit 1
    fi
    echo -e "  ${GREEN}✓${NC} Lint checks passed"
    echo ""
else
    echo -e "${BLUE}[5/6] Skipping code quality checks${NC}"
    echo ""
fi

# Build
echo -e "${BLUE}[6/6] Building project...${NC}"
if ! cmake --build . --parallel ${JOBS}; then
    echo -e "${RED}ERROR: Build failed${NC}"
    exit 1
fi
echo ""

# Success
echo -e "${GREEN}============================================${NC}"
echo -e "${GREEN}✓ Build completed successfully!${NC}"
echo -e "${GREEN}============================================${NC}"
echo ""
echo -e "Binary: ${BUILD_DIR}/jettison_state_rx"
echo ""
echo "To run:"
echo "  ${BUILD_DIR}/jettison_state_rx sych.local"
echo ""
