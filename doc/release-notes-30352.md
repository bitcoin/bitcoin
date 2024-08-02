P2P and network changes
-----------------------

- Pay To Anchor(P2A) is a new standard witness output type for spending,
  a newly recognised output template. This allows for key-less anchor
  outputs, with compact spending conditions for additional efficiencies on
  top of an equivalent `sh(OP_TRUE)` output, in addition to the txid stability
  of the spending transaction.
  N.B. propagation of this output spending on the network will be limited
  until a sufficient number of nodes on the network adopt this upgrade.
