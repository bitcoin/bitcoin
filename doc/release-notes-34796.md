Updated RPCs
------------

- The `-deprecatedrpc=startingheight` configuration option has been removed.
  The `getpeerinfo` RPC no longer returns the `startingheight` field, which
  was previously deprecated in v31.0. (#34796)
