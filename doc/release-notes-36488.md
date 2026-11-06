Updated RPCs
------------

The deprecated `confTarget` option of the `bumpfee` and `psbtbumpfee` RPCs is
now available only when the RPC server is started with
`-deprecatedrpc=confTarget`.
Use `conf_target` instead. The `confTarget` option will be removed in the next
release.
