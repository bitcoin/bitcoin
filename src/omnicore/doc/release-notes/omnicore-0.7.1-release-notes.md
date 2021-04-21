Omni Core v0.7.1
================

Omni Core 0.7.1 paves the way for trading any asset on the Omni Layer for Bitcoin in a decentralized fashion. It adds new commands to accept and execute orders on the distributed exchange.

Please note, if you have not yet upgrade from 0.6 or earlier versions: Omni Core 0.7 changed the code base of Omni Core from Bitcoin Core 0.13.2 to Bitcoin Core 0.18.1. Once consensus affecting features are enabled, this version is no longer compatible with previous versions and an upgrade is required. Due to the upgrade from Bitcoin Core 0.13.2 to 0.18.1, this version incorporates many changes, so please take your time to read through all previous release notes carefully. The first time you upgrade from Omni Core 0.6 or older versions, the database is reconstructed, which can easily consume several hours. An upgrade from 0.7 to 0.7.1 is seamless.

Please report bugs using the issue tracker on GitHub:

  https://github.com/OmniLayer/omnicore/issues


Table of contents
=================

- [Omni Core v0.7.1](#omni-core-v071)
- [Upgrading and downgrading](#upgrading-and-downgrading)
  - [How to upgrade](#how-to-upgrade)
  - [Downgrading](#downgrading)
  - [Compatibility with Bitcoin Core](#compatibility-with-bitcoin-core)
- [Imported changes and notes](#imported-changes-and-notes)
  - [Allow any token to be traded for Bitcoin](#allow-any-token-to-be-traded-for-bitcoin)
  - [Updates to omni_senddexaccept to accept orders](#updates-to-omni_senddexaccept-to-accept-orders)
  - [New command omni_senddexpay to execute orders](#new-command-omni_senddexpay-to-execute-orders)
  - [rpcallowip option changes](#rpcallowip-option-changes)
  - [Updates to the Omni Core logo](#updates-to-the-omni-core-logo)
  - [Several stability and performance improvements](#several-stability-and-performance-improvements)
- [Change log](#change-log)
- [Credits](#credits)


Upgrading and downgrading
=========================

How to upgrade
--------------

If you are running Bitcoin Core or an older version of Omni Core, shut it down. Wait until it has completely shut down, then copy the new version of `omnicored`, `omnicore-cli` and `omnicore-qt`. On Microsoft Windows the setup routine can be used to automate these steps.

When upgrading from an older version, the database is reconstructed, which can easily consume several hours.

During the first startup historical Omni transactions are reprocessed and Omni Core will not be usable for approximately 15 minutes up to two hours. The progress of the initial scan is reported on the console, the GUI and written to the `debug.log`. The scan may be interrupted, but can not be resumed, and then needs to start from the beginning.

Downgrading
-----------

Downgrading to an Omni Core version prior to 0.7.0 is not supported.

Compatibility with Bitcoin Core
-------------------------------

Omni Core is based on Bitcoin Core 0.18.1 and can be used as replacement for Bitcoin Core. Switching between Omni Core and Bitcoin Core may be supported.

However, it is not advised to upgrade or downgrade to versions other than Bitcoin Core 0.18. When switching to Omni Core, it may be necessary to reprocess Omni Layer transactions.


Imported changes and notes
==========================

Allow any token to be traded for Bitcoin
----------------------------------------

Right now the native distributed exchange of the Omni Layer protocol supports trading Omni and Test Omni for Bitcoin.

With this version, any token can be traded and there are no longer any trading restrictions. Please see the documentation for the related RPCs:

- [omni_senddexsell](https://github.com/OmniLayer/omnicore/blob/v0.7.1/src/omnicore/doc/rpc-api.md#omni_senddexsell)
- [omni_senddexaccept](https://github.com/OmniLayer/omnicore/blob/v0.7.1/src/omnicore/doc/rpc-api.md#omni_senddexaccept)
- [omni_senddexpay](https://github.com/OmniLayer/omnicore/blob/v0.7.1/src/omnicore/doc/rpc-api.md#omni_senddexpay)
- [omni_getactivedexsells](https://github.com/OmniLayer/omnicore/blob/v0.7.1/src/omnicore/doc/rpc-api.md#omni_getactivedexsells)

As well as the specification of the Omni Layer protocol:

- [Omni Layer protocol: 8.2. Distributed Exchange](https://github.com/OmniLayer/spec/#82-distributed-exchange)

Please note: this consensus change is not yet activated, but included in this release. An announcement will be made, when this feature is activated.


Updates to `omni_senddexaccept` to accept orders
------------------------------------------------

The RPC `omni_senddexaccept` was updated to properly pay transaction fees, when accepting an offer on the distributed exchange.

### omni_senddexaccept

Create and broadcast an accept offer for the specified token and amount.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `fromaddress`       | string  | required | the address to send from                                                                     |
| `toaddress`         | string  | required | the address of the seller                                                                    |
| `propertyid`        | number  | required | the identifier of the token to purchase                                                      |
| `amount`            | string  | required | the amount to accept                                                                         |
| `override`          | boolean | required | override minimum accept fee and payment window checks (use with caution!)                    |

**Result:**
```js
"hash"  // (string) the hex-encoded transaction hash
```

**Example:**

```bash
$ omnicore-cli "omni_senddexaccept" \
    "35URq1NN3xL6GeRKUP6vzaQVcxoJiiJKd8" "37FaKponF7zqoMLUjEiko25pDiuVH5YLEa" 1 "15.0"
```

---


New command `omni_senddexpay` to execute orders
-----------------------------------------------

To pay for an offer after it was successfully accepted, a new command `omni_senddexpay` was added.

### omni_senddexpay

Create and broadcast payment for an accept offer.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `fromaddress`       | string  | required | the address to send from                                                                     |
| `toaddress`         | string  | required | the address of the seller                                                                    |
| `propertyid`        | number  | required | the identifier of the token to purchase                                                      |
| `amount`            | string  | required | the Bitcoin amount to send                                                                   |

**Result:**
```js
"hash"  // (string) the hex-encoded transaction hash
```

**Example:**

```bash
$ omnicore-cli "omni_senddexpay" \
    "35URq1NN3xL6GeRKUP6vzaQVcxoJiiJKd8" "37FaKponF7zqoMLUjEiko25pDiuVH5YLEa" 1 "15.0"
```

---


`rpcallowip` option changes
------------------------

The `rpcallowip` option can no longer be used to automatically listen on all network interfaces.  Instead, the `rpcbind` parameter must be used to specify the IP addresses to listen on. Listening for RPC commands over a public network connection is insecure and should be disabled, so a warning is now printed if a user selects such a configuration.  If you need to expose RPC in order to use a tool like  Docker, ensure you only bind RPC to your localhost, e.g. `docker run [...] -p 127.0.0.1:8332:8332` (this is an extra `:8332` over the normal Docker port specification).


Updates to the Omni Core logo
-----------------------------

To show the synergy between the Omni Layer protocol and Bitcoin, the Omni Core logos were slightly changed to also include the Bitcoin logo:

- [Omni Core mainnet icon](https://github.com/OmniLayer/omnicore/blob/v0.7.1/src/qt/res/icons/bitcoin.png)
- [Omni Core testnet icon](https://github.com/OmniLayer/omnicore/blob/v0.7.1/src/qt/res/icons/bitcoin_testnet.png)


Several stability and performance improvements
----------------------------------------------

In some rare cases locking issues may have caused the client to halt. This version comes with several stability and performance improvements to resolve these issues.


Change log
==========

The following list includes relevant pull requests merged into this release:

```
- #1048 omni_senddexaccept pass min fee to CreateTransaction
- #1052 Add RPC DEx call to pay for accepted offer
- #1054 Debug and concurrency updates
- #1060 Only use wtxNew if fSuccess true
- #1064 New icons and default splash
- #1066 Prepare release of Omni Core v0.7.1
- #1068 Update release notes for v0.7.1
```


Credits
=======

Thanks to everyone who contributed to this release, especially to Peter Bushnell and all Bitcoin Core developers.
