Bitcoin Knots version 28.1.knots20250305 is now available from:

  <https://bitcoinknots.org/files/28.x/28.1.knots20250305/>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoinknots/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoinknots.org/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on macOS)
or `bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from very old versions of Bitcoin Core or Knots is possible,
but it might take some time if the data directory needs to be migrated. Old
wallet versions of Bitcoin Knots are generally supported.

Compatibility
==============

Bitcoin Knots is supported on operating systems using the Linux kernel, macOS
11.0+, and Windows 7 and newer. It is not recommended to use Bitcoin Knots on
unsupported systems.

Known Bugs
==========

In various locations, including the GUI's transaction details dialog and the
`"vsize"` result in many RPC results, transaction virtual sizes may not account
for an unusually high number of sigops (ie, as determined by the
`-bytespersigop` policy) or datacarrier penalties (ie, `-datacarriercost`).
This could result in reporting a lower virtual size than is actually used for
mempool or mining purposes.

Due to disruption of the shared Bitcoin Transifex repository, this release
still does not include updated translations, and Bitcoin Knots may be unable
to do so until/unless that is resolved.

Notable changes
===============

P2P and Network Changes
-----------------------

- Previously if Bitcoin Knots was listening for P2P connections, either using
  default settings or via `bind=addr:port` it would always also bind to
  `127.0.0.1:8334` to listen for Tor connections. It was not possible to switch
  this off, even if the node didn't use Tor. This has been changed and now
  `bind=addr:port` results in binding on `addr:port` only. The default behavior
  of binding to `0.0.0.0:8333` and `127.0.0.1:8334` has not been changed.

  If you are using a `bind=...` configuration without `bind=...=onion` and rely
  on the previous implied behavior to accept incoming Tor connections at
  `127.0.0.1:8334`, you need to now make this explicit by using
  `bind=... bind=127.0.0.1:8334=onion`. (#22729)

- When the `-port` configuration option is used, the default onion listening
  port will now be derived to be that port + 1 instead of being set to a fixed
  value (8334 on mainnet). This enables setups with multiple local nodes using
  different `-port` and not using `-bind`.

  Note that a `HiddenServicePort` manually configured in `torrc` may need
  adjustment if used in connection with the `-port` option. For example, if you
  are using `-port=5555` with a non-standard value and not using
  `-bind=...=onion`, previously Bitcoin Knots would listen for incoming Tor
  connections on `127.0.0.1:8334`. Now it would listen on `127.0.0.1:5556`
  (`-port` plus one). If you configured the hidden service manually in torrc
  now you have to change it from `HiddenServicePort 8333 127.0.0.1:8334` to
  `HiddenServicePort 8333 127.0.0.1:5556`, or configure bitcoind with
  `-bind=127.0.0.1:8334=onion` to get the previous behavior. (#31223)

- Bitcoin Knots will now fail to start up if any of its P2P binds fail, rather
  than the previous behaviour where it would only abort startup if all P2P
  binds had failed. (#22729)

- Support for Testnet4 as specified in [BIP94](https://github.com/bitcoin/bips/blob/master/bip-0094.mediawiki)
  has been added. The network can be selected with the `-testnet4` option and
  the section header is also named `[testnet4]`.
  While the intention is to phase out support for Testnet3 in an upcoming
  version, support for it is still available via the known options in this
  release. (#29775)

- UNIX domain sockets can now be used for proxy connections. Set `-onion` or
  `-proxy` to the local socket path with the prefix `unix:` (e.g.
  `-onion=unix:/home/me/torsocket`). (#27375)

- Transactions having a feerate that is too low will be opportunistically
  paired with their child transactions and submitted as a package, thus
  enabling the node to download 1-parent-1-child packages using the existing
  transaction relay protocol. Combined with other mempool policies, this change
  allows limited "package relay" when a parent transaction is below the mempool
  minimum feerate. Warning: this P2P feature is limited (unlike the
  `submitpackage` interface, a child with multiple unconfirmed parents is not
  supported) and not reliable. (#28970)

Mempool Policy Changes
----------------------

- Topologically Restricted Until Confirmation (TRUC) parents are now allowed
  to be below the minimum relay feerate (i.e., pay 0 fees).

- Pay To Anchor (P2A) is a new standard witness output type for spending,
  a newly recognised output template. This allows for key-less anchor
  outputs, with compact spending conditions for additional efficiencies on
  top of an equivalent `sh(OP_TRUE)` output, in addition to the txid stability
  of the spending transaction.
  N.B. propagation of this output spending on the network will be limited
  until a sufficient number of nodes on the network adopt this upgrade.
  (#30352)

- Limited package RBF is now enabled, where the proposed conflicting package
  would result in a connected component, aka cluster, of size 2 in the mempool.
  All clusters being conflicted against must be of size 2 or lower. (#28984)

GUI Changes
-----------

- Transactions no longer show as confirmed after a mere 6 blocks. Instead, the
  confirmation period has been extended to 16 blocks, which is a safer duration
  given the current problematic state of mining centralisation. Note that if
  you wish to be secure against China/Bitmain, you should consider transactions
  unconfirmed for a full week.

- The "Migrate Wallet" menu allows users to migrate any legacy wallet in their
  wallet directory, regardless of the wallets loaded. (gui#824)

- A very basic block visualizer has been added to the Window menu. You can use
  it to see a graphic for any block at a glance, or block templates your node
  is generating for your miner.

Signed Messages
---------------

Bitcoin has the ability for the recipient of bitcoins to a given address to
sign messages, typically intended for use agreeing to terms. Due to confusion,
this feature has often been mis-used in an attempt to prove current ownership
of bitcoins or having sent a Bitcoin transaction. However, these message
signatures do not in fact reflect either ownership or who sent a transaction.
For this reason, message signing was not implemented for Segwit in hopes of a
better standard that never manifested. Nevertheless, being able to sign as the
recipient remains useful in some scenarios, so this version of Bitcoin Knots
extends it to support newer standards:

- Verifying BIP 137, BIP 322, and Electrum signed messages is now supported.

- When signing messages for a Segwit or Taproot address, a BIP 322 signature
  will be produced. (#24058)

JSON-RPC 2.0 Support
--------------------

The JSON-RPC server now recognizes JSON-RPC 2.0 requests and responds with
strict adherence to the [specification](https://www.jsonrpc.org/specification).
See [JSON-RPC-interface.md](https://github.com/bitcoin/bitcoin/blob/master/doc/JSON-RPC-interface.md#json-rpc-11-vs-20) for details. (#27101)

JSON-RPC clients may need to be updated to be compatible with the JSON-RPC
server. Please open an issue on GitHub if any compatibility issues are found.

Updated RPCs
------------

- The `dumptxoutset` RPC now returns the UTXO set dump in a new and improved
  format. Correspondingly, the `loadtxoutset` RPC now expects this new format
  in the dumps it tries to load. Dumps with the old format are no longer
  supported and need to be recreated using the new format to be usable.
  (#29612)

- The `"warnings"` field in `getblockchaininfo`, `getmininginfo` and
  `getnetworkinfo` now returns all the active node warnings as an array
  of strings, instead of a single warning. The current behaviour
  can be temporarily restored by running Bitcoin Knots with the configuration
  option `-deprecatedrpc=warnings`. (#29845)

- Previously when using the `sendrawtransaction` RPC and specifying outputs
  that are already in the UTXO set, an RPC error code of `-27` with the
  message "Transaction already in block chain" was returned in response.
  The error message has been changed to "Transaction outputs already in utxo
  set" to more accurately describe the source of the issue. (#30212)

- The default mode for the `estimatesmartfee` RPC has been updated from
  `conservative` to `economical`, which is expected to reduce over-estimation
  for many users, particularly if Replace-by-Fee is an option. For users that
  require high confidence in their fee estimates at the cost of potentially
  over-estimating, the `conservative` mode remains available. (#30275)

- RPC `submitpackage` now allows 2 new arguments to be passed: `maxfeerate` and
  `maxburnamount`. See the submitpackage help for details. (#28950)

- The `status` action of the `scanblocks` RPC now returns an additional array
  `"relevant_blocks"` containing the matching block hashes found so far during
  a scan. (#30713)

- The `utxoupdatepsbt` method now accepts an optional third parameter,
  `prevtxs`, containing an array of previous transactions (in hex) spent in
  the PSBT being updated. The typical use-case would be when you have a too
  low-fee (perhaps presigned) or timelocked parent transaction where you want
  to sign the child transaction before broadcasting anything. (#30886)

- It is now possible to pass a named pipe (aka fifo) to the `dumptxoutset` RPC
  method. This could be used to transfer the UTXO set to another program, such
  as one which populates a database, without writing the entire UTXO set to
  disk first. (#31560)

- A new field `"cpu_load"` has been added to the `getpeerinfo` RPC output. It
  shows the CPU time (user + system) spent processing messages from the given
  peer and crafting messages for it expressed in per milles (‰) of the duration
  of the connection. The field is optional and will be omitted on platforms
  that do not support this or if still not measured. (#31672)

- The `getblocktemplate` method has been extended to accept new options to
  control template creation: `blockreservedsigops`, `blockreservedsize`, and
  `blockreservedweight` offset the maximum sigops/size/weight put into the
  returned block template, while still respecting the configured limits.

Changes to wallet-related RPCs can be found in the Wallet section below.

New RPCs
--------

- `getdescriptoractivity` can be used to find all spend/receive activity
  relevant to a given set of descriptors within a set of specified blocks. This
  call can be used with `scanblocks` to lessen the need for additional indexing
  programs. (#30708)

- `loadtxoutset` has been added, which allows loading a UTXO snapshot of the
  format generated by `dumptxoutset`. See the AssumeUTXO section below for more
  information.

Updated REST APIs
-----------------

- As with the default mode for the `estimatesmartfee` RPC, the
  `/rest/fee/unset/<TARGET>.json` endpoint has been updated to return estimates
  calculated according to the `economical` mode rather than `conservative`.

Wallet
------

- The wallet now detects when wallet transactions conflict with the mempool.
  Mempool-conflicting transactions can be seen in the `"mempoolconflicts"`
  field of `gettransaction`. The inputs of mempool-conflicted transactions can
  now be respent without manually abandoning the transactions when the parent
  transaction is dropped from the mempool, which can cause wallet balances to
  appear higher. (#27307)

- A new `max_tx_weight` option has been added to the RPCs `fundrawtransaction`,
  `walletcreatefundedpsbt`, and `send`. It specifies the maximum transaction
  weight. If the limit is exceeded during funding, the transaction will not be
  built. The default value is 4,000,000 WU. (#29523)

- A new `createwalletdescriptor` RPC allows users to add new automatically
  generated descriptors to their wallet. This can be used to upgrade wallets
  created prior to the introduction of a new standard descriptor, such as
  taproot. (#29130)

- A new RPC `gethdkeys` lists all of the BIP32 HD keys in use by all of the
  descriptors in the wallet. These keys can be used in conjunction with
  `createwalletdescriptor` to create and add single key descriptors to the
  wallet for a particular key that the wallet already knows. (#29130)

- In RPC `bumpfee`, if a `fee_rate` is specified, the feerate is no longer
  restricted to following the wallet's incremental feerate of 5 sat/vb. The
  feerate must still be at least the sum of the original fee and the mempool's
  incremental feerate. (#27969)

- The `getbalance` RPC method will now throw an error if `avoid_reuse` is set
  together with `dummy=*`. (This combination was never supported, and the
  `avoid_reuse` parameter had previously been silently ignored.)

AssumeUTXO
----------

AssumeUTXO is a new experiemental feature that allows you to make a node usable
quicker, only waiting on the complete sync to provide security. This is done by
using the new `loadtxoutset` RPC method to load a trusted UTXO snapshot. Once
this snapshot is loaded, its contents will be deserialized into a second
chainstate data structure, which is then used to sync to the network's tip.

Meanwhile, the original chainstate will complete the initial block download
process in the background, eventually validating up to the block that the
snapshot is based upon.

The result is a usable node that is current with the network tip in a matter of
minutes rather than hours. However, until the full background sync completes,
the node and any wallets using it remain insecure and should not be trusted or
relied on for confirmation of payment. (#27596)

You can find more information on this process in
[the `assumeutxo` design document](design/assumeutxo.md).

- AssumeUTXO mainnet parameters have been added for height 840,000 and 880,000.
  This means the new `loadtxoutset` RPC can be used only on mainnet with the
  matching UTXO set from one of those heights. (#28553, #31969)

- While the node remains in an incomplete AssumeUTXO state, transactions will
  correctly display as unconfirmed. Applicable RPC methods dealing with
  transactions will return an additional `"confirmations_assumed"` field until
  the background sync has completed. Note that *block* confirmation counts are
  not affected.

- When using assumeutxo with `-prune`, the prune budget may be exceeded if it
  is set lower than 1100MB (i.e. `MIN_DISK_SPACE_FOR_BLOCK_FILES * 2`). Prune
  budget is normally split evenly across each chainstate, unless the resulting
  prune budget per chainstate is beneath `MIN_DISK_SPACE_FOR_BLOCK_FILES` in
  which case that value will be used. (#27596)

CLI Tools
---------

- The `bitcoin-cli -netinfo` command output now includes information about your
  node's and peers' network services. (#30930, #31886)

Build System
------------

- GCC 11.1 or later, or Clang 16.0 or later, are now required to compile
  Bitcoin Knots. (#29091, #30263)

- The minimum required glibc to run Bitcoin Knots is now 2.31. This means that
  RHEL 8 and Ubuntu 18.04 (Bionic) are no-longer supported. (#29987)

- `--enable-lcov-branch-coverage` has been removed, given incompatibilities
  between lcov version 1 & 2. `LCOV_OPTS` should be used to set any options
  instead. (#30192)

Updated Settings
----------------

- When running with `-alertnotify`, an alert can now be raised multiple
  times instead of just once. Previously, it was only raised when unknown
  new consensus rules were activated. Its scope has now been increased to
  include all warnings. Specifically, alerts will now also be raised
  when an invalid chain with a large amount of work has been detected.
  Additional warnings may be added in the future. (#30058)

Changes to GUI or wallet related settings can be found in the GUI or Wallet
section below.

New Settings
------------

- A `pruneduringinit` setting has been added to override the `prune` setting
  only during the initial blockchain sync. It can be useful to set this higher
  to optimise for sync performance at the cost of temporarily higher disk
  usage. (#31845)

Software Expiration
-------------------

Since v0.14.2.knots20170618, each new version of Bitcoin Knots by default
expires 1-2 years after its release. This is a security precaution to help
ensure nodes remain kept up to date. To avoid potential disruption during
holidays, beginning with this version, the expiry date has been moved later,
from January until November.

This is an optional feature. You may disable it by setting `softwareexpiry=0`
in your config file. You may also set `softwareexpiry` to any other POSIX
timestamp, to trigger an expiration at that time instead.

Low-level Changes
=================

RPC
---

- The default for the `rpcthreads` and `rpcworkqueue` settings have been
  increased. This may utilise slightly more system resources, but avoids
  issues with common workloads. (#31215)

Tests
-----

- The BIP94 timewarp attack mitigation is now active on the `regtest` network.
  (#30681)

- A new `-testdatadir` option has been added to `test_bitcoin` to allow
  specifying the location of unit test data directories. (#26564)

Blockstorage
------------

- Block files are now XOR'd by default with a key stored in the blocksdir.
  Previous releases of Bitcoin Knots or previous external software will not be
  able to read the blocksdir with a non-zero XOR-key. Refer to the `-blocksxor`
  help for more details. (#28052)

Chainstate
----------

- The chainstate database flushes that occur when blocks are pruned will no
  longer empty the database cache. The cache will remain populated longer,
  which significantly reduces the time for initial block download to complete.
  (#28280)

Windows Data Directory
----------------------

The default data directory on Windows has been moved from `C:\Users\Username\AppData\Roaming\Bitcoin`
to `C:\Users\Username\AppData\Local\Bitcoin`. Bitcoin Knots will check the
existence of the old directory first and continue to use that directory for
backwards compatibility if it is present. (#27064)

Dependencies
------------

- The dependency on Boost.Process has been replaced with cpp-subprocess, which
  is contained in source. Builders will no longer need Boost.Process to build
  with external signer or Tor subprocess support. (#28981) If you wish to build
  without support for running a dedicated Tor subprocess, you can use the new
  `--disable-tor-subprocess` configure flag.

Credits
=======

Thanks to everyone who contributed to this release:
- 0xb10c
- Alfonso Roman Zubeldia
- Andrew Toth
- AngusP
- Anthony Towns
- Antoine Poinsot
- Anton A
- Ash Manning
- Ava Chow
- Ayush Singh
- Ben Westgate
- Brandon Odiwuor
- brunoerg
- bstin
- CharlesCNorton
- Charlie
- Christopher Bergqvist
- Cory Fields
- crazeteam
- Daniela Brozzoni
- David Benjamin
- David Gumberg
- dergoegge
- Edil Medeiros
- Epic Curious
- eval-exec
- Fabian Jahr
- fanquake
- furszy
- glozow
- Greg Sanders
- hanmz
- Hennadii Stepanov
- Hernan Marino
- Hodlinator
- ishaanam
- ismaelsadeeq
- Jadi
- James O'Beirne
- Jon Atack
- josibake
- jrakibi
- Karl-Johan Alm
- kevkevin
- kevkevinpal
- Konstantin Akimov
- laanwj
- Larry Ruane
- Lőrinc
- ludete
- Luis Schwab
- Luke Dashjr
- MarcoFalke
- marcofleon
- Marnix
- Martin Saposnic
- Martin Zumsande
- Matt Corallo
- Matthew Zipkin
- Matt Whitlock
- Max Edwards
- Michael Dietz
- Michael Little
- Murch
- nanlour
- pablomartin4btc
- Peter Todd
- Pieter Wuille
- @RandyMcMillan
- RoboSchmied
- Roman Zeyde
- Ryan Ofsky
- Sebastian Falbesoner
- Sergi Delgado Segura
- Sjors Provoost
- spicyzboss
- StevenMia
- stickies-v
- stratospher
- Suhas Daftuar
- sunerok
- tdb3
- TheCharlatan
- umiumi
- Vasil Dimov
- virtu
- willcl-ark
