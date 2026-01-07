# Clitboin Brainstorming & Action Plan

This document outlines the steps to turn your fork into a functional "Clitboin" network, create a wallet, and allocate tokens.

## Phase 1: Immediate Action (The Playground)

Before making deep code changes, we should get the current codebase running in **Regression Test Mode (regtest)**. This allows you to instantly mine blocks and create wallets without connecting to the real Bitcoin network.

### 1. Build the Project
You are on Windows. You need to compile the code.
- **Option A (Recommended for Devs):** Use Visual Studio (MSVC). Open the folder in VS and look for `build-windows-msvc.md`.
- **Option B (Cross-platform):** Use WSL (Windows Subsystem for Linux) and `make`.

### 2. Start the Node
Once built, run the daemon in regtest mode:
```powershell
# In your build/src directory
./bitcoind -regtest -daemon
```

### 3. Create a Wallet
Clitboin (Bitcoin Core) has a built-in wallet. Create one named "mywallet":
```powershell
./bitcoin-cli -regtest createwallet "mywallet"
```

### 4. Allocate Tokens (Mining)
In regtest, you can "allocate" tokens to yourself by mining blocks.
- **Why?** Coinbase rewards (50 coins) take 100 blocks to mature (become spendable).
- **Action:** Mine 101 blocks to get your first 50 coins.
```powershell
./bitcoin-cli -regtest -generate 101
```
- **Check Balance:**
```powershell
./bitcoin-cli -regtest getbalance
```
You should see `50.00000000`.

## Phase 2: The Fork (Rebranding Clitboin)

To make this a *real* separate coin, we need to change the network parameters.

### Files to Edit
1.  **`src/kernel/chainparams.cpp`**:
    *   **Genesis Block**: Change `pszTimestamp` to something current (e.g., "Clitboin launched on [Date]!").
    *   **Magic Bytes (`pchMessageStart`)**: Change these 4 bytes so Clitboin nodes don't talk to Bitcoin nodes.
    *   **Address Prefixes**: Change `base58Prefixes` (e.g., start addresses with 'C' instead of '1').
2.  **`src/chainparamsbase.cpp`**:
    *   **Ports**: Change `nDefaultPort` (e.g., 8333 -> 9333) and RPC port (8332 -> 9332).

### Genesis Block Generation
Changing the timestamp invalidates the Genesis Block. You will need to find a new `nNonce` and `nTime` that satisfies the difficulty. We can write a script for this later.

## Phase 3: Block Explorer

### Option A: Built-in (CLI)
Use the command line to inspect blocks:
```powershell
./bitcoin-cli -regtest getblockchaininfo
./bitcoin-cli -regtest getblock <blockhash>
```

### Option B: Mini Web Explorer (Included)
I have created a simple script `simple_explorer.py` in the root folder.
1.  Install Python dependencies: `pip install requests flask`
2.  Run the script: `python simple_explorer.py`
3.  Open `http://localhost:5000` in your browser.
    *   *Note: You need to set `rpcuser` and `rpcpassword` in your `bitcoin.conf` for this to work.*

### Option C: Full Explorer
For a production explorer, we would deploy [Iquidus](https://github.com/iquidus/explorer) or [Miningcore](https://github.com/coinfoundry/miningcore).

---

**Next Steps:**
1.  Are you able to compile the code?
2.  Shall we proceed with editing `src/kernel/chainparams.cpp` to rebrand the coin now?
