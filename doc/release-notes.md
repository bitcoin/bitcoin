# Dash Core version v24.0.3

This is a new patch version release with bug fixes, most of them for edge
cases, across governance, CoinJoin, masternodes, the wallet, the GUI and RPC.

Like v24.0.0, v24.0.1 and v24.0.2, this release carries the `v24` network
upgrade (hard fork), which is **mandatory** for all nodes: every node must run
v24.0.0 or later before the hard fork activates. Upgrading to v24.0.3 is
recommended for all nodes. The upgrade itself and everything else introduced
in v24 are described in the
[v24.0.0 release notes](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-24.0.0.md),
the fixes for `v24` activation in the
[v24.0.1 release notes](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-24.0.1.md),
and the memory leak fix in the
[v24.0.2 release notes](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-24.0.2.md).

Please report bugs using the issue tracker at GitHub:

  <https://github.com/dashpay/dash/issues>

# Upgrading and downgrading

## How to Upgrade

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Dash-Qt (on Mac) or
dashd/dash-qt (on Linux).

Upgrading from v24.0.0, v24.0.1 or v24.0.2 needs no further steps. If you are
upgrading from v23.1.x or older, read "How to Upgrade" in the
[v24.0.0 release notes](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-24.0.0.md)
first: the first start migrates the address, spent and timestamp indexes, and
some settings changed.

## Downgrade warning

### Downgrade to v24.0.0, v24.0.1 or v24.0.2

Downgrading to v24.0.0, v24.0.1 or v24.0.2 is possible and needs no special
steps.

### Downgrade to a version < v24.0.0

There is no supported downgrade after `v24` activation, and downgrading before
activation has caveats. See "Downgrade warning" in the
[v24.0.0 release notes](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-24.0.0.md).

# Release Notes

## Governance

- Create Proposal dialog (Qt): the payment window is now based on the chosen
  superblocks instead of the time the proposal was created. Previously, a
  payment could come at a different superblock than the one chosen, or be
  missed. The dialog also no longer offers superblocks whose voting deadline
  has passed. (#8154)
- Hardened the cache of governance votes that arrive before their object.
  (#8122)

## Reindex and indexes

- The node now flushes all of its databases once a reindex completes, so an
  unclean stop right after a reindex no longer needs another reindex. A node
  that already fails to start for this reason needs one more reindex. (#8103)
- Fixed index startup after an unclean stop with `-addressindex`,
  `-spentindex` or `-timestampindex`. (#8047)

## Wallet

- Mnemonic validation now checks every BIP39 checksum bit, so mnemonics with
  an invalid checksum are refused instead of creating or restoring a different
  wallet. Existing wallet files are not affected, but such a mnemonic can no
  longer be used to restore one. (#8111)
- Fixed a race in block-filter rescans (`-blockfilterindex`) that could miss
  transactions when descriptors changed during the rescan. (#8139)

## CoinJoin

- Masternodes check participants' signatures against the final mixing
  transaction. (#7815)
- A participant with several unsigned inputs is now counted once when choosing
  whose collateral to charge. (#8116)
- Mixing clients reject queue announcements with an invalid denomination, as
  masternodes already do. (#7955)

## Masternodes, quorums and sporks

- Nodes now keep the final quorum commitment with the most signers. (#8085)
- A signature share dropped because the pending queue was full is requested
  again. (#7994)
- Spork updates signed within the same second are now accepted by other
  nodes. (#8127)
- Re-signing an InstantSend or ChainLock request restarts the signing
  session's timeout. (#8203)
- Quorum lists used for ChainLocks and InstantSend are now cached only for the
  active chain, so a node no longer needs a restart to accept ChainLocks after
  some reorgs. (#8102)

## GUI

- Address book, Coin Control, options dialog, PSBT dialog, CoinJoin send tab,
  shared masternode dialogs, wallet migration, proposal list, traffic graph and
  overview page fixes. (#8123, #8124)

## RPC

- Quorum RPCs reject quorum types outside 0..255 instead of truncating them.
  (#7954)
- `masternode payments`, `masternode winners`, `getspecialtxes`,
  `verifyislock`, `getaddressdeltas`, `evodb verify`/`repair` and
  `getblockheaders`: argument handling, range checks and locking fixes.
  (#8118, #8125)
- `getdeploymentinfo` and REST `/deploymentinfo` now return an error for
  blocks that are not fully validated. (#8098)

## Contrib

- Fixed the OpenRC init script, `contrib/zmq/zmq_sub.py` on Python 3.14 and
  IPv6 handling in `contrib/qos/tc.sh`. (#8109)

# v24.0.3 Change log

See detailed [set of changes][set-of-changes].

# Credits

Thanks to everyone who directly contributed to this release:

- PastaPastaPasta

As well as everyone that submitted issues and reviewed pull requests.

# Older releases

These releases are considered obsolete. Old release notes can be found here:

- [v24.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-24.0.2.md) released Nov/14/2026
- [v24.0.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-24.0.1.md) released Nov/13/2026
- [v24.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-24.0.0.md) released Nov/5/2026
- [v23.1.10](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.1.10.md) released Nov/2/2026
- [v23.1.9](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.1.9.md) released Oct/29/2026
- [v23.1.8](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.1.8.md) released Jul/29/2026
- [v23.1.7](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.1.7.md) released Jun/30/2026
- [v23.1.5](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.1.5.md) released Jun/19/2026
- [v23.1.4](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.1.4.md) released Jun/18/2026
- [v23.1.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.1.3.md) released May/28/2026
- [v23.1.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.1.2.md) released Mar/12/2026
- [v23.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.1.0.md) released Feb/15/2026
- [v23.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.0.2.md) released Dec/4/2025
- [v23.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.0.0.md) released Nov/10/2025
- [v22.1.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-22.1.3.md) released Jul/15/2025
- [v22.1.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-22.1.2.md) released Apr/15/2025
- [v22.1.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-22.1.1.md) released Feb/17/2025
- [v22.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-22.1.0.md) released Feb/10/2025
- [v22.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-22.0.0.md) released Dec/12/2024
- [v21.1.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-21.1.1.md) released Oct/22/2024
- [v21.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-21.1.0.md) released Aug/8/2024
- [v21.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-21.0.2.md) released Aug/1/2024
- [v21.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-21.0.0.md) released Jul/25/2024
- [v20.1.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.1.1.md) released April/3/2024
- [v20.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.1.0.md) released March/5/2024
- [v20.0.4](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.0.4.md) released Jan/13/2024
- [v20.0.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.0.3.md) released December/26/2023
- [v20.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.0.2.md) released December/06/2023
- [v20.0.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.0.1.md) released November/18/2023
- [v20.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.0.0.md) released November/15/2023
- [v19.3.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-19.3.0.md) released July/31/2023
- [v19.2.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-19.2.0.md) released June/19/2023
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
- [v0.15.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.15.0.md) released February/18/2020
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
- [v0.12.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.0.md) released Aug/15/2015
- [v0.11.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.2.md) released Mar/04/2015
- [v0.11.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.1.md) released Feb/10/2015
- [v0.11.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.0.md) released Jan/15/2015
- [v0.10.x](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.10.0.md) released Sep/25/2014
- [v0.9.x](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.9.0.md) released Mar/13/2014

[set-of-changes]: https://github.com/dashpay/dash/compare/v24.0.2...dashpay:v24.0.3
