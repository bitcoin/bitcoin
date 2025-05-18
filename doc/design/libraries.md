# Libraries

| Name                     | Description |
|--------------------------|-------------|
| *libtortoisecoin_cli*         | RPC client functionality used by *tortoisecoin-cli* executable |
| *libtortoisecoin_common*      | Home for common functionality shared by different executables and libraries. Similar to *libtortoisecoin_util*, but higher-level (see [Dependencies](#dependencies)). |
| *libtortoisecoin_consensus*   | Stable, backwards-compatible consensus functionality used by *libtortoisecoin_node* and *libtortoisecoin_wallet*. |
| *libtortoisecoin_crypto*      | Hardware-optimized functions for data encryption, hashing, message authentication, and key derivation. |
| *libtortoisecoin_kernel*      | Consensus engine and support library used for validation by *libtortoisecoin_node*. |
| *libtortoisecoinqt*           | GUI functionality used by *tortoisecoin-qt* and *tortoisecoin-gui* executables. |
| *libtortoisecoin_ipc*         | IPC functionality used by *tortoisecoin-node*, *tortoisecoin-wallet*, *tortoisecoin-gui* executables to communicate when [`--enable-multiprocess`](multiprocess.md) is used. |
| *libtortoisecoin_node*        | P2P and RPC server functionality used by *tortoisecoind* and *tortoisecoin-qt* executables. |
| *libtortoisecoin_util*        | Home for common functionality shared by different executables and libraries. Similar to *libtortoisecoin_common*, but lower-level (see [Dependencies](#dependencies)). |
| *libtortoisecoin_wallet*      | Wallet functionality used by *tortoisecoind* and *tortoisecoin-wallet* executables. |
| *libtortoisecoin_wallet_tool* | Lower-level wallet functionality used by *tortoisecoin-wallet* executable. |
| *libtortoisecoin_zmq*         | [ZeroMQ](../zmq.md) functionality used by *tortoisecoind* and *tortoisecoin-qt* executables. |

## Conventions

- Most libraries are internal libraries and have APIs which are completely unstable! There are few or no restrictions on backwards compatibility or rules about external dependencies. An exception is *libtortoisecoin_kernel*, which, at some future point, will have a documented external interface.

- Generally each library should have a corresponding source directory and namespace. Source code organization is a work in progress, so it is true that some namespaces are applied inconsistently, and if you look at [`libtortoisecoin_*_SOURCES`](../../src/Makefile.am) lists you can see that many libraries pull in files from outside their source directory. But when working with libraries, it is good to follow a consistent pattern like:

  - *libtortoisecoin_node* code lives in `src/node/` in the `node::` namespace
  - *libtortoisecoin_wallet* code lives in `src/wallet/` in the `wallet::` namespace
  - *libtortoisecoin_ipc* code lives in `src/ipc/` in the `ipc::` namespace
  - *libtortoisecoin_util* code lives in `src/util/` in the `util::` namespace
  - *libtortoisecoin_consensus* code lives in `src/consensus/` in the `Consensus::` namespace

## Dependencies

- Libraries should minimize what other libraries they depend on, and only reference symbols following the arrows shown in the dependency graph below:

<table><tr><td>

```mermaid

%%{ init : { "flowchart" : { "curve" : "basis" }}}%%

graph TD;

tortoisecoin-cli[tortoisecoin-cli]-->libtortoisecoin_cli;

tortoisecoind[tortoisecoind]-->libtortoisecoin_node;
tortoisecoind[tortoisecoind]-->libtortoisecoin_wallet;

tortoisecoin-qt[tortoisecoin-qt]-->libtortoisecoin_node;
tortoisecoin-qt[tortoisecoin-qt]-->libtortoisecoinqt;
tortoisecoin-qt[tortoisecoin-qt]-->libtortoisecoin_wallet;

tortoisecoin-wallet[tortoisecoin-wallet]-->libtortoisecoin_wallet;
tortoisecoin-wallet[tortoisecoin-wallet]-->libtortoisecoin_wallet_tool;

libtortoisecoin_cli-->libtortoisecoin_util;
libtortoisecoin_cli-->libtortoisecoin_common;

libtortoisecoin_consensus-->libtortoisecoin_crypto;

libtortoisecoin_common-->libtortoisecoin_consensus;
libtortoisecoin_common-->libtortoisecoin_crypto;
libtortoisecoin_common-->libtortoisecoin_util;

libtortoisecoin_kernel-->libtortoisecoin_consensus;
libtortoisecoin_kernel-->libtortoisecoin_crypto;
libtortoisecoin_kernel-->libtortoisecoin_util;

libtortoisecoin_node-->libtortoisecoin_consensus;
libtortoisecoin_node-->libtortoisecoin_crypto;
libtortoisecoin_node-->libtortoisecoin_kernel;
libtortoisecoin_node-->libtortoisecoin_common;
libtortoisecoin_node-->libtortoisecoin_util;

libtortoisecoinqt-->libtortoisecoin_common;
libtortoisecoinqt-->libtortoisecoin_util;

libtortoisecoin_util-->libtortoisecoin_crypto;

libtortoisecoin_wallet-->libtortoisecoin_common;
libtortoisecoin_wallet-->libtortoisecoin_crypto;
libtortoisecoin_wallet-->libtortoisecoin_util;

libtortoisecoin_wallet_tool-->libtortoisecoin_wallet;
libtortoisecoin_wallet_tool-->libtortoisecoin_util;

classDef bold stroke-width:2px, font-weight:bold, font-size: smaller;
class tortoisecoin-qt,tortoisecoind,tortoisecoin-cli,tortoisecoin-wallet bold
```
</td></tr><tr><td>

**Dependency graph**. Arrows show linker symbol dependencies. *Crypto* lib depends on nothing. *Util* lib is depended on by everything. *Kernel* lib depends only on consensus, crypto, and util.

</td></tr></table>

- The graph shows what _linker symbols_ (functions and variables) from each library other libraries can call and reference directly, but it is not a call graph. For example, there is no arrow connecting *libtortoisecoin_wallet* and *libtortoisecoin_node* libraries, because these libraries are intended to be modular and not depend on each other's internal implementation details. But wallet code is still able to call node code indirectly through the `interfaces::Chain` abstract class in [`interfaces/chain.h`](../../src/interfaces/chain.h) and node code calls wallet code through the `interfaces::ChainClient` and `interfaces::Chain::Notifications` abstract classes in the same file. In general, defining abstract classes in [`src/interfaces/`](../../src/interfaces/) can be a convenient way of avoiding unwanted direct dependencies or circular dependencies between libraries.

- *libtortoisecoin_crypto* should be a standalone dependency that any library can depend on, and it should not depend on any other libraries itself.

- *libtortoisecoin_consensus* should only depend on *libtortoisecoin_crypto*, and all other libraries besides *libtortoisecoin_crypto* should be allowed to depend on it.

- *libtortoisecoin_util* should be a standalone dependency that any library can depend on, and it should not depend on other libraries except *libtortoisecoin_crypto*. It provides basic utilities that fill in gaps in the C++ standard library and provide lightweight abstractions over platform-specific features. Since the util library is distributed with the kernel and is usable by kernel applications, it shouldn't contain functions that external code shouldn't call, like higher level code targeted at the node or wallet. (*libtortoisecoin_common* is a better place for higher level code, or code that is meant to be used by internal applications only.)

- *libtortoisecoin_common* is a home for miscellaneous shared code used by different Tortoisecoin Core applications. It should not depend on anything other than *libtortoisecoin_util*, *libtortoisecoin_consensus*, and *libtortoisecoin_crypto*.

- *libtortoisecoin_kernel* should only depend on *libtortoisecoin_util*, *libtortoisecoin_consensus*, and *libtortoisecoin_crypto*.

- The only thing that should depend on *libtortoisecoin_kernel* internally should be *libtortoisecoin_node*. GUI and wallet libraries *libtortoisecoinqt* and *libtortoisecoin_wallet* in particular should not depend on *libtortoisecoin_kernel* and the unneeded functionality it would pull in, like block validation. To the extent that GUI and wallet code need scripting and signing functionality, they should be get able it from *libtortoisecoin_consensus*, *libtortoisecoin_common*, *libtortoisecoin_crypto*, and *libtortoisecoin_util*, instead of *libtortoisecoin_kernel*.

- GUI, node, and wallet code internal implementations should all be independent of each other, and the *libtortoisecoinqt*, *libtortoisecoin_node*, *libtortoisecoin_wallet* libraries should never reference each other's symbols. They should only call each other through [`src/interfaces/`](../../src/interfaces/) abstract interfaces.

## Work in progress

- Validation code is moving from *libtortoisecoin_node* to *libtortoisecoin_kernel* as part of [The libtortoisecoinkernel Project #27587](https://github.com/tortoisecoin/tortoisecoin/issues/27587)
