RPC changes
------------

### Low-level changes

- The `getwalletinfo` RPC method now returns an `hdseedid` value, which is always the same as the incorrectly-named `hdmasterkeyid` value. `hdmasterkeyid` will be removed in V0.18.

Other API changes
-----------------

- The `inactivehdmaster` property in the `dumpwallet` output has been corrected to `inactivehdseed`
