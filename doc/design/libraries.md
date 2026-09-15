# Libraries

| Name                     | Description |
|--------------------------|-------------|
| *libbitcoin_cli*         | RPC client functionality used by *bitcoin-cli* executable |
| *libbitcoin_common*      | Home for common functionality shared by different executables and libraries. Similar to *libbitcoin_util*, but higher-level (see [Dependencies](#dependencies)). |
| *libbitcoin_consensus*   | Consensus functionality used by *libbitcoin_node* and *libbitcoin_wallet*. |
| *libbitcoin_crypto*      | Hardware-optimized functions for data encryption, hashing, message authentication, and key derivation. |
| *libbitcoin_kernel*      | Consensus engine and support library used for validation by *libbitcoin_node*. |
| *libbitcoinqt*           | GUI functionality used by *bitcoin-qt* and *bitcoin-gui* executables. |
| *libbitcoin_ipc*         | IPC functionality used by *bitcoin-node*, *bitcoin-wallet*, *bitcoin-gui* executables to communicate when [`-DENABLE_IPC=ON`](multiprocess.md) is used. |
| *libbitcoin_node*        | P2P and RPC server functionality used by *bitcoind* and *bitcoin-qt* executables. |
| *libbitcoin_util*        | Home for common functionality shared by different executables and libraries. Similar to *libbitcoin_common*, but lower-level (see [Dependencies](#dependencies)). |
| *libbitcoin_wallet*      | Wallet functionality used by *bitcoind* and *bitcoin-wallet* executables. |
| *libbitcoin_wallet_tool* | Lower-level wallet functionality used by *bitcoin-wallet* executable. |
| *libbitcoin_zmq*         | [ZeroMQ](../zmq.md) functionality used by *bitcoind* and *bitcoin-qt* executables. |

## Conventions

- Most libraries are internal libraries and have APIs which are completely unstable! There are few or no restrictions on backwards compatibility or rules about external dependencies. An exception is *libbitcoin_kernel*, which, at some future point, will have a documented external interface.

- Generally each library should have a corresponding source directory and namespace. Source code organization is a work in progress, so it is true that some namespaces are applied inconsistently, and if you look at [`add_library(bitcoin_* ...)`](../../src/CMakeLists.txt) lists you can see that many libraries pull in files from outside their source directory. But when working with libraries, it is good to follow a consistent pattern like:

  - *libbitcoin_node* code lives in `src/node/` in the `node::` namespace
  - *libbitcoin_wallet* code lives in `src/wallet/` in the `wallet::` namespace
  - *libbitcoin_ipc* code lives in `src/ipc/` in the `ipc::` namespace
  - *libbitcoin_util* code lives in `src/util/` in the `util::` namespace
  - *libbitcoin_consensus* code lives in `src/consensus/` in the `Consensus::` namespace

## Dependencies

- Libraries should minimize what other libraries they depend on, and only reference symbols following the arrows shown in the dependency graph below:

<table><tr><td>

```mermaid

%%{ init : { "flowchart" : { "curve" : "basis" }}}%%

graph TD;

bitcoin-cli[bitcoin-cli]-->libbitcoin_cli;

bitcoind[bitcoind]-->libbitcoin_node;
bitcoind[bitcoind]-->libbitcoin_wallet;

bitcoin-qt[bitcoin-qt]-->libbitcoin_node;
bitcoin-qt[bitcoin-qt]-->libbitcoinqt;
bitcoin-qt[bitcoin-qt]-->libbitcoin_wallet;

bitcoin-wallet[bitcoin-wallet]-->libbitcoin_wallet;
bitcoin-wallet[bitcoin-wallet]-->libbitcoin_wallet_tool;

libbitcoin_cli-->libbitcoin_util;
libbitcoin_cli-->libbitcoin_common;

libbitcoin_consensus-->libbitcoin_crypto;

libbitcoin_common-->libbitcoin_consensus;
libbitcoin_common-->libbitcoin_crypto;
libbitcoin_common-->libbitcoin_util;

libbitcoin_kernel-->libbitcoin_consensus;
libbitcoin_kernel-->libbitcoin_crypto;
libbitcoin_kernel-->libbitcoin_util;

libbitcoin_node-->libbitcoin_consensus;
libbitcoin_node-->libbitcoin_crypto;
libbitcoin_node-->libbitcoin_kernel;
libbitcoin_node-->libbitcoin_common;
libbitcoin_node-->libbitcoin_util;

libbitcoinqt-->libbitcoin_common;
libbitcoinqt-->libbitcoin_util;

libbitcoin_util-->libbitcoin_crypto;

libbitcoin_wallet-->libbitcoin_common;
libbitcoin_wallet-->libbitcoin_crypto;
libbitcoin_wallet-->libbitcoin_util;

libbitcoin_wallet_tool-->libbitcoin_wallet;
libbitcoin_wallet_tool-->libbitcoin_util;

classDef bold stroke-width:2px, font-weight:bold, font-size: smaller;
class bitcoin-qt,bitcoind,bitcoin-cli,bitcoin-wallet bold
```
</td></tr><tr><td>

**Dependency graph**. Arrows show linker symbol dependencies. *Crypto* lib depends on nothing. *Util* lib is depended on by everything. *Kernel* lib depends only on consensus, crypto, and util.

</td></tr></table>

- The graph shows what _linker symbols_ (functions and variables) from each library other libraries can call and reference directly, but it is not a call graph. For example, there is no arrow connecting *libbitcoin_wallet* and *libbitcoin_node* libraries, because these libraries are intended to be modular and not depend on each other's internal implementation details. But wallet code is still able to call node code indirectly through the `interfaces::Chain` abstract class in [`interfaces/chain.h`](../../src/interfaces/chain.h) and node code calls wallet code through the `interfaces::ChainClient` and `interfaces::Chain::Notifications` abstract classes in the same file. In general, defining abstract classes in [`src/interfaces/`](../../src/interfaces/) can be a convenient way of avoiding unwanted direct dependencies or circular dependencies between libraries.

- *libbitcoin_crypto* should be a standalone dependency that any library can depend on, and it should not depend on any other libraries itself.

- *libbitcoin_consensus* should only depend on *libbitcoin_crypto*, and all other libraries besides *libbitcoin_crypto* should be allowed to depend on it.

- *libbitcoin_util* should be a standalone dependency that any library can depend on, and it should not depend on other libraries except *libbitcoin_crypto*. It provides basic utilities that fill in gaps in the C++ standard library and provide lightweight abstractions over platform-specific features. Since the util library is distributed with the kernel and is usable by kernel applications, it shouldn't contain functions that external code shouldn't call, like higher level code targeted at the node or wallet. (*libbitcoin_common* is a better place for higher level code, or code that is meant to be used by internal applications only.)

- *libbitcoin_common* is a home for miscellaneous shared code used by different Bitcoin Core applications. It should not depend on anything other than *libbitcoin_util*, *libbitcoin_consensus*, and *libbitcoin_crypto*.

- *libbitcoin_kernel* should only depend on *libbitcoin_util*, *libbitcoin_consensus*, and *libbitcoin_crypto*.

- The only thing that should depend on *libbitcoin_kernel* internally should be *libbitcoin_node*. GUI and wallet libraries *libbitcoinqt* and *libbitcoin_wallet* in particular should not depend on *libbitcoin_kernel* and the unneeded functionality it would pull in, like block validation. To the extent that GUI and wallet code need scripting and signing functionality, they should be able to get it from *libbitcoin_consensus*, *libbitcoin_common*, *libbitcoin_crypto*, and *libbitcoin_util*, instead of *libbitcoin_kernel*.

- GUI, node, and wallet code internal implementations should all be independent of each other, and the *libbitcoinqt*, *libbitcoin_node*, *libbitcoin_wallet* libraries should never reference each other's symbols. They should only call each other through [`src/interfaces/`](../../src/interfaces/) abstract interfaces.

## Work in progress

- Validation code is moving from *libbitcoin_node* to *libbitcoin_kernel* as part of [The libbitcoinkernel Project #27587](https://github.com/bitcoin/bitcoin/issues/27587)
