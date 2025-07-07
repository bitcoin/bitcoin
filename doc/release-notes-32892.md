RPC changes:
------------

- The `send`, `sendall` and `walletcreatefundedpsbt` will set nLockTime to
  approximately the current height to avoid fee sniping, unless transaction
  `locktime` or an input `sequence` is specified. (#32892).
