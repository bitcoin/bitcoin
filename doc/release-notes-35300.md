IPC Interface
-------------

- The IPC mining interface now provides
  `Mining.submitBlock(context, block, precious=false)`, which accepts a fully
  assembled serialized block and returns `reason`, `debug`, and a boolean
  `result`. With the default `precious=false`, a newly accepted and connected
  block returns `result=true`. Duplicate blocks return `result=false` with
  `reason="duplicate"`, valid blocks that are accepted but not connected return
  `result=false` with `reason="inconclusive"`, and invalid blocks return their
  BIP22 rejection reason and debug details. Unlike the `submitblock` RPC, this
  method does not add a missing coinbase witness reserved value, so clients
  must provide a complete block. Clients must regenerate bindings from the
  updated `mining.capnp` schema to use this method. (#34644, #35300)

- `Mining.submitBlock` and `BlockTemplate.submitSolution` now accept an optional
  `precious` argument, which defaults to `false`. When `true`, the submitted
  block is preferred over competing blocks with the same work, similar to the
  `preciousblock` RPC. In this mode, a new or already-known block returns
  `result=true` if it is connected to the active chain. (#35300)
