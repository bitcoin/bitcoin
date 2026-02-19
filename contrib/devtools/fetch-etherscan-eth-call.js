#!/usr/bin/env node

// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

/**
 * Fetch data using eth_call from Etherscan API
 * 
 * This script makes eth_call requests to Ethereum smart contracts via
 * the Etherscan API v2 proxy endpoint. It supports arbitrary contract calls
 * with custom data payloads.
 * 
 * Usage:
 *   ETHERSCAN_API_KEY=your_api_key node fetch-etherscan-eth-call.js [OPTIONS]
 * 
 * Environment Variables:
 *   ETHERSCAN_API_KEY - Required. Your Etherscan API key
 *   TO_ADDRESS        - Optional. Contract address to call
 *   CALL_DATA         - Optional. Hex-encoded call data
 *   TAG               - Optional. Block parameter (default: latest)
 * 
 * Command Line Options:
 *   --to <address>    - Contract address to call
 *   --data <hex>      - Hex-encoded call data (with or without 0x prefix)
 *   --tag <tag>       - Block parameter (default: latest)
 *   --help, -h        - Show help message
 * 
 * Examples:
 *   # Call balanceOf(address) on an ERC20 contract
 *   ETHERSCAN_API_KEY=ABC123 node fetch-etherscan-eth-call.js \
 *     --to 0xAEEF46DB4855E25702F8237E8f403FddcaF931C0 \
 *     --data 0x70a08231000000000000000000000000e16359506c028e51f16be38986ec5746251e9724
 * 
 *   # Using environment variables
 *   export ETHERSCAN_API_KEY=ABC123
 *   export TO_ADDRESS=0xAEEF46DB4855E25702F8237E8f403FddcaF931C0
 *   export CALL_DATA=0x70a08231000000000000000000000000e16359506c028e51f16be38986ec5746251e9724
 *   node fetch-etherscan-eth-call.js
 */

const https = require('https');

// Configuration
const BASE_URL = 'api.etherscan.io';
const API_PATH = '/v2/api';
const CHAIN_ID = '1'; // Ethereum mainnet

/**
 * Parse command line arguments
 * @returns {Object} - Parsed configuration
 */
function parseArguments() {
    const config = {
        apiKey: process.env.ETHERSCAN_API_KEY,
        toAddress: process.env.TO_ADDRESS,
        callData: process.env.CALL_DATA,
        tag: process.env.TAG || 'latest'
    };
    
    const args = process.argv.slice(2);
    
    for (let i = 0; i < args.length; i++) {
        const arg = args[i];
        const nextArg = args[i + 1];
        
        switch (arg) {
            case '--to':
                config.toAddress = nextArg;
                i++;
                break;
            case '--data':
                config.callData = nextArg;
                i++;
                break;
            case '--tag':
                config.tag = nextArg;
                i++;
                break;
            case '--help':
            case '-h':
                printHelp();
                process.exit(0);
                break;
        }
    }
    
    return config;
}

/**
 * Print help message
 */
function printHelp() {
    console.log(`
Etherscan API eth_call Client

This tool makes eth_call requests to Ethereum smart contracts via the
Etherscan API v2 proxy endpoint.

Usage:
  node fetch-etherscan-eth-call.js [options]

Options:
  --to <address>       Contract address to call (required)
  --data <hex>         Hex-encoded call data (required)
  --tag <tag>          Block parameter (default: latest)
  --help, -h           Show this help message

Environment Variables:
  ETHERSCAN_API_KEY    Your Etherscan API key (required)
  TO_ADDRESS           Contract address to call
  CALL_DATA            Hex-encoded call data
  TAG                  Block parameter (default: latest)

Examples:
  # Call balanceOf(address) on an ERC20 contract
  ETHERSCAN_API_KEY=ABC123 node fetch-etherscan-eth-call.js \\
    --to 0xAEEF46DB4855E25702F8237E8f403FddcaF931C0 \\
    --data 0x70a08231000000000000000000000000e16359506c028e51f16be38986ec5746251e9724

  # Using environment variables
  export ETHERSCAN_API_KEY=ABC123
  export TO_ADDRESS=0xAEEF46DB4855E25702F8237E8f403FddcaF931C0
  export CALL_DATA=0x70a08231000000000000000000000000e16359506c028e51f16be38986ec5746251e9724
  node fetch-etherscan-eth-call.js

Get your API key at: https://etherscan.io/myapikey
`);
}

/**
 * Validate configuration
 * @param {Object} config - Configuration object
 * @throws {Error} - If configuration is invalid
 */
function validateConfig(config) {
    const errors = [];
    
    if (!config.apiKey) {
        errors.push('ETHERSCAN_API_KEY is required (set env var or use --api-key)');
    }
    
    if (!config.toAddress) {
        errors.push('TO_ADDRESS is required (set env var or use --to)');
    } else {
        // Validate address format
        if (!config.toAddress.match(/^0x[a-fA-F0-9]{40}$/)) {
            errors.push('TO_ADDRESS must be a valid Ethereum address (0x followed by 40 hex characters)');
        }
    }
    
    if (!config.callData) {
        errors.push('CALL_DATA is required (set env var or use --data)');
    } else {
        // Normalize call data (add 0x prefix if missing)
        if (!config.callData.startsWith('0x')) {
            config.callData = '0x' + config.callData;
        }
        
        // Validate hex format
        if (!config.callData.match(/^0x[a-fA-F0-9]*$/)) {
            errors.push('CALL_DATA must be valid hex (optionally prefixed with 0x)');
        }
    }
    
    if (errors.length > 0) {
        throw new Error('Configuration errors:\n  - ' + errors.join('\n  - '));
    }
}

/**
 * Make eth_call request to Etherscan API
 * @param {Object} config - Configuration object
 * @returns {Promise<Object>} - API response
 */
function makeEthCall(config) {
    return new Promise((resolve, reject) => {
        // Build query parameters
        const params = {
            chainid: CHAIN_ID,
            module: 'proxy',
            action: 'eth_call',
            to: config.toAddress,
            data: config.callData,
            tag: config.tag,
            apikey: config.apiKey
        };
        
        // Build query string
        const queryString = Object.keys(params)
            .map(key => `${encodeURIComponent(key)}=${encodeURIComponent(params[key])}`)
            .join('&');
        
        const options = {
            hostname: BASE_URL,
            path: `${API_PATH}?${queryString}`,
            method: 'GET',
            headers: {
                'User-Agent': 'Bitcoin-Core-Etherscan-EthCall/1.0'
            }
        };

        const req = https.request(options, (res) => {
            let data = '';

            // Handle non-200 status codes
            if (res.statusCode !== 200) {
                reject(new Error(`HTTP Error: ${res.statusCode} ${res.statusMessage}`));
                return;
            }

            res.on('data', (chunk) => {
                data += chunk;
            });

            res.on('end', () => {
                try {
                    const jsonData = JSON.parse(data);
                    resolve(jsonData);
                } catch (error) {
                    reject(new Error(`Failed to parse JSON response: ${error.message}`));
                }
            });
        });

        req.on('error', (error) => {
            reject(new Error(`Request failed: ${error.message}`));
        });

        req.end();
    });
}

/**
 * Parse and format the result based on common contract functions
 * @param {string} result - Hex result from eth_call
 * @param {string} callData - Original call data to determine function
 * @returns {Object} - Formatted result
 */
function formatResult(result, callData) {
    if (!result || result === '0x') {
        return { raw: result, formatted: 'No data returned' };
    }
    
    // Detect function signature (first 4 bytes / 8 hex chars after 0x)
    const functionSig = callData.substring(0, 10);
    
    const formatted = { raw: result };
    
    // Common function signatures
    switch (functionSig) {
        case '0x70a08231': // balanceOf(address)
            // Result is uint256 balance
            formatted.function = 'balanceOf(address)';
            formatted.type = 'uint256';
            formatted.value = BigInt(result).toString();
            formatted.description = 'Token balance (in smallest unit)';
            break;
            
        case '0x18160ddd': // totalSupply()
            formatted.function = 'totalSupply()';
            formatted.type = 'uint256';
            formatted.value = BigInt(result).toString();
            formatted.description = 'Total token supply';
            break;
            
        case '0x313ce567': // decimals()
            formatted.function = 'decimals()';
            formatted.type = 'uint8';
            formatted.value = parseInt(result, 16);
            formatted.description = 'Token decimals';
            break;
            
        case '0x95d89b41': // symbol()
            formatted.function = 'symbol()';
            formatted.type = 'string';
            formatted.description = 'Token symbol';
            // Parse string from result (skip first 64 bytes, then read string)
            break;
            
        case '0x06fdde03': // name()
            formatted.function = 'name()';
            formatted.type = 'string';
            formatted.description = 'Token name';
            break;
            
        default:
            formatted.function = 'unknown';
            formatted.type = 'bytes';
            formatted.description = 'Raw hex result';
    }
    
    return formatted;
}

/**
 * Main function
 */
async function main() {
    try {
        // Parse and validate configuration
        const config = parseArguments();
        validateConfig(config);
        
        console.log('Etherscan API eth_call Client');
        console.log('═'.repeat(80));
        console.log('');
        console.log('Configuration:');
        console.log(`  Chain ID:   ${CHAIN_ID} (Ethereum Mainnet)`);
        console.log(`  To:         ${config.toAddress}`);
        console.log(`  Data:       ${config.callData}`);
        console.log(`  Tag:        ${config.tag}`);
        console.log('');
        console.log('Making eth_call request...');
        console.log('');
        
        // Make the API call
        const response = await makeEthCall(config);
        
        // Check for API errors
        if (response.error) {
            console.error('API Error:');
            console.error(`  Code:    ${response.error.code}`);
            console.error(`  Message: ${response.error.message}`);
            process.exit(1);
        }
        
        // Parse and format result
        const formattedResult = formatResult(response.result, config.callData);
        
        // Print results
        console.log('═'.repeat(80));
        console.log('Result:');
        console.log('─'.repeat(80));
        console.log(`Raw:         ${formattedResult.raw}`);
        
        if (formattedResult.function && formattedResult.function !== 'unknown') {
            console.log(`Function:    ${formattedResult.function}`);
            console.log(`Type:        ${formattedResult.type}`);
            if (formattedResult.value !== undefined) {
                console.log(`Value:       ${formattedResult.value}`);
            }
            console.log(`Description: ${formattedResult.description}`);
        }
        
        console.log('═'.repeat(80));
        console.log('');
        
        // Also output full response as JSON
        console.log('Full API Response:');
        console.log(JSON.stringify(response, null, 2));
        
    } catch (error) {
        console.error('');
        console.error('Error:', error.message);
        console.error('');
        
        if (error.message.includes('HTTP Error: 401')) {
            console.error('Your API key is invalid or expired.');
            console.error('Please check your ETHERSCAN_API_KEY and try again.');
        } else if (error.message.includes('HTTP Error: 429')) {
            console.error('Rate limit exceeded. Please wait and try again later.');
        } else if (error.message.includes('Request failed')) {
            console.error('Network error. Please check your internet connection.');
        }
        
        console.error('Run with --help for usage information.');
        process.exit(1);
    }
}

// Run the script
if (require.main === module) {
    main();
}

// Export for testing
module.exports = { makeEthCall, formatResult, parseArguments, validateConfig };
