RPC Wallet
----------

- RPC `walletprocesspsbt` return object now includes field `hex` (if the transaction
is complete) containing the serialized transaction suitable for RPC `sendrawtransaction`. (#28414)
