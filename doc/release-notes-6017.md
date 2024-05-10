RPC changes
-----------
- A new `sethdseed` RPC allows users to initialize their blank HD wallets with an HD seed. **A new backup must be made when a new HD seed is set.** This command cannot replace an existing HD seed if one is already set. `sethdseed` uses WIF private key as a seed. If you have a mnemonic, use the `upgradetohd` RPC.
