# Create Onchain Agent - Setup and Usage Guide

## Overview

The `create-onchain-agent` package is a CLI scaffolding tool that allows developers to quickly bootstrap onchain agent projects with best practices built-in. It's similar to `create-onchain` but specifically designed for building AI-powered blockchain agents.

## Quick Start

### Using npm (recommended)

```bash
npm create onchain-agent@latest
```

### Using npx

```bash
npx create-onchain-agent@latest
```

### Using yarn

```bash
yarn create onchain-agent
```

### Using pnpm

```bash
pnpm create onchain-agent
```

## Interactive Setup

When you run the command, you'll be prompted for:

### 1. Project Name
- **Question:** "Project name:"
- **Default:** `my-onchain-agent`
- **Validation:** Must only contain lowercase letters, numbers, and hyphens
- **Example:** `trading-agent`, `nft-bot`, `defi-assistant`

### 2. API Key (Optional)
- **Question:** "Enter your Coinbase Developer Platform Client API Key: (optional)"
- **Default:** Empty (can be configured later)
- **Purpose:** Used for OnchainKit integration
- **Where to get it:** https://portal.cdp.coinbase.com/

### 3. Usage Data (Optional)
- **Question:** "Share anonymous usage data to help improve create-onchain-agent?"
- **Default:** No
- **Purpose:** Help improve the tool through anonymous usage statistics

## What Gets Created

The CLI generates a complete Next.js project with the following structure:

```
my-onchain-agent/
├── app/
│   ├── components/
│   │   ├── ConnectButton.tsx           # Wallet connection component
│   │   ├── ConnectButton.module.css
│   │   ├── AgentInterface.tsx          # Main agent chat interface
│   │   └── AgentInterface.module.css
│   ├── layout.tsx                      # Root layout with metadata
│   ├── page.tsx                        # Home page
│   ├── page.module.css
│   ├── globals.css                     # Global styles
│   ├── rootProvider.tsx                # Providers setup (Wagmi, OnchainKit)
│   └── walletConnectors.ts             # Wallet configuration
├── public/                             # Static assets directory
├── .env                                # Environment variables
├── .env.example                        # Environment variables template
├── .gitignore                          # Git ignore rules
├── .prettierrc                         # Prettier configuration
├── eslint.config.mjs                   # ESLint configuration
├── next.config.ts                      # Next.js configuration
├── package.json                        # Project dependencies
├── tsconfig.json                       # TypeScript configuration
└── README.md                           # Project documentation
```

## Features Included

### 🤖 Agent Interface
- Ready-to-use chat interface for agent interactions
- Message history display
- Input handling and submission
- Placeholder for custom agent logic

### 👛 Wallet Integration
- Coinbase Smart Wallet support via Wagmi
- Connect/Disconnect functionality
- Address display
- Multi-chain support (Base, Base Sepolia)

### ⛓️ Blockchain Integration
- OnchainKit for blockchain interactions
- Wagmi for React Hooks
- Viem for Ethereum utilities
- Pre-configured for Base network

### 🎨 Modern UI
- Clean, responsive design
- CSS modules for scoped styling
- Gradient themes
- Dark mode ready

### 🔐 Security
- Environment variable management
- `.env` excluded from git by default
- Best practices for API key handling
- Secure wallet connections

### 📝 TypeScript
- Full TypeScript support
- Type definitions included
- Strict mode enabled

### 🧪 Development Tools
- ESLint for code quality
- Prettier for code formatting
- Next.js dev server with hot reload

## Getting Started After Creation

Once your project is created:

```bash
# Navigate to your project
cd my-onchain-agent

# Install dependencies (automatically done by CLI)
npm install

# Start the development server
npm run dev
```

Then open http://localhost:3000 to see your agent!

## Configuration

### Environment Variables

Edit `.env` to configure your agent:

```env
# Coinbase Developer Platform API Key
NEXT_PUBLIC_ONCHAINKIT_API_KEY=your-api-key-here

# Project name
NEXT_PUBLIC_PROJECT_NAME=my-onchain-agent

# Optional: Override default chain
# NEXT_PUBLIC_CHAIN_ID=8453   # Base Mainnet
# NEXT_PUBLIC_CHAIN_ID=84532  # Base Sepolia
```

### Adding More Chains

Edit `app/walletConnectors.ts`:

```typescript
import { http, createConfig } from "wagmi";
import { base, baseSepolia, mainnet, polygon } from "wagmi/chains";
import { coinbaseWallet } from "wagmi/connectors";

export const config = createConfig({
  chains: [base, baseSepolia, mainnet, polygon],
  connectors: [
    coinbaseWallet({
      appName: "Onchain Agent",
      preference: "smartWalletOnly",
    }),
  ],
  ssr: true,
  transports: {
    [base.id]: http(),
    [baseSepolia.id]: http(),
    [mainnet.id]: http(),
    [polygon.id]: http(),
  },
});
```

## Building Your Agent

### Implementing Agent Logic

The main agent logic goes in `app/components/AgentInterface.tsx`:

```typescript
const handleSubmit = async (e: React.FormEvent) => {
  e.preventDefault();
  if (!input.trim()) return;

  // Add user message
  const userMessage = { role: "user", content: input };
  setMessages([...messages, userMessage]);
  setInput("");

  // YOUR AGENT LOGIC HERE
  try {
    // Example: Query blockchain data
    const balance = await getBalance(address);
    
    // Example: Call a smart contract
    const result = await writeContract({
      address: contractAddress,
      abi: contractABI,
      functionName: 'execute',
      args: [param1, param2]
    });
    
    // Add agent response
    const agentMessage = {
      role: "assistant",
      content: `Transaction submitted: ${result.hash}`,
    };
    setMessages((prev) => [...prev, agentMessage]);
  } catch (error) {
    const errorMessage = {
      role: "assistant",
      content: `Error: ${error.message}`,
    };
    setMessages((prev) => [...prev, errorMessage]);
  }
};
```

## Example Use Cases

### Trading Agent
Build an agent that:
- Monitors token prices
- Executes trades on DEXs
- Manages stop-loss orders
- Provides portfolio analytics

### NFT Bot
Build an agent that:
- Mints NFTs based on criteria
- Monitors floor prices
- Auto-buys undervalued NFTs
- Manages NFT collections

### DeFi Assistant
Build an agent that:
- Provides yield farming recommendations
- Auto-compounds rewards
- Rebalances portfolios
- Monitors protocol health

### Analytics Agent
Build an agent that:
- Queries on-chain data
- Generates insights and reports
- Tracks wallet activities
- Alerts on significant events

## Available Scripts

In your project directory:

- `npm run dev` - Start development server
- `npm run build` - Build for production
- `npm run start` - Start production server
- `npm run lint` - Run ESLint

## Requirements

- **Node.js:** >= 18.0.0
- **npm:** >= 9.0.0 (or equivalent yarn/pnpm)

## Dependencies

The generated project includes:

### Core Dependencies
- `@coinbase/onchainkit` - OnchainKit for blockchain interactions
- `@coinbase/cdp-sdk` - Coinbase Developer Platform SDK
- `@tanstack/react-query` - Data fetching and caching
- `next` - Next.js framework (v15.3.9+)
- `react` - React library (v19)
- `react-dom` - React DOM renderer
- `viem` - TypeScript Ethereum library
- `wagmi` - React Hooks for Ethereum

### Dev Dependencies
- `typescript` - TypeScript compiler
- `eslint` - Code linting
- `eslint-config-next` - Next.js ESLint config
- Various type definitions

## Security Best Practices

### Environment Variables
- Never commit `.env` to version control
- Use `.env.local` for local development secrets
- Configure environment variables in your hosting platform

### API Keys
- Store API keys in environment variables
- Use separate keys for development and production
- Rotate keys regularly
- Never expose keys in client-side code (except those prefixed with `NEXT_PUBLIC_`)

### Wallet Interactions
- Always validate user inputs
- Show transaction details before signing
- Implement proper error handling
- Test thoroughly on testnets first

### Smart Contracts
- Audit contracts before interaction
- Use well-known, audited contracts when possible
- Implement transaction limits and safeguards
- Monitor for unusual activity

## Deployment

### Deploy to Vercel

The easiest deployment method:

1. Push your code to GitHub
2. Import project in Vercel
3. Configure environment variables
4. Deploy

[![Deploy with Vercel](https://vercel.com/button)](https://vercel.com/new)

### Environment Variables for Production

Set these in your hosting platform:
- `NEXT_PUBLIC_ONCHAINKIT_API_KEY`
- `NEXT_PUBLIC_PROJECT_NAME`
- Any other custom environment variables

## Troubleshooting

### "Command not found: create-onchain-agent"

Make sure you're using the correct syntax:
```bash
npm create onchain-agent@latest
```

Not:
```bash
npm install -g create-onchain-agent  # ❌ Wrong
```

### "Directory already exists"

Choose a different project name or remove the existing directory:
```bash
rm -rf my-onchain-agent
```

### Port 3000 already in use

Use a different port:
```bash
PORT=3001 npm run dev
```

### Wallet connection issues

1. Make sure you have the Coinbase Wallet extension installed
2. Check that your API key is correctly set in `.env`
3. Verify you're on a supported network

## Resources

- [OnchainKit Documentation](https://docs.base.org/onchainkit)
- [Base Documentation](https://docs.base.org)
- [Wagmi Documentation](https://wagmi.sh)
- [Next.js Documentation](https://nextjs.org/docs)
- [Coinbase Developer Platform](https://portal.cdp.coinbase.com/)
- [Viem Documentation](https://viem.sh)

## Support

For issues or questions:

1. Check the [create-onchain-agent README](create-onchain-agent/README.md)
2. Check the [OnchainKit documentation](https://docs.base.org/onchainkit)
3. Visit the [Base Discord](https://base.org/discord)
4. Open an issue on GitHub

## Contributing

Contributions are welcome! To contribute to `create-onchain-agent`:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

MIT License - see [LICENSE](create-onchain-agent/LICENSE) for details

## Version History

### v1.0.0 (2026-02-17)
- Initial release
- Next.js 15 with App Router
- OnchainKit integration
- Wagmi v2 integration
- TypeScript support
- Agent interface template
- Wallet connection component
- Base network support
- Interactive CLI with prompts
