Low-level RPC changes
----------------------

- Soft fork reporting in the `getblockchaininfo` return object has been
  updated. For full details, see the RPC help text. In summary:
  - The `bip9_softforks` sub-object is no longer returned
  - The `softforks` sub-object now returns an object keyed by soft fork name,
    rather than an array
  - Each softfork object in the `softforks` object contains a `type` value which
    is either `buried` (for soft fork deployments where the activation height is
    hard-coded into the client implementation), or `bip9` (for soft fork deployments
    where activation is controlled by BIP 9 signaling).

- `getblocktemplate` no longer returns a `rules` array containing `CSV`
  and `segwit` (the BIP 9 deployments that are currently in active state).
