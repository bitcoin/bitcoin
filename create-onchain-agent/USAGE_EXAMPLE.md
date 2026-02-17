# Usage Example for create-onchain-agent

## Basic Usage

### Create a new onchain agent project

```bash
npm create onchain-agent@latest
```

### Example Session

```
🚀 Create Onchain Agent

? Project name: › my-trading-agent
✔ Project name: … my-trading-agent

? Enter your Coinbase Developer Platform Client API Key: (optional) › 
✔ Enter your Coinbase Developer Platform Client API Key: (optional) … 

? Share anonymous usage data to help improve create-onchain-agent? › No
✔ Share anonymous usage data to help improve create-onchain-agent? … no

✔ Creating project...

📦 Installing dependencies...

added 234 packages in 12s

✅ All done!

To get started:

  cd my-trading-agent
  npm run dev

Then open http://localhost:3000 in your browser.

Learn more at https://docs.base.org/onchainkit
```

## What Gets Created

After running the command, you'll have a complete project:

```
my-trading-agent/
├── app/
│   ├── components/
│   │   ├── AgentInterface.tsx         # Main agent UI
│   │   ├── AgentInterface.module.css
│   │   ├── ConnectButton.tsx          # Wallet connection
│   │   └── ConnectButton.module.css
│   ├── layout.tsx
│   ├── page.tsx
│   ├── page.module.css
│   ├── globals.css
│   ├── rootProvider.tsx               # Wagmi + OnchainKit setup
│   └── walletConnectors.ts            # Wallet config
├── public/
├── .env                               # Your API key (not committed)
├── .env.example
├── .gitignore
├── .prettierrc
├── eslint.config.mjs
├── next.config.ts
├── package.json
├── tsconfig.json
└── README.md
```

## Next Steps

### 1. Start Development

```bash
cd my-trading-agent
npm run dev
```

Open http://localhost:3000

### 2. Customize Your Agent

Edit `app/components/AgentInterface.tsx` to implement your agent logic:

```typescript
const handleSubmit = async (e: React.FormEvent) => {
  e.preventDefault();
  if (!input.trim()) return;

  // Add your custom agent logic here
  const userMessage = { role: "user", content: input };
  setMessages([...messages, userMessage]);
  setInput("");

  // Example: Call a smart contract
  try {
    const result = await yourAgentFunction(input);
    const agentMessage = {
      role: "assistant",
      content: result,
    };
    setMessages((prev) => [...prev, agentMessage]);
  } catch (error) {
    // Handle errors
  }
};
```

### 3. Configure Environment Variables

Edit `.env`:

```env
NEXT_PUBLIC_ONCHAINKIT_API_KEY=your-actual-api-key
NEXT_PUBLIC_PROJECT_NAME=my-trading-agent
```

Get your API key from: https://portal.cdp.coinbase.com/

### 4. Add More Features

- Integrate with DeFi protocols
- Add transaction signing
- Implement trading strategies
- Connect to data sources
- Add more blockchain networks

## Example Agent Types

### Trading Agent
```bash
npm create onchain-agent@latest
# Project name: defi-trading-bot
```

### NFT Bot
```bash
npm create onchain-agent@latest
# Project name: nft-minting-agent
```

### Portfolio Manager
```bash
npm create onchain-agent@latest
# Project name: crypto-portfolio-manager
```

### Analytics Agent
```bash
npm create onchain-agent@latest
# Project name: blockchain-analytics-agent
```

## Deployment

Deploy to Vercel:

```bash
# Install Vercel CLI
npm i -g vercel

# Deploy
vercel
```

Or push to GitHub and deploy via Vercel dashboard.

## Troubleshooting

### "Cannot find module 'prompts'"

The CLI requires Node.js 18+. Update Node.js:
```bash
nvm install 18
nvm use 18
```

### Port 3000 in use

Use a different port:
```bash
PORT=3001 npm run dev
```

### Wallet won't connect

1. Install Coinbase Wallet extension
2. Make sure you're on a supported network
3. Check your API key in `.env`

## Resources

- [Full Documentation](CREATE_ONCHAIN_AGENT_GUIDE.md)
- [OnchainKit Docs](https://docs.base.org/onchainkit)
- [Wagmi Docs](https://wagmi.sh)
- [Base Network](https://base.org)
