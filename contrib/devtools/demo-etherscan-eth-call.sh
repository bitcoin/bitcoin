#!/bin/bash

# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

# Demo script for fetch-etherscan-eth-call.js
# 
# This script demonstrates how to use the Etherscan eth_call client
# with the example parameters from the problem statement.

set -e

# Color codes for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}═══════════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}Etherscan eth_call Client - Demo${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════════${NC}"
echo ""

# Check if API key is set
if [ -z "${ETHERSCAN_API_KEY}" ]; then
    echo -e "${RED}Error: ETHERSCAN_API_KEY environment variable is not set${NC}"
    echo ""
    echo "Please set your Etherscan API key:"
    echo "  export ETHERSCAN_API_KEY=YourApiKeyToken"
    echo ""
    echo "Get your API key at: https://etherscan.io/myapikey"
    exit 1
fi

echo -e "${GREEN}✓${NC} API key configured"
echo ""

# Example from problem statement
echo -e "${BLUE}Example 1: Call from problem statement${NC}"
echo "This replicates the curl command from the problem statement:"
echo ""
echo "  curl \"https://api.etherscan.io/v2/api?chainid=1&module=proxy&action=eth_call"
echo "        &to=0xAEEF46DB4855E25702F8237E8f403FddcaF931C0"
echo "        &data=0x70a08231000000000000000000000000e16359506c028e51f16be38986ec5746251e9724"
echo "        &tag=latest&apikey=YourApiKeyToken\""
echo ""
echo -e "${YELLOW}Running equivalent command...${NC}"
echo ""

node "$(dirname "$0")/fetch-etherscan-eth-call.js" \
    --to 0xAEEF46DB4855E25702F8237E8f403FddcaF931C0 \
    --data 0x70a08231000000000000000000000000e16359506c028e51f16be38986ec5746251e9724 \
    --tag latest

echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════════════${NC}"
echo ""

# Additional examples
echo -e "${BLUE}Example 2: Query USDC decimals${NC}"
echo "Querying decimals() on USDC contract:"
echo ""

node "$(dirname "$0")/fetch-etherscan-eth-call.js" \
    --to 0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48 \
    --data 0x313ce567

echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════════════${NC}"
echo ""

echo -e "${BLUE}Example 3: Using environment variables${NC}"
echo "Setting environment variables and calling the script:"
echo ""

export TO_ADDRESS=0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48
export CALL_DATA=0x18160ddd  # totalSupply()

echo "  export TO_ADDRESS=0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48"
echo "  export CALL_DATA=0x18160ddd"
echo ""

node "$(dirname "$0")/fetch-etherscan-eth-call.js"

echo ""
echo -e "${GREEN}✓ Demo completed successfully${NC}"
echo ""
