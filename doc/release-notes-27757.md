Wallet
------

- The `deprecatedrpc=walletwarningfield` configuration option has been removed.
  The `createwallet`, `loadwallet`, `restorewallet` and `unloadwallet` RPCs no
  longer return the "warning" string field. The same information is provided
  through the "warnings" field added in v25.0, which returns a JSON array of
  strings. The "warning" string field was deprecated also in v25.0. (#27757)
