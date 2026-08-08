P2P and network changes
-----------------------

- Each transaction sent via private broadcast (`-privatebroadcast`) is limited
  to 1,000 send attempts. After reaching the limit, broadcasting stops; call
  `sendrawtransaction` again to retry. Transactions that reach the limit remain
  available through `getprivatebroadcastinfo` and `abortprivatebroadcast`. (#35680)

Updated RPCs
------------

- `getprivatebroadcastinfo` now reports an `attempts_remaining` field for each
  transaction. (#35680)
