#!/bin/bash

# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

# Demo script for withdrawal credentials functionality
# 
# This script demonstrates how to use the withdrawal credentials tool
# to query and decode Ethereum validator withdrawal credentials.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOOL="$SCRIPT_DIR/fetch-withdrawal-credentials.js"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Ethereum Withdrawal Credentials Demo ===${NC}\n"

# Demo 1: Decode 0x01 withdrawal credentials
echo -e "${GREEN}Demo 1: Decode 0x01 (Execution Address) Withdrawal Credentials${NC}"
echo "This example shows withdrawal credentials that can receive withdrawals."
echo ""
echo -e "${YELLOW}Command:${NC}"
echo "node $TOOL --decode 0x010000000000000000000000e16359506c028e51f16be38986ec5746251e9724"
echo ""
node "$TOOL" --decode 0x010000000000000000000000e16359506c028e51f16be38986ec5746251e9724

echo ""
echo "---"
echo ""

# Demo 2: Decode 0x00 BLS withdrawal credentials
echo -e "${GREEN}Demo 2: Decode 0x00 (BLS) Withdrawal Credentials${NC}"
echo "This example shows legacy BLS credentials that require an upgrade to 0x01."
echo ""
echo -e "${YELLOW}Command:${NC}"
echo "node $TOOL --decode 0x004156d2e53f1b3e0b4e8b6c9e7d3f2a1b0c9d8e7f6a5b4c3d2e1f0a9b8c7d00"
echo ""
node "$TOOL" --decode 0x004156d2e53f1b3e0b4e8b6c9e7d3f2a1b0c9d8e7f6a5b4c3d2e1f0a9b8c7d00

echo ""
echo "---"
echo ""

# Demo 3: Show help
echo -e "${GREEN}Demo 3: Help Information${NC}"
echo "View all available options:"
echo ""
echo -e "${YELLOW}Command:${NC}"
echo "node $TOOL --help"
echo ""
node "$TOOL" --help

echo ""
echo -e "${BLUE}=== Demo Complete ===${NC}"
echo ""
echo "Notes:"
echo "  - Type 0x01 credentials can receive validator withdrawals directly"
echo "  - Type 0x00 credentials require a BLSToExecutionChange to enable withdrawals"
echo "  - Use --index or --pubkey to query live validator data from Beacon Chain API"
echo "  - Use --decode to analyze withdrawal credentials offline"
echo ""
