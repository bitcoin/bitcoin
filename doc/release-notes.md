Bitcoin Core version 28.0 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-28.0/>

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

Running bitcoin core binaries on macOS requires self signing.
```
cd /path/to/bitcoin-core/bin
xattr -d com.apple.quarantine bitcoin-cli bitcoin-qt bitcoin-tx bitcoin-util bitcoin-wallet bitcoind test_bitcoin
codesign -s - bitcoin-cli bitcoin-qt bitcoin-tx bitcoin-util bitcoin-wallet bitcoind test_bitcoin
```

Compatibility
==============

Bitcoin Core is supported and extensively tested on operating systems
using the Linux Kernel 3.17+, macOS 11.0+, and Windows 7 and newer. Bitcoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them. It is not recommended to use Bitcoin Core on
unsupported systems.

Notable changes
===============

Testnet4/BIP94 support
-----

Support for Testnet4 as specified in [BIP94](https://github.com/bitcoin/bips/blob/master/bip-0094.mediawiki)
has been added. The network can be selected with the `-testnet4` option and
the section header is also named `[testnet4]`.

While the intention is to phase out support for Testnet3 in an upcoming
version, support for it is still available via the known options in this
release.

Windows Data Directory
----------------------

The default data directory on Windows has been moved from `C:\Users\Username\AppData\Roaming\Bitcoin`
to `C:\Users\Username\AppData\Local\Bitcoin`. Bitcoin Core will check the existence
of the old directory first and continue to use that directory for backwards
compatibility if it is present. (#27064)

P2P and network changes
-----------------------

- Previously if Bitcoin Core was listening for P2P connections, either using
  default settings or via `bind=addr:port` it would always also bind to
  `127.0.0.1:8334` to listen for Tor connections. It was not possible to switch
  this off, even if the node didn't use Tor. This has been changed and now
  `bind=addr:port` results in binding on `addr:port` only. The default behavior
  of binding to `0.0.0.0:8333` and `127.0.0.1:8334` has not been changed.

  If you are using a `bind=...` configuration without `bind=...=onion` and rely
  on the previous implied behavior to accept incoming Tor connections at
  `127.0.0.1:8334`, you need to now make this explicit by using
  `bind=... bind=127.0.0.1:8334=onion`. (#22729)

- Bitcoin Core will now fail to start up if any of its P2P binds fail, rather
  than the previous behaviour where it would only abort startup if all P2P
  binds had failed. (#22729)

- UNIX domain sockets can now be used for proxy connections. Set `-onion` or `-proxy`
to the local socket path with the prefix `unix:` (e.g. `-onion=unix:/home/me/torsocket`).
(#27375)

- unix socket paths are now accepted for `-zmqpubrawblock` and `-zmqpubrawtx` with
the format `-zmqpubrawtx=unix:/path/to/file`

- Additional flags "in" and "out" have been added to `-whitelist` to control whether
  permissions apply to incoming connections and/or manual (default: incoming only).

- Transactions that are too low feerate will be opportunistically paired with their child
transactions and submitted as a package, thus enabling the node to download
1-parent-1-child packages using the existing transaction relay protocol. Combined with
other mempool policies, this allows limited "package relay" when a parent transaction
is below mempool minimum feerate; TRUC parents are additionally allowed to be below
minimum relay feerate (i.e. pay 0 fees). Use the `submitpackage` RPC to submit packages
directly to the node. Warning: this p2p feature is limited (unlike the `submitpackage`
interface, a child with multiple unconfirmed parents is not supported) and not yet
reliable under adversarial conditions. (#28970)

Mempool Policy Changes
----------------------

- Transactions with version number set to 3 are now treated as standard on all networks (#29496),
  subject to Opt-in Topologically Restricted Until Confirmation (TRUC) Transactions policy as
  described in [BIP 431](https://github.com/bitcoin/bips/blob/master/bip-0431.mediawiki).  The
  policy includes limits on spending unconfirmed outputs (#28948), eviction of a previous descendant
  if a more incentive-compatible one is submitted (#29306), and a maximum transaction size of 10,000vB
  (#29873). These restrictions simplify the assessment of incentive compatibility of accepting or
  replacing TRUC transactions, thus ensuring any replacements are more profitable for the node and
  making fee-bumping more reliable.

- Pay To Anchor (P2A) is a new standard witness output type for spending,
  a newly recognised output template. This allows for key-less anchor
  outputs, with compact spending conditions for additional efficiencies on
  top of an equivalent `sh(OP_TRUE)` output, in addition to the txid stability
  of the spending transaction.
  N.B. propagation of this output spending on the network will be limited
  until a sufficient number of nodes on the network adopt this upgrade.

- Limited package RBF is now enabled, where the proposed conflicting package would result in
  a connected component, aka cluster, of size 2 in the mempool. All clusters being conflicted
  against must be of size 2 or lower.

- `mempoolfullrbf=1` is now set by default.

Updated RPCs
------------

- The JSON-RPC server now recognizes JSON-RPC 2.0 requests and responds with
strict adherence to the [specification](https://www.jsonrpc.org/specification).
See [JSON-RPC-interface.md](https://github.com/bitcoin/bitcoin/blob/master/doc/JSON-RPC-interface.md#json-rpc-11-vs-20) for details.

- The `dumptxoutset` RPC now returns the UTXO set dump in a new and
  improved format. At the same time the `loadtxoutset` RPC now
  expects this new format in dumps it tries to load. Dumps with the
  old format are no longer supported and need to be recreated using
  the new format in order to be usable.

- AssumeUTXO mainnet parameters have been added for height 840,000.
  This means the `loadtxoutset` RPC can now be used on mainnet with
  the matching UTXO set from that height.

- The `warnings` field in `getblockchaininfo`, `getmininginfo` and
  `getnetworkinfo` now returns all the active node warnings as an array
  of strings, instead of just a single warning. The current behaviour
  can temporarily be restored by running bitcoind with configuration
  option `-deprecatedrpc=warnings`.

- Previously when using the `sendrawtransaction` rpc and specifying outputs
  that are already in the UXTO set an RPC error code `-27` with RPC error
  text "Transaction already in block chain" was returned in response.
  The help text has been updated to "Transaction outputs already in utxo set"
  to more accurately describe the source of the issue.

- The default mode for the `estimatesmartfee` RPC has been updated from `conservative` to `economical`.
  which is expected to reduce overestimation for many users, particularly if Replace-by-Fee is an option.
  For users that require high confidence in their fee estimates at the cost of potentially overestimating,
  the `conservative` mode remains available.

- An item of `unspents`, of `scantxoutset`, has two new fields: `blockhash`
  and `confirmations`. `blockhash` is the hash of the block where the UTXO was
  created. `confirmations` is the number of confirmations of the UTXO. (#30515)

- `maxfeerate` and `maxburnamount` arguments are added to submitpackage.

Changes to wallet related RPCs can be found in the Wallet section below.

New RPCs
--------

Updated REST APIs
-----------------
- Parameter validation for `/rest/getutxos` has been improved by rejecting
  truncated or overly large txids and malformed outpoint indices by raising an
  HTTP_BAD_REQUEST "Parse error". Previously, these malformed requests would be
  silently handled. (#30482, #30444)

Build System
------------

- GCC 11.1 or later, or Clang 16.0 or later,
are now required to compile Bitcoin Core.

- The minimum required glibc to run Bitcoin Core is now
2.31. This means that RHEL 8 and Ubuntu 18.04 (Bionic)
are no-longer supported. (#29987)

- `--enable-lcov-branch-coverage` has been removed, given
incompatibilities between lcov version 1 & 2. `LCOV_OPTS`
should be used to set any options instead.

Updated settings
----------------

- When running with `-alertnotify`, an alert can now be raised multiple
times instead of just once. Previously, it was only raised when unknown
new consensus rules were activated, whereas the scope has now been
increased to include all kernel warnings. Specifically, alerts will now
also be raised when an invalid chain with a large amount of work has
been detected. Additional warnings may be added in the future.
(#30058)

Changes to GUI or wallet related settings can be found in the GUI or Wallet section below.

New settings
------------

Tools and Utilities
-------------------

Wallet
------

- The wallet now detects when wallet transactions conflict with the mempool. Mempool
conflicting transactions can be seen in the `"mempoolconflicts"` field of
`gettransaction`. The inputs of mempool conflicted transactions can now be respent
without manually abandoning the transactions when the parent transaction is dropped
from the mempool, which can cause wallet balances to appear higher.

- A new `max_tx_weight` option has been added to the RPCs `fundrawtransaction`, `walletcreatefundedpsbt`, and `send`.
It specifies the maximum transaction weight. If the limit is exceeded during funding, the transaction will not be built.
The default value is 4,000,000 WU.

- A new RPC `createwalletdescriptor` is added which allows users to add new automatically
generated descriptors to their wallet. This can be used to upgrade wallets created prior to
the introduction of a new standard descriptor, such as taproot.

- A new RPC `gethdkeys` is added which will list all of the BIP 32 HD keys in use by all
of the descriptors in the wallet. These keys can be used in conjunction with `createwalletdescriptor`
to create and add single key descriptors to the wallet for a particular key that the wallet
already knows.

- The `sendall` RPC can spend unconfirmed change and will include additional fees as necessary
for the resulting transaction to bump the unconfirmed transactions' feerates to the specified feerate.

- If a `fee_rate` is specified when using the `bumpfee` RPC, the feerate is no longer restricted to
following the wallet's incremental feerate of 5 sat/vb. The feerate must still be at least the sum
of the original fee and the mempool's incremental feerate.

GUI changes
-----------

- The "Migrate Wallet" menu allows users to migrate any legacy wallet in their wallet
directory, regardless of the wallets loaded. (gui#824)

- The "Information" window now displays the maximum mempool size along with the
mempool usage. (gui#825)

Low-level changes
=================

RPC
---

Tests
-----

- The BIP94 timewarp attack mitigation is now active on the `regtest` network

- `-testdatadir` is added to `test_bitcoin` to allow specifying the location for unit test data directories.

Blockstorage
------------

- Block files are now XOR'd by default with a key stored in the blocksdir.
Previous releases of Bitcoin Core or previous external software will not be able to read the blocksdir with a non-zero XOR-key.
Refer to the `-blocksxor` help for more details.

Credits
=======

Thanks to everyone who directly contributed to this release:
- 0xb10c
- Alfonso Roman Zubeldia
- Andrew Toth
- AngusP
- Anthony Towns
- Antoine Poinsot
- Anton A
- Ava Chow
- Ayush Singh
- Ben Westgate
- Brandon Odiwuor
- brunoerg
- bstin
- Charlie
- Christopher Bergqvist
- Cory Fields
- crazeteam
- Daniela Brozzoni
- David Gumberg
- dergoegge
- Edil Medeiros
- Epic Curious
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
- Jon Atack
- josibake
- jrakibi
- kevkevin
- kevkevinpal
- Konstantin Akimov
- laanwj
- Larry Ruane
- Lőrinc
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

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoin/bitcoin/).
