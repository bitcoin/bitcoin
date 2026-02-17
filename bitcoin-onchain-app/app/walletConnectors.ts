/**
 * CDP Embedded Wallet Connector Configuration
 * 
 * This module provides utilities for creating and configuring
 * Coinbase Developer Platform (CDP) Embedded Wallet connectors
 * for use with wagmi and OnchainKit.
 * 
 * Copyright (c) 2026 The Bitcoin Core developers
 * Distributed under the MIT software license
 */

import { coinbaseWallet } from 'wagmi/connectors';
import type { CreateConnectorFn } from 'wagmi';

/**
 * Configuration options for CDP Embedded Wallet
 */
export interface CDPEmbeddedWalletConfig {
  /**
   * Application name displayed in the wallet
   */
  appName: string;
  
  /**
   * Application logo URL
   */
  appLogoUrl?: string;
  
  /**
   * Whether to prefer embedded wallet over extension
   * @default true
   */
  preference?: 'embedded' | 'eoaOnly' | 'smartWalletOnly' | 'all';
  
  /**
   * Chain ID to connect to
   */
  chainId?: number;
  
  /**
   * Enable smart wallet features
   * @default false
   */
  enableSmartWallet?: boolean;
  
  /**
   * Custom RPC URL (optional)
   */
  rpcUrl?: string;
  
  /**
   * Version of the wallet SDK to use
   * @default 4
   */
  version?: 3 | 4;
}

/**
 * Creates a CDP Embedded Wallet connector for wagmi
 * 
 * This function creates a Coinbase Wallet connector configured
 * for CDP Embedded Wallet functionality. It uses the Coinbase
 * Wallet SDK with settings optimized for embedded wallet experiences.
 * 
 * @param config - Configuration options for the wallet connector
 * @returns A wagmi connector function
 * 
 * @example
 * ```typescript
 * const connector = createCDPEmbeddedWalletConnector({
 *   appName: 'Bitcoin Core App',
 *   appLogoUrl: 'https://example.com/logo.png',
 *   preference: 'embedded',
 *   enableSmartWallet: true,
 * });
 * 
 * // Use in wagmi config
 * const wagmiConfig = createConfig({
 *   connectors: [connector],
 *   // ... other config
 * });
 * ```
 */
export function createCDPEmbeddedWalletConnector(
  config: CDPEmbeddedWalletConfig
): CreateConnectorFn {
  const {
    appName,
    appLogoUrl,
    preference = 'embedded',
    chainId,
    enableSmartWallet = false,
    rpcUrl,
    version = 4,
  } = config;

  // Validate required configuration
  if (!appName || appName.trim().length === 0) {
    throw new Error('appName is required for CDP Embedded Wallet connector');
  }

  // Create and return the Coinbase Wallet connector
  // with embedded wallet preferences
  return coinbaseWallet({
    appName,
    appLogoUrl,
    preference,
    version,
    ...(chainId && { chainId }),
    ...(rpcUrl && { jsonRpcUrl: rpcUrl }),
    ...(enableSmartWallet && { 
      // Smart wallet configuration
      // Note: This may require additional CDP SDK packages
      enableSmartWallet: true 
    }),
  }) as CreateConnectorFn;
}

/**
 * Creates a default CDP Embedded Wallet connector with sensible defaults
 * for the Bitcoin Core application
 * 
 * @returns A wagmi connector function configured for Bitcoin Core
 * 
 * @example
 * ```typescript
 * const connector = createDefaultCDPConnector();
 * ```
 */
export function createDefaultCDPConnector(): CreateConnectorFn {
  return createCDPEmbeddedWalletConnector({
    appName: 'Bitcoin Core - OnChain App',
    appLogoUrl: '/bitcoin-logo.png',
    preference: 'embedded',
    enableSmartWallet: false,
    version: 4,
  });
}

/**
 * Creates multiple wallet connectors including CDP Embedded Wallet
 * 
 * @param config - Configuration for CDP connector
 * @returns Array of wagmi connectors
 * 
 * @example
 * ```typescript
 * const connectors = createWalletConnectors({
 *   appName: 'My App',
 * });
 * ```
 */
export function createWalletConnectors(
  config: CDPEmbeddedWalletConfig
): CreateConnectorFn[] {
  // Return array of connectors with CDP Embedded Wallet as the primary option
  return [
    createCDPEmbeddedWalletConnector(config),
    // Additional connectors can be added here if needed
    // For example: injected(), walletConnect(), etc.
  ];
}
