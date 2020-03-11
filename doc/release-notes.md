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
possible, but it might take some time if the datadir needs to be migrated. Old
wallet versions of Bitcoin Core are generally supported.

Compatibility
==============

Bitcoin Core is supported and extensively tested on operating systems using
the Linux kernel, macOS 10.12+, and Windows 7 and newer. It is not recommended
to use Bitcoin Core on unsupported systems.

Bitcoin Core should also work on most other Unix-like systems but is not
as frequently tested on them.

From Bitcoin Core 0.20.0 onwards, macOS versions earlier than 10.12 are no
longer supported. Additionally, Bitcoin Core does not yet change appearance
when macOS "dark mode" is activated.

In addition to previously supported CPU platforms, this release's pre-compiled
distribution provides binaries for the RISC-V platform.

Notable changes
===============

P2P and network changes
-----------------------

#### Removal of reject network messages from Bitcoin Core (BIP61)

The command line option to enable BIP61 (`-enablebip61`) has been removed.

This feature has been disabled by default since Bitcoin Core version 0.18.0.
Nodes on the network can not generally be trusted to send valid ("reject")
messages, so this should only ever be used when connected to a trusted node.
Please use the recommended alternatives if you rely on this deprecated feature:

* Testing or debugging of implementations of the Bitcoin P2P network protocol
  should be done by inspecting the log messages that are produced by a recent
  version of Bitcoin Core. Bitcoin Core logs debug messages
  (`-debug=<category>`) to a stream (`-printtoconsole`) or to a file
  (`-debuglogfile=<debug.log>`).

* Testing the validity of a block can be achieved by specific RPCs:
  - `submitblock`
  - `getblocktemplate` with `'mode'` set to `'proposal'` for blocks with
    potentially invalid POW

* Testing the validity of a transaction can be achieved by specific RPCs:
  - `sendrawtransaction`
  - `testmempoolaccept`

* Wallets should not use the absence of "reject" messages to indicate a
  transaction has propagated the network, nor should wallets use "reject"
  messages to set transaction fees. Wallets should rather use fee estimation
  to determine transaction fees and set replace-by-fee if desired. Thus, they
  could wait until the transaction has confirmed (taking into account the fee
  target they set (compare the RPC `estimatesmartfee`)) or listen for the
  transaction announcement by other network peers to check for propagation.

The removal of BIP61 REJECT message support also has the following minor RPC
and logging implications:

* `testmempoolaccept` and `sendrawtransaction` no longer return the P2P REJECT
  code when a transaction is not accepted to the mempool. They still return the
  verbal reject reason.

* Log messages that previously reported the REJECT code when a transaction was
  not accepted to the mempool now no longer report the REJECT code. The reason
  for rejection is still reported.

Updated RPCs
------------

- `testmempoolaccept` and `sendrawtransaction` no longer return the P2P REJECT
  code when a transaction is not accepted to the mempool. See the Section
  _Removal of reject network messages from Bitcoin Core (BIP61)_ for details on
  the removal of BIP61 REJECT message support.

- A new descriptor type `sortedmulti(...)` has been added to support multisig scripts where the public keys are sorted lexicographically in the resulting script.

- `walletprocesspsbt` and `walletcreatefundedpsbt` now include BIP 32 derivation paths by default for public keys if we know them. This can be disabled by setting `bip32derivs` to `false`.

Build System
------------

- OpenSSL is no longer used by Bitcoin Core. The last usage of the library
was removed in #17265.

- glibc 2.17 or greater is now required to run the release binaries. This
retains compatibility with RHEL 7, CentOS 7, Debian 8 and Ubuntu 14.04 LTS.
Further details can be found in #17538.

New RPCs
--------

New settings
------------

- RPC Whitelist system. It can give certain RPC users permissions to only some RPC calls.
It can be set with two command line arguments (`rpcwhitelist` and `rpcwhitelistdefault`). (#12763)

- A new `-asmap` configuration option has been added to enable IP-to-ASN mapping
  for bucketing of the network peers to diversify the network connections. The
  legacy /16 prefix mapping remains the default. See [issue
  #16599](https://github.com/bitcoin/bitcoin/issues/16599), [PR
  #16702](https://github.com/bitcoin/bitcoin/pull/16702), and the `bitcoind
  help` for more information. This option is experimental and subject to changes
  or removal in future releases.

Updated settings
----------------

Importing blocks upon startup via the `bootstrap.dat` file no longer occurs by default. The file must now be specified with `-loadblock=<file>`.

-  The `-debug=db` logging category has been renamed to `-debug=walletdb`, to distinguish it from `coindb`.
   `-debug=db` has been deprecated and will be removed in the next major release.

GUI changes
-----------

- The "Start Bitcoin Core on system login" option has been removed on macOS.

Wallet
------

- The wallet now by default uses bech32 addresses when using RPC, and creates native segwit change outputs.
- The way that output trust was computed has been fixed in #16766, which impacts confirmed/unconfirmed balance status and coin selection.

- The RPC gettransaction, listtransactions and listsinceblock responses now also
includes the height of the block that contains the wallet transaction, if any.

- RPC `getaddressinfo` changes:

  - the `label` field has been deprecated in favor of the `labels` field and
    will be removed in 0.21. It can be re-enabled in the interim by launching
    with `-deprecatedrpc=label`.

  - the `labels` behavior of returning an array of JSON objects containing name
    and purpose key/value pairs has been deprecated in favor of an array of
    label names and will be removed in 0.21. The previous behavior can be
    re-enabled in the interim by launching with `-deprecatedrpc=labelspurpose`.

Low-level changes
=================

Command line
------------

Command line options prefixed with main/test/regtest network names like
`-main.port=8333` `-test.server=1` previously were allowed but ignored. Now
they trigger "Invalid parameter" errors on startup.

Tests
-----

- It is now an error to use an unqualified `walletdir=path` setting in the config file if running on testnet or regtest
  networks. The setting now needs to be qualified as `chain.walletdir=path` or placed in the appropriate `[chain]`
  section. (#17447)

- `-fallbackfee` was 0 (disabled) by default for the main chain, but 0.0002 by default for the test chains. Now it is 0
  by default for all chains. Testnet and regtest users will have to add `fallbackfee=0.0002` to their configuration if
  they weren't setting it and they want it to keep working like before. (#16524)

Credits
=======

Thanks to everyone who directly contributed to this release:


As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoin/bitcoin/).
