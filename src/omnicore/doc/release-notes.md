Omni Core v0.9.0
================

v0.9.0 is a major release and uses Segregated Witness wrapped in P2SH for newly generated addresses per default. It also adds two new transaction types to anchor arbitrary data in the blockchain. As an experimental feature, several new commands were added to support querying any Bitcoin balance.

While this release is not mandatory and doesn't change the consensus rules of the Omni Layer protocol, an upgrade is nevertheless recommended.

Upgrading from 0.8.2, 0.8.1 or 0.8.0 does not require a rescan and can be done very fast without interruption.

Please report bugs using the issue tracker on GitHub:

  https://github.com/OmniLayer/omnicore/issues


Table of contents
=================

- [Omni Core v0.9.0](#omni-core-v082)
- [Upgrading and downgrading](#upgrading-and-downgrading)
  - [How to upgrade](#how-to-upgrade)
  - [Downgrading](#downgrading)
  - [Compatibility with Bitcoin Core](#compatibility-with-bitcoin-core)
- [Improvements](#improvements)
  - [Use wrapped Segrated Witness scripts for new addresses](#use-wrapped-segrated-witness-scripts-for-new-addresses)
  - [New transactions to anchor arbitrary data](#new-transactions-to-anchor-arbitrary-data)
  - [Experimental querying of any Bitcoin balance](#experimental-querying-of-any-bitcoin-balance)
  - [Several test and under the hood improvements](#several-test-and-under-the-hood-improvements)
- [Change log](#change-log)
- [Credits](#credits)


Upgrading and downgrading
=========================

How to upgrade
--------------

If you are running Bitcoin Core or an older version of Omni Core, shut it down. Wait until it has completely shut down, then copy the new version of `omnicored`, `omnicore-cli` and `omnicore-qt`. On Microsoft Windows the setup routine can be used to automate these steps.

When upgrading from an older version than 0.8.0, the database of Omni Core is reconstructed, which can easily consume several hours. During the first startup historical Omni Layer transactions are reprocessed and Omni Core will not be usable for several hours up to more than a day. The progress of the initial scan is reported on the console, the GUI and written to the `debug.log`. The scan may be interrupted and can be resumed.

Upgrading from 0.8.1 or 0.8.0 does not require a rescan and can be done very fast without interruption.

Downgrading
-----------

Downgrading to an Omni Core version prior to 0.8.0 is not supported.

Compatibility with Bitcoin Core
-------------------------------

Omni Core is based on Bitcoin Core 0.18.1 and can be used as replacement for Bitcoin Core. Switching between Omni Core and Bitcoin Core may be supported.

However, it is not advised to upgrade or downgrade to versions other than Bitcoin Core 0.18. When switching to Omni Core, it may be necessary to reprocess Omni Layer transactions.


Improvements
============

Use wrapped Segrated Witness scripts for new addresses
------------------------------------------------------

When using `getnewaddress` new addresses are generated with Segrated Witness scripts wrapped in P2SH. On mainnet, those addresses start with a `3` instead of `1`. This upgrade lowers transaction fees significantly and should not break integrations.

In case you need to fall back to the legacy address generation, please start Omni Core with `addresstype=legacy` configuration option.


New transactions to anchor arbitrary data
-----------------------------------------

Two new transaction types were added to anchor arbitrary data on-chain. This allows the creation of overlay protocols on top of the Omni protocol, or can simply be used to store any form of data in the blockchain.

**Example of sending and pulling data**

Sending the hex-encoded data `497420776f726b7321`:

```bash
$ omnicore-cli "omni_sendanydata" "2N5bnBsaVdPBuK5xKCQ5ZTXnofBfwSxU2Th" "497420776f726b7321"
```
```
4c9776f28e7015e840a05cb0955c22fd6917cf264032ad694e5d1ee0d8ebf745
```

After a confirmation:

```bash
$ omnicore-cli "omni_gettransaction" "4c9776f28e7015e840a05cb0955c22fd6917cf264032ad694e5d1ee0d8ebf745"
```
```js
{
  "txid": "4c9776f28e7015e840a05cb0955c22fd6917cf264032ad694e5d1ee0d8ebf745",
  "fee": "0.00003240",
  "sendingaddress": "2N5bnBsaVdPBuK5xKCQ5ZTXnofBfwSxU2Th",
  "ismine": true,
  "version": 0,
  "type_int": 200,
  "type": "Embed any data",
  "data": "497420776f726b7321",
  "valid": true,
  "blockhash": "7c1e8be2c48fa6062b2b8ff6de7b2e1bc14b7d281d961979ed195a86399abd75",
  "blocktime": 1599469254,
  "positioninblock": 1,
  "block": 367,
  "confirmations": 1
}
```

For more details, please see the descriptions of the new RPCs:

- [omni_sendanydata](https://github.com/OmniLayer/omnicore/blob/master/src/omnicore/doc/rpc-api.md#omni_sendanydata)
- [omni_createpayload_anydata](https://github.com/OmniLayer/omnicore/blob/master/src/omnicore/doc/rpc-api.md#omni_createpayload_anydata)


Experimental querying of any Bitcoin balance
--------------------------------------------

A new optional database was added, which allows the user to query any Bitcoin balance or list transactions of any addresses. Please note, this feature is experimental and not enabled per default. To enable the new database, restart Omni Core with `experimental-btc-balances=1` configuration option.

**When enabling both of these options, Omni Core creates a new database for Bitcoin balances. This step can take a very long time of up to multiple days on mainnet. More than 300 GB of additional disk space are required!**

Please see the descriptions of the new RPCs for more details:

- [getaddresstxids](https://github.com/OmniLayer/omnicore/blob/master/src/omnicore/doc/rpc-api.md#getaddresstxids)
- [getaddressdeltas](https://github.com/OmniLayer/omnicore/blob/master/src/omnicore/doc/rpc-api.md#getaddressdeltas)
- [getaddressbalance](https://github.com/OmniLayer/omnicore/blob/master/src/omnicore/doc/rpc-api.md#getaddressbalance)
- [getaddressutxos](https://github.com/OmniLayer/omnicore/blob/master/src/omnicore/doc/rpc-api.md#getaddressutxos)
- [getaddressmempool](https://github.com/OmniLayer/omnicore/blob/master/src/omnicore/doc/rpc-api.md#xxxxx)
- [getblockhashes](https://github.com/OmniLayer/omnicore/blob/master/src/omnicore/doc/rpc-api.md#getblockhashes)
- [getspentinfo](https://github.com/OmniLayer/omnicore/blob/master/src/omnicore/doc/rpc-api.md#getspentinfo)

**Please also note, this feature is experimental and may provide inaccurate results.**


Several test and under the hood improvements
--------------------------------------------

To prepare Omni Core for Bitcoin Core 0.20+, OpenSSL was removed and replaced. OmniJ related tests were upgraded to use JDK 11. New tests for funded transactions were added and old bash tests were converted to the newer functional test framework. Additional checks and safe guards were implemented.

These changes improve the robustness and reliability of Omni Core.


Change log
==========

The following list includes relevant pull requests merged into this release:

```
- #1142 Remove OpenSSL usage from Omni source
- #1146 Travis OmniJ tests: upgrade to JDK 11 (from JDK 8)
- #1153 Check Omni token balance is consistent after reorg
- #1155 Add tests for CreateFundedTransaction
- #1159 Convert bash test scripts to functional test framework
- #1163 Set default address type to P2SH_SEGWIT
- #1166 Add new transaction type to embed any data
- #1168 Support adding a receiver address for "any data" transactions
- #1169 Avoid overflow on reindex with debug enabled
- #1165 Add bitcore indexing
- #1173 Bump version to 0.9.0
- #1175 Add description for -experimental-btc-balances
- #1179 Move lock to blockOnchainActive
- #1177 Add documentation for address index RPCs
- #1181 Return error, when using bitcore RPCs without addrindex
- #1176 Add release notes for Omni Core 0.9.0
```


Credits
=======

Thanks to everyone who contributed to this release, especially to Peter Bushnell.
