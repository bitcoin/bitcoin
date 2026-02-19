#!/usr/bin/env node

// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

/**
 * Fetch and decode Ethereum validator withdrawal credentials
 * 
 * This script queries Ethereum Beacon Chain validator withdrawal credentials
 * via the Beacon Chain API and decodes them to extract withdrawal addresses.
 * 
 * Withdrawal credentials are 32-byte values that specify where validator
 * withdrawals are sent:
 *   - Type 0x00: BLS withdrawal credentials (legacy)
 *   - Type 0x01: Execution layer withdrawal credentials (Ethereum address)
 * 
 * Usage:
 *   node fetch-withdrawal-credentials.js [OPTIONS]
 * 
 * Environment Variables:
 *   BEACON_API_URL    - Optional. Beacon Chain API endpoint (default: https://beaconcha.in/api/v1)
 *   VALIDATOR_INDEX   - Optional. Validator index to query
 *   VALIDATOR_PUBKEY  - Optional. Validator public key to query
 * 
 * Command Line Options:
 *   --index <number>      - Validator index
 *   --pubkey <hex>        - Validator public key (48 bytes hex)
 *   --api <url>           - Beacon Chain API endpoint
 *   --decode <hex>        - Decode withdrawal credentials hex directly
 *   --help, -h            - Show help message
 * 
 * Examples:
 *   # Query validator by index
 *   node fetch-withdrawal-credentials.js --index 12345
 * 
 *   # Query validator by public key
 *   node fetch-withdrawal-credentials.js --pubkey 0x123abc...
 * 
 *   # Decode withdrawal credentials directly
 *   node fetch-withdrawal-credentials.js --decode 0x010000000000000000000000e16359506c028e51f16be38986ec5746251e9724
 */

const https = require('https');

// Configuration
const DEFAULT_BEACON_API = 'beaconcha.in';
const DEFAULT_API_PATH = '/api/v1/validator';

/**
 * Parse command line arguments
 * @returns {Object} - Parsed configuration
 */
function parseArguments() {
    const config = {
        beaconApiUrl: process.env.BEACON_API_URL || DEFAULT_BEACON_API,
        validatorIndex: process.env.VALIDATOR_INDEX,
        validatorPubkey: process.env.VALIDATOR_PUBKEY,
        decodeOnly: null
    };
    
    const args = process.argv.slice(2);
    
    for (let i = 0; i < args.length; i++) {
        const arg = args[i];
        const nextArg = args[i + 1];
        
        switch (arg) {
            case '--index':
                config.validatorIndex = nextArg;
                i++;
                break;
            case '--pubkey':
                config.validatorPubkey = nextArg;
                i++;
                break;
            case '--api':
                config.beaconApiUrl = nextArg;
                i++;
                break;
            case '--decode':
                config.decodeOnly = nextArg;
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
Ethereum Withdrawal Credentials Tool

This tool fetches and decodes Ethereum validator withdrawal credentials
from the Beacon Chain.

USAGE:
    node fetch-withdrawal-credentials.js [OPTIONS]

OPTIONS:
    --index <number>    Query validator by index
    --pubkey <hex>      Query validator by public key (48 bytes hex)
    --api <url>         Beacon Chain API endpoint (default: beaconcha.in)
    --decode <hex>      Decode withdrawal credentials hex directly (32 bytes)
    -h, --help          Show this help message

ENVIRONMENT VARIABLES:
    BEACON_API_URL      Beacon Chain API endpoint
    VALIDATOR_INDEX     Validator index to query
    VALIDATOR_PUBKEY    Validator public key to query

EXAMPLES:
    # Query validator 12345
    node fetch-withdrawal-credentials.js --index 12345

    # Decode withdrawal credentials directly
    node fetch-withdrawal-credentials.js --decode 0x010000000000000000000000e16359506c028e51f16be38986ec5746251e9724

WITHDRAWAL CREDENTIALS FORMAT:
    - 32 bytes (64 hex characters)
    - Type 0x00: BLS withdrawal credentials (legacy, requires upgrade)
    - Type 0x01: Execution address (post-Shapella upgrade)
      Format: 0x01 + 11 zero bytes + 20-byte Ethereum address
    `);
}

/**
 * Decode withdrawal credentials
 * @param {string} credentials - Hex string of withdrawal credentials
 * @returns {Object} - Decoded information
 */
function decodeWithdrawalCredentials(credentials) {
    // Remove 0x prefix if present
    let hex = credentials.toLowerCase();
    if (hex.startsWith('0x')) {
        hex = hex.slice(2);
    }
    
    // Validate length (32 bytes = 64 hex chars)
    if (hex.length !== 64) {
        throw new Error(`Invalid withdrawal credentials length: ${hex.length} (expected 64 hex characters)`);
    }
    
    // Get credential type (first byte)
    const type = hex.slice(0, 2);
    
    const result = {
        raw: '0x' + hex,
        type: '0x' + type,
        typeDescription: ''
    };
    
    if (type === '00') {
        // BLS withdrawal credentials (legacy)
        result.typeDescription = 'BLS Withdrawal Credentials (0x00) - Legacy format, requires upgrade to 0x01';
        result.blsCredentials = '0x' + hex.slice(2);
        result.canWithdraw = false;
        result.requiresUpgrade = true;
    } else if (type === '01') {
        // Execution address withdrawal credentials
        result.typeDescription = 'Execution Address Withdrawal Credentials (0x01) - Can receive withdrawals';
        
        // Extract address (last 20 bytes)
        const address = hex.slice(-40);
        result.executionAddress = '0x' + address;
        result.canWithdraw = true;
        result.requiresUpgrade = false;
        
        // Validate padding (bytes 1-11 should be zero)
        const padding = hex.slice(2, 24);
        const validPadding = padding === '0000000000000000000000';
        result.validFormat = validPadding;
        
        if (!validPadding) {
            result.warning = 'Invalid padding detected in withdrawal credentials';
        }
    } else {
        result.typeDescription = `Unknown type (0x${type})`;
        result.canWithdraw = false;
        result.requiresUpgrade = false;
        result.warning = 'Unrecognized withdrawal credential type';
    }
    
    return result;
}

/**
 * Make HTTPS request
 * @param {string} hostname - API hostname
 * @param {string} path - API path
 * @returns {Promise<Object>} - Response data
 */
function makeRequest(hostname, path) {
    return new Promise((resolve, reject) => {
        const options = {
            hostname: hostname,
            path: path,
            method: 'GET',
            headers: {
                'User-Agent': 'Bitcoin-Core-Withdrawal-Credentials/1.0'
            }
        };
        
        const req = https.request(options, (res) => {
            let data = '';
            
            res.on('data', (chunk) => {
                data += chunk;
            });
            
            res.on('end', () => {
                try {
                    const parsed = JSON.parse(data);
                    resolve(parsed);
                } catch (error) {
                    reject(new Error(`Failed to parse JSON response: ${error.message}`));
                }
            });
        });
        
        req.on('error', (error) => {
            reject(error);
        });
        
        req.end();
    });
}

/**
 * Fetch validator data from Beacon Chain API
 * @param {string} identifier - Validator index or pubkey
 * @param {string} apiUrl - Beacon Chain API URL
 * @returns {Promise<Object>} - Validator data
 */
async function fetchValidatorData(identifier, apiUrl) {
    const path = `${DEFAULT_API_PATH}/${identifier}`;
    
    console.log(`Querying Beacon Chain API: https://${apiUrl}${path}`);
    
    try {
        const response = await makeRequest(apiUrl, path);
        
        if (response.status === 'ERROR' || !response.data) {
            throw new Error(`API Error: ${response.message || 'No data returned'}`);
        }
        
        return response.data;
    } catch (error) {
        throw new Error(`Failed to fetch validator data: ${error.message}`);
    }
}

/**
 * Format output
 * @param {Object} decoded - Decoded withdrawal credentials
 * @param {Object} validatorData - Optional validator data
 */
function formatOutput(decoded, validatorData = null) {
    console.log('\n=== Withdrawal Credentials Analysis ===\n');
    
    if (validatorData) {
        console.log('Validator Information:');
        if (validatorData.validatorindex !== undefined) {
            console.log(`  Index: ${validatorData.validatorindex}`);
        }
        if (validatorData.pubkey) {
            console.log(`  Public Key: ${validatorData.pubkey}`);
        }
        if (validatorData.status) {
            console.log(`  Status: ${validatorData.status}`);
        }
        console.log('');
    }
    
    console.log('Withdrawal Credentials:');
    console.log(`  Raw: ${decoded.raw}`);
    console.log(`  Type: ${decoded.type}`);
    console.log(`  Description: ${decoded.typeDescription}`);
    console.log('');
    
    if (decoded.type === '0x01') {
        console.log('Execution Address Information:');
        console.log(`  Address: ${decoded.executionAddress}`);
        console.log(`  Can Withdraw: ${decoded.canWithdraw ? 'Yes' : 'No'}`);
        console.log(`  Valid Format: ${decoded.validFormat ? 'Yes' : 'No'}`);
        
        if (decoded.warning) {
            console.log(`  ⚠️  Warning: ${decoded.warning}`);
        }
    } else if (decoded.type === '0x00') {
        console.log('BLS Credentials:');
        console.log(`  Hash: ${decoded.blsCredentials}`);
        console.log(`  Can Withdraw: No (requires upgrade to 0x01)`);
        console.log(`  Requires Upgrade: Yes`);
        console.log('');
        console.log('To enable withdrawals, the validator must broadcast a');
        console.log('BLSToExecutionChange message with a valid signature.');
    }
    
    console.log('');
}

/**
 * Main function
 */
async function main() {
    try {
        const config = parseArguments();
        
        // Decode only mode
        if (config.decodeOnly) {
            console.log('Decoding withdrawal credentials...');
            const decoded = decodeWithdrawalCredentials(config.decodeOnly);
            formatOutput(decoded);
            return;
        }
        
        // Validate inputs
        if (!config.validatorIndex && !config.validatorPubkey) {
            console.error('Error: Must provide --index, --pubkey, or --decode option');
            console.error('Use --help for usage information');
            process.exit(1);
        }
        
        // Fetch validator data
        const identifier = config.validatorIndex || config.validatorPubkey;
        const validatorData = await fetchValidatorData(identifier, config.beaconApiUrl);
        
        // Extract and decode withdrawal credentials
        let withdrawalCredentials = validatorData.withdrawalcredentials || validatorData.withdrawal_credentials;
        
        if (!withdrawalCredentials) {
            console.error('Error: No withdrawal credentials found in validator data');
            console.error('Raw response:', JSON.stringify(validatorData, null, 2));
            process.exit(1);
        }
        
        const decoded = decodeWithdrawalCredentials(withdrawalCredentials);
        formatOutput(decoded, validatorData);
        
    } catch (error) {
        console.error('Error:', error.message);
        process.exit(1);
    }
}

// Run main function if executed directly
if (require.main === module) {
    main();
}

// Export functions for testing
module.exports = {
    decodeWithdrawalCredentials,
    parseArguments
};
