Omni Core v0.10.0
================

v0.10.0 is a major release and updates the underlying version of Bitcoin Core from 0.18.1 to 0.20.1. This comes with a significant number of changes. In this version recovering from a hard shutdown or crash was also greatly improved.

While this release is not mandatory and doesn't change the consensus rules of the Omni Layer protocol, an upgrade is nevertheless recommended.

**Due to the upgrade from Bitcoin Core 0.18.1 to 0.20.1, this version incooperates many changes, so please take your time to read through all release notes carefully.**

Upgrading from 0.9.0, 0.8.2, 0.8.1 or 0.8.0 does not require a rescan and can be done very fast without interruption.

Please report bugs using the issue tracker on GitHub:

  https://github.com/OmniLayer/omnicore/issues


Table of contents
=================

- [Omni Core v0.10.0](#omni-core-v082)
- [Upgrading and downgrading](#upgrading-and-downgrading)
  - [How to upgrade](#how-to-upgrade)
  - [Downgrading](#downgrading)
  - [Compatibility with Bitcoin Core](#compatibility-with-bitcoin-core)
- [Improvements](#improvements)
  - [Built on top of Bitcoin Core 0.20.1](#built-on-top-of-bitcoin-core-0201)
  - [Improved coin selection, when sending Omni Layer transactions](#improved-coin-selection-when-sending-omni-layer-transactions)
  - [Better recovery from hard shutdowns or crashes](#better-recovery-from-hard-shutdowns-or-crashes)
  - [Additional testing](#additional-testing)
- [Change log](#change-log)
- [Credits](#credits)


Upgrading and downgrading
=========================

How to upgrade
--------------

If you are running Bitcoin Core or an older version of Omni Core, shut it down. Wait until it has completely shut down, then copy the new version of `omnicored`, `omnicore-cli` and `omnicore-qt`. On Microsoft Windows the setup routine can be used to automate these steps.

When upgrading from an older version than 0.8.0, the database of Omni Core is reconstructed, which can easily consume several hours. During the first startup historical Omni Layer transactions are reprocessed and Omni Core will not be usable for several hours up to more than a day. The progress of the initial scan is reported on the console, the GUI and written to the `debug.log`. The scan may be interrupted and can be resumed.

Upgrading from 0.9.0, 0.8.2, 0.8.1 or 0.8.0 does not require a rescan and can be done very fast without interruption.

Downgrading
-----------

Downgrading to an Omni Core version prior to 0.8.0 is not supported.

Compatibility with Bitcoin Core
-------------------------------

Omni Core is based on Bitcoin Core 0.20.1 and can be used as replacement for Bitcoin Core. Switching between Omni Core and Bitcoin Core may be supported.

However, it is not advised to upgrade or downgrade to versions other than Bitcoin Core 0.18. When switching to Omni Core, it may be necessary to reprocess Omni Layer transactions.


Improvements
============

Built on top of Bitcoin Core 0.20.1
-----------------------------------

The underlying base of Omni Core was upgraded from Bitcoin Core 0.18.1 to Bitcoin Core 0.20.1.

Please read the following release notes for further details very carefully:

- [Release notes for Bitcoin Core 0.19.0](https://github.com/bitcoin/bitcoin/blob/v0.20.1/doc/release-notes/release-notes-0.19.0.1.md)
- [Release notes for Bitcoin Core 0.19.1](https://github.com/bitcoin/bitcoin/blob/v0.20.1/doc/release-notes/release-notes-0.19.1.md)
- [Release notes for Bitcoin Core 0.20.0](https://github.com/bitcoin/bitcoin/blob/v0.20.0/doc/release-notes.md)
- [Release notes for Bitcoin Core 0.20.1](https://github.com/bitcoin/bitcoin/blob/v0.20.1/doc/release-notes.md)


Improved coin selection, when sending Omni Layer transactions
------------------------------------------------------------

When creating and sending Omni Layer transactions, a certain transaction fee must be paid in Bitcoin.

During transaction creation, the amount of fee needed is estimated. In the past, this estimation was rather genereous, resulting in a failure during transaction creation, even when enough Bitcoin were available to create a transaction. In this release, the fee estimation was optimized.


Better recovery from hard shutdowns or crashes
----------------------------------------------

After a hard shutdown, kill or crash, Omni Core sometimes lost it's database, because it was not properly saved and became corrupted. Restoring from an old state was not possible, which resulted in a very time consuming process of reparsing old transactions.

With this release, Omni Core is able to properly recover from an older state, without the need of a very time consuming reprocessing of old transactions.


Additional testing
----------------

More tests were added to Omni Core.


Change log
==========

The following list includes relevant pull requests merged into this release:

```
- #1186 Bitcoin 0.20.1
- #1191 [tests] check free DEx behaviour against DEx spec
- #1193 Omni overview updates
- #1194 Add Free DEx and fee cache tests
- #1197 Integrate Bitcoin Core 0.20 patches
- #1198 Additional changes missing from 0.20 merge
- #1199 Qt disable wallet change in main window
- #1200 Update version to 0.9.99 to indicate development
- #1201 Reduce amount selected for use in transactions
- #1210 cli tool: add Content-Type application/json
- #1213 If watermark not in block index load from state files
- #1214 Bump version and tests to 0.10
- #1215 Add release notes for Omni Core 0.10
- #1219 Trim Gitian build targets
```


Credits
=======

Thanks to everyone who contributed to this release, especially to Peter Bushnell.
