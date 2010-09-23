Bitcoin TEST network source code
================================

Branches here:

* svn : up-to-date mirror of the 'production' Bitcoin (from http://sourceforge.net/projects/bitcoin/).
* listtransactions: Implements new JSON-RPC command "listtransactions" (from jgarzik)
* refundtransaction : Implements new JSON-RPC command "refundtransaction"

These branches operate on the TEST network:

* svnTEST : just like 'svn' (no additional features), but patched to operate on the TEST network.
* master : runs on the TEST-network with any of what I think are "production-ready" features.

Important changes on the master branch are:

* Listen port is 18333 (instead of 8333).  Uses 18332 for JSON-RPC (instead of 8332).
* Uses irc.lfnet.org channel #bitcoinTEST  for bootstrapping.
* Never tries to connect to "well known" nodes for bootstrapping.
* Uses bitcoinTEST for the default data directory (e.g. ~/.bitcoinTEST on Linux, %APPDATA%/BitcoinTEST on Windows, etc)
* Uses a different ADDRESSVERSION for Bitcoin Addresses (0xff instead of 0x0), so you can't screw up copying and pasting Bitcoin addresses (if you try to use a TEST address on the production system it will tell you it's invalid; you can, however, lose TESTcoins by trying to send them to a valid production address)
* Has a brand-new block chain (with a different genesis block)
* Initial difficulty is 4 times easier than production Bitcoin
* protocol message header bytes are { 0xfa, 0xbf, 0xb5, 0xda } instead of { 0xf9, 0xbe, 0xb4, 0xd9 }

Code is hosted at github: http://github.com/gavinandresen/bitcoin-git

