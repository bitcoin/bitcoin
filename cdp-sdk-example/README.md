# CDP SDK Example

This directory contains a basic example setup for using the Coinbase Developer Platform (CDP) SDK.

## What This Example Demonstrates

This is a minimal TypeScript project configured to work with the CDP SDK. It includes:

- **package.json** - Configured as an ES module project
- **main.ts** - TypeScript entry point with basic CDP SDK setup
- **.env** - Environment configuration file for API credentials

## Setup Instructions

### 1. Install Dependencies

First, install the required dependencies:

```bash
npm install
```

### 2. Install CDP SDK

Install the Coinbase Developer Platform SDK:

```bash
npm install @coinbase/coinbase-sdk dotenv
```

### 3. Install TypeScript and ts-node (for running TypeScript directly)

```bash
npm install --save-dev typescript ts-node @types/node
```

### 4. Configure Your Credentials

Edit the `.env` file and add your CDP API credentials:

```bash
CDP_API_KEY_NAME=your-api-key-name-here
CDP_API_KEY_PRIVATE_KEY=your-private-key-here
```

To get your credentials:
1. Visit https://portal.cdp.coinbase.com/
2. Create a new API key
3. Copy your credentials to the `.env` file

### 5. Run the Example

```bash
npx ts-node main.ts
```

## Next Steps

Once you have this basic setup working, you can:

1. Import the CDP SDK in `main.ts`:
   ```typescript
   import { Coinbase } from '@coinbase/coinbase-sdk';
   ```

2. Initialize the SDK:
   ```typescript
   const coinbase = Coinbase.configureFromJson({
     apiKeyName: process.env.CDP_API_KEY_NAME,
     privateKey: process.env.CDP_API_KEY_PRIVATE_KEY
   });
   ```

3. Start using CDP SDK features like creating wallets, managing addresses, and more.

## Related Resources

- **CDP SDK Documentation**: https://docs.cdp.coinbase.com/
- **CDP API Tools in this repo**: See `../contrib/devtools/CDP_API_README.md`
- **CDP API Quick Start**: See `../CDP_API_QUICKSTART.md`

## Security Notes

⚠️ **Important**: 
- Never commit your `.env` file with real credentials
- The `.env` file is already in `.gitignore` to prevent accidental commits
- Keep your API keys secure and rotate them regularly

## License

MIT License - See the main repository LICENSE file for details.
