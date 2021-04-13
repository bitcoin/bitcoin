# Multiprocess Syscoin

On unix systems, the `--enable-multiprocess` build option can be passed to `./configure` to build new `syscoin-node`, `syscoin-wallet`, and `syscoin-gui` executables alongside existing `syscoind` and `syscoin-qt` executables.

`syscoin-node` is a drop-in replacement for `syscoind`, and `syscoin-gui` is a drop-in replacement for `syscoin-qt`, and there are no differences in use or external behavior between the new and old executables. But internally (after [#10102](https://github.com/syscoin/syscoin/pull/10102)), `syscoin-gui` will spawn a `syscoin-node` process to run P2P and RPC code, communicating with it across a socket pair, and `syscoin-node` will spawn `syscoin-wallet` to run wallet code, also communicating over a socket pair. This will let node, wallet, and GUI code run in separate address spaces for better isolation, and allow future improvements like being able to start and stop components independently on different machines and environments.

## Next steps

Specific next steps after [#10102](https://github.com/syscoin/syscoin/pull/10102) will be:

- [ ] Adding `-ipcbind` and `-ipcconnect` options to `syscoin-node`, `syscoin-wallet`, and `syscoin-gui` executables so they can listen and connect to TCP ports and unix socket paths. This will allow separate processes to be started and stopped any time and connect to each other.
- [ ] Adding `-server` and `-rpcbind` options to the `syscoin-wallet` executable so wallet processes can handle RPC requests directly without going through the node.
- [ ] Supporting windows, not just unix systems. The existing socket code is already cross-platform, so the only windows-specific code that needs to be written is code spawning a process and passing a socket descriptor. This can be implemented with `CreateProcess` and `WSADuplicateSocket`. Example: https://memset.wordpress.com/2010/10/13/win32-api-passing-socket-with-ipc-method/.
- [ ] Adding sandbox features, restricting subprocess access to resources and data. See [https://eklitzke.org/multiprocess-syscoin](https://eklitzke.org/multiprocess-syscoin).

## Debugging

After [#10102](https://github.com/syscoin/syscoin/pull/10102), the `-debug=ipc` command line option can be used to see requests and responses between processes.

## Installation

The multiprocess feature requires [Cap'n Proto](https://capnproto.org/) and [libmultiprocess](https://github.com/chaincodelabs/libmultiprocess) as dependencies. A simple way to get starting using it without installing these dependencies manually is to use the [depends system](../depends) with the `MULTIPROCESS=1` [dependency option](../depends#dependency-options) passed to make:

```
cd <SYSCOIN_SOURCE_DIRECTORY>
make -C depends NO_QT=1 MULTIPROCESS=1
CONFIG_SITE=$PWD/depends/x86_64-pc-linux-gnu/share/config.site ./configure
make
src/syscoin-node -regtest -printtoconsole -debug=ipc
SYSCOIND=syscoin-node test/functional/test_runner.py
```

The configure script will pick up settings and library locations from the depends directory, so there is no need to pass `--enable-multiprocess` as a separate flag when using the depends system (it's controlled by the `MULTIPROCESS=1` option).

Alternately, you can install [Cap'n Proto](https://capnproto.org/) and [libmultiprocess](https://github.com/chaincodelabs/libmultiprocess) packages on your system, and just run `./configure --enable-multiprocess` without using the depends system. The configure script will be able to locate the installed packages via [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/). See [Installation](https://github.com/chaincodelabs/libmultiprocess#installation) section of the libmultiprocess readme for install steps. See [build-unix.md](build-unix.md) and [build-osx.md](build-osx.md) for information about installing dependencies in general.
