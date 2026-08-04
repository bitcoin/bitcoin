Wallet
------

- Transactions that spend a mix of wallet-owned and non-wallet inputs are now
  reported safely by the `gettransaction`, `listtransactions`, and
  `listsinceblock` RPCs. Previously such transactions reported a misleading fee
  and attributed non-wallet outputs as wallet sends (see issue #14136).

  When every non-wallet input has a known zero value (for example a
  pay-to-anchor output), the wallet can still attribute the full fee and all
  sent outputs, so these transactions continue to be reported with the usual
  per-output `send`/`receive` entries. Otherwise the wallet cannot attribute the
  foreign outputs or its fee share, so it reports a single unattributed
  aggregate `send` summary carrying the negative total of wallet-owned inputs
  spent, instead of fabricating per-recipient sends or a fee. Wallet-owned
  outputs are still reported as `receive` entries alongside that send summary,
  so deposit detection based on the `receive` category keeps working, and the
  amounts of a transaction's entries sum to the wallet's net change.

  Affected entries additionally expose the new `involves_mixed_inputs`,
  `wallet_debit`, and `wallet_credit` fields, which let consumers detect the
  conservative case without depending on the `category` value. The `vout` field
  is now optional in the result schema: it is omitted from unattributed
  aggregate mixed-input send summaries, which do not correspond to a single
  output. The `fee` field is likewise omitted when the wallet cannot determine
  the fee. (#34872)
