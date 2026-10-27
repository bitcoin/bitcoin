Tools and Utilities
-------------------

- `bitcoin-privbcast send` also takes a parent and its child, two hex
  transactions separated by whitespace, in either order. The job announces
  only the child and serves the parent to a peer that asks for it, so that a
  parent too cheap to enter mempools on its own can be carried by a child that
  pays for both. The tool checks that the two are such a pair, but no fees.
  The report adds `parent_txid`, `parent_wtxid`, per attempt
  `parent_getdata_ms`, `parent_tx_written_ms` and `parent_hold_expired_ms`,
  and `summary.parents_served`.

Updated RPCs
------------

- With `-privatebroadcast`, `submitpackage` now queues a private broadcast job
  instead of adding the transactions to the mempool. It takes one transaction,
  or one parent and its child; more fail with `RPC_INVALID_PARAMETER`. A
  transaction whose txid is already in the mempool counts as accepted and is
  sent as given; the rest is test-accepted, one transaction alone as by
  `sendrawtransaction`, two as a package. A parent too cheap on its own goes
  out with its child if `submitpackage` without `-privatebroadcast` would
  accept the two. The job announces the child and serves the parent
  to a peer that asks for it. The call fails as `sendrawtransaction` does when
  the job cannot be queued or no Tor proxy is configured.

- A job that carries a transaction with its parent also covers the
  transaction submitted alone; a job for the transaction alone does not cover
  it submitted with a parent.

- `getprivatebroadcastinfo` shows `parent_txid` for a job that carries a
  parent, and the job's report has the package fields of the tool's.
  `abortprivatebroadcast` matches such a job by its child's txid or wtxid
  only, as two packages can share a parent.

See [Private broadcast](/doc/private-broadcast.md).
