RPC
---

- The `listdescriptors`, `decodepsbt` and similar RPC methods now show `h` rather than apostrophe (`'`) to indicate
  hardened derivation. This does not apply when using the `private` parameter, which
  matches the marker used when descriptor was generated or imported. Newly created
  wallets use `h`. This change makes it easier to handle descriptor strings manually.
  E.g. the `importdescriptors` RPC call is easiest to use `h` as the marker: `'["desc": ".../0h/..."]'`.
  With this change `listdescriptors` will use `h`, so you can copy-paste the result,
  without having to add escape characters or switch `'` to 'h' manually.
  Note that this changes the descriptor checksum.
  For legacy wallets the `hdkeypath` field in `getaddressinfo` is unchanged,
  nor is the serialization format of wallet dumps. (#26076)
