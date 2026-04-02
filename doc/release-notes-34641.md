Updated `dbcache` default settings
----------------------------------

- When `-dbcache` is not set explicitly, Bitcoin Core now chooses a RAM-aware default between 100 MiB and 2 GiB.
  The selected `-dbcache` value is still used for both IBD and steady-state operation and unused mempool allocation may be shared with this cache.
  In environments with external memory limits (e.g. containers), automatic sizing may not match effective limits.
  Set `-dbcache` explicitly if needed, to maintain the previous behavior, set `-dbcache=450`. (#34641)
