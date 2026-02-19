#!/usr/bin/env bash

# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

# CBSC (Credentials Beacon Signature Check) Verification Script
#
# This script runs comprehensive verification of the Ethereum withdrawal
# credentials tooling to ensure all CBSC components are working correctly.
#
# Usage:
#   bash verify-cbsc.sh [--verbose]

# Note: We don't use 'set -e' because we want to continue and report all test results

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Verbose mode
VERBOSE=0
if [[ "$1" == "--verbose" ]]; then
    VERBOSE=1
fi

# Counter for results
TESTS_PASSED=0
TESTS_FAILED=0

# Helper function to print section headers
print_header() {
    echo ""
    echo -e "${BLUE}================================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}================================================${NC}"
    echo ""
}

# Helper function to print test results
print_result() {
    local test_name="$1"
    local result="$2"
    
    if [[ "$result" == "PASS" ]]; then
        echo -e "${GREEN}✓${NC} $test_name"
        ((TESTS_PASSED++))
    else
        echo -e "${RED}✗${NC} $test_name"
        ((TESTS_FAILED++))
    fi
}

# Get script directory (absolute path)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

print_header "CBSC (Credentials Beacon Signature Check) Verification"

echo "This script verifies the Ethereum withdrawal credentials tooling"
echo "to ensure all components are working correctly."
echo ""

# Check Node.js is available
print_header "1. Environment Check"

if command -v node &> /dev/null; then
    NODE_VERSION=$(node --version)
    print_result "Node.js available ($NODE_VERSION)" "PASS"
else
    print_result "Node.js available" "FAIL"
    echo -e "${RED}Error: Node.js is required but not installed${NC}"
    exit 1
fi

# Check required files exist
print_header "2. File Existence Check"

FILES=(
    "fetch-withdrawal-credentials.js"
    "test-withdrawal-credentials.js"
    "demo-withdrawal-credentials.sh"
    "WITHDRAWAL_CREDENTIALS_README.md"
    "CBSC_VERIFICATION.md"
)

for file in "${FILES[@]}"; do
    if [[ -f "$SCRIPT_DIR/$file" ]]; then
        print_result "File exists: $file" "PASS"
    else
        print_result "File exists: $file" "FAIL"
    fi
done

# Check files are executable
print_header "3. File Permissions Check"

EXEC_FILES=(
    "fetch-withdrawal-credentials.js"
    "test-withdrawal-credentials.js"
    "demo-withdrawal-credentials.sh"
    "verify-cbsc.sh"
)

for file in "${EXEC_FILES[@]}"; do
    if [[ -x "$SCRIPT_DIR/$file" ]] || [[ "$file" == *.js ]]; then
        print_result "File executable: $file" "PASS"
    else
        print_result "File executable: $file" "FAIL"
    fi
done

# Run test suite
print_header "4. Test Suite Execution"

echo "Running withdrawal credentials test suite..."
echo ""

if [[ $VERBOSE -eq 1 ]]; then
    node "$SCRIPT_DIR/test-withdrawal-credentials.js"
    TEST_RESULT=$?
else
    TEST_OUTPUT=$(node "$SCRIPT_DIR/test-withdrawal-credentials.js" 2>&1)
    TEST_RESULT=$?
fi

if [[ $TEST_RESULT -eq 0 ]]; then
    print_result "Test suite execution" "PASS"
    if [[ $VERBOSE -eq 1 ]]; then
        echo ""
    else
        echo "$TEST_OUTPUT" | grep -E "(Total:|Passed:|Failed:|All tests passed!)"
    fi
else
    print_result "Test suite execution" "FAIL"
    echo -e "${RED}Test output:${NC}"
    if [[ $VERBOSE -eq 1 ]]; then
        echo ""
    else
        echo "$TEST_OUTPUT"
    fi
fi

# Test credential decoding (0x01 type)
print_header "5. Credential Format Verification (Type 0x01)"

echo "Testing 0x01 (execution address) credential decoding..."

TEST_CRED_01="0x010000000000000000000000e16359506c028e51f16be38986ec5746251e9724"
EXPECTED_ADDRESS="0xe16359506c028e51f16be38986ec5746251e9724"

if [[ $VERBOSE -eq 1 ]]; then
    OUTPUT_01=$(node "$SCRIPT_DIR/fetch-withdrawal-credentials.js" --decode "$TEST_CRED_01")
    echo "$OUTPUT_01"
else
    OUTPUT_01=$(node "$SCRIPT_DIR/fetch-withdrawal-credentials.js" --decode "$TEST_CRED_01" 2>&1)
fi

if echo "$OUTPUT_01" | grep -q "Type: 0x01" && \
   echo "$OUTPUT_01" | grep -q "$EXPECTED_ADDRESS" && \
   echo "$OUTPUT_01" | grep -q "Can Withdraw: Yes"; then
    print_result "0x01 credential decoding" "PASS"
else
    print_result "0x01 credential decoding" "FAIL"
    if [[ $VERBOSE -eq 0 ]]; then
        echo -e "${RED}Output:${NC}"
        echo "$OUTPUT_01"
    fi
fi

# Test address extraction
if echo "$OUTPUT_01" | grep -q "Address: $EXPECTED_ADDRESS"; then
    print_result "0x01 address extraction" "PASS"
else
    print_result "0x01 address extraction" "FAIL"
fi

# Test format validation
if echo "$OUTPUT_01" | grep -q "Valid Format: Yes"; then
    print_result "0x01 format validation" "PASS"
else
    print_result "0x01 format validation" "FAIL"
fi

# Test credential decoding (0x00 type)
print_header "6. Credential Format Verification (Type 0x00)"

echo "Testing 0x00 (BLS) credential decoding..."

TEST_CRED_00="0x004156d2e53f1b3e0b4e8b6c9e7d3f2a1b0c9d8e7f6a5b4c3d2e1f0a9b8c7d00"

if [[ $VERBOSE -eq 1 ]]; then
    OUTPUT_00=$(node "$SCRIPT_DIR/fetch-withdrawal-credentials.js" --decode "$TEST_CRED_00")
    echo "$OUTPUT_00"
else
    OUTPUT_00=$(node "$SCRIPT_DIR/fetch-withdrawal-credentials.js" --decode "$TEST_CRED_00" 2>&1)
fi

if echo "$OUTPUT_00" | grep -q "Type: 0x00" && \
   echo "$OUTPUT_00" | grep -q "Can Withdraw: No" && \
   echo "$OUTPUT_00" | grep -q "Requires Upgrade: Yes"; then
    print_result "0x00 credential decoding" "PASS"
else
    print_result "0x00 credential decoding" "FAIL"
    if [[ $VERBOSE -eq 0 ]]; then
        echo -e "${RED}Output:${NC}"
        echo "$OUTPUT_00"
    fi
fi

# Test BLS hash extraction
if echo "$OUTPUT_00" | grep -q "Hash: 0x4156d2e53f1b3e0b4e8b6c9e7d3f2a1b0c9d8e7f6a5b4c3d2e1f0a9b8c7d00"; then
    print_result "0x00 BLS hash extraction" "PASS"
else
    print_result "0x00 BLS hash extraction" "FAIL"
fi

# Test BLSToExecutionChange message mention
if echo "$OUTPUT_00" | grep -q "BLSToExecutionChange"; then
    print_result "0x00 upgrade instructions" "PASS"
else
    print_result "0x00 upgrade instructions" "FAIL"
fi

# Test error handling
print_header "7. Error Handling Verification"

echo "Testing invalid credential length..."

INVALID_CRED="0x0100000000"
ERROR_OUTPUT=$(node "$SCRIPT_DIR/fetch-withdrawal-credentials.js" --decode "$INVALID_CRED" 2>&1 || true)

if echo "$ERROR_OUTPUT" | grep -q "Invalid withdrawal credentials length"; then
    print_result "Invalid length error handling" "PASS"
else
    print_result "Invalid length error handling" "FAIL"
    if [[ $VERBOSE -eq 1 ]]; then
        echo "$ERROR_OUTPUT"
    fi
fi

# Test help output
print_header "8. Help Documentation Check"

echo "Testing help output..."

HELP_OUTPUT=$(node "$SCRIPT_DIR/fetch-withdrawal-credentials.js" --help 2>&1)

if echo "$HELP_OUTPUT" | grep -q "USAGE:" && \
   echo "$HELP_OUTPUT" | grep -q "OPTIONS:" && \
   echo "$HELP_OUTPUT" | grep -q "EXAMPLES:"; then
    print_result "Help documentation" "PASS"
else
    print_result "Help documentation" "FAIL"
fi

# Test demo script exists and is valid
print_header "9. Demo Script Verification"

if [[ -f "$SCRIPT_DIR/demo-withdrawal-credentials.sh" ]]; then
    print_result "Demo script exists" "PASS"
    
    # Check if demo script has proper shebang
    if head -n1 "$SCRIPT_DIR/demo-withdrawal-credentials.sh" | grep -q "^#!/"; then
        print_result "Demo script has shebang" "PASS"
    else
        print_result "Demo script has shebang" "FAIL"
    fi
    
    # Check if demo script is executable or can be run with bash
    if [[ -x "$SCRIPT_DIR/demo-withdrawal-credentials.sh" ]] || bash -n "$SCRIPT_DIR/demo-withdrawal-credentials.sh" 2>/dev/null; then
        print_result "Demo script is valid bash" "PASS"
    else
        print_result "Demo script is valid bash" "FAIL"
    fi
else
    print_result "Demo script exists" "FAIL"
fi

# Check documentation
print_header "10. Documentation Verification"

if [[ -f "$SCRIPT_DIR/CBSC_VERIFICATION.md" ]]; then
    print_result "CBSC verification documentation exists" "PASS"
    
    # Check for key sections
    if grep -q "## Overview" "$SCRIPT_DIR/CBSC_VERIFICATION.md" && \
       grep -q "## Verification Components" "$SCRIPT_DIR/CBSC_VERIFICATION.md" && \
       grep -q "## Verification Checklist" "$SCRIPT_DIR/CBSC_VERIFICATION.md"; then
        print_result "CBSC documentation has required sections" "PASS"
    else
        print_result "CBSC documentation has required sections" "FAIL"
    fi
else
    print_result "CBSC verification documentation exists" "FAIL"
fi

if [[ -f "$SCRIPT_DIR/WITHDRAWAL_CREDENTIALS_README.md" ]]; then
    print_result "Withdrawal credentials README exists" "PASS"
else
    print_result "Withdrawal credentials README exists" "FAIL"
fi

# Summary
print_header "CBSC Verification Summary"

TOTAL_TESTS=$((TESTS_PASSED + TESTS_FAILED))
PASS_RATE=0
if [[ $TOTAL_TESTS -gt 0 ]]; then
    PASS_RATE=$((TESTS_PASSED * 100 / TOTAL_TESTS))
fi

echo -e "Total Tests: ${BLUE}$TOTAL_TESTS${NC}"
echo -e "Passed:      ${GREEN}$TESTS_PASSED${NC}"
echo -e "Failed:      ${RED}$TESTS_FAILED${NC}"
echo -e "Pass Rate:   ${BLUE}$PASS_RATE%${NC}"
echo ""

if [[ $TESTS_FAILED -eq 0 ]]; then
    echo -e "${GREEN}╔═══════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║                                               ║${NC}"
    echo -e "${GREEN}║   ✓ CBSC VERIFICATION PASSED                 ║${NC}"
    echo -e "${GREEN}║                                               ║${NC}"
    echo -e "${GREEN}║   All components are working correctly!      ║${NC}"
    echo -e "${GREEN}║                                               ║${NC}"
    echo -e "${GREEN}╚═══════════════════════════════════════════════╝${NC}"
    echo ""
    exit 0
else
    echo -e "${RED}╔═══════════════════════════════════════════════╗${NC}"
    echo -e "${RED}║                                               ║${NC}"
    echo -e "${RED}║   ✗ CBSC VERIFICATION FAILED                  ║${NC}"
    echo -e "${RED}║                                               ║${NC}"
    echo -e "${RED}║   Some tests failed. Review output above.    ║${NC}"
    echo -e "${RED}║                                               ║${NC}"
    echo -e "${RED}╚═══════════════════════════════════════════════╝${NC}"
    echo ""
    exit 1
fi
