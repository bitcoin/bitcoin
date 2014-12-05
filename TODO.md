Porting Bitcoin 0.9.3 to Darkcoin
=================================

Staging tree for Darkcoin-0.11.0.


DONE:
-----

- Strings in config, path and pid (~/.darkcoin)
- Ports for communication and RPC (port=9999; rpcport=9998)
- Version numbers, protocol version, wallet version (compatible with DRK network)
- Added darkcoin seednodes
- Updated address versions (Public keys, Multisig keys)
- Changed genesisblockhash and timestamp
- Reviewed and updated checkpoints
- Adjusted algorithm (X11)
- Updated subsidity function (Block value)
- Adjusted wallet keypool size to 1000 and added loading indicator on fresh wallet load
- Adjusted difficulty and blockvalue (KGW, DGW based on blockheight)
- Defined regression test genesis block
- Updated wallet layout and branding
- Reset testnet (v4) with new genesis and address version (start with x)


MANDATORY:
----------

- Add masternode payment checks a.k.a. enforcement (based on blockheight)
- Fix mining protocol to include correct pow and masternodes


OPTIONAL:
---------

- Define BIP0032 addresses EXT_PUBLIC_KEY and EXT_SECRET_KEY for Darkcoin
- Include Evan's public key for msg signing
- Darksend, Instant Transactions, Atomic Transfers, etc. pp.
- Include centralized checkpoint syncing (peercoin style)
- Remove Bitcoin dead weight (SHA256, hardcoded keys, nodes)
- Update strings
- Write tests
