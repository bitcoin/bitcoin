Wallet
------
- A new RPC `addhdkey` is added which allows a BIP 32 extended key to be added to the wallet without
  needing to import it as part of a separate descriptor. This key will not be used to produce any
  output scripts unless it is explicitly imported as part of a separate descriptor independent of
  the `addhdkey` RPC.
