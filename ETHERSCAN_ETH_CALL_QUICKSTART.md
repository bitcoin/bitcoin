# Quick Start Guide - Etherscan eth_call API Tool

## Installation

No installation required! The tool uses Node.js built-in modules.

## Prerequisites

- Node.js v12 or higher
- Etherscan API key (get one at https://etherscan.io/myapikey)

## Quick Start

### 1. Set Your API Key

```bash
export ETHERSCAN_API_KEY=YourApiKeyToken
```

### 2. Run a Simple Query

Query USDC decimals:
```bash
node contrib/devtools/fetch-etherscan-eth-call.js \
  --to 0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48 \
  --data 0x313ce567
```

### 3. Query from Problem Statement

```bash
node contrib/devtools/fetch-etherscan-eth-call.js \
  --to 0xAEEF46DB4855E25702F8237E8f403FddcaF931C0 \
  --data 0x70a08231000000000000000000000000e16359506c028e51f16be38986ec5746251e9724
```

## Common Use Cases

### Query Token Balance (balanceOf)

```bash
# Function signature: balanceOf(address)
# Signature: 0x70a08231

node contrib/devtools/fetch-etherscan-eth-call.js \
  --to <TOKEN_CONTRACT_ADDRESS> \
  --data 0x70a08231000000000000000000000000<ADDRESS_TO_CHECK>
```

### Query Token Decimals

```bash
# Function signature: decimals()
# Signature: 0x313ce567

node contrib/devtools/fetch-etherscan-eth-call.js \
  --to <TOKEN_CONTRACT_ADDRESS> \
  --data 0x313ce567
```

### Query Total Supply

```bash
# Function signature: totalSupply()
# Signature: 0x18160ddd

node contrib/devtools/fetch-etherscan-eth-call.js \
  --to <TOKEN_CONTRACT_ADDRESS> \
  --data 0x18160ddd
```

### Query Token Symbol

```bash
# Function signature: symbol()
# Signature: 0x95d89b41

node contrib/devtools/fetch-etherscan-eth-call.js \
  --to <TOKEN_CONTRACT_ADDRESS> \
  --data 0x95d89b41
```

### Query Token Name

```bash
# Function signature: name()
# Signature: 0x06fdde03

node contrib/devtools/fetch-etherscan-eth-call.js \
  --to <TOKEN_CONTRACT_ADDRESS> \
  --data 0x06fdde03
```

## Using Environment Variables

```bash
export ETHERSCAN_API_KEY=YourApiKeyToken
export TO_ADDRESS=0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48
export CALL_DATA=0x313ce567
export TAG=latest

node contrib/devtools/fetch-etherscan-eth-call.js
```

## Testing

Run the test suite:
```bash
node contrib/devtools/test-etherscan-eth-call.js
```

Expected output:
```
════════════════════════════════════════════════════════════════════════════════
Etherscan eth_call Client - Test Suite
════════════════════════════════════════════════════════════════════════════════

Testing formatResult function...
✓ Should identify balanceOf function
✓ Should parse uint256 value correctly (100 in this case)
...

Test Summary:
  Total:  14
  Passed: 14
  Failed: 0
════════════════════════════════════════════════════════════════════════════════
```

## Demo

Run the demo script to see various examples:
```bash
export ETHERSCAN_API_KEY=YourApiKeyToken
./contrib/devtools/demo-etherscan-eth-call.sh
```

## Help

View all available options:
```bash
node contrib/devtools/fetch-etherscan-eth-call.js --help
```

## Troubleshooting

### Error: ETHERSCAN_API_KEY is required
**Solution**: Set your API key:
```bash
export ETHERSCAN_API_KEY=YourApiKeyToken
```

### Error: TO_ADDRESS must be a valid Ethereum address
**Solution**: Ensure address starts with 0x and has exactly 40 hex characters:
```bash
# Correct format
--to 0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48
```

### Error: CALL_DATA must be valid hex
**Solution**: Ensure data is hex-encoded (0x prefix optional):
```bash
# Both formats work
--data 0x313ce567
--data 313ce567
```

### HTTP Error: 401 Unauthorized
**Solution**: Check that your API key is valid

### HTTP Error: 429 Rate Limit
**Solution**: Wait a moment and try again, or upgrade your Etherscan API plan

## More Information

- **Implementation Guide**: See `ETHERSCAN_ETH_CALL_IMPLEMENTATION.md`
- **Verification**: See `IMPLEMENTATION_VERIFICATION.md`
- **Full Documentation**: See `contrib/devtools/README.md`

## Support

For issues or questions, please refer to the documentation files or open an issue in the repository.
