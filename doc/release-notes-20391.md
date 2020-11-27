Updated RPCs
------------

- `settxfee` is now a hidden RPC and marked as deprecated in favor of the new
  `setfeerate` RPC described in the "New RPCs" section. This change is part of a
  larger migration from BTC/kvB to sat/vB units for fee rates. (#20391)

New RPCs
------------

- A new `setfeerate` RPC is added to set the transaction fee for a wallet in
  sat/vB, instead of in BTC/kvB like `settxfee`. This RPC is intended to replace
  `settxfee`, which becomes a hidden RPC and is marked as deprecated. This
  change is part of a larger migration from BTC/kvB to sat/vB units for fee
  rates. (#20391)

