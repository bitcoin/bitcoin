#!/usr/bin/env bash
# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.
#
# ═══════════════════════════════════════════════════════════════════
# GLOBAL OWNERSHIP & CREATOR STATUS
# ═══════════════════════════════════════════════════════════════════
# Repository Owner: kushmanmb.eth (Ethereum Name Service)
# Creator: Kushman MB
# ENS Identifiers:
#   - Primary: kushmanmb.eth (Ethereum Mainnet)
#   - Base Network: kushmanmb.base.eth
# 
# This script is part of the kushmanmb-org/bitcoin repository
# and is maintained by the repository owner and authorized contributors.
# ═══════════════════════════════════════════════════════════════════
#
# Script: verify-attestation.sh
# Purpose: Verify GitHub attestations for build artifacts
# Usage: ./verify-attestation.sh <artifact-path> [owner]
#
# This script verifies the cryptographic attestation of build artifacts
# to ensure they were built by trusted GitHub Actions workflows.
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
OWNER="${2:-kushmanmb-org}"
ENS_IDENTITY="kushmanmb.base.eth"

# Print usage
usage() {
    cat << EOF
Usage: $0 <artifact-path> [owner]

Verify GitHub attestation for build artifacts.

Arguments:
    artifact-path    Path to the artifact to verify (required)
    owner           Repository owner (default: kushmanmb-org)

Environment Variables:
    GH_TOKEN        GitHub token for API access (required)

Examples:
    # Verify bitcoind binary
    $0 build/src/bitcoind

    # Verify with specific owner
    $0 build/src/bitcoind kushmanmb-org

    # Verify bitcoin-cli
    $0 build/src/bitcoin-cli

ENS Identity: ${ENS_IDENTITY}
Repository: https://github.com/kushmanmb-org/bitcoin

For more information:
    https://docs.github.com/en/actions/security-guides/using-artifact-attestations-to-establish-provenance-for-builds
EOF
}

# Check if gh CLI is installed
if ! command -v gh &> /dev/null; then
    echo -e "${RED}Error: GitHub CLI (gh) is not installed${NC}"
    echo "Install it from: https://cli.github.com/"
    exit 1
fi

# Check if artifact path is provided
if [ $# -lt 1 ]; then
    echo -e "${RED}Error: Artifact path is required${NC}"
    echo ""
    usage
    exit 1
fi

ARTIFACT_PATH="$1"

# Check if artifact exists
if [ ! -f "$ARTIFACT_PATH" ]; then
    echo -e "${RED}Error: Artifact not found: ${ARTIFACT_PATH}${NC}"
    exit 1
fi

# Check if GH_TOKEN is set
if [ -z "$GH_TOKEN" ]; then
    echo -e "${YELLOW}Warning: GH_TOKEN environment variable is not set${NC}"
    echo "Attempting to use existing gh CLI authentication..."
fi

# Print header
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}GitHub Attestation Verification${NC}"
echo -e "${BLUE}========================================${NC}"
echo -e "Repository Owner: ${GREEN}${OWNER}${NC}"
echo -e "ENS Identity:     ${GREEN}${ENS_IDENTITY}${NC}"
echo -e "Artifact:         ${GREEN}${ARTIFACT_PATH}${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Verify attestation
echo "Verifying attestation..."
if gh attestation verify "$ARTIFACT_PATH" --owner "$OWNER" --format json > /tmp/verification-result.json; then
    echo -e "${GREEN}✓ Attestation verified successfully!${NC}"
    echo ""
    
    # Display key information
    echo -e "${BLUE}Verification Details:${NC}"
    echo "--------------------"
    
    # Extract and display certificate information
    if command -v jq &> /dev/null; then
        echo -e "${GREEN}Certificate Information:${NC}"
        jq -r '.[0].verificationResult.signature.certificate | 
            "  Issuer: \(.issuer)\n  SAN: \(.subjectAlternativeName)\n  Source Repository: \(.extensions.sourceRepository // "N/A")\n  Source Repository Owner: \(.extensions.sourceRepositoryOwner // "N/A")"' \
            /tmp/verification-result.json
        
        echo ""
        echo -e "${GREEN}Subject (Artifact):${NC}"
        jq -r '.[0].verificationResult.statement.subject[] | 
            "  Name: \(.name)\n  Digest: \(.digest.sha256 // .digest.sha512)"' \
            /tmp/verification-result.json
        
        echo ""
        echo -e "${GREEN}Predicate Type:${NC}"
        jq -r '.[0].verificationResult.statement.predicateType | 
            "  \(.)"' \
            /tmp/verification-result.json
        
        echo ""
        echo -e "${GREEN}Build Info:${NC}"
        jq -r '.[0].verificationResult.statement.predicate.buildDefinition | 
            "  Build Type: \(.buildType)\n  Repository: \(.externalParameters.workflow.repository // "N/A")\n  Ref: \(.externalParameters.workflow.ref // "N/A")"' \
            /tmp/verification-result.json 2>/dev/null || echo "  Build information not available"
    else
        echo "Full verification result (install jq for formatted output):"
        cat /tmp/verification-result.json
    fi
    
    echo ""
    echo -e "${GREEN}Full verification result saved to: /tmp/verification-result.json${NC}"
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}Verification Complete - SUCCESS${NC}"
    echo -e "${GREEN}========================================${NC}"
    
    exit 0
else
    exit_code=$?
    echo -e "${RED}✗ Attestation verification failed!${NC}"
    echo ""
    echo "Possible reasons:"
    echo "  - Attestation has not been generated for this artifact"
    echo "  - Artifact was not built by a trusted GitHub Actions workflow"
    echo "  - Network connectivity issues"
    echo "  - GitHub token does not have sufficient permissions"
    echo ""
    echo "To generate attestations, ensure your build workflow includes:"
    echo "  permissions:"
    echo "    id-token: write"
    echo "    attestations: write"
    echo ""
    echo "  - uses: actions/attest-build-provenance@v2"
    echo "    with:"
    echo "      subject-path: <path-to-artifact>"
    echo ""
    echo -e "${RED}========================================${NC}"
    echo -e "${RED}Verification Complete - FAILED${NC}"
    echo -e "${RED}========================================${NC}"
    
    exit $exit_code
fi
