P2P and network changes
-----------------------

- Memory usage of private broadcast (`-privatebroadcast`) is now bounded: for
  each queued transaction only cumulative send/acknowledgment counters are
  kept, plus per-peer details for currently connected private-broadcast peers,
  instead of a record for every broadcast attempt ever made. (#35680)

Updated RPCs
------------

- `getprivatebroadcastinfo` now reports the new fields `num_sent` and
  `num_acknowledged` per transaction, with the total number of send attempts
  and acknowledged sends. The `peers` list now only contains entries for
  currently connected private-broadcast peers instead of every peer the
  transaction was ever sent to. (#35680)
