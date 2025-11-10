# Custom Coin Types for Bitcoin Core

## Overview

This implementation provides **extended coin types** that build upon Bitcoin Core's base `Coin` class (UTXO). These custom types are designed for:

- **Research and experimentation** with coin metadata
- **Asset/token prototyping** (colored coins)
- **Time-locked UTXO demonstrations**
- **UTXO set analytics and tracking**
- **Educational purposes** to understand coin architecture

> **⚠️ WARNING**: These are for **testing and educational purposes only**. Not intended for production use in Bitcoin Core.

## Architecture

### File Structure

```
src/test/custom_coin_types.h       - Custom coin type definitions
src/test/custom_coin_tests.cpp     - Comprehensive test suite
```

### Coin Type Hierarchy

```
Base Coin (Bitcoin Core UTXO)
    ↓
┌───────────────┬──────────────┬──────────────┬───────────────┐
│ ExtendedCoin  │ ColoredCoin  │ TimeLockCoin │ AnnotatedCoin │
│               │              │              │               │
│ + metadata    │ + asset_id   │ + time_lock  │ + analytics   │
│ + tags        │ + properties │ + lock_type  │ + origin      │
│ + tracking    │ + issuer     │              │ + category    │
└───────────────┴──────────────┴──────────────┴───────────────┘
                    ↓
                MultiCoin (polymorphic variant)
```

## Custom Coin Types

### 1. ExtendedCoin

**Purpose**: Add metadata and tracking to coins

**Features**:
- Timestamp tracking (creation time)
- User-defined tags for categorization
- Arbitrary metadata storage
- Unique tracking ID for analytics
- Full serialization support

**Use Cases**:
- Wallet coin categorization (hot wallet, cold storage, etc.)
- Transaction flow analysis
- Coin provenance tracking
- Custom labeling systems

**Example**:

```cpp
#include <test/custom_coin_types.h>

// Create a tagged coin
Coin base = CreateBaseCoin(5 * COIN, 200, false);
ExtendedCoin ext_coin(std::move(base), "cold_storage");

// Add metadata
ext_coin.SetMetadata("Owner: Alice, Purpose: Long-term savings");

// Access extended properties
std::string tag = ext_coin.GetTag();           // "cold_storage"
uint64_t created = ext_coin.GetTimestamp();    // Unix timestamp
uint256 id = ext_coin.GetTrackingId();         // Unique ID
```

### 2. ColoredCoin

**Purpose**: Represent assets/tokens using UTXO model

**Features**:
- Asset ID for unique asset identification
- Separate asset amount from BTC value
- Multiple asset types (Token, NFT, Bond, Equity, Custom)
- Optional issuer identification
- Key-value property storage
- Full serialization support

**Asset Types**:
- `STANDARD` - Regular BTC (no coloring)
- `TOKEN` - Fungible tokens
- `NFT` - Non-fungible tokens
- `BOND` - Debt instruments
- `EQUITY` - Equity shares
- `CUSTOM` - User-defined assets

**Use Cases**:
- Colored coin implementations
- Token/asset representation
- NFT tracking
- Synthetic asset research
- Multi-asset UTXO systems

**Example - Fungible Token**:

```cpp
// Create a token with 1,000,000 units
uint256 asset_id = uint256S("0x1234...abcd");
Coin base = CreateBaseCoin(1000, 100, false);  // Dust limit for UTXO

ColoredCoin token(
    std::move(base),
    asset_id,
    1000000,  // Asset amount
    ColoredCoin::AssetType::TOKEN
);

// Set token properties
token.SetProperty("name", "MyToken");
token.SetProperty("symbol", "MTK");
token.SetProperty("decimals", "8");

// Set issuer
uint256 issuer_id = uint256S("0xabcd...1234");
token.SetIssuer(issuer_id);

// Query properties
auto name = token.GetProperty("name");  // Optional<string> = "MyToken"
CAmount btc_value = token.GetBTCValue();     // 1000 satoshis
uint64_t tokens = token.GetAssetAmount();    // 1000000 tokens
```

**Example - NFT**:

```cpp
// Create an NFT (amount always = 1)
uint256 nft_id = uint256S("0xdead...beef");
ColoredCoin nft(
    CreateBaseCoin(10000, 100, false),
    nft_id,
    1,  // NFTs have amount = 1
    ColoredCoin::AssetType::NFT
);

// Set NFT metadata
nft.SetProperty("name", "CryptoPunk #1234");
nft.SetProperty("image", "ipfs://QmXyZ...");
nft.SetProperty("rarity", "legendary");
nft.SetProperty("collection", "CryptoPunks");
```

### 3. TimeLockCoin

**Purpose**: Demonstrate time-based spending restrictions

**Features**:
- Multiple lock types (absolute/relative, time/height)
- Spendability checking
- Lock expiration tracking
- Compatible with CSV/CLTV concepts

**Lock Types**:
- `NONE` - No time lock
- `ABSOLUTE_TIME` - Locked until specific Unix timestamp
- `ABSOLUTE_HEIGHT` - Locked until specific block height
- `RELATIVE_TIME` - Locked for duration after confirmation
- `RELATIVE_HEIGHT` - Locked for number of blocks after confirmation (CSV-like)

**Use Cases**:
- Time-locked savings/vesting
- Payment channels with timeouts
- Escrow with time conditions
- Covenant research
- CheckSequenceVerify (CSV) demonstrations

**Example - Absolute Time Lock**:

```cpp
// Lock until May 2033
uint64_t unlock_timestamp = 2000000000;
TimeLockCoin locked(
    CreateBaseCoin(10 * COIN, 100, false),
    TimeLockCoin::LockType::ABSOLUTE_TIME,
    unlock_timestamp
);

// Check if spendable
uint64_t current_time = std::time(nullptr);
int current_height = 1000;

if (locked.IsSpendable(current_time, current_height)) {
    // Can spend the coin
}

uint64_t when = locked.GetUnlockTime();  // 2000000000
```

**Example - Relative Height Lock (CSV-style)**:

```cpp
// Lock for 144 blocks (~1 day) after confirmation
TimeLockCoin csv_locked(
    CreateBaseCoin(5 * COIN, 100, false),  // Created at height 100
    TimeLockCoin::LockType::RELATIVE_HEIGHT,
    144  // Number of blocks to wait
);

// Spendable at height 100 + 144 = 244
bool spendable_at_243 = csv_locked.IsSpendable(0, 243);  // false
bool spendable_at_244 = csv_locked.IsSpendable(0, 244);  // true

int unlock_height = csv_locked.GetUnlockHeight();  // 244
```

### 4. AnnotatedCoin

**Purpose**: Rich analytics and research metadata

**Features**:
- Origin transaction tracking
- Creation timestamp
- Address labeling
- Category classification
- Spend attempt counting
- Dust/change detection
- Full analytics data structure

**Use Cases**:
- UTXO set analysis
- Chain analytics research
- Wallet behavior studies
- Dust tracking
- Change detection algorithms

**Example**:

```cpp
Txid origin_tx = Txid::FromUint256(InsecureRand256());
AnnotatedCoin annotated(
    CreateBaseCoin(2 * COIN, 150, false),
    origin_tx,
    3  // Output index in origin transaction
);

// Set analytics metadata
annotated.SetCategory("mining_reward");
annotated.SetAddressLabel("Mining Pool XYZ");
annotated.MarkAsDust(false);
annotated.MarkAsChange(false);

// Access analytics
const auto& analytics = annotated.GetAnalytics();
std::cout << "Origin: " << analytics.origin_tx.ToString() << std::endl;
std::cout << "Output index: " << analytics.origin_output_index << std::endl;
std::cout << "Category: " << analytics.category << std::endl;
std::cout << "Created: " << analytics.creation_time << std::endl;
```

### 5. MultiCoin

**Purpose**: Polymorphic container for any coin type

**Type**: `std::variant<Coin, ExtendedCoin, ColoredCoin, TimeLockCoin, AnnotatedCoin>`

**Use Cases**:
- Mixed coin collections
- Polymorphic coin handling
- Generic coin utilities
- Type-agnostic operations

**Example**:

```cpp
#include <test/custom_coin_types.h>

std::vector<MultiCoin> coin_collection;

// Add different coin types
coin_collection.push_back(CreateBaseCoin(COIN, 100, false));
coin_collection.push_back(ExtendedCoin(CreateBaseCoin(2 * COIN, 100, false), "tag"));
coin_collection.push_back(ColoredCoin(/* ... */));

// Use helper functions for any type
for (const auto& coin : coin_collection) {
    bool spent = CustomCoinHelpers::IsSpent(coin);
    CAmount value = CustomCoinHelpers::GetValue(coin);
    const Coin& base = CustomCoinHelpers::GetBaseCoin(coin);
}
```

## Building and Testing

### Build the Test Suite

```bash
cd /home/user/bitcoin
mkdir -p build && cd build
cmake -B . -S ..
cmake --build . --target test_bitcoin
```

### Run All Custom Coin Tests

```bash
./src/test/test_bitcoin --run_test=custom_coin_tests
```

### Run Specific Tests

```bash
# ExtendedCoin tests
./src/test/test_bitcoin --run_test=custom_coin_tests/extended_coin_basic
./src/test/test_bitcoin --run_test=custom_coin_tests/extended_coin_tagging

# ColoredCoin tests
./src/test/test_bitcoin --run_test=custom_coin_tests/colored_coin_tokens
./src/test/test_bitcoin --run_test=custom_coin_tests/colored_coin_nft

# TimeLockCoin tests
./src/test/test_bitcoin --run_test=custom_coin_tests/timelock_coin_absolute_time
./src/test/test_bitcoin --run_test=custom_coin_tests/timelock_coin_relative_height

# AnnotatedCoin tests
./src/test/test_bitcoin --run_test=custom_coin_tests/annotated_coin_analytics

# Serialization tests
./src/test/test_bitcoin --run_test=custom_coin_tests/serialization_extended_coin
./src/test/test_bitcoin --run_test=custom_coin_tests/serialization_colored_coin

# MultiCoin tests
./src/test/test_bitcoin --run_test=custom_coin_tests/multicoin_variant
```

## Test Coverage

The test suite (`src/test/custom_coin_tests.cpp`) includes **15 comprehensive test cases**:

1. **extended_coin_basic** - Basic ExtendedCoin functionality
2. **extended_coin_tagging** - Multiple coins with different tags
3. **colored_coin_tokens** - Fungible token creation
4. **colored_coin_nft** - NFT creation and properties
5. **colored_coin_asset_types** - Different asset type handling
6. **timelock_coin_absolute_time** - Absolute timestamp locks
7. **timelock_coin_absolute_height** - Absolute block height locks
8. **timelock_coin_relative_time** - Relative time duration locks
9. **timelock_coin_relative_height** - Relative block height locks (CSV-like)
10. **annotated_coin_analytics** - Analytics tracking
11. **annotated_coin_utxo_analysis** - UTXO set analysis
12. **multicoin_variant** - Polymorphic coin handling
13. **serialization_extended_coin** - ExtendedCoin serialization
14. **serialization_colored_coin** - ColoredCoin serialization
15. **memory_usage_comparison** - Memory usage tracking

## Advanced Usage Examples

### Colored Coin Token Transfer Simulation

```cpp
// Sender has a colored coin with tokens
ColoredCoin sender_coin(
    CreateBaseCoin(1000, 100, false),
    token_asset_id,
    1000000,  // 1M tokens
    ColoredCoin::AssetType::TOKEN
);

// Create output to receiver (500k tokens)
ColoredCoin receiver_coin(
    CreateBaseCoin(1000, 101, false),
    token_asset_id,
    500000,  // 500k tokens
    ColoredCoin::AssetType::TOKEN
);

// Change back to sender (500k tokens)
ColoredCoin change_coin(
    CreateBaseCoin(1000, 101, false),
    token_asset_id,
    500000,  // 500k tokens
    ColoredCoin::AssetType::TOKEN
);

// Verify conservation: 1M = 500k + 500k
assert(sender_coin.GetAssetAmount() ==
       receiver_coin.GetAssetAmount() + change_coin.GetAssetAmount());
```

### Vesting Schedule with TimeLockCoins

```cpp
// Create vesting schedule: unlock 25% every 6 months
std::vector<TimeLockCoin> vesting_coins;

CAmount total_amount = 100 * COIN;
CAmount quarter = total_amount / 4;
int creation_height = 100;

for (int i = 0; i < 4; i++) {
    // Each coin unlocks after (i+1) * 26280 blocks (~6 months)
    TimeLockCoin vested(
        CreateBaseCoin(quarter, creation_height, false),
        TimeLockCoin::LockType::RELATIVE_HEIGHT,
        (i + 1) * 26280  // blocks per 6 months
    );
    vesting_coins.push_back(std::move(vested));
}

// Check vesting at different heights
int current_height = creation_height + 30000;  // ~7 months later
for (const auto& coin : vesting_coins) {
    if (coin.IsSpendable(0, current_height)) {
        std::cout << "Coin unlocked!\n";
    }
}
```

### UTXO Set Analytics with AnnotatedCoins

```cpp
// Analyze a set of UTXOs
std::map<std::string, CAmount> category_totals;
std::map<std::string, int> category_counts;

std::vector<AnnotatedCoin> utxo_set = /* ... */;

for (const auto& coin : utxo_set) {
    const auto& analytics = coin.GetAnalytics();
    std::string category = analytics.category;

    category_totals[category] += coin.base_coin.out.nValue;
    category_counts[category]++;
}

// Print results
for (const auto& [category, total] : category_totals) {
    std::cout << category << ": "
              << category_counts[category] << " coins, "
              << total << " satoshis\n";
}
```

## Serialization Support

All custom coin types support full serialization/deserialization:

```cpp
// Serialize
ExtendedCoin original(CreateBaseCoin(COIN, 100, false), "test");
DataStream ss{};
ss << original;

// Deserialize
ExtendedCoin restored;
ss >> restored;

// Properties preserved
assert(restored.GetValue() == original.GetValue());
assert(restored.GetTag() == original.GetTag());
```

## Memory Usage Considerations

Each custom coin type adds overhead to the base `Coin`:

| Type | Additional Memory |
|------|------------------|
| `Coin` | ~25 bytes + scriptPubKey size |
| `ExtendedCoin` | + ~100 bytes (tag, metadata, tracking_id, timestamp) |
| `ColoredCoin` | + ~64 bytes + properties map |
| `TimeLockCoin` | + ~24 bytes (lock info) |
| `AnnotatedCoin` | + ~120 bytes (analytics struct) |

Monitor memory usage:

```cpp
size_t memory = coin.DynamicMemoryUsage();
```

## Integration with Existing Code

Custom coins wrap the base `Coin` class and maintain compatibility:

```cpp
ExtendedCoin ext_coin(/* ... */);

// Access base coin
Coin& base = ext_coin.base_coin;

// Use with CCoinsViewCache
CCoinsViewCache cache(&base_view);
COutPoint outpoint(txid, 0);

// Store the base coin
cache.AddCoin(outpoint, std::move(ext_coin.base_coin), false);
```

## Research Applications

### 1. Colored Coins Research
Study asset representation on UTXO blockchains without modifying consensus.

### 2. Covenant Prototypes
Experiment with time locks and spending conditions.

### 3. UTXO Set Analysis
Analyze UTXO composition, dust, change detection.

### 4. Privacy Research
Track coin flows and analyze anonymity sets.

### 5. Layer 2 Prototypes
Model state channels, vaults, and other Layer 2 constructions.

## Limitations

- **Not consensus-compatible**: These types exist only in test code
- **No network support**: Cannot be used in actual transactions
- **Memory overhead**: Additional metadata increases memory usage
- **Educational only**: Not audited or designed for production

## Contributing

When extending custom coin types:

1. Add new types to `src/test/custom_coin_types.h`
2. Implement `SERIALIZE_METHODS` for persistence
3. Add comprehensive tests to `src/test/custom_coin_tests.cpp`
4. Document use cases and examples
5. Consider memory usage implications

## Related Files

- `src/coins.h` - Base Coin class
- `src/coins.cpp` - Coin cache implementation
- `src/test/coins_tests.cpp` - Base coin tests
- `src/test/coin_utility_demo.cpp` - Coin creation utilities

## Further Reading

- **Colored Coins**: https://en.bitcoin.it/wiki/Colored_Coins
- **Covenants**: Bitcoin covenant proposals and research
- **UTXO Set**: Bitcoin Core UTXO set design documentation
- **Timelocks**: BIP 65 (CHECKLOCKTIMEVERIFY) and BIP 112 (CHECKSEQUENCEVERIFY)

## License

Copyright (c) 2025 The Bitcoin Core developers

Distributed under the MIT software license, see the accompanying
file COPYING or http://www.opensource.org/licenses/mit-license.php.
