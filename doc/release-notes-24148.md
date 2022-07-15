Notable changes
===============

Wallet
------

- The `wsh()` output descriptor was extended with Miniscript support. You can import Miniscript
  descriptors for P2WSH in a watchonly wallet to track coins, but you can't spend from them using
  the Bitcoin Core wallet yet.
  You can find more about Miniscript on the [reference website](https://bitcoin.sipa.be/miniscript/).


Low-level changes
=================

RPC
---

- The `deriveaddresses`, `getdescriptorinfo`, `importdescriptors` and `scantxoutset` commands now
  accept Miniscript expression within a `wsh()` descriptor.

- The `getaddressinfo`, `decodescript`, `listdescriptors` and `listunspent` commands may now output
  a Miniscript descriptor inside a `wsh()` where a `wsh(raw())` descriptor was previously returned.
