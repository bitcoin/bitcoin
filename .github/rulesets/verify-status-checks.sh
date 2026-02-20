#!/bin/bash
# Script to verify and display actual status check names from recent PRs
# This helps ensure the rulesets have correct status check context names

set -e

REPO="kushmanmb-org/bitcoin"
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== GitHub Status Check Verification ===${NC}"
echo ""

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

echo -e "${GREEN}1. Fetching recent pull requests...${NC}"
echo ""

# Get recent PRs
recent_prs=$(gh pr list --repo "$REPO" --limit 5 --state all --json number,title,state)

if [ -z "$recent_prs" ] || [ "$recent_prs" == "[]" ]; then
    echo -e "${YELLOW}No pull requests found. Creating a test workflow run might be needed.${NC}"
    echo ""
    echo -e "${BLUE}Alternative: Check workflow files for job names:${NC}"
    echo ""
    
    echo -e "${GREEN}From CI workflow:${NC}"
    if [ -f ".github/workflows/ci.yml" ]; then
        grep -E "^  [a-z-]+:" .github/workflows/ci.yml | sed 's/://g' | sed 's/^/  - CI \/ /g'
    fi
    
    echo ""
    echo -e "${GREEN}From other workflows:${NC}"
    for workflow in .github/workflows/*.yml; do
        if [ -f "$workflow" ]; then
            workflow_name=$(grep "^name:" "$workflow" | head -1 | sed 's/name://g' | sed 's/^[[:space:]]*//g' | tr -d '"')
            if [ -n "$workflow_name" ]; then
                echo "  Workflow: $workflow_name"
            fi
        fi
    done
    exit 0
fi

echo -e "${BLUE}Recent PRs:${NC}"
echo "$recent_prs" | jq -r '.[] | "  #\(.number) - \(.title) (\(.state))"'
echo ""

# Get the most recent PR with status checks
echo -e "${GREEN}2. Checking status checks from recent PRs...${NC}"
echo ""

found_checks=false

echo "$recent_prs" | jq -r '.[].number' | while read -r pr_number; do
    echo -e "${BLUE}PR #$pr_number:${NC}"
    
    # Get status checks for this PR
    status_checks=$(gh pr view "$pr_number" --repo "$REPO" --json statusCheckRollup --jq '.statusCheckRollup[]' 2>/dev/null || echo "")
    
    if [ -z "$status_checks" ]; then
        echo -e "  ${YELLOW}No status checks found${NC}"
        echo ""
        continue
    fi
    
    # Display status check contexts
    echo "$status_checks" | jq -r 'select(.context != null) | "  ✓ \(.context)"' 2>/dev/null || echo -e "  ${YELLOW}Unable to parse status checks${NC}"
    echo ""
    
    found_checks=true
done

if [ "$found_checks" == "false" ]; then
    echo -e "${YELLOW}No status checks found in recent PRs.${NC}"
    echo ""
fi

echo -e "${GREEN}3. Current ruleset configuration (master-branch-protection):${NC}"
echo ""

if [ -f ".github/rulesets/master-branch-protection.json" ]; then
    echo -e "${BLUE}Required status checks:${NC}"
    jq -r '.rules[] | select(.type == "required_status_checks") | .parameters.required_status_checks[] | "  - \(.context)"' .github/rulesets/master-branch-protection.json
else
    echo -e "${RED}Error: master-branch-protection.json not found${NC}"
fi

echo ""
echo -e "${GREEN}4. Workflow Analysis:${NC}"
echo ""

# Analyze workflows to predict status check names
echo -e "${BLUE}Expected status check formats:${NC}"
echo ""

# CI workflow
if [ -f ".github/workflows/ci.yml" ]; then
    ci_workflow_name=$(grep "^name:" .github/workflows/ci.yml | head -1 | sed 's/name://g' | sed 's/^[[:space:]]*//g' | tr -d '"')
    echo "From workflow: $ci_workflow_name"
    
    # Get job names
    grep -E "^  [a-z-]+:" .github/workflows/ci.yml | sed 's/://g' | sed 's/^[[:space:]]*//' | while read -r job; do
        # Skip non-job entries
        if [[ "$job" != "push" && "$job" != "pull_request" && "$job" != "on" && "$job" != "env" && "$job" != "run" && "$job" != "group" ]]; then
            echo "  - $ci_workflow_name / $job"
        fi
    done
fi

echo ""

# CodeQL workflow
if [ -f ".github/workflows/codeql.yml" ]; then
    codeql_workflow_name=$(grep "^name:" .github/workflows/codeql.yml | head -1 | sed 's/name://g' | sed 's/^[[:space:]]*//g' | tr -d '"')
    echo "From workflow: $codeql_workflow_name"
    grep -E "^  [a-z-]+:" .github/workflows/codeql.yml | sed 's/://g' | sed 's/^[[:space:]]*//' | while read -r job; do
        if [[ "$job" != "push" && "$job" != "pull_request" && "$job" != "on" && "$job" != "env" && "$job" != "run" && "$job" != "group" ]]; then
            echo "  - $codeql_workflow_name / $job"
        fi
    done
fi

echo ""

# GitGuardian workflow
if [ -f ".github/workflows/gitguardian.yml" ]; then
    gg_workflow_name=$(grep "^name:" .github/workflows/gitguardian.yml | head -1 | sed 's/name://g' | sed 's/^[[:space:]]*//g' | tr -d '"')
    echo "From workflow: $gg_workflow_name"
    grep -E "^  [a-z-]+:" .github/workflows/gitguardian.yml | sed 's/://g' | sed 's/^[[:space:]]*//' | while read -r job; do
        if [[ "$job" != "push" && "$job" != "pull_request" && "$job" != "on" && "$job" != "env" && "$job" != "run" && "$job" != "group" ]]; then
            echo "  - $gg_workflow_name / $job"
        fi
    done
fi

echo ""
echo -e "${YELLOW}Note: Status check names must match exactly (case-sensitive).${NC}"
echo -e "${YELLOW}Matrix jobs will include the matrix value: 'workflow / job (matrix-value)'${NC}"
echo ""
echo -e "${BLUE}=== Verification Complete ===${NC}"
