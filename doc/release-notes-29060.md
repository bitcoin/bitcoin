- Logging and RPC

  - Bitcoin Core now reports a debug message explaining why transaction inputs are non-standard.

  - This information is now returned in the responses of the transaction-sending RPCs `submitpackage`,
   `sendrawtransaction`, and `testmempoolaccept`, and is also logged to `debug.log` when such transactions
   are received over the P2P network.

  - This does not change the existing error code `bad-txns-nonstandard-inputs`, but instead adds additional debug information to it.
