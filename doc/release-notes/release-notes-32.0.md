v32.0 Release Notes
===================

Bitcoin Core version 32.0 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-32.0/>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

With the release of this new major version, versions 29.x and
older are at "End of Life" and will no longer receive updates.

In accordance with the security policy, we will in two weeks disclose:

* Medium and high severity vulnerabilities fixed in 30.0. There are 3 of these.

* Low severity vulnerabilities fixed in 32.0. There are 0 of these.

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on macOS)
or `bin/` and `libexec/` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of Bitcoin Core are generally supported.

Compatibility
==============

Bitcoin Core is supported and tested on the following operating systems or newer:
Linux Kernel 3.17, macOS 14, and Windows 10 (version 1903). Bitcoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them. It is not recommended to use Bitcoin Core on
unsupported systems.

Notable changes
===============

P2P and network changes
-----------------------

- Tor hidden services that are created automatically by Bitcoin Core will have
  [PoW defenses](https://tpo.pages.torproject.net/onion-services/ecosystem/technology/security/pow/)
  enabled if the Tor daemon supports that. (#33414)
- To reduce memory and CPU usage during periods of high transaction volume,
  rate-limiting of outgoing transaction relay has been changed to use a global
  backlog instead of being done on a per-peer basis. The default rate-limit
  remains as 14 tx/s (boosted by 2.5x for outbound peers), though this can be
  changed via the `-txsendrate` configuration option. An additional bandwidth
  rate-limit has also been introduced at 12MB of transactions per 10 minutes,
  with a high burst rate. The size of the global backlog and the token bucket
  values for the rate limits can be queried via the `getnetworkinfo` RPC.
  (#34628)
- Fix a possible leak of the originator's IP address for transactions sent with
  `sendrawtransaction` RPC when `-privatebroadcast=1`. When Bitcoin Core
  connects to a peer, if that peer has been advertised to support P2P protocol
  v2, then Bitcoin Core tries to use the v2 protocol and if that fails it
  retries the connection using the v1 protocol. When the private broadcast is
  about to send a transaction to an IPv4 or IPv6 peer it overrides the normal
  proxy selection and forces the connection through the Tor proxy (and thus
  through the Tor network, protecting the sender's IP address). However if v2
  protocol is tried and it fails, then the v1 retry connection would be made
  disregarding the "override proxy" request, possibly making an IPv4 or IPv6
  direct connection. In other words, for this to happen the following must be
  true:
  1. An IPv4 or IPv6 address has been advertised to the sender, indicating v2
     support.
  2. The sender must have Tor configured.
  3. The sender's configuration must be such that connections to IPv4 or IPv6
     are made directly (no `-proxy=` is used for IPv4 or IPv6).
  4. The recipient does not support v2 (the flags from 1. are bogus). (#35410)
- The private-broadcast queue (transactions submitted via `sendrawtransaction`
  when `-privatebroadcast` is enabled and not yet echoed back from the network)
  is now capped at 10,000 entries. When full, new submissions are rejected. It
  is up to the caller to inspect the queue via `getprivatebroadcastinfo` and
  free up space when stuck via `abortprivatebroadcast`. (#35406)
- Each transaction sent via private broadcast (`-privatebroadcast`) is limited
  to 1,000 send attempts. After reaching the limit, broadcasting stops; call
  `sendrawtransaction` again to retry. Transactions that reach the limit remain
  available through `getprivatebroadcastinfo` and `abortprivatebroadcast`.
  (#35680)
- Support for the legacy ElGamal (type 0) encryption type when creating I2P
  sessions is being sunset by the I2P network and will be removed from bitcoind
  on or before v34. Nodes using I2P with versions of Bitcoin Core earlier than
  v26.1 (PRs #29200, #29209) will soon only be able to connect to other legacy
  ElGamal I2P peers and will be increasingly isolated from the rest of the
  network, with a reduced anonymity set. See #35696 for details. (#35696)
- The project is currently considering whether to drop support for CJDNS (see
  discussion in #36041). Since its introduction in v23.0, it has not gained
  substantial adoption amongst nodes, and there is some concern about supporting
  a network with such a low number of peers. (#36202)
- The CJDNS documentation was updated in #34811 to simplify the setup / peer
  discovery process. (#36202)
- The default maximum number of automatic peer connections (`-maxconnections`)
  has increased from 125 to 200, providing additional inbound capacity for
  block-relay-only peers. By default, transaction-relaying peers may use at most
  50% of inbound connection slots. This percentage can be configured with the
  new `-inboundrelaypercent` option, which accepts values from 0 to 100 and
  defaults to 50. The number of automatic outbound connections is unchanged.
  (#28463)
- Explicitly configured `-maxconnections` values remain unchanged, but the new
  inbound transaction-relay limit still applies. To retain the previous
  connection limits, use `-maxconnections=125 -inboundrelaypercent=100`.

Mempool
-------

- mempoolfullrbf=1 behaviour has been the default since v28 and the argument has
  been removed since v29 subsequently. The `getmempoolinfo` RPC stops returning
  the deprecated `fullrbf` key in the response unless the user requests it via
  the `-deprecatedrpc=fullrbf` node argument. Also, the `bip125-replaceable` key
  is removed from the mempool RPCs responses (because it, too, has been
  deprecated since v29) unless the user requests it via `-deprecatedrpc=bip125`
  node argument. Affected mempool RPCs are `getrawmempool`,
  `getmempoolancestors`, `getmempooldescendants`, and `getmempoolentry`.
  (#34911)

Updated RPCs
------------

- `createpsbt`, `walletcreatefundedpsbt`, `converttopsbt`, and `psbtbumpfee`
  will now default to creating version 2 PSBTs. An optional `psbt_version`
  argument is added to these RPCs which allows specifying the version of PSBT to
  create. (#21283)
- 'taproot' has been removed from 'getdeploymentinfo' because its historical
  activation height is no longer used anywhere in the codebase. Applications
  that rely on `deployments.taproot` to detect Taproot support should either
  assume Taproot to be always active (since Bitcoin Core v24.0), or check for
  `TAPROOT` in the `script_flags` array returned by `getdeploymentinfo` (since
  Bitcoin Core v31.0). (#26201)
- Bitcoin Core now reports a debug message explaining why transaction inputs are
  non-standard. This information is now returned in the responses of the
  transaction-sending RPCs `submitpackage`, `sendrawtransaction`, and
  `testmempoolaccept`, and is also logged to `debug.log` (if `mempoolrej` +
  debug category is enabled) when such transactions are received over the P2P
  network. This does not change the existing error code
  `bad-txns-nonstandard-inputs`, but instead adds additional debug information
  to it. (#29060)
- The `dumptxoutset` RPC now supports writing to a named pipe on UNIX-like
  systems (see mkfifo(1) and mkfifo(3) man pages). This allows the raw UTXO set
  data to be consumed directly by another process (e.g., the
  `contrib/utxo-tools/utxo_to_sqlite.py` conversion script) without first
  writing it to disk. (#31560)
- Mempool RPCs (`getrawmempool`, `getmempoolentry`, `testmempoolaccept`,
  `submitpackage`) now include an additional field `vsize_adjusted` (which is
  the sigop-adjusted virtual size used for policy) and `vsize_bip141` (which
  represents the raw BIP141 virtual size). While `vsize` is marked as
  DEPRECATED, it was previously erroneously described as the BIP 141 vsize, but
  is actually sigops-adjusted vsize. Use `vsize_bip141` to actually get that
  behavior or switch to the explicit `vsize_adjusted` for retained behavior.
  (#32800)
- `getrawtransaction` RPC now includes an additional field `vsize_adjusted`,
  which is the sigop-adjusted virtual size if the transaction is in the mempool.
  (#32800)
- The `getblockchaininfo` RPC now exposes progress for background validation if
  the `assumeutxo` feature is used. Once a node has synced from snapshot to tip,
  `verificationprogress` returns 1.0 and `initialblockdownload` false even
  though the node may still be validating blocks in the background. A new
  object, `backgroundvalidation`, provides details about the snapshot being
  validated, including snapshot height, number of blocks processed, best block
  hash, chainwork, median time, and verification progress. (#33259)
- `getbalances` now includes a `nonmempool` field under `mine`, reporting the
  net balance of wallet transactions that are not in the mempool (e.g. due to
  too low a feerate, too many unconfirmed ancestors, or a too-large `OP_RETURN`
  output). This value is always negative, and is typically an over-estimate
  since it accounts for coins being spent but not for change outputs returning
  to the wallet (which are also outside the mempool). Without this field, funds
  involved in non-mempool transactions could appear to go missing from the
  wallet balance. (#33671)
- The `estimatesmartfee` RPC now combines two fee rate estimators: the existing
  block policy fee rate estimator and a new mempool fee rate estimator. (#34075)
- The new mempool fee rate estimator produces conservative and economical fee
  rate estimates from the current contents of the mempool. It only produces a
  fee rate estimate when recent blocks indicate a healthy mempool, and falls
  back to the higher of the minimum relay fee rate and the current mempool
  minimum fee rate when the mempool is too sparse. Its statistics are persisted
  to `fees/mempool_policy_estimator.dat` and reloaded on startup.
  `estimatesmartfee` returns the lower of the two fee rate estimators' results,
  so the mempool fee rate estimator can only lower the block policy fee rate
  estimate. (#34075)
- `estimatesmartfee` accepts an `options` object with `fee_rate_estimator`.
  Recognized values are `"auto"` (the default, combined behavior described
  above), `"block_policy"` (use only the block policy fee rate estimator), and
  `"mempool_policy"` (use only the mempool fee rate estimator). All unknown
  values are rejected. Users who want the previous behavior can
  select the block policy fee rate estimator explicitly. (#34075)
- The options object also accepts `verbosity`. A verbosity of `2` or higher also
  returns `mempool_health_statistics`. (#34075)
- When `fee_rate_estimator` is `"auto"` and the estimate succeeds, the response
  also includes an `estimator` field identifying which fee rate estimator
  produced the result. (#34075)
- Block policy fee estimator data is now stored in
  `fees/block_policy_estimates.dat`. If the new file does not exist, the legacy
  `fee_estimates.dat` file is moved to the new path during startup. If both
  files exist, the legacy file is removed. (#34075)
- Wallet fee rate estimation uses the default combined estimate. (#34075)

- The `-deprecatedrpc=startingheight` configuration option has been removed. The
  `getpeerinfo` RPC no longer returns the `startingheight` field, which was
  previously deprecated in v31.0. (#34796)
- The `migratewallet` RPC now gives the option to not load the descriptor wallet
  after migrating it from a legacy wallet. This will now allow pruned nodes to
  migrate a wallet out of sync below the pruning height. Note that to use the
  new wallet it must be loaded to a full node anyway. (#35266)
- The `getprivatebroadcastinfo` and `abortprivatebroadcast` RPCs now return an
  error `-32601`, "Method not found", when `-privatebroadcast` is not enabled at
  startup. (#35267)
- `gettransaction`, `listtransactions`, and `listsinceblock` now have an
  `alternate_wtxids` field which lists the wtxids of all transactions that have
  the same txid. When there is only one known witness variant the field is an
  empty array, analogous to `walletconflicts` and `mempoolconflicts`. (#35501)
- `getprivatebroadcastinfo` now reports an `attempts_remaining` field for each
  transaction. (#35680)
- `getnetworkinfo` now reports the `asmap_version` if `-asmap` is in use. The
  version is the hash of the asmap data and can be verified against the hash of
  the raw asmap data file that also gets attested in the
  [asmap-data](https://github.com/bitcoin-core/asmap-data) repository with
  `sha256sum`. (#36249)

Changes to wallet related RPCs can be found in the Wallet section below.

New RPCs
--------

- A new `exportasmap` RPC writes the ASMap data embedded at build time to a
  file. (#33920)
- A new `getopenrpcinfo` RPC returns a machine-readable OpenRPC 1.4.1
  description of the RPCs available on the running node, including their
  parameters and result schemas. The new `rpc.discover` method provides the same
  public document using OpenRPC’s standard service discovery method. To include
  hidden commands and arguments, call `getopenrpcinfo` with `show_hidden=true`.
  (#34683)

Build System
------------

- The next major release is planned to drop support for building Bitcoin Core on
  Windows with MSVC, once clang-cl support is ready. See
  [#36153](https://github.com/bitcoin/bitcoin/issues/36153).
- The undocumented `BITCOIN_GENBUILD_NO_GIT` environment variable is no longer
  required and has been removed. The build system now automatically detects when
  it is being built from a source archive or as a subproject of a git-aware
  parent project and skips git metadata fetching. Users who need to disable git
  execution explicitly can still do so by configuring with
  `-DCMAKE_DISABLE_FIND_PACKAGE_Git=ON`. (#32220)

Updated settings
----------------

- The `bip125-replaceable` key in the wallet transaction RPCs such as
  `listtransactions`, `listsinceblock`, and `gettransaction` is marked as
  deprecated. Users still have the option to retrieve this key by passing the
  `-deprecatedrpc=bip125` startup option. Also, the `-walletrbf` startup option
  has been marked as deprecated and will be fully removed in the next release.
  Using this option emits a warning in the logs. (#34917)
- The `libevent` logging category has been removed. Configurations like
  `-debug=libevent` or `-debugexclude=libevent` will log a deprecation warning
  and be ignored. These configurations will result in an error in a future
  release.
- A new configuration option `-rpcmaxconnections` (default `16`) limits the
  number of simultaneously connected HTTP clients to the server. The application
  will now attempt to reserve file descriptors for the HTTP server sockets. If
  your system has limited resources, consider using a lower setting.
- A new `mining` logging category has been added, enabled with `-debug=mining`.
  The `CreateNewBlock(): block weight: ...` line, previously logged on every
  block template build regardless of logging configuration, is now logged only
  when this category is enabled. `getblocktemplate` and Mining IPC users are
  encouraged to enable it to see this log and future mining log.

Changes to GUI or wallet related settings can be found in the GUI or Wallet section below.

New settings
------------

- Block validation can now prefetch input prevouts from the chainstate database
  in parallel while connecting blocks, speeding up validation when prevouts need
  to be read from disk. A new `-prevoutfetchthreads=<n>` option controls the
  number of prefetch worker threads. The default is 8 threads, up to a maximum
  of 16; set it to 0 to disable parallel prefetching. (#35295)

Tools and Utilities
-------------------

- CLI -addrinfo now returns the full set of known addresses. In previous
  versions (v22.0 - v30.0) the set of returned addresses was filtered for
  quality and recency. This was changed since it does not match the logic for
  selecting peers to connect to, which does not filter. Note: CLI -addrinfo now
  requires bitcoind v26.0 or later, as it uses the getaddrmaninfo RPC
  internally. Users querying older, unmaintained node versions would need to use
  an older bitcoin-cli version. (#26988)
- A new `bitcoin-util getchainparams` command returns hardcoded details about
  the selected chain. (#35610)

Wallet
------

- A new RPC `addhdkey` is added which allows a BIP 32 extended key to be added
  to the wallet without needing to import it as part of a separate descriptor.
  This key will not be used to produce any output scripts unless it is
  explicitly imported as part of a separate descriptor independent of the
  `addhdkey` RPC. (#29136)
- A new `exportwatchonlywallet` RPC creates a watchonly wallet file from an
  existing descriptor wallet. The exported file contains the wallet's public
  descriptors (with derived key caches where needed), transactions, and address
  book data, but no private keys. It can be imported on another node using the
  existing `restorewallet` RPC. (#32489)
- The [Offline Signing Tutorial](/doc/offline-signing-tutorial.md) has been
  updated to use `exportwatchonlywallet` for setting up the online watch-only
  wallet, replacing the previous manual descriptor import workflow. (#32489)
- A new `derivehdkey` RPC is available to obtain an xpub or xprv for a
  derivation path with at least one hardened step from an HD key known to the
  wallet. This can be used to coordinate a multisig setup, where each signer
  shares an xpub using a different derivation path than the default
  single-signature descriptors. The example in `doc/multisig-tutorial.md` is
  updated to use this RPC. (#32784)
- The `fee_reason` field returned by wallet transaction creation RPCs now
  reports the reason the wallet selected the fee rate ("Fee Rate Estimator",
  "Mempool Min Fee", "User Specified Fee", "Fallback fee" or
  "Minimum Required Fee") instead of the block policy fee rate estimator's
  internal threshold details. Those details remain available in the block
  policy fee rate estimator debug log. (#34075)
- Wallet names that are relative paths including `..` and `.` elements, and
  wallets named `/` are no longer allowed. Any users that depended on this
  behavior can instead use absolute paths to their wallet or move their wallet
  to a safer path. (#34544)
- The `send` and `sendall` RPCs are no longer marked as experimental. (#35601)
- The `fundrawtransaction` RPC no longer accepts a boolean as the second
  positional argument. This silent no-op fallback was removed and the argument
  is now fully type checked. Passing a boolean will raise an error. (#35836)
- On non-Windows systems, an authenticated RPC caller allowed to create wallets
  could execute arbitrary commands as the node process account when
  `-walletnotify` was configured, by crafting a wallet name with regex
  replacement characters. Wallet notification placeholder replacement now treats
  wallet names literally. (#36048)
- The `removeprunedfunds` RPC has been deprecated and will be removed in the
  next major release. In order to continue using it, `bitcoind` must be started
  with the `-deprecatedrpc=removeprunedfunds` option. (#35605)

GUI changes
-----------

- A menu action has been added to allow creating a watchonly wallet file from an
  existing descriptor wallet. This option mirrors the `exportwatchonlywallet`
  RPC - the exported file can be imported to another node using the Restore
  Wallet menu action. (gui#872)
- The "Enable Replace-By-Fee" checkbox has been removed from the Send tab, and
  the send confirmation dialog no longer states whether a transaction signals
  BIP 125 replaceability. Transactions created in the GUI now follow the wallet's
  `-walletrbf` setting, which defaults to signalling replaceability and is
  deprecated in this release (see Updated settings). (gui#936)
- The migrate wallet option now allows to disable wallet loading after
  migrating. It is useful in case the node is pruned and the wallet was created
  before the pruned height. (gui#953)
- The GUI is now built with Qt 6.8.4. (#35709)

Mining
------

- The IPC mining interface now rejects out-of-range block template options
  instead of silently clamping them, such as oversized reserved block weight or
  coinbase sigops limits. (#33966)
- The `-blockmaxweight` startup option is now rejected when it is lower than
  `-blockreservedweight`, instead of being silently clamped. (#33966)


IPC Interface
-------------

- `BlockTemplate.submitSolution` now returns `reason` and `debug` rejection
  details in addition to the boolean result. Clients must regenerate IPC
  bindings from the updated `mining.capnp` schema to use the new method. The
  previous `@7` method now returns an error directing clients to update.
  (#34672)
- `BlockTemplate.submitSolution` now reports duplicate blocks as failures with
  `reason="duplicate"`, matching `Mining.submitBlock`, instead of returning
  success for duplicate submissions. (#34672)

Logging
-------

- BIP 9 bits 5 to 28 inclusive are now ignored for soft fork signaling, as per
  BIP 323. We won't warn about unknown deployments when receiving blocks that
  set any of those bits in their version. (#34779)

REST API
--------

- REST responses now include `Cache-Control` headers to guide intermediary
  caches. Immutable responses such as block binary and hex data, block parts,
  block filters, spent transaction outputs, and block-specific deployment info
  are marked cacheable for one day. Responses that can change with active chain
  or node state, as well as errors, are marked `no-store`. (#34794)

Low-level changes
=================

Indexes
-------

- The transaction index (`-txindex`) now stores less data on disk; a fully
  rebuilt index takes less than half the space. The index is backwards
  compatible, so existing users will not see the space saving unless the index
  is recreated. To do so, stop the node, delete the `<datadir>/indexes/txindex`
  directory, and restart; rebuilding can take up to a few hours depending on
  hardware. Progress can be monitored using the `getindexinfo` RPC. Once
  rebuilt, the index can no longer be read by previous releases, so downgrading
  will rebuild it again in the old format. When downgrading permanently, delete
  the `<datadir>/indexes/txindex` directory first, since previous releases do
  not reclaim the space used by entries in the new format. (#35531)
- The transaction output spender index (`-txospenderindex`) now uses less disk
  space. Existing indexes remain compatible and no action is required. To
  reclaim the space for data indexed before upgrading, stop the node, delete the
  `<datadir>/indexes/txospenderindex/` directory, and restart with
  `-txospenderindex` enabled to rebuild it. (#35634, #35568)

RPC
---

- Libevent has been replaced with a custom HTTP server (#35182). Certain HTTP
  edge cases will observe different behavior to be more RFC-compliant:
  - Stricter enforcement of maximum headers size (8192 bytes)
  - Reject requests with whitespace in header field-names
  - "Line Folding" is rejected (whitespace at start of a header line)
  - Tolerate `%` at the end of requested URLs
  - Multiple "Content-Length" headers with different values are rejected
- Clients attempting to connect from addresses not allowed by the `-rpcallowip`
  option (or its default, `localhost`) will now be immediately disconnected
  instead of receiving a `403 Forbidden`. (#35592)

Credits
=======

Thanks to everyone who directly contributed to this release:

- 0xb10c
- Adam Andrews
- Alexander Wiederin
- Alfonso Roman Zubeldia
- ameen-alam
- amisha
- Amiti Uttarwar
- Andreas Kouloumos
- Andrew Toth
- Anthony Towns
- Anthropic Security
- Antoine Poinsot
- ArvinFarrelP
- Ava Chow
- azuchi
- benthecarman
- Ben Woosley
- Bicaru20
- b-l-u-e
- Bortlesboat
- Brandon Odiwuor
- Bruno Garcia
- CaesarCoder
- Chandra Pratap
- codeabysss
- Cory Fields
- Craig Raw
- crStiv
- csjones
- cyb3ralbert
- Daniela Brozzoni
- Daniel Pfeifer
- Danny van Heumen
- David Gumberg
- deadmanoz
- dergoegge
- ekzyis
- Enoch Azariah
- Eugene Siegel
- Evan Sultanik
- Fabian Jahr
- fanquake
- frankomosh
- Gloria Zhao
- Greg Sanders
- Guillermo Fernandes
- GuTS805
- Hao Xu
- Hennadii Stepanov
- Henry Romp
- Hodlinator
- HouseOfHufflepuff
- ImMike
- ismaelsadeeq
- janb84
- jeanpablojp
- Jean-Philippe Aumasson
- joaonevess
- Jon Atack
- jpk68
- jrakibi
- junbyjun1238
- Kanan
- kevkevinpal
- Kyle 🐆
- l0rinc
- Laxman Acharya
- littleyier
- Luke Dashjr
- maflcko
- marcofleon
- Martin Zumsande
- Matias Furszyfer
- Matthew Zipkin
- Max Edwards
- Mccalabrese
- Memetic Money
- Michael Dietz
- Muhammad
- Murch
- Musa Haruna
- naiyoma
- nebula-21
- nervana21
- Novo
- optout
- Pablo Martin
- Peter Zafonte
- Philips Ucheolisah .N
- Pieter Wuille
- Pol Espinasa
- pseudoramdom
- pythcoiner
- Renato Britto
- rkrux
- Rob Hamilton
- Robin David
- Roqqit
- Ruslan Kasheparov
- Russell Yanofsky
- rustaceanrob
- Sanjana2906
- satsfy
- sayed nabhan
- Sebastian Falbesoner
- Sebastian van Staa
- sedited
- seduless
- shaurya2k06
- Shrey
- Shuvam Pandey
- Sjors Provoost
- stickies-v
- stratospher
- stringintech
- taboger
- takeshikurosawaa
- Thomas
- Torkel Rogstad
- Vasil Dimov
- vicjuma
- ViniciusCestarii
- w0xlt
- Will Clark
- winterrdog
- xyzconstant
- Yancy
- yuvicc

As well as to everyone that helped with translations on
[Transifex](https://explore.transifex.com/bitcoin/bitcoin/).
