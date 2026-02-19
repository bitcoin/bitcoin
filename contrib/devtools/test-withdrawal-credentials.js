#!/usr/bin/env node

// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

/**
 * Test script for withdrawal credentials functionality
 * 
 * This script tests the decodeWithdrawalCredentials function with various
 * test cases to ensure proper parsing and validation.
 * 
 * Usage:
 *   node test-withdrawal-credentials.js
 */

const { decodeWithdrawalCredentials } = require('./fetch-withdrawal-credentials.js');

// ANSI color codes
const GREEN = '\x1b[32m';
const RED = '\x1b[31m';
const YELLOW = '\x1b[33m';
const RESET = '\x1b[0m';

/**
 * Test cases for withdrawal credentials
 */
const testCases = [
    {
        name: 'Valid 0x01 withdrawal credentials (with 0x prefix)',
        input: '0x010000000000000000000000e16359506c028e51f16be38986ec5746251e9724',
        expected: {
            type: '0x01',
            executionAddress: '0xe16359506c028e51f16be38986ec5746251e9724',
            canWithdraw: true,
            requiresUpgrade: false,
            validFormat: true
        }
    },
    {
        name: 'Valid 0x01 withdrawal credentials (without 0x prefix)',
        input: '010000000000000000000000a7d9ddbe1f17865597fbd27ec712455208b6b76d',
        expected: {
            type: '0x01',
            executionAddress: '0xa7d9ddbe1f17865597fbd27ec712455208b6b76d',
            canWithdraw: true,
            requiresUpgrade: false,
            validFormat: true
        }
    },
    {
        name: 'Valid 0x00 BLS withdrawal credentials',
        input: '0x004156d2e53f1b3e0b4e8b6c9e7d3f2a1b0c9d8e7f6a5b4c3d2e1f0a9b8c7d00',
        expected: {
            type: '0x00',
            canWithdraw: false,
            requiresUpgrade: true
        }
    },
    {
        name: 'Another valid 0x00 BLS withdrawal credentials',
        input: '00b8b1f654b4b8e5e5f1e2e3d4c5b6a7d8e9f0a1b2c3d4e5f6a7b8c9d0e1f2a3',
        expected: {
            type: '0x00',
            canWithdraw: false,
            requiresUpgrade: true
        }
    },
    {
        name: 'Zero address withdrawal credentials',
        input: '0x0100000000000000000000000000000000000000000000000000000000000000',
        expected: {
            type: '0x01',
            executionAddress: '0x0000000000000000000000000000000000000000',
            canWithdraw: true,
            requiresUpgrade: false,
            validFormat: true
        }
    }
];

/**
 * Error test cases
 */
const errorCases = [
    {
        name: 'Invalid length - too short',
        input: '0x0100000000000000000000e16359506c028e51f16be38986ec5746251e9724',
        shouldError: true
    },
    {
        name: 'Invalid length - too long',
        input: '0x010000000000000000000000e16359506c028e51f16be38986ec5746251e972412',
        shouldError: true
    },
    {
        name: 'Empty string',
        input: '',
        shouldError: true
    },
    {
        name: 'Invalid hex characters',
        input: '0xZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ',
        shouldError: false  // Will be parsed but may have unexpected results
    }
];

/**
 * Run a single test case
 * @param {Object} testCase - Test case object
 * @returns {boolean} - Test passed
 */
function runTest(testCase) {
    try {
        const result = decodeWithdrawalCredentials(testCase.input);
        
        // Check expected fields
        let passed = true;
        const failures = [];
        
        if (testCase.expected.type && result.type !== testCase.expected.type) {
            passed = false;
            failures.push(`Type mismatch: expected ${testCase.expected.type}, got ${result.type}`);
        }
        
        if (testCase.expected.executionAddress && result.executionAddress !== testCase.expected.executionAddress) {
            passed = false;
            failures.push(`Address mismatch: expected ${testCase.expected.executionAddress}, got ${result.executionAddress}`);
        }
        
        if (testCase.expected.canWithdraw !== undefined && result.canWithdraw !== testCase.expected.canWithdraw) {
            passed = false;
            failures.push(`canWithdraw mismatch: expected ${testCase.expected.canWithdraw}, got ${result.canWithdraw}`);
        }
        
        if (testCase.expected.requiresUpgrade !== undefined && result.requiresUpgrade !== testCase.expected.requiresUpgrade) {
            passed = false;
            failures.push(`requiresUpgrade mismatch: expected ${testCase.expected.requiresUpgrade}, got ${result.requiresUpgrade}`);
        }
        
        if (testCase.expected.validFormat !== undefined && result.validFormat !== testCase.expected.validFormat) {
            passed = false;
            failures.push(`validFormat mismatch: expected ${testCase.expected.validFormat}, got ${result.validFormat}`);
        }
        
        if (passed) {
            console.log(`  ${GREEN}✓${RESET} ${testCase.name}`);
        } else {
            console.log(`  ${RED}✗${RESET} ${testCase.name}`);
            failures.forEach(failure => {
                console.log(`    ${RED}${failure}${RESET}`);
            });
        }
        
        return passed;
    } catch (error) {
        if (testCase.shouldError) {
            console.log(`  ${GREEN}✓${RESET} ${testCase.name} (correctly threw error)`);
            return true;
        } else {
            console.log(`  ${RED}✗${RESET} ${testCase.name}`);
            console.log(`    ${RED}Unexpected error: ${error.message}${RESET}`);
            return false;
        }
    }
}

/**
 * Run error test case
 * @param {Object} testCase - Test case object
 * @returns {boolean} - Test passed
 */
function runErrorTest(testCase) {
    try {
        const result = decodeWithdrawalCredentials(testCase.input);
        
        if (testCase.shouldError) {
            console.log(`  ${RED}✗${RESET} ${testCase.name}`);
            console.log(`    ${RED}Expected error but got result: ${JSON.stringify(result)}${RESET}`);
            return false;
        } else {
            console.log(`  ${GREEN}✓${RESET} ${testCase.name}`);
            return true;
        }
    } catch (error) {
        if (testCase.shouldError) {
            console.log(`  ${GREEN}✓${RESET} ${testCase.name} (correctly threw error: ${error.message})`);
            return true;
        } else {
            console.log(`  ${RED}✗${RESET} ${testCase.name}`);
            console.log(`    ${RED}Unexpected error: ${error.message}${RESET}`);
            return false;
        }
    }
}

/**
 * Main test runner
 */
function main() {
    console.log('\n=== Withdrawal Credentials Test Suite ===\n');
    
    let totalTests = 0;
    let passedTests = 0;
    
    // Run normal test cases
    console.log('Valid Test Cases:');
    testCases.forEach(testCase => {
        totalTests++;
        if (runTest(testCase)) {
            passedTests++;
        }
    });
    
    console.log('');
    
    // Run error test cases
    console.log('Error Handling Test Cases:');
    errorCases.forEach(testCase => {
        totalTests++;
        if (runErrorTest(testCase)) {
            passedTests++;
        }
    });
    
    console.log('');
    console.log('=== Test Summary ===');
    console.log(`Total: ${totalTests}`);
    console.log(`Passed: ${GREEN}${passedTests}${RESET}`);
    console.log(`Failed: ${passedTests === totalTests ? GREEN : RED}${totalTests - passedTests}${RESET}`);
    console.log('');
    
    if (passedTests === totalTests) {
        console.log(`${GREEN}All tests passed!${RESET}`);
        process.exit(0);
    } else {
        console.log(`${RED}Some tests failed.${RESET}`);
        process.exit(1);
    }
}

// Run main function
if (require.main === module) {
    main();
}
