Wallet
------

- Fixed an assertion failure in `abandontransaction` that could abort the node
  when a wallet descendant of the abandoned transaction was concurrently
  confirmed in a block or in the mempool (e.g. during a chain reorganisation
  or when RBF candidates share inputs). Such descendants are now left untouched
  and the abandonment proceeds without crashing. (#34599)
