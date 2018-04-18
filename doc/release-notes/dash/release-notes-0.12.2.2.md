Dash Core version 0.12.2.2
==========================

Release is now available from:

  <https://www.dash.org/downloads/#wallets>

This is a new minor version release, bringing various bugfixes and other
improvements.

Please report bugs using the issue tracker at github:

  <https://github.com/dashpay/dash/issues>


Upgrading and downgrading
=========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Dash-Qt (on Mac) or
dashd/dash-qt (on Linux). Because of the per-UTXO fix (see below) there is a
one-time database upgrade operation, so expect a slightly longer startup time on
the first run.

Downgrade warning
-----------------

### Downgrade to a version < 0.12.2.2

Because release 0.12.2.2 includes the per-UTXO fix (see below) which changes the
structure of the internal database, this release is not fully backwards
compatible. You will have to reindex the database if you decide to use any
previous version.

This does not affect wallet forward or backward compatibility.


Notable changes
===============

Per-UTXO fix
------------

This fixes a potential vulnerability, so called 'Corebleed', which was
demonstrated this summer at the Вrеаkіng Віtсоіn Соnfеrеnсе іn Раrіs. The DoS
can cause nodes to allocate excessive amounts of memory, which leads them to a
halt. You can read more about the fix in the original Bitcoin Core pull request
https://github.com/bitcoin/bitcoin/pull/10195

To fix this issue in Dash Core however, we had to backport a lot of other
improvements from Bitcoin Core, see full list of backports in the detailed
change log below.

Additional indexes fix
----------------------

If you were using additional indexes like `addressindex`, `spentindex` or
`timestampindex` it's possible that they are not accurate. Please consider
reindexing the database by starting your node with `-reindex` command line
option. This is a one-time operation, the issue should be fixed now.

InstantSend fix
---------------

InstantSend should work with multisig addresses now.

PrivateSend fix
---------------

Some internal data structures were not cleared properly, which could lead
to a slightly higher memory consumption over a long period of time. This was
a minor issue which was not affecting mixing speed or user privacy in any way.

Removal of support for local masternodes
----------------------------------------

Keeping a wallet with 1000 DASH unlocked for 24/7 is definitely not a good idea
anymore. Because of this fact, it's also no longer reasonable to update and test
this feature, so it's completely removed now. If for some reason you were still
using it, please follow one of the guides and setup a remote masternode instead.

Dropping old (pre-12.2) peers
-----------------------------

Connections from peers with protocol lower than 70208 are no longer accepted.

Other improvements and bug fixes
--------------------------------

As a result of previous intensive refactoring and some additional fixes,
it should be possible to compile Dash Core with `--disable-wallet` option now.

This release also improves sync process and significantly lowers the time after
which `getblocktemplate` rpc becomes available on node start.

And as usual, various small bugs and typos were fixed and more refactoring was
done too.


0.12.2.2 Change log
===================

See detailed [change log](https://github.com/dashpay/dash/compare/v0.12.2.1...dashpay:v0.12.2.2) below.

### Backports:
- [`996f5103a`](https://github.com/dashpay/dash/commit/996f5103a) Backport #7056: Save last db read
- [`23fe35a18`](https://github.com/dashpay/dash/commit/23fe35a18) Backport #7756: Add cursor to iterate over utxo set, use this in `gettxoutsetinfo`
- [`17f2ea5d7`](https://github.com/dashpay/dash/commit/17f2ea5d7) Backport #7904: txdb: Fix assert crash in new UTXO set cursor
- [`2e54bd2e8`](https://github.com/dashpay/dash/commit/2e54bd2e8) Backport #7927: Minor changes to dbwrapper to simplify support for other databases
- [`abaf524f0`](https://github.com/dashpay/dash/commit/abaf524f0) Backport #7815: Break circular dependency main ↔ txdb
- [`02a6cef94`](https://github.com/dashpay/dash/commit/02a6cef94) Move index structures into spentindex.h
- [`d92b454a2`](https://github.com/dashpay/dash/commit/d92b454a2) Add SipHash-2-4 primitives to hash
- [`44526af95`](https://github.com/dashpay/dash/commit/44526af95) Use SipHash-2-4 for CCoinsCache index
- [`60e6a602e`](https://github.com/dashpay/dash/commit/60e6a602e) Use C++11 thread-safe static initializers in coins.h/coins.cpp
- [`753cb1563`](https://github.com/dashpay/dash/commit/753cb1563) Backport #7874: Improve AlreadyHave
- [`952383e16`](https://github.com/dashpay/dash/commit/952383e16) Backport #7933: Fix OOM when deserializing UTXO entries with invalid length
- [`e3b7ed449`](https://github.com/dashpay/dash/commit/e3b7ed449) Backport #8273: Bump `-dbcache` default to 300MiB
- [`94e01eb66`](https://github.com/dashpay/dash/commit/94e01eb66) Backport #8467: [Trivial] Do not shadow members in dbwrapper
- [`105fd1815`](https://github.com/dashpay/dash/commit/105fd1815) Use fixed preallocation instead of costly GetSerializeSize
- [`6fbe93aa7`](https://github.com/dashpay/dash/commit/6fbe93aa7) Backport #9307: Remove undefined FetchCoins method declaration
- [`6974f1723`](https://github.com/dashpay/dash/commit/6974f1723) Backport #9346: Batch construct batches
- [`4b4d22293`](https://github.com/dashpay/dash/commit/4b4d22293) Backport #9308: [test] Add CCoinsViewCache Access/Modify/Write tests
- [`a589c94a9`](https://github.com/dashpay/dash/commit/a589c94a9) Backport #9107: Safer modify new coins
- [`09b3e042f`](https://github.com/dashpay/dash/commit/09b3e042f) Backport #9310: Assert FRESH validity in CCoinsViewCache::BatchWrite
- [`ceb64fcd4`](https://github.com/dashpay/dash/commit/ceb64fcd4) Backport #8610: Share unused mempool memory with coincache
- [`817ecc03d`](https://github.com/dashpay/dash/commit/817ecc03d) Backport #9353: Add data() method to CDataStream (and use it)
- [`249db2776`](https://github.com/dashpay/dash/commit/249db2776) Backport #9999: [LevelDB] Plug leveldb logs to bitcoin logs
- [`cfefd34f4`](https://github.com/dashpay/dash/commit/cfefd34f4) Backport #10126: Compensate for memory peak at flush time
- [`ff9b2967a`](https://github.com/dashpay/dash/commit/ff9b2967a) Backport #10133: Clean up calculations of pcoinsTip memory usage
- [`567043d36`](https://github.com/dashpay/dash/commit/567043d36) Make DisconnectBlock and ConnectBlock static in validation.cpp
- [`9a266e68d`](https://github.com/dashpay/dash/commit/9a266e68d) Backport #10297: Simplify DisconnectBlock arguments/return value
- [`fc5ced317`](https://github.com/dashpay/dash/commit/fc5ced317) Backport #10445: Add test for empty chain and reorg consistency for gettxoutsetinfo.
- [`6f1997182`](https://github.com/dashpay/dash/commit/6f1997182) Add COMPACTSIZE wrapper similar to VARINT for serialization
- [`b06a6a2e7`](https://github.com/dashpay/dash/commit/b06a6a2e7) Fix use of missing self.log in blockchain.py
- [`8ed672219`](https://github.com/dashpay/dash/commit/8ed672219) Backport #10250: Fix some empty vector references
- [`afa96b7c1`](https://github.com/dashpay/dash/commit/afa96b7c1) Backport #10249: Switch CCoinsMap from boost to std unordered_map
- [`c81394b97`](https://github.com/dashpay/dash/commit/c81394b97) Backport #10195: Switch chainstate db and cache to per-txout model
- [`d4562b5e5`](https://github.com/dashpay/dash/commit/d4562b5e5) Fix CCoinsViewCache::GetPriority to use new per-utxo
- [`92bb65894`](https://github.com/dashpay/dash/commit/92bb65894) Fix address index to use new per-utxo DB
- [`9ad56fe18`](https://github.com/dashpay/dash/commit/9ad56fe18) Dash related fixes for per-utxo DB
- [`4f807422f`](https://github.com/dashpay/dash/commit/4f807422f) Backport #10550: Don't return stale data from CCoinsViewCache::Cursor()
- [`151c552c7`](https://github.com/dashpay/dash/commit/151c552c7) Backport #10537: Few Minor per-utxo assert-semantics re-adds and tweak
- [`06aa02ff6`](https://github.com/dashpay/dash/commit/06aa02ff6) Backport #10559: Change semantics of HaveCoinInCache to match HaveCoin
- [`549839a50`](https://github.com/dashpay/dash/commit/549839a50) Backport #10581: Simplify return values of GetCoin/HaveCoin(InCache)
- [`5b232161a`](https://github.com/dashpay/dash/commit/5b232161a) Backport #10558: Address nits from per-utxo change
- [`1a9add78c`](https://github.com/dashpay/dash/commit/1a9add78c) Backport #10660: Allow to cancel the txdb upgrade via splashscreen keypress 'q'
- [`4102211a3`](https://github.com/dashpay/dash/commit/4102211a3) Backport #10526: Force on-the-fly compaction during pertxout upgrade
- [`8780c762e`](https://github.com/dashpay/dash/commit/8780c762e) Backport #10985: Add undocumented -forcecompactdb to force LevelDB compactions
- [`4cd19913d`](https://github.com/dashpay/dash/commit/4cd19913d) Backport #10998: Fix upgrade cancel warnings
- [`371feda4c`](https://github.com/dashpay/dash/commit/371feda4c) Backport #11529: Avoid slow transaction search with txindex enabled
- [`cdb2b1944`](https://github.com/dashpay/dash/commit/cdb2b1944) build: quiet annoying warnings without adding new ones
- [`fee05dab9`](https://github.com/dashpay/dash/commit/fee05dab9) build: silence gcc7's implicit fallthrough warning

### Masternodes:
- [`312663b4b`](https://github.com/dashpay/dash/commit/312663b4b) Remove support for local masternodes (#1706)

### PrivateSend:
- [`7e96af4e6`](https://github.com/dashpay/dash/commit/7e96af4e6) Refactor PrivateSend (#1735)
- [`f4502099a`](https://github.com/dashpay/dash/commit/f4502099a) make CheckDSTXes() private, execute it on both client and server (#1736)

### InstantSend:
- [`4802a1fb7`](https://github.com/dashpay/dash/commit/4802a1fb7) Allow IS for all txes, not only for txes with p2pkh and data outputs (#1760)
- [`f37a64208`](https://github.com/dashpay/dash/commit/f37a64208) InstantSend txes should never qualify to be a 0-fee txes (#1777)

### DIP0001:
- [`3028af19f`](https://github.com/dashpay/dash/commit/3028af19f) post-DIP0001 cleanup (#1763)
- [`51b2c7501`](https://github.com/dashpay/dash/commit/51b2c7501) Fix WarningBitsConditionChecker (#1765)

### Network/Sync:
- [`5d58dd90c`](https://github.com/dashpay/dash/commit/5d58dd90c) Make sure to clear setAskFor in Dash submodules (#1730)
- [`328009749`](https://github.com/dashpay/dash/commit/328009749) fine-tune sync conditions in getblocktemplate (#1739)
- [`362becbcc`](https://github.com/dashpay/dash/commit/362becbcc) Bump MIN_PEER_PROTO_VERSION to 70208 (#1772)
- [`930afd7df`](https://github.com/dashpay/dash/commit/930afd7df) Fix mnp and mnv invs (#1775)
- [`63e306148`](https://github.com/dashpay/dash/commit/63e306148) Improve sync (#1779)
- [`a79c97248`](https://github.com/dashpay/dash/commit/a79c97248) Fix ProcessVerifyBroadcast (#1780)

### Build:
- [`c166ed39b`](https://github.com/dashpay/dash/commit/c166ed39b) Allow compilation with `--disable-wallet` (#1733)
- [`31bc9d4ee`](https://github.com/dashpay/dash/commit/31bc9d4ee) Show test progress for tests running in wine to avoid Travis timeout (#1740)
- [`32f21698e`](https://github.com/dashpay/dash/commit/32f21698e) Adjust tests to avoid Travis timeouts (#1745)
- [`837c4fc5d`](https://github.com/dashpay/dash/commit/837c4fc5d) Force rcc to use resource format version 1. (#1784)

### GUI:
- [`70cb2a4af`](https://github.com/dashpay/dash/commit/70cb2a4af) Fix traditional UI theme (#1741)
- [`e975f891c`](https://github.com/dashpay/dash/commit/e975f891c) Fix ru typo (#1742)

### Docs:
- [`bc8342558`](https://github.com/dashpay/dash/commit/bc8342558) Two small fixes in docs (#1746)
- [`9e7cc56cb`](https://github.com/dashpay/dash/commit/9e7cc56cb) Fix typo in release-notes.md (#1759)
- [`3f3705c47`](https://github.com/dashpay/dash/commit/3f3705c47) [Trivial] Typo/doc updates and RPC help formatting (#1758)
- [`e96da9f19`](https://github.com/dashpay/dash/commit/e96da9f19) move 0.12.2 release notes
- [`6915ee45e`](https://github.com/dashpay/dash/commit/6915ee45e) Bump version in README.md to 0.12.2 (#1774)
- [`0291604ad`](https://github.com/dashpay/dash/commit/0291604ad) Clarify usage of pointers and references in code (#1778)

### Other:
- [`ccbd5273e`](https://github.com/dashpay/dash/commit/ccbd5273e) bump to 0.12.3.0 (#1726)
- [`865b61b50`](https://github.com/dashpay/dash/commit/865b61b50) Unify GetNextWorkRequired (#1737)
- [`d1aeac1b2`](https://github.com/dashpay/dash/commit/d1aeac1b2) Spelling mistake in validation.cpp (#1752)
- [`442325b07`](https://github.com/dashpay/dash/commit/442325b07) add `maxgovobjdatasize` field to the output of `getgovernanceinfo` (#1757)
- [`c5ec2f82a`](https://github.com/dashpay/dash/commit/c5ec2f82a) Drop `IsNormalPaymentScript`, use `IsPayToPublicKeyHash` (#1761)
- [`f9f28e7c7`](https://github.com/dashpay/dash/commit/f9f28e7c7) De-bump to 0.12.2.2 (#1768)
- [`54186a159`](https://github.com/dashpay/dash/commit/54186a159) Make sure additional indexes are recalculated correctly in VerifyDB (#1773)
- [`86e6f0dd2`](https://github.com/dashpay/dash/commit/86e6f0dd2) Fix CMasternodeMan::ProcessVerify* logs (#1782)


Credits
=======

Thanks to everyone who directly contributed to this release:

- Alexander Block
- shade
- sidhujag
- thephez
- turbanoff
- Ilya Savinov
- UdjinM6
- Will Wray

As well as Bitcoin Core Developers and everyone that submitted issues,
reviewed pull requests or helped translating on
[Transifex](https://www.transifex.com/projects/p/dash/).


Older releases
==============

Dash was previously known as Darkcoin.

Darkcoin tree 0.8.x was a fork of Litecoin tree 0.8, original name was XCoin
which was first released on Jan/18/2014.

Darkcoin tree 0.9.x was the open source implementation of masternodes based on
the 0.8.x tree and was first released on Mar/13/2014.

Darkcoin tree 0.10.x used to be the closed source implementation of Darksend
which was released open source on Sep/25/2014.

Dash Core tree 0.11.x was a fork of Bitcoin Core tree 0.9,
Darkcoin was rebranded to Dash.

Dash Core tree 0.12.0.x was a fork of Bitcoin Core tree 0.10.

Dash Core tree 0.12.1.x was a fork of Bitcoin Core tree 0.12.

These release are considered obsolete. Old release notes can be found here:

- [v0.12.2](release-notes/dash/release-notes-0.12.2.md) released Nov/08/2017
- [v0.12.1](release-notes/dash/release-notes-0.12.1.md) released ???/??/2016
- [v0.12.0](release-notes/dash/release-notes-0.12.0.md) released ???/??/2015
- [v0.11.2](release-notes/dash/release-notes-0.11.2.md) released Mar/25/2015
- [v0.11.1](release-notes/dash/release-notes-0.11.1.md) released Feb/10/2015
- [v0.11.0](release-notes/dash/release-notes-0.11.0.md) released Jan/15/2015
- [v0.10.x](release-notes/dash/release-notes-0.10.0.md) released Sep/25/2014
- [v0.9.x](release-notes/dash/release-notes-0.9.0.md) released Mar/13/2014

