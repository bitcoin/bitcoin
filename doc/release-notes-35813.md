New RPCs
--------

- A new `listrawtransactions` RPC has been added to the wallet. Unlike
  `listtransactions`, which only lists transactions with a logical economic
  category (sends to external addresses, receives from external addresses), this
  RPC returns every transaction the wallet knows about, including consolidations
  and self-transfers that would otherwise be invisible. Each transaction appears
  exactly once with its net wallet balance change excluding fee (`amount`) and,
  when the wallet funded the transaction, the (negative) fee paid (`fee`). The
  results support `count` and `skip` pagination, and a `verbose` flag that adds
  a `decoded` field with the full decoded transaction data. (#35813)
