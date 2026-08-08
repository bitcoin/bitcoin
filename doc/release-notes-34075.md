Updated RPCs
------------

- The `estimatesmartfee` RPC now combines two fee rate estimators: the existing
  block policy fee rate estimator and a new mempool fee rate estimator.

- The new mempool fee rate estimator produces conservative and economical fee
  rate estimates from the current contents of the mempool. It only produces a
  fee rate estimate when recent blocks indicate a healthy mempool, and falls
  back to the higher of the minimum relay fee rate and the current mempool
  minimum fee rate when the mempool is too sparse. Its statistics are persisted
  to `fees/mempool_policy_estimator.dat` and reloaded on startup.

  `estimatesmartfee` returns the lower of the two fee rate estimators' results,
  so the mempool fee rate estimator can only lower the block policy fee rate
  estimate.

- The combined estimate requires both estimators to succeed. If the mempool fee
  rate estimator cannot produce an estimate, for example, while the mempool is
  still loading, when too few recent blocks have been observed, or when the
  mempool is suspected to be unhealthy, an error is returned.

- `estimatesmartfee` accepts an `options` object with `fee_rate_estimator`.
  Recognized values are `"none"` (the default, combined behavior described
  above) and `"block_policy"` (use only the block policy fee rate estimator).
  All unknown values are treated as `"none"`.
  Users who want the previous behavior can select the block policy fee rate
  estimator explicitly.

- The options object also accepts `verbosity`. A verbosity of `2` or higher
  returns recent mempool health statistics only when `fee_rate_estimator` is
  `"none"`. When `fee_rate_estimator` is `"none"` and the estimate succeeds,
  the response also includes an `estimator` field identifying which fee rate
  estimator produced the result.

- Block policy fee estimator data is now stored in
  `fees/block_policy_estimates.dat`. If the new file does not exist, the
  legacy `fee_estimates.dat` file is moved to the new path during startup. If
  both files exist, the legacy file is removed.

- Wallet fee rate estimation uses the default combined estimate.

Wallet
------

- The `fee_reason` field returned by wallet transaction creation RPCs now
  reports the reason the wallet selected the fee rate (fee rate estimator,
  mempool minimum, fallback, or minimum required) instead of the block policy
  fee rate estimator's internal threshold details. Those details remain
  available in the block policy fee rate estimator debug log.
