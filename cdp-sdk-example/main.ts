// CDP SDK Example - TypeScript Entry Point
// This example demonstrates how to set up a basic CDP SDK project

import { config } from 'dotenv';

// Load environment variables from .env file
config();

async function main() {
  console.log('CDP SDK Example');
  console.log('================');
  console.log('');
  console.log('This is a basic example project setup for the Coinbase Developer Platform SDK.');
  console.log('');
  console.log('Environment Configuration:');
  console.log('- CDP_API_KEY_NAME:', process.env.CDP_API_KEY_NAME ? '✓ Set' : '✗ Not set');
  console.log('- CDP_API_KEY_PRIVATE_KEY:', process.env.CDP_API_KEY_PRIVATE_KEY ? '✓ Set' : '✗ Not set');
  console.log('');
  console.log('Next steps:');
  console.log('1. Install the CDP SDK: npm install @coinbase/coinbase-sdk');
  console.log('2. Configure your .env file with CDP API credentials');
  console.log('3. Import and use the CDP SDK in this file');
  console.log('');
  console.log('For more information, see:');
  console.log('- CDP API Documentation: https://docs.cdp.coinbase.com/');
  console.log('- Bitcoin repository CDP integration: ../contrib/devtools/CDP_API_README.md');
}

main().catch(console.error);
