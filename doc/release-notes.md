Dash Core version 0.12.2
========================

Release is now available from:

  <https://www.dash.org/downloads/#wallets>

This is a new major version release, bringing new features and other improvements.

Please report bugs using the issue tracker at github:

  <https://github.com/dashpay/dash/issues>

Upgrading and downgrading
=========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Dash-Qt (on Mac) or
dashd/dash-qt (on Linux).

Downgrade warning
-----------------
### Downgrade to a version < 0.12.2

Because release 0.12.2 includes DIP0001 (2 MB block size hardfork) plus
a transaction fee reduction and a fix for masternode rank calculation algo
(which activation also depends on DIP0001) this release will not be
backwards compatible after DIP0001 lock in/activation happens.

This does not affect wallet forward or backward compatibility.

Notable changes
===============

DIP0001
-------

We outline an initial scaling mechanism for Dash. After deployment and activation, Dash will be able to handle double the transactions it can currently handle. Together with the faster block times, Dash we will be prepared to handle eight times the traffic of Bitcoin.

https://github.com/dashpay/dips/blob/master/dip-0001.md


Fee reduction
-------------

All transaction fees are reduced 10x (from 10K per Kb to 1K per Kb), including fees for InstantSend (from 0.001 DASH per input to 0.0001 per input)

InstantSend fix
---------------

The potential vulnerability found by Matt Robertson and Alexander Block was fixed in d7a8489f3 (#1620).

RPC changes
-----------

There are few changes in existing RPC in this release:
- There is no more `bcconfirmations` field in RPC output and `confirmations` shows blockchain only confirmations by default now. You can change this behaviour by switching new `addlockconf` param to `true`. There is a new rpc field `instantlock` which indicates whether a given transaction is locked via InstantSend. For more info and examples please see https://github.com/dashpay/dash/blob/v0.12.2.x/doc/instantsend.md;
- `gobject list` and `gobject diff` accept `funding`, `delete` and `endorsed` filtering options now, in addition to `valid` and `all` currently available;
- `vin` field in `masternode` commands is renamed to `outpoint` and shows data in short format now;
- `getblocktemplate` output is extended with versionbits-related information;
- Output of wallet-related commands `validateaddress` is extended with optional `hdkeypath` and `hdchainid` fields.

There are few new RPC commands also:
- `masternodelist info` shows additional information about sentinel for each masternode in the list;
- `masternodelist pubkey` shows pubkey corresponding to masternodeprivkey for each masternode in the list;
- `gobject check` allows to check proposal data for correctness before preparing/submitting the proposal, `gobject prepare` and `gobject submit` should also perform additional validation now though;
- `setnetworkactive` allows to turn all network activity on and off;
- `dumphdinfo` displays some information about HD wallet (if available).

Command-line options
--------------------

New: `assumevalid`, `blocksonly`, `reindex-chainstate`

Experimental: `usehd`, `mnemonic`, `mnemonicpassphrase`, `hdseed`

See `Help -> Command-line options` in Qt wallet or `dashd --help` for more info.

PrivateSend improvements
------------------------

Algorithm for selecting inputs was slightly changed in [`6067896ae`](https://github.com/dashpay/dash/commit/6067896ae) ([#1248](https://github.com/dashpay/dash/pull/1248)). This should allow user to get some mixed funds much faster.

Lots of backports, refactoring and bug fixes
--------------------------------------------

We backported some performance improvements from Bitcoin Core and aligned our codebase with their source a little bit better. We still do not have all the improvements so this work is going to be continued in next releases.

A lot of refactoring and other fixes should make code more reliable and easier to review now.

Experimental HD wallet
----------------------

This release includes experimental implementation of BIP39/BIP44 compatible HD wallet. Wallet type (HD or non-HD) is selected when wallet is created via `usehd` command-line option, default is `0` which means that a regular non-deterministic wallet is going to be used. If you decide to use HD wallet, you can also specify BIP39 mnemonic and mnemonic passphrase (see `mnemonic` and `mnemonicpassphrase` command-line options) but you can do so only on initial wallet creation and can't change these afterwards. If you don't specify them, mnemonic is going to be generated randomly and mnemonic passphrase is going to be just a blank string.

**WARNING:** The way it's currently implemented is NOT safe and is NOT recommended to use on mainnet. Wallet is created unencrypted with mnemonic stored inside, so even if you encrypt it later there will be a short period of time when mnemonic is stored in plain text. This issue will be addressed in future releases.

0.12.2 Change log
=================

Detailed [change log](https://github.com/dashpay/dash/compare/v0.12.1.x...dashpay:v0.12.2.x) below.

### Backports:
- [`ff30aed68`](https://github.com/dashpay/dash/commit/ff30aed68) Align with btc 0.12 (#1409)
- [`9901cf433`](https://github.com/dashpay/dash/commit/9901cf433) Fix for dash-qt issue with startup and multiple monitors. (#1461)
- [`39750439b`](https://github.com/dashpay/dash/commit/39750439b) Force to use C++11 mode for compilation (#1463)
- [`e30faab6f`](https://github.com/dashpay/dash/commit/e30faab6f) Make strWalletFile const (#1459)
- [`c4fe22900`](https://github.com/dashpay/dash/commit/c4fe22900) Access WorkQueue::running only within the cs lock. (#1460)
- [`8572d54a9`](https://github.com/dashpay/dash/commit/8572d54a9) trivial: fix bloom filter init to isEmpty = true (#1458)
- [`b272ae56a`](https://github.com/dashpay/dash/commit/b272ae56a) Avoid ugly exception in log on unknown inv type (#1457)
- [`e99dbe620`](https://github.com/dashpay/dash/commit/e99dbe620) Don't return the address of a P2SH of a P2SH (#1455)
- [`f24efd483`](https://github.com/dashpay/dash/commit/f24efd483) Generate auth cookie in hex instead of base64 (#1454)
- [`3efcb755e`](https://github.com/dashpay/dash/commit/3efcb755e) Do not shadow LOCK's criticalblock variable for LOCK inside LOCK (#1453)
- [`b7822464f`](https://github.com/dashpay/dash/commit/b7822464f) Remove unnecessary LOCK(cs_main) in getrawpmempool (#1452)
- [`72176e501`](https://github.com/dashpay/dash/commit/72176e501) Remove duplicate bantablemodel.h include (#1446)
- [`634ef6c06`](https://github.com/dashpay/dash/commit/634ef6c06) Fix for build on Ubuntu 14.04 with system libraries (#1467)
- [`c0450f609`](https://github.com/dashpay/dash/commit/c0450f609) Improve EncodeBase58/DecodeBase58 performance (#1456)
- [`258ed119a`](https://github.com/dashpay/dash/commit/258ed119a) auto_ptr → unique_ptr
- [`be968206b`](https://github.com/dashpay/dash/commit/be968206b) Boost 1.61.0
- [`7f8775409`](https://github.com/dashpay/dash/commit/7f8775409) Boost 1.63.0
- [`11afc8f4b`](https://github.com/dashpay/dash/commit/11afc8f4b) Minimal fix to slow prevector tests as stopgap measure
- [`11121747b`](https://github.com/dashpay/dash/commit/11121747b) build: fix qt5.7 build under macOS (#1469)
- [`5988e1e7f`](https://github.com/dashpay/dash/commit/5988e1e7f) Increase minimum debug.log size to 10MB after shrink. (#1480)
- [`a443d4e2d`](https://github.com/dashpay/dash/commit/a443d4e2d) Backport Bitcoin PRs #6589, #7180 and remaining part of #7181: enable per-command byte counters in `CNode` (#1496)
- [`f9730cb2e`](https://github.com/dashpay/dash/commit/f9730cb2e) Increase test coverage for addrman and addrinfo (#1497)
- [`a12491448`](https://github.com/dashpay/dash/commit/a12491448) Eliminate unnecessary call to CheckBlock (#1498)
- [`b0843c397`](https://github.com/dashpay/dash/commit/b0843c397) Backport Bincoin PR#7348: MOVE ONLY: move rpc* to rpc/ + same for Dash-specific rpc (#1502)
- [`f65017cfe`](https://github.com/dashpay/dash/commit/f65017cfe) Backport Bitcoin PR#7349: Build against system UniValue when available (#1503)
- [`ac6c3c900`](https://github.com/dashpay/dash/commit/ac6c3c900) Backport Bitcoin PR#7350: Banlist updates (#1505)
- [`d787fe4ab`](https://github.com/dashpay/dash/commit/d787fe4ab) Backport Bitcoin PR#7458: [Net] peers.dat, banlist.dat recreated when missing (#1506)
- [`6af9955fa`](https://github.com/dashpay/dash/commit/6af9955fa) Backport Bitcoin PR#7696: Fix de-serialization bug where AddrMan is corrupted after exception (#1507)
- [`b39c518d5`](https://github.com/dashpay/dash/commit/b39c518d5) Backport Bitcoin PR#7749: Enforce expected outbound services (#1508)
- [`9268a336d`](https://github.com/dashpay/dash/commit/9268a336d) Backport Bitcoin PR#7917: Optimize reindex (#1515)
- [`b47984f30`](https://github.com/dashpay/dash/commit/b47984f30) Remove non-determinism which is breaking net_tests #8069 (#1517)
- [`9a8a290b8`](https://github.com/dashpay/dash/commit/9a8a290b8) fix race that could fail to persist a ban (#1518)
- [`5a1961e5e`](https://github.com/dashpay/dash/commit/5a1961e5e) Backport Bitcoin PR#7906: net: prerequisites for p2p encapsulation changes (#1521)
- [`7b5556a29`](https://github.com/dashpay/dash/commit/7b5556a29) Backport Bitcoin PR#8084: Add recently accepted blocks and txn to AttemptToEvictConnection (#1522)
- [`9ce2b966c`](https://github.com/dashpay/dash/commit/9ce2b966c) Backport Bitcoin PR#8113: Rework addnode behaviour (#1525)
- [`aa32f1dc9`](https://github.com/dashpay/dash/commit/aa32f1dc9) Remove bad chain alert partition check (#1529)
- [`d934ffb2f`](https://github.com/dashpay/dash/commit/d934ffb2f) Added feeler connections increasing good addrs in the tried table. (#1530)
- [`290fb3b57`](https://github.com/dashpay/dash/commit/290fb3b57) Backport Bitcoin PR#7942: locking for Misbehave() and other cs_main locking fixes (#1535)
- [`a9d771e49`](https://github.com/dashpay/dash/commit/a9d771e49) Backport Bitcoin PR#8085: p2p: Begin encapsulation (#1537)
- [`82851b439`](https://github.com/dashpay/dash/commit/82851b439) Backport Bitcoin PR#8049: Expose information on whether transaction relay is enabled in `getnetwork` (#1545)
- [`7707c0789`](https://github.com/dashpay/dash/commit/7707c0789) backport 9008: Remove assert(nMaxInbound > 0) (#1548)
- [`b621cfb5f`](https://github.com/dashpay/dash/commit/b621cfb5f) Backport Bitcoin PR#8708: net: have CConnman handle message sending (#1553)
- [`cc4db34f4`](https://github.com/dashpay/dash/commit/cc4db34f4) net: only delete CConnman if it's been created (#1555)
- [`a3c8cb20d`](https://github.com/dashpay/dash/commit/a3c8cb20d) Backport Bitcoin PR#8865: Decouple peer-processing-logic from block-connection-logic (#1556)
- [`e7e106e22`](https://github.com/dashpay/dash/commit/e7e106e22) Backport Bitcoin PR#8969: Decouple peer-processing-logic from block-connection-logic (#2) (#1558)
- [`b4b343145`](https://github.com/dashpay/dash/commit/b4b343145) Backport Bitcoin PR#9075: Decouple peer-processing-logic from block-connection-logic (#3) (#1560)
- [`415085c73`](https://github.com/dashpay/dash/commit/415085c73) Backport Bitcoin PR#9183: Final Preparation for main.cpp Split (#1561)
- [`bcf5455bf`](https://github.com/dashpay/dash/commit/bcf5455bf) Backport Bitcoin PR#8822: net: Consistent checksum handling (#1565)
- [`df6d458b8`](https://github.com/dashpay/dash/commit/df6d458b8) Backport Bitcoin PR#9260: Mrs Peacock in The Library with The Candlestick (killed main.{h,cpp}) (#1566)
- [`42c784dc7`](https://github.com/dashpay/dash/commit/42c784dc7) Backport Bitcoin PR#9289: net: drop boost::thread_group (#1568)
- [`b9c67258b`](https://github.com/dashpay/dash/commit/b9c67258b) Backport Bitcoin PR#9609: net: fix remaining net assertions (#1575) + Dashify
- [`2472999da`](https://github.com/dashpay/dash/commit/2472999da) Backport Bitcoin PR#9441: Net: Massive speedup. Net locks overhaul (#1586)
- [`ccee103a0`](https://github.com/dashpay/dash/commit/ccee103a0) Backport "assumed valid blocks" feature from Bitcoin 0.13 (#1582)
- [`105122181`](https://github.com/dashpay/dash/commit/105122181) Partially backport Bitcoin PR#9626: Clean up a few CConnman cs_vNodes/CNode things (#1591)
- [`76181f575`](https://github.com/dashpay/dash/commit/76181f575) Do not add random inbound peers to addrman. (#1593)
- [`589d22f2c`](https://github.com/dashpay/dash/commit/589d22f2c) net: No longer send local address in addrMe (#1600)
- [`b41d9eac2`](https://github.com/dashpay/dash/commit/b41d9eac2) Backport Bitcoin PR#7868: net: Split DNS resolving functionality out of net structures (#1601)
- [`b82b9787d`](https://github.com/dashpay/dash/commit/b82b9787d) Backport Bitcoin PR#8128: Net: Turn net structures into dumb storage classes (#1604)
- [`690cb58f8`](https://github.com/dashpay/dash/commit/690cb58f8) Backport Bitcoin Qt/Gui changes up to 0.14.x part 1 (#1614)
- [`9707ca5ce`](https://github.com/dashpay/dash/commit/9707ca5ce) Backport Bitcoin Qt/Gui changes up to 0.14.x part 2 (#1615)
- [`91d99fcd3`](https://github.com/dashpay/dash/commit/91d99fcd3) Backport Bitcoin Qt/Gui changes up to 0.14.x part 3 (#1617)
- [`4cac044d9`](https://github.com/dashpay/dash/commit/4cac044d9) Merge #8944: Remove bogus assert on number of oubound connections. (#1685)
- [`d23adcc0f`](https://github.com/dashpay/dash/commit/d23adcc0f) Merge #10231: [Qt] Reduce a significant cs_main lock freeze (#1704)

### PrivateSend:
- [`6067896ae`](https://github.com/dashpay/dash/commit/6067896ae) mix inputs with highest number of rounds first (#1248)
- [`559f8421b`](https://github.com/dashpay/dash/commit/559f8421b) Few fixes for PrivateSend (#1408)
- [`7242e2922`](https://github.com/dashpay/dash/commit/7242e2922) Refactor PS (#1437)
- [`68e858f8d`](https://github.com/dashpay/dash/commit/68e858f8d) PrivateSend: dont waste keys from keypool on failure in CreateDenominated (#1473)
- [`2daea77a5`](https://github.com/dashpay/dash/commit/2daea77a5) fix calculation of (unconfirmed) anonymizable balance (#1477)
- [`029ee6494`](https://github.com/dashpay/dash/commit/029ee6494) split CPrivateSend (#1492)
- [`739ef9a68`](https://github.com/dashpay/dash/commit/739ef9a68) Expire confirmed DSTXes after ~1h since confirmation (#1499)
- [`7abac068b`](https://github.com/dashpay/dash/commit/7abac068b) fix MakeCollateralAmounts (#1500)
- [`0f05e25c7`](https://github.com/dashpay/dash/commit/0f05e25c7) fix a bug in CommitFinalTransaction (#1540)
- [`82595b1b9`](https://github.com/dashpay/dash/commit/82595b1b9) fix number of blocks to wait after successful mixing tx (#1597)
- [`33e460f30`](https://github.com/dashpay/dash/commit/33e460f30) Fix losing keys on PrivateSend (#1616)
- [`ea793a71f`](https://github.com/dashpay/dash/commit/ea793a71f) Make sure mixing masternode follows bip69 before signing final mixing tx (#1510)
- [`8c15f5f87`](https://github.com/dashpay/dash/commit/8c15f5f87) speedup MakeCollateralAmounts by skiping denominated inputs early (#1631)
- [`8e9289e12`](https://github.com/dashpay/dash/commit/8e9289e12) Keep track of wallet UTXOs and use them for PS balances and rounds calculations (#1655)
- [`f77efcf24`](https://github.com/dashpay/dash/commit/f77efcf24) do not calculate stuff that are not going to be visible in simple PS UI anyway (#1656)

### InstantSend:
- [`68e1a8c79`](https://github.com/dashpay/dash/commit/68e1a8c79) Safety check in CInstantSend::SyncTransaction (#1412)
- [`4a9fbca08`](https://github.com/dashpay/dash/commit/4a9fbca08) Fix potential deadlock in CInstantSend::UpdateLockedTransaction (#1571)
- [`f786ce6ab`](https://github.com/dashpay/dash/commit/f786ce6ab) fix instantsendtoaddress param convertion (#1585)
- [`ae909d0a0`](https://github.com/dashpay/dash/commit/ae909d0a0) Fix: Reject invalid instantsend transaction (#1583)
- [`84ecccefc`](https://github.com/dashpay/dash/commit/84ecccefc) InstandSend overhaul (#1592)
- [`5f4362cb8`](https://github.com/dashpay/dash/commit/5f4362cb8) fix SPORK_5_INSTANTSEND_MAX_VALUE validation in CWallet::CreateTransaction (#1619)
- [`d7a8489f3`](https://github.com/dashpay/dash/commit/d7a8489f3) Fix masternode score/rank calculations (#1620)
- [`b41f8d3dd`](https://github.com/dashpay/dash/commit/b41f8d3dd) fix instantsend-related RPC output (#1628)
- [`502748487`](https://github.com/dashpay/dash/commit/502748487) bump MIN_INSTANTSEND_PROTO_VERSION to 70208 (#1650)
- [`788ae63ac`](https://github.com/dashpay/dash/commit/788ae63ac) Fix edge case for IS (skip inputs that are too large) (#1695)
- [`470e5435c`](https://github.com/dashpay/dash/commit/470e5435c) remove InstantSend votes for failed lock attemts after some timeout (#1705)
- [`a9293ad03`](https://github.com/dashpay/dash/commit/a9293ad03) update setAskFor on TXLOCKVOTE (#1713)
- [`859144809`](https://github.com/dashpay/dash/commit/859144809) fix bug introduced in #1695 (#1714)

### Governance:
- [`4595db0ce`](https://github.com/dashpay/dash/commit/4595db0ce) Few changes for governance rpc: (#1351)
- [`411332f94`](https://github.com/dashpay/dash/commit/411332f94) sentinel uses status of funding votes (#1440)
- [`a109a611f`](https://github.com/dashpay/dash/commit/a109a611f) Validate proposals on prepare and submit (#1488)
- [`a439e9840`](https://github.com/dashpay/dash/commit/a439e9840) Replace watchdogs with ping (#1491)
- [`109c5fd1d`](https://github.com/dashpay/dash/commit/109c5fd1d) Fixed issues with propagation of governance objects (#1489)
- [`f7aa81586`](https://github.com/dashpay/dash/commit/f7aa81586) Fix issues with mapSeenGovernanceObjects (#1511)
- [`8075370d1`](https://github.com/dashpay/dash/commit/8075370d1) change invalid version string constant (#1532)
- [`916af52c0`](https://github.com/dashpay/dash/commit/916af52c0) Fix vulnerability with mapMasternodeOrphanObjects (#1512)
- [`70eb83a5c`](https://github.com/dashpay/dash/commit/70eb83a5c) New rpc call "masternodelist info" (#1513)
- [`f47f0daf9`](https://github.com/dashpay/dash/commit/f47f0daf9) add 6 to strAllowedChars (#1542)
- [`6cb3fddcc`](https://github.com/dashpay/dash/commit/6cb3fddcc) fixed potential deadlock in CSuperblockManager::IsSuperblockTriggered (#1536)
- [`15958c594`](https://github.com/dashpay/dash/commit/15958c594) fix potential deadlock (PR#1536 fix) (#1538)
- [`1c4e2946a`](https://github.com/dashpay/dash/commit/1c4e2946a) fix potential deadlock in CGovernanceManager::ProcessVote (#1541)
- [`4942884c7`](https://github.com/dashpay/dash/commit/4942884c7) fix potential deadlock in CMasternodeMan::CheckMnbAndUpdateMasternodeList (#1543)
- [`4ed838cb5`](https://github.com/dashpay/dash/commit/4ed838cb5) Fix MasternodeRateCheck (#1490)
- [`6496fc9da`](https://github.com/dashpay/dash/commit/6496fc9da) fix off-by-1 in CSuperblock::GetPaymentsLimit (#1598)
- [`48d63ab29`](https://github.com/dashpay/dash/commit/48d63ab29) Relay govobj and govvote to every compatible peer, not only to the one with the same version (#1662)
- [`6f57519c6`](https://github.com/dashpay/dash/commit/6f57519c6) allow up to 40 chars in proposal name (#1693)
- [`ceda3abe6`](https://github.com/dashpay/dash/commit/ceda3abe6) start_epoch, end_epoch and payment_amount should be numbers, not strings (#1707)

### Network/Sync:
- [`62963e911`](https://github.com/dashpay/dash/commit/62963e911) fix sync reset which is triggered erroneously during reindex (#1478)
- [`fc406f2d8`](https://github.com/dashpay/dash/commit/fc406f2d8) track asset sync time (#1479)
- [`9e9df2820`](https://github.com/dashpay/dash/commit/9e9df2820) do not use masternode connections in feeler logic (#1533)
- [`9694658cd`](https://github.com/dashpay/dash/commit/9694658cd) Make sure mixing messages are relayed/accepted properly (#1547)
- [`87707c012`](https://github.com/dashpay/dash/commit/87707c012) fix CDSNotificationInterface::UpdatedBlockTip signature (#1562)
- [`105713c10`](https://github.com/dashpay/dash/commit/105713c10) Sync overhaul (#1564)
- [`0fc13434b`](https://github.com/dashpay/dash/commit/0fc13434b) limit UpdatedBlockTip in IBD (#1570)
- [`510c0a06d`](https://github.com/dashpay/dash/commit/510c0a06d) Relay tx in sendrawtransaction according to its inv.type (#1584)
- [`c56ba56e7`](https://github.com/dashpay/dash/commit/c56ba56e7) net: Consistently use GetTimeMicros() for inactivity checks (#1588)
- [`4f5455000`](https://github.com/dashpay/dash/commit/4f5455000) Use GetAdjustedTime instead of GetTime when dealing with network-wide timestamps (#1590)
- [`4f0618ae8`](https://github.com/dashpay/dash/commit/4f0618ae8) Fix sync issues (#1599)
- [`169afafd5`](https://github.com/dashpay/dash/commit/169afafd5) Fix duplicate headers download in initial sync (#1589)
- [`91ae0b712`](https://github.com/dashpay/dash/commit/91ae0b712) Use connman passed to ThreadSendAlert() instead of g_connman global. (#1610)
- [`8da26da71`](https://github.com/dashpay/dash/commit/8da26da71) Eliminate g_connman use in spork module. (#1613)
- [`4956ba7a7`](https://github.com/dashpay/dash/commit/4956ba7a7) Eliminate g_connman use in instantx module. (#1626)
- [`10eddb52d`](https://github.com/dashpay/dash/commit/10eddb52d) Move some (spamy) CMasternodeSync log messages to new `mnsync` log category (#1630)
- [`753b1e486`](https://github.com/dashpay/dash/commit/753b1e486) Eliminate remaining uses of g_connman in Dash-specific code. (#1635)
- [`8949f4345`](https://github.com/dashpay/dash/commit/8949f4345) Wait for full sync in functional tests that use getblocktemplate. (#1642)
- [`5f0da8aa7`](https://github.com/dashpay/dash/commit/5f0da8aa7) fix sync (#1643)
- [`7a8910443`](https://github.com/dashpay/dash/commit/7a8910443) Fix unlocked access to vNodes.size() (#1654)
- [`278cf144b`](https://github.com/dashpay/dash/commit/278cf144b) Remove cs_main from ThreadMnbRequestConnections (#1658)
- [`52cd4d40d`](https://github.com/dashpay/dash/commit/52cd4d40d) Fix bug: nCachedBlockHeight was not updated on start (#1673)
- [`1df889e23`](https://github.com/dashpay/dash/commit/1df889e23) Add more logging for MN votes and MNs missing votes (#1683)
- [`28c8d1729`](https://github.com/dashpay/dash/commit/28c8d1729) Fix sync reset on lack of activity (#1686)
- [`dfb8dbbf6`](https://github.com/dashpay/dash/commit/dfb8dbbf6) Fix mnp relay bug (#1700)

### GUI:
- [`5758ae1bf`](https://github.com/dashpay/dash/commit/5758ae1bf) Full path in "failed to load cache" warnings (#1411)
- [`18c83f58e`](https://github.com/dashpay/dash/commit/18c83f58e) Qt: bug fixes and enhancement to traffic graph widget  (#1429)
- [`72fbfe93d`](https://github.com/dashpay/dash/commit/72fbfe93d) Icon Cutoff Fix (#1485)
- [`4df8a20f9`](https://github.com/dashpay/dash/commit/4df8a20f9) Fix windows installer script, should handle `dash:` uri correctly now (#1550)
- [`8b7dffbb6`](https://github.com/dashpay/dash/commit/8b7dffbb6) Update startup shortcuts (#1551)
- [`6ff7b7aa5`](https://github.com/dashpay/dash/commit/6ff7b7aa5) fix TrafficGraphData bandwidth calculation (#1618)
- [`026ad8421`](https://github.com/dashpay/dash/commit/026ad8421) Fix empty tooltip during sync under specific conditions (#1637)
- [`08e503da2`](https://github.com/dashpay/dash/commit/08e503da2) [GUI] Change look of modaloverlay (#1653)
- [`296cfd2ef`](https://github.com/dashpay/dash/commit/296cfd2ef) Fix compilation with qt < 5.2 (#1672)
- [`11afd7cfd`](https://github.com/dashpay/dash/commit/11afd7cfd) Translations201710 - en, de, fi, fr, ru, vi (#1659)
- [`14d11e4a8`](https://github.com/dashpay/dash/commit/14d11e4a8) Translations 201710 part2 (#1676)
- [`2144dae91`](https://github.com/dashpay/dash/commit/2144dae91) Add hires version of `light` theme for Hi-DPI screens (#1712)

### DIP0001:
- [`cd262bf64`](https://github.com/dashpay/dash/commit/cd262bf64) DIP0001 implementation (#1594)
- [`6a6b31b74`](https://github.com/dashpay/dash/commit/6a6b31b74) Dip0001-related adjustments (inc. constants) (#1621)
- [`e22453c90`](https://github.com/dashpay/dash/commit/e22453c90) fix fDIP0001* flags initialization (#1625)
- [`25fa44d5a`](https://github.com/dashpay/dash/commit/25fa44d5a) fix DIP0001 implementation (#1639)
- [`d07ac4fbd`](https://github.com/dashpay/dash/commit/d07ac4fbd) fix: update DIP0001 related stuff even in IBD (#1652)
- [`d28619872`](https://github.com/dashpay/dash/commit/d28619872) fix: The idea behind fDIP0001LockedInAtTip was to indicate that it WAS locked in, not that it IS locked in (#1666)

### Wallet:
- [`27f3218de`](https://github.com/dashpay/dash/commit/27f3218de) HD wallet (#1405)
- [`b6804678f`](https://github.com/dashpay/dash/commit/b6804678f) Minor Warning Fixed (#1482)
- [`cd76f2a15`](https://github.com/dashpay/dash/commit/cd76f2a15) Disable HD wallet by default (#1629)
- [`8f850c60f`](https://github.com/dashpay/dash/commit/8f850c60f) Lower tx fees 10x (#1632)
- [`7ab175a8e`](https://github.com/dashpay/dash/commit/7ab175a8e) Ensure Dash wallets < 0.12.2 can't open HD wallets (#1638)
- [`7efa5e79d`](https://github.com/dashpay/dash/commit/7efa5e79d) fix fallback fee (#1649)

### RPC:
- [`a0851494d`](https://github.com/dashpay/dash/commit/a0851494d) add `masternodelist pubkey` to rpc (#1549)
- [`825b3ccc9`](https://github.com/dashpay/dash/commit/825b3ccc9) more "vin" -> "outpoint" in masternode rpc output (#1633)
- [`0c1679e58`](https://github.com/dashpay/dash/commit/0c1679e58) fix `masternode current` rpc (#1640)
- [`8c1e5e838`](https://github.com/dashpay/dash/commit/8c1e5e838) remove send addresses from listreceivedbyaddress output (#1664)
- [`c3bc06bbf`](https://github.com/dashpay/dash/commit/c3bc06bbf) fix Examples section of the RPC output for listreceivedbyaccount, listreceivedbyaccount and sendfrom commands (#1665)
- [`ece884994`](https://github.com/dashpay/dash/commit/ece884994) RPC help formatting updates (#1670)
- [`32ad53e77`](https://github.com/dashpay/dash/commit/32ad53e77) Revert "fix `masternode current` rpc (#1640)" (#1681)
- [`a5f99ef2f`](https://github.com/dashpay/dash/commit/a5f99ef2f) partially revert "[Trivial] RPC help formatting updates #1670" (#1711)

### Docs:
- [`82a464313`](https://github.com/dashpay/dash/commit/82a464313) Doc: fix broken formatting in markdown #headers (#1462)
- [`ee4daed83`](https://github.com/dashpay/dash/commit/ee4daed83) Added clarifications in INSTALL readme for newcomers (#1481)
- [`5d2795029`](https://github.com/dashpay/dash/commit/5d2795029) Documentation: Add spork message / details to protocol-documentation.md (#1493)
- [`5617aef07`](https://github.com/dashpay/dash/commit/5617aef07) Documentation: Update undocumented messages in protocol-documentation.md  (#1596)
- [`72ef788c5`](https://github.com/dashpay/dash/commit/72ef788c5) Update `instantsend.md` according to PR#1628 changes (#1663)
- [`304b886d0`](https://github.com/dashpay/dash/commit/304b886d0) Update build-osx.md formatting (#1690)
- [`578d55979`](https://github.com/dashpay/dash/commit/578d55979) 12.2 release notes (#1675)

### Other (noticeable) refactoring and fixes:
- [`98990b683`](https://github.com/dashpay/dash/commit/98990b683) Refactor: CDarkSendSigner (#1410)
- [`397ea95db`](https://github.com/dashpay/dash/commit/397ea95db) Implement BIP69 outside of CTxIn/CTxOut (#1514)
- [`27b6f3633`](https://github.com/dashpay/dash/commit/27b6f3633) fix deadlock (#1531)
- [`e0d6c5b5a`](https://github.com/dashpay/dash/commit/e0d6c5b5a) fixed potential deadlock in CMasternodePing::SimpleCheck (#1534)
- [`8b5f47e68`](https://github.com/dashpay/dash/commit/8b5f47e68) Masternode classes: Remove repeated/un-needed code and data (#1572)
- [`23582aea4`](https://github.com/dashpay/dash/commit/23582aea4) add/use GetUTXO\[Coins/Confirmations\] helpers instead of GetInputAge\[IX\] (#1578)
- [`fe81d641d`](https://github.com/dashpay/dash/commit/fe81d641d) drop pCurrentBlockIndex and use cached block height instead (nCachedBlockHeight) (#1579)
- [`8012f2ca7`](https://github.com/dashpay/dash/commit/8012f2ca7) drop masternode index (#1580)
- [`b5f7be649`](https://github.com/dashpay/dash/commit/b5f7be649) slightly refactor CDSNotificationInterface (#1581)
- [`9028a22b8`](https://github.com/dashpay/dash/commit/9028a22b8) safe version of GetMasternodeByRank (#1595)
- [`05da4557d`](https://github.com/dashpay/dash/commit/05da4557d) Refactor masternode management (#1611)
- [`adc7c6cb1`](https://github.com/dashpay/dash/commit/adc7c6cb1) Remove some recursive locks (#1624)

### Other (technical) commits:
- [`1a528d945`](https://github.com/dashpay/dash/commit/1a528d945) bump to 0.12.2.0 (#1407)
- [`b815a7b6a`](https://github.com/dashpay/dash/commit/b815a7b6a) Merge remote-tracking branch 'remotes/origin/master' into v0.12.2.x
- [`7a5943c3a`](https://github.com/dashpay/dash/commit/7a5943c3a) Merge pull request #1431 from dashpay/v0.12.2.x-merge_upstream
- [`8bbcf6200`](https://github.com/dashpay/dash/commit/8bbcf6200) Fixed pow (test and algo) (#1415)
- [`f3b92a95d`](https://github.com/dashpay/dash/commit/f3b92a95d) c++11: don't throw from the reverselock destructor (#1421)
- [`b40f8f333`](https://github.com/dashpay/dash/commit/b40f8f333) Rename bitcoinconsensus library to dashconsensus. (#1432)
- [`28a1d0ecc`](https://github.com/dashpay/dash/commit/28a1d0ecc) Fix the same header included twice. (#1474)
- [`adf97e12a`](https://github.com/dashpay/dash/commit/adf97e12a) fix travis ci mac build (#1483)
- [`a28fa724c`](https://github.com/dashpay/dash/commit/a28fa724c) fix BIP34 starting blocks for mainnet/testnet (#1476)
- [`bea548c61`](https://github.com/dashpay/dash/commit/bea548c61) adjust/fix some log and error messages (#1484)
- [`715504357`](https://github.com/dashpay/dash/commit/715504357) Dashify bitcoin unix executables (#1486)
- [`1d67d5212`](https://github.com/dashpay/dash/commit/1d67d5212) Don't try to create empty datadir before the real path is known (#1494)
- [`549b659e8`](https://github.com/dashpay/dash/commit/549b659e8) Force self-recheck on CActiveMasternode::ManageStateRemote() (#1441)
- [`96f0d6ec2`](https://github.com/dashpay/dash/commit/96f0d6ec2) various trivial cleanup fixes (#1501)
- [`f9dd40888`](https://github.com/dashpay/dash/commit/f9dd40888) include atomic (#1523)
- [`eea78d45e`](https://github.com/dashpay/dash/commit/eea78d45e) Revert "fixed regtest+ds issues" (#1524)
- [`1b1d52ac3`](https://github.com/dashpay/dash/commit/1b1d52ac3) workaround for travis (#1526)
- [`c608bbec1`](https://github.com/dashpay/dash/commit/c608bbec1) Pass reference when calling HasPayeeWithVotes (#1569)
- [`b22cda4df`](https://github.com/dashpay/dash/commit/b22cda4df) typo: "Writting" -> "Writing" (#1605)
- [`ace00175c`](https://github.com/dashpay/dash/commit/ace00175c) build: silence gcc7's implicit fallthrough warning (#1622)
- [`02e882c3d`](https://github.com/dashpay/dash/commit/02e882c3d) bump MIN_MASTERNODE_PAYMENT_PROTO_VERSION_2 and PROTOCOL_VERSION to 70208 (#1636)
- [`d3829e55b`](https://github.com/dashpay/dash/commit/d3829e55b) fix BIP68 granularity and mask (#1641)
- [`72b221f74`](https://github.com/dashpay/dash/commit/72b221f74) Revert "fix BIP68 granularity and mask (#1641)" (#1647)
- [`381ffdc4b`](https://github.com/dashpay/dash/commit/381ffdc4b) Fork testnet to test 12.2 migration (#1660)
- [`33dbafbba`](https://github.com/dashpay/dash/commit/33dbafbba) fork testnet again to re-test dip0001 because of 2 bugs found in 1st attempt (#1667)
- [`0b6955a7b`](https://github.com/dashpay/dash/commit/0b6955a7b) update nMinimumChainWork and defaultAssumeValid for testnet (#1668)
- [`4ecbedbe7`](https://github.com/dashpay/dash/commit/4ecbedbe7) fix `setnetworkactive` (typo) (#1682)
- [`46342b2e8`](https://github.com/dashpay/dash/commit/46342b2e8) update nCollateralMinConfBlockHash for local (hot) masternode on mn start (#1689)
- [`f5286b179`](https://github.com/dashpay/dash/commit/f5286b179) Fix/optimize images (#1688)
- [`673e161d5`](https://github.com/dashpay/dash/commit/673e161d5) fix trafficgraphdatatests for qt4 (#1699)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Alex Werner
- Alexander Block
- Allan Doensen
- Bob Feldbauer
- chaeplin
- crowning-
- diego-ab
- Gavin Westwood
- gladcow
- Holger Schinzel
- Ilya Savinov
- Kamuela Franco
- krychlicki
- Nathan Marley
- Oleg Girko
- QuantumExplorer
- sorin-postelnicu
- Spencer Lievens
- taw00
- TheLazieR Yip
- thephez
- Tim Flynn
- UdjinM6
- Will Wray

As well as Bitcoin Core Developers and everyone that submitted issues or helped translating on [Transifex](https://www.transifex.com/projects/p/dash/).


Older releases
==============

Dash was previously known as Darkcoin.

Darkcoin tree 0.8.x was a fork of Litecoin tree 0.8, original name was XCoin
which was first released on Jan/18/2014.

Darkcoin tree 0.9.x was the open source implementation of masternodes based on
the 0.8.x tree and was first released on Mar/13/2014.

Darkcoin tree 0.10.x used to be the closed source implementation of Darksend
which was released open source on Sep/25/2014.

Dash Core tree 0.11.x was a fork of Bitcoin Core tree 0.9, Darkcoin was rebranded
to Dash.

Dash Core tree 0.12.0.x was a fork of Bitcoin Core tree 0.10.

Dash Core tree 0.12.1.x was a fork of Bitcoin Core tree 0.12.

These release are considered obsolete. Old changelogs can be found here:

- [v0.12.1](release-notes/dash/release-notes-0.12.1.md) released ???/??/2016
- [v0.12.0](release-notes/dash/release-notes-0.12.0.md) released ???/??/2015
- [v0.11.2](release-notes/dash/release-notes-0.11.2.md) released Mar/25/2015
- [v0.11.1](release-notes/dash/release-notes-0.11.1.md) released Feb/10/2015
- [v0.11.0](release-notes/dash/release-notes-0.11.0.md) released Jan/15/2015
- [v0.10.x](release-notes/dash/release-notes-0.10.0.md) released Sep/25/2014
- [v0.9.x](release-notes/dash/release-notes-0.9.0.md) released Mar/13/2014

