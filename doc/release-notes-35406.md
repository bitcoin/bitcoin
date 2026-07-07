P2P and network changes
-----------------------

- The private-broadcast queue (transactions submitted via `sendrawtransaction`
when `-privatebroadcast` is enabled and not yet echoed back from the network)
is now capped at 10,000 entries. When full, new submissions are rejected. It is
up to the caller to inspect the queue via `getprivatebroadcastinfo` and free
up space when stuck via `abortprivatebroadcast`. (#35406)
