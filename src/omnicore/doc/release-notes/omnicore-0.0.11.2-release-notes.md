Omni Core v0.0.11.2
===================

v0.0.11.2 is a bugfix release which resolves an issue where, in the case where a buyer accepts more than available for sale on the traditional distributed exchange, the RPC API reported an amount higher than available. This release also disables the alert system as per default.

v0.0.11.1 is a bugfix release which resolves a critical bug in the RPC API whereby under certain circumstances retrieving data about a sell offer may trigger a failsafe and cause the automatic shutdown of the client.

This version is built on top of v0.0.11, which is a major release and consensus critical in terms of the Omni Layer protocol rules. An upgrade is mandatory, and highly recommended. Prior releases will not be compatible with new behavior in this release.

Please report bugs using the issue tracker on GitHub:

  https://github.com/OmniLayer/omnicore/issues

Table of contents
=================

- [Omni Core v0.0.11.2](#omni-core-v00112)
- [Upgrading and downgrading](#upgrading-and-downgrading)
  - [How to upgrade](#how-to-upgrade)
  - [Downgrading](#downgrading)
  - [Compatibility with Bitcoin Core](#compatibility-with-bitcoin-core)
- [Consensus affecting changes](#consensus-affecting-changes)
  - [Trading of all pairs on the Distributed Exchange](#trading-of-all-pairs-on-the-distributed-exchange)
  - [Fee distribution system on the Distributed Exchange](#fee-distribution-system-on-the-distributed-exchange)
  - [Send to Owners cross property suport](#send-to-owners-cross-property-support)
- [Other notable changes](#other-notable-changes)
  - [Raw payload creation API](#raw-payload-creation-api)
  - [Other API extensions](#other-api-extensions)
  - [Increased OP_RETURN payload size to 80 byte](#increased-op_return-payload-size-to-80-bytes)
  - [Improved consensus checks](#improved-consensus-checks)
  - [Various bug fixes and clean-ups](#various-bug-fixes-and-clean-ups)
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

Consensus affecting changes
===========================

All changes of the consensus rules are enabled by activation transactions.

Please note, while Omni Core 0.0.11 contains support for several new rules and features they are not enabled immediately and will be activated via the feature activation mechanism described above.

It follows an overview and a description of the consensus rule changes:

Trading of all pairs on the Distributed Exchange
------------------------------------------------

Once activated trading of any property against any other (within the same ecosystem) will be permitted on the Distributed Exchange.

Due to this change the existing trading UI in the QT version is no longer suitable and has been disabled for this release.  Please use the RPC interface to interact with the Distributed Exchange in this release.  The trading UI will be re-enabled in a future version to accommodate non-Omni pair trading.

This change is identified by `"featureid": 8` and labeled by the GUI as `"Allow trading all pairs on the Distributed Exchange"`.

Fee distribution system on the Distributed Exchange
---------------------------------------------------

Omni Core 0.11 contains a fee caching and distribution system.  This system collects small amounts of tokens in a cache until a distribution threshold is reached.  Once this distribution threshold (trigger) is reached for a property, the fees in the cache will be distributed proportionally to holders of the Omni (#1) and Test-Omni (#2) tokens based on the percentage of the total Omni tokens owned.

Once activated fees will be collected from trading of non-Omni pairs on the Distributed Exchange (there is no fee for trading Omni pairs).  The party removing liquidity from the market will incur a 0.05% fee which will be transferred to the fee cache, and subsequently distributed to holders of the Omni token.

- Placing a trade where one side of the pair is Omni (#1) or Test-Omni (#2) incurs no fee
- Placing a trade where liquidity is added to the market (i.e. the trade does not immediately execute an existing trade) incurs no fee
- Placing a trade where liquidity is removed from the market (i.e. the trade immediately executes an existing trade) the liquidity taker incurs a 0.05% fee

See also [fee system JSON-RPC API documentation](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#fee-system).

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

Other notable changes
=====================

Raw payload creation API
------------------------

Omni Core 0.0.11 adds support for payload creation via the RPC interface.

The calls are similar to the send transactions (e.g. `omni_send`), without the requirement for an address or any of the balance checks.

This allows integrators to build transactions via the [raw transactions interface](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#raw-transactions).

Other API extensions
--------------------

An optional parameter `height` can be provided, when using [omni_decodetransaction](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#omni_decodetransaction), which is used to determine the parsing rules. If no `height` is provided, the chain height is used as default.

When retrieving feature activation transactions with [omni_gettransaction](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#omni_gettransaction), then additional fields are included in the result: `"featureid"`, `"activationblock"` and `"minimumversion"`.

The Omni Core client version is now also exposed under the new key `"omnicoreversion"`, as well as inter via `"omnicoreversion_int"`, when using [omni_getinfo](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#omni_getinfo). The old key `"mastercoreversion"` remains for compatibility in this version.

The field `"positioninblock"` was added to RPCs retrieving or listing Omni transactions to provide visibility into the order of an Omni transaction within a block.

Increased OP_RETURN payload size to 80 bytes
--------------------------------------------

The maximum payload for OP_RETURN outputs was increased to 80 byte.

At this point a majority of the network supports 80 byte payloads, so Omni Core can safely use the larger payload size. This can result in cheaper transactions, as there is no fallback to bare multisig encoding.

Improved consensus checks
-------------------------

Consensus hashing now covers much more of the state to provide wider coverage of the state. The state of properties, crowdsales and the Distributed Exchange are included in the new consensus hashing process.

Checkpoints have been updated in Omni Core 0.0.11 to reflect the new consensus hashing algorithm. Seed blocks (for faster initial transaction scanning) and checkpoints are included with Omni Core 0.0.11 up to block 410,000.

Various bug fixes and clean-ups
------------------------------

Various smaller improvements were added Omni Core 0.0.11, such as:

- Grow balances to fit on "Overview" tab
- Switch to "Bitcoin" tab in "Send" page when handling Bitcoin URIs
- Improve and adjust fee warning threshold when sending transactions
- Fix missing client notification for new feature activations
- Fix Travis CI builds without cache
- Fix syntax error in walletdb key parser
- Fix too-aggressive database clean in block reorganization events
- Fix issues related to `omni_gettransaction` and `getactivedexsells`

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
- #341 Add omni_getmetadexhash RPC call to hash state of MetaDEx
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
- #384 Add fee system RPC calls to API doc
- #385 Add RPC documentation for createpayload calls
- #386 Don't warn user about unknown block versions
- #377 Add release notes for Omni Core 0.0.11
- #376 Bump version to Omni Core 0.0.11-rc1
- #390 Add cross-property (v1) Send To Owners
- #395 Move test scripts into /src/omnicore/test
- #396 Add workaround for "bytes per sigops" limit
- #400 Change default confirm target to 15 blocks
- #398 Update release notes for 0.0.11-rc2
- #397 Bump version to Omni Core 0.0.11-rc2
- #402 Add seed blocks for 410,000 to 420,000
- #403 Add consensus hash for block 420,000
- #405 Use uint256 when calculating desired BTC for DEx 1
- #404 Bump version to Omni Core 0.0.11-rel
- #409 Protect uint256 plain integer math
- #411 Bump version to Omni Core 0.0.11.1-rel
- #419 Add consensus hash for block 430,000
- #420 Add seed blocks for 420,000 to 430,000
- #421 Fix edge case of DEx 1 over-accepts
- #422 Disable alert system as per default
- #423 Bump version to Omni Core 0.0.11.2-rel
```

Credits
=======

Thanks to everyone who contributed to this release, and especially the Bitcoin Core developers for providing the foundation for Omni Core!
