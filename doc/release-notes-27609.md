- A new RPC, `submitpackage`, has been added. It can be used to submit a list of raw hex
  transactions to the mempool to be evaluated as a package using consensus and mempool policy rules,
including package CPFP (allowing a child to bump a parent below the mempool minimum feerate).
Warning: successful submission does not mean the transactions will propagate throughout the network,
as package relay is not used.

    - Not all features are available. For example, RBF is not supported and the package is limited
      to a child with its unconfirmed parents. Refer to doc/policy/packages.md for more details on
      package policies and limitations.

    - This RPC is experimental. Its interface may change.
