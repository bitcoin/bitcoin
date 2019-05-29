Dash Core version 0.14.0.0
==========================

Release is now available from:

  <https://www.dash.org/downloads/#wallets>

This is a new major version release, bringing new features, various bugfixes and other improvements.

Please report bugs using the issue tracker at github:

  <https://github.com/dashpay/dash/issues>


Upgrading and downgrading
=========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Dash-Qt (on Mac) or
dashd/dash-qt (on Linux). If you upgrade after DIP0003 activation and you were
using version < 0.13 you will have to reindex (start with -reindex-chainstate
or -reindex) to make sure your wallet has all the new data synced. Upgrading from
version 0.13 should not require any additional actions.

Downgrade warning
-----------------

### Downgrade to a version < 0.13.0.0

Downgrading to a version smaller than 0.13 is not supported anymore as DIP2/DIP3 has
activated on mainnet and testnet.

### Downgrade to versions 0.13.0.0 - 0.13.3.0

Downgrading to 0.13 releases is fully supported until DIP0008 activation but is not
recommended unless you have some serious issues with version 0.14.

Notable changes
===============

DIP0004 - Coinbase Payload v2
-----------------------------
Coinbase Payload v2 introduces new field `merkleRootQuorums` which represents the merkle root of
all the hashes of the final quorum commitments of all active LLMQ sets. This allows SPV clients
to verify active LLMQ sets and use this information to further verify ChainLocks and LLMQ-based
InstantSend messages. Coinbase Payload v2 relies on DIP0008 (bit 4) activation.

https://github.com/dashpay/dips/blob/master/dip-0004.md#calculating-the-merkle-root-of-the-active-llmqs

DIP0008 - ChainLocks
--------------------
This version introduces ChainLocks, a technology for near-instant confirmation of blocks and
finding near-instant consensus on the longest valid/accepted chain. ChainLocks leverages LLMQ
Signing Requests/Sessions to accomplish this. ChainLocks relies on DIP0008 (bit 4) activation and
`SPORK_19_CHAINLOCKS_ENABLED` spork.

Read more: https://github.com/dashpay/dips/blob/master/dip-0008.md

DIP0010 - LLMQ-based InstantSend
--------------------------------
InstantSend is a feature to allow instant confirmations of payments. It works by locking transaction
inputs through masternode quorums. It has been present in Dash for a few years and been proven to work.
Nevertheless, there are some limits which could theoretically be removed in the old system but doing so
would have created risks in terms of scalability and security.

We introduce LLMQ-based InstantSend which is designed to be much more scalable without sacrificing
security and which allows all transactions to be treated as InstantSend transactions. The old system
differentiated transactions as InstantSend transactions by using the P2P message “ix” instead of “tx”.
Since this distinction is not required in the new system, the P2P message “ix” will be removed after
DIP0008 deployment (for now, transactions sent via "ix" message will be relayed further via "tx" message).

Read more: https://github.com/dashpay/dips/blob/master/dip-0010.md

Network
------
Legacy messages `mnw`, `mnwb`, `mnget`, `mnb`, `mnp`, `dseg`, `mnv`, `qdcommit` and their corresponding
inventory types (7, 10, 14, 15, 19, 22) are no longer suported.

Message `version` is extended with a 256 bit field - a challenge sent to a masternode. Masternode which
received such a challenge must reply with new p2p message `mnauth` directly after `verack`. This `mnauth`
message must include a signed challenge that was previously sent via `version`.

Mining
------
Due to changes in coinbase payload this version requires for miners to signal their readiness via
BIP9-like mechanism - by setting bit 4 of the block version to 1. Note that if your mining software
simply uses `coinbase_payload` field from `getblocktemplate` RPC and doesn't construct coinbase payload
manually then there should be no changes to your mining software required. We however encourage pools
and solo-miners to check their software compatibility on testnet to ensure flawless migration.

PrivateSend
-----------
The wallet will try to create and consume denoms a bit more accurately now. It will also only create a
limited number of inputs for each denominated amount to prevent bloating itself with mostly the smallest
denoms. You can control this number of inputs via new `-privatesenddenoms` cmd-line option (default is 300).

InstantSend
-----------
Legacy InstantSend is going to be superseded by the newly implemented LLMQ-based one once DIP0008 (bit 4)
is active and `SPORK_20_INSTANTSEND_LLMQ_BASED` spork is ON.

Sporks
------
There are two new sporks introduced in this version - `SPORK_19_CHAINLOCKS_ENABLED` and
`SPORK_20_INSTANTSEND_LLMQ_BASED`. `SPORK_17_QUORUM_DKG_ENABLED` was introduced in v0.13 but was kept OFF.
It will be turned on once 80% masternodes are upgraded to v0.14 which will enable DKG and DKG-based PoSe.

QR codes
--------
Wallet can now show QR codes for addresses in the address book, receiving addresses and addresses identified
in transactions list (right click -> "Show QR-code").

RPC changes
-----------
There are a few changes in existing RPC interfaces in this release:
- for blockchain based RPC commands `instantlock` will say `true` if the transaction
was locked via LLMQ based ChainLocks (for backwards compatibility reasons)
- `prioritisetransaction` no longer allows adjusting priority
- `getgovernanceinfo` no longer has `masternodewatchdogmaxseconds` and `sentinelpingmaxseconds` fields
- `masternodelist` no longer supports `activeseconds`, `daemon`, `lastseen`, `protocol`, `keyid`, `rank`
and `sentinel` modes, new mode - `pubkeyoperator`
- `masternode count` no longer has `ps_compatible` and `qualify` fields and `ps` and `qualify` modes
- `masternode winner` and `masternode current` no longer have `protocol`, `lastseen` and `activeseconds`
fields, new field - `proTxHash`
- `debug` supports new categories: `chainlocks`, `llmq`, `llmq-dkg`, `llmq-sigs`
- `mnsync` no longer has `IsMasternodeListSynced` and `IsWinnersListSynced` fields
- various RPCs that had `instantlock` field have `chainlock` (excluding mempool RPCs) and
`instantlock_internal` fields now

There are also new RPC commands:
- `bls fromsecret` parses a BLS secret key and returns the secret/public key pair
- `quorum` is a set of commands for quorums/LLMQs e.g. `list` to show active quorums (by default,
can specify different `count`) or `info` to shows detailed information about some specific quorum
etc., see `help quorum`

Few RPC commands are no longer supported: `estimatepriority`, `estimatesmartpriority`,
`gobject getvotes`, `masternode start-*`, `masternode genkey`, `masternode list-conf`, `masternode check`,
`masternodebroadcast`, `sentinelping`

See `help command` in rpc for more info.

ZMQ changes
-----------
Added two new messages `hashchainlock` and `rawchainlock` which return the hash of the chainlocked block
or the raw block itself respectively.

Command-line options
--------------------

Changes in existing cmd-line options:
- `-bip9params` supports optional `window` and `threshold` values now

New cmd-line options:
- `-watchquorums`
- `-privatesenddenoms`
- `-dip3params` (regtest-only)
- `-llmqchainlocks` (devnet-only)

Few cmd-line options are no longer supported: `-limitfreerelay`, `-relaypriority`, `-blockprioritysize`,
`-sendfreetransactions`, `-mnconf`, `-mnconflock`, `-masternodeprivkey`

See `Help -> Command-line options` in Qt wallet or `dashd --help` for more info.

Miscellaneous
-------------

A lot of refactoring, backports, code cleanups and other small fixes were done in this release.

 0.14.0.0 Change log
===================

See detailed [set of changes](https://github.com/dashpay/dash/compare/v0.13.3.0...dashpay:v0.14.0.0).

- [`612a90e8c`](https://github.com/dashpay/dash/commit/612a90e8c) Set CLIENT_VERSION_IS_RELEASE to true (#2926)
- [`9482f77e5`](https://github.com/dashpay/dash/commit/9482f77e5) Update help text via gen-manpages.sh (#2929)
- [`211a39d19`](https://github.com/dashpay/dash/commit/211a39d19) 0.14 release notes draft (#2896)
- [`84162021a`](https://github.com/dashpay/dash/commit/84162021a) Fix duplicate `-instantsendnotify` invocation (#2925)
- [`38aab0c5e`](https://github.com/dashpay/dash/commit/38aab0c5e) Add blocks conflicting with ChainLocks to block index (#2923)
- [`394bec483`](https://github.com/dashpay/dash/commit/394bec483) Skip processing in SyncTransaction when chain is not synced yet (#2920)
- [`c8a80b279`](https://github.com/dashpay/dash/commit/c8a80b279) Set DIP0008 mainnet activation params (#2915)
- [`040abafe3`](https://github.com/dashpay/dash/commit/040abafe3) [0.14] Bump chainparams (#2910)
- [`240238b19`](https://github.com/dashpay/dash/commit/240238b19) Fix db leaks in LLMQ db (#2914)
- [`56f31195d`](https://github.com/dashpay/dash/commit/56f31195d) Fall back to ReadBlockFromDisk when blockTxs is not filled yet (#2908)
- [`4dc5c0e9e`](https://github.com/dashpay/dash/commit/4dc5c0e9e) Bump "keepOldConnections" by one for all LLMQ types (#2909)
- [`7696a6fc8`](https://github.com/dashpay/dash/commit/7696a6fc8) Print inputs on which we voted and quorums used for signing (#2907)
- [`a173e6836`](https://github.com/dashpay/dash/commit/a173e6836) Implement integration tests for DKG error handling (#2905)
- [`89f6f7591`](https://github.com/dashpay/dash/commit/89f6f7591) Implement zmq notifications for chainlocked blocks (#2899)
- [`66a2cdeaf`](https://github.com/dashpay/dash/commit/66a2cdeaf) Properly handle conflicts between ChainLocks and InstantSend (#2904)
- [`b63f7dd39`](https://github.com/dashpay/dash/commit/b63f7dd39) Fix a crash in `masternode winners` when `count` is too large (#2902)
- [`357b7279d`](https://github.com/dashpay/dash/commit/357b7279d) Implement isolate_node/reconnect_isolated_node in tests (#2901)
- [`7fdc66dd8`](https://github.com/dashpay/dash/commit/7fdc66dd8) Ask for locked TXs after removing conflicting TXs (#2898)
- [`5d05ab17a`](https://github.com/dashpay/dash/commit/5d05ab17a) Fix PrivateSend log (#2892)
- [`53827a376`](https://github.com/dashpay/dash/commit/53827a376) Remove code for QDEBUGSTATUS propagation (#2891)
- [`783cb9ca6`](https://github.com/dashpay/dash/commit/783cb9ca6) Skip CheckCbTxMerkleRoots until assumeValid block (#2890)
- [`4dee7c4a2`](https://github.com/dashpay/dash/commit/4dee7c4a2) Cache heavy parts of `CalcCbTxMerkleRoot*` (#2885)
- [`b3ed6410f`](https://github.com/dashpay/dash/commit/b3ed6410f) Be more accurate with denom creation/consumption (#2853)
- [`3d993ee8f`](https://github.com/dashpay/dash/commit/3d993ee8f) Translations v14 (#2638)
- [`fbd244dde`](https://github.com/dashpay/dash/commit/fbd244dde) Bail out in few more places when blockchain is not synced yet (#2888)
- [`fd6aaae7f`](https://github.com/dashpay/dash/commit/fd6aaae7f) Add proTxHash to masternode list rpc response (#2887)
- [`dd3977523`](https://github.com/dashpay/dash/commit/dd3977523) More "bench" log for Dash specific parts (#2884)
- [`8ffdcbf99`](https://github.com/dashpay/dash/commit/8ffdcbf99) A bunch of mostly trivial tweaks/fixes (#2889)
- [`195100161`](https://github.com/dashpay/dash/commit/195100161) New LLMQ-based IS should have no legacy IS strings in UI and no legacy restrictions (#2883)
- [`7f419ae7f`](https://github.com/dashpay/dash/commit/7f419ae7f) Accept non-spent LLMQ IS locked outpoints from mempool in PS mixing (#2878)
- [`2652030a2`](https://github.com/dashpay/dash/commit/2652030a2) Use larger nPruneAfterSizeIn parameter for mapAlreadyAskedFor (#2882)
- [`03021fa53`](https://github.com/dashpay/dash/commit/03021fa53) Harden DIP3 activation (#2881)
- [`dcdb9dba1`](https://github.com/dashpay/dash/commit/dcdb9dba1) Add NotifyMasternodeListChanged signal to CClientUIInterface (#2880)
- [`19a9e2f4c`](https://github.com/dashpay/dash/commit/19a9e2f4c) Report `instantlock: true` for transactions locked via ChainLocks (#2877)
- [`5cfceab86`](https://github.com/dashpay/dash/commit/5cfceab86) Refactor IS-lock GUI notification and implement a similar one for ChainLocks (#2875)
- [`ed30db713`](https://github.com/dashpay/dash/commit/ed30db713) Ensure wallet is available and unlocked for some `governance` and `evo` RPCs (#2874)
- [`0c2fdf4da`](https://github.com/dashpay/dash/commit/0c2fdf4da) Refactor some Dash/mixing-specific wallet functions (#2850)
- [`22ae0bc21`](https://github.com/dashpay/dash/commit/22ae0bc21) Archive islock hashes when removing confirmed islocks (#2872)
- [`b322b4828`](https://github.com/dashpay/dash/commit/b322b4828) Wait longer until re-requesting recovered sigs and ISLOCKs from other peers (#2871)
- [`2502aadd7`](https://github.com/dashpay/dash/commit/2502aadd7) Fix infinite loop in CDBTransactionIterator::SkipDeletedAndOverwritten (#2870)
- [`cd94cbe6f`](https://github.com/dashpay/dash/commit/cd94cbe6f) Track which TXs are not locked yet and use this info in ProcessPendingRetryLockTxs (#2869)
- [`c4549aca2`](https://github.com/dashpay/dash/commit/c4549aca2) Change `quorum list` behaviour to list active quorums by default (#2866)
- [`cff9f9717`](https://github.com/dashpay/dash/commit/cff9f9717) Prefix all bls/quorum threads with `dash-` (#2865)
- [`5e865f9c8`](https://github.com/dashpay/dash/commit/5e865f9c8) Bump mempool counter on each successful IS lock (#2864)
- [`a67e66143`](https://github.com/dashpay/dash/commit/a67e66143) Don't disconnect peers on MNAUTH verification failure (#2868)
- [`1d8c7226d`](https://github.com/dashpay/dash/commit/1d8c7226d) Fix race condition in sendheaders.py (#2862)
- [`0c54e41f2`](https://github.com/dashpay/dash/commit/0c54e41f2) Retry locking of child TXs in batches instead of per locked parent (#2858)
- [`fbe44761c`](https://github.com/dashpay/dash/commit/fbe44761c) Don't wake up select if it was already woken up (#2863)
- [`7fe1a4a78`](https://github.com/dashpay/dash/commit/7fe1a4a78) Also invoke WriteInstantSendLockMined when IS lock comes after the mined block (#2861)
- [`f32f9523b`](https://github.com/dashpay/dash/commit/f32f9523b) Use lazy BLS signatures more often and don't always verify self-recovered sigs (#2860)
- [`5e8ae2ceb`](https://github.com/dashpay/dash/commit/5e8ae2ceb) Disable optimistic send in PushMessage by default (#2859)
- [`90b1b7196`](https://github.com/dashpay/dash/commit/90b1b7196) Move processing of InstantSend locks into its own worker thread (#2857)
- [`ae78360e5`](https://github.com/dashpay/dash/commit/ae78360e5) Add cache to CQuorumManager::ScanQuorums (#2856)
- [`241f76f9b`](https://github.com/dashpay/dash/commit/241f76f9b) Collection of minor performance optimizations (#2855)
- [`82a47f543`](https://github.com/dashpay/dash/commit/82a47f543) Allow testing ChainLocks enforcement with spork19 == 1 (#2854)
- [`e67618ac5`](https://github.com/dashpay/dash/commit/e67618ac5) Clean up a few GUI related items (#2846)
- [`225c3898d`](https://github.com/dashpay/dash/commit/225c3898d) Don't skip "safe TX" check when ChainLocks are not enforced yet (#2851)
- [`27b2cd2cc`](https://github.com/dashpay/dash/commit/27b2cd2cc) Skip required services and port checks when outgoing connections is a MN (#2847)
- [`aeb4c60c8`](https://github.com/dashpay/dash/commit/aeb4c60c8) Reimplement CMNAuth::NotifyMasternodeListChanged to work with new interface
- [`fa90c0204`](https://github.com/dashpay/dash/commit/fa90c0204) Also call NotifyMasternodeListChanged when MNs have been updated
- [`db781b32d`](https://github.com/dashpay/dash/commit/db781b32d) Pass oldList and diff instead of newList into NotifyMasternodeListChanged
- [`b0260e970`](https://github.com/dashpay/dash/commit/b0260e970) Do not maintain CService in masternodeQuorumNodes
- [`60788ce32`](https://github.com/dashpay/dash/commit/60788ce32) Connect to most recently updated address in ThreadOpenMasternodeConnections
- [`93b1b3d73`](https://github.com/dashpay/dash/commit/93b1b3d73) Fix shadowing of "addr"
- [`5bebdda71`](https://github.com/dashpay/dash/commit/5bebdda71) Add GetValidMNByService to CDeterministicMNList
- [`5d94d6bdf`](https://github.com/dashpay/dash/commit/5d94d6bdf) Remove unused CConnman::ForEachQuorumMember
- [`1ba8694cd`](https://github.com/dashpay/dash/commit/1ba8694cd) Various fixes for RemoveInvalidVotes() (#2845)
- [`b5bc7c9da`](https://github.com/dashpay/dash/commit/b5bc7c9da) Call HandleFullyConfirmedBlock when ChainLocks are enabled but not enforced (#2844)
- [`9fa09b974`](https://github.com/dashpay/dash/commit/9fa09b974) CBLSWrapper::SetHexStr() should not accept non-hex strings (#2843)
- [`0f0d8eaf4`](https://github.com/dashpay/dash/commit/0f0d8eaf4) Add RPC for BLS secret to public key (#2841)
- [`6982d9854`](https://github.com/dashpay/dash/commit/6982d9854) Ignore cache files on reindex (#2840)
- [`f8bedba7c`](https://github.com/dashpay/dash/commit/f8bedba7c) Don't retry locks when new IS system is disabled (#2837)
- [`92feade81`](https://github.com/dashpay/dash/commit/92feade81) Remove unused forward declaration (#2838)
- [`206e5a1b4`](https://github.com/dashpay/dash/commit/206e5a1b4) Use big endian inversed height in CInstantSendDb
- [`4b9f6cd3a`](https://github.com/dashpay/dash/commit/4b9f6cd3a) Use big endian inversed height in BuildInversedHeightKey
- [`53656b3e8`](https://github.com/dashpay/dash/commit/53656b3e8) Compare CDataStream internal vector with unsigned comparison
- [`dd21d046f`](https://github.com/dashpay/dash/commit/dd21d046f) Avoid unnecessary calls to parentIt->GetKey
- [`d34ec7866`](https://github.com/dashpay/dash/commit/d34ec7866) Update wallet transactions when confirmed IS locks are removed
- [`b897505f8`](https://github.com/dashpay/dash/commit/b897505f8) Remove the need for maintaining the last ChainLocked block in the DB
- [`8e7083cb8`](https://github.com/dashpay/dash/commit/8e7083cb8) Use db.RemoveConfirmedISLocks() in NotifyChainLock to remove confirmed locks
- [`4577438e8`](https://github.com/dashpay/dash/commit/4577438e8) Implement RemoveConfirmedInstantSendLocks to prune confirmed IS locks from DB
- [`d6e775851`](https://github.com/dashpay/dash/commit/d6e775851) Keep track of when IS locks were mined
- [`0a6416e06`](https://github.com/dashpay/dash/commit/0a6416e06) Wipe llmq db on reindex (#2835)
- [`4af5ea8a7`](https://github.com/dashpay/dash/commit/4af5ea8a7) Remove netfulfilledman.h duplicate (#2834)
- [`208406df7`](https://github.com/dashpay/dash/commit/208406df7) Re-introduce nInstantSendKeepLock check for LLMQ-based IS when spork19 is OFF (#2829)
- [`7d765a0fc`](https://github.com/dashpay/dash/commit/7d765a0fc) Track best block to later know if a DB upgrade is needed
- [`1a25c2084`](https://github.com/dashpay/dash/commit/1a25c2084) Apply suggestions from code review
- [`282cb697a`](https://github.com/dashpay/dash/commit/282cb697a) Use version 2 CCbTx in create_coinbase to fix test failures
- [`adc101a11`](https://github.com/dashpay/dash/commit/adc101a11) Implement quorum commitment merkle root tests in dip4-coinbasemerkleroots.py
- [`40ad06e77`](https://github.com/dashpay/dash/commit/40ad06e77) Return the fresh quorum hash from mine_quorum
- [`17b9318a0`](https://github.com/dashpay/dash/commit/17b9318a0) Mine SIGN_HEIGHT_OFFSET additional blocks after the quorum commitment
- [`5e832e2fa`](https://github.com/dashpay/dash/commit/5e832e2fa) Implement support for CbTx version 2 (with quorum merkle root)
- [`b1b41f02a`](https://github.com/dashpay/dash/commit/b1b41f02a) Fix a crash in mininode.py when inventory type is unknown
- [`44a3b9c90`](https://github.com/dashpay/dash/commit/44a3b9c90) Don't use pindex->GetBlockHash() in ProcessCommitment
- [`f9dbe3ed5`](https://github.com/dashpay/dash/commit/f9dbe3ed5) Track in which block a quorum commitment was mined
- [`ba459663b`](https://github.com/dashpay/dash/commit/ba459663b) Add deletedQuorums and newQuorums to CSimplifiedMNListEntry
- [`8f7929bed`](https://github.com/dashpay/dash/commit/8f7929bed) Implement and enforce quorum commitment merkle roots in coinbases
- [`07620746a`](https://github.com/dashpay/dash/commit/07620746a) Implement GetMined(AndActive)CommitmentsUntilBlock and use it in ScanQuorums
- [`d5250a333`](https://github.com/dashpay/dash/commit/d5250a333) Track at which height a quorum commitment was mined
- [`806948f90`](https://github.com/dashpay/dash/commit/806948f90) Store the full commitment in CQuorum
- [`b67f6a0dc`](https://github.com/dashpay/dash/commit/b67f6a0dc) Implement CDBTransactionIterator
- [`6d1599bc6`](https://github.com/dashpay/dash/commit/6d1599bc6) Change CDBTransaction to compare keys by their serialized form
- [`5482083eb`](https://github.com/dashpay/dash/commit/5482083eb) Support passing CDataStream as key into CDBWrapper/CDBBatch/CDBIterator
- [`c23dfaf57`](https://github.com/dashpay/dash/commit/c23dfaf57) Update qa/rpc-tests/dip4-coinbasemerkleroots.py
- [`9f2e5d085`](https://github.com/dashpay/dash/commit/9f2e5d085) Use FromHex to deserialize block header
- [`999848432`](https://github.com/dashpay/dash/commit/999848432) Implement dip4-coinbasemerkleroots.py integration tests
- [`ade5760a9`](https://github.com/dashpay/dash/commit/ade5760a9) Allow registering MNs without actually starting them
- [`ef6b6a1e6`](https://github.com/dashpay/dash/commit/ef6b6a1e6) Implement support for GETMNLISTD and MNLISTDIFF P2P message in mininode.py
- [`585b9c281`](https://github.com/dashpay/dash/commit/585b9c281) Make CBlock.get_merkle_root static
- [`1e0bdbc9b`](https://github.com/dashpay/dash/commit/1e0bdbc9b) Implement CPartialMerkleTree and CMerkleBlock in mininode.py
- [`d8778f555`](https://github.com/dashpay/dash/commit/d8778f555) Implement CService in mininode.py
- [`02480402b`](https://github.com/dashpay/dash/commit/02480402b) Implement deser_dyn_bitset and ser_dyn_bitset in mininode.py
- [`b0850fad0`](https://github.com/dashpay/dash/commit/b0850fad0) Do not skip pushing of vMatch and vHashes in CMerkleBlock (#2826)
- [`58589fb50`](https://github.com/dashpay/dash/commit/58589fb50) Trivial: Fix a couple typos (#2818)
- [`992922c49`](https://github.com/dashpay/dash/commit/992922c49) Specify DIP3 enforcement block height/hash for mainnet params
- [`a370bbfe3`](https://github.com/dashpay/dash/commit/a370bbfe3) Update immer library to current master (0a718d2d76bab6ebdcf43de943bd6c7d2dbfe2f9) (#2821)
- [`9f04855ae`](https://github.com/dashpay/dash/commit/9f04855ae) Fix blsWorker (#2820)
- [`377dd3b82`](https://github.com/dashpay/dash/commit/377dd3b82) There can be no two votes which differ by the outcome only (#2819)
- [`a87909ec3`](https://github.com/dashpay/dash/commit/a87909ec3) Keep the most recent gobject votes only (#2815)
- [`010752d4e`](https://github.com/dashpay/dash/commit/010752d4e) Set fAllowMultiplePorts to true for testnet (#2817)
- [`74d999e56`](https://github.com/dashpay/dash/commit/74d999e56) Remove watchdogs from existence (#2816)
- [`bfc288afb`](https://github.com/dashpay/dash/commit/bfc288afb) Update getblock rpc help text (#2814)
- [`aeba4afce`](https://github.com/dashpay/dash/commit/aeba4afce) Fix vote ratecheck (#2813)
- [`ad7defba9`](https://github.com/dashpay/dash/commit/ad7defba9) Drop all kind of invalid votes from all types of gobjects (#2812)
- [`e75760fa7`](https://github.com/dashpay/dash/commit/e75760fa7) Update "listtransactions" and "listsinceblock" RPC help (#2811)
- [`8987a6c3e`](https://github.com/dashpay/dash/commit/8987a6c3e) Update "debug" rpc help text (#2810)
- [`4b4234f39`](https://github.com/dashpay/dash/commit/4b4234f39) Refactor: fix layer violation for LLMQ based IS in UI (#2808)
- [`614cb6c2e`](https://github.com/dashpay/dash/commit/614cb6c2e) Fix getgovernanceinfo rpc help text (#2809)
- [`39ba45f3c`](https://github.com/dashpay/dash/commit/39ba45f3c) Show chainlocked txes as fully confirmed (#2807)
- [`f87035d14`](https://github.com/dashpay/dash/commit/f87035d14) Fix qt tests and actually run them (#2801)
- [`162acc5a0`](https://github.com/dashpay/dash/commit/162acc5a0) Fix potential deadlock in LoadWallet() (#2806)
- [`81eeff1c5`](https://github.com/dashpay/dash/commit/81eeff1c5) Fix devnet genesis check in InitBlockIndex() (#2805)
- [`4d8ef3512`](https://github.com/dashpay/dash/commit/4d8ef3512) Reset local/static cache in LogAcceptCategory when categories change (#2804)
- [`4a79f7a70`](https://github.com/dashpay/dash/commit/4a79f7a70) Few trivial cleanups (#2803)
- [`5057ad511`](https://github.com/dashpay/dash/commit/5057ad511) Drop DBG macros uses from governance modules (#2802)
- [`29a9e24b4`](https://github.com/dashpay/dash/commit/29a9e24b4) Prepare Dash-related stuff before starting ThreadImport (#2800)
- [`8f280f346`](https://github.com/dashpay/dash/commit/8f280f346) Split "llmq" debug category into "llmq", "llmq-dkg" and "llmq-sigs" (#2799)
- [`15c720dd4`](https://github.com/dashpay/dash/commit/15c720dd4) Stop tracking interested/participating nodes and send/announce to MNAUTH peers (#2798)
- [`f20620b0a`](https://github.com/dashpay/dash/commit/f20620b0a) Also handle MNAUTH on non-masternodes (#2797)
- [`b18f8cb77`](https://github.com/dashpay/dash/commit/b18f8cb77) Implement MNAUTH and allow unlimited inbound MN connections (#2790)
- [`aae985746`](https://github.com/dashpay/dash/commit/aae985746) Update log categories in help message and in decomposition of "dash" category (#2792)
- [`7b76e7abb`](https://github.com/dashpay/dash/commit/7b76e7abb) Implement BIP9 style deployment for DIP8/ChainLocks and fix a bug with late headers (#2793)
- [`3ead8cd85`](https://github.com/dashpay/dash/commit/3ead8cd85) Fix potential travis failures due to network failures (#2795)
- [`02db06658`](https://github.com/dashpay/dash/commit/02db06658) Fix loop in CLLMQUtils::GetQuorumConnections to add at least 2 connections (#2796)
- [`071b60ded`](https://github.com/dashpay/dash/commit/071b60ded) Bump MAX_OUTBOUND_MASTERNODE_CONNECTIONS to 250 on masternodes (#2791)
- [`0ed5ae05a`](https://github.com/dashpay/dash/commit/0ed5ae05a) Fix bug in GetNextMasternodeForPayment (#2789)
- [`7135f01a1`](https://github.com/dashpay/dash/commit/7135f01a1) Fix revoke reason check for ProUpRevTx (#2787)
- [`658ce9eff`](https://github.com/dashpay/dash/commit/658ce9eff) Apply Bloom filters to DIP2 transactions extra payload (#2786)
- [`a1e4ac21f`](https://github.com/dashpay/dash/commit/a1e4ac21f) Disable logging of libevent debug messages (#2794)
- [`9a1362abd`](https://github.com/dashpay/dash/commit/9a1362abd) Introduce SENDDSQUEUE to indicate that a node is interested in DSQ messages (#2785)
- [`9e70209e4`](https://github.com/dashpay/dash/commit/9e70209e4) Honor bloom filters when announcing LLMQ based IS locks (#2784)
- [`12274e578`](https://github.com/dashpay/dash/commit/12274e578) Introduce "qsendrecsigs" to indicate that plain recovered sigs should be sent (#2783)
- [`60a91848a`](https://github.com/dashpay/dash/commit/60a91848a) Skip mempool.dat when wallet is starting in "zap" mode (#2782)
- [`b87821047`](https://github.com/dashpay/dash/commit/b87821047) Make LLMQ/InstantSend/ChainLocks code less spammy (#2781)
- [`591b0185c`](https://github.com/dashpay/dash/commit/591b0185c) Bump proto version and only send LLMQ related messages to v14 nodes (#2780)
- [`c3602372c`](https://github.com/dashpay/dash/commit/c3602372c) Implement retroactive IS locking of transactions first seen in blocks instead of mempool (#2770)
- [`9df6acdc2`](https://github.com/dashpay/dash/commit/9df6acdc2) Disable in-wallet miner for win/macos Travis/Gitian builds (#2778)
- [`5299d3933`](https://github.com/dashpay/dash/commit/5299d3933) Multiple refactorings/fixes for LLMQ bases InstantSend and ChainLocks (#2779)
- [`a5d2edbe0`](https://github.com/dashpay/dash/commit/a5d2edbe0) Relay spork after updating internal spork maps (#2777)
- [`e52763d21`](https://github.com/dashpay/dash/commit/e52763d21) Refactor and fix instantsend tests/utils (#2776)
- [`25205fd46`](https://github.com/dashpay/dash/commit/25205fd46) RPC - Remove P2PKH message from protx help (#2773)
- [`a69a5cf4a`](https://github.com/dashpay/dash/commit/a69a5cf4a) Use smaller (3 out of 5) quorums for regtest/Travis (#2774)
- [`396ebc2dc`](https://github.com/dashpay/dash/commit/396ebc2dc) Fix tests after 2768 (#2772)
- [`6f90cf7a1`](https://github.com/dashpay/dash/commit/6f90cf7a1) Merge bitcoin#9602: Remove coin age priority and free transactions - implementation (#2768)
- [`6350adf1b`](https://github.com/dashpay/dash/commit/6350adf1b) Slightly refactor ProcessInstantSendLock (#2767)
- [`3a1aeb000`](https://github.com/dashpay/dash/commit/3a1aeb000) Multiple fixes/refactorings for ChainLocks (#2765)
- [`152a78eab`](https://github.com/dashpay/dash/commit/152a78eab) Add compatibility code to P2PFingerprintTest until we catch up with backports
- [`72af215a3`](https://github.com/dashpay/dash/commit/72af215a3) Fix CreateNewBlock_validity by not holding cs_main when calling createAndProcessEmptyBlock
- [`95192d5b5`](https://github.com/dashpay/dash/commit/95192d5b5) Require no cs_main lock for ProcessNewBlock/ActivateBestChain
- [`2eb553174`](https://github.com/dashpay/dash/commit/2eb553174) Avoid cs_main in net_processing ActivateBestChain calls
- [`f69c4370d`](https://github.com/dashpay/dash/commit/f69c4370d) Refactor ProcessGetData in anticipation of avoiding cs_main for ABC
- [`7f54372bb`](https://github.com/dashpay/dash/commit/7f54372bb) Create new mutex for orphans, no cs_main in PLV::BlockConnected
- [`6085de378`](https://github.com/dashpay/dash/commit/6085de378) Add ability to assert a lock is not held in DEBUG_LOCKORDER
- [`9344dee8a`](https://github.com/dashpay/dash/commit/9344dee8a) Merge #11580: Do not send (potentially) invalid headers in response to getheaders
- [`d1a602260`](https://github.com/dashpay/dash/commit/d1a602260) Merge #11113: [net] Ignore getheaders requests for very old side blocks
- [`d1db98c67`](https://github.com/dashpay/dash/commit/d1db98c67) Merge #9665: Use cached [compact] blocks to respond to getdata messages
- [`0905b911d`](https://github.com/dashpay/dash/commit/0905b911d) Actually use cached most recent compact block
- [`cd0f94fb5`](https://github.com/dashpay/dash/commit/cd0f94fb5) Give wait_for_quorum_phase more time
- [`4ae52758b`](https://github.com/dashpay/dash/commit/4ae52758b) Remove size check in CDKGSessionManager::GetVerifiedContributions
- [`e21da2d99`](https://github.com/dashpay/dash/commit/e21da2d99) Move simple PoSe tests into llmq-simplepose.py
- [`6488135f4`](https://github.com/dashpay/dash/commit/6488135f4) Track index into self.nodes in mninfo
- [`f30ea6dfd`](https://github.com/dashpay/dash/commit/f30ea6dfd) Replace BITCOIN_UNORDERED_LRU_CACHE_H with DASH_UNORDERED_LRU_CACHE_H
- [`e763310b5`](https://github.com/dashpay/dash/commit/e763310b5) Add missing LOCK(cs_main)
- [`3a5e7c433`](https://github.com/dashpay/dash/commit/3a5e7c433) Do not hold cs_vNodes in CSigSharesManager::SendMessages() for too long (#2758)
- [`fbf0dcb08`](https://github.com/dashpay/dash/commit/fbf0dcb08) Various small cleanups (#2761)
- [`588eb30b8`](https://github.com/dashpay/dash/commit/588eb30b8) Fix deadlock in CSigSharesManager::SendMessages (#2757)
- [`7b24f9b8b`](https://github.com/dashpay/dash/commit/7b24f9b8b) Drop --c++11 brew flag in build-osx.md (#2755)
- [`ac00c6628`](https://github.com/dashpay/dash/commit/ac00c6628) Make InstantSend locks persistent
- [`293c9ad6a`](https://github.com/dashpay/dash/commit/293c9ad6a) Use unordered_lru_cache in CRecoveredSigsDb
- [`9e4aa1f98`](https://github.com/dashpay/dash/commit/9e4aa1f98) Implement unordered_lru_cache
- [`609114a80`](https://github.com/dashpay/dash/commit/609114a80) Code review: re-add string cast in mininode.py
- [`85ffc1d64`](https://github.com/dashpay/dash/commit/85ffc1d64) drop `swap_outputs_in_rawtx` and `DecimalEncoder` in smartfees.py
- [`bc593c84b`](https://github.com/dashpay/dash/commit/bc593c84b) Revert "Fix use of missing self.log in blockchain.py"
- [`0e91ebcf4`](https://github.com/dashpay/dash/commit/0e91ebcf4) Use logging framework in Dash specific tests
- [`dd1245c2a`](https://github.com/dashpay/dash/commit/dd1245c2a) Update dnsseed-policy.md (#2751)
- [`f351145e6`](https://github.com/dashpay/dash/commit/f351145e6) Use GetVoteForId instead of maintaining votes on inputs
- [`d4cf78fe2`](https://github.com/dashpay/dash/commit/d4cf78fe2) Add HasVotedOnId/GetVoteForId to CSigningManager
- [`43e1bf674`](https://github.com/dashpay/dash/commit/43e1bf674) Add key prefix to "rs_" for CRecoveredSigsDb keys
- [`61e10f651`](https://github.com/dashpay/dash/commit/61e10f651) Use llmqDb for CRecoveredSigsDb
- [`b2cd1db40`](https://github.com/dashpay/dash/commit/b2cd1db40) Don't use CEvoDB in CDKGSessionManager and instead use llmqDb
- [`e2cad1bd6`](https://github.com/dashpay/dash/commit/e2cad1bd6) Introduce global llmq::llmqDb instance of CDBWrapper
- [`acb52f6ec`](https://github.com/dashpay/dash/commit/acb52f6ec) Don't pass CEvoDB to CDKGSessionHandler and CDKGSession
- [`06fc65559`](https://github.com/dashpay/dash/commit/06fc65559) Actually remove from finalInstantSendLocks in CInstantSendManager::RemoveFinalISLock
- [`041a1c26d`](https://github.com/dashpay/dash/commit/041a1c26d) Move safe TX checks into TestForBlock and TestPackageTransactions
- [`4d3365ddb`](https://github.com/dashpay/dash/commit/4d3365ddb) Completely disable InstantSend while filling mempool in autoix-mempool.py
- [`fae33e03a`](https://github.com/dashpay/dash/commit/fae33e03a) Let ProcessPendingReconstructedRecoveredSigs return void instead of bool
- [`f8f867a6b`](https://github.com/dashpay/dash/commit/f8f867a6b) Sync blocks after generating in mine_quorum()
- [`3e60d2d2d`](https://github.com/dashpay/dash/commit/3e60d2d2d) Adjust LLMQ based InstantSend tests for new spork20
- [`41a71fe44`](https://github.com/dashpay/dash/commit/41a71fe44) update autoix-mempool.py to test both "old" and "new" InstantSend (and fix CheckCanLock to respect mempool limits)
- [`843b6d7a9`](https://github.com/dashpay/dash/commit/843b6d7a9) update p2p-autoinstantsend.py to test both "old" and "new" InstantSend
- [`a8da11ac5`](https://github.com/dashpay/dash/commit/a8da11ac5) update p2p-instantsend.py to test both "old" and "new" InstantSend
- [`55152cb31`](https://github.com/dashpay/dash/commit/55152cb31) Move IS block filtering into ConnectBlock
- [`2299ee283`](https://github.com/dashpay/dash/commit/2299ee283) Rename IXLOCK to ISLOCK and InstantX to InstantSend
- [`f5dcb00ac`](https://github.com/dashpay/dash/commit/f5dcb00ac) Introduce spork SPORK_20_INSTANTSEND_LLMQ_BASED to switch between new/old system
- [`280690792`](https://github.com/dashpay/dash/commit/280690792) Combine loops in CChainLocksHandler::NewPoWValidBlock
- [`9e9081110`](https://github.com/dashpay/dash/commit/9e9081110) Add override keywork to CDSNotificationInterface::NotifyChainLock
- [`e2f99f4ae`](https://github.com/dashpay/dash/commit/e2f99f4ae) Set llmqForInstantSend for mainnet and testnet
- [`5b8344e8f`](https://github.com/dashpay/dash/commit/5b8344e8f) Use scheduleFromNow instead of schedule+boost::chrono
- [`f44f09ca0`](https://github.com/dashpay/dash/commit/f44f09ca0) Fix crash in BlockAssembler::addPackageTxs
- [`baf8b81c4`](https://github.com/dashpay/dash/commit/baf8b81c4) Fix no-wallet build
- [`2a7a5c633`](https://github.com/dashpay/dash/commit/2a7a5c633) Only sign ChainLocks when all included TXs are "safe"
- [`96291e7a0`](https://github.com/dashpay/dash/commit/96291e7a0) Cheaper/Faster bailout from TrySignChainTip when already signed before
- [`0a5e8eb86`](https://github.com/dashpay/dash/commit/0a5e8eb86) Move ChainLock signing into TrySignChainTip and call it periodically
- [`bd7edc8ae`](https://github.com/dashpay/dash/commit/bd7edc8ae) Track txids of new blocks and first-seen time of TXs in CChainLocksHandler
- [`7945192ff`](https://github.com/dashpay/dash/commit/7945192ff) Enforce IX locks from the new system
- [`dc97835ec`](https://github.com/dashpay/dash/commit/dc97835ec) Disable explicit lock requests when the new IX system is active
- [`68cfdc932`](https://github.com/dashpay/dash/commit/68cfdc932) Also call ProcessTx from sendrawtransaction and RelayWalletTransaction
- [`1d2d370cd`](https://github.com/dashpay/dash/commit/1d2d370cd) Whenever we check for locked TXs, also check for the new system having a lock
- [`3a6cc2cc1`](https://github.com/dashpay/dash/commit/3a6cc2cc1) Show "verified via LLMQ based InstantSend" in GUI TX status
- [`34a8b2997`](https://github.com/dashpay/dash/commit/34a8b2997) Disable old IX code when the new system is active
- [`5ff4db0a0`](https://github.com/dashpay/dash/commit/5ff4db0a0) Downgrade TXLOCKREQUEST to TX when new IX system is active
- [`1959f3e4a`](https://github.com/dashpay/dash/commit/1959f3e4a) Handle incoming TXs by calling CInstantXManager::ProcessTx
- [`83dbcc483`](https://github.com/dashpay/dash/commit/83dbcc483) Implement CInstantSendManager and related P2P messages
- [`5bbc12274`](https://github.com/dashpay/dash/commit/5bbc12274) Implement PushReconstructedRecoveredSig in CSigningManager
- [`2bbac8ff7`](https://github.com/dashpay/dash/commit/2bbac8ff7) Introduce NotifyChainLock signal and invoke it when CLSIGs get processed
- [`e47af29d4`](https://github.com/dashpay/dash/commit/e47af29d4) Fix "chainlock" in WalletTxToJSON (#2748)
- [`e21d8d6b9`](https://github.com/dashpay/dash/commit/e21d8d6b9) Fix error message for invalid voting addresses (#2747)
- [`80891ee6f`](https://github.com/dashpay/dash/commit/80891ee6f) Make -masternodeblsprivkey mandatory when -masternode is given (#2745)
- [`521d4ae08`](https://github.com/dashpay/dash/commit/521d4ae08) Implement 2-stage commit for CEvoDB to avoid inconsistencies after crashes (#2744)
- [`e63cdadc9`](https://github.com/dashpay/dash/commit/e63cdadc9) Add a button/context menu items to show QR codes for addresses (#2741)
- [`b6177740c`](https://github.com/dashpay/dash/commit/b6177740c) Add collateraladdress into masternode/protx list rpc output (#2740)
- [`b5466e20a`](https://github.com/dashpay/dash/commit/b5466e20a) Add "chainlock" field to more rpcs (#2743)
- [`8dd934922`](https://github.com/dashpay/dash/commit/8dd934922) Don't be too harsh for invalid CLSIGs (#2742)
- [`a34fb6d6f`](https://github.com/dashpay/dash/commit/a34fb6d6f) Fix banning when local node doesn't have the vvec (#2739)
- [`4a495c6b4`](https://github.com/dashpay/dash/commit/4a495c6b4) Only include selected TX types into CMerkleBlock (#2737)
- [`0db2d1596`](https://github.com/dashpay/dash/commit/0db2d1596) code review and reset file perms
- [`f971da831`](https://github.com/dashpay/dash/commit/f971da831) Stop g_connman first before deleting it (#2734)
- [`9eb0ca704`](https://github.com/dashpay/dash/commit/9eb0ca704) Ignore sig share inv messages when we don't have the quorum vvec (#2733)
- [`080b59a57`](https://github.com/dashpay/dash/commit/080b59a57) Backport bitcoin#14385: qt: avoid system harfbuzz and bz2 (#2732)
- [`2041186f4`](https://github.com/dashpay/dash/commit/2041186f4) On timeout, print members proTxHashes from members which did not send a share (#2731)
- [`ea90296b6`](https://github.com/dashpay/dash/commit/ea90296b6) Actually start the timers for sig share and recSig verification (#2730)
- [`5c84cab0f`](https://github.com/dashpay/dash/commit/5c84cab0f) Send/Receive multiple messages as part of one P2P message in CSigSharesManager (#2729)
- [`d2573c43b`](https://github.com/dashpay/dash/commit/d2573c43b) Only return from wait_for_chainlock when the block is actually processed (#2728)
- [`6ac49da24`](https://github.com/dashpay/dash/commit/6ac49da24) Send QSIGSESANN messages when sending first message for a session
- [`8ce8cb9ca`](https://github.com/dashpay/dash/commit/8ce8cb9ca) Remove MarkXXX methods from CSigSharesNodeState
- [`fa25728ca`](https://github.com/dashpay/dash/commit/fa25728ca) Use new sessionId based session management in CSigSharesManager
- [`34e3f8eb5`](https://github.com/dashpay/dash/commit/34e3f8eb5) Implement session management based on session ids and announcements
- [`7372f6f10`](https://github.com/dashpay/dash/commit/7372f6f10) Move RebuildSigShare from CBatchedSigShares to CSigSharesManager
- [`55a6182b1`](https://github.com/dashpay/dash/commit/55a6182b1) Introduce QSIGSESANN/CSigSesAnn P2P message
- [`80375a0b4`](https://github.com/dashpay/dash/commit/80375a0b4) Change CSigSharesInv and CBatchedSigShares to be sessionId based
- [`9b4285b1c`](https://github.com/dashpay/dash/commit/9b4285b1c) Use salted hashing for keys for unordered maps/sets in LLMQ code
- [`b5462f524`](https://github.com/dashpay/dash/commit/b5462f524) Implement std::unordered_map/set compatible hasher classes for salted hashes
- [`c52e8402c`](https://github.com/dashpay/dash/commit/c52e8402c) Remove now obsolete TODO comment above CRecoveredSigsDb
- [`e83e32b95`](https://github.com/dashpay/dash/commit/e83e32b95) Add in-memory cache for CRecoveredSigsDb::HasRecoveredSigForHash
- [`677c0040c`](https://github.com/dashpay/dash/commit/677c0040c) Add in-memory cache to CQuorumBlockProcessor::HasMinedCommitment
- [`f305cf77b`](https://github.com/dashpay/dash/commit/f305cf77b) Multiple fixes and optimizations for LLMQs and ChainLocks (#2724)
- [`c3eb0c788`](https://github.com/dashpay/dash/commit/c3eb0c788) reset file perms
- [`c17356efc`](https://github.com/dashpay/dash/commit/c17356efc) Merge #9970: Improve readability of segwit.py, smartfees.py
- [`ee6e5654e`](https://github.com/dashpay/dash/commit/ee6e5654e) Merge #9505: Prevector Quick Destruct
- [`c4a3cd6a1`](https://github.com/dashpay/dash/commit/c4a3cd6a1) Merge #8665: Assert all the things!
- [`b09e3e080`](https://github.com/dashpay/dash/commit/b09e3e080) Merge #9977: QA: getblocktemplate_longpoll.py should always use >0 fee tx
- [`e8df27f8e`](https://github.com/dashpay/dash/commit/e8df27f8e) Merge #9984: devtools: Make github-merge compute SHA512 from git, instead of worktree
- [`c55e019bf`](https://github.com/dashpay/dash/commit/c55e019bf) Merge #9940: Fix verify-commits on OSX, update for new bad Tree-SHA512, point travis to different keyservers
- [`f9a2e4c4f`](https://github.com/dashpay/dash/commit/f9a2e4c4f) Merge #9514: release: Windows signing script
- [`ee2048ae4`](https://github.com/dashpay/dash/commit/ee2048ae4) Merge #9830: Add safe flag to listunspent result
- [`914bd7877`](https://github.com/dashpay/dash/commit/914bd7877) Merge #9972: Fix extended rpc tests broken by #9768
- [`dad8c67d3`](https://github.com/dashpay/dash/commit/dad8c67d3) Merge #9768: [qa] Add logging to test_framework.py
- [`c75d7dc83`](https://github.com/dashpay/dash/commit/c75d7dc83) Merge #9962: [trivial] Fix typo in rpc/protocol.h
- [`49b743e39`](https://github.com/dashpay/dash/commit/49b743e39) Merge #9538: [util] Remove redundant call to get() on smart pointer (thread_specific_ptr)
- [`e5c4a67a2`](https://github.com/dashpay/dash/commit/e5c4a67a2) Merge #9916: Fix msvc compiler error C4146 (minus operator applied to unsigned type)
- [`fcd3b4fd4`](https://github.com/dashpay/dash/commit/fcd3b4fd4) Disallow new proposals using legacy serialization (#2722)
- [`668b84b1e`](https://github.com/dashpay/dash/commit/668b84b1e) Fix stacktraces compilation issues (#2721)
- [`0fd1fb7d5`](https://github.com/dashpay/dash/commit/0fd1fb7d5) Don't build docker image when running Travis job on some another repo (#2718)
- [`48d92f116`](https://github.com/dashpay/dash/commit/48d92f116) Implement optional pretty printed stacktraces (#2420)
- [`0b552be20`](https://github.com/dashpay/dash/commit/0b552be20) Fix file permissions broken in 2682 (#2717)
- [`74bb23cac`](https://github.com/dashpay/dash/commit/74bb23cac) Add link to bugcrowd in issue template (#2716)
- [`252ee89c3`](https://github.com/dashpay/dash/commit/252ee89c3) Implement new algo for quorum connections (#2710)
- [`104c6e776`](https://github.com/dashpay/dash/commit/104c6e776) Cleanup successful sessions before doing timeout check (#2712)
- [`26db020d1`](https://github.com/dashpay/dash/commit/26db020d1) Separate init/destroy and start/stop steps in LLMQ flow (#2709)
- [`9f5869032`](https://github.com/dashpay/dash/commit/9f5869032) Avoid using ordered maps in LLMQ signing code (#2708)
- [`bb90eb4bf`](https://github.com/dashpay/dash/commit/bb90eb4bf) backports-0.15-pr6 code review
- [`7a192e2e4`](https://github.com/dashpay/dash/commit/7a192e2e4) Optimize sleeping behavior in CSigSharesManager::WorkThreadMain (#2707)
- [`d7bd0954f`](https://github.com/dashpay/dash/commit/d7bd0954f) Use pipe() together with fcntl instead of pipe2()
- [`742a25898`](https://github.com/dashpay/dash/commit/742a25898) Implement caching in CRecoveredSigsDb
- [`500b9c89a`](https://github.com/dashpay/dash/commit/500b9c89a) Use CBLSLazySignature in CBatchedSigShares
- [`02b68885a`](https://github.com/dashpay/dash/commit/02b68885a) Implement CBLSLazySignature for lazy serialization/deserialization
- [`6e8f50aa5`](https://github.com/dashpay/dash/commit/6e8f50aa5) Faster default-initialization of BLS primitives by re-using the null-hash
- [`c03480d20`](https://github.com/dashpay/dash/commit/c03480d20) Disable optimistic sending when pushing sig share related messages
- [`acb87895f`](https://github.com/dashpay/dash/commit/acb87895f) Implement WakeupSelect() to allow preliminary wakeup after message push
- [`cf2932098`](https://github.com/dashpay/dash/commit/cf2932098) Allow to disable optimistic send in PushMessage()
- [`bedfc262e`](https://github.com/dashpay/dash/commit/bedfc262e) Rework handling of CSigSharesManager worker thread (#2703)
- [`3e4286a58`](https://github.com/dashpay/dash/commit/3e4286a58) Less cs_main locks in quorums (#2702)
- [`3bbc75fc4`](https://github.com/dashpay/dash/commit/3bbc75fc4) Reintroduce spork15 so that it's relayed by 0.14 nodes (#2701)
- [`b71a3f48d`](https://github.com/dashpay/dash/commit/b71a3f48d) Remove not used and not implemented methods from net.h (#2700)
- [`c0cb27465`](https://github.com/dashpay/dash/commit/c0cb27465) Fix incorrect usage of begin() when genesis block is requested in "protx diff" (#2699)
- [`b239bb24a`](https://github.com/dashpay/dash/commit/b239bb24a) Do not process blocks in CDeterministicMNManager before dip3 activation (#2698)
- [`86fc05049`](https://github.com/dashpay/dash/commit/86fc05049) Drop no longer used code and bump min protos (#2697)
- [`fef8e5d45`](https://github.com/dashpay/dash/commit/fef8e5d45) A small overhaul of the way MN list/stats UI and data are tied together (#2696)
- [`90bb3ca2f`](https://github.com/dashpay/dash/commit/90bb3ca2f) Backport #14701: build: Add CLIENT_VERSION_BUILD to CFBundleGetInfoString (#2687)
- [`00f904ec7`](https://github.com/dashpay/dash/commit/00f904ec7) Change the way invalid ProTxes are handled in `addUnchecked` and `existsProviderTxConflict` (#2691)
- [`5478183e7`](https://github.com/dashpay/dash/commit/5478183e7) Call existsProviderTxConflict after CheckSpecialTx (#2690)
- [`1be5a72a9`](https://github.com/dashpay/dash/commit/1be5a72a9) Merge #9853: Fix error codes from various RPCs
- [`1bfc069e3`](https://github.com/dashpay/dash/commit/1bfc069e3) Merge #9575: Remove unused, non-working RPC PostCommand signal
- [`e53da66b2`](https://github.com/dashpay/dash/commit/e53da66b2) Merge #9936: [trivial] Fix three typos introduced into walletdb.h in commit 7184e25
- [`2121ba776`](https://github.com/dashpay/dash/commit/2121ba776) Merge #9945: Improve logging in bctest.py if there is a formatting mismatch
- [`b1d64f3a1`](https://github.com/dashpay/dash/commit/b1d64f3a1) Merge #9548: Remove min reasonable fee
- [`1c08f9a5f`](https://github.com/dashpay/dash/commit/1c08f9a5f) Merge #9369: Factor out CWallet::nTimeSmart computation into a method.
- [`9a3067115`](https://github.com/dashpay/dash/commit/9a3067115) fix compile error caused by #9605
- [`7e4257254`](https://github.com/dashpay/dash/commit/7e4257254) Allow to override llmqChainLocks with "-llmqchainlocks" on devnet (#2683)
- [`bed57cfbf`](https://github.com/dashpay/dash/commit/bed57cfbf) Stop checking MN protocol version before signalling DIP3 (#2684)
- [`8d249d4df`](https://github.com/dashpay/dash/commit/8d249d4df) Merge #9605: Use CScheduler for wallet flushing, remove ThreadFlushWalletDB
- [`9c8c12ed4`](https://github.com/dashpay/dash/commit/9c8c12ed4) devtools: Fix a syntax error typo
- [`8a436ec36`](https://github.com/dashpay/dash/commit/8a436ec36) Merge #9932: Fix verify-commits on travis and always check top commit's tree
- [`31267f4c8`](https://github.com/dashpay/dash/commit/31267f4c8) Merge #9555: [test] Avoid reading a potentially uninitialized variable in tx_invalid-test (transaction_tests.cpp)
- [`a31b2de7a`](https://github.com/dashpay/dash/commit/a31b2de7a) Merge #9906: Disallow copy constructor CReserveKeys
- [`22cda1a92`](https://github.com/dashpay/dash/commit/22cda1a92) Merge #9929: tests: Delete unused function `_rpchost_to_args`
- [`6addbe074`](https://github.com/dashpay/dash/commit/6addbe074) Merge #9880: Verify Tree-SHA512s in merge commits, enforce sigs are not SHA1
- [`29bbfc58f`](https://github.com/dashpay/dash/commit/29bbfc58f) Merge #8574: [Wallet] refactor CWallet/CWalletDB/CDB
- [`67a86091a`](https://github.com/dashpay/dash/commit/67a86091a) Implement and use secure BLS batch verification (#2681)
- [`e2ae2ae63`](https://github.com/dashpay/dash/commit/e2ae2ae63) Update misspelled Synchronizing in GetSyncStatus (#2680)
- [`721965990`](https://github.com/dashpay/dash/commit/721965990) Add missing help text for `operatorPayoutAddress` (#2679)
- [`68c0de4ba`](https://github.com/dashpay/dash/commit/68c0de4ba) Do not send excessive messages in governance sync (#2124)
- [`09e71de80`](https://github.com/dashpay/dash/commit/09e71de80) Fix bench log for payee and special txes (#2678)
- [`03fa11550`](https://github.com/dashpay/dash/commit/03fa11550) Speed up CQuorumManager::ScanQuorums (#2677)
- [`071035b93`](https://github.com/dashpay/dash/commit/071035b93) Apply code review suggestions #2647
- [`501227dee`](https://github.com/dashpay/dash/commit/501227dee) Merge #9333: Document CWalletTx::mapValue entries and remove erase of nonexistent "version" entry.
- [`a61b747a2`](https://github.com/dashpay/dash/commit/a61b747a2) Merge #9547: bench: Assert that division by zero is unreachable
- [`b821dfa7d`](https://github.com/dashpay/dash/commit/b821dfa7d) Merge #9739: Fix BIP68 activation test
- [`56890f98f`](https://github.com/dashpay/dash/commit/56890f98f) Merge #9832: [qa] assert_start_raises_init_error
- [`31755267a`](https://github.com/dashpay/dash/commit/31755267a) Missing `=` characters (#2676)
- [`088525bde`](https://github.com/dashpay/dash/commit/088525bde) Multiple fixes for LLMQs and BLS batch verification (#2674)
- [`ae70e8a34`](https://github.com/dashpay/dash/commit/ae70e8a34) Fix negative "keys left since backup" (#2671)
- [`2a330f17a`](https://github.com/dashpay/dash/commit/2a330f17a) Fix endless wait in RenameThreadPool (#2675)
- [`1400df2e5`](https://github.com/dashpay/dash/commit/1400df2e5) Invoke CheckSpecialTx after all normal TX checks have passed (#2673)
- [`18950f923`](https://github.com/dashpay/dash/commit/18950f923) Optimize DKG debug message processing for performance and lower bandwidth (#2672)
- [`4615da99f`](https://github.com/dashpay/dash/commit/4615da99f) Merge #9576: [wallet] Remove redundant initialization
- [`8944b5a78`](https://github.com/dashpay/dash/commit/8944b5a78) Merge #9905: [contrib] gh-merge: Move second sha512 check to the end
- [`8dfddf503`](https://github.com/dashpay/dash/commit/8dfddf503) Merge #9910: Docs: correct and elaborate -rpcbind doc
- [`395b53716`](https://github.com/dashpay/dash/commit/395b53716) Merge #9774: Enable host lookups for -proxy and -onion parameters
- [`2c3dde75c`](https://github.com/dashpay/dash/commit/2c3dde75c) Merge #9828: Avoid -Wshadow warnings in wallet_tests
- [`3d3443b6a`](https://github.com/dashpay/dash/commit/3d3443b6a) Merge #8808: Do not shadow variables (gcc set)
- [`053b97c94`](https://github.com/dashpay/dash/commit/053b97c94) Merge #9903: Docs: add details to -rpcclienttimeout doc
- [`5d1c97da1`](https://github.com/dashpay/dash/commit/5d1c97da1) Add getspecialtxes rpc (#2668)
- [`ca6c8f547`](https://github.com/dashpay/dash/commit/ca6c8f547) Add missing default value for SPORK_19_CHAINLOCKS_ENABLED (#2670)
- [`6da341379`](https://github.com/dashpay/dash/commit/6da341379) Use smaller LLMQs for ChainLocks on testnet and devnet (#2669)
- [`54f576ea7`](https://github.com/dashpay/dash/commit/54f576ea7) Fix LLMQ related test failures on Travis (#2666)
- [`6fe479aa1`](https://github.com/dashpay/dash/commit/6fe479aa1) Don't leak skShare in logs (#2662)
- [`559bdfc6e`](https://github.com/dashpay/dash/commit/559bdfc6e) ProcessSpecialTxsInBlock should respect fJustCheck (#2653)
- [`805aeaa16`](https://github.com/dashpay/dash/commit/805aeaa16) Drop cs_main from UpdatedBlockTip in CDKGSessionManager/CDKGSessionHandler (#2655)
- [`2a4fbb6e4`](https://github.com/dashpay/dash/commit/2a4fbb6e4) Bump block stats when adding commitment tx into block (#2654)
- [`25cb14b61`](https://github.com/dashpay/dash/commit/25cb14b61) Fix confusion between dip3 activation and enforcement (#2651)
- [`7caf9394e`](https://github.com/dashpay/dash/commit/7caf9394e) drop dash-docs folder and instead link to dash-docs.github.io in README (#2650)
- [`70a9e905c`](https://github.com/dashpay/dash/commit/70a9e905c) Use helper function to produce help text for params of `protx` rpcs (#2649)
- [`f123248f1`](https://github.com/dashpay/dash/commit/f123248f1) update copyright (#2648)
- [`d398bf052`](https://github.com/dashpay/dash/commit/d398bf052) reverse order from jsonRequest, strSubCommand
- [`ad4b3cda3`](https://github.com/dashpay/dash/commit/ad4b3cda3) Instead of asserting signatures once, wait for them to change to the expected state
- [`3237668b1`](https://github.com/dashpay/dash/commit/3237668b1) Rename inInvalidate->inEnforceBestChainLock and make it atomic
- [`5033d5ef4`](https://github.com/dashpay/dash/commit/5033d5ef4) Don't check for conflicting ChainLocks when phashBlock is not set
- [`08d915dc2`](https://github.com/dashpay/dash/commit/08d915dc2) Increase waiting time in LLMQ signing tests
- [`886299a40`](https://github.com/dashpay/dash/commit/886299a40) Implement llmq-chainlocks.py integration tests
- [`3413ff917`](https://github.com/dashpay/dash/commit/3413ff917) Add info about ChainLocks to block and transaction RPCs
- [`135829dc4`](https://github.com/dashpay/dash/commit/135829dc4) Add SPORK_19_CHAINLOCKS_ENABLED
- [`29532ba19`](https://github.com/dashpay/dash/commit/29532ba19) Implement and enforce ChainLocks
- [`2bf6eb1c7`](https://github.com/dashpay/dash/commit/2bf6eb1c7) Track parent->child relations for blocks
- [`04a51c9ef`](https://github.com/dashpay/dash/commit/04a51c9ef) Use a block that is 8 blocks in the past for SelectQuorumForSigning
- [`cf33efc9e`](https://github.com/dashpay/dash/commit/cf33efc9e) Move SelectQuorumForSigning into CSigningManager and make it height based
- [`4026ea203`](https://github.com/dashpay/dash/commit/4026ea203) Implement VerifyRecoveredSig to allow verifcation of sigs found in P2P messages
- [`9f211ef12`](https://github.com/dashpay/dash/commit/9f211ef12) Add listener interface to listen for recovered sigs
- [`189cee210`](https://github.com/dashpay/dash/commit/189cee210) Don't pass poolSize to SelectQuorum and instead use consensus params
- [`13855674d`](https://github.com/dashpay/dash/commit/13855674d) Add missing new-line character in log output
- [`d31edf66a`](https://github.com/dashpay/dash/commit/d31edf66a) Wait for script checks to finish before messing with txes in Dash-specific way (#2652)
- [`2c477b0d4`](https://github.com/dashpay/dash/commit/2c477b0d4) Fix no_wallet for rpcmasternode/rpcevo
- [`fc00b7bae`](https://github.com/dashpay/dash/commit/fc00b7bae) add import to rpcevo fixing backport 8775
- [`30b03863e`](https://github.com/dashpay/dash/commit/30b03863e) Apply suggestions from code review #2646
- [`c70aa6079`](https://github.com/dashpay/dash/commit/c70aa6079) change #8775 to keep dash codebase improvement, but still backport #9908
- [`afdb0a267`](https://github.com/dashpay/dash/commit/afdb0a267) Merge #9908: Define 7200 second timestamp window constant
- [`c094d4bbe`](https://github.com/dashpay/dash/commit/c094d4bbe) fix #8775 backport
- [`f9147466f`](https://github.com/dashpay/dash/commit/f9147466f) remove other rpc references to pwalletMain
- [`d7474fd56`](https://github.com/dashpay/dash/commit/d7474fd56) remove all references to pwalletMain in rpc folder
- [`87af11781`](https://github.com/dashpay/dash/commit/87af11781) Merge #8775: RPC refactoring: Access wallet using new GetWalletForJSONRPCRequest
- [`1fa7f7e74`](https://github.com/dashpay/dash/commit/1fa7f7e74) stop test failures
- [`444f671ab`](https://github.com/dashpay/dash/commit/444f671ab) Update src/miner.cpp
- [`514769940`](https://github.com/dashpay/dash/commit/514769940) fix 9868
- [`7ee31cbd6`](https://github.com/dashpay/dash/commit/7ee31cbd6) Speed up integration tests with masternodes (#2642)
- [`fda16f1fe`](https://github.com/dashpay/dash/commit/fda16f1fe) Fix off-by-1 in phase calculations and the rest of llmq-signing.py issues (#2641)
- [`b595f9e6a`](https://github.com/dashpay/dash/commit/b595f9e6a) Fix LLMQ signing integration tests (#2640)
- [`597748689`](https://github.com/dashpay/dash/commit/597748689) Bring back ResetLocalSessionStatus call (#2639)
- [`682a3b993`](https://github.com/dashpay/dash/commit/682a3b993) Merge #9904: test: Fail if InitBlockIndex fails
- [`55a656c24`](https://github.com/dashpay/dash/commit/55a656c24) Merge #9359: Add test for CWalletTx::GetImmatureCredit() returning stale values.
- [`68f6b43d1`](https://github.com/dashpay/dash/commit/68f6b43d1) fix #9143 backport
- [`bba55e262`](https://github.com/dashpay/dash/commit/bba55e262) Merge #9143: Refactor ZapWalletTxes to avoid layer violations
- [`02f4661b3`](https://github.com/dashpay/dash/commit/02f4661b3) fix #9894 backport
- [`07b50aefa`](https://github.com/dashpay/dash/commit/07b50aefa) Merge #9894: remove 'label' filter for rpc command help
- [`8035769d4`](https://github.com/dashpay/dash/commit/8035769d4) remove removed argument from #9834
- [`904e56fb1`](https://github.com/dashpay/dash/commit/904e56fb1) Merge #9834: qt: clean up initialize/shutdown signals
- [`2df84c6f1`](https://github.com/dashpay/dash/commit/2df84c6f1) fix merge error from #9821
- [`21e00e905`](https://github.com/dashpay/dash/commit/21e00e905) Merge #9821: util: Specific GetOSRandom for Linux/FreeBSD/OpenBSD
- [`f9c585776`](https://github.com/dashpay/dash/commit/f9c585776) manual fixes on #9868
- [`3ddf3dc62`](https://github.com/dashpay/dash/commit/3ddf3dc62) Merge #9868: Abstract out the command line options for block assembly
- [`397792355`](https://github.com/dashpay/dash/commit/397792355) Merge #9861: Trivial: Debug log ambiguity fix for peer addrs
- [`3d1a0b3ab`](https://github.com/dashpay/dash/commit/3d1a0b3ab) Merge #9871: Add a tree sha512 hash to merge commits
- [`8264e15cd`](https://github.com/dashpay/dash/commit/8264e15cd) Merge #9822: Remove block file location upgrade code
- [`f51d2e094`](https://github.com/dashpay/dash/commit/f51d2e094) Merge #9732: [Trivial] Remove nonsense #undef foreach
- [`3e10ff63f`](https://github.com/dashpay/dash/commit/3e10ff63f) Merge #9867: Replace remaining sprintf with snprintf
- [`0d38c16e7`](https://github.com/dashpay/dash/commit/0d38c16e7) Merge #9350: [Trivial] Adding label for amount inside of tx_valid/tx_invalid.json
- [`d2ddc2a00`](https://github.com/dashpay/dash/commit/d2ddc2a00) A couple of fixes/refactorings for CDKGSessionHandler (#2637)
- [`b2b97f258`](https://github.com/dashpay/dash/commit/b2b97f258) Fix some strings, docs and cmd-line/rpc help messages (#2632)
- [`e7981e468`](https://github.com/dashpay/dash/commit/e7981e468) Remove fLLMQAllowDummyCommitments from consensus params (#2636)
- [`8cd7287ba`](https://github.com/dashpay/dash/commit/8cd7287ba) Fix missing lupdate in depends (#2633)
- [`b0ad1425e`](https://github.com/dashpay/dash/commit/b0ad1425e) Review fixes (mostly if/else related but no change in logic)
- [`c905f1fe1`](https://github.com/dashpay/dash/commit/c905f1fe1) Initialize g_connman before initializing the LLMQ system
- [`b8d069bcd`](https://github.com/dashpay/dash/commit/b8d069bcd) fix/cleanup qt rpcnestedtests
- [`b970c20a9`](https://github.com/dashpay/dash/commit/b970c20a9) Avoid using immature coinbase UTXOs for dummy TXins
- [`4d25148c0`](https://github.com/dashpay/dash/commit/4d25148c0) Add llmq-signing.py tests
- [`d020ffa00`](https://github.com/dashpay/dash/commit/d020ffa00) Add wait_for_sporks_same and mine_quorum to DashTestFramework
- [`0cc1cf279`](https://github.com/dashpay/dash/commit/0cc1cf279) Add receivedFinalCommitment flag to CDKGDebugSessionStatus
- [`23d7ed80d`](https://github.com/dashpay/dash/commit/23d7ed80d) Implement "quorum sign/hasrecsig/isconflicting" RPCs
- [`316b6bf0d`](https://github.com/dashpay/dash/commit/316b6bf0d) Faster re-requesting of recovered sigs
- [`c38f889e7`](https://github.com/dashpay/dash/commit/c38f889e7) Implement processing, verifcation and propagation of signature shares
- [`43fd1b352`](https://github.com/dashpay/dash/commit/43fd1b352) Implement CSigningManager to process and propagage recovered signatures
- [`56ee83a76`](https://github.com/dashpay/dash/commit/56ee83a76) Add ReadDataStream to CDBWrapper to allow manual deserialization
- [`b6346a2f6`](https://github.com/dashpay/dash/commit/b6346a2f6) Implement CBLSInsecureBatchVerifier for convenient batch verification
- [`dd8f24588`](https://github.com/dashpay/dash/commit/dd8f24588) Implement IsBanned to allow checking for banned nodes outside of net_processing.cpp
- [`49de41726`](https://github.com/dashpay/dash/commit/49de41726) Implement CFixedVarIntsBitSet and CAutoBitSet
- [`76a58f5a4`](https://github.com/dashpay/dash/commit/76a58f5a4) Add `src/bls/*.h` and .cpp to CMakeLists.txt
- [`b627528ce`](https://github.com/dashpay/dash/commit/b627528ce) Use void as return type for WriteContributions
- [`edac100f5`](https://github.com/dashpay/dash/commit/edac100f5) Fix "quorum" RPCs help and unify logic in the sub-commands RPC entry point
- [`217f3941d`](https://github.com/dashpay/dash/commit/217f3941d) Skip starting of cache populator thread in case we don't have a valid vvec
- [`679a9895b`](https://github.com/dashpay/dash/commit/679a9895b) Add comments about why it's ok to ignore some failures
- [`15c34ccbd`](https://github.com/dashpay/dash/commit/15c34ccbd) Implement CQuorum and CQuorumManager
- [`8e4fe3660`](https://github.com/dashpay/dash/commit/8e4fe3660) [PrivateSend] Fallback to less participants if possible, fix too long timeouts on server side (#2616)
- [`ee808d819`](https://github.com/dashpay/dash/commit/ee808d819) Add checkbox to show only masternodes the wallet has keys for (#2627)
- [`fff50af3c`](https://github.com/dashpay/dash/commit/fff50af3c) Revert "Set CLIENT_VERSION_IS_RELEASE to true (#2591)"
- [`fed4716c0`](https://github.com/dashpay/dash/commit/fed4716c0) Remove support for "0" as an alternative to "" when the default is requested (#2622)
- [`8b7771a31`](https://github.com/dashpay/dash/commit/8b7771a31) Add some `const`s
- [`0b1347c0d`](https://github.com/dashpay/dash/commit/0b1347c0d) Pass self-created message to CDKGPendingMessages instead of processing them
- [`02c7932f4`](https://github.com/dashpay/dash/commit/02c7932f4) Add owner and voting addresses to rpc output, unify it across different methods (#2618)
- [`c948c0ff3`](https://github.com/dashpay/dash/commit/c948c0ff3) Fix help for optional parameters in "quorum dkgstatus"
- [`957652bf3`](https://github.com/dashpay/dash/commit/957652bf3) Fix help for "quorum dkgstatus" and remove support for "0" proTxHash
- [`b7b436b7d`](https://github.com/dashpay/dash/commit/b7b436b7d) Apply review suggestions to rpcquorums.cpp
- [`3fe991063`](https://github.com/dashpay/dash/commit/3fe991063) Drop unused overload of GetMasternodeQuorums
- [`5daeedabf`](https://github.com/dashpay/dash/commit/5daeedabf) Batched logger should not break log parsing
- [`2aed51c55`](https://github.com/dashpay/dash/commit/2aed51c55) Give nodes more time per phase when doing PoSe tests
- [`5958f8b81`](https://github.com/dashpay/dash/commit/5958f8b81) Remove dkgRndSleepTime from consensus params and make sleeping it non-random
- [`0dae46c2f`](https://github.com/dashpay/dash/commit/0dae46c2f) Move RandBool() into random.h/cpp
- [`e1901d24a`](https://github.com/dashpay/dash/commit/e1901d24a) Handle review suggestions
- [`352edbd33`](https://github.com/dashpay/dash/commit/352edbd33) Introduce SPORK_18_QUORUM_DEBUG_ENABLED to enable/disable LLMQ debug messages
- [`324406bfe`](https://github.com/dashpay/dash/commit/324406bfe) Implement debugging messages and RPC for LLMQ DKGs
- [`098b09495`](https://github.com/dashpay/dash/commit/098b09495) Pass scheduler to InitLLMQSystem
- [`a1f4853d6`](https://github.com/dashpay/dash/commit/a1f4853d6) Use LLMQ DKGs for PoSe testing in DIP3 tests
- [`6836f8c38`](https://github.com/dashpay/dash/commit/6836f8c38) Implement LLMQ DKG
- [`318b59813`](https://github.com/dashpay/dash/commit/318b59813) Prepare inter-quorum masternode connections
- [`4bf736f33`](https://github.com/dashpay/dash/commit/4bf736f33) Add cxxtimer header only libraries
- [`c6be8cfcd`](https://github.com/dashpay/dash/commit/c6be8cfcd) Allow to skip malleability check when deserializing BLS primitives
- [`9d25bb1d8`](https://github.com/dashpay/dash/commit/9d25bb1d8) Add batched logger
- [`0df3871d1`](https://github.com/dashpay/dash/commit/0df3871d1) Remove dummy DKG
- [`55f205eba`](https://github.com/dashpay/dash/commit/55f205eba) A couple of fixes for `masternode list` rpc (#2615)
- [`fa18d3e10`](https://github.com/dashpay/dash/commit/fa18d3e10) More fixes for PrivateSend after 2612 (#2614)
- [`bade33273`](https://github.com/dashpay/dash/commit/bade33273) Fix 2612 (#2613)
- [`5c5932eb9`](https://github.com/dashpay/dash/commit/5c5932eb9) [PrivateSend] Allow more than 3 mixing participants (#2612)
- [`0acfbf640`](https://github.com/dashpay/dash/commit/0acfbf640) Gracefully shutdown on evodb inconsistency instead of crashing (#2611)
- [`07dcddb4c`](https://github.com/dashpay/dash/commit/07dcddb4c) Backports 0.15 pr2 (#2597)
- [`04d1671b9`](https://github.com/dashpay/dash/commit/04d1671b9) armv7l build support (#2601)
- [`7d58d87f4`](https://github.com/dashpay/dash/commit/7d58d87f4) Remove a few sporks which are not used anymore (#2607)
- [`d1910eaff`](https://github.com/dashpay/dash/commit/d1910eaff) Refactor remains of CMasternode/-Man into CMasternodeMeta/-Man (#2606)
- [`cdc8ae943`](https://github.com/dashpay/dash/commit/cdc8ae943) Don't hold CDeterministicMNManager::cs while calling signals (#2608)
- [`968eb3fc5`](https://github.com/dashpay/dash/commit/968eb3fc5) Add real timestamp to log output when mock time is enabled (#2604)
- [`0648496e2`](https://github.com/dashpay/dash/commit/0648496e2) Fix flaky p2p-fullblocktest (#2605)
- [`96d4f7459`](https://github.com/dashpay/dash/commit/96d4f7459) Try to fix flaky IX tests in DIP3 tests (#2602)
- [`896733223`](https://github.com/dashpay/dash/commit/896733223) Dashify copyright_header.py (#2598)
- [`c58f775cc`](https://github.com/dashpay/dash/commit/c58f775cc) De-dashify env vars and dashify help text in tests instead (#2603)
- [`a49f4123e`](https://github.com/dashpay/dash/commit/a49f4123e) Backports 0.15 pr1 (#2590)
- [`f95aae2b3`](https://github.com/dashpay/dash/commit/f95aae2b3) Remove all legacy/compatibility MN code (#2600)
- [`0e28f0aa9`](https://github.com/dashpay/dash/commit/0e28f0aa9) Fix flaky autoix tests by disabling autoix while filling mempool (#2595)
- [`78c22ad0f`](https://github.com/dashpay/dash/commit/78c22ad0f) Multiple fixes for "masternode list"
- [`e54f6b274`](https://github.com/dashpay/dash/commit/e54f6b274) Use ban score of 10 for invalid DSQ sigs
- [`536229d17`](https://github.com/dashpay/dash/commit/536229d17) Apply suggestions from code review
- [`75024e117`](https://github.com/dashpay/dash/commit/75024e117) Merge #10365: [tests] increase timeouts in sendheaders test
- [`1efd77358`](https://github.com/dashpay/dash/commit/1efd77358) Remove non-DIP3 code path in CMasternodePayments::IsScheduled
- [`4c749b7e9`](https://github.com/dashpay/dash/commit/4c749b7e9) Directly use deterministicMNManager in "masternode list"
- [`0fe97a045`](https://github.com/dashpay/dash/commit/0fe97a045) Remove support for "masternode list rank"
- [`adc2ec225`](https://github.com/dashpay/dash/commit/adc2ec225) Remove unsupported types/fields from "masternode list"
- [`4b150e72f`](https://github.com/dashpay/dash/commit/4b150e72f) Directly use deterministicMNManager instead of mnodeman.CountXXX
- [`4c3bb7304`](https://github.com/dashpay/dash/commit/4c3bb7304) Remove call to mnodeman.PoSeBan
- [`0594cd719`](https://github.com/dashpay/dash/commit/0594cd719) Remove code that is incompatible now due to GetMasternodeRanks returning DMNs now
- [`37541ee00`](https://github.com/dashpay/dash/commit/37541ee00) Change GetMasternodeScores and GetMasternodeRank/s to use CDeterministicMNCPtr
- [`17c792cd3`](https://github.com/dashpay/dash/commit/17c792cd3) Remove MN upgrade check in ComputeBlockVersion
- [`71a695100`](https://github.com/dashpay/dash/commit/71a695100) Move logic from FindRandomNotInVec into GetRandomNotUsedMasternode
- [`2f66d6ada`](https://github.com/dashpay/dash/commit/2f66d6ada) Replace uses of mnodeman in PS code when deterministicMNManager can be used directly
- [`eedb15845`](https://github.com/dashpay/dash/commit/eedb15845) Remove use of mnodeman.GetMasternodeInfo from IX code
- [`fb13b000b`](https://github.com/dashpay/dash/commit/fb13b000b) Remove support for legacy operator keys in CPrivateSendBroadcastTx
- [`5f5fcc49c`](https://github.com/dashpay/dash/commit/5f5fcc49c) Remove legacy signatures support in CPrivateSendQueue
- [`da924519a`](https://github.com/dashpay/dash/commit/da924519a) Remove support for legacy signatures in CTxLockVote
- [`2b2e4f45d`](https://github.com/dashpay/dash/commit/2b2e4f45d) Remove a few uses of mnodeman from governance code
- [`14d8ce03c`](https://github.com/dashpay/dash/commit/14d8ce03c) Don't use GetMasternodeInfo in CTxLockVote::IsValid
- [`1ff241881`](https://github.com/dashpay/dash/commit/1ff241881) Directly use deterministicMNManager in some places
- [`45f34e130`](https://github.com/dashpay/dash/commit/45f34e130) Implement HasValidMN, HasValidMNByCollateral and GetValidMNByCollateral
- [`bc29fe160`](https://github.com/dashpay/dash/commit/bc29fe160) Remove compatibility code from governance RPCs and directly use deterministicMNManager
- [`b49ef5d71`](https://github.com/dashpay/dash/commit/b49ef5d71) Directly use deterministicMNManager when processing DSTX
- [`ae229e283`](https://github.com/dashpay/dash/commit/ae229e283) Directly use deterministicMNManager in CGovernanceManager::GetCurrentVotes
- [`96e0385db`](https://github.com/dashpay/dash/commit/96e0385db) Let "masternode winner/current" directly use deterministicMNManager
- [`82745dd07`](https://github.com/dashpay/dash/commit/82745dd07) Use DIP3 MNs in auto-IX tests (#2588)
- [`ce5a51134`](https://github.com/dashpay/dash/commit/ce5a51134) Speed up stopping of nodes in tests by splitting up stop and wait (#2587)
- [`0c9fb6968`](https://github.com/dashpay/dash/commit/0c9fb6968) Harden spork15 on testnet (#2586)
- [`361900e46`](https://github.com/dashpay/dash/commit/361900e46) Bump version to 0.14 (#2589)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Alexander Block (codablock)
- Amir Abrams (AmirAbrams)
- Daniel Hoffmann (dhoffmann)
- Duke Leto (leto)
- fish-en
- gladcow
- -k (charlesrocket)
- Nathan Marley (nmarley)
- PastaPastaPasta
- Peter Bushnell (Bushstar)
- strophy
- thephez
- UdjinM6

As well as everyone that submitted issues and reviewed pull requests.

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

- [v0.13.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.13.3.md) released Apr/04/2019
- [v0.13.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.13.2.md) released Mar/15/2019
- [v0.13.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.13.1.md) released Feb/9/2019
- [v0.13.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.13.0.md) released Jan/14/2019
- [v0.12.3.4](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.4.md) released Dec/14/2018
- [v0.12.3.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.3.md) released Sep/19/2018
- [v0.12.3.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.2.md) released Jul/09/2018
- [v0.12.3.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.1.md) released Jul/03/2018
- [v0.12.2.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.2.3.md) released Jan/12/2018
- [v0.12.2.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.2.2.md) released Dec/17/2017
- [v0.12.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.2.md) released Nov/08/2017
- [v0.12.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.1.md) released Feb/06/2017
- [v0.12.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.0.md) released Aug/15/2015
- [v0.11.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.2.md) released Mar/04/2015
- [v0.11.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.1.md) released Feb/10/2015
- [v0.11.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.0.md) released Jan/15/2015
- [v0.10.x](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.10.0.md) released Sep/25/2014
- [v0.9.x](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.9.0.md) released Mar/13/2014

