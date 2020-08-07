Updated RPCs
------------

- `fundrawtransaction` and `walletcreatefundedpsbt` when used with the `lockUnspents`
   argument now lock manually selected coins, in addition to automatically selected
   coins. Note that locked coins are never used in automatic coin selection, but
   can still be manually selected.
