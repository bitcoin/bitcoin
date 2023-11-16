# Dash Core version v19.2.0

Release is now available from:

  <https://www.dash.org/downloads/#wallets>

This is a new minor version release, bringing various bugfixes and other
improvements.

This release is mandatory for all nodes. This release resolves issues around the
v19 Hard Fork activation. All nodes must upgrade to continue syncing properly.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/dashpay/dash/issues>


# Upgrading and downgrading

## How to Upgrade

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Dash-Qt (on Mac) or
dashd/dash-qt (on Linux). If you upgrade after DIP0003 activation and you were
using version < 0.13 you will have to reindex (start with -reindex-chainstate
or -reindex) to make sure your wallet has all the new data synced. Upgrading
from version 0.13 should not require any additional actions.

At the first startup Dash Core will run a migration process which can take
anywhere from a few minutes to thirty minutes to finish. After the migration,
a downgrade to an older version is only possible with a reindex.

Please note that seamless migration is only possible on nodes which did not
reach the v19 fork block. Nodes that reached it can either rewind a couple
of blocks back to a pre-v19 block using `invalidateblock` RPC while still
running the old version or they can reindex instead.

## Downgrade warning

### Downgrade to a version < v19.2.0

Downgrading to a version older than v19.2.0 is not supported due to changes
in the evodb database. If you need to use an older version, you must either
reindex or re-sync the whole chain.

# Notable changes

## Resolve v19 Hard Fork Issues

One of the goals for the v19 Hard Fork was to activate basic BLS scheme and
start using it in various on-chain and p2p messages. The motivation behind this
change is the need to be aligned with IETF standards. Unfortunately, a few edge
cases were missed in our functional tests and were not caught on testnet either.
v19 activation attempt on mainnet hit one of these edge cases and mainnet
stopped producing blocks. As an intermediate solution v19.1.0 was released which
delayed the start of the signaling for the v19 Hard Fork until June 14th.

To resolve these issues we had to rework the way BLS public keys are handled
including the way they are serialized in the internal database. This made it
incompatible with older versions of Dash Core, so a db migration path was
implemented for all recent versions.

## Improve migration and historical data support on light clients

As a side-effect, the solution implemented to resolve v19 Hard Fork issues
opened a path to simplify v19 migration for mobile wallets.

With previous implementation mobile wallets would have to convert 4k+ pubkeys
at the v19 fork point and that can easily take 10-15 seconds if not more.
Also, after the v19 Hard Fork, if a masternode list is requested from a block
before the v19 Hard Fork, the operator keys were coming in basic BLS scheme,
but the masternode merkleroot hash stored in coinbase transaction at that time
was calculated with legacy BLS scheme. Hence it was impossible to verify the
merkleroot hash.

To fix these issues a new field `nVersion` was introduced for every entry in
`mnlistdiff` p2p message. This field indicates which BLS scheme should be used
when deserialising the message - legacy or basic. `nVersion` of the `mnlistdiff`
message itself will no longer indicate the scheme and must always be set to `1`.

## Improve mixing support on light clients

Recent changes to `dsq` and `dstx` messages allowed mobile clients that get
masternode lists from `mnlistdiff` message to determine the masternode related
to these messages because the `proTxHash` was used instead of the
`masternodeOutpoint`. Once the v19 Hard Fork activates the signature of `dsq`
and `dstx` messages will be based on the `proTxHash` which should make it
possible for mobile clients to verify it.

## Allow keeping Chainlocks enabled without signing new ones

Before this version Chainlocks were either enabled or disabled. Starting with
this version it's possible to set `SPORK_19_CHAINLOCKS_ENABLED` to a non-zero
value to disable the signing of new Chainlocks while still enforcing the best
known one.

## Other changes

There were a few other minor changes too, specifically:
- reindex on DB corruption should now start properly in Qt
- a mnemonic passphrase longer than 256 symbols no longer crashes the wallet
- a Qt node running with `-disablewawllet` flag should not crash in Settings now
- `-masternodeblsprivkey` and `-sporkkey` values are no longer printed in
`debug.log`
- should use less memory in the long run comparing to older versions
- gmp library detection should work better on macos
- fixed a couple of typos

# v19.2.0 Change log

See detailed [set of changes](https://github.com/dashpay/dash/compare/v19.1.0...dashpay:v19.2.0).

# Credits

Thanks to everyone who directly contributed to this release:

- Kittywhiskers Van Gogh (kittywhiskers)
- Konstantin Akimov (knst)
- Nathan Marley (nmarley)
- Odysseas Gabrielides (ogabrielides)
- PastaPastaPasta
- thephez
- UdjinM6

As well as everyone that submitted issues, reviewed pull requests and helped
debug the release candidates.

# Older releases

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

- [v19.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-19.1.0.md) released May/22/2023
- [v19.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-19.0.0.md) released Apr/14/2023
- [v18.2.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.2.2.md) released Mar/21/2023
- [v18.2.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.2.1.md) released Jan/17/2023
- [v18.2.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.2.0.md) released Jan/01/2023
- [v18.1.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.1.1.md) released January/08/2023
- [v18.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.1.0.md) released October/09/2022
- [v18.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.0.2.md) released October/09/2022
- [v18.0.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.0.1.md) released August/17/2022
- [v0.17.0.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.17.0.3.md) released June/07/2021
- [v0.17.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.17.0.2.md) released May/19/2021
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
