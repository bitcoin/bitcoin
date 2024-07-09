## Remote Procedure Call (RPC) Changes

### Improved support of composite commands

Dash Core's composite commands such as `quorum list` or `bls generate` now are compatible with a whitelist feature.

For example, whitelist `getblockcount,quorumlist` will let to call commands `getblockcount`, `quorum list`, but not `quorum sign`

Note, that adding simple `quorum` in whitelist will allow to use all kind of `quorum...` commands, such as `quorum`, `quorum list`, `quorum sign`, etc
