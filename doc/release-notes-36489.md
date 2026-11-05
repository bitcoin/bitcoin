Updated RPCs
------------

- The `fundrawtransaction` and `walletcreatefundedpsbt` RPCs now support the
  snake_case `change_address`, `change_position`, `lock_unspents`, and
  `subtract_fee_from_outputs` options as named parameters. Previously,
  `subtract_fee_from_outputs` was silently ignored.
