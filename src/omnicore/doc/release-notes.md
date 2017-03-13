Omni Core v0.0.12
=================

v0.0.12 greatly improves Omni Core's performance during the initial parsing, and it includes new logic for the fee distribution system, as well as for cross-property send-to-owner transactions.

v0.0.12 is a major release and consensus critical in terms of the Omni Layer protocol rules. An upgrade is mandatory, and highly recommended. Prior releases will not be compatible with new behavior in this release.

Please report bugs using the issue tracker on GitHub:

  https://github.com/OmniLayer/omnicore/issues

Table of contents
=================

- [Omni Core v0.0.12](#omni-core-v0012)
- [Upgrading and downgrading](#upgrading-and-downgrading)
  - [How to upgrade](#how-to-upgrade)
  - [Downgrading](#downgrading)
  - [Compatibility with Bitcoin Core](#compatibility-with-bitcoin-core)
  - [Consensus affecting changes](#consensus-affecting-changes)
    - [Fee distribution system on the Distributed Exchange](#fee-distribution-system-on-the-distributed-exchange)
    - [Send to Owners cross property suport](#send-to-owners-cross-property-support)
- [Notable changes](#notable-changes)
  - [Performance improvements during initial parsing](#performance-improvements-during-initial-parsing)
- [Change log](#change-log)
- [Credits](#credits)

Upgrading and downgrading
=========================

How to upgrade
--------------

If you are running Bitcoin Core or an older version of Omni Core, shut it down. Wait until it has completely shut down, then copy the new version of `omnicored`, `omnicore-cli` and `omnicore-qt`. On Microsoft Windows the setup routine can be used to automate these steps.

During the first startup historical Omni transactions are reprocessed and Omni Core will not be usable for approximately 15 minutes up to two hours. The progress of the initial scan is reported on the console, the GUI and written to the `debug.log`. The scan may be interrupted, but can not be resumed, and then needs to start from the beginning.

Downgrading
-----------

Downgrading to an Omni Core version prior 0.0.12 is generally not supported as older versions will not provide accurate information due to the changes in consensus rules.

Compatibility with Bitcoin Core
-------------------------------

Omni Core is based on Bitcoin Core 0.10.4 and can be used as replacement for Bitcoin Core. Switching between Omni Core and Bitcoin Core is fully supported at any time.

Downgrading to a Bitcoin Core version prior 0.10 is not supported due to the new headers-first synchronization.

Consensus affecting changes
===========================

All changes of the consensus rules are enabled by activation transactions.

Please note, while Omni Core 0.0.12 contains support for several new rules and features they are not enabled immediately and will be activated via the feature activation mechanism described above.

It follows an overview and a description of the consensus rule changes:

Fee distribution system on the Distributed Exchange
---------------------------------------------------

Omni Core 0.0.12 contains a fee caching and distribution system.  This system collects small amounts of tokens in a cache until a distribution threshold is reached.  Once this distribution threshold (trigger) is reached for a property, the fees in the cache will be distributed proportionally to holders of the Omni (#1) and Test-Omni (#2) tokens based on the percentage of the total Omni tokens owned.

Once activated fees will be collected from trading of non-Omni pairs on the Distributed Exchange (there is no fee for trading Omni pairs).  The party removing liquidity from the market will incur a 0.05% fee which will be transferred to the fee cache, and subsequently distributed to holders of the Omni token.

- Placing a trade where one side of the pair is Omni (#1) or Test-Omni (#2) incurs no fee
- Placing a trade where liquidity is added to the market (i.e. the trade does not immediately execute an existing trade) incurs no fee
- Placing a trade where liquidity is removed from the market (i.e. the trade immediately executes an existing trade) the liquidity taker incurs a 0.05% fee

See also [fee system JSON-RPC API documentation](https://github.com/OmniLayer/omnicore/blob/master/src/omnicore/doc/rpc-api.md#fee-system).

This change is identified by `"featureid": 9` and labeled by the GUI as `"Fee system (inc 0.05% fee from trades of non-Omni pairs)"`.

Send To Owners cross property support
-------------------------------------

Once activated distributing tokens via the Send To Owners transaction will be permitted to cross properties if using version 1 of the transaction.

Tokens of property X then may be distributed to holders of property Y.

There is a significantly increased fee (0.00001000 per recipient) for using version 1 of the STO transaction.  The fee remains the same (0.00000001) per recipient for using version 0 of the STO transaction.

Sending an STO transaction via Omni Core that distributes tokens to holders of the same property will automatically be sent as version 0, and sending a cross-property STO will automatically be sent as version 1.

The transaction format of new Send To Owners version is as follows:

| **Field**                      | **Type**        | **Example** |
| ------------------------------ | --------------- | ----------- |
| Transaction version            | 16-bit unsigned | 65535       |
| Transaction type               | 16-bit unsigned | 65534       |
| Tokens to transfer             | 32-bit unsigned | 6           |
| Amount to transfer             | 64-bit signed   | 700009      |
| Token holders to distribute to | 32-bit unsigned | 23          |

This change is identified by `"featureid": 10` and labeled by the GUI as `"Cross-property Send To Owners"`.

Notable changes
===============

Performance improvements during initial parsing
-----------------------------------------------

Due to various improvements and optimizations, the initial parsing process, when running Omni Core the first time, or when starting Omni Core with `-startclean` flag, is faster by a factor of up to 10x. The improvements also have a positive impact on the time, when processing a new block.

Change log
==========

The following list includes relevant pull requests merged into this release:
```
- #449 Back port fixes & improvements from develop
- #455 Bump version to Omni Core 0.0.12.0-rel
```

Credits
=======

Thanks to everyone who contributed to this release, and especially the Bitcoin Core developers for providing the foundation for Omni Core!
