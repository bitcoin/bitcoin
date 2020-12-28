Dash Core version 0.16
======================

Release is now available from:

  <https://www.dash.org/downloads/#wallets>

This is a new major version release, bringing new features, various bugfixes
and other improvements.

This release is mandatory for all nodes.

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
or -reindex) to make sure your wallet has all the new data synced. Upgrading
from version 0.13 should not require any additional actions.

When upgrading from a version prior to 0.14.0.3, the
first startup of Dash Core will run a migration process which can take a few
minutes to finish. After the migration, a downgrade to an older version is only
possible with a reindex (or reindex-chainstate).

Downgrade warning
-----------------

### Downgrade to a version < 0.14.0.3

Downgrading to a version older than 0.14.0.3 is no longer supported due to
changes in the "evodb" database format. If you need to use an older version,
you must either reindex or re-sync the whole chain.

### Downgrade of masternodes to < 0.16

Starting with this release, masternodes will verify the protocol version of other
masternodes. This will result in PoSe punishment/banning for outdated masternodes,
so downgrading is not recommended.

Notable changes
===============

Block Reward Reallocation
-------------------------
This version implements Block Reward Reallocation which was proposed in order
to slow the growth rate of Dash’s circulating supply by encouraging the
formation of masternodes and was voted in by the network. The resulting allocation
will split all non-proposal block rewards 40% toward miners and 60% toward
masternodes in the end-state once the transition period is complete.

The reallocation will take place over 4.5 years with a total of 18 reallocation
periods between the start and end state. The transition is being made gradually
to avoid market volatility and minimize network disruption.

Note that this is a hardfork which must be activated by miners. To do this they
should start creating blocks signalling bit 5 in the `version` field of the block header.

### Reallocation periods

Each reallocation period will last three superblock cycles (approximately one
quarter). The following table shows the split of the non-proposal block rewards during
each period.

|Event|Miner|Masternode|
|--|-----|-----|
|Now|50.0%|50.0%|
|1 |48.7%|51.3%|
|2 |47.4%|52.6%|
|3 |46.7%|53.3%|
|4 |46.0%|54.0%|
|5 |45.4%|54.6%|
|6 |44.8%|55.2%|
|7 |44.3%|55.7%|
|8 |43.8%|56.2%|
|9 |43.3%|56.7%|
|10|42.8%|57.2%|
|11|42.3%|57.7%|
|12|41.8%|58.2%|
|13|41.5%|58.5%|
|14|41.2%|58.8%|
|15|40.9%|59.1%|
|16|40.6%|59.4%|
|17|40.3%|59.7%|
|18|40.1%|59.9%|
|End|40.0%|60.0%|

Dynamic Activation Thresholds
-----------------------------
In Dash we have used lower thresholds (80% vs 95% in BTC) to activate upgrades
via a BIP9-like mechanism for quite some time. While it's preferable to have as much
of the network hashrate signal update readiness as possible, this can result in
quite lengthy upgrades if one large non-upgraded entity stalls
all progress. Simply lowering thresholds even further can result in network
upgrades occurring too quickly and potentially introducing network instability. This version
implements BIP9-like dynamic activation thresholds which drop from some initial
level to a minimally acceptable one over time at an increasing rate. This provides
a safe non-blocking way of activating proposals.

This mechanism applies to the Block Reward Reallocation proposal mentioned above.
Its initial threshold is 80% and it will decrease to a minimum of 60% over the
course of 10 periods. Each period is 4032 blocks (approximately one week).

Concentrated Recovery
---------------------
In the current system, signature shares are propagated to all LLMQ members
until one of them has collected enough shares to recover the signature. All
members keep propagating and verifying each share until this recovered signature
is propagated in the LLMQ. This causes significant load on the LLMQ and results
in decreased throughput.

This new system initially sends all shares to a single deterministically selected node,
so that this node can recover the signature and propagate the recovered signature.
This way only the recovered signature needs to be propagated and verified by all
members. After sending their share to this node, each member waits for a
timeout and then sends their share to another deterministically selected member.
This process is repeated until a recovered signature is finally created and propagated.

This timeout begins at two seconds and increases exponentially up to ten seconds
(i.e. `2,4,8,10,10`) for each node that times out. This is to minimize the time
taken to generate a signature if the recovery node is down, while also
minimizing the traffic generated when the network is under stress.

The new system is activated with the newly added `SPORK_21_QUORUM_ALL_CONNECTED`
which has two hardcoded values with a special meaning: `0` activates Concentrated
Recovery for every LLMQ and `1` excludes `400_60` and `400_85` quorums.

Increased number of masternode connections
------------------------------------------
To implement "Concentrated Recovery", it is now necessary for all members of a LLMQ
to connect to all other members of the same LLMQ. This significantly increases the general
connection count for masternodes. Although these intra-quorum connections are less resource
intensive than normal p2p connections (as they only exchange LLMQ/masternode related
messages), masternode hardware and network requirements will still be higher than before.

Initially this change will only be activated for the smaller LLMQs (50 members).
Eventually it may be activated for larger quorums (400 members).

This is also controlled via `SPORK_21_QUORUM_ALL_CONNECTED`.

Masternode Connection Probing
-----------------------------
While each LLMQ member must have a connection to each other member, it's not necessary
for all members to actually connect to all other members. This is because only
one of a pair of two masternodes need to initiate the connection while the other one can
wait for an incoming connection. Probing is done in the case where a masternode doesn't
really need an outbound connection, but still wants to verify that the other side
has its port open. This is done by initiating a short lived connection, waiting
for `MNAUTH` to succeed and then disconnecting again.

After this process, each member of a LLMQ knows which members are unable to accept
connections, after which they will vote on these members to be "bad".

Masternode Minimum Protocol Version Checks
------------------------------------------
Members of LLMQs will now also check all other members for minimum protocol versions
while in DKG. If a masternode determines that another LLMQ member has a protocol
version that is too low, it will vote for the other member to be "bad".

PoSe punishment/banning
-----------------------
If 80% of all LLMQ members voted for the same member to be bad, it is excluded
in the final stages of the DKG. This causes the bad masternode to get PoSe punished
and then eventually PoSe banned.

Network performance improvements
--------------------------------
This version of Dash Core includes multiple optimizations to the network and p2p message
handling code. The most important one is the introduction of `epoll` on linux-based
systems. This removes most of the CPU overhead caused by the sub-optimal use of `select`,
which could easily use up 50-80% of the CPU time spent in the network thread when many
connections were involved.

Since these optimizations are exclusive to linux, it is possible that masternodes hosted
on windows servers will be unable to handle the network load and should consider migrating
to a linux based operating system.

Other improvements were made to the p2p message handling code, so that LLMQ
related connections do less work than full/normal p2p connections.

Wallet files
------------
The `--wallet=<path>` option now accepts full paths instead of requiring
wallets to be located in the `-walletdir` directory.

If `--wallet=<path>` is specified with a path that does not exist, it will now
create a wallet directory at the specified location (containing a `wallet.dat`
data file, a `db.log` file, and `database/log.??????????` files) instead of just
creating a data file at the path and storing log files in the parent
directory. This should make backing up wallets more straightforward than
before because the specified wallet path can just be directly archived without
having to look in the parent directory for transaction log files.

For backwards compatibility, wallet paths that are names of existing data files
in the `--walletdir` directory will continue to be accepted and interpreted the
same as before.

When Dash Core is not started with any `--wallet=<path>` options, the name of
the default wallet returned by `getwalletinfo` and `listwallets` RPCs is
now the empty string `""` instead of `"wallet.dat"`. If Dash Core is started
with any `--wallet=<path>` options, there is no change in behavior, and the
name of any wallet is just its `<path>` string.

PrivateSend coin management improvements
----------------------------------------
A new algorithm for the creation of mixing denominations was implemented which
should reduce the number of the smallest inputs created and give users more
control on soft and hard caps. A much more accurate fee management algorithm was
also implemented which should fix issues for users who have custom fees
specified in wallet config or in times when network is under load.

There is a new "PrivateSend" tab in the GUI which allows spending fully
mixed coins only. The CoinControl feature was also improved to display coins
based on the tab it was opened in, to protect users from accidentally spending
mixed and non-mixed coins in the same transaction and to give better overview of
spendable mixed coins.

PrivateSend Random Round Mixing
-------------------------------
Some potential attacks on PrivateSend assume that all inputs had been mixed
for the same number of rounds (up to 16). While this assumption alone is not
enough to break PrivateSend privacy, it could still provide some additional
information for other methods like cluster analysis and help to narrow results.

With Random Round Mixing, an input will be mixed to N rounds like prior. However,
at this point, a salted hash of each input is used to pick ~50% of inputs to
be mixed for another round. This rule is then executed again on the new inputs.
This results in an exponential decay where if you mix a set
of inputs, half of those inputs will be mixed for N rounds, 1/4 will be mixed N+1,
1/8 will be mixed N+2, etc. Current implementation caps it at N+3 which results
in mixing an average of N+0.875 rounds and sounds like a reasonable compromise
given the privacy gains.

GUI changes
-----------
All dialogs, tabs, icons, colors and interface elements were reworked to improve
user experience, let the wallet look more consistent and to make the GUI more
flexible. There is a new "Appearance setup" dialog that will show up on the first start
of this version and a corresponding tab has been added to the options which allows the
user to pick a theme and to tweak the font family, the font weight and the font size.
This feature specifically should help users who had font size/scaling issues previously.

For advanced users and developers there is a new way to control the wallet's look
by specifying a path to custom css files via `--custom-css-dir`. Additionally, the new
`--debug-ui` will force Dash Core to reload the custom css files as soon as they get updated
which makes it possible to see and debug all css adjustments live in the running GUI.

From now on the "Pay To" field in "Send" and "PrivateSend" tabs also accepts Dash URIs.
The Dash address and the amount from the URI are assigned to corresponding fields automatically
if a Dash URI gets pasted into the field.

Sporks
------
Two new sporks were introduced in this version:
- `SPORK_21_QUORUM_ALL_CONNECTED` allows to control Concentrated Recovery and
Masternode Probing activation;
- `SPORK_22_PS_MORE_PARTICIPANTS` raises the number of users that can participate
in a single PrivateSend mixing transaction to 20.

Sporks `SPORK_15_DETERMINISTIC_MNS_ENABLED`, `SPORK_16_INSTANTSEND_AUTOLOCKS`
and `SPORK_20_INSTANTSEND_LLMQ_BASED` which were previously deprecated in v0.15
are completely removed now. `SPORK_6_NEW_SIGS` was never activated on mainnet
and was also removed in this version.

Build system
------------
The minimum version of the GCC compiler required to compile Dash Core is now 4.8.
The minimum version of Qt is now 5.5.1. Some packages in `depends/` as well as
`secp256k1` and `leveldb` subtrees were updated to newer versions.

RPC changes
-----------
There are a few changes in existing RPC interfaces in this release:
- `getpeerinfo` has new field `masternode` to indicate whether connection was
  due to masternode connection attempt
- `getprivatesendinfo` `denoms` field was replaced by `denoms_goal` and
  `denoms_hardcap`
- `listunspent` has new filter option `coinType` to be able to filter different
  types of coins (all, mixed etc.)
- `protx diff` returns more detailed information about new quorums
- `quorum dkgstatus` shows `quorumConnections` for each LLMQ with detailed
  information about each participating masternode
- `quorum sign` has an optional `quorumHash` argument to pick the exact quorum
- `socketevents` in `getnetworkinfo` rpc shows the socket events mode,
  either `epoll`, `poll` or `select`

There are also new RPC commands:
- `quorum selectquorum` returns the quorum that would/should sign a request

There are also new RPC commands backported from Bitcoin Core 0.16:
- `help-console` for more info about using console in Qt
- `loadwallet` loads a wallet from a wallet file or directory
- `rescanblockchain` rescans the local blockchain for wallet related transactions
- `savemempool` dumps the mempool to disk

Please check Bitcoin Core 0.16 release notes in a [section](#backports-from-bitcoin-core-016)
below and `help <command>` in rpc for more information.

Command-line options
--------------------
Changes in existing cmd-line options:

New cmd-line options:
- `--custom-css-dir`
- `--debug-ui`
- `--disablegovernance`
- `--font-family`
- `--font-scale`
- `--font-weight-bold`
- `--font-weight-normal`
- `--llmqdevnetparams`
- `--llmqtestparams`
- `--privatesenddenomsgoal`
- `--privatesenddenomshardcap`
- `--socketevents`

Few cmd-line options are no longer supported:
- `--litemode`
- `--privatesenddenoms`

There are also new command-line options backported from Bitcoin Core 0.16:
- `--addrmantest`
- `--debuglogfile`
- `--deprecatedrpc`
- `--enablebip61`
- `--getinfo`
- `--stdinrpcpass`

Please check Bitcoin Core 0.16 release notes in a [section](#backports-from-bitcoin-core-016)
below and `Help -> Command-line options` in Qt wallet or `dashd --help` for more information.

Backports from Bitcoin Core 0.16
--------------------------------

Most of the changes between Bitcoin Core 0.15 and Bitcoin Core 0.16 have been
backported into Dash Core 0.16. We only excluded backports which do not align
with Dash, like SegWit or RBF related changes.

You can read about changes brought by backporting from Bitcoin Core 0.16 in
following docs:
- https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.16.0.md
- https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.16.1.md
- https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.16.2.md

Some other individual PRs were backported from versions 0.17+, you can find the
full list of backported PRs and additional fixes in [release-notes-0.16-backports.md](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.16-backports.md)

Miscellaneous
-------------
A lot of refactoring, code cleanups and other small fixes were done in this release.

0.16 Change log
===============

See detailed [set of changes](https://github.com/dashpay/dash/compare/v0.15.0.0...dashpay:v0.16.0.1).

- [`37fe9a6e2f`](https://github.com/dashpay/dash/commit/37fe9a6e2f) Fix mempool sync (#3725)
- [`69b77765ba`](https://github.com/dashpay/dash/commit/69b77765ba) qt: Fix font family updates (#3746)
- [`cf2da32ffe`](https://github.com/dashpay/dash/commit/cf2da32ffe) qt: Fix font size and scaling issues (#3734)
- [`586f166499`](https://github.com/dashpay/dash/commit/586f166499) qt: Fix Recent transactions list height (#3744)
- [`12d0782071`](https://github.com/dashpay/dash/commit/12d0782071) Translations 2020-09 (#3736)
- [`fc8ca6ad50`](https://github.com/dashpay/dash/commit/fc8ca6ad50) Bump few things and update man pages for v0.16 (#3737)
- [`8e6706dede`](https://github.com/dashpay/dash/commit/8e6706dede) qt: Add PrivateSend tab in OptionsDialog, allow to show/hide PS UI (#3717)
- [`dbfdf8cb15`](https://github.com/dashpay/dash/commit/dbfdf8cb15) qt: Finetune OverviewPage (#3715)
- [`9028712b60`](https://github.com/dashpay/dash/commit/9028712b60) qt: Finetune TransactionsView (#3710)
- [`f4e3bf95d0`](https://github.com/dashpay/dash/commit/f4e3bf95d0) qt: Finetune CoinControlDialog + bitcoin#14828 (#3701)
- [`e405f553b9`](https://github.com/dashpay/dash/commit/e405f553b9) RPC: Update getprivatesendinfo help (#3727)
- [`b4a78fb285`](https://github.com/dashpay/dash/commit/b4a78fb285) Fix testnet icon (#3726)
- [`3650ee6556`](https://github.com/dashpay/dash/commit/3650ee6556) qt: Remove unused assets (#3721)
- [`cb078422cd`](https://github.com/dashpay/dash/commit/cb078422cd) qt: Finetune RPCConsole (#3720)
- [`a198a74a4f`](https://github.com/dashpay/dash/commit/a198a74a4f) privatesend: Avoid interacting with keypool in CTransactionBuilder ctor (#3723)
- [`f601122cbd`](https://github.com/dashpay/dash/commit/f601122cbd) qt: Hide remaining PrivateSend UI if PrivateSend is not enabled  (#3716)
- [`c7896eb657`](https://github.com/dashpay/dash/commit/c7896eb657) qt: Disable missing macOS focus rects in AddressBookPage (#3711)
- [`c136522e84`](https://github.com/dashpay/dash/commit/c136522e84) qt: Finetune Options Dialog (#3709)
- [`bb0d5d4808`](https://github.com/dashpay/dash/commit/bb0d5d4808) qt: Make sure send confirmation dialog uses correct font settings (#3714)
- [`ab5ffe3c11`](https://github.com/dashpay/dash/commit/ab5ffe3c11) qt: Use scaled font size for all QToolTip instances (#3708)
- [`42b606ea76`](https://github.com/dashpay/dash/commit/42b606ea76) qt: Make sure font size in  TransactionDescDialog is adjusted properly (#3707)
- [`8fa280140a`](https://github.com/dashpay/dash/commit/8fa280140a) qt: Tweak few strings (#3706)
- [`a70056c5a8`](https://github.com/dashpay/dash/commit/a70056c5a8) privatesend: Implement Random Round Mixing (#3661)
- [`eb163c53a5`](https://github.com/dashpay/dash/commit/eb163c53a5) Implement dynamic activation thresholds (#3692)
- [`7d6aba47cf`](https://github.com/dashpay/dash/commit/7d6aba47cf) Implement Block Reward Reallocation (#3691)
- [`19aab3a1d8`](https://github.com/dashpay/dash/commit/19aab3a1d8) qt|wallet: Fix "Use available balance" for PrivateSend  (#3700)
- [`37af70a188`](https://github.com/dashpay/dash/commit/37af70a188) Merge #13622: Remove mapRequest tracking that just effects Qt display. (#3694)
- [`e13c746a7d`](https://github.com/dashpay/dash/commit/e13c746a7d) chainparams: Remove llmq_50_60 from regtest (#3696)
- [`f2c1a9213c`](https://github.com/dashpay/dash/commit/f2c1a9213c) Fix two potential issues in the way pending islocks are processed (#3678)
- [`01a3a6c81a`](https://github.com/dashpay/dash/commit/01a3a6c81a) qt: Finetune ModalOverlay (#3699)
- [`16f9d94404`](https://github.com/dashpay/dash/commit/16f9d94404) qt: Make sure the statusbar reflects internal states correct (#3698)
- [`9e425d66c6`](https://github.com/dashpay/dash/commit/9e425d66c6) Partially merge #3604:
- [`2bb94c73eb`](https://github.com/dashpay/dash/commit/2bb94c73eb) masternode|net|rpc: Improve masternode sync process (#3690)
- [`aa81a06264`](https://github.com/dashpay/dash/commit/aa81a06264) masternode|rpc: Remove unused code (#3689)
- [`2e92237865`](https://github.com/dashpay/dash/commit/2e92237865) qt: Improved status bar (#3688)
- [`54ed089f97`](https://github.com/dashpay/dash/commit/54ed089f97) test: Implement unit tests for CTransactionBuilder (#3677)
- [`ab0f789be6`](https://github.com/dashpay/dash/commit/ab0f789be6) wallet: Fix and improve CWallet::CreateTransaction (#3668)
- [`dd55609d6b`](https://github.com/dashpay/dash/commit/dd55609d6b) test: Implement unit tests for CWallet::CreateTransaction (#3667)
- [`9c988d5b5a`](https://github.com/dashpay/dash/commit/9c988d5b5a) privatesend|wallet|qt: Improve calculations in CreateDenominated/MakeCollateralAmounts (#3657)
- [`ba2dcb0399`](https://github.com/dashpay/dash/commit/ba2dcb0399) qt: Update assets and colorize them theme related (#3574)
- [`50d7a2fecf`](https://github.com/dashpay/dash/commit/50d7a2fecf) qt: Ignore GUIUtil::updateFont calls until GUIUtil::loadFonts was called (#3687)
- [`608f481a9c`](https://github.com/dashpay/dash/commit/608f481a9c) qt: Fix block update signals/slots in BitcoinGUI and SendCoinsDialog (#3685)
- [`25e4fed75b`](https://github.com/dashpay/dash/commit/25e4fed75b) QT: add last block hash to debug ui (#3672)
- [`195dcad1ee`](https://github.com/dashpay/dash/commit/195dcad1ee) trivial/docs: minor adjustments to PrivateSend help text (#3669)
- [`b1a35b2d32`](https://github.com/dashpay/dash/commit/b1a35b2d32) Merge #13007: test: Fix dangling wallet pointer in vpwallets (#3666)
- [`f9bf42cb90`](https://github.com/dashpay/dash/commit/f9bf42cb90) Harden spork6 logic then remove spork6 (#3662)
- [`604d6a4225`](https://github.com/dashpay/dash/commit/604d6a4225) Fix some translation issues (#3656)
- [`60d298376b`](https://github.com/dashpay/dash/commit/60d298376b) Fix crash on splash screen when wallet fails to load (#3629)
- [`ca76dd0868`](https://github.com/dashpay/dash/commit/ca76dd0868) qt: Give PrivateSend separate instances of SendCoinsDialog + CCoinControl (#3625)
- [`0969c2d268`](https://github.com/dashpay/dash/commit/0969c2d268) qt: Splashscreen redesign (#3613)
- [`966be38c79`](https://github.com/dashpay/dash/commit/966be38c79) qt: Make sure stylesheet updates of -debug-ui are activated (#3623)
- [`0ed7f1672c`](https://github.com/dashpay/dash/commit/0ed7f1672c) qt: Add missing placeholders (#3575)
- [`103fda2cb7`](https://github.com/dashpay/dash/commit/103fda2cb7) qt: Fix appearancewidget.h to make lint-include-guards.sh happy (#3627)
- [`2cc36dffde`](https://github.com/dashpay/dash/commit/2cc36dffde) qt: Drop PlatformStyle (#3573)
- [`b073ae9853`](https://github.com/dashpay/dash/commit/b073ae9853) qt: Redesign scrollbar styles (#3571)
- [`1a2867f886`](https://github.com/dashpay/dash/commit/1a2867f886) qt: Introduce appearance tab and setup dialog (#3568)
- [`d44611bc2b`](https://github.com/dashpay/dash/commit/d44611bc2b) qt: General qt/c++ related fixes and updates (#3562)
- [`d22ab43910`](https://github.com/dashpay/dash/commit/d22ab43910) qt: Introduce platform specific css sections (#3570)
- [`a16cfa77de`](https://github.com/dashpay/dash/commit/a16cfa77de) qt: Redesign BitcoinAmountField (#3569)
- [`7db9909a17`](https://github.com/dashpay/dash/commit/7db9909a17) qt: Introduce runtime theme changes (#3559)
- [`b3738648d6`](https://github.com/dashpay/dash/commit/b3738648d6) qt: General CSS related redesigns (#3563)
- [`1dea248d0d`](https://github.com/dashpay/dash/commit/1dea248d0d) qt: Make use of GUIUtil themed colors/styles (#3561)
- [`06d69d74c2`](https://github.com/dashpay/dash/commit/06d69d74c2) qt: Replace usage of QTabBar with custom replacement (#3560)
- [`edfc3a4646`](https://github.com/dashpay/dash/commit/edfc3a4646) qt: Call GUIUtil::loadFonts earlier (#3593)
- [`f371e4771c`](https://github.com/dashpay/dash/commit/f371e4771c) qt: Add -debug-ui command line parameter (#3558)
- [`dde4ab5f84`](https://github.com/dashpay/dash/commit/dde4ab5f84) qt: Add -custom-css-dir commmand line parameter (#3557)
- [`067622764c`](https://github.com/dashpay/dash/commit/067622764c) qt: Disable macOS system focus rectangles for dash themes (#3556)
- [`d296cbdca1`](https://github.com/dashpay/dash/commit/d296cbdca1) qt: Implement application wide font management (#3555)
- [`b409254587`](https://github.com/dashpay/dash/commit/b409254587) qt: Redesign of the main toolbar (#3554)
- [`c5fcc811d6`](https://github.com/dashpay/dash/commit/c5fcc811d6) qt: Generalized css files, simple design changes, added scripts to keep track of color usage (#3508)
- [`8aeeb97995`](https://github.com/dashpay/dash/commit/8aeeb97995) Print exception origin in crash messages (#3653)
- [`bd5c047e28`](https://github.com/dashpay/dash/commit/bd5c047e28) Implement a safer version of GetCrashInfoFromException (#3652)
- [`4414b5c3c7`](https://github.com/dashpay/dash/commit/4414b5c3c7) llmq: Fix spork check in CSigSharesManager::ForceReAnnouncement (#3650)
- [`c232f2a12d`](https://github.com/dashpay/dash/commit/c232f2a12d) [RPC] Show address of fundDest when no funds (#3649)
- [`abe3d578b2`](https://github.com/dashpay/dash/commit/abe3d578b2) Include protocol version into MNAUTH (#3631)
- [`4db8968d4a`](https://github.com/dashpay/dash/commit/4db8968d4a) rpc: update help text for BLS operator key arguments (#3628)
- [`bbb0064b60`](https://github.com/dashpay/dash/commit/bbb0064b60) init: Fix crash due to -litemode and improve its deprecation warning (#3626)
- [`8d99e7836c`](https://github.com/dashpay/dash/commit/8d99e7836c) Fix `-resetguisettings` (#3624)
- [`508fc2db4b`](https://github.com/dashpay/dash/commit/508fc2db4b) privatesend: Increase max participants to 20 (#3610)
- [`f425316eed`](https://github.com/dashpay/dash/commit/f425316eed) util: Change TraceThread's "name" type: "const char*" -> "const std::string" (#3609)
- [`a170c649b8`](https://github.com/dashpay/dash/commit/a170c649b8) Fail GetTransaction when the block from txindex is not in mapBlockIndex (#3606)
- [`62c3fb5748`](https://github.com/dashpay/dash/commit/62c3fb5748) llmq: Fix thread handling in CDKGSessionManager and CDKGSessionHandler (#3601)
- [`aa3bec6106`](https://github.com/dashpay/dash/commit/aa3bec6106) evo: Avoid some unnecessary copying in BuildNewListFromBlock (#3594)
- [`6a249f59b6`](https://github.com/dashpay/dash/commit/6a249f59b6) Merge #13564: [wallet] loadwallet shouldn't create new wallets. (#3592)
- [`b45b1373a0`](https://github.com/dashpay/dash/commit/b45b1373a0) Prefer creating larger denoms at the second step of CreateDenominated (#3589)
- [`bac02d0c9a`](https://github.com/dashpay/dash/commit/bac02d0c9a) More accurate fee calculation in CreateDenominated (#3588)
- [`baf18a35d5`](https://github.com/dashpay/dash/commit/baf18a35d5) More pruning improvements (#3579)
- [`0297eb428a`](https://github.com/dashpay/dash/commit/0297eb428a) Change litemode to disablegovernance (#3577)
- [`da38e2bf83`](https://github.com/dashpay/dash/commit/da38e2bf83) Change litemode from disabling all Dash specific features to disabling governance validation (#3488)
- [`772b6bfe7c`](https://github.com/dashpay/dash/commit/772b6bfe7c) Disable new connection handling and concentrated recovery for large LLMQs (#3548)
- [`c72bc354f8`](https://github.com/dashpay/dash/commit/c72bc354f8) contrib: Move dustinface.pgp into contrib/gitian-keys (#3547)
- [`0b70380fff`](https://github.com/dashpay/dash/commit/0b70380fff) Fix argument handling for devnets (#3549)
- [`5f9fc5edb6`](https://github.com/dashpay/dash/commit/5f9fc5edb6) Fix CSigningManager::VerifyRecoveredSig (#3546)
- [`00486eca06`](https://github.com/dashpay/dash/commit/00486eca06) Use exponential backoff timeouts for recovery (#3535)
- [`34c354eaad`](https://github.com/dashpay/dash/commit/34c354eaad) Dont skip sendmessages (#3534)
- [`40814d945e`](https://github.com/dashpay/dash/commit/40814d945e) Avoid overriding validation states, return more specific states in some cases (#3541)
- [`139f589b6f`](https://github.com/dashpay/dash/commit/139f589b6f) Don'd send SENDXXX messages to fMasternode connections (#3537)
- [`f064964678`](https://github.com/dashpay/dash/commit/f064964678) Only relay DKG messages to intra quorum connection members (#3536)
- [`d08b971ddb`](https://github.com/dashpay/dash/commit/d08b971ddb) Use correct CURRENT_VERSION constants when creating ProTx-es via rpc (#3524)
- [`4ae57a2276`](https://github.com/dashpay/dash/commit/4ae57a2276) qt: Fix label updates in SendCoinsEntry (#3523)
- [`1a54f0392d`](https://github.com/dashpay/dash/commit/1a54f0392d) Revert "implemented labeler which automatically adds RPC label to anything modifying RPC files (#3499)" (#3517)
- [`acff9998e1`](https://github.com/dashpay/dash/commit/acff9998e1) UpgradeDBIfNeeded failure should require reindexing (#3516)
- [`1163fc70a2`](https://github.com/dashpay/dash/commit/1163fc70a2) Use correct CURRENT_VERSION constants when checking ProTx version (#3515)
- [`0186fdfe60`](https://github.com/dashpay/dash/commit/0186fdfe60) Fix chainlock scheduler handling (#3514)
- [`b4008ee4fc`](https://github.com/dashpay/dash/commit/b4008ee4fc) Some Dashification (#3513)
- [`ab5aeed920`](https://github.com/dashpay/dash/commit/ab5aeed920) Optimize MN lists cache (#3506)
- [`91d9329093`](https://github.com/dashpay/dash/commit/91d9329093) Make CDeterministicMN::internalId private to make sure it's set/accessed properly (#3505)
- [`232430fac9`](https://github.com/dashpay/dash/commit/232430fac9) Fix ProcessNewBlock vs EnforceBestChainLock deadlocks in ActivateBestChain (#3492)
- [`fe98b81b80`](https://github.com/dashpay/dash/commit/fe98b81b80) implemented labeler which automatically adds RPC label to anything modifying RPC files (#3499)
- [`ae5faf23da`](https://github.com/dashpay/dash/commit/ae5faf23da) Better error handling while processing special txes (#3504)
- [`13a45ec323`](https://github.com/dashpay/dash/commit/13a45ec323) rpc: Validate provided keys for query_options parameter in listunspent (#3507)
- [`9b47883884`](https://github.com/dashpay/dash/commit/9b47883884) contrib: Added dustinface.pgp (#3502)
- [`048503bcb5`](https://github.com/dashpay/dash/commit/048503bcb5) qt: Some UI fixes and improvements (#3500)
- [`8fcda67a54`](https://github.com/dashpay/dash/commit/8fcda67a54) Remove spork 15, 16, 20 (#3493)
- [`9d3546baee`](https://github.com/dashpay/dash/commit/9d3546baee) Reintroduce mixing hard cap for all but the largest denom (#3489)
- [`397630a82c`](https://github.com/dashpay/dash/commit/397630a82c) CI: Fix Gitlab nowallet and release builds (#3491)
- [`a9fc40fb0a`](https://github.com/dashpay/dash/commit/a9fc40fb0a) add "Verifying a Rebase" section to CONTRIBUTING.md (#3487)
- [`0c5c99243a`](https://github.com/dashpay/dash/commit/0c5c99243a) rpc/wallet: Add coinType to queryOptions of listunspent (#3483)
- [`3a56ed9ca6`](https://github.com/dashpay/dash/commit/3a56ed9ca6) Fix NO_WALLET=1 build (#3490)
- [`926087aac6`](https://github.com/dashpay/dash/commit/926087aac6) Implement significantly improved createdenominations algorithm (#3479)
- [`fe208c98e3`](https://github.com/dashpay/dash/commit/fe208c98e3) Feat. request for Dash Platform: `quorum sign` rpc command with additional quorumHash #3424 (#3446)
- [`4c1f65baae`](https://github.com/dashpay/dash/commit/4c1f65baae) Fix #3241 UX/UI - Introduce PrivateSend tab which allows to spend fully mixed coins only (#3442)
- [`f46617dbab`](https://github.com/dashpay/dash/commit/f46617dbab) add litemode information to help texts regarding CL/IS and change getbestchainlock to throw an error if running in litemode (#3478)
- [`5cabc8f5ca`](https://github.com/dashpay/dash/commit/5cabc8f5ca) Introduce ONLY_PRIVATESEND coin type to select fully mixed coins only (#3459)
- [`e0ff9af2b0`](https://github.com/dashpay/dash/commit/e0ff9af2b0) qt: Allow and process URIs pasted to the payTo field of SendCoinsEntry (#3475)
- [`1c20160933`](https://github.com/dashpay/dash/commit/1c20160933) Fix `gobject submit`: replace request params with txidFee (#3471)
- [`970c23d6e6`](https://github.com/dashpay/dash/commit/970c23d6e6) Remove logic for forcing chainlocks without DIP8 activation on testnet (#3470)
- [`ae15299117`](https://github.com/dashpay/dash/commit/ae15299117) Feature request #3425 - Add information to "protx diff" (#3468)
- [`017c4779ca`](https://github.com/dashpay/dash/commit/017c4779ca) Fix recovery from coin db crashes (and dbcrash.py test) (#3467)
- [`d5f403d3fd`](https://github.com/dashpay/dash/commit/d5f403d3fd) Refactor and fix GetRealOutpointPrivateSendRounds (#3460)
- [`c06838e205`](https://github.com/dashpay/dash/commit/c06838e205) Streamline processing of pool state updates (#3458)
- [`538fcf2f1b`](https://github.com/dashpay/dash/commit/538fcf2f1b) Disable qt menu heuristics for openConfEditorAction (#3466)
- [`6e1c5480cd`](https://github.com/dashpay/dash/commit/6e1c5480cd) qt: Maximize the window if the dock icon gets clicked on macOS (#3465)
- [`3960d622c5`](https://github.com/dashpay/dash/commit/3960d622c5) Fix incorrect nTimeFirstKey reset due to missing count of hd pubkeys (#3461)
- [`d0bb30838b`](https://github.com/dashpay/dash/commit/d0bb30838b) Various (mostly trivial) PS fixes (#3457)
- [`b0963b079e`](https://github.com/dashpay/dash/commit/b0963b079e) Fix deadlocks (#3456)
- [`bdce58756a`](https://github.com/dashpay/dash/commit/bdce58756a) Remove duplicated condition check (#3464)
- [`10baa4a857`](https://github.com/dashpay/dash/commit/10baa4a857) Update Windows build instructions (#3453)
- [`7fdc4c7b0d`](https://github.com/dashpay/dash/commit/7fdc4c7b0d) change miniupnp lib server (#3452)
- [`911b5580e4`](https://github.com/dashpay/dash/commit/911b5580e4) Fix typo in error log when EPOLL_CTL_ADD fails for wakeup pipe (#3451)
- [`b775fa263f`](https://github.com/dashpay/dash/commit/b775fa263f) Lower DEFAULT_PRIVATESEND_DENOMS (#3434)
- [`d59deea77b`](https://github.com/dashpay/dash/commit/d59deea77b) Implement epoll support (#3445)
- [`96faa8155e`](https://github.com/dashpay/dash/commit/96faa8155e) Don't disconnect masternode probes for a few seconds (#3449)
- [`608aed3d85`](https://github.com/dashpay/dash/commit/608aed3d85) Don't try to connect to itself through CLLMQUtils::GetQuorumConnections (#3448)
- [`d663f48085`](https://github.com/dashpay/dash/commit/d663f48085) Lower SELECT_TIMEOUT_MILLISECONDS for USE_WAKEUP_PIPE case (#3444)
- [`f5f4ccbf24`](https://github.com/dashpay/dash/commit/f5f4ccbf24) Refactor/Prepare CConnman for upcoming epoll support (#3432)
- [`93aa640af8`](https://github.com/dashpay/dash/commit/93aa640af8) Fix #3248: use blue logo for Traditional theme (#3441)
- [`3d24290bcb`](https://github.com/dashpay/dash/commit/3d24290bcb) Take all nodes into account in check_sigs instead of just just masternodes (#3437)
- [`d06597a421`](https://github.com/dashpay/dash/commit/d06597a421) [Trivial] Adjust some text in mnauth.cpp (#3413)
- [`de931a25a3`](https://github.com/dashpay/dash/commit/de931a25a3) Wakeup wakeup-pipe when new peers are added (#3433)
- [`75a1968c96`](https://github.com/dashpay/dash/commit/75a1968c96) Fix abandonconflict.py (#3436)
- [`2610e718cd`](https://github.com/dashpay/dash/commit/2610e718cd) Don't delete MN list snapshots and diffs from DB when reorgs take place (#3435)
- [`6d83b0a053`](https://github.com/dashpay/dash/commit/6d83b0a053) Make socketevents mode (poll vs select) configurable via parameter (#3431)
- [`9a8caf0986`](https://github.com/dashpay/dash/commit/9a8caf0986) Remove fix for fNetworkActive vs OpenNetworkConnection race (#3430)
- [`96ed9fae39`](https://github.com/dashpay/dash/commit/96ed9fae39) Fix flushing of rejects before disconnecting (#3429)
- [`0cb385c567`](https://github.com/dashpay/dash/commit/0cb385c567) Improve network connections related logging (#3428)
- [`d5092c44cb`](https://github.com/dashpay/dash/commit/d5092c44cb) Make sure that cleanup is not triggered too early in llmq-signing.py (#3427)
- [`402b13907d`](https://github.com/dashpay/dash/commit/402b13907d) Only call DisconnectNodes once per second (#3421)
- [`ee995ef02a`](https://github.com/dashpay/dash/commit/ee995ef02a) Implement more reliable wait_for_masternode_probes in test framework (#3422)
- [`755a23ca00`](https://github.com/dashpay/dash/commit/755a23ca00) Always pass current mocktime to started nodes (#3423)
- [`1e30054b9e`](https://github.com/dashpay/dash/commit/1e30054b9e) Avoid calling SendMessages (and others) for all nodes all the time (#3420)
- [`8aa85c084b`](https://github.com/dashpay/dash/commit/8aa85c084b) Deterministically choose which peers to drop on duplicate MNAUTH (#3419)
- [`79f0bb1033`](https://github.com/dashpay/dash/commit/79f0bb1033) Fix crash in validateaddress (#3418)
- [`d032d02f10`](https://github.com/dashpay/dash/commit/d032d02f10) Multiple fixes for failing tests (#3410)
- [`e20c63f535`](https://github.com/dashpay/dash/commit/e20c63f535) A few simple/trivial optimizations (#3398)
- [`9bcdeaea57`](https://github.com/dashpay/dash/commit/9bcdeaea57) Avoid unnecessary processing/verification of reconstructed recovered signatures (#3399)
- [`38556a3d49`](https://github.com/dashpay/dash/commit/38556a3d49) Don't try to connect to masternodes that we already have a connection to (#3401)
- [`0e56e32c22`](https://github.com/dashpay/dash/commit/0e56e32c22) Add cache for CBlockTreeDB::HasTxIndex (#3402)
- [`2dff0501e9`](https://github.com/dashpay/dash/commit/2dff0501e9) Remove semaphore for masternode connections (#3403)
- [`c1d9dd553a`](https://github.com/dashpay/dash/commit/c1d9dd553a) FindDevNetGenesisBlock remove unused arg (#3405)
- [`c7b6eb851d`](https://github.com/dashpay/dash/commit/c7b6eb851d) Implement "concentrated recovery" of LLMQ signatures (#3389)
- [`e518ce4e13`](https://github.com/dashpay/dash/commit/e518ce4e13) Increase DIP0008 bip9 window by 10 years (#3391)
- [`1aba86567b`](https://github.com/dashpay/dash/commit/1aba86567b) Implement checking of open ports and min proto versions in DKGs (#3390)
- [`f43cdbc586`](https://github.com/dashpay/dash/commit/f43cdbc586) Gradually bump mocktime in wait_for_quorum_connections (#3388)
- [`3b904a0fa1`](https://github.com/dashpay/dash/commit/3b904a0fa1) Add a note about dash_hash under dependencies in test/README.md (#3386)
- [`b0668028b6`](https://github.com/dashpay/dash/commit/b0668028b6) Implement more randomized behavior in GetQuorumConnections (#3385)
- [`27dfb5a34d`](https://github.com/dashpay/dash/commit/27dfb5a34d) Move wait_proc into wait_for_quorum_connections (#3384)
- [`ff6f391aea`](https://github.com/dashpay/dash/commit/ff6f391aea) Refactor Gitlab builds to use multiple stages (#3377)
- [`a5a3e51554`](https://github.com/dashpay/dash/commit/a5a3e51554) Let all LLMQ members connect to each other instead of only a few ones (#3380)
- [`8ab1a3734a`](https://github.com/dashpay/dash/commit/8ab1a3734a) Bump mocktime each time waiting for phase1 fails (#3383)
- [`c68b5f68aa`](https://github.com/dashpay/dash/commit/c68b5f68aa) Hold CEvoDB lock while iterating mined commitments (#3379)
- [`deba865b17`](https://github.com/dashpay/dash/commit/deba865b17) Also verify quorumHash when waiting for DKG phases (#3382)
- [`17ece14f40`](https://github.com/dashpay/dash/commit/17ece14f40) Better/more logging for DKGs (#3381)
- [`80be2520a2`](https://github.com/dashpay/dash/commit/80be2520a2) Call FlushBackgroundCallbacks before resetting CConnman (#3378)
- [`b6bdb8be9e`](https://github.com/dashpay/dash/commit/b6bdb8be9e) Faster opening of masternode connections (#3375)
- [`94bcf85347`](https://github.com/dashpay/dash/commit/94bcf85347) Refactor and unify quorum connection handling #(3367)
- [`c0bb06e766`](https://github.com/dashpay/dash/commit/c0bb06e766) Multiple fixes for masternode connection handling (#3366)
- [`f2ece1031f`](https://github.com/dashpay/dash/commit/f2ece1031f) Remove logging for waking of select() (#3370)
- [`cf1f8c3825`](https://github.com/dashpay/dash/commit/cf1f8c3825) Support devnets in mininode (#3364)
- [`f7ddee13a1`](https://github.com/dashpay/dash/commit/f7ddee13a1) Fix possible segfault (#3365)
- [`40cdfe8662`](https://github.com/dashpay/dash/commit/40cdfe8662) Add peer id to "socket send error" logs (#3363)
- [`0fa2e14065`](https://github.com/dashpay/dash/commit/0fa2e14065) Fix issues introduced with asynchronous signal handling (#3369)
- [`b188c5c25e`](https://github.com/dashpay/dash/commit/b188c5c25e) Refactor some PrivateSend related code to use WalletModel instead of accessing the wallet directly from qt (#3345)
- [`05c134c783`](https://github.com/dashpay/dash/commit/05c134c783) Fix litemode vs txindex check (#3355)
- [`c9881d0fc7`](https://github.com/dashpay/dash/commit/c9881d0fc7) Masternodes must have required services enabled (#3350)
- [`c6911354a1`](https://github.com/dashpay/dash/commit/c6911354a1) Few tweaks for MakeCollateralAmounts (#3347)
- [`56b8e97ab0`](https://github.com/dashpay/dash/commit/56b8e97ab0) Refactor and simplify PrivateSend based on the fact that we only mix one single denom at a time now (#3346)
- [`af7cfd6a3f`](https://github.com/dashpay/dash/commit/af7cfd6a3f) Define constants for keys in CInstantSendDb and use them instead of plain strings (#3352)
- [`d52020926c`](https://github.com/dashpay/dash/commit/d52020926c) Fix undefined behaviour in stacktrace printing. (#3357)
- [`4c01ca4573`](https://github.com/dashpay/dash/commit/4c01ca4573) Fix undefined behaviour in unordered_limitedmap and optimise it. (#3349)
- [`2521970a50`](https://github.com/dashpay/dash/commit/2521970a50) Add configurable devnet quorums (#3348)
- [`7075083f07`](https://github.com/dashpay/dash/commit/7075083f07) Detect mixing session readiness based on the current pool state (#3328)
- [`dff9430c5e`](https://github.com/dashpay/dash/commit/dff9430c5e) A couple of fixes for CActiveMasternodeManager::Init() (#3326)
- [`4af4432cb9`](https://github.com/dashpay/dash/commit/4af4432cb9) Add unit tests for CPrivateSend::IsCollateralAmount (#3310)
- [`e1fc378ffd`](https://github.com/dashpay/dash/commit/e1fc378ffd) Refactor PS a bit and make it so that the expected flow for mixing is to time out and fallback (#3309)
- [`39b17fd5a3`](https://github.com/dashpay/dash/commit/39b17fd5a3) Fix empty TRAVIS_COMMIT_RANGE for one-commit-branch builds in Travis (#3299)
- [`f4f9f918dc`](https://github.com/dashpay/dash/commit/f4f9f918dc) [Pretty Trivial] Adjust some comments (#3252)
- [`26fb682e91`](https://github.com/dashpay/dash/commit/26fb682e91) Speed up prevector initialization and vector assignment from prevectors (#3274)
- [`9c9cac6d67`](https://github.com/dashpay/dash/commit/9c9cac6d67) Show quorum connections in "quorum dkgstatus" and use it in mine_quorum (#3271)
- [`aca6af0a0e`](https://github.com/dashpay/dash/commit/aca6af0a0e) Use smaller LLMQs in regtest (#3269)
- [`88da298082`](https://github.com/dashpay/dash/commit/88da298082) Add -whitelist to all nodes in smartfees.py (#3273)
- [`7e3ed76e54`](https://github.com/dashpay/dash/commit/7e3ed76e54) Make a deep copy of extra_args before modifying it in set_dash_test_params (#3270)
- [`75bb7ec022`](https://github.com/dashpay/dash/commit/75bb7ec022) A few optimizations/speedups for Dash related tests (#3268)
- [`2afdc8c6f6`](https://github.com/dashpay/dash/commit/2afdc8c6f6) Add basic PrivateSend RPC Tests (#3254)
- [`dc656e3236`](https://github.com/dashpay/dash/commit/dc656e3236) Bump version to 0.16 on develop (#3239)
- [`c182c6ca14`](https://github.com/dashpay/dash/commit/c182c6ca14) Upgrade Travis to use Bionic instead of Trusty (#3143)

Credits
=======

Thanks to everyone who directly contributed to this release:

- 10xcryptodev
- Akshay CM (akshaynexus)
- Alexander Block (codablock)
- Cofresi
- CryptoTeller
- dustinface (xdustinface)
- konez2k
- Oleg Girko (OlegGirko)
- PastaPastaPasta
- Piter Bushnell (Bushstar)
- sc-9310
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

- [v0.15.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.15.0.0.md) released Febrary/18/2020
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
