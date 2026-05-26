# Dash Core version v23.1.2

This is a new patch version release, bringing GUI improvements, new features, bugfixes, and performance optimizations.
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

## GUI changes

- Introduced a framework for sourcing and applying data with dedicated feeds, used by the Masternode and Proposal list views for improved data flow and separation of concerns (dash#7146).
- Added a new "Proposal Information" widget to the Information tab with an interactive donut chart showing proposal budget allocation (dash#7159).
- Added distinct widgets for Dash-specific reporting in the Debug window, including dedicated Information and Network tabs (dash#7118).
- Added support for reporting `OP_RETURN` payloads as Data Transactions in the transaction list (dash#7144).
- Added Tahoe styled icons for macOS with runtime styling for each network type (mainnet, testnet, devnet, regtest), updated bundle icon, and added mask-based tray icon with generation scripts (dash#7180).
- Filter preferences in the masternode list are now persisted across sessions (dash#7148).
- Fixed overview page font double scaling, recalculated minimum width correctly, fixed `SERVICE` and `STATUS` column sorting, and fixed common types filtering in masternode list (dash#7147).
- Fixed `labelError` styling by moving it from `proposalcreate.ui` into `general.css` for consistency (dash#7145).
- Fixed banned masternodes incorrectly returning status=0 instead of their actual ban status (dash#7157).

## Bug Fixes

- Fixed MN update notifications where the old and new masternode lists were swapped, causing incorrect change detection (dash#7154).
- Reject identity elements in BLS deserialization and key generation to prevent invalid keys from being accepted (dash#7193).
- Fixed quorum labels not being correctly reseated when new quorum types are inserted (dash#7191).
- Skip collecting block txids during IBD to prevent unbounded memory growth in `ChainLockSigner` (dash#7208).
- Serialize `TrySignChainTip` to prevent concurrent signing races that could split signing shares across different block hashes (dash#7209).
- Properly skip evodb repair when reindexing to prevent unnecessary repair attempts (dash#7222).

## Miscellaneous

- Renamed `bitcoin-util` manpage and test references to `dash-util` (dash#7221).

## Interfaces

- Consolidated masternode counts into a single struct and exposed chainlock, InstantSend, credit pool, and quorum statistics through the node interface (dash#7160).

## Performance Improvements

- Replaced two heavy `HashMap` constructions with linear lookups in hot paths where the maps were rarely used, reducing overhead (dash#7176).

# v23.1.2 Change log

See detailed [set of changes][set-of-changes].

# Credits

Thanks to everyone who directly contributed to this release:

- Kittywhiskers Van Gogh
- Konstantin Akimov
- PastaPastaPasta
- UdjinM6

As well as everyone that submitted issues, reviewed pull requests and helped
debug the release candidates.

# Older releases

These releases are considered obsolete. Old release notes can be found here:

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

[set-of-changes]: https://github.com/dashpay/dash/compare/v23.1.0...dashpay:v23.1.2
