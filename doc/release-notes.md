# Dash Core version v24.0.1

This is a new patch version release. It fixes a problem that has delayed the
`v24` network upgrade on mainnet: `LLMQ_400_85` quorums were failing to form
because slower masternodes could not finish the last step of the DKG in time,
and the `v24` EHF signal is only attempted again once a different quorum is
selected for it. It also changes how masternodes vote on superblock triggers
so that a superblock cannot be left unpaid by a change in the approved
proposals, and fixes several other bugs.

Like v24.0.0, this release carries the `v24` network upgrade (hard fork) and is
**mandatory** for all nodes. Masternodes should upgrade to v24.0.1 as soon as
possible. All other nodes must run v24.0.0 or later before the hard fork
activates, and upgrading to v24.0.1 is strongly recommended. The upgrade itself
and everything else introduced in v24 are described in the
[v24.0.0 release notes](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-24.0.0.md).

Please report bugs using the issue tracker at GitHub:

  <https://github.com/dashpay/dash/issues>

# Upgrading and downgrading

## How to Upgrade

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Dash-Qt (on Mac) or
dashd/dash-qt (on Linux).

Upgrading from v24.0.0 needs no further steps. If you are upgrading from
v23.1.x or older, read "How to Upgrade" in the
[v24.0.0 release notes](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-24.0.0.md)
first: the first start migrates the address, spent and timestamp indexes, and
some settings changed.

## Downgrade warning

### Downgrade to v24.0.0

Downgrading to v24.0.0 is possible and needs no special steps.

### Downgrade to a version < v24.0.0

There is no supported downgrade after `v24` activation, and downgrading before
activation has caveats. See "Downgrade warning" in the
[v24.0.0 release notes](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-24.0.0.md).

# Release Notes

## v24 activation

Activation of `v24` starts with masternodes signing an EHF signal with the
`LLMQ_400_85` quorum, which needs 340 of a quorum's 400 members. On mainnet
that signal has not been signed yet. When the signal window opened, too few
members of the selected quorum had upgraded, and a masternode signs the signal
only once per quorum. The signal is attempted again when a different quorum is
selected for it, but no `LLMQ_400_85` round on mainnet has formed a quorum
since then, so the selected quorum has not changed.

- **DKG members no longer reload every verification vector at the commit
  step.** To build its commitment, and to verify each commitment it received,
  a member read every other member's verification vector back from disk. For a
  400-member quorum that took even the fastest masternodes 20 to 30 seconds
  and slower ones minutes, while the commit phase lasts only four blocks. When
  those blocks came quickly, too few members finished in time and the quorum
  did not form. The vectors are now kept in memory for the duration of the DKG
  session, and a masternode handles its own DKG messages before queued
  messages from peers.
  A masternode benefits as soon as it is upgraded, including in quorums whose
  other members still run v24.0.0. (#8135, #8130)
- A quorum commitment is no longer dropped from a node's pool when the block
  that carried it is rejected for an unrelated reason. Previously the
  commitment was lost on every node that had validated that block and the
  quorum was not mined in that DKG cycle. (#8104)

An `LLMQ_400_85` DKG runs once a day. Each time a different quorum is selected
for the signal, its members sign it, including members that run v24.0.0, and
the signal is completed if 340 of the quorum's 400 members run v24.0.0 or
later and are online. The selected quorum changes at the latest once four new
quorums have formed.

The parameters of the `v24` deployment are unchanged from v24.0.0. Miner
signaling still starts at the first 4032-block window boundary after the EHF
signal is mined, so the earliest possible activation date given in the v24.0.0
release notes moves back by about a week for every window boundary that passes
before the signal is mined.

## Governance

- A masternode now waits until a superblock trigger for its own candidate
  exists before it votes NO-FUNDING on other triggers, and casts those NO
  votes at the latest three blocks before the superblock. Previously it voted
  NO on every trigger that did not match its candidate at that block and never
  revisited the vote. If the set of approved proposals changed while triggers
  were being created, masternodes could end up having voted NO on the trigger
  that matched their final candidate, and the superblock could pay no
  proposals. (#8131)
- After a cycle that ended without a funded trigger, a masternode that had
  voted YES in that cycle no longer votes NO on the next cycle's trigger.
  (#8131)
- On test networks, a node restarted inside the maturity window no longer
  drops its stored triggers and then rejects the superblock. Mainnet was not
  affected. (#8097)

Masternodes running older versions keep the old voting behaviour, so this
protection becomes effective as masternodes upgrade. Masternode operators
should upgrade before a superblock's maturity window opens, which is 1662
blocks before the superblock. For the superblock at block 2575480 the window
opens at block 2573818 (around November 17).

## Other fixes

- Fixed an assertion failure in governance that could stop a node after a
  reorg that changed the block in which a masternode registration was mined.
  The failure repeated on every start until `governance.dat` was deleted.
  (#8100)
- The network thread no longer spins at full CPU while a peer's send buffer is
  full. (#8094)
- `GET /rest/block/<hash>.json` no longer terminates the node when the block
  contains a provider transaction whose payload can't be decoded. (#8099)
- GUI: a shared masternode contribution can be reserved again after it was
  released. (#8052)

## Build and tests

- The depends build fetches the miniupnpc sources from a location that still
  serves them. (#8088)
- `test_dash-qt` no longer hangs or crashes when built against a distribution
  Qt that uses the GLib event loop. (#8080)

# v24.0.1 Change log

See detailed [set of changes][set-of-changes].

# Credits

Thanks to everyone who directly contributed to this release:

- Konstantin Akimov
- PastaPastaPasta

As well as everyone that submitted issues, reviewed pull requests and helped
debug the release candidates.

# Older releases

These releases are considered obsolete. Old release notes can be found here:

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

[set-of-changes]: https://github.com/dashpay/dash/compare/v24.0.0...dashpay:v24.0.1
