## Wallet Tool Enhancements

This release introduces several improvements and new features to the `dash-wallet` tool, making it more versatile and user-friendly for managing Dash wallets.

### Wallet Version Bump

Wallets created with the `dash-wallet` tool will now utilize the `FEATURE_LATEST` version of wallet which is the HD (Hierarchical Deterministic) wallets with HD chain inside.

### New functionality
- new command line argument `-descriptors` to enable _experimental_ support of Descriptor wallets. It lets users to create descriptor wallets directly from the command line. This change aims to align the command-line interface with the `createwallet` RPC, promoting the use of descriptor wallets which offer a more robust and flexible way to manage wallet addresses and keys.
- new command line argument `-usehd` which let to create non-Hierarchical Deterministic (non-HD) wallets with the `wallettool` for compatibility reasons since default version is bumped to HD version
- new commands `dump` and `createfromdump` have been added, enhancing the wallet's storage migration capabilities. The `dump` command allows for exporting every key-value pair from the wallet as comma-separated hex values, facilitating a storage agnostic dump. Meanwhile, the `createfromdump` command enables the creation of a new wallet file using the records specified in a dump file. These commands are similar to BDB's `db_dump` and `db_load` tools and are crucial for manual wallet file construction for testing or migration purposes.
