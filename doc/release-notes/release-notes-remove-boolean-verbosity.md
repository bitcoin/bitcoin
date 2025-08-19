# RPC Changes

## Boolean verbosity parameters removed

The following RPC methods no longer accept boolean values for their verbosity parameters:

- `getblock`
- `getrawtransaction` 
- `getrawmempool`

**Before (deprecated, now removed):**
```bash
bitcoin-cli getblock <blockhash> true   # Error
bitcoin-cli getblock <blockhash> false  # Error
bitcoin-cli getrawtransaction <txid> true   # Error
bitcoin-cli getrawtransaction <txid> false  # Error  
bitcoin-cli getrawmempool true   # Error
bitcoin-cli getrawmempool false  # Error
```

**Use integers instead:**
```bash
bitcoin-cli getblock <blockhash> 1   # Verbose JSON format
bitcoin-cli getblock <blockhash> 0   # Raw hex format
bitcoin-cli getrawtransaction <txid> 1   # Verbose JSON format
bitcoin-cli getrawtransaction <txid> 0   # Raw hex format
bitcoin-cli getrawmempool 1   # Verbose JSON format  
bitcoin-cli getrawmempool 0   # List of transaction IDs
```

**Breaking Change:** Any scripts or applications using boolean verbosity parameters will need to be updated to use integer values:
- `false` → `0`
- `true` → `1`

This change improves consistency across RPC methods and enables support for additional verbosity levels in the future.