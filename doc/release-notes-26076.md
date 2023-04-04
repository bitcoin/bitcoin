RPC
---

- The `listdescriptors` RPC now shows `h` rather than apostrophe (`'`) to indicate
  hardened derivation. This does not apply when using the `private` parameter, which
  matches the marker used when descriptor was generated or imported. Newly created
  wallets use `h`. This change makes it easier to handle descriptor strings manually.
  E.g. an RPC call that takes an array of descriptors can now use '["desc": ".../0h/..."]'.
  Either `h` or `'` can still be used when providing a descriptor to e.g. `getdescriptorinfo`
  or `importdescriptors`. Note that this choice changes the descriptor checksum.
  For legacy wallets the `hdkeypath` field in `getaddressinfo` is unchanged,
  nor is the serialization format of wallet dumps. (#26076)
