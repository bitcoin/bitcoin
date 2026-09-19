Wallet
======

- A new wallet startup option `-maxfeerate` is added.
- This option sets the upper fee rate limit for wallet transactions.
- The wallet will not submit or broadcast transactions whose fee rate exceeds `-maxfeerate`.
- The default is 0.10 BTC/kvB (10,000 sat/vB).

RPC
===

- Transaction broadcasts now check the specified `maxfeerate` limit and fail
  if the transaction fee rate exceeds it.
- Transaction broadcast fee limit error messages are now more specific:
  `-maxtxfee` errors return "Fee exceeds maximum configured by user
  (maxtxfee)", while `maxfeerate` errors return a `MAX_FEE_RATE_EXCEEDED`
  error with "Fee rate exceeds maximum configured by user (maxfeerate)".
