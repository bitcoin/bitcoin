# Notable changes

## HD Wallets Enabled by Default

In this release, we are taking a significant step towards enhancing the Dash wallet's usability by enabling Hierarchical Deterministic (HD) wallets by default. This change aligns the behavior of `dashd` and `dash-qt` with the previously optional `-usehd=1` flag, making HD wallets the standard for all users.

While HD wallets are now enabled by default to improve user experience, Dash Core still allows for the creation of non-HD wallets by using the `-usehd=0` flag. However, users should be aware that this option is intended for legacy support and will be removed in future releases. Importantly, even with an HD wallet, users can still import non-HD private keys, ensuring flexibility in managing their funds.
