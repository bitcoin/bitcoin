(note: this is a temporary file, to be added-to by anybody, and moved to
release-notes at release time)

Notable changes
===============

Example item
----------------


bitcoin-cli: arguments privacy
--------------------------------

The RPC command line client gained a new argument, `-stdin`
to read extra arguments from standard input, one per line until EOF/Ctrl-D.
For example:

    $ echo -e "mysecretcode\n120" | src/bitcoin-cli -stdin walletpassphrase

It is recommended to use this for sensitive information such as wallet
passphrases, as command-line arguments can usually be read from the process
table by any user on the system.

0.13.0 Change log
=================

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors and string updates. For convenience in locating
the code changes and accompanying discussion, both the pull request and
git merge commit are mentioned.

### RPC and REST

Asm script outputs now contain OP_CHECKLOCKTIMEVERIFY in place of OP_NOP2
-------------------------------------------------------------------------

OP_NOP2 has been renamed to OP_CHECKLOCKTIMEVERIFY by [BIP 
65](https://github.com/bitcoin/bips/blob/master/bip-0065.mediawiki)

The following outputs are affected by this change:
- RPC `getrawtransaction` (in verbose mode)
- RPC `decoderawtransaction`
- RPC `decodescript`
- REST `/rest/tx/` (JSON format)
- REST `/rest/block/` (JSON format when including extended tx details)
- `bitcoin-tx -json`

### Configuration and command-line options

### Block and transaction handling

### P2P protocol and network code

### Validation

### Build system

### Wallet

### GUI

### Tests

### Miscellaneous

