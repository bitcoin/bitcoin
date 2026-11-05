# Dash Core version v23.1.10

This is a new patch version release. It fixes a governance issue that could
stop a superblock from paying any proposals, and hardens DKG and BLS signature
verification against misbehaving quorum members and peers.

Upgrading is **strongly recommended** for all nodes, and required for
masternodes.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/dashpay/dash/issues>

# Upgrading and downgrading

## How to Upgrade

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Dash-Qt (on Mac) or
dashd/dash-qt (on Linux).

## Downgrade warning

### Downgrade to a version < v23.0.0

Downgrading to a version older than v23.0.0 is not supported, and will
require a reindex.

# Release Notes

## Governance

- Masternodes no longer vote NO-FUNDING on a superblock trigger before that
  superblock's maturity window opens (about 3 days before the superblock on
  mainnet), when they have no candidate of their own to compare it against.
  These early votes could keep a trigger with the correct payments from
  reaching the funding threshold, so the superblock paid no proposals.
  Triggers that don't match a masternode's candidate still receive a NO vote
  once the window opens.
- Masternodes no longer vote on triggers until they are fully synced. A
  masternode restarting inside the maturity window could otherwise vote NO on
  the correct trigger before it was able to build its own candidate.

Masternodes running older versions still cast the early NO votes, so this
protection becomes effective as masternodes upgrade. Masternode operators
should upgrade before the next superblock.

## Security and denial-of-service hardening

These issues do not affect consensus and do not put funds at risk.

- BLS batch verification (signature shares, recovered signatures, InstantSend
  locks and DKG messages) now weights each signature with a secret random
  factor before aggregating. Previously two invalid signatures whose errors
  cancelled out could pass as a valid batch. A peer could use this to disrupt
  InstantSend or ChainLock signing, get honest peers penalised, or get honest
  DKG members marked bad.
- Batched verification of DKG contributions is weighted the same way, so
  colluding members can no longer give another member invalid secret-key
  shares that pass verification, which could leave that member unable to sign
  for the quorum. This did not expose the quorum key.
- A DKG member copying another member's entire contribution could prevent the
  whole quorum from forming. Members whose verification vectors share a
  coefficient are now excluded from the commitment instead of the round
  failing. This excludes both the copying member and the member it copied,
  and both receive the usual PoSe penalty for failed DKG participation.
- Colluding DKG members could stop chosen honest members from building their
  secret-key share, which could prevent a quorum from forming without any
  member being blamed. The same condition made a node penalise an honest peer
  serving quorum data during share recovery. Equal secret-key shares from
  different members are now accepted; the resulting key share is still
  verified against the quorum's verification vector.

Like the governance fix, the DKG fixes protect a quorum once enough of its
members run this version. Weighted batch verification does more work per
message, so masternodes may see higher CPU use during DKG and signing.

# v23.1.10 Change log

See detailed [set of changes][set-of-changes].

# Credits

Thanks to everyone who directly contributed to this release:

- PastaPastaPasta
- UdjinM6

As well as everyone that submitted issues, reviewed pull requests and helped
debug the release candidates.

# Older releases

These releases are considered obsolete. Old release notes can be found here:

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

[set-of-changes]: https://github.com/dashpay/dash/compare/v23.1.9...dashpay:v23.1.10
