# Bitcoin Core Coin Creation Test Utility

## Overview

This test utility (`src/test/coin_utility_demo.cpp`) provides comprehensive examples of how to create, manage, and test coins (UTXOs) in Bitcoin Core. It serves as both a test suite and educational resource for understanding Bitcoin Core's coin management system.

## What is a "Coin" in Bitcoin Core?

In Bitcoin Core, a **Coin** represents a **UTXO (Unspent Transaction Output)** - the fundamental unit of value that can be spent in a transaction. Each coin contains:

- **CTxOut out**: The transaction output with value (in satoshis) and spending conditions (scriptPubKey)
- **nHeight**: The block height where the coin was created (31-bit value)
- **fCoinBase**: Flag indicating if this coin came from a coinbase transaction (1-bit value)

## File Location

```
src/test/coin_utility_demo.cpp
```

## Building and Running

### Build the Test Suite

```bash
# From the bitcoin directory
mkdir -p build && cd build
cmake -B . -S ..
cmake --build . --target test_bitcoin
```

### Run the Coin Utility Demo Tests

```bash
# Run all coin utility demos
./src/test/test_bitcoin --run_test=coin_utility_demo

# Run a specific demo
./src/test/test_bitcoin --run_test=coin_utility_demo/demo_basic_coin_creation
```

### Available Test Cases

1. **demo_basic_coin_creation** - Creating coins with different properties
2. **demo_add_coins_to_cache** - Adding coins to CCoinsViewCache
3. **demo_spend_coins** - Spending coins from the cache
4. **demo_coin_states** - Understanding coin states and properties
5. **demo_different_scripts** - Creating coins with various script types
6. **demo_cache_hierarchy** - Understanding the cache hierarchy
7. **demo_bulk_operations** - Working with multiple coins efficiently
8. **demo_memory_usage** - Tracking dynamic memory usage

## Code Examples

### Creating a Simple Coin

```cpp
#include <coins.h>
#include <primitives/transaction.h>

// Create a coin with 1 BTC at height 100
CTxOut txout;
txout.nValue = COIN;  // 100,000,000 satoshis
txout.scriptPubKey = CScript() << OP_DUP << OP_HASH160
                               << std::vector<uint8_t>(20, 0x01)
                               << OP_EQUALVERIFY << OP_CHECKSIG;

Coin coin(std::move(txout), /* height */ 100, /* is_coinbase */ false);
```

### Adding Coins to Cache

```cpp
#include <coins.h>

CCoinsView base_view;
CCoinsViewCache cache(&base_view);

// Create an outpoint (txid + output index)
COutPoint outpoint(txid, 0);

// Add the coin to the cache
cache.AddCoin(outpoint, std::move(coin), /* possible_overwrite */ false);

// Verify it exists
assert(cache.HaveCoin(outpoint));
```

### Spending a Coin

```cpp
// Spend a coin and save it for undo data
Coin spent_coin;
bool success = cache.SpendCoin(outpoint, &spent_coin);

if (success) {
    // spent_coin now contains the original coin data
    // The coin is marked as spent in the cache
    std::cout << "Spent " << spent_coin.out.nValue << " satoshis\n";
}
```

### Creating a Coinbase Coin

```cpp
// Coinbase coins require 100 confirmations before spending
Coin coinbase_coin(
    CTxOut(50 * COIN, scriptPubKey),
    /* height */ 1,
    /* is_coinbase */ true
);

const int COINBASE_MATURITY = 100;
int current_height = 150;

// Check if mature
if (current_height - coinbase_coin.nHeight >= COINBASE_MATURITY) {
    // Can spend this coinbase coin
}
```

### Working with Different Script Types

```cpp
// P2PKH (Pay to Public Key Hash)
CScript p2pkh = CScript() << OP_DUP << OP_HASH160
                          << pubkey_hash
                          << OP_EQUALVERIFY << OP_CHECKSIG;

// P2SH (Pay to Script Hash)
CScript p2sh = CScript() << OP_HASH160
                         << script_hash
                         << OP_EQUAL;

// P2WPKH (SegWit - Pay to Witness Public Key Hash)
CScript p2wpkh = CScript() << OP_0 << pubkey_hash;

// Create coins with different scripts
Coin p2pkh_coin(CTxOut(COIN, p2pkh), 100, false);
Coin p2sh_coin(CTxOut(COIN, p2sh), 100, false);
Coin p2wpkh_coin(CTxOut(COIN, p2wpkh), 100, false);
```

## Key Concepts Demonstrated

### 1. Coin Lifecycle

```
Created (UTXO) → Added to Cache → Spent → Possibly Undone (on reorg)
```

### 2. Cache Hierarchy

Bitcoin Core uses a multi-layer caching system:

```
CCoinsViewDB (LevelDB storage)
    ↑
CCoinsViewCache (L1 cache)
    ↑
CCoinsViewCache (L2 cache - for temporary changes)
```

Coins flow downward through `Flush()` operations.

### 3. Cache Entry Flags

- **DIRTY**: Entry differs from parent cache (needs flushing)
- **FRESH**: Entry doesn't exist in parent (can be deleted if spent)

### 4. Coin States

- **Unspent**: `coin.IsSpent() == false`, has valid nValue and scriptPubKey
- **Spent**: `coin.IsSpent() == true`, out.nValue == -1 (null)

### 5. Memory Management

Coins track dynamic memory usage based on scriptPubKey size:

```cpp
size_t memory = coin.DynamicMemoryUsage();
// Returns size of scriptPubKey in bytes
```

## Utility Functions Provided

### CreateTestCoin()

Creates a simple test coin with standard P2PKH script.

```cpp
Coin CreateTestCoin(CAmount value, int height, bool is_coinbase = false);
```

### CreateCoinWithScript()

Creates a coin with a custom script.

```cpp
Coin CreateCoinWithScript(
    CAmount value,
    int height,
    const CScript& script,
    bool is_coinbase = false
);
```

### CoinInfo()

Returns a detailed string describing a coin's properties.

```cpp
std::string CoinInfo(const Coin& coin, const COutPoint& outpoint);
```

## Related Files

### Core Implementation
- `src/coins.h` - Coin class definition and cache interfaces
- `src/coins.cpp` - Coin cache implementation
- `src/primitives/transaction.h` - CTxOut and COutPoint definitions

### Test Utilities
- `src/test/util/coins.h` - AddTestCoin() helper
- `src/test/coins_tests.cpp` - Comprehensive coin tests

### Validation
- `src/validation.cpp` - UpdateCoins(), ApplyTxInUndo()

## Important Constants

```cpp
COIN = 100000000          // 1 BTC in satoshis
COINBASE_MATURITY = 100   // Blocks before coinbase can be spent
```

## Common Use Cases

### Testing Transaction Validation

```cpp
// Create coins for transaction inputs
CCoinsViewCache view(&base);
for (const auto& input : tx.vin) {
    Coin coin = CreateTestCoin(COIN, 100, false);
    view.AddCoin(input.prevout, std::move(coin), false);
}

// Validate transaction can spend these coins
```

### Simulating Block Processing

```cpp
// Add outputs from new transactions
for (const auto& tx : block.vtx) {
    for (size_t i = 0; i < tx.vout.size(); i++) {
        COutPoint outpoint(tx.GetHash(), i);
        Coin coin(tx.vout[i], block_height, tx.IsCoinBase());
        cache.AddCoin(outpoint, std::move(coin), false);
    }
}

// Flush to persist changes
cache.Flush();
```

### Testing Chain Reorganization

```cpp
// Save coins before spending
std::vector<Coin> undo_data;

// Spend coins (simulating transaction)
for (const auto& input : tx.vin) {
    Coin spent;
    cache.SpendCoin(input.prevout, &spent);
    undo_data.push_back(spent);
}

// Later, restore coins (simulating reorg)
for (size_t i = 0; i < tx.vin.size(); i++) {
    cache.AddCoin(tx.vin[i].prevout, std::move(undo_data[i]), true);
}
```

## Performance Considerations

1. **Cache Locality**: Keep frequently accessed coins in higher-level caches
2. **Batch Operations**: Use `Flush()` to write multiple changes at once
3. **Memory Usage**: Monitor `DynamicMemoryUsage()` for large UTXO sets
4. **Spent Coin Cleanup**: FRESH coins are deleted immediately when spent

## Tips for Using This Utility

1. **Learning Tool**: Read through each demo test to understand coin operations
2. **Template Code**: Copy utility functions for your own tests
3. **Debugging**: Use `CoinInfo()` to print coin details during debugging
4. **Experimentation**: Modify demos to test specific scenarios

## Running Individual Demos

```bash
# Basic coin creation
./test_bitcoin --run_test=coin_utility_demo/demo_basic_coin_creation --log_level=all

# Cache operations
./test_bitcoin --run_test=coin_utility_demo/demo_add_coins_to_cache --log_level=all

# Spending coins
./test_bitcoin --run_test=coin_utility_demo/demo_spend_coins --log_level=all

# Memory tracking
./test_bitcoin --run_test=coin_utility_demo/demo_memory_usage --log_level=all
```

## Further Reading

- Bitcoin Core Developer Documentation: https://github.com/bitcoin/bitcoin/tree/master/doc
- UTXO Set Design: See `doc/design/utxo-set.md`
- Coin Database: See `src/txdb.h` and `src/txdb.cpp`
- Transaction Validation: See `src/validation.cpp`

## Contributing

When adding new coin-related functionality:

1. Add test cases to this demo suite or `coins_tests.cpp`
2. Document memory usage implications
3. Consider cache hierarchy effects
4. Test with both coinbase and regular coins

## License

Copyright (c) 2025 The Bitcoin Core developers

Distributed under the MIT software license, see the accompanying
file COPYING or http://www.opensource.org/licenses/mit-license.php.
