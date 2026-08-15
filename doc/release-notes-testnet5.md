P2P and network changes
-----------------------

- A new test network `testnet5` has been added, as specified in BIP95, selectable
  with `-testnet5` (or `-chain=testnet5`). Unlike testnet3 and testnet4 it does not
  allow minimum-difficulty blocks, its minimum difficulty is raised to ~1,000,000,
  and the BIP54 consensus cleanup rules are enforced from block 1. (#XXXXX)
