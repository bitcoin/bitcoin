Omni Core v0.11.0
================

v0.11.0 is a major release and adds the ability to issue non-fungible token and to delegate the management of a token.

This release is mandatory and changes the rules of the Omni Layer protocol. An upgrade is required.

Upgrading from 0.9.0, 0.8.2, 0.8.1 or 0.8.0 does not require a rescan and can be done very fast without interruption.

Please report bugs using the issue tracker on GitHub:

  https://github.com/OmniLayer/omnicore/issues


Table of contents
=================

- [Omni Core v0.11.0](#omni-core-v082)
- [Upgrading and downgrading](#upgrading-and-downgrading)
  - [How to upgrade](#how-to-upgrade)
  - [Downgrading](#downgrading)
  - [Compatibility with Bitcoin Core](#compatibility-with-bitcoin-core)
- [Improvements](#improvements)
  - [Non-fungible tokens](#non-fungible-tokens)
  - [Delegate management of tokens](#delegate-management-of-tokens)
  - [Move CI test builds to Cirrus](#move-ci-test-builds-to-cirrus)
- [Change log](#change-log)
- [Credits](#credits)


Upgrading and downgrading
=========================

How to upgrade
--------------

If you are running Bitcoin Core or an older version of Omni Core, shut it down. Wait until it has completely shut down, then copy the new version of `omnicored`, `omnicore-cli` and `omnicore-qt`. On Microsoft Windows the setup routine can be used to automate these steps.

When upgrading from a version older than 0.8.0, the database of Omni Core is reconstructed, which can easily consume several hours. During the first startup historical Omni Layer transactions are reprocessed and Omni Core will not be usable for several hours up to more than a day. The progress of the initial scan is reported on the console, the GUI and written to the `debug.log`. The scan may be interrupted and can be resumed.

Upgrading from 0.9.0, 0.8.2, 0.8.1 or 0.8.0 does not require a rescan and can be done very fast without interruption.

Downgrading
-----------

Downgrading to an Omni Core version prior to 0.11.0 is not supported.

Compatibility with Bitcoin Core
-------------------------------

Omni Core is based on Bitcoin Core 0.20.1 and can be used as replacement for Bitcoin Core. Switching between Omni Core and Bitcoin Core may be supported.

However, it is not advised to upgrade or downgrade to versions other than Bitcoin Core 0.18. When switching to Omni Core, it may be necessary to reprocess Omni Layer transactions.


Improvements
============

Non-fungible tokens
-------------------

This release adds the ability to create managed tokens with a unique index, grant data, issuer data and holder data. Grant data is set on issuance, issuer data and holder data can be set at any point by the current issuer or holder respectively.  Grant data repurposes the memo field used in grant transactions that were not previously used other than to set data on the blockchain.

This feature needs to be enabled with an activation transaction.

Two new transaction types were added:

- `5`: "Unique Send"
- `201`: "Set Non-Fungible Token Data"

#### Overview

A new token type "unique" was added, which can be specified when creating a new managed token with ["omni_sendissuancemanaged"](https://github.com/OmniLayer/omnicore/blob/develop/src/omnicore/doc/rpc-api.md#omni_sendissuancemanaged). Unique, or non-fungible tokens, can individually be identified, sent or enhanced with extra data. This can be useful to represent art, real estate or other digital or real world items on the blockchain. Individual units of a non-fungible token can be transferred with ["omni_sendnonfungible"](https://github.com/OmniLayer/omnicore/blob/develop/src/omnicore/doc/rpc-api.md#omni_sendnonfungible).

#### RPC changes

New RPCs:

- ["omni_sendnonfungible"](https://github.com/OmniLayer/omnicore/blob/develop/src/omnicore/doc/rpc-api.md#omni_sendnonfungible)
- ["omni_setnonfungibledata"](https://github.com/OmniLayer/omnicore/blob/develop/src/omnicore/doc/rpc-api.md#omni_setnonfungibledata)
- ["omni_getnonfungibletokens"](https://github.com/OmniLayer/omnicore/blob/develop/src/omnicore/doc/rpc-api.md#omni_getnonfungibletokens)
- ["omni_getnonfungibletokendata"](https://github.com/OmniLayer/omnicore/blob/develop/src/omnicore/doc/rpc-api.md#omni_getnonfungibletokendata)
- ["omni_getnonfungibletokenranges"](https://github.com/OmniLayer/omnicore/blob/develop/src/omnicore/doc/rpc-api.md#omni_getnonfungibletokenranges)
- ["omni_createpayload_sendnonfungible"](https://github.com/OmniLayer/omnicore/blob/develop/src/omnicore/doc/rpc-api.md#omni_createpayload_sendnonfungible)
- ["omni_createpayload_setnonfungibledata"](https://github.com/OmniLayer/omnicore/blob/develop/src/omnicore/doc/rpc-api.md#omni_createpayload_setnonfungibledata)

Updated RPC:

- ["omni_sendissuancemanaged"](https://github.com/OmniLayer/omnicore/blob/develop/src/omnicore/doc/rpc-api.md#omni_sendissuancemanaged) "type" can be set to `5` to create new NFTs.
- ["omni_sendgrant"](https://github.com/OmniLayer/omnicore/blob/develop/src/omnicore/doc/rpc-api.md#omni_sendgrant) "memo" field is changed into "grantdata".


Delegate management of tokens
-----------------------------

This release adds the delegation of issuance authority to a new address, for the issuance, freezing or unfreezing of units of a managed token.

This feature needs to be enabled with an activation transaction.

Two new transaction types were added:

- `73`: "Add delegate"
- `74`: "Remove delegate"

#### Overview

Alice is the issuer of token and wants to delegate the issuance of new token units to Bob. She then uses ["omni_sendadddelegate"](https://github.com/OmniLayer/omnicore/blob/develop/src/omnicore/doc/rpc-api.md#omni_sendadddelegate) to empower Bob. Alice won't be able to issue new units until she removes Bob as delegate. She doesn't give up her ability to name or remove delegates and she remains to be the one who can enable or disable freezing of units. On a more practical level, this feature can be useful, if a token was issued from a regular non-multisig address and a multisig-address is used as delegate.

#### Rules overview

When adding a delegate:

- The property must be managed.
- The sender of the transaction must be the issuer.
- When adding a delegate, when there is already one, the old one is overwritten.
- When a delegate is set, only the delegate can issue, revoke, freeze or unfreeze units.
- Only the issuer can enable or disable freezing.

When removing a delegate:

- To remove a delegate, the property must be managed
- The sender of the transaction can be the issuer or the delegate itself.
- The property has a delegate and the delegate matches the one to remove.
- The delegate to remove must be named explicitly.

#### RPC changes

New RPCs:

- ["omni_sendadddelegate"](https://github.com/OmniLayer/omnicore/blob/develop/src/omnicore/doc/rpc-api.md#omni_sendadddelegate)
- ["omni_sendremovedelegate"](https://github.com/OmniLayer/omnicore/blob/develop/src/omnicore/doc/rpc-api.md#omni_sendremovedelegate)
- ["omni_createpayload_adddelegate"](https://github.com/OmniLayer/omnicore/blob/develop/src/omnicore/doc/rpc-api.md#omni_createpayload_adddelegate)
- ["omni_createpayload_removedelegate"](https://github.com/OmniLayer/omnicore/blob/develop/src/omnicore/doc/rpc-api.md#omni_createpayload_removedelegate)

Updated RPC:

- ["omni_getproperty"](https://github.com/OmniLayer/omnicore/blob/develop/src/omnicore/doc/rpc-api.md#omni_getproperty) add "delegate" field.


Move CI test builds to Cirrus
-----------------------------

Automated testing was moved from Travis CI to Cirrus CI.


Change log
==========

The following list includes relevant pull requests merged into this release:

```
- #1219 Trim Gitian build targets
- #1220 Move test builds to Cirrus
- #1223 Show Omni version in Qt and CLI
- #1207 Feature Non-Fungible Tokens
- #1224 Add opcode when calculating Omni class
- #1227 Query address for all NFTs without specifying property ID
- #1228 Add support to add a delegate for managed issuance
- #1231 Gitian Linux confi8g: replace arm 32-bit with arm64 (aarch64)
- #1232 Bump release version to 0.11
- #1234 Update RPC docs for delegated issuance
- #1233 Update release notes for Omni Core 0.11
```


Credits
=======

Thanks to everyone who contributed to this release, especially to Peter Bushnell and Sean Gilligan.
