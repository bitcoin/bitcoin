Wallet
------

* Address Purposes strings are now restricted to the currently known values of "send", "receive", and "refund".
  Wallets that have unrecognized purpose strings will have loading warnings, and the `listlabels`
  RPC will raise an error if an unrecognized purpose is requested.
