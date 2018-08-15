# Internal c++ interfaces

The following interfaces are defined here:

* [`Node`](node.h) — used by GUI to start & stop bitcoin node. Added in [#10244](https://github.com/bitcoin/bitcoin/pull/10244).

* [`Wallet`](wallet.h) — used by GUI to access wallets. Added in [#10244](https://github.com/bitcoin/bitcoin/pull/10244).

* [`Handler`](handler.h) — returned by `handleEvent` methods on interfaces above and used to manage lifetimes of event handlers.

The interfaces above define boundaries between major components of bitcoin code (node, wallet, and gui), making it possible for them to run in different processes, and be tested, developed, and understood independently. These interfaces are not currently designed to be stable or to be used externally.
