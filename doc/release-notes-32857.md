Updated RPCs
------------

- The `send`, `sendall`, `walletprocesspsbt`, `walletcreatefundedpsbt`, and
  `descriptorprocesspsbt` RPCs now accept a `keypath_only` option. When enabled,
  they do not add new Taproot script-path data, sign script paths, or finalize
  script-path spends. Existing script-path data and signatures remain in a
  supplied PSBT. (#32857)
