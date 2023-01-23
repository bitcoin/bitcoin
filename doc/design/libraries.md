# Libraries

| Name                     | Description |
|--------------------------|-------------|
| *libwidecoin_cli*         | RPC client functionality used by *widecoin-cli* executable |
| *libwidecoin_common*      | Home for common functionality shared by different executables and libraries. Similar to *libwidecoin_util*, but higher-level (see [Dependencies](#dependencies)). |
| *libwidecoin_consensus*   | Stable, backwards-compatible consensus functionality used by *libwidecoin_node* and *libwidecoin_wallet* and also exposed as a [shared library](../shared-libraries.md). |
| *libwidecoinconsensus*    | Shared library build of static *libwidecoin_consensus* library |
| *libwidecoin_kernel*      | Consensus engine and support library used for validation by *libwidecoin_node* and also exposed as a [shared library](../shared-libraries.md). |
| *libwidecoinqt*           | GUI functionality used by *widecoin-qt* and *widecoin-gui* executables |
| *libwidecoin_ipc*         | IPC functionality used by *widecoin-node*, *widecoin-wallet*, *widecoin-gui* executables to communicate when [`--enable-multiprocess`](multiprocess.md) is used. |
| *libwidecoin_node*        | P2P and RPC server functionality used by *widecoind* and *widecoin-qt* executables. |
| *libwidecoin_util*        | Home for common functionality shared by different executables and libraries. Similar to *libwidecoin_common*, but lower-level (see [Dependencies](#dependencies)). |
| *libwidecoin_wallet*      | Wallet functionality used by *widecoind* and *widecoin-wallet* executables. |
| *libwidecoin_wallet_tool* | Lower-level wallet functionality used by *widecoin-wallet* executable. |
| *libwidecoin_zmq*         | [ZeroMQ](../zmq.md) functionality used by *widecoind* and *widecoin-qt* executables. |

## Conventions

- Most libraries are internal libraries and have APIs which are completely unstable! There are few or no restrictions on backwards compatibility or rules about external dependencies. Exceptions are *libwidecoin_consensus* and *libwidecoin_kernel* which have external interfaces documented at [../shared-libraries.md](../shared-libraries.md).

- Generally each library should have a corresponding source directory and namespace. Source code organization is a work in progress, so it is true that some namespaces are applied inconsistently, and if you look at [`libwidecoin_*_SOURCES`](../../src/Makefile.am) lists you can see that many libraries pull in files from outside their source directory. But when working with libraries, it is good to follow a consistent pattern like:

  - *libwidecoin_node* code lives in `src/node/` in the `node::` namespace
  - *libwidecoin_wallet* code lives in `src/wallet/` in the `wallet::` namespace
  - *libwidecoin_ipc* code lives in `src/ipc/` in the `ipc::` namespace
  - *libwidecoin_util* code lives in `src/util/` in the `util::` namespace
  - *libwidecoin_consensus* code lives in `src/consensus/` in the `Consensus::` namespace

## Dependencies

- Libraries should minimize what other libraries they depend on, and only reference symbols following the arrows shown in the dependency graph below:

<table><tr><td>

```mermaid

%%{ init : { "flowchart" : { "curve" : "linear" }}}%%

graph TD;

widecoin-cli[widecoin-cli]-->libwidecoin_cli;

widecoind[widecoind]-->libwidecoin_node;
widecoind[widecoind]-->libwidecoin_wallet;

widecoin-qt[widecoin-qt]-->libwidecoin_node;
widecoin-qt[widecoin-qt]-->libwidecoinqt;
widecoin-qt[widecoin-qt]-->libwidecoin_wallet;

widecoin-wallet[widecoin-wallet]-->libwidecoin_wallet;
widecoin-wallet[widecoin-wallet]-->libwidecoin_wallet_tool;

libwidecoin_cli-->libwidecoin_common;
libwidecoin_cli-->libwidecoin_util;

libwidecoin_common-->libwidecoin_util;
libwidecoin_common-->libwidecoin_consensus;

libwidecoin_kernel-->libwidecoin_consensus;
libwidecoin_kernel-->libwidecoin_util;

libwidecoin_node-->libwidecoin_common;
libwidecoin_node-->libwidecoin_consensus;
libwidecoin_node-->libwidecoin_kernel;
libwidecoin_node-->libwidecoin_util;

libwidecoinqt-->libwidecoin_common;
libwidecoinqt-->libwidecoin_util;

libwidecoin_wallet-->libwidecoin_common;
libwidecoin_wallet-->libwidecoin_util;

libwidecoin_wallet_tool-->libwidecoin_util;
libwidecoin_wallet_tool-->libwidecoin_wallet;

classDef bold stroke-width:2px, font-weight:bold, font-size: smaller;
class widecoin-qt,widecoind,widecoin-cli,widecoin-wallet bold
```
</td></tr><tr><td>

**Dependency graph**. Arrows show linker symbol dependencies. *Consensus* lib depends on nothing. *Util* lib is depended on by everything. *Kernel* lib depends only on consensus and util.

</td></tr></table>

- The graph shows what _linker symbols_ (functions and variables) from each library other libraries can call and reference directly, but it is not a call graph. For example, there is no arrow connecting *libwidecoin_wallet* and *libwidecoin_node* libraries, because these libraries are intended to be modular and not depend on each other's internal implementation details. But wallet code still is still able to call node code indirectly through the `interfaces::Chain` abstract class in [`interfaces/chain.h`](../../src/interfaces/chain.h) and node code calls wallet code through the `interfaces::ChainClient` and `interfaces::Chain::Notifications` abstract classes in the same file. In general, defining abstract classes in [`src/interfaces/`](../../src/interfaces/) can be a convenient way of avoiding unwanted direct dependencies or circular dependencies between libraries.

- *libwidecoin_consensus* should be a standalone dependency that any library can depend on, and it should not depend on any other libraries itself.

- *libwidecoin_util* should also be a standalone dependency that any library can depend on, and it should not depend on other internal libraries.

- *libwidecoin_common* should serve a similar function as *libwidecoin_util* and be a place for miscellaneous code used by various daemon, GUI, and CLI applications and libraries to live. It should not depend on anything other than *libwidecoin_util* and *libwidecoin_consensus*. The boundary between _util_ and _common_ is a little fuzzy but historically _util_ has been used for more generic, lower-level things like parsing hex, and _common_ has been used for widecoin-specific, higher-level things like parsing base58. The difference between util and common is mostly important because *libwidecoin_kernel* is not supposed to depend on *libwidecoin_common*, only *libwidecoin_util*. In general, if it is ever unclear whether it is better to add code to *util* or *common*, it is probably better to add it to *common* unless it is very generically useful or useful particularly to include in the kernel.


- *libwidecoin_kernel* should only depend on *libwidecoin_util* and *libwidecoin_consensus*.

- The only thing that should depend on *libwidecoin_kernel* internally should be *libwidecoin_node*. GUI and wallet libraries *libwidecoinqt* and *libwidecoin_wallet* in particular should not depend on *libwidecoin_kernel* and the unneeded functionality it would pull in, like block validation. To the extent that GUI and wallet code need scripting and signing functionality, they should be get able it from *libwidecoin_consensus*, *libwidecoin_common*, and *libwidecoin_util*, instead of *libwidecoin_kernel*.

- GUI, node, and wallet code internal implementations should all be independent of each other, and the *libwidecoinqt*, *libwidecoin_node*, *libwidecoin_wallet* libraries should never reference each other's symbols. They should only call each other through [`src/interfaces/`](`../../src/interfaces/`) abstract interfaces.

## Work in progress

- Validation code is moving from *libwidecoin_node* to *libwidecoin_kernel* as part of [The libwidecoinkernel Project #24303](https://github.com/widecoin/widecoin/issues/24303)
- Source code organization is discussed in general in [Library source code organization #15732](https://github.com/widecoin/widecoin/issues/15732)
