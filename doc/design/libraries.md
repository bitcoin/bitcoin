# Libraries

| Name                     | Description |
|--------------------------|-------------|
| *libbritanniacoin_cli*         | RPC client functionality used by *britanniacoin-cli* executable |
| *libbritanniacoin_common*      | Home for common functionality shared by different executables and libraries. Similar to *libbritanniacoin_util*, but higher-level (see [Dependencies](#dependencies)). |
| *libbritanniacoin_consensus*   | Stable, backwards-compatible consensus functionality used by *libbritanniacoin_node* and *libbritanniacoin_wallet* and also exposed as a [shared library](../shared-libraries.md). |
| *libbritanniacoinconsensus*    | Shared library build of static *libbritanniacoin_consensus* library |
| *libbritanniacoin_kernel*      | Consensus engine and support library used for validation by *libbritanniacoin_node* and also exposed as a [shared library](../shared-libraries.md). |
| *libbritanniacoinqt*           | GUI functionality used by *britanniacoin-qt* and *britanniacoin-gui* executables |
| *libbritanniacoin_ipc*         | IPC functionality used by *britanniacoin-node*, *britanniacoin-wallet*, *britanniacoin-gui* executables to communicate when [`--enable-multiprocess`](multiprocess.md) is used. |
| *libbritanniacoin_node*        | P2P and RPC server functionality used by *britanniacoind* and *britanniacoin-qt* executables. |
| *libbritanniacoin_util*        | Home for common functionality shared by different executables and libraries. Similar to *libbritanniacoin_common*, but lower-level (see [Dependencies](#dependencies)). |
| *libbritanniacoin_wallet*      | Wallet functionality used by *britanniacoind* and *britanniacoin-wallet* executables. |
| *libbritanniacoin_wallet_tool* | Lower-level wallet functionality used by *britanniacoin-wallet* executable. |
| *libbritanniacoin_zmq*         | [ZeroMQ](../zmq.md) functionality used by *britanniacoind* and *britanniacoin-qt* executables. |

## Conventions

- Most libraries are internal libraries and have APIs which are completely unstable! There are few or no restrictions on backwards compatibility or rules about external dependencies. Exceptions are *libbritanniacoin_consensus* and *libbritanniacoin_kernel* which have external interfaces documented at [../shared-libraries.md](../shared-libraries.md).

- Generally each library should have a corresponding source directory and namespace. Source code organization is a work in progress, so it is true that some namespaces are applied inconsistently, and if you look at [`libbritanniacoin_*_SOURCES`](../../src/Makefile.am) lists you can see that many libraries pull in files from outside their source directory. But when working with libraries, it is good to follow a consistent pattern like:

  - *libbritanniacoin_node* code lives in `src/node/` in the `node::` namespace
  - *libbritanniacoin_wallet* code lives in `src/wallet/` in the `wallet::` namespace
  - *libbritanniacoin_ipc* code lives in `src/ipc/` in the `ipc::` namespace
  - *libbritanniacoin_util* code lives in `src/util/` in the `util::` namespace
  - *libbritanniacoin_consensus* code lives in `src/consensus/` in the `Consensus::` namespace

## Dependencies

- Libraries should minimize what other libraries they depend on, and only reference symbols following the arrows shown in the dependency graph below:

<table><tr><td>

```mermaid

%%{ init : { "flowchart" : { "curve" : "basis" }}}%%

graph TD;

britanniacoin-cli[britanniacoin-cli]-->libbritanniacoin_cli;

britanniacoind[britanniacoind]-->libbritanniacoin_node;
britanniacoind[britanniacoind]-->libbritanniacoin_wallet;

britanniacoin-qt[britanniacoin-qt]-->libbritanniacoin_node;
britanniacoin-qt[britanniacoin-qt]-->libbritanniacoinqt;
britanniacoin-qt[britanniacoin-qt]-->libbritanniacoin_wallet;

britanniacoin-wallet[britanniacoin-wallet]-->libbritanniacoin_wallet;
britanniacoin-wallet[britanniacoin-wallet]-->libbritanniacoin_wallet_tool;

libbritanniacoin_cli-->libbritanniacoin_util;
libbritanniacoin_cli-->libbritanniacoin_common;

libbritanniacoin_common-->libbritanniacoin_consensus;
libbritanniacoin_common-->libbritanniacoin_util;

libbritanniacoin_kernel-->libbritanniacoin_consensus;
libbritanniacoin_kernel-->libbritanniacoin_util;

libbritanniacoin_node-->libbritanniacoin_consensus;
libbritanniacoin_node-->libbritanniacoin_kernel;
libbritanniacoin_node-->libbritanniacoin_common;
libbritanniacoin_node-->libbritanniacoin_util;

libbritanniacoinqt-->libbritanniacoin_common;
libbritanniacoinqt-->libbritanniacoin_util;

libbritanniacoin_wallet-->libbritanniacoin_common;
libbritanniacoin_wallet-->libbritanniacoin_util;

libbritanniacoin_wallet_tool-->libbritanniacoin_wallet;
libbritanniacoin_wallet_tool-->libbritanniacoin_util;

classDef bold stroke-width:2px, font-weight:bold, font-size: smaller;
class britanniacoin-qt,britanniacoind,britanniacoin-cli,britanniacoin-wallet bold
```
</td></tr><tr><td>

**Dependency graph**. Arrows show linker symbol dependencies. *Consensus* lib depends on nothing. *Util* lib is depended on by everything. *Kernel* lib depends only on consensus and util.

</td></tr></table>

- The graph shows what _linker symbols_ (functions and variables) from each library other libraries can call and reference directly, but it is not a call graph. For example, there is no arrow connecting *libbritanniacoin_wallet* and *libbritanniacoin_node* libraries, because these libraries are intended to be modular and not depend on each other's internal implementation details. But wallet code is still able to call node code indirectly through the `interfaces::Chain` abstract class in [`interfaces/chain.h`](../../src/interfaces/chain.h) and node code calls wallet code through the `interfaces::ChainClient` and `interfaces::Chain::Notifications` abstract classes in the same file. In general, defining abstract classes in [`src/interfaces/`](../../src/interfaces/) can be a convenient way of avoiding unwanted direct dependencies or circular dependencies between libraries.

- *libbritanniacoin_consensus* should be a standalone dependency that any library can depend on, and it should not depend on any other libraries itself.

- *libbritanniacoin_util* should also be a standalone dependency that any library can depend on, and it should not depend on other internal libraries.

- *libbritanniacoin_common* should serve a similar function as *libbritanniacoin_util* and be a place for miscellaneous code used by various daemon, GUI, and CLI applications and libraries to live. It should not depend on anything other than *libbritanniacoin_util* and *libbritanniacoin_consensus*. The boundary between _util_ and _common_ is a little fuzzy but historically _util_ has been used for more generic, lower-level things like parsing hex, and _common_ has been used for britanniacoin-specific, higher-level things like parsing base58. The difference between util and common is mostly important because *libbritanniacoin_kernel* is not supposed to depend on *libbritanniacoin_common*, only *libbritanniacoin_util*. In general, if it is ever unclear whether it is better to add code to *util* or *common*, it is probably better to add it to *common* unless it is very generically useful or useful particularly to include in the kernel.


- *libbritanniacoin_kernel* should only depend on *libbritanniacoin_util* and *libbritanniacoin_consensus*.

- The only thing that should depend on *libbritanniacoin_kernel* internally should be *libbritanniacoin_node*. GUI and wallet libraries *libbritanniacoinqt* and *libbritanniacoin_wallet* in particular should not depend on *libbritanniacoin_kernel* and the unneeded functionality it would pull in, like block validation. To the extent that GUI and wallet code need scripting and signing functionality, they should be get able it from *libbritanniacoin_consensus*, *libbritanniacoin_common*, and *libbritanniacoin_util*, instead of *libbritanniacoin_kernel*.

- GUI, node, and wallet code internal implementations should all be independent of each other, and the *libbritanniacoinqt*, *libbritanniacoin_node*, *libbritanniacoin_wallet* libraries should never reference each other's symbols. They should only call each other through [`src/interfaces/`](`../../src/interfaces/`) abstract interfaces.

## Work in progress

- Validation code is moving from *libbritanniacoin_node* to *libbritanniacoin_kernel* as part of [The libbritanniacoinkernel Project #24303](https://github.com/britanniacoin/britanniacoin/issues/24303)
- Source code organization is discussed in general in [Library source code organization #15732](https://github.com/britanniacoin/britanniacoin/issues/15732)
