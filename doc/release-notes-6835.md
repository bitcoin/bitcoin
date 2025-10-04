Mobile CoinJoin Compatibility
------------

- Fixed an issue where CoinJoin funds mixed in Dash Android wallet were
  invisible when importing the mnemonic into Dash Core. Descriptor Wallets now
  include an additional default descriptor for mobile CoinJoin funds, ensuring
  seamless wallet migration and complete fund visibility across different
  Dash wallet implementations.

- This is a breaking change that increases the default number of descriptors
  from 2 to 3 on mainnet (internal, external, mobile CoinJoin) for newly created
  descriptor wallets only - existing wallets are unaffected.


Updated RPCs
------------

- The `listdescriptors` RPC now includes an optional coinjoin field to identify
  CoinJoin descriptors.

(#6835)
