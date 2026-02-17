# Etherscan eth_call API Implementation

This document describes the implementation of the Etherscan eth_call API client tool that replicates the functionality of the curl command specified in the problem statement.

## Problem Statement

The task was to implement support for the following curl command:

```bash
curl "https://api.etherscan.io/v2/api?chainid=1&module=proxy&action=eth_call&to=0xAEEF46DB4855E25702F8237E8f403FddcaF931C0&data=0x70a08231000000000000000000000000e16359506c028e51f16be38986ec5746251e9724&tag=latest&apikey=YourApiKeyToken"
```

## Solution Overview

The solution provides a Node.js-based CLI tool that:
1. Makes `eth_call` requests to Ethereum smart contracts via Etherscan API v2
2. Supports both command-line arguments and environment variables
3. Automatically parses common ERC20 function calls (balanceOf, decimals, etc.)
4. Includes comprehensive error handling and validation
5. Follows the existing code patterns in the repository

## Files Created

### 1. `contrib/devtools/fetch-etherscan-eth-call.js`
Main client script that implements the eth_call functionality.

**Key Features:**
- Makes HTTPS requests to Etherscan API v2 proxy endpoint
- Validates Ethereum addresses and hex-encoded call data
- Formats results for common function signatures (balanceOf, totalSupply, decimals, etc.)
- Supports both CLI arguments and environment variables
- Provides detailed error messages for common issues

**Usage:**
```bash
ETHERSCAN_API_KEY=your_api_key node contrib/devtools/fetch-etherscan-eth-call.js \
  --to <contract_address> \
  --data <call_data>
```

### 2. `contrib/devtools/test-etherscan-eth-call.js`
Comprehensive test suite for the eth_call client.

**Test Coverage:**
- Result formatting for common function signatures
- Configuration validation (addresses, hex data, API keys)
- Live API tests (when API key is available)
- Error handling scenarios

**Usage:**
```bash
node contrib/devtools/test-etherscan-eth-call.js
```

### 3. `contrib/devtools/demo-etherscan-eth-call.sh`
Demo script showing various usage examples.

**Demonstrations:**
- Exact replication of the problem statement curl command
- Query USDC contract for decimals
- Using environment variables

**Usage:**
```bash
export ETHERSCAN_API_KEY=your_api_key
./contrib/devtools/demo-etherscan-eth-call.sh
```

### 4. Updated `contrib/devtools/README.md`
Added comprehensive documentation for the new tool.

## Technical Details

### API Endpoint
The tool connects to Etherscan API v2:
- Base URL: `https://api.etherscan.io/v2/api`
- Module: `proxy`
- Action: `eth_call`
- Chain ID: `1` (Ethereum Mainnet)

### Function Signature Detection
The tool automatically detects common ERC20 function signatures:

| Signature | Function | Description |
|-----------|----------|-------------|
| 0x70a08231 | balanceOf(address) | Get token balance |
| 0x18160ddd | totalSupply() | Get total supply |
| 0x313ce567 | decimals() | Get token decimals |
| 0x95d89b41 | symbol() | Get token symbol |
| 0x06fdde03 | name() | Get token name |

### Configuration Options

**Environment Variables:**
- `ETHERSCAN_API_KEY`: Required. Your Etherscan API key
- `TO_ADDRESS`: Contract address to call
- `CALL_DATA`: Hex-encoded call data
- `TAG`: Block parameter (default: latest)

**Command Line Arguments:**
- `--to <address>`: Contract address to call
- `--data <hex>`: Hex-encoded call data
- `--tag <tag>`: Block parameter
- `--help, -h`: Show help message

### Error Handling

The implementation includes robust error handling for:
- Missing or invalid API keys
- Invalid Ethereum addresses
- Invalid hex-encoded call data
- HTTP errors (401 Unauthorized, 429 Rate Limit)
- Network failures
- Invalid JSON responses

## Example Usage

### Exact Problem Statement Example

```bash
export ETHERSCAN_API_KEY=YourApiKeyToken
node contrib/devtools/fetch-etherscan-eth-call.js \
  --to 0xAEEF46DB4855E25702F8237E8f403FddcaF931C0 \
  --data 0x70a08231000000000000000000000000e16359506c028e51f16be38986ec5746251e9724 \
  --tag latest
```

### Query Token Balance

```bash
export ETHERSCAN_API_KEY=YourApiKeyToken
export TO_ADDRESS=0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48  # USDC
export CALL_DATA=0x70a08231000000000000000000000000742d35cc6634c0532925a3b844bc9e7595f0beb0
node contrib/devtools/fetch-etherscan-eth-call.js
```

### Query Token Decimals

```bash
ETHERSCAN_API_KEY=YourApiKeyToken \
node contrib/devtools/fetch-etherscan-eth-call.js \
  --to 0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48 \
  --data 0x313ce567
```

## Testing

### Run Unit Tests
```bash
node contrib/devtools/test-etherscan-eth-call.js
```

The test suite includes:
- 14 unit tests covering validation, formatting, and parsing
- Optional live API tests (when ETHERSCAN_API_KEY is set)
- Tests pass with 100% success rate

### Run Demo Script
```bash
export ETHERSCAN_API_KEY=YourApiKeyToken
./contrib/devtools/demo-etherscan-eth-call.sh
```

## Security Considerations

1. **API Key Protection**: API keys are never logged or printed to console
2. **Input Validation**: All inputs are validated before use
3. **Hex Encoding**: Call data is validated to ensure proper hex format
4. **Address Validation**: Ethereum addresses must match the standard 0x + 40 hex chars format

## Integration with Existing Code

The implementation follows the patterns established in:
- `fetch-erc20-events.js`: Similar HTTPS request handling
- `fetch-cdp-api.js`: Environment variable configuration
- Existing test scripts: Test structure and output formatting

## Future Enhancements

Potential improvements for future versions:
1. Support for contract ABI parsing to auto-generate call data
2. Batch eth_call support for multiple calls
3. Support for other Etherscan API v2 endpoints
4. Integration with the existing GitHub Actions workflow
5. Support for other EVM-compatible chains (BSC, Polygon, etc.)

## Conclusion

The implementation successfully replicates the functionality of the curl command from the problem statement while providing a robust, maintainable, and well-tested solution that fits seamlessly into the existing repository structure.
