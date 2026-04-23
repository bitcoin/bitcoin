#!/bin/bash

set -e

BUILD_DIR="${BUILD_DIR:-build}"
JOBS="${JOBS:-4}"
EXTENDED="${EXTENDED:-false}"
FAILFAST="${FAILFAST:-false}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "========================================="
echo "Bitcoin Core Test Runner"
echo "========================================="
echo "Build Directory: $BUILD_DIR"
echo "Parallel Jobs: $JOBS"
echo "Extended Tests: $EXTENDED"
echo "Fail Fast: $FAILFAST"
echo "========================================="

if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory $BUILD_DIR not found${NC}"
    echo "Please build Bitcoin Core first"
    exit 1
fi

if [ ! -f "$BUILD_DIR/bin/bitcoind" ]; then
    echo -e "${RED}Error: bitcoind binary not found in $BUILD_DIR/bin/${NC}"
    echo "Please build Bitcoin Core first"
    exit 1
fi

if [ ! -f "$BUILD_DIR/test/functional/test_runner.py" ]; then
    echo -e "${RED}Error: test_runner.py not found${NC}"
    exit 1
fi

TEST_CMD="$BUILD_DIR/test/functional/test_runner.py --jobs=$JOBS"

if [ "$EXTENDED" = "true" ]; then
    TEST_CMD="$TEST_CMD --extended"
fi

if [ "$FAILFAST" = "true" ]; then
    TEST_CMD="$TEST_CMD --failfast"
fi

echo -e "${GREEN}Running tests with command:${NC}"
echo "$TEST_CMD"
echo ""

if eval "$TEST_CMD"; then
    echo ""
    echo -e "${GREEN}=========================================${NC}"
    echo -e "${GREEN}All tests passed!${NC}"
    echo -e "${GREEN}=========================================${NC}"
    exit 0
else
    echo ""
    echo -e "${RED}=========================================${NC}"
    echo -e "${RED}Tests failed!${NC}"
    echo -e "${RED}=========================================${NC}"
    exit 1
fi
