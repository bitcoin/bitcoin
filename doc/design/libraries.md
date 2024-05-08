# Libraries

| Name                     | Description |
|--------------------------|-------------|
| *libbitcoin_cli*         | RPC client functionality used by *bitcoin-cli* executable |
| *libbitcoin_common*      | Home for common functionality shared by different executables and libraries. Similar to *libbitcoin_util*, but higher-level (see [Dependencies](#dependencies)). |
| *libbitcoin_consensus*   | Stable, backwards-compatible consensus functionality used by *libbitcoin_node* and *libbitcoin_wallet*. |
| *libbitcoin_kernel*      | Consensus engine and support library used for validation by *libbitcoin_node*. |
| *libbitcoinqt*           | GUI functionality used by *bitcoin-qt* and *bitcoin-gui* executables. |
| *libbitcoin_ipc*         | IPC functionality used by *bitcoin-node*, *bitcoin-wallet*, *bitcoin-gui* executables to communicate when [`--enable-multiprocess`](multiprocess.md) is used. |
| *libbitcoin_node*        | P2P and RPC server functionality used by *bitcoind* and *bitcoin-qt* executables. |
| *libbitcoin_util*        | Home for common functionality shared by different executables and libraries. Similar to *libbitcoin_common*, but lower-level (see [Dependencies](#dependencies)). |
| *libbitcoin_wallet*      | Wallet functionality used by *bitcoind* and *bitcoin-wallet* executables. |
| *libbitcoin_wallet_tool* | Lower-level wallet functionality used by *bitcoin-wallet* executable. |
| *libbitcoin_zmq*         | [ZeroMQ](../zmq.md) functionality used by *bitcoind* and *bitcoin-qt* executables. |

## Conventions

- Most libraries are internal libraries and have APIs which are completely unstable! There are few or no restrictions on backwards compatibility or rules about external dependencies. An exception is *libbitcoin_kernel*, which, at some future point, will have a documented external interface.

- Generally each library should have a corresponding source directory and namespace. Source code organization is a work in progress, so it is true that some namespaces are applied inconsistently, and if you look at [`libbitcoin_*_SOURCES`](../../src/Makefile.am) lists you can see that many libraries pull in files from outside their source directory. But when working with libraries, it is good to follow a consistent pattern like:

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

libbitcoin_common-->libbitcoin_consensus;
libbitcoin_common-->libbitcoin_util;

libbitcoin_kernel-->libbitcoin_consensus;
libbitcoin_kernel-->libbitcoin_util;

libbitcoin_node-->libbitcoin_consensus;
libbitcoin_node-->libbitcoin_kernel;
libbitcoin_node-->libbitcoin_common;
libbitcoin_node-->libbitcoin_util;

libbitcoinqt-->libbitcoin_common;
libbitcoinqt-->libbitcoin_util;

libbitcoin_wallet-->libbitcoin_common;
libbitcoin_wallet-->libbitcoin_util;

libbitcoin_wallet_tool-->libbitcoin_wallet;
libbitcoin_wallet_tool-->libbitcoin_util;

classDef bold stroke-width:2px, font-weight:bold, font-size: smaller;
class bitcoin-qt,bitcoind,bitcoin-cli,bitcoin-wallet bold
```
</td></tr><tr><td>

**Dependency graph**. Arrows show linker symbol dependencies. *Consensus* lib depends on nothing. *Util* lib is depended on by everything. *Kernel* lib depends only on consensus and util.

</td></tr></table>

- The graph shows what _linker symbols_ (functions and variables) from each library other libraries can call and reference directly, but it is not a call graph. For example, there is no arrow connecting *libbitcoin_wallet* and *libbitcoin_node* libraries, because these libraries are intended to be modular and not depend on each other's internal implementation details. But wallet code is still able to call node code indirectly through the `interfaces::Chain` abstract class in [`interfaces/chain.h`](../../src/interfaces/chain.h) and node code calls wallet code through the `interfaces::ChainClient` and `interfaces::Chain::Notifications` abstract classes in the same file. In general, defining abstract classes in [`src/interfaces/`](../../src/interfaces/) can be a convenient way of avoiding unwanted direct dependencies or circular dependencies between libraries.

- *libbitcoin_consensus* should be a standalone dependency that any library can depend on, and it should not depend on any other libraries itself.

- *libbitcoin_util* should also be a standalone dependency that any library can depend on, and it should not depend on other internal libraries.

- *libbitcoin_common* should serve a similar function as *libbitcoin_util* and be a place for miscellaneous code used by various daemon, GUI, and CLI applications and libraries to live. It should not depend on anything other than *libbitcoin_util* and *libbitcoin_consensus*. The boundary between _util_ and _common_ is a little fuzzy but historically _util_ has been used for more generic, lower-level things like parsing hex, and _common_ has been used for bitcoin-specific, higher-level things like parsing base58. The difference between util and common is mostly important because *libbitcoin_kernel* is not supposed to depend on *libbitcoin_common*, only *libbitcoin_util*. In general, if it is ever unclear whether it is better to add code to *util* or *common*, it is probably better to add it to *common* unless it is very generically useful or useful particularly to include in the kernel.


- *libbitcoin_kernel* should only depend on *libbitcoin_util* and *libbitcoin_consensus*.

- The only thing that should depend on *libbitcoin_kernel* internally should be *libbitcoin_node*. GUI and wallet libraries *libbitcoinqt* and *libbitcoin_wallet* in particular should not depend on *libbitcoin_kernel* and the unneeded functionality it would pull in, like block validation. To the extent that GUI and wallet code need scripting and signing functionality, they should be get able it from *libbitcoin_consensus*, *libbitcoin_common*, and *libbitcoin_util*, instead of *libbitcoin_kernel*.

- GUI, node, and wallet code internal implementations should all be independent of each other, and the *libbitcoinqt*, *libbitcoin_node*, *libbitcoin_wallet* libraries should never reference each other's symbols. They should only call each other through [`src/interfaces/`](../../src/interfaces/) abstract interfaces.

## Work in progress

- Validation code is moving from *libbitcoin_node* to *libbitcoin_kernel* as part of [The libbitcoinkernel Project #24303](https://github.com/bitcoin/bitcoin/issues/24303)
- Source code organization is discussed in general in [Library source code organization #15732](https://github.com/bitcoin/bitcoin/issues/15732)
