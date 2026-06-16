Updated RPCs
------------

- The `send`, `sendall`, `walletprocesspsbt`, and `descriptorprocesspsbt` RPCs
  now accept a `keypath_only` option that only signs the key path for taproot
  inputs. (#32857)
