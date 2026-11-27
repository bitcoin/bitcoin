New RPCs
--------

- A new `analyzedescriptor` wallet RPC tells which of a descriptor's keys the
  wallet holds (`mine`, `not_mine`, `watch_only`, or `unknown` when the wallet
  is locked and a key can't be checked without its private key), whether the
  wallet can sign for it, the first address of each path (e.g. receive and
  change), and whether the descriptor contains private keys. It also reports
  whether the descriptor is a recognized k-of-n multisig setup (`wsh(multi)`,
  `wsh(sortedmulti)`, or `tr(K,multi_a)`/`tr(K,sortedmulti_a)` whose key path
  `K` is the BIP 341 NUMS point or a `musig()` of at least k distinct
  cosigners) and, if not, why. It can be used to check a multisig descriptor
  received from a coordinator before importing it. (#36885)

Updated RPCs
------------

- `getdescriptorinfo` has a new `verbose` argument. When set to true, the
  result also includes the descriptor's key occurrences (`keys`), how they are
  combined (`policy`) and the multisig setup analysis (`analysis`), which needs
  no wallet. (#36885)
