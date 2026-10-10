Wallet
------

- `sendall` now rejects unknown keys in its `options` object, like `send`,
  `fundrawtransaction` and `walletcreatefundedpsbt` already do. Misspelled
  options and options of `send` that `sendall` does not support, such as
  `subtract_fee_from_outputs`, were previously silently ignored. Option
  values are also type-checked before any coin is selected.

- `sendall` no longer documents or accepts the `solving_data` option, which
  it never used.
