Porting Bitcoin 0.9.3 to Darkcoin
=================================

Staging tree for Darkcoin-0.9.3.


MUST-HAVE:
----------

- Strings in config path (.bitcoin --> .darkcoin)
- Ports for communication and RPC (8333 --> 9999; 8332 --> 9998)
- Version numbers, protocol version, wallet version (compatible with DRK network)
- Adress versions (Public keys, Multisig keys)
- Adjust algorithm (SHA256 --> X11)
- Adjust difficulty (KGW, DGW based on blockheight)
- Add masternode payment checks a.k.a. enforcement (based on blockheight)


OPTIONAL:
---------

- All the above for Testnet
- Darksend, Instant Transactions, Atomic Transfers, etc. pp.
