Updated RPCs
------------

- The `getpeerinfo` RPC no longer returns the `startingheight` field unless
  the configuration option `-deprecatedrpc=startingheight` is used. The
  `startingheight` field will be fully removed in the next major release.
  (#34197)
