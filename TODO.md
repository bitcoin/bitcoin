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
- BIP0032 addresses xpub and xpriv start with x (unchanged by design)
- Changed Darkcoin units to DRK and added duffs
- Fixed internal walletminer


MANDATORY:
----------

- Check rpcminer (should be working though)


BUGS:
-----

- Daemon and CLI tool can't connect to testnet/regtest instances (wrong port?)
- Daemon and CLI tool can't authenticate via RPC (uh-oh?)
- Qt wallet can't find the config file in testnet mode (wrong path?)


ADDITIONAL:
-----------

- Include trusted public key for message signing
- Masternodes, Enforcement, Darksend, InstantX, Atomic Transfers, ...
- Remove Bitcoin dead weight (SHA256, hardcoded keys, seednodes, ...)
- Update strings
- Write tests
