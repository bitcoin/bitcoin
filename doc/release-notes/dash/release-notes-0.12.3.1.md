Dash Core version 0.12.3.1
==========================

Release is now available from:

  <https://www.dash.org/downloads/#wallets>

This is a new major version release, bringing new features, various bugfixes and other
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
dashd/dash-qt (on Linux).

Downgrade warning
-----------------

### Downgrade to a version < 0.12.2.2

Because release 0.12.2.2 included the [per-UTXO fix](release-notes/dash/release-notes-0.12.2.2.md#per-utxo-fix)
which changed the structure of the internal database, you will have to reindex
the database if you decide to use any pre-0.12.2.2 version.

Wallet forward or backward compatibility was not affected.

### Downgrade to 0.12.2.2/3

Downgrading to these versions does not require any additional actions, should be
fully compatible.

Notable changes
===============

Introducing Named Devnets
-------------------------

We introduce a new feature called [Named Devnets](https://github.com/dashpay/dash/pull/1791).
This feature allows the creation of multiple independent devnets. Each one is
identified by a name which is hardened into a "devnet genesis" block,
which is automatically positioned at height 1. Validation rules will
ensure that a node from `devnet=test1` will never be able to accept blocks
from `devnet=test2`. This is done by checking the expected devnet genesis
block.

The genesis block of the devnet is the same as the one from regtest. This
starts the devnet with a very low difficulty, allowing us to fill up
needed balances for masternodes very fast.

Also, the devnet name is put into the sub-version of the `VERSION` message.
If a node connects to the wrong network, it will immediately be disconnected.

New format of network message signatures
----------------------------------------

We introduced a new signature format for Dash-specific network messages,
read more [here](https://github.com/dashpay/dash/pull/1936) and [here](https://github.com/dashpay/dash/pull/1937).
We also introduced a new spork `SPORK_6_NEW_SIGS` which is going to be used to activate the new format after the network has finished the upgrade.
Note that old pre-12.3 nodes won't be able to recognize and verify new signatures after `SPORK_6_NEW_SIGS` activates.

The old format is partly kept in the code to keep backwards compatibility. This code will be removed in an upcoming
release.

Governance system improvements
------------------------------

We do not use watchdogs since 12.2.x, instead we include all required information about sentinel into masternode
pings. With this update we add some additional information and cover everything with a signature to ensure that
masternode ping wasn't maleated by some intermediary node. All messages and logic related to watchdogs are
completely removed now. We also improved proposal message format, as well as proposal validation and processing,
which should lower network traffic and CPU usage. Handling of triggers was also improved slightly.

`SPORK_13_OLD_SUPERBLOCK_FLAG` was removed now as it was unused since some time.

PrivateSend improvements
------------------------

PrivateSend collaterals are no longer required to be N times of the PrivateSend fee (PSF), instead any input
which is greater or equal 1 PSF but less or equal 4 PSF can be used as a collateral. Inputs that are greater or
equal 1 PSF but strictly less than 2 PSF will be used in collaterals with OP_RETURN outputs. Note that such
inputs will be consumed completely, with no change outputs at all. This should lower number of inputs wallet
would need to take care of, improve privacy by eliminating the case where user accidentally merge small
non-private inputs together and also decrease global UTXO set size.

It might feel that thanks to this change mixing fees are going to be slightly higher on average if have lots of
such small inputs. However, you need to keep in mind that creating new PrivateSend collaterals cost some fee too
and since such small inputs were not used at all you'd need more txes to create such collaterals. So in general,
we believe average mixing fees should stay mostly the same.

There are also some other minor fixes which should also slightly improve mixing process.

Additional indexes cover P2PK now
---------------------------------

Additional indexes like `addressindex` etc. process P2PK outputs correctly now. Note, that these indexes will
not be re-built automatically on wallet update, you must reindex manually to update indexes with P2PK outputs.

Support for pruned nodes in Lite Mode
-------------------------------------

It is now possible to run a pruned node which stores only some recent blocks and not the whole blockchain.
However this option is only available in so called Lite Mode. In this mode, Dash specific features are disabled, meaning
that such nodes won't fully validate the blockchain (masternode payments and superblocks).
PrivateSend and InstantSend functions are also disabled on such nodes. Such nodes are comparable to SPV-like nodes
in terms of security and validation - it relies a lot on surrounding nodes, so please keep this in mind if you decide to
use it for something.

Default maximum block size
--------------------------

We've changed the default maximum block size to 2MB. Such blocks were already allowed before, but the default setting
for the maximum block size (which only affects miners) was kept in place until this version.

RPC changes
-----------

There are a few changes in existing RPC interfaces in this release:
- `gobject count`, `masternode count` and `masternode list` will now by default return JSON formatted output;
If you rely on the old output format, you can still specify an additional parameter for backwards compatibility (`all` for `count` and `status` for `list`);
- `masternodelist` has a few new modes: `daemon`, `json`, `sentinel`;
- `debug` rpc now requires categories to be separated via `+`, not `,` like before (e.g. `dash+net`);
- `getchaintips` now shows the block fork occurred in `forkpoint` field;
- `getrawmempool`'s has InstantSend-related info (`instantsend` and `instantlock`);
- `getgovernanceinfo` has new field `sentinelpingmaxseconds`;
- `getwalletinfo` now shows PrivateSend balance in `privatesend_balance` field;
- `sendrawtransaction` no longer bypasses transaction policy limits by default.
- `dumphdinfo` should throw an error when wallet isn't HD now

There is also a new RPC command `listaddressbalances`.

You can read about RPC changes brought by backporting from Bitcoin Core in following docs:
- https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.13.0.md#low-level-rpc-changes
- https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.13.1.md#low-level-rpc-changes
- https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.14.0.md#low-level-rpc-changes

Command-line options
--------------------

New cmd-line options:
- introduced in Dash Core 0.12.3.1: `allowprivatenet`, `bip9params`, `sporkaddr`, `devnet`;
- backported from Bitcoin Core 0.13/0.14: `blockreconstructionextratxn`, `maxtimeadjustment`, `maxtipage`,
`incrementalrelayfee`, `dustrelayfee`, `blockmintxfee`.

See `Help -> Command-line options` in Qt wallet or `dashd --help` for more info.

New Masternode Information Dialog
---------------------------------

You can now double-click on your masternode in `My Masternodes` list on `Masternodes` tab to reveal the new
Masternode Information dialog. It will show you some basic information as well as software versions reported by the
masternode. There is also a QR code now which encodes corresponding masternode private key (the one you set with
mnprivkey during MN setup and NOT the one that controls the 1000 DASH collateral) which should make the process of pairing with
mobile software allowing you to vote with your masternode a bit easier (this software is still in development).

Testnet fixes
-------------

While we've been in release preparation, a miner used his ASICs on testnet. This resulted in too many blocks being mined
in a too short time. It revealed a few corner-case bugs in validation and synchronisation rules which we have fixed now.
We've also backported a special testnet rule for our difficulty adjustment algorithm that allows to mine a low difficulty
block on testnet when the last block is older than 5 minutes. This and the other fixes should stabilize our testnet in
case of future ASIC uses on testnet.

Using masternode lists for initial peers discovery
--------------------------------------------------

We now use a recent masternode list to feed the hardcoded seed nodes list in Dash Core. This list was previously
unmaintained as we fully relied on DNS based discovery on startup. DNS discovery is still used as the main discovery
method, but the hardcoded seed list should now be able to serve as a proper backup in case DNS fails for some reason.

Lots of backports, refactoring and bug fixes
--------------------------------------------

We backported many performance improvements and refactoring from Bitcoin Core and aligned most of our codebase with version 0.14.
Most notable ones besides various performance and stability improvements probably are
[Compact Block support (BIP 152)](https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.13.0.md#compact-block-support-bip-152),
[Mining transaction selection ("Child Pays For Parent")](https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.13.0.md#mining-transaction-selection-child-pays-for-parent),
[Null dummy soft fork (BIP 147, without SegWit)](https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.13.1.md#null-dummy-soft-fork),
[Nested RPC Commands in Debug Console](https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.14.0.md#nested-rpc-commands-in-debug-console) and
[Support for JSON-RPC Named Arguments](https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.14.0.md#support-for-json-rpc-named-arguments).

You can read more about all changes in Bitcoin Core 0.13 and 0.14 in following documents:
- [release-notes-0.13.0.md](https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.13.0.md);
- [release-notes-0.13.1.md](https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.13.1.md);
- [release-notes-0.13.2.md](https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.13.2.md);
- [release-notes-0.14.0.md](https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.14.0.md);
- [release-notes-0.14.1.md](https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.14.1.md);
- [release-notes-0.14.2.md](https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.14.2.md).

Note that some features were already backported earlier (per-UTXO fix, -assumevalid, GUI overlay etc.) and some were not backported at all
(SegWit and feefilter, you can read more about why we did so [here](https://blog.dash.org/segwit-lighting-rbf-in-dash-9536868ca861) and [here](https://github.com/dashpay/dash/pull/2025)).
The alert system was also kept in place for now. We are going to continue backporting the most notable fixes and improvements from Bitcoin Core versions 0.15 and 0.16 in future releases.

A lot of refactoring, code cleanups and other small fixes were done in this release again. We are going to continue making code more reliable and easier to review in future releases as well.


0.12.3.1 Change log
===================

See detailed [change log](https://github.com/dashpay/dash/compare/v0.12.2.3...dashpay:v0.12.3.1) below.

### Governance:
- [`6c79c348e`](https://github.com/dashpay/dash/commit/6c79c348e) Drop "MAY, 2018" clause for proposal validation on mainnet (#2101)
- [`6079b860e`](https://github.com/dashpay/dash/commit/6079b860e) Drop trigger objects when triggers are deleted or failed to be created (#2098)
- [`2583e1963`](https://github.com/dashpay/dash/commit/2583e1963) Test: Add few valid/invalid proposals (internationalization) (#2044)
- [`25eb6d7b3`](https://github.com/dashpay/dash/commit/25eb6d7b3) clean up governance vote code (#2042)
- [`a0874b72a`](https://github.com/dashpay/dash/commit/a0874b72a) Validate data size for proposals only (#2004)
- [`15fac7c7e`](https://github.com/dashpay/dash/commit/15fac7c7e) Validate proposals for expiration (#2003)
- [`a3bcc4307`](https://github.com/dashpay/dash/commit/a3bcc4307) Refactor governance (#1993)
- [`04a23bf0c`](https://github.com/dashpay/dash/commit/04a23bf0c) Fix the bug in CGovernanceObject::ProcessVote() (#1989)
- [`b0868093b`](https://github.com/dashpay/dash/commit/b0868093b) simplify gobject JSON format, remove unused fields (#1902)
- [`1dda9fe6f`](https://github.com/dashpay/dash/commit/1dda9fe6f) CProposalValidator refactoring and follow up fixes (#1956)
- [`89380b4c9`](https://github.com/dashpay/dash/commit/89380b4c9) Drop watchdogs, replace them with sentinel pings (#1949)
- [`e71cb3861`](https://github.com/dashpay/dash/commit/e71cb3861) Store CGovernanceVote hash in memory instead of recalculating it via GetHash() every time (#1946)
- [`580c4884c`](https://github.com/dashpay/dash/commit/580c4884c) Fix trigger execution and expiration (#1920)
- [`0670695fe`](https://github.com/dashpay/dash/commit/0670695fe) Move prev/next sb height logic from rpc to CSuperblock::GetNearestSuperblocksHeights (#1919)
- [`741fcbc90`](https://github.com/dashpay/dash/commit/741fcbc90) Remove excessive custom validation in CProposalValidator::ValidatePaymentAddress (#1917)
- [`354aac8d1`](https://github.com/dashpay/dash/commit/354aac8d1) rename nEpochStart variable and adjust comments (#1915)
- [`8ea1bd0f5`](https://github.com/dashpay/dash/commit/8ea1bd0f5) remove unused method GetObjectSubtype (#1914)
- [`8a387ee09`](https://github.com/dashpay/dash/commit/8a387ee09) Drop SPORK_13_OLD_SUPERBLOCK_FLAG and check superblock start hash (#1872)

### InstantSend:
- [`8c2d16f5f`](https://github.com/dashpay/dash/commit/8c2d16f5f) Limit IS quorums by updated MNs only (#2107)
- [`ef85d5144`](https://github.com/dashpay/dash/commit/ef85d5144) Comment updates - InstantSend (#2062)
- [`c0a109998`](https://github.com/dashpay/dash/commit/c0a109998) Fix instantsend in testnet and regtest (#2016)
- [`2f1661678`](https://github.com/dashpay/dash/commit/2f1661678) Locked txes should not expire until mined and have sufficient confirmations (#2011)
- [`846f1d217`](https://github.com/dashpay/dash/commit/846f1d217) Avoid processing tx lock request twice in the wallet it was sent from (#2007)
- [`c0c998da3`](https://github.com/dashpay/dash/commit/c0c998da3) Fix CInstantSend::GetTxLockRequest() (#2006)
- [`7d5223b5e`](https://github.com/dashpay/dash/commit/7d5223b5e) Network-specific thresholds for required confirmations (IS) (#1962)
- [`2c04504f1`](https://github.com/dashpay/dash/commit/2c04504f1) Refactor IS votes processing (#1951)

### PrivateSend:
- [`0d5426343`](https://github.com/dashpay/dash/commit/0d5426343) Fix an edge case in PrepareDenominate (#2138)
- [`8e129877a`](https://github.com/dashpay/dash/commit/8e129877a) Partially revert 1922 (#2108)
- [`fcac40ab4`](https://github.com/dashpay/dash/commit/fcac40ab4) RPC: fix wallet lock check in `privatesend start` (#2102)
- [`dbbedc031`](https://github.com/dashpay/dash/commit/dbbedc031) Fix JoinExistingQueue bug (#2100)
- [`7ac4b972a`](https://github.com/dashpay/dash/commit/7ac4b972a) Require all participants to submit equal number of inputs (#2075)
- [`d1bf615f3`](https://github.com/dashpay/dash/commit/d1bf615f3) No POOL_STATE_ERROR or POOL_STATE_SUCCESS on masternodes (#2009)
- [`d03adb7c3`](https://github.com/dashpay/dash/commit/d03adb7c3) Check if in masternode mode first and only then do the job (or not) (#2008)
- [`ddff32b96`](https://github.com/dashpay/dash/commit/ddff32b96) Fix TransactionRecord::PrivateSendMakeCollaterals tx type (#1996)
- [`4f978a263`](https://github.com/dashpay/dash/commit/4f978a263) Drop Nx requirements for PS collaterals (#1995)
- [`a44f48743`](https://github.com/dashpay/dash/commit/a44f48743) Allow data outputs in PS collaterals (#1984)
- [`ef9a9f2d6`](https://github.com/dashpay/dash/commit/ef9a9f2d6) Fix unlocking error on "Start Mixing" (#1941)
- [`0bd8c8e43`](https://github.com/dashpay/dash/commit/0bd8c8e43) Refactor: vecTxIn -> vecOutPoints for CompactTallyItem (#1932)
- [`d7f55d508`](https://github.com/dashpay/dash/commit/d7f55d508) Switch nTimeLastSuccessfulStep from GetTimeMillis() to GetTime() (#1923)
- [`204b1fe99`](https://github.com/dashpay/dash/commit/204b1fe99) Drop unnecessary AcceptToMemoryPool in PS (and corresponding cs-main locks), just relay what we have (#1922)
- [`271c249e1`](https://github.com/dashpay/dash/commit/271c249e1) Skip next mn payments winners when selecting a MN to mix on (#1921)
- [`ca89c7b87`](https://github.com/dashpay/dash/commit/ca89c7b87) [Trivial] Update PrivateSend denominations in comments / typo fixes (#1910)
- [`b1817dd93`](https://github.com/dashpay/dash/commit/b1817dd93) Introduce CDarksendAccept class (for DSACCEPT messages) (#1875)
- [`d69ad9d61`](https://github.com/dashpay/dash/commit/d69ad9d61) Skip existing masternode conections on mixing (#1833)
- [`1d620d1f9`](https://github.com/dashpay/dash/commit/1d620d1f9) Fix calls to AcceptToMemoryPool in PS submodules (#1823)

### Network:
- [`fda74b4a8`](https://github.com/dashpay/dash/commit/fda74b4a8) Use correct protocol when serializing messages in reply to `getdata` (#2157)
- [`6bf389afb`](https://github.com/dashpay/dash/commit/6bf389afb) Don't drop mnb-s for outdated MNs (#2131)
- [`c60079b59`](https://github.com/dashpay/dash/commit/c60079b59) ThreadOpenMasternodeConnections should process only one mn at a time (#2080)
- [`a648d6eff`](https://github.com/dashpay/dash/commit/a648d6eff) Drop delayed headers logic and fix duplicate initial headers sync by handling block inv correctly (#2032)
- [`99085c5b6`](https://github.com/dashpay/dash/commit/99085c5b6) swap devnet magic bytes around (#2028)
- [`a37dbd6d2`](https://github.com/dashpay/dash/commit/a37dbd6d2) Fix netfulfilledman usage (#2033)
- [`08033ffe4`](https://github.com/dashpay/dash/commit/08033ffe4) Reject Dash-specific messages from obsolete peers (#1983)
- [`43671a39d`](https://github.com/dashpay/dash/commit/43671a39d) Deprecate nMnCount in mnget (#1942)
- [`451f7f071`](https://github.com/dashpay/dash/commit/451f7f071) Fix issues with mnp, mnw and dsq signatures via new spork (SPORK_6_NEW_SIGS) (#1936)
- [`048062641`](https://github.com/dashpay/dash/commit/048062641) Force masternodes to have listen=1 and maxconnections to be at least DEFAULT_MAX_PEER_CONNECTIONS (#1935)
- [`aadec3735`](https://github.com/dashpay/dash/commit/aadec3735) Change format of gobject, store/transmit vchData instead of hex-encoded string of a string (#1934)
- [`ed712eb81`](https://github.com/dashpay/dash/commit/ed712eb81) Fix nDelayGetHeadersTime (int64_t max == never delay) (#1916)
- [`f35b5979a`](https://github.com/dashpay/dash/commit/f35b5979a) Refactor CGovernanceManager::Sync (split in two) (#1930)
- [`b5046d59c`](https://github.com/dashpay/dash/commit/b5046d59c) Dseg fixes (#1929)
- [`312088b56`](https://github.com/dashpay/dash/commit/312088b56) Fix connectivity check in CActiveMasternode::ManageStateInitial (#1918)
- [`8f2c1998d`](https://github.com/dashpay/dash/commit/8f2c1998d) Rename vBlockHashesFromINV to vDelayedGetHeaders (#1909)
- [`4719ec477`](https://github.com/dashpay/dash/commit/4719ec477) Remove some locking in net.h/net.cpp (#1905)
- [`a6ba82ac9`](https://github.com/dashpay/dash/commit/a6ba82ac9) Use masternode list to generate hardcoded seeds (#1892)
- [`1b1a440f4`](https://github.com/dashpay/dash/commit/1b1a440f4) Do not send dash-specific requests to masternodes before we are fully connected (#1882)
- [`1ca270ed8`](https://github.com/dashpay/dash/commit/1ca270ed8) No need for msgMakerInitProto for sporks because we loop by fully connected nodes only now (#1877)
- [`b84afb251`](https://github.com/dashpay/dash/commit/b84afb251) Allow to filter for fully connected nodes when calling CopyNodeVector (#1864)
- [`532b9fa3d`](https://github.com/dashpay/dash/commit/532b9fa3d) Use OpenNetworkConnection instead of calling ConnectNode directly in Dash code (#1857)
- [`3aad9d908`](https://github.com/dashpay/dash/commit/3aad9d908) Fix logging in PushInventory (#1847)
- [`81fb931fb`](https://github.com/dashpay/dash/commit/81fb931fb) Don't delay GETHEADERS when no blocks have arrived yet in devnet (#1807)

### Mining:
- [`ff93dd613`](https://github.com/dashpay/dash/commit/ff93dd613) Check devnet genesis block (#2057)
- [`1dbf5a0f6`](https://github.com/dashpay/dash/commit/1dbf5a0f6) Fix transaction/block versions for devnet genesis blocks (#2056)
- [`880cbf43b`](https://github.com/dashpay/dash/commit/880cbf43b) Backport fPowAllowMinDifficultyBlocks rule to DarkGravityWave (#2027)
- [`27dfed424`](https://github.com/dashpay/dash/commit/27dfed424) Bump default max block size from 750k to 2MB (#2023)
- [`79183f630`](https://github.com/dashpay/dash/commit/79183f630) Add tests for GetBlockSubsidy algorithm (#2022)

### Wallet:
- [`0a71c693e`](https://github.com/dashpay/dash/commit/0a71c693e) Remove explicit wallet lock in MasternodeList::StartAll() (#2106)
- [`0de79d70b`](https://github.com/dashpay/dash/commit/0de79d70b) Do not create oversized transactions (bad-txns-oversize) (#2103)
- [`0260821f8`](https://github.com/dashpay/dash/commit/0260821f8) fix SelectCoinsByDenominations (#2074)
- [`b7bd96e2b`](https://github.com/dashpay/dash/commit/b7bd96e2b) Clarify the warning displayed when encrypting HD wallet (#2002)
- [`4930bb9f5`](https://github.com/dashpay/dash/commit/4930bb9f5) Don't hold cs_storage in CKeyHolderStorage while calling functions which might lock cs_wallet (#2000)
- [`4d442376e`](https://github.com/dashpay/dash/commit/4d442376e) Limit the scope of cs_wallet lock in CPrivateSendClient::PrepareDenominate() (#1997)
- [`1d32d1c32`](https://github.com/dashpay/dash/commit/1d32d1c32) Add missing includes required for compilation with --disable-wallet flag (#1991)
- [`3f0c8723e`](https://github.com/dashpay/dash/commit/3f0c8723e) Slightly refactor AutoBackupWallet (#1927)
- [`9965d51bb`](https://github.com/dashpay/dash/commit/9965d51bb) Avoid reference leakage in CKeyHolderStorage::AddKey (#1840)
- [`c532be1c0`](https://github.com/dashpay/dash/commit/c532be1c0) Protect CKeyHolderStorage via mutex (#1834)
- [`144850657`](https://github.com/dashpay/dash/commit/144850657) Switch KeePassHttp integration to new AES lib, add tests and a note about KeePassHttp security (#1818)
- [`fa2549986`](https://github.com/dashpay/dash/commit/fa2549986) Swap iterations and fUseInstantSend parameters in ApproximateBestSubset (#1819)

### RPC:
- [`700b7ceb7`](https://github.com/dashpay/dash/commit/700b7ceb7) RPC: dumphdinfo should throw an error when wallet isn't HD (#2134)
- [`5669fc880`](https://github.com/dashpay/dash/commit/5669fc880) Fix typos and rpc help text (#2120)
- [`806d7f049`](https://github.com/dashpay/dash/commit/806d7f049) Fix rpc tests broken by 2110 (#2118)
- [`8d8fdb433`](https://github.com/dashpay/dash/commit/8d8fdb433) sendrawtransaction no longer bypasses transaction policy limits by default (#2110)
- [`6ab1fd763`](https://github.com/dashpay/dash/commit/6ab1fd763) RPC: Add description for InstantSend-related fields of mempool entry (#2050)
- [`138441eb8`](https://github.com/dashpay/dash/commit/138441eb8) Add `forkpoint` to `getchaintips` (#2039)
- [`9b17f2b9c`](https://github.com/dashpay/dash/commit/9b17f2b9c) Convert `gobject count` output to json (by default) (#1994)
- [`4b128b1b9`](https://github.com/dashpay/dash/commit/4b128b1b9) Fix listaddressbalances (#1987)
- [`d115efacb`](https://github.com/dashpay/dash/commit/d115efacb) [RPC] Few additions to masternodelist (#1971)
- [`9451782a0`](https://github.com/dashpay/dash/commit/9451782a0) RPC: Add `listaddressbalances` (#1972)
- [`bab543f3e`](https://github.com/dashpay/dash/commit/bab543f3e) Various RPC fixes (#1958)
- [`151152b98`](https://github.com/dashpay/dash/commit/151152b98) rpc - Update getaddednodeinfo help to remove dummy argument (#1947)
- [`3c44dde2e`](https://github.com/dashpay/dash/commit/3c44dde2e) Return JSON object for masternode count (by default but still support old modes for now) (#1900)
- [`4bc4a7dac`](https://github.com/dashpay/dash/commit/4bc4a7dac) Fix `debug` rpc (#1897)
- [`063bc5542`](https://github.com/dashpay/dash/commit/063bc5542) Fix `masternode list` (#1893)
- [`5a5f61872`](https://github.com/dashpay/dash/commit/5a5f61872) Shorten MN outpoint output from getvotes (#1871)
- [`86d33b276`](https://github.com/dashpay/dash/commit/86d33b276) Remove double registration of "privatesend" RPC (#1853)
- [`c2de362b9`](https://github.com/dashpay/dash/commit/c2de362b9) Actually honor fMiningRequiresPeers in getblocktemplate (#1844)
- [`1cffb8a7e`](https://github.com/dashpay/dash/commit/1cffb8a7e) Include p2pk into addressindex (#1839)

### GUI:
- [`7ab5b4a28`](https://github.com/dashpay/dash/commit/7ab5b4a28) Update/optimize images (#2147)
- [`82805a6c6`](https://github.com/dashpay/dash/commit/82805a6c6) swap out old logo for T&C logo in Qt GUI (#2081)
- [`e9f63073d`](https://github.com/dashpay/dash/commit/e9f63073d) Warn when more than 50% of masternodes are using newer version (#1963)
- [`653600352`](https://github.com/dashpay/dash/commit/653600352) Draw text on top of everything else in TrafficGraphWidget (#1944)
- [`118eeded6`](https://github.com/dashpay/dash/commit/118eeded6) [GUI] Create QR-code for Masternode private key (#1970)
- [`9f2467af8`](https://github.com/dashpay/dash/commit/9f2467af8) Hide autocompleter on Enter/Return key (#1898)
- [`e30009c31`](https://github.com/dashpay/dash/commit/e30009c31) Fix qt and fontconfig depends #1884

### Docs:
- [`a80ef0423`](https://github.com/dashpay/dash/commit/a80ef0423) Update release notes (#2155)
- [`5e1149a65`](https://github.com/dashpay/dash/commit/5e1149a65) Update release notes (#2142)
- [`d46dc0f56`](https://github.com/dashpay/dash/commit/d46dc0f56) Update release notes (#2135)
- [`d076ad4ce`](https://github.com/dashpay/dash/commit/d076ad4ce) Update release notes and staging tree in README (#2116)
- [`ca2eae6e6`](https://github.com/dashpay/dash/commit/ca2eae6e6) 12.3 release notes draft (#2045)
- [`faeb4480a`](https://github.com/dashpay/dash/commit/faeb4480a) Update manpages with ./contrib/devtools/gen-manpages.sh (#2088)
- [`4148b8200`](https://github.com/dashpay/dash/commit/4148b8200) Release notes cleanup (#2034)
- [`d2c46a6a3`](https://github.com/dashpay/dash/commit/d2c46a6a3) Update protocol-documentation.md (#1964)
- [`4db8483d4`](https://github.com/dashpay/dash/commit/4db8483d4) [Docs] Doxyfile Project version update (#1938)
- [`6e022c57b`](https://github.com/dashpay/dash/commit/6e022c57b) Remove src/drafted folder (#1907)
- [`0318c76ba`](https://github.com/dashpay/dash/commit/0318c76ba) Update links and references to current communication channels (#1906)
- [`e23861c0e`](https://github.com/dashpay/dash/commit/e23861c0e) [Trivial] RPC Typos / markdown formatting (#1830)
- [`3dc62106b`](https://github.com/dashpay/dash/commit/3dc62106b) [Docs] Doxygen config update (#1796)

### Other fixes and improvements:
- [`4dbde218b`](https://github.com/dashpay/dash/commit/4dbde218b) Fix p2pkh tests asserts (#2153)
- [`26c891f67`](https://github.com/dashpay/dash/commit/26c891f67) Fix block value/payee validation in lite mode (#2148)
- [`9af9d57b4`](https://github.com/dashpay/dash/commit/9af9d57b4) Release 0.12.3 (#2145)
- [`8e6364694`](https://github.com/dashpay/dash/commit/8e6364694) Bump SERIALIZATION_VERSION_STRINGs (#2136)
- [`641070521`](https://github.com/dashpay/dash/commit/641070521) Fix 2 small issues in sporks module (#2133)
- [`97b9b4fed`](https://github.com/dashpay/dash/commit/97b9b4fed) Bump nMinimumChainWork, defaultAssumeValid and checkpoints (#2130)
- [`1c9917e22`](https://github.com/dashpay/dash/commit/1c9917e22) Fix CVE-2018-12356 by hardening the regex (#2126)
- [`b7c326115`](https://github.com/dashpay/dash/commit/b7c326115) Do not create mnb until sync is finished (#2122)
- [`b98643c27`](https://github.com/dashpay/dash/commit/b98643c27) Split sentinel expiration in CMasternode::Check() in two parts (timeout and version) (#2121)
- [`836e10471`](https://github.com/dashpay/dash/commit/836e10471) Bump proto to 70210 (#2109)
- [`23ba94b37`](https://github.com/dashpay/dash/commit/23ba94b37) Bump remaining min protocols (#2097)
- [`9299a84b1`](https://github.com/dashpay/dash/commit/9299a84b1) Bump few consts (#2096)
- [`7b43720f0`](https://github.com/dashpay/dash/commit/7b43720f0) Fix copying of final binaries into dashcore-binaries (#2090)
- [`cc593615e`](https://github.com/dashpay/dash/commit/cc593615e) Bump copyright year to 2018 (#2087)
- [`2129ee4d8`](https://github.com/dashpay/dash/commit/2129ee4d8) Add docker support when doing Gitian builds (#2084)
- [`6a1456ef4`](https://github.com/dashpay/dash/commit/6a1456ef4) Update gitian key for codablock (#2085)
- [`ab96a6af6`](https://github.com/dashpay/dash/commit/ab96a6af6) Update gitian keys, script and doc (#2082)
- [`5d057cf66`](https://github.com/dashpay/dash/commit/5d057cf66) Add string_cast benchmark (#2073)
- [`cf71f5767`](https://github.com/dashpay/dash/commit/cf71f5767) Fix potential DoS vector for masternode payments (#2071)
- [`febdc2116`](https://github.com/dashpay/dash/commit/febdc2116) Fix `nl` locale alias (#2061)
- [`1264a5577`](https://github.com/dashpay/dash/commit/1264a5577) Fix spork signature check for new nodes after SPORK_6_NEW_SIGS is switched to ON (#2060)
- [`41680f4d9`](https://github.com/dashpay/dash/commit/41680f4d9) small cleanup in a few places (#2058)
- [`741b94875`](https://github.com/dashpay/dash/commit/741b94875) Translations201804 (#2012)
- [`8e24b087b`](https://github.com/dashpay/dash/commit/8e24b087b) replace boost iterators in dash-specific code (#2048)
- [`7719b7ec2`](https://github.com/dashpay/dash/commit/7719b7ec2) Update BIP147 deployment times, nMinimumChainWork and defaultAssumeValid (#2030)
- [`b07503f01`](https://github.com/dashpay/dash/commit/b07503f01) Some cleanup (mostly trivial) (#2038)
- [`f8e5c5d56`](https://github.com/dashpay/dash/commit/f8e5c5d56) Simplify spork defaults by using a map (#2037)
- [`6dd8304a5`](https://github.com/dashpay/dash/commit/6dd8304a5) Remove duplication of "class CBlockIndex;" (#2036)
- [`4ea790377`](https://github.com/dashpay/dash/commit/4ea790377) Dashify lib names (#2035)
- [`53093c65b`](https://github.com/dashpay/dash/commit/53093c65b) Run tests in mocked time (#2031)
- [`f7b9aae27`](https://github.com/dashpay/dash/commit/f7b9aae27) Correctly update pindexBestHeader and pindexBestInvalid in InvalidateBlock (#2029)
- [`8b09e779b`](https://github.com/dashpay/dash/commit/8b09e779b) Bump testnet checkpoint and nMinimumChainWork/defaultAssumeValid params (#2026)
- [`eecc69223`](https://github.com/dashpay/dash/commit/eecc69223) Fix a very ancient bug from mid 2015 (#2021)
- [`72a225b9b`](https://github.com/dashpay/dash/commit/72a225b9b) Few fixes for lite mode (#2014)
- [`c7e9ea9fb`](https://github.com/dashpay/dash/commit/c7e9ea9fb) Avoid repeating the full scan in CMasternodeMan::UpdateLastPaid() on non-MNs (#1985)
- [`f28a58e0a`](https://github.com/dashpay/dash/commit/f28a58e0a) Refactor and fix restart (#1999)
- [`7248700b3`](https://github.com/dashpay/dash/commit/7248700b3) Add missing cs_main locks (#1998)
- [`9e98c856f`](https://github.com/dashpay/dash/commit/9e98c856f) A pack of small fixes (#1992)
- [`19ea1a791`](https://github.com/dashpay/dash/commit/19ea1a791) Use operator[] instead of emplace in CMasternodePayments::AddPaymentVote (#1980)
- [`ca3655f49`](https://github.com/dashpay/dash/commit/ca3655f49) Fix some (potential dead)locks (#1977)
- [`2a7e6861d`](https://github.com/dashpay/dash/commit/2a7e6861d) Include "clientversion.h" in rpc/masternode.cpp (#1979)
- [`ef1a86c3e`](https://github.com/dashpay/dash/commit/ef1a86c3e) Add dummy CMakeLists.txt file to make development with CLion easier (#1978)
- [`a9d8e2c5d`](https://github.com/dashpay/dash/commit/a9d8e2c5d) [Init] Avoid segfault when called with -enableinstantsend=0 (#1976)
- [`3200eae9b`](https://github.com/dashpay/dash/commit/3200eae9b) Don't use short version of 'tinyformat/fmt' namespace in util.h (#1975)
- [`97a07cbc4`](https://github.com/dashpay/dash/commit/97a07cbc4) Refactor `CMasternodePayment*` (#1974)
- [`4ffa7bac0`](https://github.com/dashpay/dash/commit/4ffa7bac0) Introduce DIP0001Height (#1973)
- [`611879aa6`](https://github.com/dashpay/dash/commit/611879aa6) Use spork addresses instead of raw keys and allow changing them on startup (#1969)
- [`9ef38c6d7`](https://github.com/dashpay/dash/commit/9ef38c6d7) Switch CNetFulfilledRequestManager and CMasternodeMan maps/funcs to CService (#1967)
- [`929c1584a`](https://github.com/dashpay/dash/commit/929c1584a) Rename CheckPreviousBlockVotes to CheckBlockVotes and adjust its log output a bit (#1965)
- [`bf0854e58`](https://github.com/dashpay/dash/commit/bf0854e58) Swap `expired` and `sentinel_expired` states in order (#1961)
- [`9876207ce`](https://github.com/dashpay/dash/commit/9876207ce) Multiple devnet fixes (#1960)
- [`e37b6c7da`](https://github.com/dashpay/dash/commit/e37b6c7da) Fix BIP147 deployment threshold parameter (#1955)
- [`106276a3e`](https://github.com/dashpay/dash/commit/106276a3e) Adjust/fix log output (#1954)
- [`0abd1894e`](https://github.com/dashpay/dash/commit/0abd1894e) Call CheckMnbAndUpdateMasternodeList when starting MN (#1945)
- [`e23f61822`](https://github.com/dashpay/dash/commit/e23f61822) Make TrafficGraphDataTests more general (#1943)
- [`5b1c4d8a1`](https://github.com/dashpay/dash/commit/5b1c4d8a1) Few (mostly trivial) cleanups and fixes (#1940)
- [`99273f63a`](https://github.com/dashpay/dash/commit/99273f63a) Use SPORK_6_NEW_SIGS to switch from signing string messages to hashes (#1937)
- [`c65613350`](https://github.com/dashpay/dash/commit/c65613350) Switch masternode id in Dash data structures from CTxIn to COutPoint (#1933)
- [`2ea6f7d82`](https://github.com/dashpay/dash/commit/2ea6f7d82) Use `override` keyword for overriden class member functions (#1644)
- [`d5ef77ba9`](https://github.com/dashpay/dash/commit/d5ef77ba9) Refactor: use constant refs and `Ret` suffix (#1928)
- [`2e04864b2`](https://github.com/dashpay/dash/commit/2e04864b2) Replace boost::lexical_cast<int> with atoi (#1926)
- [`0f4d963ba`](https://github.com/dashpay/dash/commit/0f4d963ba) Add DSHA256 and X11 benchmarks, refactor names of other algo benchmarks to group them together (#1925)
- [`4528c735f`](https://github.com/dashpay/dash/commit/4528c735f) Replace some instantsend/privatesend magic numbers with constants (#1924)
- [`120893c63`](https://github.com/dashpay/dash/commit/120893c63) Update timeLastMempoolReq when responding to MEMPOOL request (#1904)
- [`bb20b4e7b`](https://github.com/dashpay/dash/commit/bb20b4e7b) Few cleanups after backporting (#1903)
- [`a7fa07a30`](https://github.com/dashpay/dash/commit/a7fa07a30) Drop BOOST_FOREACH and use references in loops (const ref where applicable, Dash code only) (#1899)
- [`e0b6988a4`](https://github.com/dashpay/dash/commit/e0b6988a4) Various fixes and refactoring for Cache*Map classes (#1896)
- [`99b2789a7`](https://github.com/dashpay/dash/commit/99b2789a7) Fix DeserializeAndCheckBlockTest benchmark and store hashDevnetGenesisBlock in `consensus` (#1888)
- [`88646bd0d`](https://github.com/dashpay/dash/commit/88646bd0d) Rename `fMasterNode` to `fMasternodeMode` to clarify its meaning and to avoid confusion with `CNode::fMasternode` (#1874)
- [`f6d98422c`](https://github.com/dashpay/dash/commit/f6d98422c) Silence ratecheck_test (#1873)
- [`9cee4193b`](https://github.com/dashpay/dash/commit/9cee4193b) Separate .h generation from .json/.raw for different modules (#1870)
- [`83957f2d3`](https://github.com/dashpay/dash/commit/83957f2d3) Fix alertTests.raw.h (again) (#1869)
- [`c13afaad8`](https://github.com/dashpay/dash/commit/c13afaad8) Fix alertTests.raw.h generation (#1868)
- [`a46bf120b`](https://github.com/dashpay/dash/commit/a46bf120b) Don't directly call "wine test_dash.exe" and let "make check" handle it (#1841)
- [`e805f790e`](https://github.com/dashpay/dash/commit/e805f790e) Automatically build and push docker image to docker.io/dashpay/dashd-develop (#1809)
- [`d9058aa04`](https://github.com/dashpay/dash/commit/d9058aa04) Increase travis timeout for "wine src/test/test_dash.exe" call (#1820)
- [`10786fe8e`](https://github.com/dashpay/dash/commit/10786fe8e) Use travis_wait for "wine test_dash.exe" call to fix timeouts (#1812)
- [`4bce3bf8b`](https://github.com/dashpay/dash/commit/4bce3bf8b) Fix crash on exit when -createwalletbackups=0 (#1810)
- [`cd9c6994c`](https://github.com/dashpay/dash/commit/cd9c6994c) Implement named devnets (#1791)
- [`ebbd26a05`](https://github.com/dashpay/dash/commit/ebbd26a05) Drop IsInputAssociatedWithPubkey and optimize CheckOutpoint (#1783)

### Backports and related fixes:
- See commit list [here](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3-backports.md)


Credits
=======

Thanks to everyone who directly contributed to this release:

- Alexander Block
- Chris Adam
- Codarren Velvindron
- crowning-
- gladcow
- InhumanPerfection
- Kamil Woźniak
- Nathan Marley
- Oleg Girko
- PaulieD
- Semen Martynov
- Spencer Lievens
- thephez
- UdjinM6

As well as Bitcoin Core Developers and everyone who submitted issues,
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

- [v0.12.2.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.2.3.md) released Jan/12/2018
- [v0.12.2.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.2.2.md) released Dec/17/2017
- [v0.12.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.2.md) released Nov/08/2017
- [v0.12.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.1.md) released Feb/06/2017
- [v0.12.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.0.md) released Jun/15/2015
- [v0.11.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.2.md) released Mar/04/2015
- [v0.11.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.1.md) released Feb/10/2015
- [v0.11.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.0.md) released Jan/15/2015
- [v0.10.x](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.10.0.md) released Sep/25/2014
- [v0.9.x](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.9.0.md) released Mar/13/2014

