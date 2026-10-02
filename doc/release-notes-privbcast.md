P2P and network changes
-----------------------

- `-privatebroadcast` now runs each transaction submitted with
  `sendrawtransaction` as a private broadcast job. A job finds peers through
  the DNS seeds, resolved through Tor, connects to a small, fixed number of
  them through Tor on a schedule drawn when it starts, announces the
  transaction and stops. This
  replaces the short-lived connections to Tor and I2P peers taken from the
  address manager. A job does not retry: if the transaction does not arrive,
  submit it again.

- Jobs start in submission order, 35 to 55 seconds (drawn at random) after the
  previous start; a job submitted after that time, with nothing waiting,
  starts at once. A job runs for at most ten minutes, and one whose DNS
  queries find no peer ends at once with the error `no candidates`.
  `setnetworkactive false` (if networking stays off for more than about half
  a second) and shutdown cancel all jobs, and turning networking back on does
  not restart them.

- `-privatebroadcast` now needs a Tor proxy for onion: `-proxy`, `-onion`, or
  `-listenonion` with `-torcontrol`. I2P is no longer used. Startup fails if
  onion cannot become reachable, if the file descriptors the jobs need are not
  available (lower `-maxconnections`), on signet together with
  `-signetseednode` or `-signetchallenge`, on regtest without
  `-privatebroadcastseed`, or off regtest together with a non-zero
  `-mocktime`.
  `-connect` is now allowed, though it turns off listening and with it
  `-listenonion`, so set `-proxy` or `-onion` with it. `-proxyrandomize` no
  longer matters: jobs use fresh proxy credentials for each connection and
  each set of DNS queries. The settings that choose the node's peers
  (`-onlynet`, `-dnsseed`, `-fixedseeds`, `-connect`, `-seednode`,
  `-addnode`) do not apply to jobs.

- `getpeerinfo` no longer reports `private-broadcast` connections, and
  `bitcoin-cli -netinfo` no longer shows the `priv` type: a job's connections
  are not peers of the node.

Updated RPCs
------------

- With `-privatebroadcast`, `sendrawtransaction` queues a job and returns the
  txid. A transaction whose txid is already in the mempool counts as accepted
  and is sent as given; any other is test-accepted first. A transaction whose
  job is still queued or running (same wtxid) is not queued again. The call
  fails with `RPC_LIMIT_EXCEEDED` when the job cannot be queued (queue full,
  networking disabled, node shutting down) and with `RPC_MISC_ERROR` when no
  Tor proxy is configured.

- `getprivatebroadcastinfo` now returns `jobs` instead of `transactions`: the
  last 100 finished jobs, oldest first, then the running and the queued ones.
  Each job has `txid`, `wtxid`, `state`, `time_added`, `time_started`,
  `time_ended`, `seen_in_mempool`, `error`, `progress` while it runs, and
  `announced` and the job's `report` once it has run. The report names every
  peer dialled. The `hex`, `attempts_remaining` and `peers` fields are gone.
  Jobs and reports are kept in memory only.

- `abortprivatebroadcast` aborts every queued or running job whose txid or
  wtxid matches. A queued job is dropped at once; a running one stops shortly
  and still ends with a report. Each entry of `removed_transactions` has a new
  `state` field, `aborted` or `running`.

See [Private broadcast](/doc/private-broadcast.md).
