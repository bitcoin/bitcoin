# Onchain Agent

This is an AI-powered blockchain agent project bootstrapped with [`create-onchain-agent`](https://www.npmjs.com/package/create-onchain-agent).

## Getting Started

First, install dependencies:

```bash
npm install
# or
yarn install
# or
pnpm install
```

Next, run the development server:

```bash
npm run dev
# or
yarn dev
# or
pnpm dev
```

Open [http://localhost:3000](http://localhost:3000) with your browser to see your onchain agent in action.

## Features

- 🤖 **AI Agent Interface** - Chat-based interface for interacting with your agent
- 👛 **Wallet Connection** - Connect with Coinbase Smart Wallet
- ⛓️ **Blockchain Integration** - Built-in support for Base and Base Sepolia
- 🔐 **Secure by Default** - Best practices for wallet and API key management
- 📦 **OnchainKit Integration** - Powered by Coinbase's OnchainKit

## Project Structure

```
.
├── app/
│   ├── components/       # React components
│   │   ├── ConnectButton.tsx
│   │   └── AgentInterface.tsx
│   ├── layout.tsx        # Root layout
│   ├── page.tsx          # Home page
│   ├── rootProvider.tsx  # Providers setup
│   └── walletConnectors.ts # Wallet configuration
├── public/              # Static assets
├── .env                 # Environment variables
└── package.json         # Dependencies
```

## Customizing Your Agent

### 1. Update Agent Logic

Edit `app/components/AgentInterface.tsx` to implement your agent's behavior:

```typescript
const handleSubmit = async (e: React.FormEvent) => {
  // Add your agent logic here
  // Examples:
  // - Call smart contracts
  // - Query blockchain data
  // - Execute transactions
  // - Integrate with AI models
};
```

### 2. Configure Environment Variables

Update `.env` with your API keys and configuration:

```env
NEXT_PUBLIC_ONCHAINKIT_API_KEY="your-api-key"
NEXT_PUBLIC_PROJECT_NAME="your-agent-name"
```

### 3. Add More Chains

Edit `app/walletConnectors.ts` to add support for more chains:

```typescript
import { mainnet, polygon, optimism } from "wagmi/chains";

export const config = createConfig({
  chains: [base, baseSepolia, mainnet, polygon, optimism],
  // ...
});
```

## Building Agents

Here are some ideas for what you can build:

- **Trading Agent** - Automated trading strategies on DEXs
- **Portfolio Manager** - Monitor and rebalance crypto portfolios
- **NFT Bot** - Mint, buy, or sell NFTs based on criteria
- **Analytics Agent** - Provide insights on blockchain data
- **Social Agent** - Interact with social protocols like Farcaster
- **DeFi Assistant** - Help users navigate DeFi protocols

## Learn More

To learn more about the tools used in this project:

- [OnchainKit Documentation](https://docs.base.org/onchainkit) - Learn about OnchainKit features
- [Base Documentation](https://docs.base.org) - Learn about Base blockchain
- [Wagmi Documentation](https://wagmi.sh) - React Hooks for Ethereum
- [Next.js Documentation](https://nextjs.org/docs) - Next.js framework

## Deploy on Vercel

The easiest way to deploy your agent is to use [Vercel](https://vercel.com):

[![Deploy with Vercel](https://vercel.com/button)](https://vercel.com/new)

Remember to set your environment variables in the Vercel dashboard.

## Security

- Never commit your `.env` file
- Keep your API keys secure
- Review all transactions before signing
- Test thoroughly on testnet before mainnet deployment

## License

This project is licensed under the MIT License.
