Bitcoin Core version 0.13.0 is now available from:

  <https://bitcoin.org/bin/bitcoin-core-0.13.0/>

This is a new major version release, including new features, various bugfixes
and performance improvements, as well as updated translations.

Please report bugs using the issue tracker at github:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

Compatibility
==============

Microsoft ended support for Windows XP on [April 8th, 2014](https://www.microsoft.com/en-us/WindowsForBusiness/end-of-xp-support),
an OS initially released in 2001. This means that not even critical security
updates will be released anymore. Without security updates, using a bitcoin
wallet on a XP machine is irresponsible at least.

In addition to that, with 0.12.x there have been varied reports of Bitcoin Core
randomly crashing on Windows XP. It is [not clear](https://github.com/bitcoin/bitcoin/issues/7681#issuecomment-217439891)
what the source of these crashes is, but it is likely that upstream
libraries such as Qt are no longer being tested on XP.

We do not have time nor resources to provide support for an OS that is
end-of-life. From 0.13.0 on, Windows XP is no longer supported. Users are
suggested to upgrade to a newer verion of Windows, or install an alternative OS
that is supported.

No attempt is made to prevent installing or running the software on Windows XP,
you can still do so at your own risk, but do not expect it to work: do not
report issues about Windows XP to the issue tracker.

Notable changes
===============

Database cache memory increased
--------------------------------

As a result of growth of the UTXO set, performance with the prior default
database cache of 100 MiB has suffered.
For this reason the default was changed to 300 MiB in this release.

For nodes on low-memory systems, the database cache can be changed back to
100 MiB (or to another value) by either:

- Adding `dbcache=100` in bitcoin.conf
- Changing it in the GUI under `Options → Size of database cache`

Note that the database cache setting has the most performance impact
during initial sync of a node, and when catching up after downtime.

bitcoin-cli: arguments privacy
--------------------------------

The RPC command line client gained a new argument, `-stdin`
to read extra arguments from standard input, one per line until EOF/Ctrl-D.
For example:

    $ echo -e "mysecretcode\n120" | src/bitcoin-cli -stdin walletpassphrase

It is recommended to use this for sensitive information such as wallet
passphrases, as command-line arguments can usually be read from the process
table by any user on the system.

C++11 and Python 3
-------------------

Various code modernizations have been done. The Bitcoin Core code base has
started using C++11. This means that a C++11-capable compiler is now needed for
building. Effectively this means GCC 4.7 or higher, or Clang 3.3 or higher.

When cross-compiling for a target that doesn't have C++11 libraries, configure with
`./configure --enable-glibc-back-compat ... LDFLAGS=-static-libstdc++`.

For running the functional tests in `qa/rpc-tests`, Python3.4 or higher is now
required.

Linux ARM builds
------------------

Due to popular request, Linux ARM builds have been added to the uploaded
executables.

The following extra files can be found in the download directory or torrent:

- `bitcoin-${VERSION}-arm-linux-gnueabihf.tar.gz`: Linux binaries for the most
  common 32-bit ARM architecture.
- `bitcoin-${VERSION}-aarch64-linux-gnu.tar.gz`: Linux binaries for the most
  common 64-bit ARM architecture.

ARM builds are still experimental. If you have problems on a certain device or
Linux distribution combination please report them on the bug tracker, it may be
possible to resolve them.

Note that Android is not considered ARM Linux in this context. The executables
are not expected to work out of the box on Android.

New mempool information RPC calls
---------------------------------

RPC calls have been added to output detailed statistics for individual mempool
entries, as well as to calculate the in-mempool ancestors or descendants of a
transaction: see `getmempoolentry`, `getmempoolancestors`, `getmempooldescendants`.

Fee filtering of invs (BIP 133)
------------------------------------

The optional new p2p message "feefilter" is implemented and the protocol
version is bumped to 70013. Upon receiving a feefilter message from a peer,
a node will not send invs for any transactions which do not meet the filter
feerate. [BIP 133](https://github.com/bitcoin/bips/blob/master/bip-0133.mediawiki)

Compact Block support (BIP 152)
-------------------------------

Support for block relay using the Compact Blocks protocol has been implemented
in PR 8068.

The primary goal is reducing the bandwidth spikes at relay time, though in many
cases it also reduces propagation relay. It is automatically enabled between
compatible peers.
[BIP 152](https://github.com/bitcoin/bips/blob/master/bip-0152.mediawiki)

Hierarchical Deterministic Key Generation
-----------------------------------------
Newly created wallets will use hierarchical deterministic key generation
according to BIP32 (keypath m/0'/0'/k').
Existing wallets will still use traditional key generation.

Backups of HD wallets, regardless of when they have been created, can
therefore be used to re-generate all possible private keys, even the
ones which haven't already been generated during the time of the backup.

HD key generation for new wallets can be disabled by `-usehd=0`. Keep in
mind that this flag only has affect on newly created wallets.
You can't disable HD key generation once you have created a HD wallet.

There is no distinction between internal (change) and external keys.

[Pull request](https://github.com/bitcoin/bitcoin/pull/8035/files), [BIP 32](https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki)

Low-level P2P changes
----------------------

- The P2P alert system has been removed in PR #7692 and the `alert` P2P message
  is no longer supported.

- The transaction relay mechanism used to relay one quarter of all transactions
  instantly, while queueing up the rest and sending them out in batch. As
  this resulted in chains of dependent transactions being reordered, it
  systematically hurt transaction relay. The relay code was redesigned in PRs
  #7840 and #8082, and now always batches transactions announcements while also
  sorting them according to dependency order. This significantly reduces orphan
  transactions. To compensate for the removal of instant relay, the frequency of
  batch sending was doubled for outgoing peers.

- Since PR 7840 the BIP35 mempool command is also subject to batch processing.

- The maximum size of orphan transactions that are kept in memory until their
  ancestors arrive has been raised in PR 8179 from 5000 to 99999 bytes. They
  are now also removed from memory when they are included in a block, conflict
  with a block, and time out after 20 minutes.

- We respond at most once to a getaddr request during the lifetime of a
  connection since PR 7856.

- Connections to peers who have recently been the first one to give us a valid
  new block or transaction are protected from disconnections since PR 8084.

Low-level RPC changes
----------------------

- `gettxoutsetinfo` UTXO hash (`hash_serialized`) has changed. There was a divergence between
  32-bit and 64-bit platforms, and the txids were missing in the hashed data. This has been
  fixed, but this means that the output will be different than from previous versions.

- Full UTF-8 support in the RPC API. Non-ASCII characters in, for example,
  wallet labels have always been malformed because they weren't taken into account
  properly in JSON RPC processing. This is no longer the case. This also affects
  the GUI debug console.

- Asm script outputs replacements for OP_NOP2 and OP_NOP3

  - OP_NOP2 has been renamed to OP_CHECKLOCKTIMEVERIFY by [BIP 
65](https://github.com/bitcoin/bips/blob/master/bip-0065.mediawiki)

  - OP_NOP3 has been renamed to OP_CHECKSEQUENCEVERIFY by [BIP 
112](https://github.com/bitcoin/bips/blob/master/bip-0112.mediawiki)

  - The following outputs are affected by this change:

    - RPC `getrawtransaction` (in verbose mode)
    - RPC `decoderawtransaction`
    - RPC `decodescript`
    - REST `/rest/tx/` (JSON format)
    - REST `/rest/block/` (JSON format when including extended tx details)
    - `bitcoin-tx -json`

Low-level ZMQ changes
----------------------

- Each ZMQ notification now contains an up-counting sequence number that allows
  listeners to detect lost notifications.
  The sequence number is always the last element in a multi-part ZMQ notification and
  therefore backward compatible. Each message type has its own counter.
  PR [#7762](https://github.com/bitcoin/bitcoin/pull/7762).

0.13.0 Change log
=================

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors and string updates. For convenience in locating
the code changes and accompanying discussion, both the pull request and
git merge commit are mentioned.

### RPC and REST

### ZMQ

### Configuration and command-line options

### Block and transaction handling

### P2P protocol and network code

### Validation

### Build system

### Wallet

### GUI

### Tests

### Miscellaneous

Credits
=======

Thanks to everyone who directly contributed to this release:

As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/bitcoin/).
