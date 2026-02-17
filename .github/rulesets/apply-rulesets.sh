#!/bin/bash
# Script to apply GitHub repository rulesets
# Usage: ./apply-rulesets.sh [OPTIONS]
#
# Options:
#   --list          List all current rulesets
#   --create        Create all rulesets from JSON files
#   --delete        Delete all rulesets (requires confirmation)
#   --verify        Verify rulesets are active
#   --help          Show this help message

set -e

REPO="kushmanmb-org/bitcoin"
RULESETS_DIR=".github/rulesets"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Parse command line arguments first to allow help without authentication
if [ $# -eq 0 ] || [ "$1" == "--help" ]; then
    if [ "$1" == "--help" ]; then
        # Show help without requiring authentication
        cat << EOF
GitHub Repository Rulesets Manager

Usage: $0 [OPTIONS]

Options:
    --list          List all current rulesets
    --create        Create all rulesets from JSON files
    --delete        Delete all rulesets (requires confirmation)
    --verify        Verify expected rulesets are active
    --help          Show this help message

Examples:
    $0 --list                    # List all rulesets
    $0 --create                  # Create rulesets from JSON files
    $0 --verify                  # Check if rulesets are configured correctly

Requirements:
    - GitHub CLI (gh) must be installed and authenticated
    - jq must be installed for JSON parsing
    - Admin access to the repository

For more information, see:
    - .github/rulesets/README.md
    - RULESETS_SETUP_GUIDE.md
EOF
        exit 0
    else
        # Show help if no arguments provided
        show_help
        exit 0
    fi
fi

# Check if gh is installed
if ! command -v gh &> /dev/null; then
    echo -e "${RED}Error: GitHub CLI (gh) is not installed.${NC}"
    echo "Please install it from: https://cli.github.com/"
    exit 1
fi

# Check if jq is installed
if ! command -v jq &> /dev/null; then
    echo -e "${RED}Error: jq is not installed.${NC}"
    echo "Please install jq to parse JSON responses."
    exit 1
fi

# Check authentication
if ! gh auth status &> /dev/null; then
    echo -e "${RED}Error: Not authenticated with GitHub.${NC}"
    echo "Please run: gh auth login"
    exit 1
fi

# Function to list rulesets
list_rulesets() {
    echo -e "${GREEN}Listing rulesets for $REPO:${NC}"
    gh api "repos/$REPO/rulesets" | jq -r '.[] | "\(.id)\t\(.name)\t\(.target)\t\(.enforcement)"' | column -t -s $'\t' -N "ID,Name,Target,Status"
}

# Function to create rulesets
create_rulesets() {
    echo -e "${GREEN}Creating rulesets from JSON files...${NC}"
    
    for file in "$RULESETS_DIR"/*.json; do
        if [ -f "$file" ]; then
            filename=$(basename "$file")
            ruleset_name=$(jq -r '.name' "$file")
            
            echo -e "${YELLOW}Creating ruleset: $ruleset_name${NC}"
            
            if gh api \
                --method POST \
                -H "Accept: application/vnd.github+json" \
                "repos/$REPO/rulesets" \
                --input "$file" > /dev/null 2>&1; then
                echo -e "${GREEN}✓ Successfully created: $ruleset_name${NC}"
            else
                echo -e "${RED}✗ Failed to create: $ruleset_name${NC}"
                echo -e "${YELLOW}  Note: Ruleset may already exist or you may need admin permissions${NC}"
            fi
        fi
    done
    
    echo ""
    echo -e "${GREEN}Done! Run with --list to see all rulesets.${NC}"
}

# Function to verify rulesets
verify_rulesets() {
    echo -e "${GREEN}Verifying rulesets...${NC}"
    
    rulesets=$(gh api "repos/$REPO/rulesets")
    
    # Check for expected rulesets
    expected_rulesets=(
        "master-branch-protection"
        "release-branch-protection"
        "development-branches"
        "release-tags-protection"
    )
    
    for expected in "${expected_rulesets[@]}"; do
        if echo "$rulesets" | jq -e ".[] | select(.name == \"$expected\")" > /dev/null 2>&1; then
            status=$(echo "$rulesets" | jq -r ".[] | select(.name == \"$expected\") | .enforcement")
            if [ "$status" == "active" ]; then
                echo -e "${GREEN}✓ $expected (active)${NC}"
            else
                echo -e "${YELLOW}⚠ $expected (status: $status)${NC}"
            fi
        else
            echo -e "${RED}✗ $expected (not found)${NC}"
        fi
    done
}

# Function to delete rulesets
delete_rulesets() {
    echo -e "${RED}WARNING: This will delete all rulesets for $REPO${NC}"
    read -p "Are you sure? (yes/no): " confirm
    
    if [ "$confirm" != "yes" ]; then
        echo "Aborted."
        exit 0
    fi
    
    echo -e "${YELLOW}Deleting rulesets...${NC}"
    
    ruleset_ids=$(gh api "repos/$REPO/rulesets" | jq -r '.[].id')
    
    for id in $ruleset_ids; do
        ruleset_name=$(gh api "repos/$REPO/rulesets/$id" | jq -r '.name')
        echo -e "${YELLOW}Deleting: $ruleset_name (ID: $id)${NC}"
        
        if gh api \
            --method DELETE \
            -H "Accept: application/vnd.github+json" \
            "repos/$REPO/rulesets/$id" > /dev/null 2>&1; then
            echo -e "${GREEN}✓ Deleted: $ruleset_name${NC}"
        else
            echo -e "${RED}✗ Failed to delete: $ruleset_name${NC}"
        fi
    done
}

# Function to show help
show_help() {
    cat << EOF
GitHub Repository Rulesets Manager

Usage: $0 [OPTIONS]

Options:
    --list          List all current rulesets
    --create        Create all rulesets from JSON files
    --delete        Delete all rulesets (requires confirmation)
    --verify        Verify expected rulesets are active
    --help          Show this help message

Examples:
    $0 --list                    # List all rulesets
    $0 --create                  # Create rulesets from JSON files
    $0 --verify                  # Check if rulesets are configured correctly

Requirements:
    - GitHub CLI (gh) must be installed and authenticated
    - jq must be installed for JSON parsing
    - Admin access to the repository

For more information, see:
    - .github/rulesets/README.md
    - RULESETS_SETUP_GUIDE.md
EOF
}

# Parse command line arguments
case "$1" in
    --list)
        list_rulesets
        ;;
    --create)
        create_rulesets
        ;;
    --delete)
        delete_rulesets
        ;;
    --verify)
        verify_rulesets
        ;;
    *)
        echo -e "${RED}Error: Unknown option: $1${NC}"
        echo ""
        show_help
        exit 1
        ;;
esac
