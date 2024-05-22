- A new RPC, `submitpackage`, has been added. It can be used to submit a list of raw hex
  transactions to the mempool to be evaluated as a package using consensus and mempool policy rules.
These policies include package CPFP, allowing a child with high fees to bump a parent below the
mempool minimum feerate (but not minimum relay feerate).

    - Warning: successful submission does not mean the transactions will propagate throughout the
      network, as package relay is not supported.

    - Not all features are available. The package is limited to a child with all of its
      unconfirmed parents, and no parent may spend the output of another parent.  Also, package
      RBF is not supported. Refer to doc/policy/packages.md for more details on package policies
      and limitations.

    - This RPC is experimental. Its interface may change.
