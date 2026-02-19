# Ethereum Withdrawal Credentials Tool

## Overview

This tool provides utilities for querying and decoding Ethereum 2.0 validator withdrawal credentials. Withdrawal credentials are 32-byte values that specify where validator withdrawals and rewards are sent on the Ethereum network.

## Background

After the Shapella upgrade (April 2023), Ethereum validators can withdraw their staked ETH and rewards. Withdrawal credentials determine where these funds are sent:

- **Type 0x00 (BLS)**: Legacy format that requires an upgrade to enable withdrawals
- **Type 0x01 (Execution Address)**: Modern format with a specified Ethereum address for withdrawals

## Components

### 1. Main Script: `fetch-withdrawal-credentials.js`

Query and decode withdrawal credentials from the Beacon Chain or analyze them offline.

**Features:**
- Query validators by index or public key via Beacon Chain API
- Decode withdrawal credentials hex directly
- Identify credential type (0x00 or 0x01)
- Extract execution addresses from 0x01 credentials
- Validate credential format
- Detect legacy 0x00 credentials that require upgrades

**Usage:**
```bash
# Query validator by index
node fetch-withdrawal-credentials.js --index 12345

# Query validator by public key
node fetch-withdrawal-credentials.js --pubkey 0x123abc...

# Decode withdrawal credentials directly (offline)
node fetch-withdrawal-credentials.js --decode 0x010000000000000000000000e16359506c028e51f16be38986ec5746251e9724

# Use custom Beacon Chain API endpoint
node fetch-withdrawal-credentials.js --index 12345 --api custom-beacon-api.com
```

**Environment Variables:**
- `BEACON_API_URL`: Beacon Chain API endpoint (default: beaconcha.in)
- `VALIDATOR_INDEX`: Validator index to query
- `VALIDATOR_PUBKEY`: Validator public key

### 2. Test Suite: `test-withdrawal-credentials.js`

Comprehensive test suite for withdrawal credentials functionality.

**Usage:**
```bash
node test-withdrawal-credentials.js
```

**Test Coverage:**
- Valid 0x01 credentials (with and without 0x prefix)
- Valid 0x00 BLS credentials
- Zero address credentials
- Invalid length handling
- Error cases

### 3. Demo Script: `demo-withdrawal-credentials.sh`

Interactive demonstration of the tool's capabilities.

**Usage:**
```bash
bash demo-withdrawal-credentials.sh
```

**Demonstrations:**
- Decoding 0x01 (execution address) credentials
- Decoding 0x00 (BLS) credentials
- Viewing help information

## Withdrawal Credentials Format

### Type 0x01: Execution Address
```
Format: 0x01 + 11 zero bytes + 20-byte Ethereum address
Example: 0x010000000000000000000000e16359506c028e51f16be38986ec5746251e9724
                             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                                        Ethereum Address
```

**Properties:**
- Can receive validator withdrawals immediately
- No upgrade required
- Direct to Ethereum address
- Introduced with Shapella upgrade

### Type 0x00: BLS Credentials
```
Format: 0x00 + 31-byte BLS hash
Example: 0x004156d2e53f1b3e0b4e8b6c9e7d3f2a1b0c9d8e7f6a5b4c3d2e1f0a9b8c7d00
```

**Properties:**
- Legacy format from Ethereum 2.0 Phase 0
- Cannot receive withdrawals directly
- Requires upgrade to 0x01 via BLSToExecutionChange message
- Validator must sign message with withdrawal key

## Examples

### Example 1: Decode Valid 0x01 Credentials

```bash
$ node fetch-withdrawal-credentials.js --decode 0x010000000000000000000000e16359506c028e51f16be38986ec5746251e9724

=== Withdrawal Credentials Analysis ===

Withdrawal Credentials:
  Raw: 0x010000000000000000000000e16359506c028e51f16be38986ec5746251e9724
  Type: 0x01
  Description: Execution Address Withdrawal Credentials (0x01) - Can receive withdrawals

Execution Address Information:
  Address: 0xe16359506c028e51f16be38986ec5746251e9724
  Can Withdraw: Yes
  Valid Format: Yes
```

### Example 2: Decode Legacy 0x00 Credentials

```bash
$ node fetch-withdrawal-credentials.js --decode 0x004156d2e53f1b3e0b4e8b6c9e7d3f2a1b0c9d8e7f6a5b4c3d2e1f0a9b8c7d00

=== Withdrawal Credentials Analysis ===

Withdrawal Credentials:
  Raw: 0x004156d2e53f1b3e0b4e8b6c9e7d3f2a1b0c9d8e7f6a5b4c3d2e1f0a9b8c7d00
  Type: 0x00
  Description: BLS Withdrawal Credentials (0x00) - Legacy format, requires upgrade to 0x01

BLS Credentials:
  Hash: 0x4156d2e53f1b3e0b4e8b6c9e7d3f2a1b0c9d8e7f6a5b4c3d2e1f0a9b8c7d00
  Can Withdraw: No (requires upgrade to 0x01)
  Requires Upgrade: Yes

To enable withdrawals, the validator must broadcast a
BLSToExecutionChange message with a valid signature.
```

### Example 3: Query Live Validator

```bash
# Query validator 12345 from Beacon Chain
$ node fetch-withdrawal-credentials.js --index 12345

Querying Beacon Chain API: https://beaconcha.in/api/v1/validator/12345

=== Withdrawal Credentials Analysis ===

Validator Information:
  Index: 12345
  Public Key: 0x123abc...
  Status: active_online

Withdrawal Credentials:
  Raw: 0x010000000000000000000000...
  Type: 0x01
  Description: Execution Address Withdrawal Credentials (0x01) - Can receive withdrawals

Execution Address Information:
  Address: 0x...
  Can Withdraw: Yes
  Valid Format: Yes
```

## Integration with Repository

This tool follows the established patterns in this repository:

1. **Similar Structure**: Modeled after `fetch-etherscan-eth-call.js` and `fetch-erc20-events.js`
2. **Testing Pattern**: Follows testing conventions like `test-etherscan-eth-call.js`
3. **Demo Scripts**: Consistent with `demo-etherscan-eth-call.sh`
4. **Documentation**: Matches existing documentation standards

## Use Cases

1. **Validator Monitoring**: Check withdrawal credentials for validators you're operating or monitoring
2. **Staking Services**: Verify withdrawal addresses for customer validators
3. **Security Audits**: Validate withdrawal credential formats and addresses
4. **Migration Planning**: Identify validators that need 0x00→0x01 upgrades
5. **Educational**: Learn about Ethereum 2.0 withdrawal credentials

## API Endpoints

### Beaconcha.in API
- **Endpoint**: `https://beaconcha.in/api/v1/validator/{index_or_pubkey}`
- **Rate Limits**: Free tier has rate limits
- **Documentation**: https://beaconcha.in/api/v1/docs

### Alternative APIs
- **Lighthouse**: Custom Beacon Node endpoints
- **Prysm**: Custom Beacon Node endpoints
- **Teku**: Custom Beacon Node endpoints

Use `--api` flag to specify custom endpoint.

## Security Considerations

1. **API Keys**: Beacon Chain APIs typically don't require keys, but rate limiting applies
2. **Validation**: Always validate withdrawal addresses match your expected values
3. **0x00 Upgrades**: Requires access to validator withdrawal keys - handle with extreme care
4. **Address Verification**: Cross-reference withdrawal addresses with multiple sources

## Troubleshooting

### "Invalid withdrawal credentials length"
- Ensure hex string is exactly 64 characters (32 bytes)
- Check for missing or extra characters
- Verify 0x prefix is handled correctly

### "API Error: No data returned"
- Validator index/pubkey may be invalid
- Beacon Chain API may be down or rate limited
- Try alternative API endpoint with `--api` flag

### "Unknown type"
- Withdrawal credential type is not 0x00 or 0x01
- May indicate corrupted data or future format
- Verify data source

## References

- [Ethereum Withdrawal Credentials Documentation](https://ethereum.org/en/staking/withdrawal-credentials/)
- [EIP-4895: Beacon chain push withdrawals](https://eips.ethereum.org/EIPS/eip-4895)
- [Shapella Upgrade Information](https://ethereum.org/en/history/#shapella)
- [Beaconcha.in API Documentation](https://beaconcha.in/api/v1/docs)

## License

Copyright (c) 2026 The Bitcoin Core developers
Distributed under the MIT software license, see the accompanying
file COPYING or https://opensource.org/license/mit.

## Contributing

This tool is part of the Bitcoin Core repository's Ethereum integration toolset. For contributions, please follow the repository's contribution guidelines in [CONTRIBUTING.md](../../CONTRIBUTING.md).

---

**Related Tools:**
- `fetch-etherscan-eth-call.js` - Ethereum smart contract calls
- `fetch-erc20-events.js` - ERC20 token event monitoring
- `fetch-cdp-api.js` - Coinbase Developer Platform API
