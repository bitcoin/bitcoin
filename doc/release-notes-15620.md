Updated RPCs
------------

* The -maxtxfee setting no longer has any effect on non-wallet RPCs.

  The `sendrawtransaction` and `testmempoolaccept` RPC methods previously
  accepted an `allowhighfees` parameter to fail the mempool acceptance in case
  the transaction's fee would exceed the value of the command line argument
  `-maxtxfee`. To uncouple the RPCs from the global option, they now have a
  hardcoded default for the maximum transaction fee, that can be changed for
  both RPCs on a per-call basis with the `maxfeerate` parameter. The
  `allowhighfees` boolean option has been removed and replaced by the
  `maxfeerate` numeric option.
