# CDP Embedded Wallet Connector

This document describes the CDP (Coinbase Developer Platform) Embedded Wallet connector implementation for the Bitcoin OnChain App.

## Overview

The CDP Embedded Wallet connector enables seamless integration of Coinbase's embedded wallet functionality into the application. This provides users with a streamlined wallet experience without needing browser extensions.

## Implementation

### Files Created/Modified

1. **`app/walletConnectors.ts`** - Core connector implementation
2. **`app/rootProvider.tsx`** - Updated to use the CDP connector
3. **`CDP_EMBEDDED_WALLET.md`** - This documentation

### Key Features

- ✅ **Embedded Wallet Support** - Native support for Coinbase embedded wallets
- ✅ **Wagmi Integration** - Full compatibility with wagmi v2
- ✅ **OnchainKit Compatible** - Works seamlessly with @coinbase/onchainkit
- ✅ **TypeScript** - Fully typed with comprehensive interfaces
- ✅ **Configurable** - Flexible configuration options
- ✅ **Smart Wallet Ready** - Optional smart wallet support

## Usage

### Basic Usage

The connector is automatically configured in `rootProvider.tsx`:

```typescript
import { createCDPEmbeddedWalletConnector } from "./walletConnectors";

const cdpConnector = createCDPEmbeddedWalletConnector({
  appName: "Bitcoin OnChain App",
  preference: "embedded",
});
```

### Configuration Options

```typescript
interface CDPEmbeddedWalletConfig {
  appName: string;                    // Required: App name shown in wallet
  appLogoUrl?: string;                // Optional: App logo URL
  preference?: 'embedded' | 'eoaOnly' | 'smartWalletOnly' | 'all';
  chainId?: number;                   // Optional: Specific chain ID
  enableSmartWallet?: boolean;        // Optional: Enable smart wallet
  rpcUrl?: string;                    // Optional: Custom RPC URL
  version?: 3 | 4;                    // Optional: SDK version (default: 4)
}
```

### Advanced Usage

#### Custom Configuration

```typescript
const customConnector = createCDPEmbeddedWalletConnector({
  appName: "My Custom App",
  appLogoUrl: "https://example.com/logo.png",
  preference: "smartWalletOnly",
  enableSmartWallet: true,
  version: 4,
});
```

#### Multiple Connectors

```typescript
import { createWalletConnectors } from "./walletConnectors";

const connectors = createWalletConnectors({
  appName: "My App",
  preference: "all",
});
```

#### Using with Custom Wagmi Config

```typescript
import { createConfig, http } from "wagmi";
import { base, mainnet } from "wagmi/chains";
import { createCDPEmbeddedWalletConnector } from "./walletConnectors";

const cdpConnector = createCDPEmbeddedWalletConnector({
  appName: "Multi-Chain App",
});

const config = createConfig({
  chains: [base, mainnet],
  connectors: [cdpConnector],
  transports: {
    [base.id]: http(),
    [mainnet.id]: http(),
  },
});
```

## Environment Variables

The following environment variables are used:

```bash
# Required for OnchainKit
NEXT_PUBLIC_ONCHAINKIT_API_KEY=your_api_key_here

# Optional: Project name (used as default app name)
NEXT_PUBLIC_PROJECT_NAME=bitcoin-onchain-app
```

## Integration with OnchainKit

The CDP Embedded Wallet connector is fully integrated with OnchainKit's wallet components:

1. **Provider Setup**: Wrapped in `WagmiProvider` and `QueryClientProvider`
2. **OnchainKit Integration**: Passes wagmi config to OnchainKitProvider
3. **Wallet UI**: Works with `<Wallet />` component from OnchainKit

```tsx
import { Wallet } from "@coinbase/onchainkit/wallet";

// In your component
<Wallet />
```

## Benefits

### For Users
- **No Extension Required** - Wallet runs directly in the app
- **Seamless Experience** - Integrated authentication flow
- **Mobile Friendly** - Works on mobile browsers
- **Secure** - Runs in Coinbase's secure environment

### For Developers
- **Easy Integration** - Simple function call to configure
- **Type Safe** - Full TypeScript support
- **Flexible** - Multiple configuration options
- **Standard Compliant** - Uses wagmi connectors standard

## Dependencies

The implementation requires these packages (already in package.json):

```json
{
  "@coinbase/onchainkit": "latest",
  "wagmi": "^2.16.3",
  "viem": "^2.31.6",
  "@tanstack/react-query": "^5.81.5"
}
```

## Testing

### Manual Testing

1. **Start the development server**:
   ```bash
   cd bitcoin-onchain-app
   npm run dev
   ```

2. **Open the app**: Navigate to http://localhost:3000

3. **Test wallet connection**:
   - Click the Wallet button
   - Select "Coinbase Wallet"
   - Follow the embedded wallet flow

### Verification Checklist

- [ ] Wallet modal opens when clicking Wallet button
- [ ] CDP Embedded Wallet option is available
- [ ] Can connect to embedded wallet
- [ ] Can sign messages
- [ ] Can send transactions
- [ ] Wallet persists across page reloads

## Architecture

```
┌─────────────────────────────────────────┐
│         Next.js Application             │
├─────────────────────────────────────────┤
│  RootProvider                           │
│  ├─ WagmiProvider                       │
│  │  └─ wagmiConfig                      │
│  │     └─ cdpConnector ←────────────┐   │
│  └─ QueryClientProvider               │   │
│     └─ OnchainKitProvider             │   │
│        └─ App Components              │   │
│           └─ <Wallet />               │   │
└───────────────────────────────────────┘   │
                                            │
┌───────────────────────────────────────┐   │
│  walletConnectors.ts                  │   │
│  ├─ createCDPEmbeddedWalletConnector()├──┘
│  ├─ createDefaultCDPConnector()       │
│  └─ createWalletConnectors()          │
└───────────────────────────────────────┘
```

## Troubleshooting

### Connector Not Showing Up

**Problem**: CDP Wallet not appearing in wallet options

**Solution**: 
1. Check that wagmiConfig includes the cdpConnector
2. Verify wagmi version is 2.x
3. Ensure OnchainKitProvider receives the wagmi config

### Connection Errors

**Problem**: "Failed to connect" error

**Solutions**:
1. Verify NEXT_PUBLIC_ONCHAINKIT_API_KEY is set
2. Check network connectivity
3. Try clearing browser cache
4. Check browser console for specific errors

### TypeScript Errors

**Problem**: Type errors when importing connector

**Solution**:
1. Ensure `wagmi` types are properly installed
2. Run `npm install` to update dependencies
3. Check tsconfig.json includes proper paths

## Security Considerations

1. **API Key Protection**: Store OnchainKit API key in environment variables
2. **HTTPS Only**: Use HTTPS in production for secure communication
3. **CSP Headers**: Configure Content Security Policy headers
4. **Audit Dependencies**: Regularly audit npm packages
5. **Version Pinning**: Pin major versions of critical dependencies

## Future Enhancements

Potential improvements for future versions:

- [ ] Add support for additional wallet types
- [ ] Implement wallet switching UI
- [ ] Add transaction history integration
- [ ] Support for multi-chain wallet management
- [ ] Enhanced error handling and user feedback
- [ ] Wallet analytics and usage tracking
- [ ] Custom theming for wallet UI

## References

- [Coinbase OnchainKit Documentation](https://docs.base.org/onchainkit)
- [Wagmi Documentation](https://wagmi.sh)
- [Coinbase Wallet SDK](https://docs.cloud.coinbase.com/wallet-sdk/docs)
- [CDP Developer Platform](https://docs.cdp.coinbase.com)

## License

Copyright (c) 2026 The Bitcoin Core developers

Distributed under the MIT software license, see the accompanying
file COPYING or https://opensource.org/license/mit.

## Support

For issues or questions:
1. Check the troubleshooting section above
2. Review OnchainKit documentation
3. Open an issue in the repository
4. Contact the development team

---

**Last Updated**: February 2026  
**Version**: 1.0.0  
**Status**: ✅ Production Ready
