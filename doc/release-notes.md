Bitcoin Core version 0.13.x is now available from:

  <https://bitcoin.org/bin/bitcoin-core-0.13.x/>

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
suggested to upgrade to a newer version of Windows, or install an alternative OS
that is supported.

No attempt is made to prevent installing or running the software on Windows XP,
you can still do so at your own risk, but do not expect it to work: do not
report issues about Windows XP to the issue tracker.

Notable changes
===============

Example item
--------------

Low-level RPC changes
---------------------

- `importprunedfunds` only accepts two required arguments. Some versions accept
  an optional third arg, which was always ignored. Make sure to never pass more
  than two arguments.


0.13.1 Change log
=================

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors and string updates. For convenience in locating
the code changes and accompanying discussion, both the pull request and
git merge commit are mentioned.

### Consensus
- #8636 `9dfa0c8` Implement NULLDUMMY softfork (BIP147) (jl2012)
- #8848 `7a34a46` Add NULLDUMMY verify flag in bitcoinconsensus.h (jl2012)
- #8937 `8b66659` Define start and end time for segwit deployment (sipa)

### RPC and other APIs
- #8581 `526d2b0` Drop misleading option in importprunedfunds (MarcoFalke)
- #8699 `a5ec248` Remove createwitnessaddress RPC command (jl2012)
- #8780 `794b007` Deprecate getinfo (MarcoFalke)
- #8832 `83ad563` Throw JSONRPCError when utxo set can not be read (MarcoFalke)
- #8884 `b987348` getblockchaininfo help: pruneheight is the lowest, not highest, block (luke-jr)

### Block and transaction handling
- #8611 `a9429ca` Reduce default number of blocks to check at startup (sipa)
- #8634 `3e80ab7` Add policy: null signature for failed CHECK(MULTI)SIG (jl2012)
- #8525 `1672225` Do not store witness txn in rejection cache (sipa)
- #8499 `9777fe1` Add several policy limits and disable uncompressed keys for segwit scripts (jl2012)
- #8526 `0027672` Make non-minimal OP_IF/NOTIF argument non-standard for P2WSH (jl2012)
- #8524 `b8c79a0` Precompute sighashes (sipa)
- #8651 `b8c79a0` Predeclare PrecomputedTransactionData as struct (sipa)

### P2P protocol and network code
- #8740 `42ea51a` No longer send local address in addrMe (laanwj)
- #8427 `69d1cd2` Ignore `notfound` P2P messages (laanwj)
- #8573 `4f84082` Set jonasschnellis dns-seeder filter flag (jonasschnelli)
- #8712 `23feab1` Remove maxuploadtargets recommended minimum (jonasschnelli)
- #8862 `7ae6242` Fix a few cases where messages were sent after requested disconnect (theuni)
- #8393 `fe1975a` Support for compact blocks together with segwit (sipa)
- #8282 `2611ad7` Feeler connections to increase online addrs in the tried table (EthanHeilman)
- #8612 `2215c22` Check for compatibility with download in FindNextBlocksToDownload (sipa)
- #8606 `bbf379b` Fix some locks (sipa)
- #8594 `ab295bb` Do not add random inbound peers to addrman (gmaxwell)

### Build system
- #8293 `fa5b249` Allow building libbitcoinconsensus without any univalue (luke-jr)
- #8492 `8b0bdd3` Allow building bench_bitcoin by itself (luke-jr)
- #8563 `147003c` Add configure check for -latomic (ajtowns)
- #8626 `ea51b0f` Berkeley DB v6 compatibility fix (netsafe)
- #8520 `75f2065` Remove check for `openssl/ec.h` (laanwj)

### GUI
- #8481 `d9f0d4e` Fix minimize and close bugs (adlawren)
- #8487 `a37cec5` Persist the datadir after option reset (achow101)
- #8697 `41fd852` Fix op order to append first alert (rodasmith)
- #8678 `8e03382` Fix UI bug that could result in paying unexpected fee (jonasschnelli)
- #8911 `7634d8e` Translate all files, even if wallet disabled (laanwj)
- #8540 `1db3352` Fix random segfault when closing "Choose data directory" dialog (laanwj)
- #7579 `f1c0d78` Show network/chain errors in the GUI (jonasschnelli)

### Wallet
- #8443 `464dedd` Trivial cleanup of HD wallet changes (jonasschnelli)
- #8539 `cb07f19` CDB: fix debug output (crowning-)
- #8664 `091cdeb` Fix segwit-related wallet bug (sdaftuar)
- #8693 `c6a6291` Add witness address to address book (instagibbs)
- #8765 `6288659` Remove "unused" ThreadFlushWalletDB from removeprunedfunds (jonasschnelli)

### Tests and QA
- #8713 `ae8c7df` create_cache: Delete temp dir when done (MarcoFalke)
- #8716 `e34374e` Check legacy wallet as well (MarcoFalke)
- #8750 `d6ebe13` Refactor RPCTestHandler to prevent TimeoutExpired (MarcoFalke)
- #8652 `63462c2` remove root test directory for RPC tests (yurizhykin)
- #8724 `da94272` walletbackup: Sync blocks inside the loop (MarcoFalke)
- #8400 `bea02dc` enable rpcbind_test (yurizhykin)
- #8417 `f70be14` Add walletdump RPC test (including HD- & encryption-tests) (jonasschnelli)
- #8419 `a7aa3cc` Enable size accounting in mining unit tests (sdaftuar)
- #8442 `8bb1efd` Rework hd wallet dump test (MarcoFalke)
- #8528 `3606b6b` Update p2p-segwit.py to reflect correct behavior (instagibbs)
- #8531 `a27cdd8` abandonconflict: Use assert_equal (MarcoFalke)
- #8667 `6b07362` Fix SIGHASH_SINGLE bug in test_framework SignatureHash (jl2012)
- #8673 `03b0196` Fix obvious assignment/equality error in test (JeremyRubin)
- #8739 `cef633c` Fix broken sendcmpct test in p2p-compactblocks.py (sdaftuar)
- #8418 `ff893aa` Add tests for compact blocks (sdaftuar)
- #8803 `375437c` Ping regularly in p2p-segwit.py to keep connection alive (jl2012)
- #8827 `9bbe66e` Split up slow RPC calls to avoid pruning test timeouts (sdaftuar)
- #8829 `2a8bca4` Add bitcoin-tx JSON tests (jnewbery)
- #8834 `1dd1783` blockstore: Switch to dumb dbm (MarcoFalke)
- #8835 `d87227d` nulldummy.py: Don't run unused code (MarcoFalke)
- #8836 `eb18cc1` bitcoin-util-test.py should fail if the output file is empty (jnewbery)
- #8839 `31ab2f8` Avoid ConnectionResetErrors during RPC tests (laanwj)
- #8840 `cbc3fe5` Explicitly set encoding to utf8 when opening text files (laanwj)
- #8841 `3e4abb5` Fix nulldummy test (jl2012)
- #8854 `624a007` Fix race condition in p2p-compactblocks test (sdaftuar)
- #8857 `1f60d45` mininode: Only allow named args in wait_until (MarcoFalke)
- #8860 `0bee740` util: Move wait_bitcoinds() into stop_nodes() (MarcoFalke)
- #8882 `b73f065` Fix race conditions in p2p-compactblocks.py and sendheaders.py (sdaftuar)
- #8904 `cc6f551` Fix compact block shortids for a test case (dagurval)

### Documentation
- #8754 `0e2c6bd` Target protobuf 2.6 in OS X build notes. (fanquake)
- #8461 `b17a3f9` Document return value of networkhashps for getmininginfo RPC endpoint (jlopp)
- #8512 `156e305` Corrected JSON typo on setban of net.cpp (sevastos)
- #8683 `8a7d7ff` Fix incorrect file name bitcoin.qrc  (bitcoinsSG)
- #8891 `5e0dd9e` Update bips.md for Segregated Witness (fanquake)
- #8545 `863ae74` Update git-subtree-check.sh README (MarcoFalke)
- #8607 `486650a` Fix doxygen off-by-one comments, fix typos (MarcoFalke)
- #8560 `c493f43` Fix two VarInt examples in serialize.h (cbarcenas)
- #8737 `084cae9` UndoReadFromDisk works on undo files (rev), not on block files (paveljanik)
- #8625 `0a35573` Clarify statement about parallel jobs in rpc-tests.py (isle2983)
- #8624 `0e6d753` build: Mention curl (MarcoFalke)
- #8604 `b09e13c` build,doc: Update for 0.13.0+ and OpenBSD 5.9 (laanwj)

### Miscellaneous
- #8742 `d31ac72` Specify Protobuf version 2 in paymentrequest.proto (fanquake)
- #8414,#8558,#8676,#8700,#8701,#8702 Add missing copyright headers (isle2983, kazcw)
- #8899 `4ed2627` Fix wake from sleep issue with Boost 1.59.0 (fanquake)
- #8817 `bcf3806` update bitcoin-tx to output witness data (jnewbery)
- #8513 `4e5fc31` Fix a type error that would not compile on OSX. (JeremyRubin)
- #8392 `30eac2d` Fix several node initialization issues (sipa)
- #8548 `305d8ac` Use `__func__` to get function name for output printing (MarcoFalke)
- #8291 `a987431` [util] CopyrightHolders: Check for untranslated substitution (MarcoFalke)

Credits
=======

Thanks to everyone who directly contributed to this release:

- adlawren
- Alexey Vesnin
- Anders Øyvind Urke-Sætre
- Andrew Chow
- Anthony Towns
- BtcDrak
- Chris Stewart
- Christian Barcenas
- Cory Fields
- crowning-
- Dagur Valberg Johannsson
- Ethan Heilman
- fanquake
- Gaurav Rana
- Gregory Maxwell
- instagibbs
- isle2983
- Jameson Lopp
- Jeremy Rubin
- jnewbery
- Johnson Lau
- Jonas Schnelli
- jonnynewbs
- Justin Camarena
- Kaz Wesley
- leijurv
- Luke Dashjr
- MarcoFalke
- Marty Jones
- Matt Corallo
- Michael Ford
- Pavel Janík
- Pieter Wuille
- rodasmith
- Sev
- Suhas Daftuar
- whythat
- Wladimir J. van der Laan

As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/bitcoin/).
