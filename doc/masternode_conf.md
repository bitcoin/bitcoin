Multi masternode config
=======================

The multi masternode config allows to control multiple masternodes from a single wallet. The wallet needs to have a valid collaral output of 1000 coins for each masternode. To use this, place a file named masternode.conf in the data directory of your install:
 * Windows: %APPDATA%\Dash\
 * Mac OS: ~/Library/Application Support/Dash/
 * Unix/Linux: ~/.dash/

The new masternode.conf format consists of a space seperated text file. Each line consisting of an alias, IP address followed by port, masternode private key, collateral output transaction id, collateral output index, donation address and donation percentage (the latter two are optional and should be in format "address:percentage").

Example:
```
mn1 127.0.0.2:19999 93HaYBVUCYjEMeeH1Y4sBGLALQZE1Yc1K64xiqgX37tGBDQL8Xg 2bcd3c84c84f87eaa86e4e56834c92927a07f9e18718810b92e0d0324456a67c 0
mn2 127.0.0.3:19999 93WaAb3htPJEV8E9aQcN23Jt97bPex7YvWfgMDTUdWJvzmrMqey aa9f1034d973377a5e733272c3d0eced1de22555ad45d6b24abadff8087948d4 0 7gnwGHt17heGpG9Crfeh4KGpYNFugPhJdh:33
mn3 127.0.0.4:19999 92Da1aYg6sbenP6uwskJgEY2XWB5LwJ7bXRqc3UPeShtHWJDjDv db478e78e3aefaa8c12d12ddd0aeace48c3b451a8b41c570d0ee375e2a02dfd9 1 7gnwGHt17heGpG9Crfeh4KGpYNFugPhJdh
```

In the example above:
* the collateral for mn1 consists of transaction http://test.explorer.dash.fr/tx/2bcd3c84c84f87eaa86e4e56834c92927a07f9e18718810b92e0d0324456a67c, output index 0 has amount 1000
* masternode 2 will donate 33% of its income
* masternode 3 will donate 100% of its income


The following new RPC commands are supported:
* list-conf: shows the parsed masternode.conf
* start-alias \<alias\>
* stop-alias \<alias\>
* start-many
* stop-many
* outputs: list available collateral output transaction ids and corresponding collateral output indexes

When using the multi masternode setup, it is advised to run the wallet with 'masternode=0' as it is not needed anymore.
