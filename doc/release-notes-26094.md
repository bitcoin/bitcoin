- The `getbalances` RPC now returns a `lastprocessedblock` JSON object which contains the wallet's last processed block
  hash and height at the time the balances were calculated. This result shouldn't be cached because importing new keys could invalidate it.
- The `gettransaction` RPC now returns a `lastprocessedblock` JSON object which contains the wallet's last processed block
  hash and height at the time the transaction information was generated.
- The `getwalletinfo` RPC now returns a `lastprocessedblock` JSON object which contains the wallet's last processed block
  hash and height at the time the wallet information was generated.