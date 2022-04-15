Syscoin Core version 4.1.1 is now available from:

  https://github.com/syscoin/syscoin/releases/tag/v4.1.1

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/syscoin/syscoin/issues>


Upgrade Instructions: https://syscoin.readme.io/v4.1.1/docs/syscoin-41-upgrade-guide
Basic upgrade instructions below:

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), uninstall it, on linux run `make uninstall` and on windows/osx uninstall through the installer/package manager. Then run the
installer (on Windows) or just copy over `/Applications/Syscoin-Qt` (on Mac)
or `syscoind`/`syscoin-qt` (on Linux). IMPORTANT: YOU SHOULD UNINSTALL PREVIOUS VERSION
If you are upgrading from a version older than 4.1.0, PLEASE READ: https://syscoin.readme.io/v4.1.1/docs/syscoin-41-upgrade-guide

Upgrading directly from a version of Syscoin Core that has reached its EOL is
possible, but might take some time if the datadir needs to be migrated.  Old
wallet versions of Syscoin Core are generally supported.

Compatibility
==============

Syscoin Core is supported and extensively tested on operating systems using
the Linux kernel, macOS 10.10+, and Windows 7 and newer. It is not recommended
to use Syscoin Core on unsupported systems.

The only in-compatibility between 4.x and 4.1 is the interim .node files 
(specifically `scrypt.node`) that may exist in the binary path or /usr/local/bin
 on linux that may impede the functioning of the [relayer](https://github.com/Syscoin/relayer).
The node version used to build the [relayer](https://github.com/Syscoin/relayer) 
was incremented and thus .node requirement was removed, however if the .node files 
exist, it will not run the [relayer](https://github.com/Syscoin/relayer) and as a
result Syscoin will not get the Ethereum block headers needed for consensus 
validation of Syscoin Mint transactions. 
See: https://syscoin.readme.io/v4.1.1/docs/syscoin-41-upgrade-guide

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

Users running macOS Catalina may need to "right-click" and then choose "Open"
to open the Syscoin Core .dmg. This is due to new signing requirements
imposed by Apple, which the Syscoin Core project does not yet adhere too.

Notable changes
===============

This release fixes the 100% CPU usage [issue #382](https://github.com/syscoin/syscoin/issues/382)

4.1.1 change log
=================

MarcoFalke (6):
- Merge #17801: doc: Update license year range to 2020
- Merge #17741: build: Included test_bitcoin-qt in msvc build
- Merge #16658: validation: Rename CheckInputs to CheckInputScripts
- Merge #17849: ci: Fix brew python link
- Merge #17781: rpc: Remove mempool global from miner
- Merge #17851: tests: Add std::to_string to list of locale dependent functions

Wladimir J. van der Laan (4):
- Merge #17787: scripts: add MACHO PIE check to security-check.py
- Add missing typeinfo includes
- Merge #17762: net: Log to net category for exceptions in ProcessMessages
- Merge #17850: Serialization improvements (minimal initial commits)

fanquake (7):
- Merge #17829: scripted-diff: Bump copyright of files changed in 2019
- Merge #17825: doc: Update dependencies.md
- Merge #17688: doc: Add "ci" prefix to CONTRIBUTING.md
- Merge #17817: build: Add default configure cache file to .gitignore
- Merge #17869: refactor: Remove unused defines in qt/syscoinunits.h
- Merge #17393: doc: Added regtest config for linearize script

practicalswift (1):
- tests: Add std::to_string to list of locale dependent functions

sidhujag (14):
- update copyright
- update msvc sol file
- refactor time to only store it once instead of calling GetTime() multiple times
- fix up cs_main locks in masternode.cpp
- map mn messages for p2p relay output
- handle blocks only relay in relation to mn
- relay txs (skip blocks only) if masternode mode
- remove locale dependence of to_string, use itostr there
- remove lint
- fix typo
- set mn connections to block relay only
- update path
- move lock back down to be safe
- update to 4.1.1

