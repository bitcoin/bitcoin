v31.0 Release Notes
===================

Bitcoin Core version 31.0 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-31.0/>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the installer
(on Windows) or just copy over `/Applications/Bitcoin-Qt` (on macOS) or
`bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be
migrated. Old wallet versions of Bitcoin Core are generally supported.

Compatibility
==============

Bitcoin Core is supported and tested on the following operating systems or
newer: Linux Kernel 3.17, macOS 14, and Windows 10 (version 1903). Bitcoin Core
should also work on most other Unix-like systems but is not as frequently tested
on them. It is not recommended to use Bitcoin Core on unsupported systems.

Notable changes
===============

The default `-dbcache` value has been increased to 1024 MiB from 450 MiB on
systems where at least 4096 MiB of RAM is detected. This improves performance
but increases memory usage. On some systems (for example when running in
containers), the detected RAM may exceed the memory actually available, which
can lead to out-of-memory conditions. To maintain the previous behavior, set
`-dbcache=450`. See
[reduce-memory.md](https://github.com/bitcoin/bitcoin/blob/master/doc/reduce-memory.md)
for further guidance on low-memory systems. (#34692)

Mempool
-------

The mempool has been reimplemented with a new design ("cluster mempool"), to
facilitate better decision-making when constructing block templates, evicting
transactions, relaying transactions, and validating replacement transactions
(RBF). Most changes should be transparent to users, but some behavior changes
are noted:

- The mempool no longer enforces ancestor or descendant size/count limits.
Instead, two new default policy limits are introduced governing connected
components, or clusters, in the mempool, limiting clusters to 64 transactions
and up to 101 kB in virtual size.  Transactions are considered to be in the same
cluster if they are connected to each other via any combination of parent/child
relationships in the mempool. These limits can be overridden using command-line
arguments; see the extended help (`-help-debug`) for more information.

- Within the mempool, transactions are ordered based on the feerate at which
they are expected to be mined, which takes into account the full set, or
"chunk", of transactions that would be included together (e.g., a parent and its
child, or more complicated subsets of transactions). This ordering is utilized
by the algorithms that implement transaction selection for constructing block
templates; eviction from the mempool when it is full; and transaction relay
announcements to peers.

- The replace-by-fee validation logic has been updated so that transaction
replacements are only accepted if the resulting mempool's feerate diagram is
strictly better than before the replacement. This eliminates all known cases of
replacements occurring that make the mempool worse off, which was possible under
previous RBF rules. For singleton transactions (that are in clusters by
themselves) it's sufficient for a replacement to have a higher fee and feerate
than the original. See [delvingbitcoin.org
post](https://delvingbitcoin.org/t/an-overview-of-the-cluster-mempool-proposal/393#rbf-can-now-be-made-incentive-compatible-for-miners-11)
for more information.

- Two new RPCs have been added: `getmempoolcluster` will provide the set of
transactions in the same cluster as the given transaction, along with the
ordering of those transactions and grouping into chunks; and
`getmempoolfeeratediagram` will return the feerate diagram of the entire
mempool.

- Chunk size and chunk fees are now also included in the output of
`getmempoolentry`.

- The "CPFP Carveout" has been removed from the mempool logic. The CPFP carveout
allowed one additional child transaction to be added to a package that's already
at its descendant limit, but only if that child has exactly one ancestor (the
package's root) and is small (no larger than 10kvB). Nothing is allowed to
bypass the cluster count limit. It is expected that smart contracting use-cases
requiring similar functionality employ TRUC transactions and sibling eviction
instead going forward.

- Some additional discussion can be found at
[doc/policy/mempool-terminology.md](https://github.com/bitcoin/bitcoin/blob/master/doc/policy/mempool-terminology.md)
and
[doc/policy/mempool-replacements.md](https://github.com/bitcoin/bitcoin/blob/master/doc/policy/mempool-replacements.md).

P2P and network changes
-----------------------

- Normally local transactions are broadcast to all connected peers with which we
do transaction relay. Now, for the `sendrawtransaction` RPC this behavior can be
changed to only do the broadcast via the Tor or I2P networks. A new boolean
option `-privatebroadcast` has been added to enable this behavior. This improves
the privacy of the transaction originator in two aspects:
  1. Their IP address (and thus geolocation) is never known to the recipients.
  2. If the originator sends two otherwise unrelated transactions, they will not
  be linkable. This is because a separate connection is used for broadcasting
  each transaction. (#29415)

- New RPCs have been added to introspect and control private broadcast:
`getprivatebroadcastinfo` reports transactions currently being privately
broadcast, and `abortprivatebroadcast` removes matching transactions from the
private broadcast queue. (#34329)

- Transactions participating in one-parent-one-child package relay can now have
the parent with a feerate lower than the `-minrelaytxfee` feerate, even 0 fee.
This expands the change from 28.0 to also cover packages of non-TRUC
transactions. Note that in general the package child can have additional
unconfirmed parents, but they must already be in-mempool for the new package to
be relayed. (#33892)

- The release has asmap data embedded for the first time, allowing the asmap
feature to be used without any externally sourced file. The embedded map [was
created on 2026-03-05](https://github.com/bitcoin/bitcoin/pull/34696). Despite
the data being available, the option remains off-by-default. Users still need to
set `-asmap` or `-asmap=1` explicitly to make it possible to use a peer's ASN
(ISP/hoster identifier) in netgroup bucketing in order to ensure a higher
diversity in their peer set.

Updated RPCs
------------

- `gettxspendingprevout` has 2 new optional arguments: `mempool_only` and
`return_spending_tx`. If `mempool_only` is true it will limit scans to the
mempool even if `txospenderindex` is available. If `return_spending_tx` is true,
the full spending tx will be returned. In addition if `txospenderindex` is
available and a confirmed spending transaction is found, its block hash will be
returned. (#24539)

- The `getpeerinfo` RPC no longer returns the `startingheight` field unless the
configuration option `-deprecatedrpc=startingheight` is used. The
`startingheight` field will be fully removed in the next major release. (#34197)

- The `getblock` RPC now returns a `coinbase_tx` object at verbosity levels 1,
2, and 3. It contains `version`, `locktime`, `sequence`, `coinbase` and
`witness`. This allows for efficiently querying coinbase transaction properties
without fetching the full transaction data at verbosity 2+. (#34512)

REST API
--------

- A new REST API endpoint
(`/rest/blockpart/<BLOCK-HASH>.<bin|hex>?offset=<OFFSET>&size=<SIZE>`) has been
introduced for efficiently fetching a range of bytes from block `<BLOCK-HASH>`.
(#33657)

Build System
------------

- The minimum supported Clang compiler version has been raised to 17.0 (#33555).
- The minimum supported GCC compiler version has been raised to 12.1 (#33842).

Updated settings
----------------

- The `-paytxfee` startup option and the `settxfee` RPC are now deleted after
being deprecated in Bitcoin Core 30.0. They used to allow the user to set a
static fee rate for wallet transactions, which could potentially lead to
overpaying or underpaying. Users should instead rely on fee estimation or
specify a fee rate per transaction using the `fee_rate` argument in RPCs such as
`fundrawtransaction`, `sendtoaddress`, `send`, `sendall`, and `sendmany`.
(#32138)

- Specifying `-asmap` or `-asmap=1` will load the embedded asmap data instead of
an external file. In previous releases, if `-asmap` was specified without a
filename, this would try to load an `ip_asn.map` data file. Now loading an
external asmap file always requires an explicit filename like
`-asmap=ip_asn.map`.

- The `-maxorphantx` startup option has been removed. It was previously
deprecated and has no effect anymore since v30.0. (#33872)

- `tor` has been removed as a network specification. It was deprecated in favour
of `onion` in v0.17.0. (#34031)

- When `-logsourcelocations` is enabled, the log output now contains just the
function name instead of the entire function signature. (#34088)

- The default `-dbcache` value has been increased to `1024` MiB from `450` MiB
on systems where at least `4096` MiB of RAM is detected. This is a performance
increase, but will use more memory. To maintain the previous behaviour, set
`-dbcache=450`. (#34692)

- `-privatebroadcast` is added to enable private broadcast behavior for
`sendrawtransaction`.

New settings
------------

- `-txospenderindex` enables the creation of a transaction output spender index
that, if present, will be scanned by `gettxspendingprevout` if a spending
transaction was not found in the mempool. (#24539)

GUI changes
-----------

- The GUI has been updated to Qt 6.8. (#34650)

- The `createwallet`, `createwalletdescriptor` and `migratewallet` commands are
filtered from the console history to improve security and privacy. (gui#901)

- The Restore Wallet dialog shows an error message if the restored wallet name
is empty. (gui#924)

Fee Estimation
--------------

The Bitcoin Core fee estimator minimum fee rate bucket was updated from **1
sat/vB** to **0.1 sat/vB**, which matches the node’s default `minrelaytxfee`. This
means that for a given confirmation target, if a sub-1 sat/vB fee rate bucket is
the minimum tracked with sufficient data, its average value will be returned as
the fee rate estimate.

Restarting a node with this change invalidates previously saved
estimates in `fee_estimates.dat`, the fee estimator will start tracking fresh
stats.

IPC Interface
-------------

- The IPC mining interface now requires mining clients to use the latest
`mining.capnp` schema. Clients built against older schemas will fail when
calling `Init.makeMining` and receive an RPC error indicating the old mining
interface is no longer supported. Mining clients must update to the latest
schema and regenerate bindings to continue working. (#34568)
- `Mining.createNewBlock` now has a `cooldown` behavior (enabled by default)
that waits for IBD to finish and for the tip to catch up. This usually prevents
a flood of templates during startup, but is not guaranteed. (#34184)
- `Mining.interrupt()` can be used to interrupt `Mining.waitTipChanged` and
`Mining.createNewBlock`. (#34184)
- `Mining.createNewBlock` and `Mining.checkBlock` now require a `context`
parameter.
- `Mining.waitTipChanged` now has a default `timeout` (effectively infinite /
`maxDouble`) if the client omits it.
- `BlockTemplate.getCoinbaseTx()` now returns a structured `CoinbaseTx` instead
of raw bytes.
- Removed `BlockTemplate.getCoinbaseCommitment()` and
`BlockTemplate.getWitnessCommitmentIndex()`.
- Cap’n Proto default values were updated to match the corresponding C++
defaults for mining-related option structs (e.g. `BlockCreateOptions`,
`BlockWaitOptions`, `BlockCheckOptions`).

Credits
=======

Thanks to everyone who directly contributed to this release:

- 0xb10c
- Alexander Wiederin
- Alfonso Roman Zubeldia
- amisha
- ANAVHEOBA
- Andrew Toth
- Anthony Towns
- Antoine Poinsot
- ANtutov
- Anurag chavan
- Ava Chow
- bensig
- Ben Westgate
- billymcbip
- b-l-u-e
- Brandon Odiwuor
- brunoerg
- Bruno Garcia
- Calin Culianu
- Carl Dong
- Chandra Pratap
- Chris Stewart
- Coder
- Cory Fields
- da1sychain
- Daniela Brozzoni
- Daniel Pfeifer
- David Gumberg
- dergoegge
- Dmitry Goncharov
- Enoch Azariah
- Eugene Siegel
- Fabian Jahr
- fanquake
- Fibonacci747
- flack
- frankomosh
- furszy
- glozow
- Greg Sanders
- Hao Xu
- Hennadii Stepanov
- Henry Romp
- Hodlinator
- ismaelsadeeq
- janb84
- jayvaliya
- joaonevess
- John Moffett
- Josh Doman
- kevkevinpal
- l0rinc
- Luke Dashjr
- Mara van der Laan
- MarcoFalke
- marcofleon
- Martin Zumsande
- Matthew Zipkin
- Max Edwards
- Murch
- Musa Haruna
- naiyoma
- nervana21
- Novo
- optout
- pablomartin4btc
- Padraic Slattery
- Pieter Wuille
- Pol Espinasa
- pythcoiner
- rkrux
- Robin David
- Roman Zeyde
- rustaceanrob
- Ryan Ofsky
- SatsAndSports
- scgbckbone
- Sebastian Falbesoner
- sedited
- seduless
- Sergi Delgado Segura
- Sjors Provoost
- SomberNight
- sstone
- stickies-v
- stratospher
- stringintech
- Suhas Daftuar
- tboy1337
- TheCharlatan
- Tim Ruffing
- Vasil Dimov
- w0xlt
- WakeTrainDev
- Weixie Cui
- willcl-ark
- Woolfgm
- yancy
- Yash Bhutwala
- yuvicc
- zaidmstrr

As well as to everyone that helped with translations on
[Transifex](https://explore.transifex.com/bitcoin/bitcoin/).
