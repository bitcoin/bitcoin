(note: this is a temporary file, to be added-to by anybody, and moved to
release-notes at release time)

Notable changes
===============

Example header
----------------------

Example content.

0.12.0 Change log
=================

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors and string updates. For convenience in locating
the code changes and accompanying discussion, both the pull request and
git merge commit are mentioned.

### RPC and REST

### Configuration and command-line options

### Block and transaction handling

### P2P protocol and network code

Double-Spend Relay and Alerts
=============================
VERY IMPORTANT: *It has never been safe, and remains unsafe, to rely*
*on unconfirmed transactions.*

Relay
-----
When an attempt is seen on the network to spend the same unspent funds
more than once, it is no longer ignored.  Instead, it is broadcast, to
serve as an alert.  This broadcast is subject to protections against
denial-of-service attacks.

Wallets and other bitcoin services should alert their users to
double-spends that affect them.  Merchants and other users may have
enough time to withhold goods or services when payment becomes
uncertain, until confirmation.

Bitcoin Core Wallet Alerts
--------------------------
The Bitcoin Core wallet now makes respend attempts visible in several
ways.

If you are online, and a respend affecting one of your wallet
transactions is seen, a notification is immediately issued to the
command registered with `-respendnotify=<cmd>`.  Additionally, if
using the GUI:
 - An alert box is immediately displayed.
 - The affected wallet transaction is highlighted in red until it is
   confirmed (and it may never be confirmed).

A `respendsobserved` array is added to `gettransaction`, `listtransactions`,
and `listsinceblock` RPC results.

Warning
-------
*If you rely on an unconfirmed transaction, these changes do VERY*
*LITTLE to protect you from a malicious double-spend, because:*

 - You may learn about the respend too late to avoid doing whatever
   you were being paid for.
 - Using other relay rules, a double-spender can craft the double- 
   spend to resist broadcast.
 - Miners can choose which conflicting spend to confirm, and some
   miners may not confirm the first acceptable spend they see.


### Validation

### Build system

### Wallet

### GUI

### Tests

### Miscellaneous

