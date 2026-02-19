# WETH Smart Contract Integration

This document describes the WETH (Wrapped Ether) smart contract integration in the Bitcoin OnChain App.

## Overview

The application now includes full integration with the WETH smart contract on Ethereum Mainnet, allowing users to:
- Deposit ETH to receive WETH
- Withdraw WETH to receive ETH
- Transfer WETH to yaketh.eth ENS address

## Contract Details

- **Contract Address**: `0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2`
- **Network**: Ethereum Mainnet (Chain ID: 1)
- **Reference**: [Etherscan](https://etherscan.io/address/0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2#writeContract#F1)

## Features

### 1. Deposit (ETH → WETH)
Convert your ETH to WETH by depositing into the contract. This calls the `deposit()` function (Function F1).

**How it works:**
- Enter the amount of ETH you want to deposit
- Click "Deposit" button
- Approve the transaction in your wallet
- Receive the equivalent amount of WETH

### 2. Withdraw (WETH → ETH)
Convert your WETH back to ETH by withdrawing from the contract. This calls the `withdraw()` function.

**How it works:**
- Enter the amount of WETH you want to withdraw
- Click "Withdraw" button
- Approve the transaction in your wallet
- Receive the equivalent amount of ETH

### 3. Transfer to yaketh.eth
Transfer WETH directly to the yaketh.eth ENS address. This calls the `transfer()` function.

**How it works:**
- The app automatically resolves yaketh.eth to its Ethereum address
- Enter the amount of WETH you want to transfer
- Click "Transfer to yaketh.eth" button
- Approve the transaction in your wallet
- WETH is sent to the yaketh.eth address

## Technical Implementation

### Files

1. **`app/contracts/weth.ts`** - WETH contract constants and ABI
2. **`app/components/WETHInteraction.tsx`** - React component for WETH interactions
3. **`app/components/WETHInteraction.module.css`** - Component styles
4. **`app/page.tsx`** - Main page with WETH integration
5. **`app/rootProvider.tsx`** - Updated to support Ethereum Mainnet

### Dependencies

- **viem** (^2.31.6) - Ethereum interactions
- **wagmi** (^2.16.3) - React hooks for Ethereum
- **@coinbase/onchainkit** (latest) - OnchainKit components

### Hooks Used

- `useAccount` - Get connected wallet address and chain
- `useReadContract` - Read WETH balance
- `useWriteContract` - Execute deposit, withdraw, and transfer
- `useWaitForTransactionReceipt` - Wait for transaction confirmation
- `useEnsAddress` - Resolve yaketh.eth ENS name to address

## Usage

### Prerequisites

1. Install dependencies:
   ```bash
   npm install
   ```

2. Configure environment variables (optional):
   ```bash
   NEXT_PUBLIC_PROJECT_NAME="Your App Name"
   NEXT_PUBLIC_ONCHAINKIT_API_KEY="your-api-key"
   ```

3. Run the development server:
   ```bash
   npm run dev
   ```

4. Open [http://localhost:3000](http://localhost:3000)

### Connecting Your Wallet

1. Click the "Wallet" button in the header
2. Connect your Coinbase Smart Wallet or other supported wallet
3. Ensure you're on Ethereum Mainnet (the app will prompt you to switch if not)

### Making Transactions

1. **Check your WETH balance** - Displayed at the top of the WETH interaction section
2. **Choose an action** - Deposit, Withdraw, or Transfer
3. **Enter the amount** - Specify how much ETH/WETH
4. **Submit the transaction** - Click the button and approve in your wallet
5. **Wait for confirmation** - The transaction hash and status will be displayed

## ENS Integration

The application uses ENS (Ethereum Name Service) to resolve `yaketh.eth` to its corresponding Ethereum address. This happens automatically when the component loads.

**ENS Configuration:**
- Domain: `yaketh.eth`
- Chain: Ethereum Mainnet
- Resolution: Automatic via wagmi's `useEnsAddress` hook

## Security Notes

⚠️ **Important Security Considerations:**

1. **Mainnet Transactions** - This interacts with real contracts on Ethereum Mainnet. All transactions cost real ETH in gas fees.
2. **Wallet Security** - Never share your private keys or seed phrases
3. **Transaction Verification** - Always verify transaction details before approving
4. **Contract Address** - Verify the WETH contract address matches: `0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2`
5. **ENS Resolution** - The app resolves yaketh.eth automatically, but you can verify the resolved address before transferring

## Troubleshooting

### "Please connect your wallet"
- Click the Wallet button in the header and connect your wallet

### "Please switch to Ethereum Mainnet"
- Your wallet is on the wrong network
- Switch to Ethereum Mainnet in your wallet
- Or click any prompt from the app to switch networks

### "Resolving ENS address..."
- The app is resolving yaketh.eth to an Ethereum address
- This should complete within a few seconds
- If it persists, check your internet connection

### Transaction Fails
- Ensure you have enough ETH for gas fees
- For withdrawals, ensure you have enough WETH balance
- Check that you're on the correct network (Ethereum Mainnet)

## Development

### Running Tests

```bash
npm run lint
npx tsc --noEmit
```

### Building for Production

```bash
npm run build
npm start
```

## Related Documentation

- [WETH Contract on Etherscan](https://etherscan.io/address/0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2)
- [OnchainKit Documentation](https://docs.base.org/onchainkit)
- [Wagmi Documentation](https://wagmi.sh)
- [Viem Documentation](https://viem.sh)

## License

MIT License - See [COPYING](../COPYING) for details

---

**Repository**: kushmanmb-org/bitcoin  
**ENS**: kushmanmb.eth, yaketh.eth  
**Last Updated**: 2026-02-19
