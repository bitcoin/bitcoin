Bitcoin Core version 0.17.2 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-0.17.2/>

This is a new minor version release, including new features, various bugfixes
and performance improvements, as well as updated translations.

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

If your node has a txindex, the txindex db will be migrated the first time you
run 0.17.0 or newer, which may take up to a few hours. Your node will not be
functional until this migration completes.

The first time you run version 0.15.0 or newer, your chainstate database will be converted to a
new format, which will take anywhere from a few minutes to half an hour,
depending on the speed of your machine.

Note that the block database format also changed in version 0.8.0 and there is no
automatic upgrade code from before version 0.8 to version 0.15.0. Upgrading
directly from 0.7.x and earlier without redownloading the blockchain is not supported.
However, as usual, old wallet versions are still supported.

Downgrading warning
-------------------

The chainstate database for this release is not compatible with previous
releases, so if you run 0.15 and then decide to switch back to any
older version, you will need to run the old release with the `-reindex-chainstate`
option to rebuild the chainstate data structures in the old format.

If your node has pruning enabled, this will entail re-downloading and
processing the entire blockchain.

Compatibility
==============

Bitcoin Core is extensively tested on multiple operating systems using
the Linux kernel, macOS 10.10+, and Windows 7 and newer (Windows XP is not supported).

Bitcoin Core should also work on most other Unix-like systems but is not
frequently tested on them.

From 0.17.0 onwards macOS <10.10 is no longer supported. 0.17.0 is built using Qt 5.9.x, which doesn't
support versions of macOS older than 10.10.

Notable changes
===============

Documentation
-------------

- A new document introduces Bitcoin Core's BIP174
  [Partially-Signed Bitcoin Transactions (PSBT)](https://github.com/bitcoin/bitcoin/blob/0.17/doc/psbt.md)
  interface, which is used to allow multiple programs to collaboratively
  work to create, sign, and broadcast new transactions.  This is useful
  for offline (cold storage) wallets, multisig wallets, coinjoin
  implementations, and many other cases where two or more programs need
  to interact to generate a complete transaction.

Wallet
------

- When creating a transaction with a fee above -maxtxfee (default 0.1 BTC), the RPC commands walletcreatefundedpsbt and fundrawtransaction will now fail instead of rounding down the fee. Be aware that the feeRate argument is specified in BTC per 1,000 vbytes, not satoshi per vbyte. (#16639)

0.17.2 change log
=================

### Build system
- #15188 Update zmq to 4.3.1 (rex4539)
- #15983 build with -fstack-reuse=none (MarcoFalke)

### Documentation
- #13941 Add PSBT documentation (sipa)
- #14319 Fix PSBT howto and example parameters (priscoan)
- #14944 Update NetBSD build instructions for 8.0 (fanquake)
- #14966 fix testmempoolaccept CLI syntax (1Il1)
- #15012 Fix minor error in doc/psbt.md (bitcoinhodler)
- #15213 Remove errant paste from walletcreatefundedpsbt for nLocktime replaceable (instagibbs)

### GUI
- #14123 Add GUIUtil::bringToFront (promag)
- #14133 Favor macOS show / hide action in dock menu (promag)
- #14383 Clean system tray icon menu for '-disablewallet' mode (hebasto)
- #14597 Cleanup MacDockIconHandler class (hebasto)
- #15085 Fix incorrect application name when passing -regtest (benthecarman)

### RPC and other APIs
- #14941 Make unloadwallet wait for complete wallet unload (promag)
- #14890 Avoid creating non-standard raw transactions (MarcoFalke)

### Test
- #15985 Add test for GCC bug 90348 (sipa)

### Wallet
- #11911 Free BerkeleyEnvironment instances when not in use (ryanofsky)
- #12493 Reopen CDBEnv after encryption instead of shutting down (achow101)
- #14320 Fix duplicate fileid detection (ken2812221)
- #14350 Add WalletLocation class (promag)
- #14552 Detecting duplicate wallet by comparing the db filename (ken2812221)
- #15297 Releases dangling files on BerkeleyEnvironment::Close (promag)
- #16257 Abort when attempting to fund a transaction above -maxtxfee (sjors)
- #16322 Fix -maxtxfee check by moving it to CWallet::CreateTransaction (promag)

Credits
=======

Thanks to everyone who directly contributed to this release:

- 1Il1
- Andrew Chow
- Ben Carman
- bitcoinhodler
- Chun Kuan Lee
- David A. Harding
- Dimitris Apostolou
- fanquake
- Gregory Sanders
- Hennadii Stepanov
- João Barbosa
- MarcoFalke
- MeshCollider
- Pierre Rochard
- Pieter Wuille
- priscoan
- Russell Yanofsky
- Sjors Provoost
- Wladimir J. van der Laan

As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/bitcoin/).
