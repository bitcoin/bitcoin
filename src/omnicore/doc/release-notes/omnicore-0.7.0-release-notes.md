Omni Core v0.7.0
================

v0.7.0 is a major release and changes the code base of Omni Core from Bitcoin Core 0.13.2 to Bitcoin Core 0.18.1. Once consensus affecting features are enabled, this version is no longer compatible with previous versions and an upgrade is required. 

Compared to Omni Core v0.6.0 and previous versions, v0.7.0 enhances it's distributed exchange nad supports trading of any asset or token for Bitcoin. This version also fixes locking issues and the RPCs for funding transactions as well as omni_listtransactions.

**Due to the upgrade from Bitcoin Core 0.13.2 to 0.18.1, this version incorporates many changes, so please take your time to read through all release notes carefully. The first time you run this version, all the database is reconstructed, which can easily consume several hours.**

To avoid downtime of your system, running both versions on two instances is recommended and once v0.6.1 is up-to-date and it's behavior was confirmed to work, a hot swap may be done.

Please report bugs using the issue tracker on GitHub:

  https://github.com/OmniLayer/omnicore/issues


Table of contents
=================

- [Omni Core v0.7.0](#omni-core-v070)
- [Upgrading and downgrading](#upgrading-and-downgrading)
  - [How to upgrade](#how-to-upgrade)
  - [Downgrading](#downgrading)
  - [Compatibility with Bitcoin Core](#compatibility-with-bitcoin-core)
- [Imported changes and notes](#imported-changes-and-notes)
  - [Upgrade to Bitcoin Core 0.18.1](#upgrade-to-bitcoin-core-0181)
  - [Allow any token to be traded for Bitcoin](#allow-any-token-to-be-traded-for-bitcoin)
  - [Omni specific user-agent](#omni-specific-user-agent)
  - [getinfo deprecated](#getinfo-deprecated)
  - [Fee estimation improvements](#fee-estimation-improvements)
  - [Transaction index changes](#transaction-index-changes)
  - [rpcallowip option changes](#rpcallowip-option-changes)
  - [label and account APIs for wallet](#label-and-account-apis-for-wallet)
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

Upgrade to Bitcoin Core 0.18.1
------------------------------

The underlying base of Omni Core was upgraded from Bitcoin Core 0.13.2 to Bitcoin Core 0.18.1.

Please read the following release notes for further details very carefully:

- [Release notes for Bitcoin Core 0.14.0](https://github.com/bitcoin/bitcoin/blob/v0.18.1/doc/release-notes/release-notes-0.14.0.md)
- [Release notes for Bitcoin Core 0.14.1](https://github.com/bitcoin/bitcoin/blob/v0.18.1/doc/release-notes/release-notes-0.14.1.md)
- [Release notes for Bitcoin Core 0.14.2](https://github.com/bitcoin/bitcoin/blob/v0.18.1/doc/release-notes/release-notes-0.14.2.md)
- [Release notes for Bitcoin Core 0.14.3](https://github.com/bitcoin/bitcoin/blob/v0.18.1/doc/release-notes/release-notes-0.14.3.md)
- [Release notes for Bitcoin Core 0.15.0](https://github.com/bitcoin/bitcoin/blob/v0.18.1/doc/release-notes/release-notes-0.15.0.md)
- [Release notes for Bitcoin Core 0.15.0.1](https://github.com/bitcoin/bitcoin/blob/v0.18.1/doc/release-notes/release-notes-0.15.0.1.md)
- [Release notes for Bitcoin Core 0.15.1](https://github.com/bitcoin/bitcoin/blob/v0.18.1/doc/release-notes/release-notes-0.15.1.md)
- [Release notes for Bitcoin Core 0.15.2](https://github.com/bitcoin/bitcoin/blob/v0.18.1/doc/release-notes/release-notes-0.15.2.md)
- [Release notes for Bitcoin Core 0.16.0](https://github.com/bitcoin/bitcoin/blob/v0.18.1/doc/release-notes/release-notes-0.16.0.md)
- [Release notes for Bitcoin Core 0.16.1](https://github.com/bitcoin/bitcoin/blob/v0.18.1/doc/release-notes/release-notes-0.16.1.md)
- [Release notes for Bitcoin Core 0.16.2](https://github.com/bitcoin/bitcoin/blob/v0.18.1/doc/release-notes/release-notes-0.16.2.md)
- [Release notes for Bitcoin Core 0.16.3](https://github.com/bitcoin/bitcoin/blob/v0.18.1/doc/release-notes/release-notes-0.16.3.md)
- [Release notes for Bitcoin Core 0.17.0](https://github.com/bitcoin/bitcoin/blob/v0.18.1/doc/release-notes/release-notes-0.17.0.md)
- [Release notes for Bitcoin Core 0.17.0.1](https://github.com/bitcoin/bitcoin/blob/v0.18.1/doc/release-notes/release-notes-0.17.0.1.md)
- [Release notes for Bitcoin Core 0.17.1](https://github.com/bitcoin/bitcoin/blob/v0.18.1/doc/release-notes/release-notes-0.17.1.md)
- [Release notes for Bitcoin Core 0.18.0](https://github.com/bitcoin/bitcoin/blob/v0.18.0/doc/release-notes.md)
- [Release notes for Bitcoin Core 0.18.1](https://github.com/bitcoin/bitcoin/blob/v0.18.1/doc/release-notes.md)


Allow any token to be traded for Bitcoin
----------------------------------------

Right now the native distributed exchange of the Omni Layer protocol supports trading Omni and Test Omni for Bitcoin.

With this version, any token can be traded and there are no longer any trading restrictions.

Please note: this consensus change is not yet activated, but included in this release. An announcement will be made, when this feature is activated.


Omni specific user-agent
------------------------

As per default, starting with this version, Omni Core will identify itself as `/Satoshi:0.18.1 (Omni:0.7.0)/`.

With the configuration option `-omniuseragent=0` will hide the client as regular Bitcoin Core node.


`getinfo` deprecated
--------------------

The `getinfo` RPC has been deprecated. Each field in the RPC
has been moved to another commands output with that command also giving
additional information that `getinfo` did not provide. The following table
shows where each field has been moved to:

|`getinfo` field   | Moved to                                  |
|------------------|-------------------------------------------|
`"version"`	   | `getnetworkinfo()["version"]`
`"protocolversion"`| `getnetworkinfo()["protocolversion"]`
`"walletversion"`  | `getwalletinfo()["walletversion"]`
`"balance"`	   | `getwalletinfo()["balance"]`
`"blocks"`	   | `getblockchaininfo()["blocks"]`
`"timeoffset"`	   | `getnetworkinfo()["timeoffset"]`
`"connections"`	   | `getnetworkinfo()["connections"]`
`"proxy"`	   | `getnetworkinfo()["networks"][0]["proxy"]`
`"difficulty"`	   | `getblockchaininfo()["difficulty"]`
`"testnet"`	   | `getblockchaininfo()["chain"] == "test"`
`"keypoololdest"`  | `getwalletinfo()["keypoololdest"]`
`"keypoolsize"`	   | `getwalletinfo()["keypoolsize"]`
`"unlocked_until"` | `getwalletinfo()["unlocked_until"]`
`"paytxfee"`	   | `getwalletinfo()["paytxfee"]`
`"relayfee"`	   | `getnetworkinfo()["relayfee"]`
`"errors"`	   | `getnetworkinfo()["warnings"]`


Fee estimation improvements
---------------------------

Fee estimation has been significantly improved with Bitcoin Core 0.15, with more accurate fee estimates used by the wallet and a wider range of options for advanced users of the `estimatesmartfee` and `estimaterawfee` RPCs (See [PR 10199](https://github.com/bitcoin/bitcoin/pull/10199)).

### Changes to internal logic and wallet behavior

- Internally, estimates are now tracked on three different time horizons. This allows for longer targets and means estimates adjust more quickly to changes in conditions.
- Estimates can now be *conservative* or *economical*. *Conservative* estimates use longer time horizons to produce an estimate which is less susceptible to rapid changes in fee conditions. *Economical* estimates use shorter time horizons and will be more affected by short-term changes in fee conditions. Economical estimates may be considerably lower during periods of low transaction activity (for example over weekends), but may result in transactions being unconfirmed if prevailing fees increase rapidly.
- By default, the wallet will use conservative fee estimates to increase the reliability of transactions being confirmed within the desired target. For transactions that are marked as replaceable, the wallet will use an economical estimate by default, since the fee can be 'bumped' if the fee conditions change rapidly (See [PR 10589](https://github.com/bitcoin/bitcoin/pull/10589)).
- Estimates can now be made for confirmation targets up to 1008 blocks (one week).
- More data on historical fee rates is stored, leading to more precise fee estimates.
- Transactions which leave the mempool due to eviction or other non-confirmed reasons are now taken into account by the fee estimation logic, leading to more accurate fee estimates.
- The fee estimation logic will make sure enough data has been gathered to return a meaningful estimate. If there is insufficient data, a fallback default fee is used.

### Changes to fee estimate RPCs

- The `estimatefee` RPC is now deprecated in favor of using only `estimatesmartfee` (which is the implementation used by the GUI)
- The `estimatesmartfee` RPC interface has been changed (See [PR 10707](https://github.com/bitcoin/bitcoin/pull/10707)):
    - The `nblocks` argument has been renamed to `conf_target` (to be consistent with other RPC methods).
    - An `estimate_mode` argument has been added. This argument takes one of the following strings: `CONSERVATIVE`, `ECONOMICAL` or `UNSET` (which defaults to `CONSERVATIVE`).
    - The RPC return object now contains an `errors` member, which returns errors encountered during processing.
    - If Bitcoin Core has not been running for long enough and has not seen enough blocks or transactions to produce an accurate fee estimation, an error will be returned (previously a value of -1 was used to indicate an error, which could be confused for a feerate).
- A new `estimaterawfee` RPC is added to provide raw fee data. External clients can query and use this data in their own fee estimation logic.


Transaction index changes
-------------------------

The transaction index is now built separately from the main node procedure,
meaning the `-txindex` flag can be toggled without a full reindex. If omnicored
is run with `-txindex` on a node that is already partially or fully synced
without one, the transaction index will be built in the background and become
available once caught up. When switching from running `-txindex` to running
without the flag, the transaction index database will *not* be deleted
automatically, meaning it could be turned back on at a later time without a full
resync.

However, please note, Omni Core requires an enabled transaction index!

This change also has the effect that certain RPCs may not be available, until the
transaction index was fully built.

`rpcallowip` option changes
------------------------

The `rpcallowip` option can no longer be used to automatically listen on all network interfaces.  Instead, the `rpcbind` parameter must be used to specify the IP addresses to listen on. Listening for RPC commands over a public network connection is insecure and should be disabled, so a warning is now printed if a user selects such a configuration.  If you need to expose RPC in order to use a tool like  Docker, ensure you only bind RPC to your localhost, e.g. `docker run [...] -p 127.0.0.1:8332:8332` (this is an extra `:8332` over the normal Docker port specification).


`label` and `account` APIs for wallet
-------------------------------------

A new 'label' API has been introduced for the wallet. This is intended as a
replacement for the deprecated 'account' API. The 'account' has been removed.

The label RPC methods mirror the account functionality, with the following functional differences:

- Labels can be set on any address, not just receiving addresses. This functionality was previously only available through the GUI.
- Labels can be deleted by reassigning all addresses using the `setlabel` RPC method.
- There isn't support for sending transactions _from_ a label, or for determining which label a transaction was sent from.
- Labels do not have a balance.

Here are the changes to RPC methods:

| Deprecated Method       | New Method            | Notes       |
| :---------------------- | :-------------------- | :-----------|
| `getaccount`            | `getaddressinfo`      | `getaddressinfo` returns a JSON object with address information instead of just the name of the account as a string. |
| `getaccountaddress`     | n/a                   | There is no replacement for `getaccountaddress` since labels do not have an associated receive address. |
| `getaddressesbyaccount` | `getaddressesbylabel` | `getaddressesbylabel` returns a json object with the addresses as keys, instead of a list of strings. |
| `getreceivedbyaccount`  | `getreceivedbylabel`  | _no change in behavior_ |
| `listaccounts`          | `listlabels`          | `listlabels` does not return a balance or accept `minconf` and `watchonly` arguments. |
| `listreceivedbyaccount` | `listreceivedbylabel` | Both methods return new `label` fields, along with `account` fields for backward compatibility. |
| `move`                  | n/a                   | _no replacement_ |
| `sendfrom`              | n/a                   | _no replacement_ |
| `setaccount`            | `setlabel`            | Both methods now: <ul><li>allow assigning labels to any address, instead of raising an error if the address is not receiving address.<li>delete the previous label associated with an address when the final address using that label is reassigned to a different label, instead of making an implicit `getaccountaddress` call to ensure the previous label still has a receiving address. |

| Changed Method         | Notes   |
| :--------------------- | :------ |
| `addmultisigaddress`   | Renamed `account` named parameter to `label`. Still accepts `account` for backward compatibility if running with '-deprecatedrpc=accounts'. |
| `getnewaddress`        | Renamed `account` named parameter to `label`. Still accepts `account` for backward compatibility. if running with '-deprecatedrpc=accounts' |
| `listunspent`          | Returns new `label` fields. `account` field will be returned for backward compatibility if running with '-deprecatedrpc=accounts' |
| `sendmany`             | The `account` named parameter has been renamed to `dummy`. If provided, the `dummy` parameter must be set to the empty string, unless running with the `-deprecatedrpc=accounts` argument (in which case functionality is unchanged). |
| `listtransactions`     | The `account` named parameter has been renamed to `dummy`. If provided, the `dummy` parameter must be set to the string `*`, unless running with the `-deprecatedrpc=accounts` argument (in which case functionality is unchanged). |
| `getbalance`           | `account`, `minconf` and `include_watchonly` parameters are deprecated, and can only be used if running with '-deprecatedrpc=accounts' |


Change log
==========

The following list includes relevant pull requests merged into this release:

```
- #946 Update version to 0.5.0.99 to indicate development
- #957 fixed omni_listblocktransactions cmd typo
- #964 depends: Update fontconfig
- #981 Rebase Omni Core on Bitcoin Core 0.18
- #985 rpc: Use RPCHelpMan with Omni RPC calls
- #987 Update Gitian build for Omni
- #988 Omni Core 0.18.1
- #989 Remove deprecated boost calls
- #994 Raise CXX symbol level to 4.5.0
- #997 Bump version to Omni Core 0.6.0
- #1004 Run lint tests as close as possible to defaults and refactor as required
- #998 Add release notes for Omni Core 0.6.0
- #1010 Fix rescan minutes
- #1019 Set fSuccess true on valid wtxNew
- #1020 Updates to omni_funded_send
- #1022 Fix #1011: different transactions getting the same sortKey in FetchWalletOmniTransactions
- #1023 Remove lint white space check
- #1024 Lock cs_main and cs_tally when calling LoadAlerts
- #1026 Lock cs_main before calling AcceptToMemoryPool
- #1027 Fail funded send TXs with more outputs than expected
- #1035 Set Omni bespoke user agent string
- #1038 Allow any token to be traded for Bitcoin
- #1034 Prepare release of Omni Core v0.7.0
```


Credits
=======

Thanks to everyone who contributed to this release, especially to Peter Bushnell and all Bitcoin Core developers.
