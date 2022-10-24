# Libraries

| Name                     | Description |
|--------------------------|-------------|
| *libsyscoin_cli*         | RPC client functionality used by *syscoin-cli* executable |
| *libsyscoin_common*      | Home for common functionality shared by different executables and libraries. Similar to *libsyscoin_util*, but higher-level (see [Dependencies](#dependencies)). |
| *libsyscoin_consensus*   | Stable, backwards-compatible consensus functionality used by *libsyscoin_node* and *libsyscoin_wallet* and also exposed as a [shared library](../shared-libraries.md). |
| *libsyscoinconsensus*    | Shared library build of static *libsyscoin_consensus* library |
| *libsyscoin_kernel*      | Consensus engine and support library used for validation by *libsyscoin_node* and also exposed as a [shared library](../shared-libraries.md). |
| *libsyscoinqt*           | GUI functionality used by *syscoin-qt* and *syscoin-gui* executables |
| *libsyscoin_ipc*         | IPC functionality used by *syscoin-node*, *syscoin-wallet*, *syscoin-gui* executables to communicate when [`--enable-multiprocess`](multiprocess.md) is used. |
| *libsyscoin_node*        | P2P and RPC server functionality used by *syscoind* and *syscoin-qt* executables. |
| *libsyscoin_util*        | Home for common functionality shared by different executables and libraries. Similar to *libsyscoin_common*, but lower-level (see [Dependencies](#dependencies)). |
| *libsyscoin_wallet*      | Wallet functionality used by *syscoind* and *syscoin-wallet* executables. |
| *libsyscoin_wallet_tool* | Lower-level wallet functionality used by *syscoin-wallet* executable. |
| *libsyscoin_zmq*         | [ZeroMQ](../zmq.md) functionality used by *syscoind* and *syscoin-qt* executables. |

## Conventions

- Most libraries are internal libraries and have APIs which are completely unstable! There are few or no restrictions on backwards compatibility or rules about external dependencies. Exceptions are *libsyscoin_consensus* and *libsyscoin_kernel* which have external interfaces documented at [../shared-libraries.md](../shared-libraries.md).

- Generally each library should have a corresponding source directory and namespace. Source code organization is a work in progress, so it is true that some namespaces are applied inconsistently, and if you look at [`libsyscoin_*_SOURCES`](../../src/Makefile.am) lists you can see that many libraries pull in files from outside their source directory. But when working with libraries, it is good to follow a consistent pattern like:

  - *libsyscoin_node* code lives in `src/node/` in the `node::` namespace
  - *libsyscoin_wallet* code lives in `src/wallet/` in the `wallet::` namespace
  - *libsyscoin_ipc* code lives in `src/ipc/` in the `ipc::` namespace
  - *libsyscoin_util* code lives in `src/util/` in the `util::` namespace
  - *libsyscoin_consensus* code lives in `src/consensus/` in the `Consensus::` namespace

## Dependencies

- Libraries should minimize what other libraries they depend on, and only reference symbols following the arrows shown in the dependency graph below:

<table><tr><td>

```mermaid

%%{ init : { "flowchart" : { "curve" : "basis" }}}%%

graph TD;

syscoin-cli[syscoin-cli]-->libsyscoin_cli;

syscoind[syscoind]-->libsyscoin_node;
syscoind[syscoind]-->libsyscoin_wallet;

syscoin-qt[syscoin-qt]-->libsyscoin_node;
syscoin-qt[syscoin-qt]-->libsyscoinqt;
syscoin-qt[syscoin-qt]-->libsyscoin_wallet;

syscoin-wallet[syscoin-wallet]-->libsyscoin_wallet;
syscoin-wallet[syscoin-wallet]-->libsyscoin_wallet_tool;

libsyscoin_cli-->libsyscoin_util;
libsyscoin_cli-->libsyscoin_common;

libsyscoin_common-->libsyscoin_consensus;
libsyscoin_common-->libsyscoin_util;

libsyscoin_kernel-->libsyscoin_consensus;
libsyscoin_kernel-->libsyscoin_util;

libsyscoin_node-->libsyscoin_consensus;
libsyscoin_node-->libsyscoin_kernel;
libsyscoin_node-->libsyscoin_common;
libsyscoin_node-->libsyscoin_util;

libsyscoinqt-->libsyscoin_common;
libsyscoinqt-->libsyscoin_util;

libsyscoin_wallet-->libsyscoin_common;
libsyscoin_wallet-->libsyscoin_util;

libsyscoin_wallet_tool-->libsyscoin_wallet;
libsyscoin_wallet_tool-->libsyscoin_util;

classDef bold stroke-width:2px, font-weight:bold, font-size: smaller;
class syscoin-qt,syscoind,syscoin-cli,syscoin-wallet bold
```
</td></tr><tr><td>

**Dependency graph**. Arrows show linker symbol dependencies. *Consensus* lib depends on nothing. *Util* lib is depended on by everything. *Kernel* lib depends only on consensus and util.

</td></tr></table>

- The graph shows what _linker symbols_ (functions and variables) from each library other libraries can call and reference directly, but it is not a call graph. For example, there is no arrow connecting *libsyscoin_wallet* and *libsyscoin_node* libraries, because these libraries are intended to be modular and not depend on each other's internal implementation details. But wallet code is still able to call node code indirectly through the `interfaces::Chain` abstract class in [`interfaces/chain.h`](../../src/interfaces/chain.h) and node code calls wallet code through the `interfaces::ChainClient` and `interfaces::Chain::Notifications` abstract classes in the same file. In general, defining abstract classes in [`src/interfaces/`](../../src/interfaces/) can be a convenient way of avoiding unwanted direct dependencies or circular dependencies between libraries.

- *libsyscoin_consensus* should be a standalone dependency that any library can depend on, and it should not depend on any other libraries itself.

- *libsyscoin_util* should also be a standalone dependency that any library can depend on, and it should not depend on other internal libraries.

- *libsyscoin_common* should serve a similar function as *libsyscoin_util* and be a place for miscellaneous code used by various daemon, GUI, and CLI applications and libraries to live. It should not depend on anything other than *libsyscoin_util* and *libsyscoin_consensus*. The boundary between _util_ and _common_ is a little fuzzy but historically _util_ has been used for more generic, lower-level things like parsing hex, and _common_ has been used for syscoin-specific, higher-level things like parsing base58. The difference between util and common is mostly important because *libsyscoin_kernel* is not supposed to depend on *libsyscoin_common*, only *libsyscoin_util*. In general, if it is ever unclear whether it is better to add code to *util* or *common*, it is probably better to add it to *common* unless it is very generically useful or useful particularly to include in the kernel.


- *libsyscoin_kernel* should only depend on *libsyscoin_util* and *libsyscoin_consensus*.

- The only thing that should depend on *libsyscoin_kernel* internally should be *libsyscoin_node*. GUI and wallet libraries *libsyscoinqt* and *libsyscoin_wallet* in particular should not depend on *libsyscoin_kernel* and the unneeded functionality it would pull in, like block validation. To the extent that GUI and wallet code need scripting and signing functionality, they should be get able it from *libsyscoin_consensus*, *libsyscoin_common*, and *libsyscoin_util*, instead of *libsyscoin_kernel*.

- GUI, node, and wallet code internal implementations should all be independent of each other, and the *libsyscoinqt*, *libsyscoin_node*, *libsyscoin_wallet* libraries should never reference each other's symbols. They should only call each other through [`src/interfaces/`](`../../src/interfaces/`) abstract interfaces.

## Work in progress

- Validation code is moving from *libsyscoin_node* to *libsyscoin_kernel* as part of [The libsyscoinkernel Project #24303](https://github.com/syscoin/syscoin/issues/24303)
- Source code organization is discussed in general in [Library source code organization #15732](https://github.com/syscoin/syscoin/issues/15732)
