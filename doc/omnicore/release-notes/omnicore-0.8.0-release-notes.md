Omni Core v0.8.0
================

v0.8.0 is a major release and an upgrade is required.

A consensus affecting issue in an earlier version of Omni Core has been identified, which may cause some transactions to be executed twice. This has been addressed and fixed in this release.

More information about this issue will be announced on https://blog.omni.foundation/.

The first time you start this version, the internal database for Omni Layer transactions is reconstructed, which may consume several hours or even more than a day. Please plan your downtime accordingly.

Please report bugs using the issue tracker on GitHub:

  https://github.com/OmniLayer/omnicore/issues


Table of contents
=================

- [Omni Core v0.8.0](#omni-core-v071)
- [Upgrading and downgrading](#upgrading-and-downgrading)
  - [How to upgrade](#how-to-upgrade)
  - [Downgrading](#downgrading)
  - [Compatibility with Bitcoin Core](#compatibility-with-bitcoin-core)
- [Imported notes](#imported-notes)
  - [Transaction replays](#transaction-replays)
  - [Searching for affected transactions](#searching-for-affected-transactions)
- [Change log](#change-log)
- [Credits](#credits)


Upgrading and downgrading
=========================

How to upgrade
--------------

If you are running Bitcoin Core or an older version of Omni Core, shut it down. Wait until it has completely shut down, then copy the new version of `omnicored`, `omnicore-cli` and `omnicore-qt`. On Microsoft Windows the setup routine can be used to automate these steps.

When upgrading from an older version, the database is reconstructed, which can easily consume several hours.

During the first startup historical Omni transactions are reprocessed and Omni Core will not be usable for several hours up to more than a day. The progress of the initial scan is reported on the console, the GUI and written to the `debug.log`. The scan may be interrupted, but can not be resumed, and then needs to start from the beginning.

Downgrading
-----------

Downgrading to an Omni Core version prior to 0.8.0 is not supported.

Compatibility with Bitcoin Core
-------------------------------

Omni Core is based on Bitcoin Core 0.18.1 and can be used as replacement for Bitcoin Core. Switching between Omni Core and Bitcoin Core may be supported.

However, it is not advised to upgrade or downgrade to versions other than Bitcoin Core 0.18. When switching to Omni Core, it may be necessary to reprocess Omni Layer transactions.


Imported notes
==============

Transaction replays
-------------------

An issue has been discovered that affects all 0.6 and higher versions of Omni Core.

The result of this issue is that some transactions may be executed twice. The effect is that tokens will be credited and debited more than once, leaving some token balances lower or higher than they should be. No new money can be created with this issue.

This problem goes back to an update of Omni Core 0.6 in August 2019 and since then, while not necessarily exclusive, transactions of the following blocks may have been executed twice:

  `619141`, `618465`, `614732`, `599587`, `591848`, `589999`, `578141`

The first startup of the 0.8.0 release will trigger a full reparse of all blocks, after which balances will be restored to their correct state. This will remove additional tokens credited by this error and any transactions which include them. This step can take several hours or more than a day.


Searching for affected transactions
-----------------------------------

To exchanges, wallet operators, and any integrators who use another database on top of Omni Core to track transactions or balances, we can suggest the following options for checking the validity of your transaction history. Each of the following options have different time or technical requirements and produce varying levels of details.

Neither option is required, if you solely use Omni Core to track balances and transaction histories, as after the first start of Omni Core 0.8.0, it's internal state is correct.

### Option 1:

* Time Commitment: Least
* Technical Commitment: Least
* Details: Least
* Reliability: fair
* Steps:
  * Before shutting down your existing node:
  * Pause all incoming/outgoing transactions for your hot/cold wallets
  * Run `omnicore-cli omni_getallbalancesforaddress <hot/cold wallet address>`
  * Record the balances for the addresses
  * Stop your client, upgrade to 0.8 according to the upgrade instructions of this release, relaunch and let synchronize and reparse
  * After synchronization and reparse is complete rerun the `omni_getallbalancesforaddress` command and compare the balances reported to the previously saved balances. (Optionally, during the parse you can also compare the saved balances to balances reported on omniexplorer.info)
  * If balances match before and after you are most likely not affected (see footnote below)
  * If you notice a discrepancy it is advised you proceed with verification via Option 2 below

Footnote: If your hot wallet utilizes the send all transaction type for sweeping this method may produce a false favorable result. You should proceed with option 2 below for optimal verifications.

### Option 2:

* Time Commitment: Most
* Technical Commitment: Moderate
* Details: Full
* Reliability: Most
* Steps:
  * Stop your client, upgrade to 0.8 according to the upgrade instructions of this release, relaunch and let synchronize and reparse
  * After synchronization and reparse is complete:
  * Compile a list of all deposit transaction hashes, amounts you have processed since before you upgraded to 0.6 or later (~roughly around Aug 2019).
  * Using `omnicore-cli omni_gettransaction <txhash>` reprocess the list of transaction hashes checking:
  * The transaction is still valid `response['valid']`
  * The transaction amount matches the amount you recorded/processed in your database `response['amount']`
  * Note: If your service supports deposits from 'send_all' transactions you will need to adjust the amount field to check for all `amounts` in the `response['subsends']` array of the transaction
  * For any transaction that does not match the previous two conditions, flag it for manual followup and check the discrepancy between the updated client output and the details of what you processed in the database.


Credits
=======

Thanks to everyone who contributed to this release, especially to Peter Bushnell.
