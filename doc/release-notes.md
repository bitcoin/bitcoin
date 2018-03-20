Syscoin Core version 0.12.1 is now available from:

  <https://www.syscoin.org/downloads/>




Older releases
--------------

Syscoin was previously known as Darkcoin.

Darkcoin tree 0.8.x was a fork of Litecoin tree 0.8, original name was XCoin
which was first released on Jan/18/2014.

### Downgrade to a version < 0.12.0

Because release 0.12.0 and later will obfuscate the chainstate on every
fresh sync or reindex, the chainstate is not backwards-compatible with
pre-0.12 versions of Syscoin Core or other software.

If you want to downgrade after you have done a reindex with 0.12.0 or later,
you will need to reindex when you first start Syscoin Core version 0.11 or
earlier.

Notable changes
===============

Example item
---------------------------------------

Example text.

0.12.1 Change log
=================

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors and string updates. For convenience in locating
the code changes and accompanying discussion, both the pull request and
git merge commit are mentioned.

### RPC and REST

Asm script outputs replacements for OP_NOP2 and OP_NOP3
-------------------------------------------------------

OP_NOP2 has been renamed to OP_CHECKLOCKTIMEVERIFY by [BIP 
65](https://github.com/syscoin/bips/blob/master/bip-0065.mediawiki)

OP_NOP3 has been renamed to OP_CHECKSEQUENCEVERIFY by [BIP 
112](https://github.com/syscoin/bips/blob/master/bip-0112.mediawiki)

The following outputs are affected by this change:
- RPC `getrawtransaction` (in verbose mode)
- RPC `decoderawtransaction`
- RPC `decodescript`
- REST `/rest/tx/` (JSON format)
- REST `/rest/block/` (JSON format when including extended tx details)
- `syscoin-tx -json`

### ZMQ

Each ZMQ notification now contains an up-counting sequence number that allows
listeners to detect lost notifications.
The sequence number is always the last element in a multi-part ZMQ notification and
therefore backward compatible.
Each message type has its own counter.
(https://github.com/syscoin/syscoin/pull/7762)

### Configuration and command-line options

### Block and transaction handling

### P2P protocol and network code

### Validation

### Build system

### Wallet

### GUI

### Tests and QA

### Miscellaneous

Credits
=======

Thanks to everyone who directly contributed to this release:


As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/syscoin/).

Darkcoin tree 0.9.x was the open source implementation of masternodes based on
the 0.8.x tree and was first released on Mar/13/2014.

Darkcoin tree 0.10.x used to be the closed source implementation of Darksend
which was released open source on Sep/25/2014.

Syscoin Core tree 0.11.x was a fork of Syscoin Core tree 0.9, Darkcoin was rebranded
to Syscoin.

Syscoin Core tree 0.12.0.x was a fork of Syscoin Core tree 0.10.

These release are considered obsolete. Old changelogs can be found here:

- [v0.12.0](release-notes/syscoin/release-notes-0.12.0.md) released ???/??/2015
- [v0.11.2](release-notes/syscoin/release-notes-0.11.2.md) released Mar/25/2015
- [v0.11.1](release-notes/syscoin/release-notes-0.11.1.md) released Feb/10/2015
- [v0.11.0](release-notes/syscoin/release-notes-0.11.0.md) released Jan/15/2015
- [v0.10.x](release-notes/syscoin/release-notes-0.10.0.md) released Sep/25/2014
- [v0.9.x](release-notes/syscoin/release-notes-0.9.0.md) released Mar/13/2014

