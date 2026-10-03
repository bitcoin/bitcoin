Wallet
------

- Transactions not spending any inputs from the wallet are no longer
  rebroadcast, and are no longer re-added to the mempool on startup. It is now
  the sole responsibility of the sender to rebroadcast as necessary. Incoming
  transactions can be rebroadcast manually by passing the `hex` field of
  `gettransaction` to `sendrawtransaction`. (#36398)
