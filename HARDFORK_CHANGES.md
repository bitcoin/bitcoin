# Hardfork Changes - Lightweight Bitcoin

## Summary
This hardfork removes all test networks and optimizes for lightweight blockchain storage.

## Changes Made

### 1. Removed Test Networks
- ❌ TESTNET (testnet3) - REMOVED
- ❌ TESTNET4 - REMOVED  
- ❌ SIGNET - REMOVED
- ❌ REGTEST - REMOVED
- ✅ MAIN - ONLY supported network

### 2. Network Configuration Files Modified
- `src/chainparamsbase.cpp` - Removed all testnet/signet/regtest options
- `src/chainparams.cpp` - Simplified to support MAIN only

### 3. Blockchain Optimization
For lightweight blocks and minimal data:

**Default Configuration (should be set in bitcoin.conf):**
```ini
# Enable aggressive pruning - keep only 5 GB
prune=5120

# Reduce block download bandwidth
maxuploadtarget=100

# Minimal mempool size
maxmempool=10

# Reduce database cache (low memory usage)
dbcache=50

# Only download headers initially
blockonly=1

# Reduce peer connections
maxconnections=8
```

### 4. Block Weight Optimization
- Maximum block size still limited by consensus rules
- But with pruning enabled, old blocks are automatically deleted
- Network syncs much faster with fewer GB downloaded

### 5. Faucet Information
- No built-in faucets
- Use external faucet application if needed
- Or use mining (PoW) to generate initial coins

## Running the Hardfork

```bash
# Compile
./autogen.sh
./configure
make -j4

# Run with minimal data
./src/bitcoind -datadir=.bitcoin_light

# Or create ~/.bitcoin/bitcoin.conf with settings above
./src/bitcoind
```

## Note
Any attempt to use testnet, signet, or regtest will throw an error:
```
ERROR: Only MAIN network is supported. Test networks disabled.
```
