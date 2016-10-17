Win32, Linux, MacOSX and source releases for bitcoin v0.3.23 have been uploaded to
https://sourceforge.net/projects/bitcoin/files/Bitcoin/bitcoin-0.3.23/

This is another quick bugfix release, trying to deal with the influx of new bitcoin users.

Main items of note:

* P2P connect-to-node logic changed to reduce timeout a bit.  The network saw a huge influx of new users, who do not permit incoming connections.  This change is a short-term hack, to more quickly hunt for useful P2P connections.  Better "leaf node" logic is in the works, but this should let us limp along until then.  One may use -upnp to properly forward ports, and help the network.
* Transaction fee reduced to 0.0005 for new transactions
* Client will relay transactions with fees as low as 0.0001 BTC
