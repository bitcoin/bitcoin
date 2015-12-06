Omni Core v0.0.10
=================

v0.0.10 is a major release and consensus critical in terms of the Omni Layer protocol rules. An upgrade is mandatory, and highly recommended. Prior releases will not be compatible with new behavior in this release.

Please report bugs using the issue tracker on GitHub:

  https://github.com/OmniLayer/omnicore/issues

Table of contents
=================

- [Omni Core v0.0.10](#omni-core-v0010)
- [Upgrading and downgrading](#upgrading-and-downgrading)
  - [How to upgrade](#how-to-upgrade)
  - [Downgrading](#downgrading)
  - [Compatibility with Bitcoin Core](#compatibility-with-bitcoin-core)
- [Imported changes and notes](#imported-changes-and-notes)
  - [Upgrade to Bitcoin Core 0.10.4](#upgrade-to-bitcoin-core-0104)
  - [Headers-first synchronization](#headers-first-synchronization)
  - [Dust threshold values](#dust-threshold-values)
  - [Transaction fee changes](#transaction-fee-changes)
  - [Rebranding to Omni Core](#rebranding-to-omni-core)
  - [Incompatible API changes](#incompatible-api-changes)
  - [Feature and consensus rule activations](#feature-and-consensus-rule-activations)
- [Consensus affecting changes](#consensus-affecting-changes)
  - [Data embedding with OP_RETURN](#data-embedding-with-op_return)
  - [Distributed token exchange](#distributed-token-exchange)
  - [Remove side effects of granting tokens](#remove-side-effects-of-granting-tokens)
  - [Disable DEx "over-offers" and use plain integer math](#disable-dex-over-offers-and-use-plain-integer-math)
  - [New "send-all" transaction type](#new-send-all-transaction-type)
  - [Don't allow ecosystem crossovers for crowdsales](#dont-allow-ecosystem-crossovers-for-crowdsales)
- [Other notable changes](#other-notable-changes)
  - [JSON-RPC API updates](#json-rpc-api-updates)
  - [Debug logging categories](#debug-logging-categories)
  - [Configuration options](#configuration-options)
  - [Various performance improvements](#various-performance-improvements)
  - [Built-in consensus checks](#built-in-consensus-checks)
  - [Checkpoints and faster initial transaction scanning](#checkpoints-and-faster-initial-transaction-scanning)
  - [Disable-wallet mode](#disable-wallet-mode)
  - [CI and testing of Omni Core](#ci-and-testing-of-omni-core)
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

Downgrading to an Omni Core version prior 0.0.10 is generally not supported as older versions will not provide accurate information due to the changes in consensus rules.

Compatibility with Bitcoin Core
-------------------------------

Omni Core is based on Bitcoin Core 0.10.4 and can be used as replacement for Bitcoin Core. Switching between Omni Core and Bitcoin Core is fully supported at any time.

Downgrading to a Bitcoin Core version prior 0.10 is not supported due to the new headers-first synchronization.

Imported changes and notes
==========================

Upgrade to Bitcoin Core 0.10.4
------------------------------

The underlying base of Omni Core was upgraded from Bitcoin Core 0.9.5 to Bitcoin Core 0.10.4.

Please see the following release notes for further details:

- [Release notes for Bitcoin Core 0.10.0](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/doc/release-notes/release-notes-0.10.0.md)
- [Release notes for Bitcoin Core 0.10.1](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/doc/release-notes/release-notes-0.10.1.md)
- [Release notes for Bitcoin Core 0.10.2](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/doc/release-notes/release-notes-0.10.2.md)
- [Release notes for Bitcoin Core 0.10.3](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/doc/release-notes/release-notes-0.10.3.md)
- [Release notes for Bitcoin Core 0.10.4](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/doc/release-notes.md)

Headers-first synchronization
-----------------------------

With the upgrade to Bitcoin Core 0.10 headers-first synchronization is supported and blocks are downloaded in parallel.

Blocks are no longer stored in order on disk, and as result, the block files and databases are not backwards-compatible with older versions of Bitcoin Core prior 0.10.

Dust threshold values
---------------------

The default `minrelaytxfee` was raised from `0.00001` to `0.00005` in Bitcoin Core 0.10.3 as temporary measure against massive memory pool bloat.

The minimum relay fee influences the "dust threshold", and has an impact on the output values of Omni transactions, which are chosen to be as low as possible. As per default, Omni transactions created with Master Core had output values between `0.00000546` BTC and `0.00000882` BTC, whereby the new output values are between `0.0000273` BTC and `0.0000441` BTC.

To continue to create transactions with lower values, start Omni Core with `-minrelaytxfee=0.00001` or add the following to your `bitcoin.conf`:
```
minrelaytxfee=0.00001
```

Lowering the `minrelaytxfee` may result in higher memory consumption, and too low values can result in delayed transaction propagation. A value less than than `0.00001` is generally not recommended.

Transaction fee changes
-----------------------

Starting with Bitcoin Core 0.10, transaction fees, as per default, are no longer hardcoded, but estimated based on previous blocks.

This behavior can result in significantly different fees compared to Master Core 0.0.9, and manual tweaking is recommended, if the default fee estimation doesn't yield satisfying results.

The following fee related configuration options are available:

- `txconfirmtarget=<n>`: create transactions that have enough fees (or priority) so they are likely to begin confirmation within n blocks (default: `6`). This setting is overridden by the `paytxfee` option.
- `paytxfee=<amount>`: fee (in BTC/kB) to add to transactions you send.
- `sendfreetransactions=0|1`: send transactions as zero-fee transactions if possible (default: `0`).

New RPC commands for fee estimation:

- `estimatefee nblocks`: returns approximate fee-per-1,000-bytes needed for a transaction to begin confirmation within nblocks. Returns `-1` if not enough transactions have been observed to compute an estimate.
- `estimatepriority nblocks`: returns approximate priority needed for a zero-fee transaction to begin confirmation within nblocks. Returns `-1` if not enough free transactions have been observed to compute an estimate.

Please note, the fee estimation is not necessarily accurate.


Rebranding to Omni Core
-----------------------

Master Core was re-branded to Omni Core on all levels:

- `mastercored`, `mastercore-cli`, `mastercore-qt`, `test_mastercore` and `test_mastercore-qt` were renamed to `omnicored`, `omnicore-cli`, `omnicore-qt`, `test_omnicore` and `test_omnicore-qt`
- the debug log file `mastercore.log` was renamed  to `omnicore.log`
- the hardcoded token SP#1 was renamed from `"MasterCoin"` to `"Omni"`
- the hardcoded token SP#2 was renamed from `"Test MasterCoin"` to `"Test Omni"`
- the user interface now refers to `"Omni"`, `"OMNI"`, `"Test Omni"` or `"TOMNI"` instead of `"Mastercoin"`, `"MSC"`, `"Test Mastercoin"` or `"TMSC"`

Incompatible API changes
------------------------

The field `"subaction"` in the RPC response for traditional DEx orders of `"gettransaction_MP"` as well as `"listtransaction_MP"` was renamed to `"action"`.

The values associated with `"action"` were renamed from `"New"`, `"Update"`, `"Cancel"` to `"new"`, `"update"`, `"cancel"`.

Feature and consensus rule activations
--------------------------------------

Omni Core 0.0.10 introduces feature activations, a new concept that allows the Omni team to decouple the release process from the process of making new features live.

Prior to 0.0.10, the block height that a new feature would be made live was hard coded, placing substantial limits on both the frequency of releases and the flexibility in introducing new features. The feature activations system allows to separate the release process and activation of features by allowing the Omni team to activate features with a special Omni transaction instead of with a hard coded block height within a specific release.

The activations system imposes set limits on when and by whom new features can be activated. A minimum 2048 block notice period (roughly 2 weeks) is enforced on the main network to give users with older incompatible clients time to upgrade.

Prior to all activations, additional notifications are published via different channels, such as:

- [the mailing list for developers](https://groups.google.com/a/mastercoin.org/d/forum/dev)
- [the mailing list for announcements](https://groups.google.com/a/mastercoin.org/d/forum/announcements)
- http://blog.omni.foundation

As per default Omni Core accepts activation messages from the following source:
```
{
  "address": "3Fc5gWzEQh1YGeqVXH6E4GDEGgbZJREJQ3",
  "redeemScript": "542102d797b8526701a3dfdb52d11f89377e0288b14e29b1414d64de065cd337069c3b21036a4caa95ec1d55f1b75a8b6c7345f22b4efc9e25d38ab058ef7d6f60b3b744f7410499e86235a6a98fc295d7cfe641d37409f2840ad32e0211579cae488bd86cf01daf7ad8f082d968ea4bca77e794fffd1a31583f36f37b1198e51d0651cdbcf3214104b7a3d7f7ccdf211dfd180815b87332b4773cc40bff72a4d0bb60f3a85409d19f99709331c6b11c976fe274a86d789a1cf2b3b0be29fe5fc55c93ad9e08459c4f4104e65b098558d637cfcf3194214637f8838338b141259b698d2a027b069d405b6502ad4a4e9aa75094fa431a6c9af580f5917834a6d4cec946054df33194b2967855ae"
}
```

The activation message must be signed by at least 4 of the 5 nominated members of the Omni team in order to be accepted. These members are as follows:

- Zathras - Project Maintainer and Developer
- dexx - Project Maintainer and Developer
- J.R. Willett - Founder and Board Member
- Craig Sellars - Technologist and Board Member
- Adam Chamely - Project Maintainer and Developer

The transaction format of activation messages is as follows:

| **Field**                  | **Type**        | **Example**                        |
| -------------------------- | --------------- | ---------------------------------- |
| Transaction version        | 16-bit unsigned | 65535                              |
| Transaction type           | 16-bit unsigned | 65534                              |
| Feature identifier         | 16-bit unsigned | 1 (Class C activation)             |
| Activation block           | 32-bit unsigned | 385000                             |
| Min. client version        | 32-bit unsigned | 1000000 (Omni Core 0.0.10)         |

Should a particular client version not support a new feature when it is activated, the client will shut down to prevent providing potentially inaccurate data. In this case the client should be upgraded. Whilst this behavior can be overridden with the configuration option `-overrideforcedshutdown`, it is strongly recommended against.

Senders of activation messages can be whitelisted or blacklisted with the configuration options `-omnialertallowsender=<sender>` and `-omnialertignoresender=<sender>`. Please be aware that overriding activations may cause the client to provide inaccurate data that does not reflect the rest of the network.

The GUI displays warnings for pending activations and daemon users can view pending and completed activations via the [omni_getactivations](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#omni_getactivations) JSON-RPC API call.

Consensus affecting changes
===========================

Starting with this version, all changes of the consensus rules are enabled by activation transactions.

Please note, while Omni Core 0.0.10 contains support for several new rules and features they are not enabled immediately and will be activated via the feature activation mechanism described above.

It follows an overview and a description of the consensus rule changes:

Data embedding with OP_RETURN
-----------------------------

Omni Core 0.0.10 contains support for Class C transactions which move the data encoding from bare-multisig data outputs to an `OP_RETURN` output. `OP_RETURN` outputs carry no output value and can be immediately pruned.

In addition to migrating data encoding to `OP_RETURN`, the requirement to send an output to the Exodus address has also been removed with Class C. Instead the bytes `0x6f6d6e69` ("omni") are prefixed to the data payload to provide marker identification.

This helps to address a common criticism of systems that store data on the Bitcoin blockchain; "UTXO bloat". Since Class C transactions no longer store the data in spendable outputs, there is no need to store them in the UTXO set, and since the outputs are not stored in the UTXO set, Class C transactions do not contribute to its growth.

Due to size and count restrictions of `OP_RETURN` outputs currently enforced by the Bitcoin network, the client will automatically fall back to Class B (bare-multisig) in the event that a transaction is too large to send via Class C (for example a property creation with lots of metadata).

The configuration option `-datacarriersize=<n bytes>` can be used to set the maximal size of `OP_RETURN` payloads, and is set to `40` bytes as per default. A value of `0` can be used to disable Class C encoding completely.

This change is identified by `"featureid": 1` and labeled by the GUI as `"Class C transaction encoding"`.

Distributed token exchange
--------------------------

The distributed token exchange, or "MetaDEx", was integrated into Omni Core, to support trading of tokens with automated order matching.

Four new transaction types were added.

##### Create order:

| **Field**                  | **Type**        | **Example**                        |
| -------------------------- | --------------- | ---------------------------------- |
| Transaction version        | 16-bit unsigned | 0                                  |
| Transaction type           | 16-bit unsigned | 25                                 |
| Tokens to list for sale    | 32-bit unsigned | 2147483831 (Gold Coins)            |
| Amount to list for sale    | 64-bit signed   | 5000000000 (50.0 divisible tokens) |
| Tokens desired in exchange | 32-bit unsigned | 2 (Test Omni)                      |
| Amount desired in exchange | 64-bit signed   | 1000000000 (10.0 divisible tokens) |

Transaction type 25 can be used to create a new order.

##### Cancel all orders at price:

| **Field**                  | **Type**        | **Example**                        |
| -------------------------- | --------------- | ---------------------------------- |
| Transaction version        | 16-bit unsigned | 0                                  |
| Transaction type           | 16-bit unsigned | 26                                 |
| Tokens listed for sale     | 32-bit unsigned | 2147483831 (Gold Coins)            |
| Amount listed for sale     | 64-bit signed   | 2500000000 (25.0 divisible tokens) |
| Tokens desired in exchange | 32-bit unsigned | 2 (Test Omni)                      |
| Amount desired in exchange | 64-bit signed   | 500000000 (5.0 divisible tokens)   |

Transaction type 26 cancels open orders for a given set of currencies at a given price. It is required that the token identifiers and price exactly match the order to be canceled.

##### Cancel all orders of currency pair:

| **Field**                  | **Type**        | **Example**                        |
| -------------------------- | --------------- | ---------------------------------- |
| Transaction version        | 16-bit unsigned | 0                                  |
| Transaction type           | 16-bit unsigned | 27                                 |
| Tokens listed for sale     | 32-bit unsigned | 2147483831 (Gold Coins)            |
| Tokens desired in exchange | 32-bit unsigned | 2 (Test Omni)                      |

Transaction type 27 cancels all open orders for a given set of two currencies (one side of the order book).

##### Cancel all orders in ecosystem:

| **Field**                  | **Type**        | **Example**                        |
| -------------------------- | --------------- | ---------------------------------- |
| Transaction version        | 16-bit unsigned | 0                                  |
| Transaction type           | 16-bit unsigned | 28                                 |
| Ecosystem                  | 8-bit unsigned  | 2 (test ecosystem)                 |

Transaction type 28 can be used to cancel all open orders for all currencies in the specified ecosystem.

Transactions for the distributed token exchange are enabled on testnet, in regtest mode and when trading tokens in the test ecosystem. Transactions targeting the main ecosystem are supported after the feature activation.

See also: [omni_sendtrade](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#omni_sendtrade), [omni_sendcanceltradesbyprice](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#omni_sendcanceltradesbyprice), [omni_sendcanceltradesbypair](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#omni_sendcanceltradesbypair), [omni_sendcancelalltrades](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#omni_sendcancelalltrades), [omni_gettrade](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#omni_gettrade), [omni_getorderbook](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#omni_getorderbook), [omni_gettradehistoryforpair](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#omni_gettradehistoryforpair), [omni_gettradehistoryforaddress](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#omni_gettradehistoryforaddress) JSON-RPC API calls

This change is identified by `"featureid": 2` and labeled by the GUI as `"Distributed Meta Token Exchange"`.

Remove side effects of granting tokens
--------------------------------------

The transaction type "grant tokens" may trigger crowdsale participations as secondary effect, if the recipient of granted tokens has an active crowdsale, which issues tokens in exchange for the granted tokens.

Once this change is activated, "grant tokens" no longer triggers passive crowdsale participations.

This change is identified by `"featureid": 4` and labeled by the GUI as `"Remove grant side effects"`.

Disable DEx "over-offers" and use plain integer math
----------------------------------------------------

After this feature is enabled, it is no longer valid to create orders on the traditional distributed exchange, which offer more than the seller has available, and the amounts are no longer adjusted based on the actual balance. Previously such a transaction was valid, and the whole available amount was offered.

Plain integer math (instead of floating point numbers) is used to determine the purchased amount. The purchased amount is rounded up, which may be in favor of the buyer, to avoid small leftovers of 1 willet. This is not exploitable due to transaction fees.

This change is identified by `"featureid": 5` and labeled by the GUI as `"DEx integer math update"`.

New "send-all" transaction type
-------------------------------

A new transaction type was added to send all tokens an entity has in one single transaction.

Transactions with type 4 transfer available balances of all tokens in the specified ecosystem from the source to the reference destination. Transactions of this type are considered valid, if at least one token was transferred, and invalid otherwise.

The transaction format is as follows:

| **Field**                  | **Type**        | **Example**                        |
| -------------------------- | --------------- | ---------------------------------- |
| Transaction version        | 16-bit unsigned | 0                                  |
| Transaction type           | 16-bit unsigned | 4                                  |
| Ecosystem                  | 8-bit unsigned  | 2 (test ecosystem)                 |

"Send-all" transactions are enabled on testnet, in regtest mode and when transferring tokens in the test ecosystem. Transactions targeting the main ecosystem are supported after the feature activation.

See also: [omni_sendall](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#omni_sendall) JSON-RPC API call

This change is identified by `"featureid": 6` and labeled by the GUI as `"Send All transactions"`.

Don't allow ecosystem crossovers for crowdsales
-----------------------------------------------

A design goal of different ecosystems was to seperate both systems cleanly. However, this was not enforced for crowdsales, and it was possible to create crowdsales, which issue test ecosystem tokens in exchange for main ecosystem tokens and vice versa.

After the feature activation, the tokens to issue via crowdsale must be in the same ecosystem as the tokens desired.

This change is identified by `"featureid": 7` and labeled by the GUI as `"Disable crowdsale ecosystem crossovers"`.

Other notable changes
=====================

JSON-RPC API updates
--------------------

The JSON-RPC API was extended significantly, and now supports sending transactions of any transaction type, and the creation of raw Omni transactions.

Several new API calls are available to retrieve information about unconfirmed or confirmed Omni transactions, and the state of the system.

Please see the JSON-RPC API documentation for more details:

- https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md

Debug logging categories
------------------------

Debug logging is categorized and default logging options may be changed with the `-omnidebug` parameter. Use multiple `-omnidebug` parameters to specify multiple categories.

The following categories are available:

- `parser`: Log additional information when parsing
- `parser_data`: Log verbose additional information when parsing
- `parser_dex`: Log additional information about DEx payments when parsing
- `parser_readonly`: Log extra information when parsing in read only mode (e.g. via RPC interface)
- `verbose`: Log verbose information about sender identification
- `vin`: Log additional information about the inputs used when parsing
- `script`: Log additional information about the scripts used when parsing
- `dex`: Log additional information about DEx transactions
- `tokens`: Log additional information about input selection
- `spec`: Warn about non-sequential sequence numbers in Class B transactions
- `exo`: Log additional information about actions involving the Exodus address
- `tally`: Log before/after values when balances are updated
- `sp`: Log additional information about actions involving smart property updates
- `sto`: Log additional information about Send To Owners calculations
- `txdb`: Log additional information about interactions with the transaction database
- `tradedb`: Log additional information about interactions with the trades database
- `persistence`: Log additional information about interactions with the persistent state files
- `pending`: Log additional information about pending transactions
- `metadex1`: Log additional information about MetaDEx transactions
- `metadex2`: Log the state of the MetaDEx before and after each MetaDEx transaction
- `metadex3`: Log the state of the MetaDEx before and after each MetaDEx action and log each MetaDEx object during iteration of MetaDEx maps
- `packets`: Log additional information about the payload included within each Omni transaction
- `packets_readonly`: Log additional information about the payload included within each Omni transaction when parsing in read only mode (e.g. via RPC interface)
- `walletcache`: Log additional information about the wallet cache and hits/misses
- `consensus_hash`: Log additional information about each data point added to the consensus hash
- `consensus_hash_every_block`: Generate and log a consensus hash for every block processed
- `alerts`: Log additional information about alert transactions

Special keywords:

- `all`: Enable all logging categories (note: extremely verbose)
- `none`: Disable all logging categories

Usage examples:
```
omnicored -omnidebug=all
```
```
omnicored -omnidebug=parser -omnidebug=parser_data -omnidebug=vin -omnidebug=script
```

Configuration options
---------------------

For a listing of other Omni Core specific configuration options see:

- https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/configuration.md

Various performance improvements
--------------------------------

Omni Core was improved on several levels to increase performance and responsiveness.

Previously RPC requests were blocking, creating a bottleneck for the JSON-RPC API and the whole application. Due to much finer and more targeted locking of critical sections, threads are now only blocked when they need to be, which allows concurrent requests without immediately creating a queue. The number of threads designated to handle RPC requests can be configured with the option `-rpcthreads=<n threads>`, which is set to `4` threads as per default.

The serialization of token related database entries was previously JSON based, and the time to retrieve information about transactions, tokens or crowdsales, for example with [omni_gettransaction](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#omni_gettransaction), [omni_getproperty](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#omni_getproperty), [omni_getcrowdsale](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#omni_getcrowdsale) or [omni_getactivecrowdsales](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#omni_getactivecrowdsales), was primarily delayed by slow conversions of the data stored in the database. Entries are now stored in their raw byte representation, which allows much faster deserialization of the data. Benchmarks have shown this update provides a speed improvement of about 8x or more for these calls.

Most notably on Microsoft Windows, many updates of the GUI, for example when connecting a new block, created the impression of a non-responsive user interface, because the client couldn't keep up with the updates. This was optimized such that no queue of update events is created in the first place, and new update requests are dropped, in case there are already events which haven't been handled.

Built-in consensus checks
-------------------------

It is mandatory that the state of the system is similar for all participants, and Omni Core 0.0.10 introduces the concept of "consensus hashes", which are a compact representation of the state. This allows to compare the state of the system with other participants, and to compare historical state with known reference states, to ensure consensus.

To log consensus hashes, the configuration options `-omnidebug=consensus_hash` and `-omnidebug=consensus_hash_every_block` can be used.

Checkpoints and faster initial transaction scanning
---------------------------------------------------

When Omni Core is used the very first time, or when Omni Core hasn't been used for a longer period, then historical blocks are scanned for Omni transactions. With the introduction of "consensus hashes", a significant speed improvement during the initial stage was possible by skipping known blocks without Omni transactions, and by comparing the resulting state with checkpoints. If the state of the client doesn't match the reference state, the client shuts down to prevent providing inaccurate data.

Omni Core was intentionally not delivered with a snapshot of the most recent state, and transactions are fully verified in any case. One design goal of Omni Core has always been to minimize the role of central parties (in this case: the ones providing a snapshot or checkpoints), and to further minimize the dependency on a hardcoded list of information, the configuration option `-omniseedblockfilter=0` can be used to disable the skipping of blocks without Omni transactions.

The configuration option `-overrideforcedshutdown` can be used to prevent Omni Core from shutting down in case of a checkpoint mismatch. Using the latter is generally not recommended, as this exposes the user to data, which isn't considered as valid by other participants.

Disable-wallet mode
-------------------

Since this version Omni Core can be build and used without wallet support.

This can be handy, when using Omni Core as plain data provider, without the need to send transactions. [Data retrieval](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#data-retrieval) and [raw transaction](https://github.com/OmniLayer/omnicore/blob/omnicore-0.0.10/src/omnicore/doc/rpc-api.md#raw-transactions) API calls are enabled in this mode.

To compile Omni Core without wallet support (illustrated for Unix):
```
./autogen.sh
./configure --disable-wallet
make
```

To disable to wallet via configuration options start Omni Core with `-disablewallet`, or add the following to your `bitcoin.conf`:
```
disablewallet=1
```

CI and testing of Omni Core
---------------------------

As of now, Omni Core 0.10 has nearly 5000 lines of source code for unit tests in addition to the unit tests inherited from Bitcoin Core. Furthermore [OmniJ](https://github.com/OmniLayer/OmniJ#omni-integration-testing-with-spock-framework) is used for integration testing.

To run the unit tests locally, start `test_omnicore` or `test_omnicore-qt`. On Unix, the integration tests can be started with `qa/pull-tester/omnicore-rpc-tests.sh`. Please note, the script for the integration tests is not included in the release packages. For other operating systems, or to manually run the integration tests, see the [instructions for OmniJ](https://github.com/OmniLayer/OmniJ#omni-integration-testing-with-spock-framework).

Travis CI was integrated into the work flow, to automatically build and test Omni Core on several different platforms and operating systems.

Change log
==========

The following list includes relevant pull requests merged into this release:
```
- #1, #2 Upgrade code base to Bitcoin Core 0.10.2
- #7 Select only coins, which can be spent
- #4 Fix shutdown issue after running Boost tests
- #8, #26, #33, #35, #67 Finalize MetaDEx logic
- #5 MetaDEx user interface
- #13 Support creation of any (enabled) Omni transaction via RPC
- #13 Support Class C/OP_RETURN encoded transactions
- #17 Report initial parsing progress during startup
- #17 Handle shutdown requests during initial parsing
- #42 Integrate Travis CI and OmniJ into workflow
- #50 Allow debug levels to be specified without needing to recompile
- #34 Set `txindex=1` flag during startup, if confirmed by user
- #68 Various code cleanups and improved code documentation
- #68 Resolve and refine several `LOCKs`
- #68 Support concurrent RPC queries
- #79 Show script verification errors in `signrawtransaction` result
- #81 Enable pay-to-script-hash support in send dialog
- #86 Return all available information via `validateaddress`
- #98 Indicate "pending" status of outgoing transactions
- #109 Log every invalid processing of Omni transactions
- #91 Rebrand project to Omni Core
- #91 Rename files to `omnicore-qt`, `omnicored`, `omnicore-cli`, ...
- #102, #126 Fully support cross-platform, and deterministic, building
- #113, #118 Unify and improve RPC help descriptions
- #121 Fix shutdown issue after declining to reindex blockchain
- #126 Add images to installer icon for deterministic building
- #127 Reduce math and big numbers in crowdsale participation
- #128 Run OmniJ tests with all build targets except OS X
- #131 When closing a crowdsale, write info to debug log, not file
- #133 Report progress based on transactions, report estimated time remaining
- #132 Reduce amount of verbose logging by default
- #140 Use higher resolution program icons
- #145 Implement tally hashing for consensus testing and checkpoints
- #146 Fix trading restrictions for testnet
- #136 Skip loading unneeded blocks during initial parsing
- #150 #152 Add MetaDEx guide documentation
- #149 Update Omni Core API documentation
- #156 Move simple send logic and friends to the other logic methods
- #159 Implement and switch to Feature-Activation-By-Message
- #161 Don't spam log about non-sequential seqence numbers
- #163 Prepare deactivation of "grant tokens" side effects
- #164 Add hidden RPC command to activate features
- #158 Trigger UI updates after DEx payments and Exodus purchases
- #173 Fix/add missing fields for transaction retrieval via RPC
- #177 Sanitize RPC responses and replace non-UTF-8 compliant characters
- #178 Fix Omni transaction count value passed into block end handler
- #179 Add consensus checkpoint for block 370000
- #180 Update seed blocks to 370000
- #181 Update base to Bitcoin Core 0.10~ tip
- #174 Add RPC support for unconfrimed Omni transactions
- #184 Stop recording structurally-invalid transactions in txlistdb
- #167 Mostly cosmetic overhaul of traditional DEx logic
- #187 Automate state refresh on client version change when needed
- #195 Refine amount recalculations of traditional DEx via RPC
- #160 Improve wallet handling and support --disable-wallet builds
- #197 Reject DEx "over-offers" and use plain integer math
- #203 Minor cleanup of MetaDEx status and RPC output
- #207 No longer allow wildcard when cancelling all MetaDEx orders
- #209 Remove unused addresses in MetaDEx cancel dialog after update
- #210 Don't restrict number of trades and transactions in UI history
- #213 Refine RPC input parsing related to addresses
- #217 Rebrand "Mastercoin" to "Omni"
- #218 Fix/remove empty build target of Travis CI
- #220 Fix Bitcoin balance disappears from the UI on reorg
- #221 Don't call StartShutdown() after updating config
- #223 Fix calculation of crowdsale participation on Windows
- #224 Update links to moved API documentation
- #225 Rebrand token and fix links to RPC API sections
- #227 Do not save state when a block fails checkpoint validation
- #229 Minor update to MetaDEx guide
- #238 Fix crowdsale purchase detection in test ecosystem
- #231 Change icon to make Win64 installer deterministic
- #240 Various small improvements to ensure correct state
- #232 Add new RPC to list pending Omni transactions
- #243 Fix STO DB corruption on reorg
- #239 Fix "raised amount" in RPC result of crowdsales
- #234 Update documentation of configuration options
- #233 Use tables and add results to JSON-RPC API documentation
- #250 Fix missing crowdsale purchase entries in verbose "omni_getcrowdsale"
- #253 Update handling of crowdsales and missing bonus amounts
- #142 Add RPC to decode raw Omni transactions
- #176 Implement updated alerting & feature activations
- #252 Restrict ecosystem crossovers for crowdsales
- #258 Explicitly set transaction and relay fee for RPC tests
- #260 Add ARM and a no-wallet build as build target for Travis CI
- #262 Filter empty balances in omni_getall* RPC calls
- #263 Update base to Bitcoin Core 0.10.3
- #77 Support creation of raw transactions with non-wallet inputs
- #273 Remove "Experimental UI" label GUI splash screen
- #274 Add consensus checkpoint for block 380000
- #276 Update seed blocks for blocks 370,000 to 380,000
- #278 Replace splashes to remove OmniWallet branding
- #269 Add release notes for Omni Core 0.0.10
- #162 Bump version to 0.0.10.0-rc1
- #282 Update base to Bitcoin Core 0.10.4
- #285 Don't use "N/A" label for transactions with type 0
- #286 Bump version to Omni Core 0.0.10-rc3
- #288 Expose payload over RPC and add payload size
- #291 Add error handlers for "omni_getpayload"
- #292 Add API documentation for "omni_getpayload"
- #295 Fix overflow when trading divisible against divisible
- #303 Force UI update every block with Omni transactions
- #305 Change default confirm target to 6 blocks
- #307 Update release notes for 0.0.10
- #306 Bump version to Omni Core 0.0.10-rel
```

Credits
=======

Thanks to everyone who contributed to this release, and especially the Bitcoin Core developers for providing the foundation for Omni Core!
