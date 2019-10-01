*After branching off for a major version release of Syscoin Core, use this
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

Syscoin Core version *version* is now available from:

  <https://syscoincore.org/bin/syscoin-core-*version*/>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/syscoin/syscoin/issues>

To receive security and update notifications, please subscribe to:

  <https://syscoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over `/Applications/Syscoin-Qt` (on Mac)
or `syscoind`/`syscoin-qt` (on Linux).

Upgrading directly from a version of Syscoin Core that has reached its EOL is
possible, but might take some time if the datadir needs to be migrated.  Old
wallet versions of Syscoin Core are generally supported.

Compatibility
==============

Syscoin Core is supported and extensively tested on operating systems using
the Linux kernel, macOS 10.10+, and Windows 7 and newer. It is not recommended
to use Syscoin Core on unsupported systems.

Syscoin Core should also work on most other Unix-like systems but is not
as frequently tested on them.

From 0.17.0 onwards, macOS <10.10 is no longer supported. 0.17.0 is
built using Qt 5.9.x, which doesn't support versions of macOS older than
10.10. Additionally, Syscoin Core does not yet change appearance when
macOS "dark mode" is activated.

In addition to previously supported CPU platforms, this release's pre-compiled
distribution provides binaries for the RISC-V platform.

Notable changes
===============


RPC
---

- `getblockchaininfo` no longer returns a `bip9_softforks` object.
  Instead, information has been moved into the `softforks` object and
  an additional `type` field describes how Syscoin Core determines
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
  network-specific section (e.g. testnet) will now produce an error
  preventing startup instead of just a warning unless the network is
  mainnet.  This prevents settings intended for mainnet from being
  applied to testnet or regtest. (#15629)

- On platforms supporting `thread_local`, log lines can be prefixed with
  the name of the thread that caused the log. To enable this behavior,
  use `-logthreadnames=1`. (#15849)

Network
-------

- When fetching a transaction announced by multiple peers, previous versions of
  Syscoin Core would sequentially attempt to download the transaction from each
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
  SYS), the RPC commands `walletcreatefundedpsbt` and
  `fundrawtransaction` will now fail instead of rounding down the fee.
  Be aware that the `feeRate` argument is specified in SYS per 1,000
  vbytes, not satoshi per vbyte. (#16257)

- A new wallet flag `avoid_reuse` has been added (default off). When
  enabled, a wallet will distinguish between used and unused addresses,
  and default to not use the former in coin selection.  When setting
  this flag on an existing wallet, rescanning the blockchain is required
  to correctly mark previously used destinations.  Together with "avoid
  partial spends" (added in Syscoin Core v0.17.0), this can eliminate a
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

=======
Credits
=======

Thanks to everyone who directly contributed to this release:


As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/syscoin/syscoin/).
