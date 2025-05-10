- `-datacarriersize` default is now set to effectively uncapped limit, and the argument
  is now used to limit the aggregate size of scriptPubKeys in a transaction's
  OP_RETURN outputs. This allows multiple datacarrier outputs within a transaction while respecting
  the set limit. Both `-datacarrier` and `-datacarriersize` are marked as deprecated. (#32406)
