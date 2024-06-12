P2P and network changes
-----------------------

- Limited package RBF is now enabled, where the proposed conflicting package would result in
  a connected component, aka cluster, of size 2 in the mempool. All clusters being conflicted
  against must be of size 2 or lower.
