Release Notes for Bitcoin Unlimited v1.0.3.0
============================================

Bitcoin Unlimited version 1.0.3.0 is now available from:

  <https://bitcoinunlimited.info/download>

Please report bugs using the issue tracker at github:

  <https://github.com/BitcoinUnlimited/BitcoinUnlimited/issues>

This is a stable release.

Upgrading
---------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Bitcoin-Qt (on Mac) or
bitcoind/bitcoin-qt (on Linux).

Main Changes
------------

Changes are as follows:

- Improve performance during IBD
- Improve handling of block headers
- Widen the use of the request manager (e.g. direct headers fetch)
- Relaxing peers banning criteria due to p2p misbehaviour
- Reboot nolnet: new address format, new genesis block and network byte, and new difficulty adjustment algorithm
- Re-enable transaction re-request for Xthin
- Move more thinkblock code logic moving it from from `main.cpp` to `thinblock.cpp`
- Increase the minimum number of outgoing connections to 12 the minimum number of Xthin capable nodes to 8

Commit details
--------------

- `bd76012` release 1.0.3 label (Andrew Stone)
- `f6d83f0` Revert to BOOST_FOREACH instead of c++11 for loop (Peter Tschipper)
- `a8fe6da` Add more tests for unconnecting headers in sendheaders.py (Peter Tschipper)
- `1c413e0` Handle headers that won't connect right away. (Peter Tschipper)
- `2242012` reformat bitnodes files (Andrew Stone)
- `0bd9df2` match against altnames and the CN (common name) so that different spam and hosting solutions work.  Fix the short read error (fix provided by @bitcartel). (Andrew Stone)
- `18de13a` Also check pindexBestHeader when determining fScriptChecks (Peter Tschipper)
- `539a948` Remove DOS from unconnecting thinblock (Peter Tschipper)
- `23c373e` Do not continue if the header will not connect (Peter Tschipper)
- `1e1d8ad` Do not check scripts unless blocks are less than 30 days old (Peter Tschipper)
- `378ec7c` Changed http:// to https:// on one link (Marius Kjærstad)
- `8a8bfdc` Renable the check IsInitialBlockDownload() for txns during IBD (Peter Tschipper)
- `a93f71f` Make sure we receive recent headers when doing initial handshake (Peter Tschipper)
- `4ddf7df` Remove 4 hour ban if initial headers not received (Peter Tschipper)
- `36e2632` Fix --enable-debug on OSX (Neil Booth)
- `a047389` add missing constructor for CThinBlockInFlight() (Peter Tschipper)
- `ff33f45` Add strCommand to log message if VERSION not received first. (Peter Tschipper)
- `d0d832b` Remove comments that are no longer valid (Peter Tschipper)
- `3edbf6b` Do not assign Misbehavior if unconnecting headers are received (Peter Tschipper)
- `684d306` Relax banning for VERSION and VERACK type messages (Peter Tschipper)
- `0f0c383` Update thinblock_tests.cpp to reflect the new mapThinBlocksInFlight (Peter Tschipper)
- `f3cde4e` Disregard whether this node is thinblock capable or not (Peter Tschipper)
- `caf5e6f` Use a struct to track thinblocksinflight information (Peter Tschipper)
- `01f8368` Re-enable the thinblock re-request functionality (Peter Tschipper)
- `3536bfa` Don't process unrequested xthins or xblocktx (Peter Tschipper)
- `3df4b02` Cleanup thinblock.cpp (Neil Booth)
- `a4a304f` Increase the default min-xthin-nodes to 8 (Peter Tschipper)
- `6000ba5` fix exception raised due to invalid type in CInv (Andrew Stone)
- `d8838d4` Reboot nolnet. (Neil Booth)
- `8bb82ed` Remove block stalling code (Peter Tschipper)
- `0237dce` Only remove block sources after block is stored to disk (Peter Tschipper)
- `c46b0c2` Remove references to "staller" (Peter Tschipper)
- `7c36826` Always add every block source during IBD (Peter Tschipper)
- `502fa69` Move bitnodes seeding to be after DNS seeding. (Peter Tschipper)
- `efc80f3` [Nit] Move "advertising address" log under "net" (Justaphf)
- `79a45bb` Zero the nBytesOrphanPool if the orphan map has been cleared. (Peter Tschipper)
- `c67a2e8` Set nBytesOrphanPool to zero on startup (Peter Tschipper)
- `938a218` take out accidental printf left in code (Peter Tschipper)
- `5370375` Remove dead orphan pool code (Peter Tschipper)
- `4c4f9fd` Keep track of the in memory orphan tx size (Peter Tschipper)
- `f4b1bba` Reduce the MAX_DISCONNECTS (Peter Tschipper)
- `15b6a69` Once per day reset the number of nDisconnections allowed (Peter Tschipper)
- `a806917` Default constructor should set priority (Neil Booth)
- `75e8b34` do not ban whitelisted peers (nomnombtc)
- `da66f10` Backport changes required for SENDHEADERS (Peter Tschipper)
- `b658eb3` Test for special case when only a single non-continous header (Peter Tschipper)
- `f954ac6` Set uint256 to null (Peter Tschipper)
- `87156f5` Remove redundant code (Peter Tschipper)
- `3ab3591` Check that the block headers are continous before proceeding (Peter Tschipper)
- `8142766` Use request manager for headers direct fetch (Peter Tschipper)
- `de598d2` Add test for handling of unconnecting headers (Suhas Daftuar)
- `864a14e` take out reference to PV (Peter Tschipper)
- `8351504` Use ENTER and LEAVE critical section (Peter Tschipper)
- `290b450` Use a bool return value for RequestBlock (Peter Tschipper)
- `77ba162` merge auto-formatting from dev to release (Andrew Stone)
- `02b776f` Add missing cs_xval locks (Neil Booth)
- `aa877bb` Have HandleExpeditedRequest return a result (Neil Booth)
- `3ee878d` Fall back to full block as comment and log states (Neil Booth)
- `a06fed3` Move thinblock handling from main.cpp to thinblock.cpp (Neil Booth)
- `499dbbd` Move missing tx request handling from main.cpp to thinblock.cpp (Neil Booth)
- `624b9fe` Move missing tx response handling from main.cpp to thinblock.cpp (Neil Booth)
- `3b34768` Update seeders list. (Andrea Suisani)
- `739c95b` Fix Thinblock Missing transactiosn log message (Andrea Suisani)
- `51086f9` Fix getnetworkinfo XThinBlock statistics (port to `release`) (gandrewstone)
- `31cac6b` Whitelist test nodes to prevent banning in abandonconflict test (ftrader)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Andrea Suisani
- Andrew Stone
- ftrader
- Justaphf
- Marius Kjærstad
- Neil Booth
- nomnombtc
- Peter Tschipper

We also have backported changes from the other projects code contributed by:

- Suhas Daftuar







