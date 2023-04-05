Notable changes
===============

P2P and network changes
-----------------------
* Deprecate the BIP35 `mempool` message due to disuse and poor privacy.


Updated settings
----------------
* `--whitebind=` and friends will now not accept `mempool` as a NetPermission.
  Setting `mempool` will result in the node failing to start up with error:
  `Error: Invalid P2P permission: 'mempool'`

