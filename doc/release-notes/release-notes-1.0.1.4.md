Release Notes for Bitcoin Unlimited v1.0.1.4
============================================

Bitcoin Unlimited version 1.0.1.4 is now available from:

  <https://bitcoinunlimited.info/download>

Please report bugs using the issue tracker at github:

  <https://github.com/BitcoinUnlimited/BitcoinUnlimited/issues>

This is an hotfix release.

Upgrading
---------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Bitcoin-Qt (on Mac) or
bitcoind/bitcoin-qt (on Linux).

Main Changes
------------

Changes are as follows:

- Fix a memory exhaustion attack due to a missing check on incoming block headers in the XTHIN code.

- Ensure that we connect to at least `min-xthin-nodes` (default: 4) XTHIN capable nodes

- Turn SENDHEADERS on when a peer requests it. If a peer requests HEADERS first then we'll send them otherwise we'll send an INV.

- If the thinblocks service bit is turned off then your node you will not be able to request thinkblocks or receive thinblocks.

- Fix for issues with startup and mutiple monitors on windows. If you have multiple monitors and go down to one monitor, your bitcoind on restart used to be placed onto the non-existent monitor.  This fix only places the bitcoind in it shutdown location if it exists.  Otherwise the default location is used.

- When changing excessive block size and mining block size through the QT GUI, the GUI now prevents you from entering invalid values, and shows an error string.

Commit details
--------------

- `5fda711` Add a release notes template to the doc directory (Andrea Suisani)
- `9d36bec` bump to version 1.0.1.4 (Andrew Stone)
- `0855f98` check that the thin block has appropriate difficulty for the blockchain and other contextual checks (Andrew Stone)
- `aeb3c500` Turn off thinblocks for txn_clone.py (Peter Tschipper)
- `65b83db` Fix for potential deadlock (Peter Tschipper)
- `ab04d14` Disconnect peer if VERACK not received within the timeout period (Peter Tschipper)
- `22aa3bb` Add consistency check for VERSION, VERACK, BUVERSION AND BUVERACK (Peter Tschipper)
- `14f8355` Add exploit_tests.cpp (Peter Tschipper)
- `1fccb37` fix formatting (Peter Tschipper)
- `206cfc5` Add a CTweak() for min-xthin-nodes (Peter Tschipper)
- `22828be` Ensure that we connect to at least a few XTHIN capable nodes. (Peter Tschipper)
- `f7f9d9c` Sync blocks properly for bip68-112-113-p2p.py (Peter Tschipper)
- `ea0e5fe` Turn SENDHEADERS on when a peer requests it. (Peter Tschipper)
- `2a749f8` Mempool limiter has been introduced in BU 0.12.1 (Andrea Suisani)
- `f535a92` Make maxTxFee a CTweak (Amaury SECHET)
- `db3cc0b` Merge #8540: qt: Fix random segfault when closing "Choose data directory" dialog (Wladimir J. van der Laan)
- `7ebc646` mempool: Replace maxFeeRate of 10000* minRelayTxFee with maxTxFee (MarcoFalke)
- `a90a6e3` resolve conflict in backport of pr#349 (Peter Tschipper)
- `4e55fdd` Disconnect and ban a node if they fail to provide the inital HEADERS (Peter Tschipper)
- `adb80a7` Catch exceptions creating directory paths (Neil Booth)
- `274870c` Remove #pragma once (Neil Booth)
- `1a23288` Fix for issues with startup and mutiple monitors on windows. (Allan Doensen)
- `a1e0357` Made the RPC code that sets the excessive block size actually use the universal validator. (Allan Doensen)
- `2002cb8` Made the unit test clean up after itself so as to not interfere with other test cases. (Allan Doensen)
- `b448fd5` Added unit tests for emergent consesus validation functions. (Allan Doensen)
- `3b13dc1` Reversed logic in code to make it more human readable. Minor variable name change to make it more consistant with the usage. (Allan Doensen)
- `56f0447` Fixed up poorly named local variables. Changed binary test logic to be consistant with the name of the function. (Allan Doensen)
- `dd4f39e` Centralized all validation for block size in one location in the code. (Allan Doensen)
- `c212385` sources.list.d is the correct directory (姜家志)
- `204cc48` Added checking for mined block size > excessive block size. (Allan Doensen)
- `850b572` Added functionality for 'reset options' button. (Allan Doensen)
- `56096a5` MaxGeneratedBlock, ExcessiveBlockSize & ExcessiveAcceptDepth qt text field widgets in the bitcoin unlimited settings dialog. (Allan Doensen)


Credits
=======

Thanks to everyone who directly contributed to this release:

- Allan Doensen
- Amaury SÉCHET
- Andrea Suisani
- Andrew Stone
- MarcoFalke
- Neil Booth
- Peter Tschipper
- Wladimir J. van der Laan
- 姜家志

