P2P and network changes
-----------------------

#### Removal of reject network messages from Dash Core (BIP61)

The command line option to enable BIP61 (`-enablebip61`) has been removed.

This feature has been disabled by default since Dash Core version 0.19.0.
Nodes on the network can not generally be trusted to send valid ("reject")
messages, so this should only ever be used when connected to a trusted node.

Since Dash Core version 0.20.0 there are extra changes:

The removal of BIP61 REJECT message support also has the following minor RPC
and logging implications:

* `testmempoolaccept` and `sendrawtransaction` no longer return the P2P REJECT
  code when a transaction is not accepted to the mempool. They still return the
  verbal reject reason.

* Log messages that previously reported the REJECT code when a transaction was
  not accepted to the mempool now no longer report the REJECT code. The reason
  for rejection is still reported.

Updated RPCs
------------

- `testmempoolaccept` and `sendrawtransaction` no longer return the P2P REJECT
  code when a transaction is not accepted to the mempool. See the Section
  _Removal of reject network messages from Bitcoin Core (BIP61)_ for details on
  the removal of BIP61 REJECT message support.
