## RPC changes

- `getblockfrompeer`, introduced in v21.0.0, named argument `block_hash` has been
  renamed `blockhash` to be aligned with the rest of the codebase. This is a breaking
  change if using named parameters. (#6149)
