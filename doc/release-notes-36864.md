Updated RPCs
------------

- The `getaddressinfo` RPC no longer returns the `iswatchonly` field unless
  the configuration option `-deprecatedrpc=iswatchonly` is used. The field has
  always been `false` since v30 and will be fully removed in the next major
  release. (#36864)
