Updated RPCs
--------

- `bls generate` and `bls fromsecret`: The new `scheme` field will be returned indicating which scheme was used to serialise the public key. Valid returned values are `legacy` and`basic`.
- `bls generate` and `bls fromsecret`: Both RPCs accept an incoming optional boolean argument `legacy` that enforces the use of legacy BLS scheme for the serialisation of the reply even if v19 is active.
