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

New user documentation
----------------------

- [Reduce memory](https://github.com/bitcoin/bitcoin/blob/master/doc/reduce-memory.md)
  suggests configuration tweaks for running Bitcoin Core on systems with
  limited memory. (#16339)

New RPCs
--------

- `getbalances` returns an object with all balances (`mine`,
  `untrusted_pending` and `immature`). Please refer to the RPC help of
  `getbalances` for details. The new RPC is intended to replace
  `getbalance`, `getunconfirmedbalance`, and the balance fields in
  `getwalletinfo`.  These old calls and fields may be removed in a
  future version. (#15930, #16239)

- `setwalletflag` sets and unsets wallet flags that enable or disable
  features specific to that existing wallet, such as the new
  `avoid_reuse` feature documented elsewhere in these release notes.
  (#13756)

- `getblockfilter` gets the BIP158 filter for the specified block.  This
  RPC is only enabled if block filters have been created using the
  `-blockfilterindex` configuration option. (#14121)

New settings
------------

- `-blockfilterindex` enables the creation of BIP158 block filters for
  the entire blockchain.  Filters will be created in the background and
  currently use about 4 GiB of space.  Note: this version of Bitcoin
  Core does not serve block filters over the P2P network, although the
  local user may obtain block filters using the `getblockfilter` RPC.
  (#14121)

Updated settings
----------------

- `whitebind` and `whitelist` now accept a list of permissions to
  provide peers connecting using the indicated interfaces or IP
  addresses.  If no permissions are specified with an address or CIDR
  network, the implicit default permissions are the same as previous
  releases.  See the `bitcoind -help` output for these two options for
  details about the available permissions. (#16248)

Updated RPCs
------------

Note: some low-level RPC changes mainly useful for testing are described in the
Low-level Changes section below.

- `sendmany` no longer has a `minconf` argument.  This argument was not
  well specified and would lead to RPC errors even when the wallet's
  coin selection succeeded.  Users who want to influence coin selection
  can use the existing `-spendzeroconfchange`, `-limitancestorcount`,
  `-limitdescendantcount` and `-walletrejectlongchains` configuration
  arguments. (#15596)

- `getbalance` and `sendtoaddress`, plus the new RPCs `getbalances` and
  `createwallet`, now accept an "avoid_reuse" parameter that controls
  whether already used addresses should be included in the operation.
  Additionally, `sendtoaddress` will avoid partial spends when
  `avoid_reuse` is enabled even if this feature is not already enabled
  via the `-avoidpartialspends` command line flag because not doing so
  would risk using up the "wrong" UTXO for an address reuse case.
  (#13756)

- `listunspent` now returns a "reused" bool for each output if the
  wallet flag "avoid_reuse" is enabled. (#13756)

- `getblockstats` now uses BlockUndo data instead of the transaction
  index, making it much faster, no longer dependent on the `-txindex`
  configuration option, and functional for all non-pruned blocks.
  (#14802)

- `utxoupdatepsbt` now accepts a `descriptors` parameter that will fill
  out input and output scripts and keys when known. P2SH-witness inputs
  will be filled in from the UTXO set when a descriptor is provided that
  shows they're spending segwit outputs.  See the RPC help text for full
  details. (#15427)

- `sendrawtransaction` and `testmempoolaccept` no longer accept a
  `allowhighfees` parameter to fail mempool acceptance if the
  transaction fee exceedes the value of the configuration option
  `-maxtxfee`.  Now there is a hardcoded default maximum feerate that
  can be changed when calling either RPC using a `maxfeerate` parameter.
  (#15620)

- `getmempoolancestors`, `getmempooldescendants`, `getmempoolentry`, and
  `getrawmempool` no longer return a `size` field unless the
  configuration option `-deprecatedrpc=size` is used.  Instead a new
  `vsize` field is returned with the transaction's virtual size
  (consistent with other RPCs such as `getrawtransaction`). (#15637)

- `getwalletinfo` now includes a `scanning` field that is either `false`
  (no scanning) or an object with information about the duration and
  progress of the wallet's scanning historical blocks for transactions
  affecting its balances. (#15730)

- `createwallet` accepts a new `passphrase` parameter.  If set, this
  will create the new wallet encrypted with the given passphrase.  If
  unset (the default) or set to an empty string, no encryption will be
  used. (#16394)

- `getmempoolentry` now provides a `weight` field containing the
  transaction weight as defined in BIP141. (#16647)

- `getdescriptorinfo` now returns an additional `checksum` field
  containing the checksum for the unmodified descriptor provided by the
  user (that is, before the descriptor is normalized for the
  `descriptor` field). (#15986)

- `walletcreatefundedpsbt` now signals BIP125 Replace-by-Fee if the
  `-walletrbf` configuration option is set to true. (#15911)

GUI changes
-----------

- Provides bech32 addresses by default.  The user may change the address
  during invoice generation using a GUI toggle, or the default address
  type may be changed by the `-addresstype` configuration option.
  (#15711, #16497)

Deprecated or removed configuration options
-------------------------------------------

- `-mempoolreplacement` is removed, although default node behavior
  remains the same.  This option previously allowed the user to prevent
  the node from accepting or relaying BIP125 transaction replacements.
  This is different from the remaining configuration option
  `-walletrbf`. (#16171)

Deprecated or removed RPCs
--------------------------

- `bumpfee` no longer accepts a `totalFee` option unless the
  configuration parameter `deprecatedrpc=totalFee` is specified.  This
  parameter will be fully removed in a subsequent release. (#15996)

- `generate` is now removed after being deprecated in Bitcoin Core 0.18.
  Use the `generatetoaddress` RPC instead. (#15492)

P2P changes
-----------

- BIP 61 reject messages were deprecated in v0.18. They are now disabled
  by default, but can be enabled by setting the `-enablebip61` command
  line option.  BIP 61 reject messages will be removed entirely in a
  future version of Bitcoin Core. (#14054)

- To eliminate well-known denial-of-service vectors in Bitcoin Core,
  especially for nodes with spinning disks, the default value for the
  `-peerbloomfilters` configuration option has been changed to false.
  This prevents Bitcoin Core from sending the BIP111 NODE_BLOOM service
  flag, accepting BIP37 bloom filters, or serving merkle blocks or
  transactions matching a bloom filter.  Users who still want to provide
  bloom filter support may either set the configuration option to true
  to re-enable both BIP111 and BIP37 support or enable just BIP37
  support for specific peers using the updated `-whitelist` and
  `-whitebind` configuration options described elsewhere in these
  release notes.  For the near future, lightweight clients using public
  BIP111/BIP37 nodes should still be able to connect to older versions
  of Bitcoin Core and nodes that have manually enabled BIP37 support,
  but developers of such software should consider migrating to either
  using specific BIP37 nodes or an alternative transaction filtering
  system. (#16152)

Miscellaneous CLI Changes
-------------------------

- The `testnet` field in `bitcoin-cli -getinfo` has been renamed to
  `chain` and now returns the current network name as defined in BIP70
  (main, test, regtest). (#15566)

Low-level changes
=================

RPC
---

- `getblockchaininfo` no longer returns a `bip9_softforks` object.
  Instead, information has been moved into the `softforks` object and
  an additional `type` field describes how Bitcoin Core determines
  whether that soft fork is active (e.g. BIP9 or BIP90).  See the RPC
  help for details. (#16060)

- `getblocktemplate` no longer returns a `rules` array containing `CSV`
  and `segwit` (the BIP9 deployments that are currently in active
  state). (#16060)

- `getrpcinfo` now returns a `logpath` field with the path to
  `debug.log`. (#15483)

Tests
-----

- The regression test chain enabled by the `-regtest` command line flag
  now requires transactions to not violate standard policy by default.
  This is the same default used for mainnet and makes it easier to test
  mainnet behavior on regtest. Note that the testnet still allows
  non-standard txs by default and that the policy can be locally
  adjusted with the `-acceptnonstdtxn` command line flag for both test
  chains. (#15891)

Configuration
------------

- A setting specified in the default section but not also specified in a
  network-specific section (e.g. testnet) will now produce a error
  preventing startup instead of just a warning unless the network is
  mainnet.  This prevents settings intended for mainnet from being
  applied to testnet or regtest. (#15629)

- On platforms supporting `thread_local`, log lines can be prefixed with
  the name of the thread that caused the log. To enable this behavior,
  use `-logthreadnames=1`. (#15849)

Network
-------

- When fetching a transaction announced by multiple peers, previous versions of
  Bitcoin Core would sequentially attempt to download the transaction from each
  announcing peer until the transaction is received, in the order that those
  peers' announcements were received.  In this release, the download logic has
  changed to randomize the fetch order across peers and to prefer sending
  download requests to outbound peers over inbound peers. This fixes an issue
  where inbound peers could prevent a node from getting a transaction.
  (#14897, #15834)

- If a Tor hidden service is being used, Bitcoin Core will be bound to
  the standard port 8333 even if a different port is configured for
  clearnet connections.  This prevents leaking node identity through use
  of identical non-default port numbers. (#15651)

Mempool and transaction relay
-----------------------------

- Allows one extra single-ancestor transaction per package.  Previously,
  if a transaction in the mempool had 25 descendants, or it and all of
  its descendants were over 101,000 vbytes, any newly-received
  transaction that was also a descendant would be ignored.  Now, one
  extra descendant will be allowed provided it is an immediate
  descendant (child) and the child's size is 10,000 vbytes or less.
  This makes it possible for two-party contract protocols such as
  Lightning Network to give each participant an output they can spend
  immediately for Child-Pays-For-Parent (CPFP) fee bumping without
  allowing one malicious participant to fill the entire package and thus
  prevent the other participant from spending their output. (#15681)

- Transactions with outputs paying v1 to v16 witness versions (future
  segwit versions) are now accepted into the mempool, relayed, and
  mined.  Attempting to spend those outputs remains forbidden by policy
  ("non-standard").  When this change has been widely deployed, wallets
  and services can accept any valid bech32 Bitcoin address without
  concern that transactions paying future segwit versions will become
  stuck in an unconfirmed state. (#15846)

- Legacy transactions (transactions with no segwit inputs) must now be
  sent using the legacy encoding format, enforcing the rule specified in
  BIP144.  (#14039)

Wallet
------

- When in pruned mode, a rescan that was triggered by an `importwallet`,
  `importpubkey`, `importaddress`, or `importprivkey` RPC will only fail
  when blocks have been pruned. Previously it would fail when `-prune`
  has been set.  This change allows setting `-prune` to a high value
  (e.g. the disk size) without the calls to any of the import RPCs
  failing until the first block is pruned. (#15870)

- When creating a transaction with a fee above `-maxtxfee` (default 0.1
  BTC), the RPC commands `walletcreatefundedpsbt` and
  `fundrawtransaction` will now fail instead of rounding down the fee.
  Be aware that the `feeRate` argument is specified in BTC per 1,000
  vbytes, not satoshi per vbyte. (#16257)

- A new wallet flag `avoid_reuse` has been added (default off). When
  enabled, a wallet will distinguish between used and unused addresses,
  and default to not use the former in coin selection.  When setting
  this flag on an existing wallet, rescanning the blockchain is required
  to correctly mark previously used destinations.  Together with "avoid
  partial spends" (added in Bitcoin Core v0.17.0), this can eliminate a
  serious privacy issue where a malicious user can track spends by
  sending small payments to a previously-paid address that would then
  be included with unrelated inputs in future payments. (#13756)

Build system changes
--------------------

- Python >=3.5 is now required by all aspects of the project. This
  includes the build systems, test framework and linters. The previously
  supported minimum (3.4), was EOL in March 2019. (#14954)

- The minimum supported miniUPnPc API version is set to 10. This keeps
  compatibility with Ubuntu 16.04 LTS and Debian 8 `libminiupnpc-dev`
  packages. Please note, on Debian this package is still vulnerable to
  [CVE-2017-8798](https://security-tracker.debian.org/tracker/CVE-2017-8798)
  (in jessie only) and
  [CVE-2017-1000494](https://security-tracker.debian.org/tracker/CVE-2017-1000494)
  (both in jessie and in stretch). (#15993)

Credits
=======

Thanks to everyone who directly contributed to this release:


As well as everyone that helped translating on [Transifex](https://www.transifex.com/bitcoin/bitcoin/).
