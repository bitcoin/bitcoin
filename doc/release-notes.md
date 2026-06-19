# Dash Core version v23.1.4

This is a new patch version release, bringing performance improvements and bug fixes.
This release is **optional** for all nodes, although recommended.

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

## Notable changes
- Asset lock transactions that Dash Platform cannot process are now treated as
  non-standard, so they are no longer relayed across the network.
  This covers asset locks with more than 100 inputs, or larger than 20480 bytes,
  and those using the v2 payload (for post-v24 release), protecting users from
  creating oversized asset locks that could result in lost funds.
  Such transactions remain valid for consensus if mined into a block (dash#7359).

## Bug Fixes

- Reject two ProRegTx/ProUpServTx with the same Platform node ID from being
  present in the mempool simultaneously.
- Compilation fix for some compilers due to invalid final mark (dash#7317).

## Performance Improvements

- Reduced lock contention when penalizing peers for invalid transactions,
  improving networking responsiveness under load (dash#7332).
- Made header sync almost twice as fast by optimization of low-level math
  primitives (dash#7352).
- Avoid re-validation of EHF signals during block connect. This also prevents
  a potential assertion crash during large blockchain re-organizations when a
  buried fork is re-validated in `BlockUndo`.
- Early bail-out for invalid oversized llmq messages.

# v23.1.4 Change log

See detailed [set of changes][set-of-changes].

# Credits

Thanks to everyone who directly contributed to this release:

- Konstantin Akimov

As well as everyone that submitted issues, reviewed pull requests and helped
debug the release candidates.

# Older releases

These releases are considered obsolete. Old release notes can be found here:

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

[set-of-changes]: https://github.com/dashpay/dash/compare/v23.1.3...dashpay:v23.1.4
