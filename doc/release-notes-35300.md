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
  `preciousblock` RPC. A new block returns `result=true` if it is connected to
  the active chain. Duplicate blocks return `result=false` with
  `reason="duplicate"`, even if making the block precious changes the active
  tip. Validation failures still return their rejection reason.

  The methods accepting `precious` use new ordinals: `Mining.submitBlock @10`
  and `BlockTemplate.submitSolution @11`. Clients must regenerate IPC
  bindings from the updated `mining.capnp` schema. The previous methods
  (`Mining.submitBlock @7` and `BlockTemplate.submitSolution @10`) now
  return errors directing clients to update. (#35300)
