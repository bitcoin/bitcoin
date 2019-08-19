*After branching off for a major version release of Bitcoin Core, use this
template to create the initial release notes draft.*

*The release notes draft is a temporary file that can be added to by anyone. See
[/doc/developer-notes.md#release-notes](/doc/developer-notes.md#release-notes)
for the process.*

*Create the draft, named* "*version* Release Notes Draft"
*(e.g. "0.20.0 Release Notes Draft"), as a collaborative wiki in:*

https://github.com/bitcoin-core/bitcoin-devwiki/wiki/

*Before the final release, move the notes back to this git repository.*

*version* Release Notes Draft
===============================

Bitcoin Core version *version* is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-*version*/>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on Mac)
or `bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but might take some time if the datadir needs to be migrated.  Old
wallet versions of Bitcoin Core are generally supported.

Compatibility
==============

Bitcoin Core is supported and extensively tested on operating systems using
the Linux kernel, macOS 10.10+, and Windows 7 and newer. It is not recommended
to use Bitcoin Core on unsupported systems.

Bitcoin Core should also work on most other Unix-like systems but is not
as frequently tested on them.

From 0.17.0 onwards, macOS <10.10 is no longer supported. 0.17.0 is
built using Qt 5.9.x, which doesn't support versions of macOS older than
10.10. Additionally, Bitcoin Core does not yet change appearance when
macOS "dark mode" is activated.

In addition to previously-supported CPU platforms, this release's
pre-compiled distribution also provides binaries for the RISC-V
platform.

Notable changes
===============

New RPCs
--------

- `getbalances` returns an object with all balances (`mine`,
  `untrusted_pending` and `immature`). Please refer to the RPC help of
  `getbalances` for details. The new RPC is intended to replace
  `getunconfirmedbalance` and the balance fields in `getwalletinfo`, as well as
  `getbalance`. The old calls may be removed in a future version.

- A new `setwalletflag` RPC sets/unsets flags for an existing wallet.

Updated RPCs
------------

Note: some low-level RPC changes mainly useful for testing are described in the
Low-level Changes section below.

- The `sendmany` RPC had an argument `minconf` that was not well specified and
  would lead to RPC errors even when the wallet's coin selection would succeed.
  The `sendtoaddress` RPC never had this check, so to normalize the behavior,
  `minconf` is now ignored in `sendmany`. If the coin selection does not
  succeed due to missing coins, it will still throw an RPC error. Be reminded
  that coin selection is influenced by the `-spendzeroconfchange`,
  `-limitancestorcount`, `-limitdescendantcount` and `-walletrejectlongchains`
  command line arguments.

- Several RPCs have been updated to include an "avoid_reuse" flag, used
  to control whether already used addresses should be left out or
  included in the operation.  These include:

    - createwallet
    - getbalance
    - getbalances
    - sendtoaddress

  In addition, `sendtoaddress` has been changed to avoid partial spends
  when `avoid_reuse` is enabled (if not already enabled via the
  `-avoidpartialspends` command line flag), as it would otherwise risk
  using up the "wrong" UTXO for an address reuse case.

  The listunspent RPC has also been updated to now include a "reused"
  bool, for nodes with "avoid_reuse" enabled.

- The `getblockstats` RPC is faster for fee calculation by using
  BlockUndo data. Also , `-txindex` is no longer required and
  `getblockstats` works for all non-pruned blocks.

- `createwallet` can now create encrypted wallets if a non-empty
  passphrase is specified.

- The `utxoupdatepsbt` RPC method has been updated to take a
  `descriptors` argument. When provided, input and output scripts and
  keys will be filled in when known, and P2SH-witness inputs will be
  filled in from the UTXO set when a descriptor is provided that shows
  they're spending segwit outputs.

  See the RPC help text for full details.

- The -maxtxfee setting no longer has any effect on non-wallet RPCs.

  The `sendrawtransaction` and `testmempoolaccept` RPC methods previously
  accepted an `allowhighfees` parameter to fail the mempool acceptance in case
  the transaction's fee would exceed the value of the command line argument
  `-maxtxfee`. To uncouple the RPCs from the global option, they now have a
  hardcoded default for the maximum transaction fee, that can be changed for
  both RPCs on a per-call basis with the `maxfeerate` parameter. The
  `allowhighfees` boolean option has been removed and replaced by the
  `maxfeerate` numeric option.

- In getmempoolancestors, getmempooldescendants, getmempoolentry and
  getrawmempool RPCs, to be consistent with the returned value and other
  RPCs such as getrawtransaction, vsize has been added and size is now
  deprecated. size will only be returned if bitcoind is started with
  `-deprecatedrpc=size`.

- The RPC `getwalletinfo` response now includes the `scanning` key with
  an object if there is a scanning in progress or `false` otherwise.
  Currently the object has the scanning duration and progress.

- `createwallet` now returns a warning if an empty string is used as an
  encryption password, and does not encrypt the wallet, instead of
  raising an error.  This makes it easier to disable encryption but also
  specify other options when using the `bitcoin-cli` tool.

- `getmempoolentry` now provides a `weight` field containing the
  transaction weight as defined in BIP 141.

Deprecated or removed RPCs
--------------------------

- The `totalFee` option of the `bumpfee` RPC has been deprecated and will be
  removed in 0.20.  To continue using this option start with
  `-deprecatedrpc=totalFee`.  See the `bumpfee` RPC help text for more details.

P2P changes
-----------

- BIP 61 reject messages were deprecated in v0.18. They are now disabled
  by default, but can be enabled by setting the `-enablebip61` command
  line option.  BIP 61 reject messages will be removed entirely in a
  future version of Bitcoin Core.

- The default value for the -peerbloomfilters configuration option (and,
  thus, NODE_BLOOM support) has been changed to false.  This resolves
  well-known DoS vectors in Bitcoin Core, especially for nodes with
  spinning disks. It is not anticipated that this will result in a
  significant lack of availability of NODE_BLOOM-enabled nodes in the
  coming years, however, clients which rely on the availability of
  NODE_BLOOM-supporting nodes on the P2P network should consider the
  process of migrating to a more modern (and less trustful and
  privacy-violating) alternative over the coming years.

Miscellaneous CLI Changes
-------------------------
- The `testnet` field in `bitcoin-cli -getinfo` has been renamed to
  `chain` and now returns the current network name as defined in BIP70
  (main, test, regtest).

Low-level changes
=================

RPC
---

- Soft fork reporting in the `getblockchaininfo` return object has been
  updated. For full details, see the RPC help text. In summary:

  - The `bip9_softforks` sub-object is no longer returned
  - The `softforks` sub-object now returns an object keyed by soft fork name,
    rather than an array
  - Each softfork object in the `softforks` object contains a `type`
    value which is either `buried` (for soft fork deployments where the
    activation height is hard-coded into the client implementation), or
    `bip9` (for soft fork deployments where activation is controlled by
    BIP 9 signaling).

- `getblocktemplate` no longer returns a `rules` array containing `CSV`
  and `segwit` (the BIP 9 deployments that are currently in active
  state).

Tests
-----

- The regression test chain, that can be enabled by the `-regtest` command line
  flag, now requires transactions to not violate standard policy by default.
  Making the default the same as for mainnet, makes it easier to test mainnet
  behavior on regtest. Be reminded that the testnet still allows non-standard
  txs by default and that the policy can be locally adjusted with the
  `-acceptnonstdtxn` command line flag for both test chains.

Configuration
------------

- An error is issued where previously a warning was issued when a setting in
  the config file was specified in the default section, but not overridden for
  the selected network. This change takes only effect if the selected network
  is not mainnet.

- On platforms supporting `thread_local`, log lines can be prefixed with
  the name of the thread that caused the log. To enable this behavior,
  use `-logthreadnames=1`.

Network
-------

- When fetching a transaction announced by multiple peers, previous versions of
  Bitcoin Core would sequentially attempt to download the transaction from each
  announcing peer until the transaction is received, in the order that those
  peers' announcements were received.  In this release, the download logic has
  changed to randomize the fetch order across peers and to prefer sending
  download requests to outbound peers over inbound peers. This fixes an issue
  where inbound peers can prevent a node from getting a transaction.

Wallet
------

- When in pruned mode, a rescan that was triggered by an `importwallet`,
  `importpubkey`, `importaddress`, or `importprivkey` RPC will only fail when
  blocks have been pruned. Previously it would fail when `-prune` has been set.
  This change allows to set `-prune` to a high value (e.g. the disk size) and
  the calls to any of the import RPCs would fail when the first block is
  pruned.

- When creating a transaction with a fee above `-maxtxfee` (default 0.1
  BTC), the RPC commands `walletcreatefundedpsbt` and
  `fundrawtransaction` will now fail instead of rounding down the fee.
  Beware that the `feeRate` argument is specified in BTC per kilobyte,
  not satoshi per byte.

- A new wallet flag `avoid_reuse` has been added (default off). When
  enabled, a wallet will distinguish between used and unused addresses,
  and default to not use the former in coin selection.

  Rescanning the blockchain is required, to correctly mark previously
  used destinations.

  Together with "avoid partial spends" (present as of Bitcoin v0.17),
  this addresses a serious privacy issue where a malicious user can
  track spends by peppering a previously paid to address with near-dust
  outputs, which would then be inadvertently included in future
  payments.

Build system changes
--------------------

- Python >=3.5 is now required by all aspects of the project. This
  includes the build systems, test framework and linters. The previously
  supported minimum (3.4), was E OL in March 2019. See #14954 for more
  details.

- The minimum supported miniUPnPc API version is set to 10. This keeps
  compatibility with Ubuntu 16.04 LTS and Debian 8 `libminiupnpc-dev`
  packages. Please note, on Debian this package is still vulnerable to
  [CVE-2017-8798](https://security-tracker.debian.org/tracker/CVE-2017-8798)
  (in jessie only) and
  [CVE-2017-1000494](https://security-tracker.debian.org/tracker/CVE-2017-1000494)
  (both in jessie and in stretch).

Credits
=======

Thanks to everyone who directly contributed to this release:


As well as everyone that helped translating on [Transifex](https://www.transifex.com/bitcoin/bitcoin/).
