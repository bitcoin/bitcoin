# create-onchain-agent

CLI tool to quickly scaffold onchain agent projects with best practices built-in.

## Usage

Create a new onchain agent project using npm:

```bash
npm create onchain-agent@latest
```

Or using npx:

```bash
npx create-onchain-agent@latest
```

Or using yarn:

```bash
yarn create onchain-agent
```

## What You Get

When you run `create-onchain-agent`, you'll get a fully configured Next.js project with:

- 🤖 **AI Agent Interface** - Ready-to-use chat interface for your agent
- 👛 **Wallet Integration** - Coinbase Smart Wallet support with Wagmi
- ⛓️ **Multi-chain Support** - Base and Base Sepolia configured out of the box
- 📦 **OnchainKit** - Full OnchainKit integration for blockchain interactions
- 🎨 **Modern UI** - Clean, responsive interface with CSS modules
- 🔐 **Security First** - Environment variable management and best practices
- 📝 **TypeScript** - Fully typed for better development experience
- 🧪 **ESLint & Prettier** - Code quality and formatting tools configured

## Interactive Setup

The CLI will prompt you for:

1. **Project name** - Name your agent project
2. **API Key** (optional) - Your Coinbase Developer Platform API key
3. **Usage data** - Whether to share anonymous usage data

## Project Structure

```
my-onchain-agent/
├── app/
│   ├── components/
│   │   ├── ConnectButton.tsx       # Wallet connection UI
│   │   └── AgentInterface.tsx      # Agent chat interface
│   ├── layout.tsx                  # Root layout
│   ├── page.tsx                    # Home page
│   ├── rootProvider.tsx            # Providers (Wagmi, OnchainKit)
│   ├── walletConnectors.ts         # Wallet configuration
│   └── globals.css                 # Global styles
├── public/                         # Static assets
├── .env                            # Environment variables
├── .gitignore                      # Git ignore rules
├── eslint.config.mjs              # ESLint configuration
├── next.config.ts                  # Next.js configuration
├── package.json                    # Dependencies
├── tsconfig.json                   # TypeScript configuration
└── README.md                       # Project documentation
```

## Quick Start

After creating your project:

```bash
cd my-onchain-agent
npm run dev
```

Then open [http://localhost:3000](http://localhost:3000) to see your agent.

## Building Your Agent

### 1. Implement Agent Logic

Edit `app/components/AgentInterface.tsx` to add your agent's capabilities:

```typescript
const handleSubmit = async (e: React.FormEvent) => {
  e.preventDefault();
  
  // Your agent logic here
  // Examples:
  // - Call smart contracts
  // - Query blockchain data
  // - Execute transactions
  // - Integrate with AI/ML models
};
```

### 2. Add Smart Contract Interactions

Use OnchainKit and Wagmi to interact with smart contracts:

```typescript
import { useWriteContract } from 'wagmi';

const { writeContract } = useWriteContract();

// Call a smart contract
await writeContract({
  address: '0x...',
  abi: [...],
  functionName: 'transfer',
  args: [recipient, amount],
});
```

### 3. Configure Additional Chains

Edit `app/walletConnectors.ts` to support more chains:

```typescript
import { mainnet, polygon, optimism } from 'wagmi/chains';

export const config = createConfig({
  chains: [base, baseSepolia, mainnet, polygon, optimism],
  // ...
});
```

## Example Agent Use Cases

- **Trading Agent** - Execute automated trading strategies on DEXs
- **Portfolio Manager** - Monitor and rebalance crypto portfolios
- **NFT Bot** - Automated NFT minting, buying, and selling
- **DeFi Assistant** - Help users navigate DeFi protocols
- **Analytics Agent** - Provide insights on blockchain data
- **Social Agent** - Interact with onchain social protocols

## Requirements

- Node.js >= 18.0.0
- npm >= 9.0.0

## Environment Variables

The generated project uses the following environment variables:

- `NEXT_PUBLIC_ONCHAINKIT_API_KEY` - Your Coinbase Developer Platform API key
- `NEXT_PUBLIC_PROJECT_NAME` - Your project name
- `NEXT_PUBLIC_CHAIN_ID` - (Optional) Override default chain

Get your API key at [https://portal.cdp.coinbase.com/](https://portal.cdp.coinbase.com/)

## Resources

- [OnchainKit Documentation](https://docs.base.org/onchainkit)
- [Base Documentation](https://docs.base.org)
- [Wagmi Documentation](https://wagmi.sh)
- [Next.js Documentation](https://nextjs.org/docs)
- [Coinbase Developer Platform](https://portal.cdp.coinbase.com/)

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

MIT License - see LICENSE file for details

## Support

For issues and questions:
- Open an issue on GitHub
- Check the [OnchainKit documentation](https://docs.base.org/onchainkit)
- Visit the [Base Discord](https://base.org/discord)
