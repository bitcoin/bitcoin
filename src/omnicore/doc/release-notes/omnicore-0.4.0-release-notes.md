Omni Core v0.4.0
================

v0.4.0 is a major release and consensus critical in terms of the Omni Layer protocol rules. An upgrade is mandatory, and highly recommended. Prior releases may not be compatible with new behaviour in this release.

Please report bugs using the issue tracker on GitHub:

  https://github.com/OmniLayer/omnicore/issues

Table of contents
=================

- [Omni Core v0.4.0](#omni-core-v040)
- [Upgrading and downgrading](#upgrading-and-downgrading)
  - [How to upgrade](#how-to-upgrade)
  - [Downgrading](#downgrading)
  - [Compatibility with Bitcoin Core](#compatibility-with-bitcoin-core)
- [Notable changes](#notable-changes)
  - [Add new RPC: "omni_listblockstransactions"](#add-new-rpc-omni_listblockstransactions)
  - [Fix RPC edge case of not identifying crowdsale purchase](#fix-rpc-edge-case-of-not-identifying-crowdsale-purchase)
  - [Show "ecosystem" = "all", when all tokens are moved](#show-ecosystem--all-when-all-tokens-are-moved)
  - [Log failures when trying to restore state](#log-failures-when-trying-to-restore-state)
  - [Add system for random database consistency checks](#add-system-for-random-database-consistency-checks)
  - [Add checkpoint for block 562708](#add-checkpoint-for-block-562708)
  - [Internal database related code improvements](#internal-database-related-code-improvements)
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

Downgrading to an Omni Core version prior to 0.4.0 is generally not advised as older versions may not provide accurate information due to the changes in consensus rules.

Compatibility with Bitcoin Core
-------------------------------

Omni Core is based on Bitcoin Core 0.13.2 and can be used as replacement for Bitcoin Core. Switching between Omni Core and Bitcoin Core may be supported.

Upgrading to a higher Bitcoin Core version is generally supported, but when downgrading from Bitcoin Core 0.15, Omni Core needs to be started with `-reindex-chainstate` flag, to rebuild the chainstate data structures in a compatible format.

Downgrading to a Bitcoin Core version prior to 0.12 may not be supported due to the obfuscation of the blockchain database. In this case the database also needs to be rebuilt by starting Omni Core with `-reindex-chainstate` flag.

Downgrading to a Bitcoin Core version prior to 0.10 is not supported due to the new headers-first synchronization.

Notable changes
===============

Add new RPC: "omni_listblockstransactions"
-----------------------------------------

The new RPC "omni_listblockstransactions" can be used to retrieve an unordered list of Omni transactions within a range of blocks:

---

### omni_listblockstransactions

Lists all Omni transactions in a given range of blocks.

Note: the list of transactions is unordered and can contain invalid transactions!

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `firstblock`        | number  | required | the index of the first block to consider                                                     |
| `lastblock`         | number  | required | the index of the last block to consider                                                      |

**Result:**
```js
[      // (array of string)
  "hash",  // (string) the hash of the transaction
  ...
]
```

**Example:**

```bash
$ omnicore-cli "omni_omni_listblocktransactions" 279007 300000
```

---


Fix RPC edge case of not identifying crowdsale purchase
-----------------------------------------

When a "Simple Send" transaction is also a "Crowdsale Purchase", always return "Crowdsale Purchase" for "type", when using "omni_gettransaction".


Show "ecosystem" = "all", when all tokens are moved
-----------------------------------------

When moving all tokens with the "Send All" transaction and no specific ecosystem is selected, show "all" for "ecosystem", when getting information about such a transaction with "omni_gettransaction".


Log failures when trying to restore state
-----------------------------------------

When, due to whatever reason, a rescan of Omni Layer transactions is triggered during a start, the reason for the rescan is written to the log file.


Add system for random database consistency checks
-----------------------------------------

During startup, the existence of a collection of historical transactions is checked to detect DB inconsistencies. In this case, all Omni Layer transcations are rescanned during the start.


Add checkpoint for block 562708
-----------------------------------------

To further ensure consensus consistency, a more up-to-date checkpoint was added:

```
{
  "block": 562710,
  "blockhash": "0000000000000000001673ef66bfbc02946c1ff7f42e8aef72d875ab7044de1e",
  "consensushash": "0be8eadf798cc595db247b85617815c936a1e607ac8faab6dec44b2ee585bd51"
}
```


Internal database related code improvements
-----------------------------------------

Pointer names of all databases were renamed and unified to match actual database names.


Change log
==========

The following list includes relevant pull requests merged into this release:
```
- #773 Log failures when trying to restore state
- #769 Don't create two similar outputs, when funding transactions
- #768 Update version to 0.3.1.99 to indicate development
- #779 Refine documentation of new funding RPCs
- #835 Add new RPC: omni_listblockstransactions
- #848 Fix RPC edge case of not identifying crowdsale purchase
- #851 Unify pointer names of internal DBs
- #874 Fix log incompability of invalid datetime
- #878 Show "ecosystem" = "all", when all tokens are moved
- #879 Bump version to Omni Core 0.4.0
- #881 Add consensus hash for block 562708
- #882 Add system to check existence of historical transactions
- #884 Remove two transactions from probing
- #885 Rename Mastercoin to Omni Layer in error message
- #883 Add release notes for Omni Core 0.4.0
```

Credits
=======

Thanks to everyone who contributed to this release, and especially Dmitry Petukhov from @Simplexum for his valuable contributions!
