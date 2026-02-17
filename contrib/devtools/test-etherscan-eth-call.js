#!/usr/bin/env node

// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

/**
 * Test script for fetch-etherscan-eth-call.js
 * 
 * This script tests the functionality of the Etherscan eth_call client
 * with various scenarios including validation and error handling.
 * 
 * Usage:
 *   node test-etherscan-eth-call.js
 */

const {
    makeEthCall,
    formatResult,
    parseArguments,
    validateConfig
} = require('./fetch-etherscan-eth-call.js');

// ANSI color codes for test output
const colors = {
    green: '\x1b[32m',
    red: '\x1b[31m',
    yellow: '\x1b[33m',
    blue: '\x1b[34m',
    reset: '\x1b[0m'
};

// Test result tracking
let testsRun = 0;
let testsPassed = 0;
let testsFailed = 0;

/**
 * Assert function for tests
 * @param {boolean} condition - Condition to check
 * @param {string} message - Test description
 */
function assert(condition, message) {
    testsRun++;
    if (condition) {
        testsPassed++;
        console.log(`${colors.green}✓${colors.reset} ${message}`);
    } else {
        testsFailed++;
        console.log(`${colors.red}✗${colors.reset} ${message}`);
    }
}

/**
 * Test formatResult function
 */
function testFormatResult() {
    console.log('\n' + colors.blue + 'Testing formatResult function...' + colors.reset);
    
    // Test balanceOf result
    const balanceResult = '0x0000000000000000000000000000000000000000000000000000000000000064';
    const balanceData = '0x70a08231000000000000000000000000e16359506c028e51f16be38986ec5746251e9724';
    const formatted = formatResult(balanceResult, balanceData);
    
    assert(
        formatted.function === 'balanceOf(address)',
        'Should identify balanceOf function'
    );
    assert(
        formatted.value === '100',
        'Should parse uint256 value correctly (100 in this case)'
    );
    
    // Test empty result
    const emptyFormatted = formatResult('0x', balanceData);
    assert(
        emptyFormatted.formatted === 'No data returned',
        'Should handle empty result'
    );
    
    // Test totalSupply
    const supplyResult = '0x00000000000000000000000000000000000000000000d3c21bcecceda1000000';
    const supplyData = '0x18160ddd';
    const supplyFormatted = formatResult(supplyResult, supplyData);
    
    assert(
        supplyFormatted.function === 'totalSupply()',
        'Should identify totalSupply function'
    );
    
    // Test decimals
    const decimalsResult = '0x0000000000000000000000000000000000000000000000000000000000000012';
    const decimalsData = '0x313ce567';
    const decimalsFormatted = formatResult(decimalsResult, decimalsData);
    
    assert(
        decimalsFormatted.function === 'decimals()',
        'Should identify decimals function'
    );
    assert(
        decimalsFormatted.value === 18,
        'Should parse uint8 decimals correctly'
    );
    
    // Test unknown function
    const unknownData = '0x12345678';
    const unknownFormatted = formatResult(balanceResult, unknownData);
    
    assert(
        unknownFormatted.function === 'unknown',
        'Should handle unknown function signatures'
    );
}

/**
 * Test validateConfig function
 */
function testValidateConfig() {
    console.log('\n' + colors.blue + 'Testing validateConfig function...' + colors.reset);
    
    // Test valid config
    const validConfig = {
        apiKey: 'test-key',
        toAddress: '0xAEEF46DB4855E25702F8237E8f403FddcaF931C0',
        callData: '0x70a08231000000000000000000000000e16359506c028e51f16be38986ec5746251e9724',
        tag: 'latest'
    };
    
    try {
        validateConfig(validConfig);
        assert(true, 'Should accept valid configuration');
    } catch (error) {
        assert(false, 'Should accept valid configuration');
    }
    
    // Test missing API key
    const noKeyConfig = { ...validConfig, apiKey: null };
    try {
        validateConfig(noKeyConfig);
        assert(false, 'Should reject config without API key');
    } catch (error) {
        assert(
            error.message.includes('ETHERSCAN_API_KEY'),
            'Should reject config without API key'
        );
    }
    
    // Test missing address
    const noAddressConfig = { ...validConfig, toAddress: null };
    try {
        validateConfig(noAddressConfig);
        assert(false, 'Should reject config without TO_ADDRESS');
    } catch (error) {
        assert(
            error.message.includes('TO_ADDRESS'),
            'Should reject config without TO_ADDRESS'
        );
    }
    
    // Test invalid address format
    const invalidAddressConfig = { ...validConfig, toAddress: '0xinvalid' };
    try {
        validateConfig(invalidAddressConfig);
        assert(false, 'Should reject invalid address format');
    } catch (error) {
        assert(
            error.message.includes('valid Ethereum address'),
            'Should reject invalid address format'
        );
    }
    
    // Test missing call data
    const noDataConfig = { ...validConfig, callData: null };
    try {
        validateConfig(noDataConfig);
        assert(false, 'Should reject config without CALL_DATA');
    } catch (error) {
        assert(
            error.message.includes('CALL_DATA'),
            'Should reject config without CALL_DATA'
        );
    }
    
    // Test call data without 0x prefix (should be normalized)
    const noPrefixConfig = {
        ...validConfig,
        callData: '70a08231000000000000000000000000e16359506c028e51f16be38986ec5746251e9724'
    };
    try {
        validateConfig(noPrefixConfig);
        assert(
            noPrefixConfig.callData.startsWith('0x'),
            'Should add 0x prefix to call data if missing'
        );
    } catch (error) {
        assert(false, 'Should normalize call data without 0x prefix');
    }
    
    // Test invalid hex in call data
    const invalidHexConfig = { ...validConfig, callData: '0xGGGG' };
    try {
        validateConfig(invalidHexConfig);
        assert(false, 'Should reject invalid hex in call data');
    } catch (error) {
        assert(
            error.message.includes('valid hex'),
            'Should reject invalid hex in call data'
        );
    }
}

/**
 * Test makeEthCall function (requires API key)
 */
async function testMakeEthCall() {
    console.log('\n' + colors.blue + 'Testing makeEthCall function...' + colors.reset);
    
    // Check if API key is available
    const apiKey = process.env.ETHERSCAN_API_KEY;
    
    if (!apiKey) {
        console.log(colors.yellow + '⚠ Skipping API tests - ETHERSCAN_API_KEY not set' + colors.reset);
        console.log('  Set ETHERSCAN_API_KEY environment variable to run API tests');
        return;
    }
    
    console.log('API key found, running live API tests...');
    
    try {
        // Test with a well-known contract (USDC on Ethereum)
        const config = {
            apiKey: apiKey,
            toAddress: '0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48',  // USDC contract
            callData: '0x313ce567',  // decimals() function
            tag: 'latest'
        };
        
        const response = await makeEthCall(config);
        
        assert(
            response && response.result,
            'Should receive response from API'
        );
        
        if (response.result) {
            const formatted = formatResult(response.result, config.callData);
            assert(
                formatted.value === 6,
                'Should correctly parse USDC decimals (6)'
            );
        }
        
    } catch (error) {
        // API tests may fail due to network issues, rate limits, etc.
        console.log(colors.yellow + `⚠ API test failed: ${error.message}` + colors.reset);
        console.log('  This may be due to network issues or API rate limits');
    }
}

/**
 * Main test runner
 */
async function runTests() {
    console.log('\n' + colors.blue + '═'.repeat(80) + colors.reset);
    console.log(colors.blue + 'Etherscan eth_call Client - Test Suite' + colors.reset);
    console.log(colors.blue + '═'.repeat(80) + colors.reset);
    
    // Run all tests
    testFormatResult();
    testValidateConfig();
    await testMakeEthCall();
    
    // Print summary
    console.log('\n' + colors.blue + '═'.repeat(80) + colors.reset);
    console.log('Test Summary:');
    console.log(`  Total:  ${testsRun}`);
    console.log(`  ${colors.green}Passed: ${testsPassed}${colors.reset}`);
    if (testsFailed > 0) {
        console.log(`  ${colors.red}Failed: ${testsFailed}${colors.reset}`);
    } else {
        console.log(`  Failed: ${testsFailed}`);
    }
    console.log(colors.blue + '═'.repeat(80) + colors.reset);
    
    // Exit with appropriate code
    process.exit(testsFailed > 0 ? 1 : 0);
}

// Run tests
if (require.main === module) {
    runTests();
}
