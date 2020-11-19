Dash Core version 0.15
======================

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

When upgrading from a version prior to 0.14.0.3, the
first startup of Dash Core will run a migration process which can take a few minutes
to finish. After the migration, a downgrade to an older version is only possible with
a reindex (or reindex-chainstate).

Downgrade warning
-----------------

### Downgrade to a version < 0.14.0.3

Downgrading to a version smaller than 0.14.0.3 is not supported anymore due to changes
in the "evodb" database format. If you need to use an older version, you have to perform
a reindex or re-sync the whole chain.

Notable changes
===============

Removal of the p2p alert system
-------------------------------
The p2p alert system was designed to send messages to all nodes supporting it by someone who holds
so called alert keys to notify such nodes in case of severe network issues. This version removes
the `alert` p2p message and `--alert` option. Internal alerts, partition detection warnings and the
`--alertnotify` option features remain.

Removal of the legacy InstantSend system
----------------------------------------
Version 0.14 introduced the new LLMQ-based InstantSend system which is designed to be much more scalable
than the legacy one without sacrificing security. The new system also allows all transactions to be treated as
InstantSend transactions. The legacy system was disabled together with the successful deployment of ChainLocks,
but we had to keep supporting the legacy system for a while to ensure a smooth transition period. This version finally drops
the legacy system completely.

Read more about ChainLocks: https://github.com/dashpay/dips/blob/master/dip-0008.md
Read more about LLMQ-based InstantSend: https://github.com/dashpay/dips/blob/master/dip-0010.md

Sporks
------
The security level of ChainLocks and LLMQ-based InstantSend made sporks `SPORK_5_INSTANTSEND_MAX_VALUE` and
`SPORK_12_RECONSIDER_BLOCKS` obsolete, so they are removed now. Sporks `SPORK_15_DETERMINISTIC_MNS_ENABLED`,
`SPORK_16_INSTANTSEND_AUTOLOCKS` and `SPORK_20_INSTANTSEND_LLMQ_BASED` have no code logic behind them anymore
because they were used as part of the DIP0003, DIP0008 and DIP0010 activation process which is finished now.
They are still kept and relayed only to ensure smooth operation of v0.14 clients and will be removed in some
future version.

Mempool sync improvements
-------------------------
Nodes joining the network will now try to sync their mempool from other v0.15+ peers via the `mempool` p2p message.
This behaviour can be disabled via the new `--syncmempool` option. Nodes serving such requests will now also push
`inv` p2p messages for InstandSend locks which are held for transactions in their mempool. These two changes
should help new nodes to quickly catchup on start and detect any potential double-spend as soon as possible.
This should also help wallets to slightly improve UX by showing the correct status of unconfirmed transactions
locked via InstandSend, if they were sent while the receiving wallet was offline. Note that bloom-filters
still apply to such `inv` messages, just like they do for transactions and locks that are relayed on a
regular basis.

PrivateSend improvements
------------------------
This version decouples the so called "Lite Mode" and client-side PrivateSend mixing, which allows client-side mixing
on pruned nodes running with `--litemode` option. Such nodes will have to also specify the newly redefined
`--enableprivatesend` option. Non-prunned nodes do not have to do this but they can use `--enableprivatesend`
option to disable mixing completely instead. Please note that specifying this option does not start mixing
automatically anymore (which was the case in previous versions). To automatically start mixing, use the new
`--privatesendautostart` option in addition to `--enableprivatesend`. Additionally, PrivateSend can always be
controlled with the `privatesend` RPC.

Thanks to LLMQ-based InstantSend and its ability to lock chains of unconfirmed transactions (and not only a single
one like in the legacy system), PrivateSend mixing speed has improved significantly. In such an environment
Liquidity Provider Mode, which was introduced a long time ago to support mixing volume, is no longer needed and
is removed now. As such the `--liquidityprovider` option is not available anymore.

Some other improvements were also introduced to speed up mixing, e.g. by joining more queues or dropping potential
malicious mixing participants faster by checking some rules earlier etc. Lots of related code was refactored to
further improve its readability, which should make it easier for someone to re-implement PrivateSend
correctly in other wallets if there is a desire to do so.

Wallet changes
--------------
Wallet internals were optimized to significantly improve performance which should be especially notable for huge
wallets with tens of thousands of transactions or more. The GUI for such wallets should be much more responsive too now.

Running Masternodes from local wallets was deprecated a long time ago and starting from this version we disable
wallet functionality on Masternodes completely.

GUI changes
-----------
The Qt GUI went through a refresh to follow branding color guides and to make it feel lighter. All old themes besides
the Traditional one (the one with a minimal styling) were removed and instead a new Dark theme was added.

In this version we made a lot of optimizations to remove various lags and lockups, the GUI in general should feel
much more smoother now, especially for huge wallets or when navigating through the masternode list. The latter has
a few new columns (collateral, owner and voting addresses) which give more options to filter and/or sort the list.
All issues with hi-dpi monitors should also be fixed now.

The "Send" popup dialog was slightly tweaked to improve the language and provide a bit more information about inputs
included in the transaction, its size and the actual resulting fee rate. It will also show the number of inputs
a PrivateSend transaction is going to consume and display a warning regarding sender privacy if this number is 10
or higher.

Changes in regtest and devnet p2p/rpc ports
-------------------------------------------
Default p2p and rpc ports for devnets and regtest were changed to ensure their consistency and to avoid
any potential interference with bitcoin's regtests. New default p2p/rpc ports for devnet are 19799/19798,
for regtest - 19899/19898 respectively.

ZMQ changes
-----------
Added two new messages `rawchainlocksig` and `rawtxlocksig` which return the raw data of the block/transaction
concatenated with the corresponding `clsig`/`islock` message respectively.

Crash reports and stack traces
------------------------------
Binaries built with Gitian (including all official releases) will from now on always have crash reports and
crash hooks enabled. This means, that all binaries will print some information about the stack trace at the
time of a crash. If no debug information is present at crash time (which is usually the case), the binaries
will print a line that looks like this:
```
2020-01-06 14:41:08 Windows Exception: EXCEPTION_ACCESS_VIOLATION
No debug information available for stacktrace. You should add debug information and then run:
dashd.exe -printcrashinfo=bvcgc43iinzgc43ijfxgm3yba....
```
If you encounter such a crash, include these lines when you report the crash and we will be able to debug it
further. Anyone interested in running the specified `-printcrashinfo` command can do so after copying the debug info
file from the Gitian build to the same place where the binary is located. This will then print a detailed stack
trace.

RPC changes
-----------
There are a few changes in existing RPC interfaces in this release:
- no more `instantsend` field in various RPC commands
- `use-IS`, `use_is` and `instantsend` options are deprecated in various RPC commands and have no effect anymore
- added new `merkleRootQuorums` field in `getblock` RPC results
- individual Dash-specific fields which were used to display soft-fork progress in `getblockchaininfo` are replaced
 with the backported `statistics` object
- `privatesend_balance` field is shown in all related RPC results regardless of the Lite Mode or PrivateSend state
- added `pubKeyOperator` field for each masternode in `quorum info` RPC response

There are also new RPC commands:
- `getbestchainlock`
- `getmerkleblocks`
- `getprivatesendinfo`

`getpoolinfo` was deprecated in favor of `getprivatesendinfo` and no longer returns any data.

There are also new RPC commands backported from Bitcoin Core 0.15:
- `abortrescan`
- `combinerawtransaction`
- `getblockstats`
- `getchaintxstats`
- `listwallets`
- `logging`
- `uptime`

Make sure to check Bitcoin Core 0.15 release notes in a [section](#backports-from-bitcoin-core-015) below
for more RPC changes.

See `help command` in rpc for more info.

Command-line options
--------------------
Changes in existing cmd-line options:
- `--enableprivatesend` option has a new meaning now, see [PrivateSend](#privatesend) section for more info

New cmd-line options:
- `--printcrashinfo`
- `--syncmempool`
- `--privatesendautostart`

Few cmd-line options are no longer supported:
- `--alerts`
- `--masternode`, deprecated, specifying `--masternodeblsprivkey` option alone is enough to enable masternode mode now
- `--liquidityprovider`
- `--enableinstantsend`, dropped due to removal of the Legacy InstantSend

Make sure to check Bitcoin Core 0.15 release notes in a [section](#backports-from-bitcoin-core-015) below
for more changes in command-line options.

See `Help -> Command-line options` in Qt wallet or `dashd --help` for more info.

Build system
------------
This version always includes stacktraces in binaries now, `--enable-stacktraces` option is no longer available.
Instead you can choose if you want to hook crash reporting into various types of crashes by using `--enable-crash-hooks`
option (default is `no`). When using this option on macOS make sure to build binaries with `make -C src osx_debug`.

Backports from Bitcoin Core 0.15
--------------------------------

Most of the changes between Bitcoin Core 0.14 and Bitcoin Core 0.15 have been backported into Dash Core 0.15.
We only excluded backports which do not align with Dash, like SegWit or RBF related changes.

You can read about changes brought by backporting from Bitcoin Core 0.15 in following docs:
- https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.15.0.md
- https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.15.1.md

Some other individual PRs were backported from versions 0.16+, you can find the full list of backported PRs
and additional fixes in https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.15-backports.md

Miscellaneous
-------------
A lot of refactoring, backports, code cleanups and other small fixes were done in this release. Dash-specific
modules were reorganized in separate folders to make navigation through code a bit easier.

0.15 Change log
===============

See detailed [set of changes](https://github.com/dashpay/dash/compare/v0.14.0.5...dashpay:v0.15.0.0).

- [`3c055bf79`](https://github.com/dashpay/dash/commit/3c055bf79) Bump nMinimumChainWork and defaultAssumeValid (#3336)
- [`818e7a6f7`](https://github.com/dashpay/dash/commit/818e7a6f7) Update release notes
- [`9d5c3d12e`](https://github.com/dashpay/dash/commit/9d5c3d12e) Try to actually accept newly created dstx-es into masternode's mempool (#3332)
- [`f23e722da`](https://github.com/dashpay/dash/commit/f23e722da) Switch CLIENT_VERSION_IS_RELEASE to `true` for v0.15 (#3306)
- [`b57f1dac8`](https://github.com/dashpay/dash/commit/b57f1dac8) Update release notes
- [`15c6df583`](https://github.com/dashpay/dash/commit/15c6df583) Bring back "about" menu icon (#3329)
- [`2c30818f7`](https://github.com/dashpay/dash/commit/2c30818f7) Add pubKeyOperator to `quorum info` rpc response (#3327)
- [`2bbf78c1b`](https://github.com/dashpay/dash/commit/2bbf78c1b) Update release-notes.md
- [`2c305d02d`](https://github.com/dashpay/dash/commit/2c305d02d) Update translations 2020-02-03 (#3322)
- [`672e18e48`](https://github.com/dashpay/dash/commit/672e18e48) Only sync mempool from v0.15+ (proto 70216+) nodes (#3321)
- [`829bde81e`](https://github.com/dashpay/dash/commit/829bde81e) Fix dark text on dark background in combobox dropdowns on windows (#3315)
- [`c0a671e84`](https://github.com/dashpay/dash/commit/c0a671e84) Fix node protection logic false positives (#3314)
- [`8d5fc6e0a`](https://github.com/dashpay/dash/commit/8d5fc6e0a) Merge #13162: [net] Don't incorrectly log that REJECT messages are unknown.
- [`9e711befd`](https://github.com/dashpay/dash/commit/9e711befd) More of 13946
- [`e5e3572e9`](https://github.com/dashpay/dash/commit/e5e3572e9) Merge #13946: p2p: Clarify control flow in ProcessMessage
- [`dbbc51121`](https://github.com/dashpay/dash/commit/dbbc51121) Add `automake` package to dash-win-signer's packages list (#3307)
- [`fd0f24335`](https://github.com/dashpay/dash/commit/fd0f24335) [Trivial] Release note update (#3308)
- [`058872d4f`](https://github.com/dashpay/dash/commit/058872d4f) Update release-notes.md
- [`546e69f1a`](https://github.com/dashpay/dash/commit/546e69f1a) Fix CActiveMasternodeManager::GetLocalAddress to prefer IPv4 if multiple local addresses are known (#3304)
- [`e4ef7e8d0`](https://github.com/dashpay/dash/commit/e4ef7e8d0) Drop unused `invSet` in `CDKGSession` (#3303)
- [`da7686c93`](https://github.com/dashpay/dash/commit/da7686c93) Update translations 2020-01-23 (#3302)
- [`6b5d3edae`](https://github.com/dashpay/dash/commit/6b5d3edae) Fix dip4-coinbasemerkleroots.py race condition (#3297)
- [`a8213cadb`](https://github.com/dashpay/dash/commit/a8213cadb) Various fixes for DSTX-es (#3295)
- [`2c26bdf2d`](https://github.com/dashpay/dash/commit/2c26bdf2d) Update release-notes.md
- [`1d9adbe63`](https://github.com/dashpay/dash/commit/1d9adbe63) Replace generic CScopedDBTransaction with specialized CEvoDBScopedCommitter (#3292)
- [`8fd486c6b`](https://github.com/dashpay/dash/commit/8fd486c6b) Translations 2020-01 (#3192)
- [`3c54f6527`](https://github.com/dashpay/dash/commit/3c54f6527) Bump copyright year to 2020 (#3290)
- [`35d28c748`](https://github.com/dashpay/dash/commit/35d28c748) Update man pages (#3291)
- [`203cc9077`](https://github.com/dashpay/dash/commit/203cc9077)  trivial: adding SVG and high contrast icons  (#3209)
- [`e875d4925`](https://github.com/dashpay/dash/commit/e875d4925) Define defaultTheme and darkThemePrefix as constants and use them instead of plain strings (#3288)
- [`1d203b422`](https://github.com/dashpay/dash/commit/1d203b422) Bump PROTOCOL_VERSION to 70216 (#3287)
- [`b84482ac5`](https://github.com/dashpay/dash/commit/b84482ac5) Let regtest have its own qt settings (#3286)
- [`1c885bbed`](https://github.com/dashpay/dash/commit/1c885bbed) Only load valid themes, fallback to "Light" theme otherwise (#3285)
- [`ce924278d`](https://github.com/dashpay/dash/commit/ce924278d) Don't load caches when blocks/chainstate was deleted and also delete old caches (#3280)
- [`ebf529e8a`](https://github.com/dashpay/dash/commit/ebf529e8a) Drop new connection instead of old one when duplicate MNAUTH is received (#3272)
- [`817cd9a17`](https://github.com/dashpay/dash/commit/817cd9a17) AppInitMain should quit early and return `false` if shutdown was requested at some point (#3267)
- [`42e104932`](https://github.com/dashpay/dash/commit/42e104932) Tweak few more strings re mixing and balances (#3265)
- [`d30eeb6f8`](https://github.com/dashpay/dash/commit/d30eeb6f8) Use -Wno-psabi for arm builds on Travis/Gitlab (#3264)
- [`1df3c67a8`](https://github.com/dashpay/dash/commit/1df3c67a8) A few fixes for integration tests (#3263)
- [`6e50a7b2a`](https://github.com/dashpay/dash/commit/6e50a7b2a) Fix params.size() check in "protx list wallet" RPC (#3259)
- [`1a1cec224`](https://github.com/dashpay/dash/commit/1a1cec224) Fix pull request detection in .gitlab-ci.yml (#3256)
- [`31afa9c0f`](https://github.com/dashpay/dash/commit/31afa9c0f) Don't disconnect masternode connections when we have less then the desired amount of outbound nodes (#3255)
- [`cecbbab3c`](https://github.com/dashpay/dash/commit/cecbbab3c) move privatesend rpc methods from rpc/masternode.cpp to new rpc/privatesend.cpp (#3253)
- [`8e054f374`](https://github.com/dashpay/dash/commit/8e054f374) Sync mempool from other nodes on start (#3251)
- [`474f25b8d`](https://github.com/dashpay/dash/commit/474f25b8d) Push islock invs when syncing mempool (#3250)
- [`3b0f8ff8b`](https://github.com/dashpay/dash/commit/3b0f8ff8b) Skip mnsync restrictions for whitelisted and manually added nodes (#3249)
- [`fd94e9c38`](https://github.com/dashpay/dash/commit/fd94e9c38) Streamline, refactor and unify PS checks for mixing entries and final txes (#3246)
- [`c42b20097`](https://github.com/dashpay/dash/commit/c42b20097) Try to avoid being marked as a bad quorum member when we sleep for too long in SleepBeforePhase (#3245)
- [`db6ea1de8`](https://github.com/dashpay/dash/commit/db6ea1de8) Fix log output in CDKGPendingMessages::PushPendingMessage (#3244)
- [`416d85b29`](https://github.com/dashpay/dash/commit/416d85b29) Tolerate parent cache with empty cache-artifact directory (#3240)
- [`0c9c27c6f`](https://github.com/dashpay/dash/commit/0c9c27c6f) Add ccache to gitian packages lists (#3237)
- [`65206833e`](https://github.com/dashpay/dash/commit/65206833e) Fix menu bar text color in Dark theme (#3236)
- [`7331d4edd`](https://github.com/dashpay/dash/commit/7331d4edd) Bump wait_for_chainlocked_block_all_nodes timeout in llmq-is-retroactive.py to 30 sec when mining lots of blocks at once (#3238)
- [`dad102669`](https://github.com/dashpay/dash/commit/dad102669) Update static and dns seeds for mainnet and testnet (#3234)
- [`9378c271b`](https://github.com/dashpay/dash/commit/9378c271b) Modify makesseeds.py to work with "protx list valid 1" instead of "masternode list (#3235)
- [`91a996e32`](https://github.com/dashpay/dash/commit/91a996e32) Make sure mempool txes are properly processed by CChainLocksHandler despite node restarts (#3226)
- [`2b587f0eb`](https://github.com/dashpay/dash/commit/2b587f0eb) Slightly refactor CDKGSessionHandler::SleepBeforePhase (#3224)
- [`fdb05860e`](https://github.com/dashpay/dash/commit/fdb05860e) Don't join thread in CQuorum::~CQuorum when called from within the thread (#3223)
- [`4c00d98ea`](https://github.com/dashpay/dash/commit/4c00d98ea) Allow re-signing of IS locks when performing retroactive signing (#3219)
- [`b4b9d3467`](https://github.com/dashpay/dash/commit/b4b9d3467) Tests: Fix the way nodes are connected to each other in setup_network/start_masternodes (#3221)
- [`dfe99c950`](https://github.com/dashpay/dash/commit/dfe99c950) Decouple cs_mnauth/cs_main (#3220)
- [`08f447af9`](https://github.com/dashpay/dash/commit/08f447af9) Tests: Allow specifying different cmd-line params for each masternode (#3222)
- [`9dad60386`](https://github.com/dashpay/dash/commit/9dad60386) Tweak "Send" popup and refactor related code a bit (#3218)
- [`bb7a32d2e`](https://github.com/dashpay/dash/commit/bb7a32d2e) Add Dark theme (#3216)
- [`05ac4dbb4`](https://github.com/dashpay/dash/commit/05ac4dbb4) Dashify few strings (#3214)
- [`482a549a2`](https://github.com/dashpay/dash/commit/482a549a2) Add collateral, owner and voting addresses to masternode list table (#3207)
- [`37f96f5a3`](https://github.com/dashpay/dash/commit/37f96f5a3) Bump version to 0.15 and update few const-s/chainparams (#3204)
- [`9de994988`](https://github.com/dashpay/dash/commit/9de994988) Compliance changes to terminology (#3211)
- [`d475f17bc`](https://github.com/dashpay/dash/commit/d475f17bc) Fix styles for progress dialogs, shutdown window and text selection (#3212)
- [`df372ec5f`](https://github.com/dashpay/dash/commit/df372ec5f) Fix off-by-one error for coinbase txes confirmation icons (#3206)
- [`1e94e3333`](https://github.com/dashpay/dash/commit/1e94e3333) Fix styling for disabled buttons (#3205)
- [`7677b5578`](https://github.com/dashpay/dash/commit/7677b5578) Actually apply CSS styling to RPC console (#3201)
- [`63cc22d5e`](https://github.com/dashpay/dash/commit/63cc22d5e) More Qt tweaks (#3200)
- [`7aa9c43f8`](https://github.com/dashpay/dash/commit/7aa9c43f8) Few Qt tweaks (#3199)
- [`fd50c1c71`](https://github.com/dashpay/dash/commit/fd50c1c71) Hold cs_main/cs_wallet in main MakeCollateralAmounts (#3197)
- [`460e0f475`](https://github.com/dashpay/dash/commit/460e0f475) Fix locking of funds for mixing (#3194)
- [`415b81e41`](https://github.com/dashpay/dash/commit/415b81e41) Refactor some pow functions (#3198)
- [`b2fed3862`](https://github.com/dashpay/dash/commit/b2fed3862) A few trivial fixes for RPCs (#3196)
- [`f8296364a`](https://github.com/dashpay/dash/commit/f8296364a) Two trivial fixes for logs (#3195)
- [`d5cc88f00`](https://github.com/dashpay/dash/commit/d5cc88f00) Should mark tx as a PS one regardless of change calculations in CreateTransaction (#3193)
- [`e9235b9bb`](https://github.com/dashpay/dash/commit/e9235b9bb) trivial: Rename txid paramater for gobject voteraw (#3191)
- [`70b320bab`](https://github.com/dashpay/dash/commit/70b320bab) Detect masternode mode from masternodeblsprivkey arg (#3188)
- [`1091ab3c6`](https://github.com/dashpay/dash/commit/1091ab3c6) Translations201909 (#3107)
- [`251fb5e69`](https://github.com/dashpay/dash/commit/251fb5e69) Slightly optimize ApproximateBestSubset and its usage for PS txes (#3184)
- [`a55624b25`](https://github.com/dashpay/dash/commit/a55624b25) Fix 3182: Append scrollbar styles (#3186)
- [`1bbe1adb4`](https://github.com/dashpay/dash/commit/1bbe1adb4) Add a simple test for payoutAddress reuse in `protx update_registrar` (#3183)
- [`372389d23`](https://github.com/dashpay/dash/commit/372389d23) Disable styling for scrollbars on macos (#3182)
- [`e0781095f`](https://github.com/dashpay/dash/commit/e0781095f) A couple of fixes for additional indexes (#3181)
- [`d3ce0964b`](https://github.com/dashpay/dash/commit/d3ce0964b) Add Qt GUI refresh w/branding updates (#3000)
- [`9bc699ff2`](https://github.com/dashpay/dash/commit/9bc699ff2) Update activemn if protx info changed (#3176)
- [`bbd9b10d4`](https://github.com/dashpay/dash/commit/bbd9b10d4) Refactor nonLockedTxsByInputs (#3178)
- [`64a913d6f`](https://github.com/dashpay/dash/commit/64a913d6f) Allow empty strings in `protx update_registrar` as an option to re-use current values (#3177)
- [`3c21d2577`](https://github.com/dashpay/dash/commit/3c21d2577) Slightly adjust some README.md files (#3175)
- [`883fcbe8b`](https://github.com/dashpay/dash/commit/883fcbe8b) Always run extended tests in Gitlab CI (#3173)
- [`a7492c1d3`](https://github.com/dashpay/dash/commit/a7492c1d3) Handle coin type via CCoinControl (#3172)
- [`0d1a04905`](https://github.com/dashpay/dash/commit/0d1a04905) Don't show individual messages for each TX when too many come in at once (#3170)
- [`589c89250`](https://github.com/dashpay/dash/commit/589c89250) Fix 2 more bottlenecks causing GUI lockups (#3169)
- [`dfd6ee472`](https://github.com/dashpay/dash/commit/dfd6ee472) Actually update spent index on DisconnectBlock (#3167)
- [`3c818e95b`](https://github.com/dashpay/dash/commit/3c818e95b) Only track last seen time instead of first and last seen time (#3165)
- [`df3dbe85b`](https://github.com/dashpay/dash/commit/df3dbe85b) Wait for sporks to propagate in llmq-chainlocks.py before mining new blocks (#3168)
- [`8cbd63d9e`](https://github.com/dashpay/dash/commit/8cbd63d9e) Make HD wallet warning a bit more natural (#3164)
- [`001c4338b`](https://github.com/dashpay/dash/commit/001c4338b) Improved messaging for ip address errors (#3163)
- [`33d04ebf2`](https://github.com/dashpay/dash/commit/33d04ebf2) Disable move ctor/operator for CKeyHolder (#3162)
- [`da2f503a4`](https://github.com/dashpay/dash/commit/da2f503a4) Fix largest part of GUI lockups with large wallets (#3155)
- [`3c6b5f98e`](https://github.com/dashpay/dash/commit/3c6b5f98e) Use wallet UTXOs whenever possible to avoid looping through all wallet txes (#3156)
- [`4db91c605`](https://github.com/dashpay/dash/commit/4db91c605) Fix Gitlab cache issues (#3160)
- [`e9ed35482`](https://github.com/dashpay/dash/commit/e9ed35482) Partially revert 3061 (#3150)
- [`4b6af8f2c`](https://github.com/dashpay/dash/commit/4b6af8f2c) Few fixes related to SelectCoinsGroupedByAddresses (#3144)
- [`859d60f81`](https://github.com/dashpay/dash/commit/859d60f81) Don't use $CACHE_DIR in after_script (#3159)
- [`be127bc2e`](https://github.com/dashpay/dash/commit/be127bc2e) Replace vecAskFor with a priority queue (#3147)
- [`a13a9182d`](https://github.com/dashpay/dash/commit/a13a9182d) Add missing "notfound" and "getsporks" to messagemap (#3146)
- [`efd8d2c82`](https://github.com/dashpay/dash/commit/efd8d2c82) Avoid propagating InstantSend related old recovered sigs (#3145)
- [`24fee3051`](https://github.com/dashpay/dash/commit/24fee3051) Add support for Gitlab CI (#3149)
- [`1cbe280ad`](https://github.com/dashpay/dash/commit/1cbe280ad) Qt: Remove old themes (#3141)
- [`dcdf1f3a6`](https://github.com/dashpay/dash/commit/dcdf1f3a6) Some refactoring for spork related functionality in tests (#3137)
- [`411241471`](https://github.com/dashpay/dash/commit/411241471) Introduce getprivatesendinfo and deprecate getpoolinfo (#3140)
- [`152c10bc4`](https://github.com/dashpay/dash/commit/152c10bc4) Various fixes for mixing queues (#3138)
- [`e0c56246f`](https://github.com/dashpay/dash/commit/e0c56246f) Fixes and refactorings related to using mnsync in tests (#3136)
- [`f8e238c5b`](https://github.com/dashpay/dash/commit/f8e238c5b) [Trivial] RPC help updates (#3134)
- [`d49ee618f`](https://github.com/dashpay/dash/commit/d49ee618f) Add more logging to DashTestFramework (#3130)
- [`cd6c5b4b4`](https://github.com/dashpay/dash/commit/cd6c5b4b4) Multiple fixes for ChainLock tests (#3129)
- [`e06c116d2`](https://github.com/dashpay/dash/commit/e06c116d2) Actually pass extra_args to nodes in assumevalid.py (#3131)
- [`737ac967f`](https://github.com/dashpay/dash/commit/737ac967f) Refactor some Dash-specific `wait_for*` functions in tests (#3122)
- [`b4aefb513`](https://github.com/dashpay/dash/commit/b4aefb513) Also consider txindex for transactions in AlreadyHave() (#3126)
- [`d9e98e31e`](https://github.com/dashpay/dash/commit/d9e98e31e) Fix scripted diff check condition (#3128)
- [`bad3243b8`](https://github.com/dashpay/dash/commit/bad3243b8) Bump mocktime before generating new blocks and generate a few blocks at the end of `test_mempool_doublespend` in `p2p-instantsend.py` (#3125)
- [`82ebba18f`](https://github.com/dashpay/dash/commit/82ebba18f) Few fixes for `wait_for_instantlock` (#3123)
- [`a2fa9bb7e`](https://github.com/dashpay/dash/commit/a2fa9bb7e) Ignore recent rejects filter for locked txes (#3124)
- [`2ca2138fc`](https://github.com/dashpay/dash/commit/2ca2138fc) Whitelist nodes in llmq-dkgerrors.py (#3112)
- [`a8fa5cff9`](https://github.com/dashpay/dash/commit/a8fa5cff9) Make orphan TX map limiting dependent on total TX size instead of TX count (#3121)
- [`746b5f8cd`](https://github.com/dashpay/dash/commit/746b5f8cd) Remove commented out code (#3117)
- [`3ac583cce`](https://github.com/dashpay/dash/commit/3ac583cce) docs: Add packages for building in Alpine Linux (#3115)
- [`c5da93851`](https://github.com/dashpay/dash/commit/c5da93851) A couple of minor improvements in IS code (#3114)
- [`43b7c31d9`](https://github.com/dashpay/dash/commit/43b7c31d9) Wait for the actual best block chainlock in llmq-chainlocks.py (#3109)
- [`22ac6ba4e`](https://github.com/dashpay/dash/commit/22ac6ba4e) Make sure chainlocks and blocks are propagated in llmq-is-cl-conflicts.py before moving to next steps (#3108)
- [`9f1ee8c70`](https://github.com/dashpay/dash/commit/9f1ee8c70) scripted-diff: Refactor llmq type consensus param names (#3093)
- [`1c74b668b`](https://github.com/dashpay/dash/commit/1c74b668b) Introduce getbestchainlock rpc and fix llmq-is-cl-conflicts.py (#3094)
- [`ac0270871`](https://github.com/dashpay/dash/commit/ac0270871) Respect `logips` config option in few more log outputs (#3078)
- [`d26b6a84c`](https://github.com/dashpay/dash/commit/d26b6a84c) Fix a couple of issues with PS fee calculations (#3077)
- [`40399fd97`](https://github.com/dashpay/dash/commit/40399fd97) Circumvent BIP69 sorting in fundrawtransaction.py test (#3100)
- [`e2d651f60`](https://github.com/dashpay/dash/commit/e2d651f60) Add OpenSSL termios fix for musl libc (#3099)
- [`783653f6a`](https://github.com/dashpay/dash/commit/783653f6a) Ensure execinfo.h and linker flags set in autoconf (#3098)
- [`7320c3da2`](https://github.com/dashpay/dash/commit/7320c3da2) Refresh zmq 4.1.5 patches (#3092)
- [`822e617be`](https://github.com/dashpay/dash/commit/822e617be) Fix chia_bls include prefix (#3091)
- [`35f079cbf`](https://github.com/dashpay/dash/commit/35f079cbf) Remove unused code (#3097)
- [`1acde17e8`](https://github.com/dashpay/dash/commit/1acde17e8) Don't care about governance cache while the blockchain isn't synced yet (#3089)
- [`0d126c2ae`](https://github.com/dashpay/dash/commit/0d126c2ae) Use chainparams factory for devnet (#3087)
- [`ac90abe89`](https://github.com/dashpay/dash/commit/ac90abe89) When mixing, always try to join an exsisting queue, only fall back to starting a new queue (#3085)
- [`68d575dc0`](https://github.com/dashpay/dash/commit/68d575dc0) Masternodes should have no wallet enabled (#3084)
- [`6b5b70fab`](https://github.com/dashpay/dash/commit/6b5b70fab) Remove liquidity provider privatesend (#3082)
- [`0b2221ed6`](https://github.com/dashpay/dash/commit/0b2221ed6) Clarify default max peer connections (#3081)
- [`c22169d57`](https://github.com/dashpay/dash/commit/c22169d57) Reduce non-debug PS log output (#3076)
- [`41ae1c7e2`](https://github.com/dashpay/dash/commit/41ae1c7e2) Add LDFLAGS_WRAP_EXCEPTIONS to dash_fuzzy linking (#3075)
- [`77b88558e`](https://github.com/dashpay/dash/commit/77b88558e) Update/modernize macOS plist (#3074)
- [`f1ff14818`](https://github.com/dashpay/dash/commit/f1ff14818) Fix bip69 vs change position issue (#3063)
- [`9abc39383`](https://github.com/dashpay/dash/commit/9abc39383) Refactor few things here and there (#3066)
- [`3d5eabcfb`](https://github.com/dashpay/dash/commit/3d5eabcfb) Update/unify `debug` and `logging` rpc descriptions (#3071)
- [`0e94e97cc`](https://github.com/dashpay/dash/commit/0e94e97cc) Add missing tx `type` to `TxToUniv` (#3069)
- [`becca24fc`](https://github.com/dashpay/dash/commit/becca24fc) Few fixes in docs/comments (#3068)
- [`9d109d6a3`](https://github.com/dashpay/dash/commit/9d109d6a3) Add missing `instantlock`/`instantlock_internal` to `getblock`'s `verbosity=2` mode (#3067)
- [`0f088d03a`](https://github.com/dashpay/dash/commit/0f088d03a) Change regtest and devnet p2p/rpc ports (#3064)
- [`190542256`](https://github.com/dashpay/dash/commit/190542256) Rework govobject/trigger cleanup a bit (#3070)
- [`386de78bc`](https://github.com/dashpay/dash/commit/386de78bc) Fix SelectCoinsMinConf to allow instant respends (#3061)
- [`cbbeec689`](https://github.com/dashpay/dash/commit/cbbeec689) RPC Getrawtransaction fix (#3065)
- [`1e3496799`](https://github.com/dashpay/dash/commit/1e3496799) Added getmemoryinfo parameter string update (#3062)
- [`9d2d8cced`](https://github.com/dashpay/dash/commit/9d2d8cced) Add a few malleability tests for DIP2/3 transactions (#3060)
- [`4983f7abb`](https://github.com/dashpay/dash/commit/4983f7abb) RPC Fix typo in getmerkleblocks help (#3056)
- [`a78dcfdec`](https://github.com/dashpay/dash/commit/a78dcfdec) Add the public GPG key for Pasta for Gitian building (#3057)
- [`929c892c0`](https://github.com/dashpay/dash/commit/929c892c0) Remove p2p alert leftovers (#3050)
- [`dd7873857`](https://github.com/dashpay/dash/commit/dd7873857) Re-verify invalid IS sigs when the active quorum set rotated (#3052)
- [`13e023510`](https://github.com/dashpay/dash/commit/13e023510) Remove recovered sigs from the LLMQ db when corresponding IS locks get confirmed (#3048)
- [`4a7525da3`](https://github.com/dashpay/dash/commit/4a7525da3) Add "instantsendlocks" to getmempoolinfo RPC (#3047)
- [`fbb49f92d`](https://github.com/dashpay/dash/commit/fbb49f92d) Bail out properly on Evo DB consistency check failures in ConnectBlock/DisconnectBlock (#3044)
- [`8d89350b8`](https://github.com/dashpay/dash/commit/8d89350b8) Use less alarming fee warning note (#3038)
- [`02f6188e8`](https://github.com/dashpay/dash/commit/02f6188e8) Do not count 0-fee txes for fee estimation (#3037)
- [`f0c73f5ce`](https://github.com/dashpay/dash/commit/f0c73f5ce) Revert "Skip mempool.dat when wallet is starting in "zap" mode (#2782)"
- [`be3bc48c9`](https://github.com/dashpay/dash/commit/be3bc48c9) Fix broken link in PrivateSend info dialog (#3031)
- [`acab8c552`](https://github.com/dashpay/dash/commit/acab8c552) Add Dash Core Group codesign certificate (#3027)
- [`a1c4321e9`](https://github.com/dashpay/dash/commit/a1c4321e9) Fix osslsigncode compile issue in gitian-build (#3026)
- [`2f21e5551`](https://github.com/dashpay/dash/commit/2f21e5551) Remove legacy InstantSend code (#3020)
- [`7a440d626`](https://github.com/dashpay/dash/commit/7a440d626) Optimize on-disk deterministic masternode storage to reduce size of evodb (#3017)
- [`85fcf32c9`](https://github.com/dashpay/dash/commit/85fcf32c9) Remove support for InstantSend locked gobject collaterals (#3019)
- [`bdec34c94`](https://github.com/dashpay/dash/commit/bdec34c94) remove DS mixes once they have been included in a chainlocked block (#3015)
- [`ee9adb948`](https://github.com/dashpay/dash/commit/ee9adb948) Use std::unique_ptr for mnList in CSimplifiedMNList (#3014)
- [`b401a3baa`](https://github.com/dashpay/dash/commit/b401a3baa) Fix compilation on Ubuntu 16.04 (#3013)
- [`c6eededca`](https://github.com/dashpay/dash/commit/c6eededca) Add "isValidMember" and "memberIndex" to "quorum memberof" and allow to specify quorum scan count (#3009)
- [`b9aadc071`](https://github.com/dashpay/dash/commit/b9aadc071) Fix excessive memory use when flushing chainstate and EvoDB (#3008)
- [`780bffeb7`](https://github.com/dashpay/dash/commit/780bffeb7) Enable stacktrace support in gitian builds (#3006)
- [`5809c5c3d`](https://github.com/dashpay/dash/commit/5809c5c3d) Implement "quorum memberof" (#3004)
- [`63424fb26`](https://github.com/dashpay/dash/commit/63424fb26) Fix 2 common Travis failures which happen when Travis has network issues (#3003)
- [`09b017fc5`](https://github.com/dashpay/dash/commit/09b017fc5) Only load signingActiveQuorumCount + 1 quorums into cache (#3002)
- [`b75e1cebd`](https://github.com/dashpay/dash/commit/b75e1cebd) Decouple lite mode and client-side PrivateSend (#2893)
- [`b9a738528`](https://github.com/dashpay/dash/commit/b9a738528) Remove skipped denom from the list on tx commit (#2997)
- [`5bdb2c0ce`](https://github.com/dashpay/dash/commit/5bdb2c0ce) Revert "Show BIP9 progress in getblockchaininfo (#2435)"
- [`b62db7618`](https://github.com/dashpay/dash/commit/b62db7618) Revert " Add real timestamp to log output when mock time is enabled (#2604)"
- [`1f6e0435b`](https://github.com/dashpay/dash/commit/1f6e0435b) [Trivial] Fix a typo in a comment in mnauth.h (#2988)
- [`f84d5d46d`](https://github.com/dashpay/dash/commit/f84d5d46d) QT: Revert "Force TLS1.0+ for SSL connections" (#2985)
- [`2e13d1305`](https://github.com/dashpay/dash/commit/2e13d1305) Add some comments to make quorum merkle root calculation more clear+ (#2984)
- [`6677a614a`](https://github.com/dashpay/dash/commit/6677a614a) Run extended tests when Travis is started through cron (#2983)
- [`d63202bdc`](https://github.com/dashpay/dash/commit/d63202bdc) Should send "reject" when mixing queue is full (#2981)
- [`8d5781f40`](https://github.com/dashpay/dash/commit/8d5781f40) Stop reporting/processing the number of mixing participants in DSSTATUSUPDATE (#2980)
- [`7334aa553`](https://github.com/dashpay/dash/commit/7334aa553) adjust privatesend formatting and follow some best practices (#2979)
- [`f14179ca0`](https://github.com/dashpay/dash/commit/f14179ca0) [Tests] Remove unused variable and inline another variable in evo_deterministicmns_tests.cpp (#2978)
- [`2756cb795`](https://github.com/dashpay/dash/commit/2756cb795) remove spork 12 (#2754)
- [`633231092`](https://github.com/dashpay/dash/commit/633231092) Provide correct params to AcceptToMemoryPoolWithTime() in LoadMempool() (#2976)
- [`e03538778`](https://github.com/dashpay/dash/commit/e03538778) Back off for 1m when connecting to quorum masternodes (#2975)
- [`bfcfb70d8`](https://github.com/dashpay/dash/commit/bfcfb70d8) Ignore blocks that do not match the filter in getmerkleblocks rpc (#2973)
- [`4739daddc`](https://github.com/dashpay/dash/commit/4739daddc) Process/keep messages/connections from PoSe-banned MNs (#2967)
- [`864856688`](https://github.com/dashpay/dash/commit/864856688) Multiple speed optimizations for deterministic MN list handling (#2972)
- [`d931cb723`](https://github.com/dashpay/dash/commit/d931cb723) Update copyright date (2019) (#2970)
- [`a83b63186`](https://github.com/dashpay/dash/commit/a83b63186) Fix UI masternode list (#2966)
- [`85c9ea400`](https://github.com/dashpay/dash/commit/85c9ea400) Throw a bit more descriptive error message on UpgradeDB failure on pruned nodes (#2962)
- [`2c5e2bc6c`](https://github.com/dashpay/dash/commit/2c5e2bc6c) Inject custom specialization of std::hash for SporkId enum into std (#2960)
- [`809aae73a`](https://github.com/dashpay/dash/commit/809aae73a) RPC docs helper updates (#2949)
- [`09d66c776`](https://github.com/dashpay/dash/commit/09d66c776) Fix compiler warning (#2956)
- [`26bd0d278`](https://github.com/dashpay/dash/commit/26bd0d278) Fix bls and bls_dkg bench (#2955)
- [`d28d318aa`](https://github.com/dashpay/dash/commit/d28d318aa) Remove logic for handling objects and votes orphaned by not-yet-known MNs (#2954)
- [`e02c562aa`](https://github.com/dashpay/dash/commit/e02c562aa) [RPC] Remove check for deprecated `masternode start-many` command (#2950)
- [`fc73b4d6e`](https://github.com/dashpay/dash/commit/fc73b4d6e) Refactor sporks to get rid of repeated if/else blocks (#2946)
- [`a149ca747`](https://github.com/dashpay/dash/commit/a149ca747) Remove references to instantx and darksend in sendcoinsdialog.cpp (#2936)
- [`b74cd3e10`](https://github.com/dashpay/dash/commit/b74cd3e10) Only require valid collaterals for votes and triggers (#2947)
- [`66b336c93`](https://github.com/dashpay/dash/commit/66b336c93) Use Travis stages instead of custom timeouts (#2948)
- [`5780fa670`](https://github.com/dashpay/dash/commit/5780fa670) Remove duplicate code from src/Makefile.am (#2944)
- [`428f30450`](https://github.com/dashpay/dash/commit/428f30450) Implement `rawchainlocksig` and `rawtxlocksig` (#2930)
- [`c08e76101`](https://github.com/dashpay/dash/commit/c08e76101) Tighten rules for DSVIN/DSTX (#2897)
- [`f1fe24b67`](https://github.com/dashpay/dash/commit/f1fe24b67) Only gracefully timeout Travis when integration tests need to be run (#2933)
- [`7c05aa821`](https://github.com/dashpay/dash/commit/7c05aa821) Also gracefully timeout Travis builds when building source takes >30min (#2932)
- [`5652ea023`](https://github.com/dashpay/dash/commit/5652ea023) Show number of InstantSend locks in Debug Console (#2919)
- [`a3f030609`](https://github.com/dashpay/dash/commit/a3f030609) Implement getmerkleblocks rpc (#2894)
- [`32aa229c7`](https://github.com/dashpay/dash/commit/32aa229c7) Reorganize Dash Specific code into folders (#2753)
- [`acbf0a221`](https://github.com/dashpay/dash/commit/acbf0a221) Bump version to 0.14.1 (#2928)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Alexander Block (codablock)
- Amir Abrams (AmirAbrams)
- -k (charlesrocket)
- Cofresi
- Nathan Marley (nmarley)
- PastaPastaPasta
- Riku (rikublock)
- strophy
- taw00
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

- [v0.14.0.5](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.5.md) released December/08/2019
- [v0.14.0.4](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.4.md) released November/22/2019
- [v0.14.0.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.3.md) released August/15/2019
- [v0.14.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.2.md) released July/4/2019
- [v0.14.0.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.1.md) released May/31/2019
- [v0.14.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.md) released May/22/2019
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
