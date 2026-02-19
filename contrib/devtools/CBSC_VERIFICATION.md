# CBSC Verification Guide

## Overview

CBSC (Credentials Beacon Signature Check) is the verification process for Ethereum validator withdrawal credentials. This document describes how to verify withdrawal credentials and their associated signatures.

## What is CBSC?

CBSC refers to the comprehensive verification of:
1. **Credentials** - Ethereum validator withdrawal credentials format and validity
2. **Beacon** - Data retrieved from the Beacon Chain API
3. **Signature** - BLS signatures for BLSToExecutionChange messages (when upgrading 0x00 to 0x01)
4. **Check** - Validation of all components working correctly

## Verification Components

### 1. Credential Format Verification

Withdrawal credentials must be validated for:
- **Length**: Exactly 32 bytes (64 hex characters)
- **Type**: First byte must be 0x00 or 0x01
- **Padding**: For 0x01 type, bytes 1-11 must be zero
- **Address**: For 0x01 type, last 20 bytes must be a valid Ethereum address

**Tool**: `fetch-withdrawal-credentials.js --decode <hex>`

### 2. Beacon Chain Data Verification

When querying live validator data:
- **API Response**: Verify API returns valid JSON with expected fields
- **Validator Status**: Check validator is in expected state
- **Credentials Match**: Ensure credentials from Beacon Chain match expected format

**Tool**: `fetch-withdrawal-credentials.js --index <number>`

### 3. Signature Verification (BLSToExecutionChange)

For validators upgrading from 0x00 to 0x01 credentials:
- **Message Format**: BLSToExecutionChange message must follow EIP-7044 specification
- **BLS Signature**: Signature must be valid for the validator's withdrawal key
- **Genesis Fork Version**: Must match the network (mainnet, goerli, etc.)

**Note**: Full BLS signature verification requires the BLS12-381 cryptographic library and is typically performed by beacon node software, not this tool.

## Verification Checklist

### Basic Verification (Offline)

- [ ] Credential hex string is exactly 64 characters (excluding 0x prefix)
- [ ] Credential type byte is 0x00 or 0x01
- [ ] For 0x01: Padding bytes (1-11) are all zeros
- [ ] For 0x01: Execution address (bytes 12-31) is valid Ethereum address
- [ ] For 0x00: Full 31-byte hash is present

### Live Beacon Chain Verification

- [ ] Beacon Chain API is accessible
- [ ] Validator exists at specified index/pubkey
- [ ] Withdrawal credentials returned match expected format
- [ ] Validator status is as expected (active, pending, exited, etc.)

### BLSToExecutionChange Signature Verification

For advanced users upgrading validators from 0x00 to 0x01:

- [ ] Have access to validator withdrawal private key
- [ ] Message includes correct validator index
- [ ] Message includes new execution address (0x01 credentials)
- [ ] Genesis fork version matches network
- [ ] BLS signature verifies against withdrawal public key
- [ ] Message is properly formatted per EIP-7044 specification

## Running Verification Tests

### 1. Test Suite

Run the comprehensive test suite:

```bash
cd contrib/devtools
node test-withdrawal-credentials.js
```

This verifies:
- Valid 0x01 credentials (with and without 0x prefix)
- Valid 0x00 credentials
- Zero address handling
- Invalid length detection
- Error handling

### 2. Demo Script

Run the demonstration script:

```bash
cd contrib/devtools
bash demo-withdrawal-credentials.sh
```

This demonstrates:
- Decoding 0x01 execution address credentials
- Decoding 0x00 BLS credentials
- Help information display

### 3. Manual Verification

Test with real examples:

```bash
# Verify a 0x01 credential
node fetch-withdrawal-credentials.js --decode 0x010000000000000000000000e16359506c028e51f16be38986ec5746251e9724

# Expected output:
# - Type: 0x01
# - Address: 0xe16359506c028e51f16be38986ec5746251e9724
# - Can Withdraw: Yes
# - Valid Format: Yes

# Verify a 0x00 credential
node fetch-withdrawal-credentials.js --decode 0x004156d2e53f1b3e0b4e8b6c9e7d3f2a1b0c9d8e7f6a5b4c3d2e1f0a9b8c7d00

# Expected output:
# - Type: 0x00
# - Can Withdraw: No
# - Requires Upgrade: Yes
```

## Integration Testing

### API Integration

To test Beacon Chain API integration (requires network access):

```bash
# Query a known validator (example index)
node fetch-withdrawal-credentials.js --index 1

# Use custom API endpoint
node fetch-withdrawal-credentials.js --index 1 --api custom-beacon-api.com
```

**Note**: Beacon Chain API queries may fail due to:
- Network/firewall restrictions
- API rate limiting
- Invalid validator index
- API service downtime

## CBSC Verification Results

### Success Criteria

A complete CBSC verification passes when:

1. ✅ **Format Check**: All test cases pass
2. ✅ **Decoding Check**: Valid credentials decode correctly
3. ✅ **Type Detection**: 0x00 and 0x01 types identified properly
4. ✅ **Address Extraction**: 0x01 credentials extract valid Ethereum addresses
5. ✅ **Error Handling**: Invalid inputs produce appropriate errors
6. ✅ **API Integration**: Beacon Chain queries work (when network available)
7. ✅ **Documentation**: All components documented and tested

### Current Status

As of this implementation:

- ✅ Credential format verification: **IMPLEMENTED & TESTED**
- ✅ Type detection (0x00/0x01): **IMPLEMENTED & TESTED**
- ✅ Address extraction: **IMPLEMENTED & TESTED**
- ✅ Error handling: **IMPLEMENTED & TESTED**
- ✅ Beacon Chain API integration: **IMPLEMENTED** (requires network)
- ⚠️  BLS signature verification: **NOT IMPLEMENTED**
  - Reason: Requires BLS12-381 cryptographic library
  - Recommendation: Use beacon node software (Lighthouse, Prysm, Teku) for signature verification
  - Documentation: Provided for reference

## BLS Signature Verification (Advanced)

For those who need to verify BLSToExecutionChange signatures, use one of these approaches:

### Option 1: Use Beacon Node Software

Beacon nodes like Lighthouse, Prysm, and Teku have built-in BLS signature verification:

```bash
# Lighthouse example
lighthouse account validator bls-to-execution-change \
  --validator-index 12345 \
  --withdrawal-credentials 0x01... \
  --execution-address 0x...
```

### Option 2: Use BLS Library

For programmatic verification, use a BLS12-381 library:

```javascript
// Example using @noble/curves (Node.js)
const { bls12_381 } = require('@noble/curves/bls12-381');

function verifyBLSSignature(message, publicKey, signature) {
    const messageHash = bls12_381.G1.hashToCurve(message);
    return bls12_381.verify(signature, messageHash, publicKey);
}
```

### Option 3: Use eth-staking-smith

The eth-staking-smith CLI tool provides BLSToExecutionChange generation and verification:

```bash
npx eth-staking-smith existing-mnemonic \
  --withdrawal_credentials 0x00... \
  --execution_address 0x...
```

## Security Considerations

### Critical Warnings

⚠️ **NEVER share your validator withdrawal private key**
⚠️ **ALWAYS verify execution addresses match your expected values**
⚠️ **VERIFY credentials on multiple independent sources before upgrading**

### Best Practices

1. **Double-check addresses**: Verify execution addresses are under your control
2. **Use offline verification**: Decode credentials offline when possible
3. **Cross-reference sources**: Verify credentials match across multiple Beacon Chain explorers
4. **Test on testnet first**: If upgrading 0x00 to 0x01, test on Goerli/Sepolia first
5. **Keep keys secure**: Withdrawal keys should be in cold storage
6. **Document changes**: Keep records of all credential changes and upgrades

## Troubleshooting

### "Invalid withdrawal credentials length"

**Problem**: Input hex string is not 64 characters

**Solution**: 
- Check for missing or extra characters
- Remove 0x prefix if included in count
- Ensure full 32 bytes are provided

### "API Error: No data returned"

**Problem**: Beacon Chain API query failed

**Solution**:
- Verify validator index/pubkey exists
- Check network connectivity
- Try alternative API endpoint with `--api` flag
- Check API rate limits

### "Unknown type"

**Problem**: Credential type byte is not 0x00 or 0x01

**Solution**:
- Verify data source is correct
- Check for data corruption
- Ensure credentials are from Ethereum Beacon Chain (not other chains)

## References

### Specifications

- [EIP-4895: Beacon chain push withdrawals](https://eips.ethereum.org/EIPS/eip-4895)
- [EIP-7044: Perpetually Valid BLSToExecutionChange](https://eips.ethereum.org/EIPS/eip-7044)
- [Ethereum Withdrawal Credentials](https://ethereum.org/en/staking/withdrawal-credentials/)

### Tools

- [Beaconcha.in](https://beaconcha.in) - Beacon Chain explorer
- [eth-staking-smith](https://www.npmjs.com/package/eth-staking-smith) - BLSToExecutionChange tool
- [Lighthouse](https://lighthouse-book.sigmaprime.io/) - Ethereum consensus client
- [Prysm](https://docs.prylabs.network/) - Ethereum consensus client
- [Teku](https://docs.teku.consensys.net/) - Ethereum consensus client

### Related Documentation

- [WITHDRAWAL_CREDENTIALS_README.md](./WITHDRAWAL_CREDENTIALS_README.md) - Main tool documentation
- [README.md](./README.md) - Developer tools overview

## License

Copyright (c) 2026 The Bitcoin Core developers
Distributed under the MIT software license, see the accompanying
file COPYING or https://opensource.org/license/mit.
