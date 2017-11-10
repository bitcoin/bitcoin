Release Notes for Bitcoin Unlimited Cash Edition 1.1.2.0
=========================================================

Bitcoin Unlimited Cash Edition version 1.1.2.0 is now available from:

  <https://bitcoinunlimited.info/download>

Please report bugs using the issue tracker at github:

  <https://github.com/BitcoinUnlimited/BitcoinUnlimited/issues>

This is a minor release version based of Bitcoin Unlimited compatible
with the Bitcoin Cash specification you could find here:

https://github.com/Bitcoin-UAHF/spec/blob/master/uahf-technical-spec.md


Upgrading
---------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Bitcoin-Qt (on Mac) or
bitcoind/bitcoin-qt (on Linux).

The first time you run this new version, your UTXO database will be converted
to a new format. This step could take a variable amount of time that will depend
on the performance of the hardware you are using.

Downgrade
---------

In case you decide to downgrade from BUcash 1.1.2.0 to a previous version you
will need to run the old release using `-reindex-chainstate` option so that the
UTXO will be rebuild using the previous format.

Main Changes
------------

The main change of this release is the introduction of a new difficulty adjustment algorithm (DAA)
that replaced the old EDA (Emergency Difficulty Adjustment). If you are interested in more detail
about the new DAA you could find more details in the [technical specification](https://github.com/Bitcoin-UAHF/spec/blob/master/nov-13-hardfork-spec.md).


Another major changes is the introduction of a new format to store the UTXO (chainstate) database.
The UTXO storage has been indexed per output rather than per transaction. The code has been ported
from the Bitcoin Core project. This feature brings advantages both in terms of a faster reindex and IBD
operation, lower memory consumption, on the other hand the on-disk space requirement increased of about 15%.


Other notable changes:

- implementation of BIP 146 as an HF (LOW_S, NULLFAIL)
- fix some UI inconsistencies introduced by the Coin Freeze feature
- migrate and separate QT settings for BUCash nodes
- only request blocks via HEADERS and not by INV (BIP 130)
- various improvements to Xthin code
- add a flag to return txns count rather than list them all for `getblock` RPC
- change Bitcoin Cash ticker from BCC to BCH

Commit details
--------------

- `426a6cf28` bump to version 1.1.2.0 (gandrewstone)
- `a560054ef` Add `newdaaactivationtime` to the list of allowed arguments (sickpig)
- `6a9c1e7d8` Fix improper formatiting according to project programming style (sickpig)
- `6a55d31a9` Use the BCH ticker rather than BCC in code comments (sickpig)
- `43dd95a3b` Change currency unit to BCH when compiled for BUCash (sickpig)
- `681d55c57` cast nTime to int64_t (sickpig)
- `7cdaf4036` Remove unnecessary assert (ptschip)
- `a6799f97a` Do some refactoring in pow.cpp to make it easier to introduce new difficulty adjustement algorithm. (deadalnix)
- `db768c5f6` Cherry fixes (ptschip)
- `fecae86d3` Add activation code for the new DAA. (deadalnix)
- `fc445296c` Implement simple moving average over work difficulty adjustement algorithm. (deadalnix)
- `676c21fa2` Replace Config with CChainParams (ptschip)
- `46535f20e` Remove tests not applicable to BitcoinUnlimited (ptschip)
- `bff625b7c` add BITCOIN_CASH define for IsCashHFEnabled() (ptschip)
- `561131017` Add policy: null signature for failed CHECK(MULTI)SIG (Johnson Lau)
- `1546d4964` Enforce LOW_S after Nov, 13 (deadalnix)
- `d9a4ddc25` Add BITCOIN_CASH define to cash hardfork activation code (ptschip)
- `aa84c2112` Next HF activation code. (deadalnix)
- `811391bc2` Add Tests for thinblocksinflight timeouts (ptschip)
- `f450ed738` Migrate QT settings for BUcash nodes (Justaphf)
- `a301f918d` Add separate Qt settings for BUcash (Justaphf)
- `ed7c9d993` Update QSettings migrate fxn to take app name as input (Justaphf)
- `6e8852e51` Use const for legacy org name instead of local string literal (Justaphf)
- `1b190a9a3` Update thinblock timeout to be responsive to blkReqRetryInterval (ptschip)
- `9975a19c1` Create a more robust and realistic reindex.py (ptschip)
- `919f10cbf` Replace Config with CChainParams (ptschip)
- `cd4b61da8` Remove tests not applicable to BitcoinUnlimited (ptschip)
- `3f8ebe1be` add command line arg handling to test_bitcoin.
- `c797e5e93` remove most stack based CNode allocations since CNodes are large (gandrewstone)
- `3445b725b` add BITCOIN_CASH define for IsCashHFEnabled() (ptschip)
- `99b84375a` Add policy: null signature for failed CHECK(MULTI)SIG (Johnson Lau)
- `06b74d6f6` Enforce LOW_S after Nov, 13 (deadalnix)
- `567279032` Add BITCOIN_CASH define to cash hardfork activation code (ptschip)
- `4450abb7a` Next HF activation code. (deadalnix)
- `47d6efce5` WIP travis wine testing: add explicit deregistration of global CStatHistory objects from boost timer (gandrewstone)
- `a7d11ea66` WIP test travis wine with no stats test (gandrewstone)
- `92bea4d30` CStatHistory needs to cancel its timer if it is being destructed & add a few small checks (gandrewstone)
- `1f6688f29` Added delete to new object creation (Marcus Diaz)
- `e5f9ea487` Modify stat_tests to create CStatHistory objects in heap (Marcus Diaz)
- `70fbae8d5` Only request blocks via HEADERS and not by INV (ptschip)
- `d93e81c5f` add assertions to ensure that accesses are within the defined array (gandrewstone)
- `299a0c5ea` Update help comments for getstat (ptschip)
- `34189ebd6` Update the list of available debug categories (sickpig)
- `798fa5bf3` Use std::rotate to move xthin messages to the front of hte queue. (ptschip)
- `bb9afb34f` Improve the performance of xthin queue jumping (ptschip)
- `f7955a743` make historyTime allocated as part of the stathistory class (gandrewstone)
- `bacf79cfa` Make fRescan a global var (ptschip)
- `ccd2eec4d` Remove riak repositories (sickpig)
- `7a61f2982` Merge #11056: disable jni in builds (laanwj)
- `671dc97bc` Merge #8563: Add configure check for -latomic (laanwj)
- `f2ca70775` drop comparison-tool flag from configure (sickpig)
- `c600a0792` Add BU ppa repo and install berkeley DB 4.8 dev pkg (sickpig)
- `109acb08e` Merge #10508: Run Qt wallet tests on travis (laanwj)
- `9952f3c4e` Merge #8534: [travis] Drop java (laanwj)
- `ff4ad40c9` Merge #8504: test: Remove java comparison tool (laanwj)
- `51e1eea9d` Remove the redundant flush in connect block (ptschip)
- `127e4d7a1` flush generated blocks to disk, and notify UI about them (gandrewstone)
- `ed4a990db` Remove unnecessary call to nThinblockBytes.load() (ptschip)
- `859ef2391` Move MAX_BLOOM_FILTER_SIZE to consensus.h (ptschip)
- `cf0766615` As a safeguard don't allow a smaller max xthin bloom filters than (ptschip)
- `89efdada0` Fix FilterSizeXthin tests (ptschip)
- `6c24277f8` Fix ThinBlockCapable and add new thinblock tests (ptschip)
- `ac8456957` Don't allow network connections while rescanning wallet (ptschip)
- `76df55508` Port of Core #11176: build: Rename --enable-experimental-asm to --enable-asm and enable by default (laanwj)
- `8352d1b0b` Port Core PR #10821: Add SSE4 optimized SHA256 (laanwj)
- `51e55302e` Fix for thinstats (ptschip)
- `062905f82` Only Check the spend height one time. (ptschip)
- `759034e26` Only GetSpendHeight() when we are checking a coinbase spend (ptschip)
- `59daed451` add buip055fork.h to header list in Makefile.am (gandrewstone)
- `e14bd3a6f` use BIP32_EXTKEY_SIZE for encode and decode functions rather than just 74. (ptschip)
- `756f10697` Remove mutex and switch to atomic bool in checkqueue.h (ptschip)
- `d1a6e0a79` Use correct magic when receiving/sendig to a peer (sickpig)
- `8c94e3465` Use atomic for nThinBlockBytes and also add more tests (ptschip)
- `b4a65eaf3` Fix spurious failures in parallel.py (ptschip)
- `e97b50ea7` Remove dead code and unused files (ptschip)
- `8449ae8dc` Adjust transaction list output. (marlengit)
- `32f8cac1f` Remove PublicLabelRole. (marlengit)
- `6f70c1075` Fix issue 776: Remove setPublicLabelFilter. (marlengit)
- `7faa4f53c` Update UndoReadFromDisk (ptschip)
- `0a2d5e28f` clang formatting (ptschip)
- `990f77eec` Add test for empty chain and reorg consistency for gettxoutsetinfo. (Gregory Maxwell)
- `c6b92a06b` script_P2SH_tests.cpp fixes (ptschip)
- `0b53682e9` coins_tests.cpp fixes (ptschip)
- `a5db9aed5` fixes for buip055_test.cpp (ptschip)
- `4d672486e` Fix logprint (ptschip)
- `1a526aa5e` Force on-the-fly compaction during pertxout upgrade (sipa)
- `cfd15cc8c` Add missing cs_utxo locks in coins.cpp (ptschip)
- `242f296c8` In AccessByTxid use DEFAULT_LARGEST_TRANSACTION (ptschip)
- `9682bca6a` Fix RemoveForReorg (ptschip)
- `17519abd7` Indicate in the UI that the database upgrade is in progress (ptschip)
- `3c1e8cab4` Cherry Fixes (ptschip)
- `9604037ae` scripted-diff: various renames for per-utxo consistency (sipa)
- `6866e6337` Extend coins_tests (sipa)
- `5d351f5e5` Rename CCoinsCacheEntry::coins to coin (sipa)
- `b4014caf6` Refactor GetUTXOStats in preparation for per-COutPoint iteration (sipa)
- `b63c6315c` Merge CCoinsViewCache's GetOutputFor and AccessCoin (sipa)
- `70937019f` [MOVEONLY] Move old CCoins class to txdb.cpp (sipa)
- `1e240a239` Upgrade from per-tx database to per-txout (sipa)
- `d3007aa52` Pack Coin more tightly (sipa)
- `85f93079b` Remove unused CCoins methods (sipa)
- `9f51ccc16` Switch CCoinsView and chainstate db from per-txid to per-txout (sipa)
- `436497e95` Replace CCoins-based CTxMemPool::pruneSpent with isSpent (sipa)
- `f9a7e75b4` Remove ModifyCoins/ModifyNewCoins (sipa)
- `f22a5cbd4` Report on-disk size in gettxoutsetinfo (sipa)
- `e97464f28` Only pass things committed to by tx's witness hash to CScriptCheck (Matt Corallo)
- `6cbcbfca0` Switch tests from ModifyCoins to AddCoin/SpendCoin (sipa)
- `e280e1782` Switch CScriptCheck to use Coin instead of CCoins (sipa)
- `99de2e8ea` Switch from per-tx to per-txout CCoinsViewCache methods in some places (sipa)
- `2e806d5fb` Introduce new per-txout CCoinsViewCache functions (sipa)
- `a7b281d17` Optimization: Coin&& to ApplyTxInUndo (sipa)
- `f39d63957` Replace CTxInUndo with Coin (sipa)
- `8f549df2f` Introduce Coin, a single unspent output (sipa)
- `717a53449` Store/allow tx metadata in all undo records (sipa)
- `978a7f7d6` Add the smallest example of a stream to streams_tests.cpp (ptschip)
- `21c67e179` wallet: Use CDataStream.data() (laanwj)
- `20ac8bce8` dbwrapper: Use new .data() method of CDataStream (laanwj)
- `08c7425f4` streams: Remove special cases for ancient MSVC (laanwj)
- `0ec4173f3` streams: Add data() method to CDataStream (laanwj)
- `a0caffc28` Remove transaction tests for segwit (ptschip)
- `392f89eaa` Fix some empty vector references (sipa)
- `df6031932` Fix conflicts in ./rpc/blockchain.cpp (ptschip)
- `18529fce1` Remove/ignore tx version in utxo and undo (sipa)
- `e5251a106` txdb: Add Cursor() method to CCoinsView to iterate over UTXO set (laanwj)
- `4ed25fa8d` Add constructor method for FastRandomeContext (ptschip)
- `77deb4e5d` Add specialization of SipHash for 256 + 32 bit data (sipa)
- `02d0ada8b` Introduce CHashVerifier to hash read data (sipa)
- `0746ff7bd` Remove conflicting code (ptschip)
- `73c73fded` Batch construct batches (sipa)
- `a665caf79` dbwrapper: Move `HandleError` to `dbwrapper_private` (laanwj)
- `693f4e5d5` dbwrapper: Pass parent CDBWrapper into CDBBatch and CDBIterator (laanwj)
- `11d684fd0` dbwrapper: Remove CDBWrapper::GetObfuscateKeyHex (laanwj)
- `da758a62e` Don't redefine WIN32_LEAN_AND_MEAN if it's already defined. (ptschip)
- `3b08aec8b` Create random uint64_t by adding two random 32 bit integers (ptschip)
- `943113cb0` Turn TryCreateDirectory() into TryCreateDirectories() (Marko Bencun)
- `a6991f803` dbwrapper: Remove throw keywords in function signatures (laanwj)
- `562ebb705` Remove `namespace fs=fs` (laanwj)
- `b06b69c6e` torcontrol: use fs::path instead of std:;string for private key path (ptschip)
- `882226714` Use fsbridge for fopen and freopen (laanwj)
- `2df0d11f8` Replace uses of boost::filesystem with fs (laanwj)
- `3152e52c9` Replace includes of boost/filesystem.h with fs.h (laanwj)
- `7518f793d` Add fs.cpp/h (laanwj)
- `74e866f05` [Trivial] Add a comment on the use of prevector in script. (Gregory Maxwell)
- `4575f1744` GetDustThreshold was missing the addition of 148 bytes. (ptschip)
- `9e313aa4a` Clarify prevector::erase and avoid swap-to-clear (sipa)
- `d9a5835a8` Add constant for maximum stack size (Gregory Sanders)
- `fdc629411` Fix OOM bug: UTXO entries with invalid script length (sipa)
- `0bbe4a026` Treat overly long scriptPubKeys as unspendable (sipa)
- `80499dfc8` Introduce constant for maximum CScript length (sipa)
- `bf7235f2e` Prevent integer overflow in ReadVarInt. (Gregory Maxwell)
- `0c096257e` `getblock` optionally returns txns count rather than txns list (sickpig)
- `e30618158` error() in disconnect for disk corruption, not inconsistency (sipa)
- `355d2e67a` Introduce FastRandomContext::randbool() (sipa)
- `ace777096` Avoid -Wshadow errors (sipa)
- `fb8d4a294` Use fixed preallocation instead of costly GetSerializeSize (sipa)
- `0fea56468` Add optimized CSizeComputer serializers (sipa)
- `a214b44ab` Make CSerAction's ForRead() constexpr (sipa)
- `c8df85957` Get rid of nType and nVersion (sipa)
- `f83b66406` Make GetSerializeSize a wrapper on top of CSizeComputer (sipa)
- `6abe3c7c1` Make nType and nVersion private and sometimes const (sipa)
- `af574e6b6` Make streams' read and write return void (sipa)
- `29d8c05c2` Remove unused ReadVersion and WriteVersion (sipa)
- `421a4d3fd` Change currency unit to BCC when compiled for BUCash (Justaphf)
- `56a26ca5d` Remove temporary workaround for BitcoinCash (ptschip)
- `75e61267c` Check the max xthin bloom filter size limits on startup (ptschip)
- `3803494e2` Consolidate and simplify bloom filter constructors (ptschip)
- `873801505` Disconnect node rather than ban if FILTERSIZEXTHIN improperly received (ptschip)
- `38dad734a` Add NODE_BLOOM service for exploit thinblock tests (ptschip)
- `97cb16853` Make XTHIN bloom filter size configurable (ptschip)
- `cb92b0766` Manual cherry pick of core's `0a4912e` (John Newbery)
- `a8969dcc9` Fix compiler warning (ptschip)
- `a9be6740c` Remove unused leveldbwrapper.cpp (ptschip)
- `8d0972784` add some benchmarks from the Core project (gandrewstone)
- `33e303fc4` Added verbose option for getstat to show timestamp with data points (Marcus Diaz)


Credits
=======

Thanks to everyone who directly contributed to this release:

- Andrea Suisani
- Andrew Stone
- Justaphf
- marlengit
- Marcus Diaz
- Peter Tschipper

We have backported a significant amount of changes from other projects, namely Bitcoin Core and ABC.

Following all the indirect contributors whose work has been imported via the above backports:

- Gregory Sanders
- John Newbery
- Marko Bencun
- Matt Corallo
- Johnson Lau
- Amaury SECHET
- Gregory Maxwell
- Amaury Séchet
- Wladimir J. van der Laan
- Pieter Wuille
