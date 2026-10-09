Updated RPCs
------------

- The `encryptwallet` RPC for encrypting an existing wallet that was created as
an unencrypted wallet has been deprecated and will be removed in a future major
release.
    - Support for creating encrypted wallets is not going away or being changed,
    it is only encrypting already unencrypted wallets that has footguns for
    users and is being deprecated.
    - In order to continue using `encryptwallet`, `bitcoind` must be started
    with the `-deprecatedrpc=encryptwallet` flag set.
