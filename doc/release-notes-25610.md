Wallet
------

- The `-walletrbf` startup option will now default to `true`. The
  wallet will now default to opt-in RBF on transactions that it creates.

Updated RPCs
------------

- The `replaceable` option for the `createrawtransaction` and
  `createpsbt` RPCs will now default to `true`. Transactions created
  with these RPCs will default to having opt-in RBF enabled.
