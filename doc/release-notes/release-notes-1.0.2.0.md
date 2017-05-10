Release Notes for Bitcoin Unlimited v1.0.2.0
============================================

Bitcoin Unlimited version 1.0.2.0 is now available from:

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

- Fix a memory exhaustion attack:
  * Avoiding the process of Xthin blocks unless they extend the chain.
  * Clear thinblock data and disconnect a node if the maximum bytes permitted for concurrent thinblock re-construction has been exceeded.
  * Calculate whether thinblock memory is too big before pushing anything, and disallow repeated tx in the thin blocks

- Overall reorganization and consolidation of Xthin and Xpedited code

- Added compilation flag: "--enable-gperf" that allows bitcoin to be linked with gperftools on Linux

- Update LevelDB to 1.19

Commit details
--------------

- `15ff34c` Check all nodes for nLocalThinBlockBytes (Peter Tschipper)
- `fc9353b` Disconnect node after clearing thinblock data (Peter Tschipper)
- `28c15e3` Maintain locking order with vNodes (Peter Tschipper)
- `58ec701` Add updateblockavailability, and fix return false vs true (Andrew Stone)
- `1c599a9` Clear vRecv1 in thinblock tests (Peter Tschipper)
- `26b80a9` Ensure we don't cause an integer underflow (Peter Tschipper)
- `990cb27` Clear Thinblock Data if we need to re-request a regular block (Peter Tschipper)
- `0069785` Clang-format, add log for too large block and apply the same logic to xthin and thin (Andrew Stone)
- `d2175ce` Calculate whether thinblock memory is too big before pushing anything, and disallow repeated tx in the thin blocks (Andrew Stone)
- `ecc870d` Use ClearThinBlockData() at the very beginning of thinblock processing (Peter Tschipper)
- `155e234` Add exploit tests for thinblock byte counting (Peter Tschipper)
- `a57f1b6` Count bytes as we build a thinblock (Peter Tschipper)
- `7b44fd6` Prevent the mapThinBlocksInflight timer from tripping (Peter Tschipper)
- `359fcd9` Backport of #541 into release (Check merkle root mutation in xthin/thinblock) (Andrew Stone)
- `428f28d` Backport of kyuupichan's xthin-checks branch to 'release' (ftrader)
- `82bd612` Combine expedited block and xthin block handling (Neil Booth)
- `c33b823` This error results in misbehaving, not outright ban (Andrew Stone)
- `8af23ef` In unit tests, ensure XTHIN service is enabled whenever we are testing it (Andrew Stone)
- `ca9cb68` Remove redeclaration of CInv in main.cpp (Peter Tschipper)
- `548e74c` Expedited blocks: only send after checking PoW (Neil Booth)
- `6728778` Take out the assigning of Misbehavior when un-requested block arrive (Peter Tschipper)
- `d7a1b8e` Solve block not requested race conditions (Andrew Stone)
- `4f0e1af` Use xpeditedBlkUp to determine whether we have an expdited node or not (Peter Tschipper)
- `0cc05e9` If a thinblock does not arrive in the timeout period then disconnect the node (Peter Tschipper)
- `1bf945a` Don't process xpedited messages if thinblocks not enabled (Peter Tschipper)
- `aae090e` Use error messages instead of LogPrintf where applicable (Peter Tschipper)
- `589affa` Replace the boost lexical casts (Peter Tschipper)
- `a9c5199` Added note as to what cs_xpedited is protecting (Peter Tschipper)
- `612e607` Code cleanup for expedited.cpp (Peter Tschipper)
- `83e701a` Cleanup HandleExpeditedBlock() (Peter Tschipper)
- `540ee29` Clean up code formatting for expedited in main.cpp (Peter Tschipper)
- `e5ff473` Add CriticalSection cs_xpedited (Peter Tschipper)
- `94e199b` Create expedited.cpp and expedited.h (Peter Tschipper)
- `7111dca` Add comments describing the purpose of these new objects (Andrew Stone)
- `d6699ae` Use global consts for "getdata" (Neil Booth)
- `da1c143` Clear out xthin/thinblock data we no longer need before processing (Peter Tschipper)
- `a008731` Re-request a full thinblock on merkleroot check failure (Peter Tschipper)
- `d3b2f98` Add gperftools memory checking tool (Andrew Stone)
- `273f084` Clear ThinBlockTimer after the request is sent rather than when  received (Peter Tschipper)
- `3dfd365` Fix thinblock_tests.cpp (Peter Tschipper)
- `560625d` Merge #8613: LevelDB 1.19 (Wladimir J. van der Laan)
- `6fad2ef` Fix png files to prevent libpng warnings (Neil Booth)
- `6708db8` Fix two small issues: side-effecting code in 2 asserts, and not releasing socket resources in an error condition (Andrew Stone)
- `b6cb5a6` Make CNode::DumpBanList local to net.cpp (Peter Tschipper)
- `9a27872` Update UI banned list (Peter Tschipper)
- `05ac461` Delete orphans from the current block (Peter Tschipper)
- `b49a0f6` Reduce the default number of max orphans in the orphan pool. (Peter Tschipper)
- `2ff87b0` Erase orphans from previous block that was processed (Peter Tschipper)
- `fc28f91` Show number of Orphans in the Information UI (Peter Tschipper)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Andrew Stone
- Neil Booth
- Peter Tschipper
- Wladimir J. van der Laan
- ftrader










