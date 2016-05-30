Omni Core v0.0.11
=================

v0.0.11 is a major release and consensus critical in terms of the Omni Layer protocol rules. An upgrade is mandatory, and highly recommended. Prior releases will not be compatible with new behavior in this release.

Please report bugs using the issue tracker on GitHub:

  https://github.com/OmniLayer/omnicore/issues

Table of contents
=================

- [Omni Core v0.0.11](#omni-core-v0011)
- [Upgrading and downgrading](#upgrading-and-downgrading)
  - [How to upgrade](#how-to-upgrade)
  - [Downgrading](#downgrading)
  - [Compatibility with Bitcoin Core](#compatibility-with-bitcoin-core)
- [Imported changes and notes](#imported-changes-and-notes)
  - TODO
- [Consensus affecting changes](#consensus-affecting-changes)
  - [Trading of all pairs on the Distributed Exchange](#trading-of-all-pairs-on-the-distributed-exchange)
  - [Fee distribution system on the Distributed Exchange](#fee-distribution-system-on-the-distributed-exchange)
- [Other notable changes](#other-notable-changes)
  - [Improved consensus checks](#improved-consensus-checks)
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

Downgrading to an Omni Core version prior 0.0.11 is generally not supported as older versions will not provide accurate information due to the changes in consensus rules.

Compatibility with Bitcoin Core
-------------------------------

Omni Core is based on Bitcoin Core 0.10.4 and can be used as replacement for Bitcoin Core. Switching between Omni Core and Bitcoin Core is fully supported at any time.

Downgrading to a Bitcoin Core version prior 0.10 is not supported due to the new headers-first synchronization.

Imported changes and notes
==========================

TODO
------------------------------

TODO

Consensus affecting changes
===========================

All changes of the consensus rules are enabled by activation transactions.

Please note, while Omni Core 0.0.11 contains support for several new rules and features they are not enabled immediately and will be activated via the feature activation mechanism described above.

It follows an overview and a description of the consensus rule changes:

Trading of all pairs on the Distributed Exchange
------------------------------------------------

Once activated trading of any property against any other (within the same ecosystem) will be permitted on the Distributed Exchange.

Due to this change the existing trading UI in the QT version is no longer suitable and has been disabled for this release.  Please use the RPC interface to interact with the Distributed Exchange in this release.  The trading UI will be re-enabled in a future version to accomodate non-Omni pair trading.

This change is identified by `"featureid": 8` and labeled by the GUI as `"Allow trading all pairs on the Distributed Exchange"`.

Fee distribution system on the Distributed Exchange
---------------------------------------------------

Omni Core 0.11 contains a fee caching & distribution system.  This system collects small amounts of tokens in a cache until a distribution threshold is reached.  Once this distribution threshold (trigger) is reached for a property, the fees in the cache will be distributed proportionally to holders of the Omni (#1) token based on the percentage of the total Omni tokens owned.

Once activated fees will be collected from trading of non-Omni pairs on the Distributed Exchange (there is no fee for trading Omni pairs).  The party removing liquidity from the market will incur a 0.05% fee which will be transferred to the fee cache, and subsequently distributed to holders of the Omni token.

- Placing a trade where one side of the pair is Omni (#1) incurs no fee
- Placing a trade where liquidity is added to the market (ie the trade does not immediately execute an existing trade) incurs no fee
- Placing a trade where liquidity is removed from the market (ie the trade immediately executes an existing trade) the liquidity taker incurs a 0.05% fee

See also: [omni_getfeecache](#), [omni_getfeedistribution](#), [omni_getfeedistributions](#), [omni_getfeeshare](#), [omni_getfeetrigger](#) JSON-RPC API calls

This change is identified by `"featureid": 9` and labeled by the GUI as `"Fee system (inc 0.05% fee from trades of non-Omni pairs)"`.


Other notable changes
=====================

Improved consensus checks
-------------------------

Consensus hashing now covers much more of the state to provide wider coverage of the state.  The state of properties, crowdsales and the Distributed Exchange are included in the new consensus hashing process.

Checkpoints have been updated in Omni Core 0.0.11 to reflect the new consensus hashing algorithm.  Seed blocks (for faster initial transaction scanning) and checkpoints are included with Omni Core 0.0.11 up to block 410,000.


Change log
==========

The following list includes relevant pull requests merged into this release:
```
- #226 Upgrade consensus hashing to cover more of the state
- #316 Support providing height for omni_decodetransaction
- #317 Expose feature activation fields when decoding transaction
- #318 Expose Omni Core client version as integer
- #321 Add consensus hash for block 390000
- #324 Fix and update seed blocks up to block 390000
- #325 Add capability to generate seed blocks over RPC
- #326 Grow balances to fit on overview tab
- #327 Switch to Bitcoin tab in Send page when handling Bitcoin URIs
- #328 Update and add unit tests for new consensus hashes
- #332 Remove seed blocks for structurally invalid transactions + reformat
- #333 Improve fee warning threshold in GUI
- #334 Update documentation for getseedblocks, getcurrentconsensushash, setautocommit
- #335 Disable logging on Windows to speed up CI RPC tests
- #336 Change the default maximum OP_RETURN size to 80 bytes
- #341 Add omni_getmetadexhash() RPC call to hash state of MetaDEx
- #343 Remove pre-OP_RETURN legacy code
- #344 Fix missing client notification for new activations
- #349 Add positioninblock attribute to RPC output for transactions
- #358 Add payload creation to the RPC interface
- #361 Unlock trading of all pairs on the MetaDEx
- #364 Fix Travis builds without cache
- #365 Fix syntax error in walletdb key parser
- #367 Bump version to Omni Core 0.0.11-dev
- #368 Fix too-aggressive database clean in reorg event
- #371 Add consensus checkpoints for blocks 400,000 & 410,000
- #372 Add seed blocks for 390,000 to 410,000
- #375 Temporarily disable the trading UI
- #TODO TODO
```

Credits
=======

Thanks to everyone who contributed to this release, and especially the Bitcoin Core developers for providing the foundation for Omni Core!
