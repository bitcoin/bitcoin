Wallet
======

- The `importdescriptor` RPC can now infer the private keys when importing a descriptor that contains an xpub for which we have the HD seed (and it is active). This requires providing origin info. Unchanged behavior is that descriptors containing multiple xpubs can only be imported into a non watch-only wallet if we can infer the private keys for all of them. (#24098)
