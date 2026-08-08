RPC
---

- `gettransaction`, `listtransactions`, and `listsinceblock` now have an `alternate_wtxids` field which lists the wtxids of all transactions that have the same txid. When there is only one known witness variant the field is an empty array, analogous to `walletconflicts` and `mempoolconflicts`.
