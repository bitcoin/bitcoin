v30.0 Release Notes
===================

Bitcoin Core version v30.0 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-30.0/>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on macOS)
or `bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of Bitcoin Core are generally supported.

Compatibility
==============

Bitcoin Core is supported and tested on operating systems using the
Linux Kernel 3.17+, macOS 13+, and Windows 10+. Bitcoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them. It is not recommended to use Bitcoin Core on
unsupported systems.

Notable changes
===============

Policy
------

- The maximum number of potentially executed legacy signature operations in a
  single standard transaction is now limited to 2500. Signature operations in all
  previous output scripts, in all input scripts, as well as all P2SH redeem
  scripts (if there are any) are counted toward the limit. The new limit is
  assumed to not affect any known typically formed standard transactions. The
  change was done to prepare for a possible BIP54 deployment in the future. (#32521)

- `-datacarriersize` is increased to 100,000 by default, which effectively uncaps
  the limit (as the maximum transaction size limit will be hit first). It can be
  overridden with `-datacarriersize=83` to revert to the limit enforced in previous
  versions. (#32406)

- Multiple data carrier (OP_RETURN) outputs in a transaction are now permitted for
  relay and mining. The `-datacarriersize` limit applies to the aggregate size of
  the scriptPubKeys across all such outputs in a transaction, not including the
  scriptPubKey size itself. (#32406)

- The minimum block feerate (`-blockmintxfee`) has been changed to 0.001 satoshi per
  vB. It can still be changed using the configuration option. This option can be used
  by miners to set a minimum feerate on packages added to block templates. (#33106)

- The default minimum relay feerate (`-minrelaytxfee`) and incremental relay feerate
  (`-incrementalrelayfee`) have been changed to 0.1 satoshis per vB. They can still
  be changed using their respective configuration options, but it is recommended to
  change both together if you decide to do so. (#33106)

  Other minimum feerates (e.g. the dust feerate, the minimum returned by the fee
  estimator, and all feerates used by the wallet) remain unchanged. The mempool minimum
  feerate still changes in response to high volume.

  Note that unless these lower defaults are widely adopted across the network, transactions
  created with lower fee rates are not guaranteed to propagate or confirm. The wallet
  feerates remain unchanged; `-mintxfee` must be changed before attempting to create
  transactions with lower feerates using the wallet. (#33106)

P2P and network changes
-----------------------

- Opportunistic 1-parent-1-child package relay has been improved to handle
  situations when the child already has unconfirmed parent(s) in the mempool.
  This means that 1p1c packages can be accepted and propagate, even if they are
  connected to broader topologies: multi-parent-1-child (where only 1 parent
  requires fee-bumping), grandparent-parent-child (where only parent requires
  fee-bumping) etc. (#31385)

- The transaction orphanage, which holds transactions with missing inputs temporarily
  while the node attempts to fetch its parents, now has improved Denial of Service protections.
  Previously, it enforced a maximum number of unique transactions (default 100,
  configurable using `-maxorphantx`). Now, its limits are as follows: the number of
  entries (unique by wtxid and peer), plus each unique transaction's input count divided
  by 10, must not exceed 3,000. The total weight of unique transactions must not exceed
  `404,000` Wu multiplied by the number of peers. (#31829)

- The `-maxorphantx` option no longer has any effect, since the orphanage no longer
  limits the number of unique transactions. Users should remove this configuration
  option if they were using it, as the setting will cause an error in future versions
  when it is no longer recognized. (#31829)

New `bitcoin` command
---------------------

- A new `bitcoin` command line tool has been added to make features more discoverable
  and convenient to use. The `bitcoin` tool just calls other executables and does not
  implement any functionality on its own. Specifically `bitcoin node` is a synonym for
  `bitcoind`, `bitcoin gui` is a synonym for `bitcoin-qt`, and `bitcoin rpc` is a synonym
  for `bitcoin-cli -named`. Other commands and options can be listed with `bitcoin help`.
  The new `bitcoin` command is an alternative to calling other commands directly, but it
  doesn't replace them, and there are no plans to deprecate existing commands. (#31375)

External Signing
----------------

- Support for external signing on Windows has been re-enabled. (#29868)

IPC Mining Interface
--------------------

- The new `bitcoin` command does support one new feature: an (experimental) IPC Mining
  Interface that allows the node to work with Stratum v2 or other mining client software,
  see (#31098). When the node is started with `bitcoin -m node -ipcbind=unix` it will
  listen on a unix socket for IPC client connections, allowing clients to request block
  templates and submit mined blocks. The `-m` option launches a new internal binary
  (`bitcoin-node` instead of `bitcoind`) and is currently required but will become optional
  in the future (with [#33229](https://github.com/bitcoin/bitcoin/pull/33229)).

- IPC connectivity introduces new dependencies (see [multiprocess.md](https://github.com/bitcoin/bitcoin/blob/master/doc/multiprocess.md)),
  which can be turned off with the `-DENABLE_IPC=OFF` build option if you do not intend
  to use IPC. (#31802)

Install changes
---------------

- The `test_bitcoin` executable is now installed in `libexec/` instead of `bin/`.
  It can still be executed directly, or accessed through the new `bitcoin` command
  as `bitcoin test`. The `libexec/` directory also contains new `bitcoin-node` and
  `bitcoin-gui` binaries which support IPC features and are called through the
  `bitcoin` tool. In source builds only, `test_bitcoin-qt`, `bench_bitcoin`, and
  `bitcoin-chainstate` are also now installed to `libexec/` instead of `bin/` and
  can be accessed through the new `bitcoin` command. See `bitcoin help` output for
  details. (#31679)

- On Windows, the installer no longer adds a “(64-bit)” suffix to entries in the
  Start Menu (#32132), and it now automatically removes obsolete artifacts during
  upgrades (#33422).

Indexes
-------

- The implementation of coinstatsindex was changed to prevent an overflow bug that
  could already be observed on the default Signet. The new version of the index will
  need to be synced from scratch when starting the upgraded node for the first time.

  The new version is stored in `/indexes/coinstatsindex/` in contrast to the old version
  which was stored at `/indexes/coinstats/`. The old version of the index is not deleted
  by the upgraded node in case the user chooses to downgrade their node in the future.
  If the user does not plan to downgrade it is safe for them to remove `/indexes/coinstats/`
  from their datadir. A future release of Bitcoin Core may remove the old version of the
  index automatically. (#30469)

Logging
-------
- Unconditional logging to disk is now rate limited by giving each source location
  a quota of 1MiB per hour. Unconditional logging is any logging with a log level
  higher than debug, that is `info`, `warning`, and `error`. All logs will be
  prefixed with `[*]` if there is at least one source location that is currently
  being suppressed. (#32604)

- When `-logsourcelocations` is enabled, the log output now contains the entire
  function signature instead of just the function name. (#32604)

Updated RPCs
------------

- The `-paytxfee` startup option and the `settxfee` RPC are now deprecated and
  will be removed in Bitcoin Core 31.0. They allowed the user to set a static fee
  rate for wallet transactions, which could potentially lead to overpaying or underpaying.
  Users should instead rely on fee estimation or specify a fee rate per transaction
  using the `fee_rate` argument in RPCs such as `fundrawtransaction`, `sendtoaddress`,
  `send`, `sendall`, and `sendmany`. (#31278)

- Any RPC in which one of the parameters is a descriptor will throw an error
  if the provided descriptor contains a whitespace at the beginning or the end
  of the public key within a fragment - e.g. `pk( KEY)` or `pk(KEY )`. (#31603)

- The `submitpackage` RPC, which allows submissions of child-with-parents
  packages, no longer requires that all unconfirmed parents be present. The
  package may contain other in-mempool ancestors as well. (#31385)

- The `waitfornewblock` RPC now takes an optional `current_tip` argument. It
  is also no longer hidden. (#30635)

- The `waitforblock` and `waitforblockheight` RPCs are no longer hidden.  (#30635)

- The `psbtbumpfee` and `bumpfee` RPCs allow a replacement under fullrbf and no
  longer require BIP-125 signalling. (#31953)

- Transaction Script validation errors used to return the reason for the error
  prefixed by either `mandatory-script-verify-flag-failed` if it was a consensus
  error, or `non-mandatory-script-verify-flag` (without "-failed") if it was a
  standardness error. This has been changed to `block-script-verify-flag-failed`
  and `mempool-script-verify-flag-failed` for all block and mempool errors
  respectively. (#33183)

- The `getmininginfo` RPC now returns "blockmintxfee" result specifying the value of
  `-blockmintxfee` configuration. (#33189)

- The `getmempoolinfo` RPC now returns an additional "permitbaremultisig" and
  "maxdatacarriersize" field, reflecting the `-permitbaremultisig` and `-datacarriersize`
  config values. (#29954)

Changes to wallet-related RPCs can be found in the Wallet section below.

New RPCs
--------

- A new REST API endpoint (`/rest/spenttxouts/BLOCKHASH`) has been introduced for
  efficiently fetching spent transaction outputs using the block's undo data (#32540).

Build System
------------

Updated settings
----------------

- The `-maxmempool` and `-dbcache` startup parameters are now capped on 32-bit systems
  to 500MB and 1GiB respectively. (#32530)

- The `-natpmp` option is now set to `1` by default. This means nodes with `-listen`
  enabled (the default) but running behind a firewall, such as a local network router,
  will be reachable if the firewall/router supports any of the `PCP` or `NAT-PMP`
  protocols. (#33004)

- The `-upnp` setting has now been fully removed. Use `-natpmp` instead. (#32500)

- Previously, `-proxy` specified the proxy for all networks (except I2P which
  uses `-i2psam`) and only the Tor proxy could have been specified separately
  via `-onion`. Now, the syntax of `-proxy` has been extended and it is possible
  to specify separately the proxy for IPv4, IPv6, Tor and CJDNS by appending `=`
  followed by the network name, for example `-proxy=127.0.0.1:5555=ipv6`
  configures a proxy only for IPv6. The `-proxy` option can be used multiple
  times to define different proxies for different networks, such as
  `-proxy=127.0.0.1:4444=ipv4 -proxy=10.0.0.1:6666=ipv6`. Later settings
  override earlier ones for the same network; this can be used to remove an
  earlier all-networks proxy and use direct connections only for a given
  network, for example `-proxy=127.0.0.1:5555 -proxy=0=cjdns`. (#32425)

- The `-blockmaxweight` startup option has been updated to be debug-only.
  It is still available to users, but now hidden from the default `-help` text
  and shown only in `-help-debug` (#32654).

Changes to GUI or wallet related settings can be found in the GUI or Wallet section below.

Wallet
------

- BDB legacy wallets can no longer be created or loaded. They can be migrated
  to the new descriptor wallet format. Refer to the `migratewallet` RPC for more
  details.

- The legacy wallet removal drops redundant options in the bitcoin-wallet tool,
  such as `-withinternalbdb`, `-legacy`, and `-descriptors`. Moreover, the
  legacy-only RPCs `addmultisigaddress`, `dumpprivkey`, `dumpwallet`,
  `importaddress`, `importmulti`, `importprivkey`, `importpubkey`,
  `importwallet`, `newkeypool`, `sethdseed`, and `upgradewallet`, are removed.
  (#32944, #28710, #32438, #31250)

- Support has been added for spending TRUC transactions received by the
  wallet, as well as creating TRUC transactions. The wallet ensures that
  TRUC policy rules are being met. The wallet will throw an error if the
  user is trying to spend TRUC utxos with utxos of other versions.
  Additionally, the wallet will treat unconfirmed TRUC sibling
  transactions as mempool conflicts. The wallet will also ensure that
  transactions spending TRUC utxos meet the required size restrictions. (#32896)

- Since descriptor wallets do not allow mixing watchonly and non-watchonly descriptors,
  the `include_watchonly` option (and its variants in naming) are removed from all RPCs
  that had it. (#32618)

- The `iswatchonly` field is removed from any RPCs that returned it. (#32618)

- `unloadwallet` - Return RPC_INVALID_PARAMETER when both the RPC wallet endpoint
  and wallet_name parameters are unspecified. Previously the RPC failed with a JSON
  parsing error. (#32845)

- `getdescriptoractivity` - Mark blockhashes and scanobjects arguments as required,
  so the user receives a clear help message when either is missing. As in `unloadwallet`,
  previously the RPC failed with a JSON parsing error. (#32845)

- `getwalletinfo` - Removes the fields `balance`, `immature_balance` and
  `unconfirmed_balance`. (#32721)

- `getunconfirmedbalance` - Removes this RPC command. You can query the `getbalances`
  RPC and inspect the `["mine"]["untrusted_pending"]` entry within the JSON
  response. (#32721)

- The following RPCs now contain a `version` parameter that allows
  the user to create transactions of any standard version number (1-3):
  - `createrawtransaction`
  - `createpsbt`
  - `send`
  - `sendall`
  - `walletcreatefundedpsbt`
  (#32896)

GUI changes
-----------

- The GUI has been migrated from Qt 5 to Qt 6. On Windows, dark mode is now supported.
  On macOS, the Metal backend is now used. (#30997)

- A transaction's fee bump is allowed under fullrbf and no longer requires
  BIP-125 signalling. (#31953)

- Custom column widths in the Transactions tab are reset as a side-effect of legacy
  wallet removal. (#32459)

Low-level changes
=================

- Logs now include which peer sent us a header. Additionally there are fewer
  redundant header log messages. A side-effect of this change is that for
  some untypical cases new headers aren't logged anymore, e.g. a direct
  `BLOCK` message with a previously unknown header and `submitheader` RPC. (#27826)

Credits
=======

Thanks to everyone who directly contributed to this release:

- 0xb10c
- amisha
- Andrew Toth
- Anthony Towns
- Antoine Poinsot
- Ava Chow
- benthecarman
- Brandon Odiwuor
- brunoerg
- Bue-von-hon
- Bufo
- Chandra Pratap
- Chris Stewart
- Cory Fields
- Daniel Pfeifer
- Daniela Brozzoni
- David Gumberg
- deadmanoz
- dennsikl
- dergoegge
- enoch
- Ethan Heilman
- Eugene Siegel
- Eunovo
- Eval EXEC
- Fabian Jahr
- fanquake
- Florian Schmaus
- fuder.eth
- furszy
- glozow
- Greg Sanders
- Hao Xu
- Haoran Peng
- Haowen Liu
- Hennadii Stepanov
- Hodlinator
- hoffman
- ishaanam
- ismaelsadeeq
- Jameson Lopp
- janb84
- Jiri Jakes
- John Bampton
- Jon Atack
- josibake
- jurraca
- kevkevin
- kevkevinpal
- kilavvy
- Kristaps Kaupe
- l0rinc
- laanwj
- leopardracer
- Lőrinc
- Luis Schwab
- Luke Dashjr
- MarcoFalke
- marcofleon
- Martin Zumsande
- Matt Corallo
- Matthew Zipkin
- Max Edwards
- monlovesmango
- Murch
- naiyoma
- nervana21
- Nicola Leonardo Susca
- Novo
- pablomartin4btc
- Peter Todd
- Pieter Wuille
- Pol Espinasa
- Prabhat Verma
- rkrux
- Roman Zeyde
- Ryan Ofsky
- Saikiran
- Salvatore Ingala
- Sebastian Falbesoner
- Sergi Delgado Segura
- Shunsuke Shimizu
- Sjors Provoost
- stickies-v
- stratospher
- stringintech
- strmfos
- stutxo
- tdb3
- TheCharlatan
- Tomás Andróil
- UdjinM6
- Vasil Dimov
- VolodymyrBg
- w0xlt
- will
- willcl-ark
- William Casarin
- woltx
- yancy
- zaidmstrr

As well as to everyone that helped with translations on
[Transifex](https://explore.transifex.com/bitcoin/bitcoin/).
