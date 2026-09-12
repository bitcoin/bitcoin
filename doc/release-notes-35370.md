Updated RPCs
------------

- The `bip32derivs` argument of the `walletprocesspsbt` and
  `descriptorprocesspsbt` RPCs now also accepts string modes for handling
  standard key-origin fields. `"add"` allows known fields to be added,
  `"preserve"` does not add or explicitly remove fields, and `"strip"`
  removes surviving global xpubs and legacy and Taproot BIP32 derivation
  fields after normal processing. The default remains `true`, equivalent to
  `"add"`; `false` remains accepted as an alias for `"preserve"`.
  Finalization may remove input key-origin fields in any mode.
