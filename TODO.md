Porting Bitcoin 0.9.3 to Darkcoin
=================================

Staging tree for Darkcoin-0.9.3.


DONE:
-----

- Strings in config, path and pid (~/.bitcoin --> ~/.darkcoin)
- Ports for communication and RPC (8333 --> 9999; 8332 --> 9998)
- Version numbers, protocol version, wallet version (compatible with DRK network)
- Added darkcoin seednodes
- Adress versions (Public keys, Multisig keys)
- Changed genesisblockhash and timestamp
- Reviewed and updated checkpoints


MUST-HAVE:
----------

- Adjust algorithm (SHA256 --> X11)
- Adjust difficulty/subsidity (KGW, DGW based on blockheight)
- Add masternode payment checks a.k.a. enforcement (based on blockheight)
- Remove bitcoin dead weight (SHA256, hardcoded keys, nodes)
- Adjust wallet keypool size and add loading indicator on fresh wallet


ADD-ON:
-------

- All the above for Testnet (including complete testnet reset)
- Update strings and wallet layout/branding
- Include Evan's public key for msg signing
- Darksend, Instant Transactions, Atomic Transfers, etc. pp.
- Figure out what regression tests are and fix them
- Define BIP38 address prefixes EXT_PUBLIC_KEY and EXT_SECRET_KEY for Darkcoin
- Include centralized checkpoint syncing (peercoin style)
