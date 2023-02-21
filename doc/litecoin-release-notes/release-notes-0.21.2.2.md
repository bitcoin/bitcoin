Litecoin Core version 0.21.2.2 is now available from:

 <https://download.litecoin.org/litecoin-0.21.2.2/>.

This is a new patch version release that includes important security updates.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/litecoin-project/litecoin/issues>

Notable changes
===============

Important Security Updates
--------------------------

This release contains fixes that harden node and network security. These fixes are important for every node operator and wallet user.

- Limit and tightly manage memory usage in events of high network traffic or when connected to extremely slow peers.
This protects nodes on lower end hardware to not run out of memory in the face of increased network activity.

RPC API Changes
---------------

* Added `addconnection` for use by functional tests
* `getpeerinfo` provides 2 new fields per peer, `addr_processed` and `addr_rate_limited`, that track `addr` message processing


Credits
=======

Thanks to everyone who directly contributed to this release:

- [The Bitcoin Core Developers](https://github.com/bitcoin/bitcoin/tree/master/doc/release-notes)
- David Burkett
- Jon Atack
- Pieter Wuille