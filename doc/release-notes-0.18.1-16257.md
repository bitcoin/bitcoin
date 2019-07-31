Wallet changes
--------------
When creating a transaction with a fee above `-maxtxfee` (default 0.1 BTC),
the RPC commands `walletcreatefundedpsbt` and  `fundrawtransaction` will now fail
instead of rounding down the fee. Beware that the `feeRate` argument is specified
in BTC per kilobyte, not satoshi per byte.
