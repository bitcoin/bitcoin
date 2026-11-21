Updated RPCs
------------

- The `sendrawtransaction` RPC has a new `delay_after_lock_time` argument that
  schedules a private broadcast for a later block height: the transaction is
  broadcast once the chain reaches the height `nLockTime + delay_after_lock_time`
  (or immediately, if that height has already been reached). Only height-based
  lock times are supported. `delay_after_lock_time` requires `-privatebroadcast`
  and is rejected otherwise.
  A scheduled transaction is validated at submission, but if it is not final
  yet (e.g. its `nLockTime` is in the future), validation stops early and it is
  accepted anyway: only basic checks are done then, and its inputs, scripts and
  fee (including `maxfeerate`) are not checked until it is due. When it is due,
  it is fully validated, and dropped if it is not valid; the caller is not
  notified of this. For a scheduled transaction, a successful result means that
  it is scheduled, not that it has been sent. Scheduled transactions are shown
  by `getprivatebroadcastinfo` (with their `release_height`) and can be
  cancelled with `abortprivatebroadcast`. They are not saved to disk and are
  lost on restart.
