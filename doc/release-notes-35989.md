Wallet
------

- `importdescriptors` now rejects a `range` ending at 2147483647 (2^31-1), as
  the wallet cannot store it. The same applies to imports without a `range`
  when `-keypool` is above 2147483647, since `-keypool` is then used as the end
  of the range. Previously such imports could crash the node.
