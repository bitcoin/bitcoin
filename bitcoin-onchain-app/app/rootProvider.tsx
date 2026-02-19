"use client";
import { ReactNode } from "react";
import { base, mainnet } from "wagmi/chains";
import { OnchainKitProvider } from "@coinbase/onchainkit";
import { createConfig, http } from "wagmi";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { WagmiProvider } from "wagmi";
import { createCDPEmbeddedWalletConnector } from "./walletConnectors";
import "@coinbase/onchainkit/styles.css";

// Create wagmi configuration with CDP Embedded Wallet connector
const cdpConnector = createCDPEmbeddedWalletConnector({
  appName: process.env.NEXT_PUBLIC_PROJECT_NAME || "Bitcoin OnChain App",
  appLogoUrl: "/bitcoin-logo.png",
  preference: "embedded",
  enableSmartWallet: false,
  version: 4,
});

const wagmiConfig = createConfig({
  chains: [base, mainnet],
  connectors: [cdpConnector],
  transports: {
    [base.id]: http(),
    [mainnet.id]: http(),
  },
  ssr: true,
});

// Create React Query client for wagmi
const queryClient = new QueryClient();

export function RootProvider({ children }: { children: ReactNode }) {
  return (
    <WagmiProvider config={wagmiConfig}>
      <QueryClientProvider client={queryClient}>
        <OnchainKitProvider
          apiKey={process.env.NEXT_PUBLIC_ONCHAINKIT_API_KEY}
          chain={base}
          config={{
            appearance: {
              mode: "auto",
            },
            wallet: {
              display: "modal",
              preference: "all",
            },
          }}
        >
          {children}
        </OnchainKitProvider>
      </QueryClientProvider>
    </WagmiProvider>
  );
}
