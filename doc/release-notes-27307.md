Wallet
---

The wallet now detects when wallet transactions conflict with the mempool. Mempool
conflicting transactions can be seen in the `"mempoolconflicts"` field of
`gettransaction`. The inputs of mempool conflicted transactions can now be respent
without manually abandoning the transactions when the parent transaction is dropped
from the mempool, which can cause wallet balances to appear higher.
