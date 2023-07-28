RPC Wallet
----------

- The `signrawtransactionwithkey`, `signrawtransactionwithwallet`,
  `walletprocesspsbt` and `descriptorprocesspsbt` calls now return the more
  specific RPC_INVALID_PARAMETER error instead of RPC_MISC_ERROR if their
  sighashtype argument is malformed.
