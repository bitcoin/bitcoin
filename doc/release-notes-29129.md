Low-level changes
=================

RPC
---

- RPC `createwallet` now accepts an `hd_account` parameter. It is a
  BIP44 account that will be used to get the descriptors from the
  external signer. (#29129)
