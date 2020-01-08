Syscoin Core version 4.1.2 is now available from:

  https://github.com/syscoin/syscoin/releases/tag/v4.1.2

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/syscoin/syscoin/issues>


Upgrade Instructions: https://syscoin.readme.io/v4.1.2/docs/syscoin-41-upgrade-guide
Basic upgrade instructions below:

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), uninstall it, on linux run `make uninstall` and on windows/osx uninstall through the installer/package manager. Then run the
installer (on Windows) or just copy over `/Applications/Syscoin-Qt` (on Mac)
or `syscoind`/`syscoin-qt` (on Linux). IMPORTANT: YOU SHOULD UNINSTALL PREVIOUS VERSION
If you are upgrading from a version older than 4.1.0, PLEASE READ: https://syscoin.readme.io/v4.1.2/docs/syscoin-41-upgrade-guide

Upgrading directly from a version of Syscoin Core that has reached its EOL is
possible, but might take some time if the datadir needs to be migrated.  Old
wallet versions of Syscoin Core are generally supported.

Compatibility
==============

Syscoin Core is supported and extensively tested on operating systems using
the Linux kernel, macOS 10.10+, and Windows 7 and newer. It is not recommended
to use Syscoin Core on unsupported systems.

The only in-compatibility between 4.0.x and 4.1.x is the interim .node files 
(specifically `scrypt.node`) that may exist in the binary path or /usr/local/bin
 on linux that may impede the functioning of the [relayer](https://github.com/Syscoin/relayer).
The node version used to build the [relayer](https://github.com/Syscoin/relayer) 
was incremented and thus .node requirement was removed, however if the .node files 
exist, it will not run the [relayer](https://github.com/Syscoin/relayer) and as a
result Syscoin will not get the Ethereum block headers needed for consensus 
validation of Syscoin Mint transactions. 
See: https://syscoin.readme.io/v4.1.2/docs/syscoin-41-upgrade-guide

Syscoin Core should also work on most other Unix-like systems but is not
as frequently tested on them. Geth is an Ethereum client that runs inside of Syscoin.
The standard 64-bit Unix distribution is packaged up when installing Syscoin. If you
have a distribution that does not work with the supplied binaries, you can download from:
https://geth.ethereum.org/downloads/ and place it in the src/bin/linux directory. Read more:
https://github.com/Syscoin/Syscoin/blob/master/src/bin/linux/README.md.

From 4.1.x onwards, macOS <10.10 is no longer supported. 4.0.x is
built using Qt 5.9.x, which doesn't support versions of macOS older than
10.10. Additionally, Syscoin Core does not yet change appearance when
macOS "dark mode" is activated.

Notable changes
===============

This release fixes mn broadcast scoping issue [issue #385](https://github.com/syscoin/syscoin/issues/385)

Deprecated or removed RPCs
--------------------------

- The `getaddressinfo` RPC `labels` field now returns an array of label name
  strings. Previously, it returned an array of JSON objects containing `name` and
  `purpose` key/value pairs, which is now deprecated and will be removed in
  future release of syscoind. To re-enable the previous behavior, launch syscoind with
  `-deprecatedrpc=labelspurpose`.

4.1.2 change log
=================

Andrew Chow (1):
- Restore English translation option

Hennadii Stepanov (4):
- qt: Add LogQtInfo() function
- qt: Force set nPruneSize in QSettings after intro
- refactor: Drop `bool force' parameter
- qt: Rename SetPrune() to InitializePruneSetting()

Samuel Dobson (4):
- Merge #17621: IsUsedDestination should count any known single-key address
- Merge #16373: bumpfee: Return PSBT when wallet has privkeys disabled
- Merge #17578: rpc: simplify getaddressinfo labels, deprecate previous behavior
- Merge #17677: Activate watchonly wallet behavior for LegacySPKM only

Wladimir J. van der Laan (1):
- Merge #16975: test: Show debug log on unit test failure

fanquake (4):
- Merge #17857: scripts: fix symbol-check & security-check argument passing
- build: add Wdate-time to Werror flags
- Merge #17452: test: update fuzz directory in .gitignore
- Merge #17696: qt: Force set nPruneSize in QSettings after the intro dialog

sidhujag (2):
- fix mn broadcast scoping issue
- 4.1.2
