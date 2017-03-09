Release Notes for Bitcoin Unlimited v1.0.1
==========================================

Bitcoin Unlimited version 1.0.1 is now available from:

  <https://bitcoinunlimited.info/download>

Please report bugs using the issue tracker at github:

  <https://github.com/BitcoinUnlimited/BitcoinUnlimited/issues>

This is a bugfix rollup release

Upgrading
---------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Bitcoin-Qt (on Mac) or
bitcoind/bitcoin-qt (on Linux).

Main Changes
------------

Changes are as follows:

- ensure consistent configuration: When the excessive block and max mined block configuration is set or changed, that excessive block is >= max mined block.
- ensure that nodes are disconnected at a thread-safe time
- documentation fixes
- fix QT bug where the tray icon menu has no choices
- if a node's message start is invalid (corrupt), ban for 4 hours.
- allow the "pushtx" RPC accept partial node IP addresses and it will search for the first match
- add help to configuration parameters defined via "CTweaks"
- add miner comment to CTweaks as "mining.comment"
- add miner maximum block generation size to CTweaks as "mining.blockSize"
- fix certain timeouts and inconsistancies in the regression test execution
- point release 1.0.0.1: fix to reserve space for miner's coinbase and correctly account for coinbase in internal miner

Commit details
--------------

- `55d8cbd` bump to version 1.0.1.0 (Andrew Stone)
- `fee66bd` add range checking to mining block size and excessive size, still need to show a warning in the GUI (Andrew Stone)
- `fb3b0a0` fix unit test for new constraint on setminingmaxblock (Andrew Stone)
- `e3b75e5` ensure that mined block size cannot be set > excessive block size (Andrew Stone)
- `60f99ca` Change the order of Network Initialization (Peter Tschipper)
- `5a0fd1a` Set a 4 hour ban only on Invalid MessageStart (Peter Tschipper)
- `eafeac9` Disconnect node using the disconnect flag (Peter Tschipper)
- `c905243` Ban node if PROCESS MESSAGESTART failure (Peter Tschipper)
- `abd60a3` Merge pull request #328 from gubatron/prioritize-tx-fix (gandrewstone)
- `8769cf0` Merge pull request #327 from gubatron/secp256k1-max_scalar-warning-fix (gandrewstone)
- `208e65e` Removed redundant parameter from mempool.PrioritiseTransaction (gubatron)
- `88b503c` Silence unused variable warning (secp256k1 test) (gubatron)
- `7395d35` Change QT org name and domain to reflect BitcoinUnlimited (Peter Tschipper)
- `7da9f84` Updates copyright for 2017 (HansHauge)
- `6aa0398` merge (Andrew Stone)
- `b3c9c78` Merge pull request #271 from jamoes/bu-tray-icon-menu (gandrewstone)
- `9532273` [QT] Bugfix: ensure tray icon menu is not empty (Stephen McCarthy)
- `b7ab016` clean up request manager entry when no source exists, and solve potential deadlock flagged by the detector (Andrew Stone)
- `929f50d` Merge pull request #276 from gandrewstone/dev (gandrewstone)
- `1862a8a` Merge pull request #288 from sickpig/fix/release-note-1.0.0-typos-release-branch (gandrewstone)
- `3e018f3` Fix a few more typos and URLs (Andrea Suisani)
- `505fca3` Merge pull request #284 from sickpig/fix/title-and-url-1.0.0-release-note-release-branch (gandrewstone)
- `8f3b506` Add h1 title to the doc and fix download URL (Andrea Suisani)
- `1f3aa99` Merge pull request #270 from sickpig/travis/backport-bu-pr261 (gandrewstone)
- `afe43f7` Merge pull request #278 from sickpig/new/release-note-1.0.0 (gandrewstone)
- `5874e9e` Add BU 1.0.0 release note (Andrea Suisani)
- `a231b13` [travis] Backport of BU PR #261 to release branch (Andrea Suisani)
- `dda0c0e` Merge pull request #265 from sickpig/fix/travis-status-release (gandrewstone)
- `c1861e6` Update Travis-ci status icon (release branch) (Andrea Suisani)
- `b471620` bump the build number (Andrew Stone)
- `967b6d8` Merge pull request #259 from gandrewstone/release (gandrewstone)
- `c831c5d` shorten runtime of miner_tests because windows test on travis may be taking too long (Andrew Stone)
- `5e82003` Add unit test for zero reserve block generation, and zero reserve block generation with different length coinbase messages. Account for possible varint lengths of 9 bytes for 2 values that are not known during transaction selection (Andrew Stone)
- `3831cc2` Add unit test for block generation, and fix a unit test issue -- an invalid configuration left by a prior test (Andrew Stone)
- `20c1f94` fix issue where a block's coinbase can make it exceed the configured value (Andrew Stone)
