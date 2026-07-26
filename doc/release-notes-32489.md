New RPCs
--------

A new `exportwatchonlywallet` RPC creates a watchonly wallet file from an
existing descriptor wallet. The exported file contains the wallet's public
descriptors (with derived key caches where needed), transactions, and address
book data, but no private keys. It can be imported on another node using the
existing `restorewallet` RPC.

Wallet
------

The [Offline Signing Tutorial](/doc/offline-signing-tutorial.md) has been
updated to use `exportwatchonlywallet` for setting up the online watch-only
wallet, replacing the previous manual descriptor import workflow.
