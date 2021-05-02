* paytxfee switch is now per KB, so it adds the correct fee for large transactions
* sending avoids using coins with less than 6 confirmations if it can
* XBitMiner processes transactions in priority order based on age of dependencies
* make sure generation doesn't start before block 74000 downloaded
* bugfixes by Dean Gores
* testnet, keypoololdest and paytxfee added to getinfo
