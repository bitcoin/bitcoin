Wallet
------

- `sendall` now rejects unknown keys in its `options` object instead of
  silently ignoring them, like `send`, `fundrawtransaction` and
  `walletcreatefundedpsbt` already do. Option values are also type-checked
  before any coin is selected.
