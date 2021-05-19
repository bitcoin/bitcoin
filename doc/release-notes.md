Dash Core version 0.17.0.1
==========================

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

### Downgrade of masternodes to < 0.17.0.1

Starting with the 0.16 release, masternodes verify the protocol version of other
masternodes. This results in PoSe punishment/banning for outdated masternodes,
so downgrading even prior to the activation of the introduced hard-fork changes
is not recommended.

Notable changes
===============

Opcode updates
--------------
Several opcodes have been reactivated/introduced to broaden the functionality
of the system and enable developers to build new solutions. These opcodes are
a combination of previously disabled ones that have been found to be safe and
new ones previously introduced by Bitcoin Cash. Details of the opcodes are
provided in [DIP-0020](https://github.com/dashpay/dips/blob/master/dip-0020.md).

These opcodes are activated via a BIP9 style hard fork that will begin
signalling on July 1st using bit 6. Any nodes that do not upgrade by the time
this feature is activated will diverge from the rest of the network.

DKG Data Sharing
----------------
Quorum resilience has been improved by enabling masternodes to request DKG data
from other quorum members. This allows Dash Platform to obtain required
information while also making it possible for corrupted masternodes to recover
the DKG data they need to participate in quorums they are part of. Details are
provided in [DIP-0021](https://github.com/dashpay/dips/blob/master/dip-0021.md).

Platform support
----------------
Support for Dash Platform has been expanded through the addition of a new
quorum type `LLMQ_100_67`, several RPCs, and a way to limit Platform RPC access
to a subset of allowed RPCs, specifically:
- `getbestblockhash`
- `getbestchainlock`
- `getblockcount`
- `getblockhash`
- `quorum sign` (for platform quorums only)
- `quorum verify`
- `verifyislock`

These changes provide necessary Platform capabilities while maximizing the
isolation between Core and Platform. New quorum type will be activated using
the same bit 6 introduced to activate new opcodes.

BLS update
----------
Dash Core’s BLS signature library has been updated based on v1.0 of the
Chia BLS library to support migration to a new BLS signature scheme which will
be implemented in a future version of DashCore. These changes will be made to
align with standards and improve security.

Network performance improvements
--------------------------------
This version of Dash Core includes multiple optimizations to the network and
p2p message handling code.

We reintroduced [Intra-Quorum Connections](https://github.com/dashpay/dips/blob/master/dip-0006.md#intra-quorum-communication)
which were temporary disabled with the introduction of
`SPORK_21_QUORUM_ALL_CONNECTED`. This should make communications for masternodes
belonging to the same quorum more robust and improve network connectivity in
general.

FreeBSD and MacOS machines will now default to the new `kqueue` network mode
which is similar to `epoll` mode on linux-based. It removes most of the CPU
overhead caused by the sub-optimal use of `select` in the network thread when
many connections were involved.

Wallet changes
--------------
Upgrading a non-Hierarchical Deterministic (HD) wallet to an HD wallet is now
possible. Previously new HD wallets could be created, but non-HD wallets could
not be upgraded. This update will enable existing non-HD wallets to upgrade to
take advantage of HD wallet features. Upgrades can be done via either the debug
console or command line and a new backup must be made when this upgrade is
performed.

Users can now load and unload wallets dynamically via RPC and GUI console.
It's now possible to have multiple wallets loaded at the same time, however only
one of them is going to be active and receive updates or be used for sending
trasactions at a specific point in time.

Also, enabling wallet encryption no longer requires a wallet restart.

Sporks
------
Several spork changes have been made to streamline code and improve system
reliability. Activation of `SPORK_22_PS_MORE_PARTICIPANTS` in DashCore v0.16
has rendered that spork unnecessary. The associated logic has been hardened
and the spork removed. `SPORK_21_QUORUM_ALL_CONNECTED` logic has been split
into two sporks, `SPORK_21_QUORUM_ALL_CONNECTED` and `SPORK_23_QUORUM_POSE`,
so that masternode quorum connectivity and quorum Proof of Service (PoSe) can
be controlled independently. Finally, `SPORK_2_INSTANTSEND_ENABLED` has a new
mode (value: 1) that enables a smooth transition in case InstantSend needs to
be disabled.

Statoshi backport
------------------
This version includes a [backport](https://github.com/dashpay/dash/pull/2515)
of [Statoshi functionality](https://github.com/jlopp/statoshi) which allows
nodes to emit metrics to a StatsD instance. This can help node operators to
learn more about node performance and network state in general. We added
several command line options to give node operators more control, see options
starting with `-stats` prefix.

PrivateSend rename
------------------
PrivateSend has been renamed to CoinJoin to better reflect the functionality
it provides and align with industry standard terminology. The renaming only
applies to the UI and RPCs but does not change functionality.

Build system
------------
Multiple packaged in `depends` were updated. Current versions are:
- biplist 1.0.3
- bls-dash 1.0.1
- boost 1.70.0
- cmake 3.14.7
- expat 2.2.5
- mac alias 2.0.7
- miniupnpc 2.0.20180203
- qt 5.9.6
- zeromq 4.2.3

Packages libX11, libXext, xextproto and xtrans are no longer used.

Minimum supported macOS version was bumped to 10.10.

RPC changes
-----------
There are seven new RPC commands which are Dash specific and seven new RPC
commands introduced through Bitcoin backports. One previously deprecated RPC,
`estimatefee`, was removed and several RPCs have been deprecated.

The new RPCs are:
- `createwallet`
- `getaddressesbylabel`
- `getaddressinfo`
- `getzmqnotifications`
- `gobject list-prepared`
- `listlabels`
- `masternode payments`
- `quorum getdata`
- `quorum verify`
- `signrawtransactionwithkey`
- `signrawtransactionwithwallet`
- `unloadwallet`
- `verifychainlock`
- `verifyislock`
- `upgradetohd`

The deprecated RPCs are all related to the deprecation of wallet accounts and
will be removed in DashCore v0.18. Note that the deprecation of wallet accounts
means that any RPCs that previously accepted an “account” parameter are
affected — please refer to the RPC help for details about specific RPCs.

The deprecated RPCs are:
- `getaccount`
- `getaccountaddress`
- `getaddressbyaccount`
- `getreceivedbyaccount`
- `listaccounts`
- `listreceivedbyaccount`
- `masternode current`
- `masternode winner`
- `move`
- `sendfrom`
- `setaccount`

`protx register` and `protx register_fund` RPCs now accept an additional `submit` parameter
which allows producing and printing ProRegTx-es without relaying them
to the network.

Also, please note that all mixing-related RPCs have been renamed to replace
“PrivateSend” with “CoinJoin” (e.g. `setprivatesendrounds` -> `setcoinjoinrounds`).

Please check `help <command>` for more detailed information on specific RPCs.

Command-line options
--------------------
Changes in existing cmd-line options:

New cmd-line options:
- `-dip8params`
- `-llmq-data-recovery`
- `-llmq-qvvec-sync`
- `-llmqinstantsend`
- `-platform-user`
- `-statsenabled`
- `-statshost`
- `-statshostname`
- `-statsport`
- `-statsns`
- `-statsperiod`
- `-zmqpubhashrecoveredsig`
- `-zmqpubrawrecoveredsig`

Also, please note that all mixing-related command-line options have been
renamed to replace “PrivateSend” with “CoinJoin” (e.g. `setprivatesendrounds`
-> `setcoinjoinrounds`).

Please check `Help -> Command-line options` in Qt wallet or `dashd --help` for
more information.

Backports from Bitcoin Core 0.17
--------------------------------

This release also introduces over 450 updates from Bitcoin v0.17 as well as
some updates from Bitcoin v0.18 and more recent versions. This includes a
number of performance improvements, dynamic loading of wallets via RPC, support
for signalling pruned nodes, and a number of other updates that will benefit
Dash users. Bitcoin changes that do not align with Dash’s product needs, such
as SegWit and RBF, are excluded from our backporting. For additional detail on
what’s included in Bitcoin v0.17, please refer to [their release notes](https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.17.0.md).

Miscellaneous
-------------
A lot of refactoring, code cleanups and other small fixes were done in this release.

0.17.0.1 Change log
===================

See detailed [set of changes](https://github.com/dashpay/dash/compare/v0.16.1.1...dashpay:v0.17.0.1).

Credits
=======

Thanks to everyone who directly contributed to this release:

- 10xcryptodev
- dustinface (xdustinface)
- Kittywhiskers Van Gogh (kittywhiskers)
- Kolby Moroz Liebl (KolbyML)
- Konstantin Shuplenkov (shuplenkov)
- Minh20
- PastaPastaPasta
- strophy
- thephez
- tomthoros
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

- [v0.16.1.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.16.1.1.md) released November/17/2020
- [v0.16.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.16.1.0.md) released November/14/2020
- [v0.16.0.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.16.0.1.md) released September/30/2020
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
