# Welcome to Clitboin!

The code has been successfully rebranded from Bitcoin Core.

## Changes Applied
- **Genesis Block**: "Clitboin 2026: The Coin for the People" (Mined with Nonce 2)
- **Ports**: 9333 (P2P), 9332 (RPC)
- **Magic Bytes**: `c1 17 b0 11`
- **Address Prefix**: 'C' (approximate, derived from version 28)
- **Difficulty**: Mainnet difficulty lowered to minimum (CPU mining possible).

## How to Build & Run

### 1. Build
Use the standard build instructions for Windows (Visual Studio or MinGW).
See `doc/build-windows.md`.

### 2. Run (Mainnet)
Since we lowered the difficulty, you can run on mainnet directly!
```powershell
./bitcoind -daemon
```

### 3. Mine Tokens
You can mine directly on mainnet with CPU:
```powershell
./bitcoin-cli createwallet "mywallet"
./bitcoin-cli -generate 101
```

### 4. Explorer
Run the included python explorer:
```powershell
pip install requests flask
python simple_explorer.py
```
*Note: You might need to update `simple_explorer.py` ports to 9332 (Mainnet RPC) instead of 18443 (Regtest).*

## Configuration
Use `clitboin.conf` for easy settings.
