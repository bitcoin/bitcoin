RPC changes
-----------
The RPC `getblockchaininfo` response changes the `softforks` key to provide an object whose keys are the softfork ids, rather than a list of softforks. This also now includes any BIP 9 softforks, and associated data. The `bip9_softforks` key is no longer included.
