Release Notes for Bitcoin Unlimited v1.0.0
==========================================

Bitcoin Unlimited version 1.0.0 is now available from:

  <https://bitcoinunlimited.info/download>

Please report bugs using the issue tracker at github:

  <https://github.com/BitcoinUnlimited/BitcoinUnlimited/issues>

The third official BU client release reflects our opinion that Bitcoin full-node
software has reached a milestone of functionality, stability and scalability.
Hence, completion of the alpha/beta phase throughout 2009-16 can be marked in
our release version 1.0.0.

The most important feature of BU's first general release is functionality to
restore market dynamics at the discretion of the full-node network. Activation
will result in eliminating the full-blocks handicap, restoring a healthy
fee-market, allow reliable confirmation times, fair user fees, and re-igniting
stalled network effect growth, based on Bitcoin Unlimited's Emergent Consensus
model to let the ecosystem decide the best values of parameters like the maximum
block size.

Bitcoin Unlimited open-source version 1.0.0 contains a large number of changes,
updates and improvements. Some optimize the Emergent Consensus logic (EC),
others improve the system in a more general way, and a number of updates are
imported from the open source work of the Bitcoin Core developers who deserve
full credit for their continued progress.


Upgrading
=========

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Bitcoin-Qt (on Mac) or
bitcoind/bitcoin-qt (on Linux).

Changes are as follows:

Emergent Consensus (EC)
=======================

Set Accepted Depth (AD) to 12 as default.
-----------------------------------------

BU has a "sticky gate" (SG) mechanism which supports the functioning of EC.  SG
means that if blocks are propagated which are bigger than a specific node's
"Excessive Block Size" (EB) and subsequent blocks reach the node's "Acceptance
Depth" (AD), then the node accepts, for 144 blocks, every block up to the size
of its EB * 16.  This mechanism is needed to ensure that nodes maintain
consensus during an uptick in the network's record block size seen and written
to the blockchain.

In co-operation with Bitcoin XT a potential attack on SG has been identified. A
minority hash-power attacker is probabilistically able to produce AD+1 blocks
before the majority produces AD blocks, so it is possible that a sequence of
very large blocks can be permanently included in the blockchain.  To make this
attack much less likely and more expensive, the standard setting for Acceptance
Depth is increased to 12. This value is the result of analysis and simulation.
Note that due to variation in node settings only a subset of nodes will be in
an SG situation at any one time.

Bitcoin Unlimited recommends miners and commercial node users set their
Accepted Depth parameter to the value of 12.

Further information:
- [Discussion of Sticky Gate / the attack Table with result of simulation](https://bitco.in/forum/threads/buip038-closed-revert-sticky-gate.1624/)


Flexible Signature Operations (sigops) Limit and an Excessive Transaction Size
------------------------------------------------------------------------------

When a Bitcoin node validates transactions, it verifies signatures. These
operations - Signature Operation, short SigOps - have CPU overhead. This makes
it possible to build complex transactions which require significant time to be
validated. This has long been a major concern against bigger blocks that tie up
node processing and affect the synchronicity of the the network, however, a
"gossip network" is inherently more robust than other types.

BUIP040 introduces several changes to mitigate these attacks while adhrering to
the BU philosophy. The pre-existing limit of 20,000 per MB block size is enough
to enable a wide scope of transactions, but mitigates against attacks. A block
size of 1 MB permits 20,000 SigOps, a blocksize of 2 MB permits 40,000 SigOps
and so on.

Attention: As this fix is consensus critical, other implementation which allow
blocks larger than 1MB should be aware of it. Please refer to BUIP040 for the
exact details.

Additionally BU has a configurable "excessive transaction size" parameter. The
default maximum size is limited to 1MB. Blocks with a transaction exceeding this
size will be simply rejected.

It is possible to configure this value, in case the network has the capacity and
a use-cases for allowing transactions >1MB. The command for the bitcoin.conf is
net.excessiveTx=1000000. But it is not expected that a change in this setting
will be network-wide for many years, if ever.
Attention: It is recommended to use the defaults.

Further information:
- [Github Repository PR#164](https://github.com/BitcoinUnlimited/BitcoinUnlimited/pull/164)
- [Discussion and Vote on BUIP40](https://bitco.in/forum/threads/buip040-passed-emergent-consensus-parameters-and-defaults-for-large-1mb-blocks.1643/)

Removal of the 32 MB message limit
----------------------------------

Satoshi Nakamoto implemented a 32MB message limit in Bitcoin. To prevent any
future controversy about a hard fork to increase this limit, BU has removed it
in advance with the expectation that, when it will be reached years ahead,
everybody will have already upgraded their software. The maximum message limit
varies from node to node as it equals excessive block size * 16.

P2P Network
===========

Wide-spectrum Anti-DoS improvements
-----------------------------------

Network traffic represents a grey-scale of useful activity. Some helps the
network to synchronize, while some is for surveillance and deliberate wastage.
Bitcoin Unlimited differentiates useful and less useful data between nodes.

Useful data is primarily valid transactions and blocks, invitations for required
data, and handshake messages.  Not useful data are typically from from spam or
spy nodes, From version 1.0.0, Bitcoin Unlimited Nodes track if peers supply
useful data.

1) All traffic is tracked by byte count, per connection, so the "usefulness" of
data can be measured for what each node is sending or requesting from a BU node.
On inbound messages, useful bytes are anything that is NOT: PING, PONG, VERACK,
VERSION or ADDR messages. Outbound are the same but includes transaction INV.
Furthermore the useful bytes are decayed over time, usually a day.

2) if a peer node does nothing useful within two minutes then the BU node chokes
their INV traffic. A lot of spam nodes normally listen to INV traffic, and this
is first of all a security concern as it is undesirable to allow anyone to just
try and collate INV traffic, but also it's a large waste of bandwidth. So,
quickly they are choked off but are still sent block INV's in case they are
blocks-only nodes.

3) If rogue nodes try to reacquire a connection aggressively a BU node gives
them a 4 hour ban.

There are other types of spam nodes that will do this, so using these three
approaches keeps BU connection slots available for useful nodes and also for the
bit-nodes crawlers that want to find the node counts.

Further information:
- [GitHub #PR 62](https://github.com/BitcoinUnlimited/BitcoinUnlimited/pull/62)

Request Manager extensions
--------------------------

Bitcoin Unlimited uses a Request Manager that tracks source nodes for blocks and
transactions. If a request is filled slowly, it reissues the same request to a
different source instead of just waiting and eventually timing out. The Request
Manager was introduced to stabilize Xthin block propagation, but is generally
helpful to improve the synchronization of the mempool and to manage resources
efficiently.

Because synchronized mempools are important for Xpedited (unsolicited Xthin) it
is "bad" for them to be non-synchronized when a node loses track of a source.
Also, if an xpedited block comes in, some transactions may be missing so we can
look up sources for them in the request manager! We don't want to request from
the xpedited source if we can avoid it, because that may be across the GFC. It
is always better to get the transactions from a local source.

With this release, the Request Manager's functionality is extended to the
Initial Blockchain Download (IBD). It eliminates slowdowns and hung connections
which occasionally happen when the request for blocks receives a slow response.

Further information:
- [GitHub PR #79](https://github.com/BitcoinUnlimited/BitcoinUnlimited/pull/79)
- [GitHub PR #229](https://github.com/BitcoinUnlimited/BitcoinUnlimited/pull/229)

Xthin block propagation optimizations
-------------------------------------

Based on many months of live traffic observation, Bitcoin Unlimited implemented
several general optimizations of the block propagation with Xthin. As tests have
shown, this enables some nodes to operate with zero missing transactions in a
24-hour period and thus achieves extremely efficient use of bandwidth resources
and minimal latency in block propagation.


Further information:
- [GitHub PR #131](https://github.com/BitcoinUnlimited/BitcoinUnlimited/pull/131)
- [GitHub PR #173](https://github.com/BitcoinUnlimited/BitcoinUnlimited/pull/173)
- [GitHub PR #174](https://github.com/BitcoinUnlimited/BitcoinUnlimited/pull/174)
- [GitHub PR #176](https://github.com/BitcoinUnlimited/BitcoinUnlimited/pull/176)
- [GitHub PR #191](https://github.com/BitcoinUnlimited/BitcoinUnlimited/pull/191)

Mempool Management
==================

Orphan Pool transaction management improvements
-----------------------------------------------

Not only blocks, but also transactions can be orphaned. A typical reason is when
the transaction's parent is missing in the mempool. In event of a growing
backlog or of long transaction chains the number of orphaned transactions
increases making management of the mempool resource intensive, further, it
widens a vector for DoS attacks.

The orphan pool is an area that affects Xthin's performance, so managing the
pool by evicting old orphans is important for bloom filter size management. The
number of orphans should reduce once larger blocks occur and mempools are
naturally of a smaller size. Occasionally they reach the current limit of 5000
and their source is not always clear: some might be intentionally created by
some miners, or are an artefact of the trickle logic.

This release restricts the size of orphaned transactions and evicts orphans
after 72 hours. This not only improves node operation, but the synchronization
of mempool data improves Xthin efficiency.

Further information:
- [GitHub PR #42](https://github.com/BitcoinUnlimited/BitcoinUnlimited/pull/42)
- [GitHub PR #100](https://github.com/BitcoinUnlimited/BitcoinUnlimited/pull/100)

Configuration
=============

Command Tweaks
--------------

To allow a better organization of parameters in the command line or the
bitcoin.conf, Bitcoin Unlimited introduces a new format using the dot notation.
For example, to manage connections users can now specify "net.maxConnections" or
"net.maxOutboundConnections".

Parameter access is unified -- the same name is used in bitcoin.conf, via
command line arguments, or via bitcoin-cli (et al). Old parameters in the legacy
format are still supported, while new parameters, like the excessive block size,
are only implemented in the new format.
Try `bitcoin-cli get help "\*"` to see the list of parameters.

Attention: Setting many of these parameters should be done by user with advanced
knowledge.

DNS-Seed
--------

Whenever a new node starts, it needs to find its peers. Usually this is done by
a DNS seed, which crawls the networks and builds a random list of other nodes to
which the new node can connect to. Currently there are DNS seeds from bluematt,
luke-jr, sipa, 21 and others. Bitcoin Unlimited aims to create its own DNS seed
capable of special requirements such as supporting the service bit of XTHIN. By
adopting and adjusting Sipa's code for a DNS seed Bitcoin Unlimited fixed a
minor bug.

The DNS Seed of Bitcoin Unlimited is currently activated on the NOL (No Limit)
Net, the testnet for Bitcoin Unlimited's Emergent Consensus. It is expected to
go live on the mainnet soon.

Further information:
- [Github sipa/bicoin-seeder PR# 42](https://github.com/sipa/bitcoin-seeder/pull/42)

Maintenance and Fixes
=====================

The release contains a variety of bug fixes, improvements for testing abilities
and maintenance that have been implemented since v0.12.1. Major examples are:

Reorganisation of global variables to eliminate SegFault bugs
-------------------------------------------------------------

During tests it was discovered that the shutdown of bitcoind can cause problems.
Sometimes the program wants to free memory which was used for global variables,
while the memory is already free. This can result in the so called SegFault, a
common bug of several C written programs. The cause for this bug is a
mis-organisation of the destruction order used during the shutdown. By
reorganizing global variables in a single file Bitcoin Unlimited fixed the
destruction order and eliminates this bug. This is especially helpful for
running bitcoind test suite.

Further information:
- [GitHub PR #67](https://github.com/BitcoinUnlimited/BitcoinUnlimited/pull/67)

Lock order checking in debug mode
---------------------------------

Bitcoin's debug mode checks that locks are taken in a consistent order to
eliminate the possibility of thread deadlocks. However, when locks were
destructed, they were not removed from the lock order database. Subsequently, if
another lock was allocated in the same memory position, the software would see
it as the same lock. This causes a false positive deadlock detection, and the
resulting exception limits the time that debug mode could be run to no more than
a few days.

ARM architecture
----------------

Version 1.0.0 has gitian support of deterministic builds for the ARM
architecture. This supplements Windows, iOS and Linux x86, which were previously
offered. This was backported from the Bitcoin Core project and adjusted for the
Bitcoin Unlimited environment.


Imported Commits
================

Changes from the open source Bitcoin Core Project
-------------------------------------------------

Software development is a co-operative process, and especially when several
teams work on open source projects, cooperation becomes a major source of
success when all teams profit from the work of other teams. And while Unlimited
and Core might disagree on some concepts and this disagreement sometimes
dominates the public debates, both teams share the goal of an ongoing
improvement of the codebase.

Bitcoin Unlimited has therefore cherry-picked a number of Bitcoin Core 0.13.x
updates and upgrades which align with BU's onchain-scaling and wider goals to
advance Bitcoin. We would like to thank developers Cory Fields, Jonas
Schnelli, Marco Falke, Michael Ford, Patrick Strateman, Pieter Wuille, Russel
Yanofsky and Wladimir J. van der Laan for their contributions.

1.0.0 Change log
=================

A list of commits grouped by author follow. This overview separates commits applied
directly to the BU code from the ones imported from Core project. Merge commits
are not included in the list.


BU specific changes
-------------------

Justaphf (13):
- `410fa89` Fix too few addnodes breaks RPC addnodes
- `a180db6` Fix too many addnode blocks incoming connections
- `013cf0b` Added missing memory cleanup for addnode semaphore
- `e55229b` Added comments
- `1c661f4` Fixed scoping issue with LOCK statement
- `e39ed97` Fix issue with initialization on startup
- `50bad14` Fix compile error when --disable-wallet
- `37214f7` Rebrand Bitcoin Core as Bitcoin
- `beb31fe` UI: Corrected tooltips not showing on Ulimited Dlg
- `5951a5d` Trivial: Move locks in AcceptConnection
- `98e7158` Correct use-after-free in nodehandling.py test.
- `fd18503` Protect all FindNode returns against use-after-free
- `71a6c4d` [Trival] - Display witness service bit for segwit remote nodes.

SartoNess (12):
- `ddb29db` Append copyright headers
- `c64bf69` Update splashscreen
- `37fe3e8` Update LicenseInfo
- `772ba24` Update rpcuser.py
- `a2eda88` Replace Core with Unlimited
- `2597fa5` Update unlimited.cpp
- `dd90e40` Update splashscreen.cpp
- `b0d6493` Update clang-format.py
- `1a3c73b` Fixation
- `52930eb` Update COPYING

bitcoinerrorlog (1):
- `8ee64d9` Edits for grammar

deadalnix (4):
- `a990e9b` Fix LargestBlockSeen
- `452290d` Remove references to LONG_LONG_MAX
- `88d3fc2` Add sublime text project file to gitignore

dgenr8 (2):
- `c902cc4` Allow precise tracking of validation sigops/bytes
- `7db79f9` Don't allow undefined setminingmaxblock parameter

digitsu (1):
- `dae894f` moved the definition to occur before usage

freetrader (21):
- `4f5b867` [qa] Harmonize unit test suite names with filenames
- `6649e8f` [qa] comment out replace-by-fee test as long as RBF is commented out in implementation
- `0e6e117` [fix] remove duplicate bad-version check
- `c8406bc` adapt -maxuploadtarget option to set the daily buffer according to EB
- `a627cf6` [doc] adapt the reduce-traffic to reflect the change to scaling maxupload daily buffer with EB
- `b61ac0b` increase baseline SYNC_WITH_PING_TIMEOUT to 60s for my slowest test platform (no impacts on faster ones)
- `dc40b01` update comment to reflect to SYNC_WITH_PING timeout increase
- `0a84749` [qa] add capability to disable/skip tests and have them show up as such in the log
- `cae87d3` [qa] re-enable test which was skipped to demonstrate how a test can be skipped
- `71f86e9` [qa] fix up check for tests specified in command line
- `36517ed` [qa] add test_classes.py to EXTRA_DIST, for builds set up using distdir target
- `bc102d8` [qa] remove rpc-tests.sh which was replaced by Python script
- `a7124b0` [qa] add test_classes.py to EXTRA_DIST, for builds set up using distdir target
- `28dda27` [qa] add -only-extended option to rpc-tests.py to run only extended tests
- `f78f9ac` [qa] add --randomseed option to Python test framework
- `e9bba70` [qa] print out seed in BitcoinTestFramework instead of requiring tests to do it
- `e2d6b1d` [qa] added doctest to test_classes.py to verify that we can get the full command
- `3f55a97` [qa] usability enhancements for rpc-tests.py: listing, better option handling, summary of test results
- `99620ff` [qa] show DISABLED, SKIPPED in summary list of test results
- `ba8f167` [qa] return exit code 1 if any tests failed (since we catch and collect test failures now for reporting)
- `8dfbb5c` add netstyle for 'nol' network

gandrewstone (47):
- `523d465` fix .deb file generation
- `fdf065a` fix a seg fault when ctrl-c, add some logging, issue blocks to req mgr whenever INVs come in, fix wallet AcceptToMemPool and SyncWithWallets
- `f5bf490` fixes to run some regression tests in BU
- `9e2b8ec` re-enable normal tests
- `398f51c` save the debug executables for crash analysis before stripping for release
- `f0db510` validate header of incoming blocks from BLOCK messages or rpc
- `915167f` fix use after erase
- `0b6d71f` remove default bip109 flag since BUIP005 signaling is now used in block explorers
- `fd206f2` add checks and feedback to setexcessiveblock cli command
- `a1ee57b` add CTweak class that gives lets you define an object that can be configured in the CLI, command line or conf file in one declaration.
- `629c524` fix execution of excessive unit test
- `a78e8e4` fix excessive unit test given new constraint
- `f8a1fb7` add a parameter to getpeerinfo that selects the node to display by IP address
- `e7c208f` isolate global variables into a separate file to control construction/destruction order
- `b7411da` move global vars into globals.cpp
- `192bffc` reorganise global variables into a single file so that constructor destructor order is correct
- `4465a7f` Merge branch 'accurate_sighash_tracking' of https://github.com/dgenr8/bitcoin into sigop_sighash
- `33392df` move a few more variables into globals.cpp
- `d9c9948` incremental memory use as block sizes increase by reworking CFileBuffer to resize the file buffer dynamically during reindex.  remove 32MB message limit (shadowing already existing 16x EB limit).  Increase max JSON message buffer size so large RPC calls can be made (like getblocktemplate).  Add configurable proportional soft limit (uses accept depth) to sigops and max tx size
- `150c86e` sigop and transaction size limits and tests
- `3f6096e` cleanup formatting, etc
- `b95cd18` remove duplicate sighash byte count
- `8828196` move net and main cleanup out of global destructor time.
- `46c5148` The deadlock detector asserts with false positives, which are detectable because the mutexes that are supposedly in inverse order are different.
- `8c9f0cd` resolve additional CCriticalSection destruction dependencies.
- `eb7db8b` extended testing and sigops proportional limits
- `bce020d` add miner-specific doc for the EC params and pushtx
- `12f2f93` fix different handling of gt and lt in github md vs standard md in miner.md
- `d8ef429` merge and covert auto_ptr to unique_ptr
- `40079e8` Fix boost c++11 undefined symbol in boost::filesystem::copy_file, by not using copy_file
- `1c03745` rearrange location of structure decl to satisfy clang c++11 compiler (MAC)
- `595ba29` additional LONG_LONG_MAX replacements
- `c322c6f` formatting cleanup
- `e87397f` fix misspelling of subversionOverride
- `568b564` add some prints so travis knows that the test is still going
- `11e3654` move long running tests to extended
- `1e08573` in block generation, account for block header and largest possible coinbase size.  Clean up excessive tests
- `afaa951` protect global statistics object with a critical section to stop multithread simultaneous access
- `3a66102` rebase to latest core 0.12 branch
- `f247a97` resolve zmq_test hang after test is complete
- `306cc2c` change default AD higher to make it costly for a minority hash power to exceed it
- `0c067b4` bump revision to 1.0.0
- `20c1f94` fix issue where a block's coinbase can make it exceed the configured value
- `3831cc2` Add unit test for block generation, and fix a unit test issue -- an invalid configuration left by a prior test
- `5e82003` Add unit test for zero reserve block generation, and zero reserve block generation with different length coinbase messages.
- `c831c5d` shorten runtime of miner_tests because windows test on travis may be taking too long
- `b471620` bump the build number

marlengit (1):
- `847e750` Update CalculateMemPoolAncestors: Remove +1 as per BU: Fix use after free bug

nomnombtc (1):
- `4c443bf` Fix some typos in build-unix.md

ptschip (62):
- `34b4b85` Detailed Protocol Documentation for XTHINs
- `0a61f0a` Wallet sync was not happening when sending raw txn
- `1260272` Re-able check for end of iterator and general cleanup
- `3338966` Prevent the removal of the block hash from request manager too early
- `457ad84` Missing cs_main lock on mapOrphanTransactions in requestmanager
- `67983d2` Creating new Critical Section for Orphan Cache
- `aebb060` General Code cleanup for readability
- `a55b073` Do not re-request objects when rate limiting is enabled
- `378cf3d` gettrafficshaping LONG_MAX should be LONG_LONG_MAX
- `a3c7631` Request Manager Reject Messages: simplify code and clarify reject messages
- `a62c3ec` Remove duplicate logic for processing Xthins
- `e16f795` Clarify log message when xthin response is beaten
- `6368e50` Prevent thinblock from being processed when we already have it
- `efa5d92` A few more locks added for cs_orphancache
- `c0f7b24` Fix IBD hang issue by using request manager
- `9f3e7a9` Auto adjust IBD download settings depending on reponse time
- `be6923a` Remove block pacer in request manager when requesting blocks
- `3fc9dd3` Filter out the nodes with above average ping times during IBD
- `9d3336f` take out one sendexpedited() as not necessary
- `efaaa24` Add more detailed logging when expedited connect not possible
- `be095ad` Explicitly disconnect if too many connection attempts
- `2266868` Fix potential deadlock with cs_orphancache
- `30d00cf` Clarify explantion for comments
- `39eb542` Revert LogPrint statement back to "net" instead of "thin"
- `7d00163` Renable re-requests when traffic shaping is enabled
- `ad048ea` Fix regression tests.  Headers not getting downloaded.
- `e8dcc82` streamline erase orphans by time
- `8eba8da` Do not increase the re-request interval when in regtest
- `7f6352c` Sync issues in mempool_reorg.py
- `852ad18` Timing fixes for excessive.py test script
- `e10f253` Fix compiler warnings
- `8dc91de` Don't allow orphans larger than the MAX_STANDARD_TX_SIZE
- `c2133c0` Fix a few potential locking issues with cs_orphancache
- `f905207` Add CCrititicalSection for thinblock stats
- `90cf11c` Remove allowing txns with less than 150000000 in priority are always allowed
- `03b8ced` Put lock on flag fishchainnearlysyncd
- `992b252` Add unit tests for EraseOrphansByTime()
- `2caa41b` Get rid of compiler warning for GetBlockTimeout()
- `b7fb50b` Update the unit tests for thinblock_tests.cpp
- `1da0ac4` Do not queue jump GET_XTHIN in test environments
- `ea4628f` Add cs_orphancache lock to expedited block forwarding
- `46c4aae` Fix thinblock stats map interators from going out of range
- `c639235` Prevent the regression test from using a port that is arleady in use
- `fe569d5` Use Atomic for nLargestBlockSeen
- `d43b3e3` remove MainCleanup and NetCleanup from getting called twice
- `8a15e33` Move all thinblock related function into thinblock.cpp
- `2171406` Create singleton for CThinBlockStats
- `283ac8f` Take out namespace from thinblock.h
- `264683c` Make it possible to create Deterministic bloom filters for thinblocks
- `f425981` Subject coinbase spends to -limitfreerelay
- `ef32f3c` do not allow HTTP timeouts to happen in excessive.py
- `96b6ffd` Fix compiler warnings for leakybucket.h
- `20109b9` Fix maxuploadtarget.py so that it works with python3
- `1330785` Fix excessive.py python typeError
- `4d5674a` Make txn size is > 100KB for excessive.py
- `9944c95` Fix txPerf.py for python3
- `f99f956` Fix maxblocksinflight for python3
- `0e8b4db` Comptool fix for the requestmanager re-request interval
- `06dd6da` Comptool.py - replace missing leading zero from blockhash
- `7f53d6f` Improve performance of bip68-112-113-p2p
- `7226212` Renable bip69-sequence.py
- `983ce29` Fix random hang for excessive.py

sickpig (46):
- `d7120ee` Fix a few typos in Xthinblocks documentation file.
- `71aeb70` Update xthin to reflect the existance of maxoutconnnections param
- `ca1cc2d` FIX: temp band aid to fix issue #84
- `9fabc25` Fix deb package build:
- `f301a17` Add quick instructions to build and install BU binaries
- `305eadd` Reduce bip68-112-113-p2py exec time from 30 to 3 min.
- `fbef257` Use the correct repo in git clone command
- `d7dbf35` Use BU repo rather than Core.
- `6e2f7ab` Use BU repo rather than Core in gitian doc
- `d94713d` Remove leftover from a prev merge
- `f310ac6` Add instrusctions to build static binaries
- `24c89e0` Fix formatting code para in static binaries sect
- `6f5eed8` [travis] disable java based comparison test
- `6dc2eb1` [travis] Disable Qt build for i686-pc-linux-gnu platform
- `8d0e84b` [travis] Use 3 jobs for 'make check'
- `0e50b8c` [travis] Comment out Dust threshold tests.
- `8566300` Fix 3 compilation warnings.
- `e3402a1` Add copyright and licence headers
- `27994a2` Remove declaration of ClearThinblockTimer from unlimited.h
- `a60fd46` Add Travis status icon to README.md
- `a16161e` Fix README.md install instructions
- `d51e075` Fix layout and use correct packet name (libqrencode-dev)
- `49907d0` Add a default seeder for the NOL network
- `575b325` Travis and rpc tests update
- `7bcf8ce` Manual cherry-pick of Core 9ca957b
- `c95f76d` switch back to python2
- `ec95cb6` fix qt out-of-tree build
- `e31f69c` fix qt out-of-tree make deploy
- `11dd4e9` Revert "Make possible to build leveldb out of tree"
- `5536d99` leveldb: integrate leveldb into our build system
- `0ee5522` Out of tree build and rpc test update
- `2f08385` Remove a rebase leftover
- `2bc6b9d` Update depends
- `8562909` Set STRIP var to override automake strip program during make install-strip
- `3640ae7` Update config.{guess,sub} for Berkeley DB 4.8.30
- `9742f95` Add aarch64 build to Travis-ci
- `38348ff` Remove a left-over generate from the last series of cherry pick
- `042758b` Merge remote-tracking branch 'upstream/0.12.1bu' into fix/pre-190-status
- `240b6a9` [travis] Use python3 to execute rpc tests
- `fce7a14` Temporary disable zmq test on travis CI session
- `22acaaf` Reanable zmq test on travis
- `99c10e9` Fix nol seeder fqdn
- `17d2e02` Mark nol seeder as able to filter service bits
- `a7935cf` Decouple amd64/i386 gitian build from aarch64/armhf
- `0e3f81b` Fix static bitcoin-qt build for x86_64-linux-gnu arch.
- `c1861e6` Update Travis-ci status icon (release branch)

sandakersmann (1):
- `40e5d72` Changed http:// to https:// on some links


Commits imported from Core.
---------------------------

gavinandresen (1):
- `d9b8928` Allow precise tracking of validation sigops / bytes hashed

jonasschnelli (6):
- `8dee97f` [QA] add fundrawtransaction test on a locked wallet with empty keypool
- `d609895` [Wallet] Bugfix: FRT: don't terminate when keypool is empty
- `1df20f7` Only pass -lQt5PlatformSupport if >=Qt5.6
- `16572d0` Use runtime linking of QT libdbus, use custom/temp. SDK URL
- `61930df` Fix bitcoin_qt.m4 and fix-xcb-include-order.patch
- `84ea06e` Add support for dnsseeds with option to filter by servicebits

laanwj (10):
- `15502d7` Merge #8187: [0.12.2] backport: [qa] Switch to py3
- `ec0afbd` Merge #8176: [0.12.x]: Versionbits: GBT support
- `82e29e8` torcontrol: Explicitly request RSA1024 private key
- `b856580` build: Enable C++11 build, require C++11 compiler
- `1a388c4` build: update ax_cxx_compile_stdcxx to serial 4
- `46f316d` doc: Add note about new build/test requirements to release notes
- `c1b7421` Merge #9211: [0.12 branch] Backports
- `4637bfc` qt: Fix out-of-tree GUI builds
- `0291686` tests: Make proxy_test work on travis servers without IPv6
- `2007ba0` depends: Mention aarch64 as common cross-compile target

sipa (1):
- `65be87c` Don't set extra flags for unfiltered DNS seed results

theuni (17):
- `3b40197` depends: use c++11
- `9862cf6` leveldb: integrate leveldb into our buildsystem
- `452a8b3` travis: switch to Trusty
- `c5f329a` travis: 'make check' in parallel and verbose
- `6c59843` travis: use slim generic image, and some fixups
- `5e85494` build: out-of-tree fixups
- `9d31229` build: a few ugly hacks to get the rpc tests working out-of-tree
- `41b21fb` build: more out-of-tree fixups
- `76a5add` build: fix out-of-tree 'make deploy' for osx
- `a99e50f` travis: use out-of-tree build
- `adc5125` depends: allow for CONFIG_SITE to be used rather than stealing prefix
- `a74b17f` gitian: use CONFIG_SITE rather than hijacking the prefix (linux only)
- `2b56ea0` depends: only build qt on linux for x86_64/x86
- `9df22f5` build: add armhf/aarch64 gitian builds (partial cherry pick)
- `36cf7a8` Partial cherry pick f25209a
- `6ed8ab8` gitian: use a wrapped gcc/g++ to avoid the need for a system change
- `1f5d5ac` depends: fix "unexpected operator" error during "make download"

MarcoFalke (6):
- `ad99a79` [rpcwallet] Don't use floating point
- `913bbfe` [travis] Exit early when check-doc.py fails
- `29357e3` [travis] Print the commit which was evaluated
- `bd77767` [travis] echo $TRAVIS_COMMIT_RANGE
- `6592740` [gitian] hardcode datetime for depends
- `f871ff0` [gitian] set correct PATH for wrappers (partial cherry-pick)

ryanofsky (1):
- `cca151b` Send tip change notification from invalidateblock

pstratem (1):
- `f30df51` Remove vfReachable and modify IsReachable to only use vfLimited.

fanquake (4):
- `bc34fc8` Fix wake from sleep issue with Boost 1.59.0
- `ddf6e05` [depends] Latest config.guess & config.sub
- `99afa8d` [depends] OpenSSL 1.0.1k - update config_opts
- `22e47de` [trivial] Add aarch64 to depends .gitignore

Credits
=======

Thanks to all BU developers who directly contributed to this release:

- BitcoinErrorLog
- Amaury (deadalnix ) Séchet
- Tom (dgenr8) Harding
- Jerry (digitsu) Chan
- ftrader
- Justaphf
- marlengit
- nomnombtc
- Peter (ptschip) Tschipper
- sandakersmann
- SartoNess
- Andrea (sickpig) Suisani
- Andrew (theZerg) Stone
