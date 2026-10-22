# UNIX BUILD NOTES

Some notes on how to build Bitcoin Core on Unix-like systems.

## To Build

```bash
cmake -B build
```
Run `cmake -B build -LH` to see the full list of available options.

```bash
cmake --build build    # Append "-j N" for N parallel jobs
ctest --test-dir build # Append "-j N" for N parallel tests
cmake --install build  # Optional
```

## Dependencies

You can either build from self-compiled [depends](/depends/README.md) or
install the dependencies from your distribution package manager. Dependencies
for additional features in later columns are optional.

| Package manager         | Required build dependencies | SQLite (wallet) | Cap'n Proto (IPC) | ZMQ <sup><a href="#note1">[1]</a></sup> | USDT | Qt and libqrencode (GUI) |
| ----------------------- | --------------------------- | --------------- | ----------------- | --------------------------------------- | ---- | ------------------------ |
| Debian / Ubuntu (`apt`) | `build-essential cmake python3 libboost-dev` | `libsqlite3-dev` | `libcapnp-dev capnproto` | `libzmq3-dev pkgconf` | `systemtap-sdt-dev` | `qt6-base-dev qt6-tools-dev qt6-l10n-tools qt6-tools-dev-tools libgl-dev qt6-wayland libqrencode-dev` |
| Fedora (`dnf`)          | `gcc-c++ cmake make python3 boost-devel` | `sqlite-devel` | `capnproto capnproto-devel` | `zeromq-devel pkgconf` | `systemtap-sdt-devel` | `qt6-qtbase-devel qt6-qttools-devel qt6-qtwayland qrencode-devel` |
| Alpine (`apk`)          | `build-base cmake linux-headers python3 boost-dev` | `sqlite-dev` | `capnproto capnproto-dev` | `zeromq-dev` | Not supported | `qt6-qtbase-dev qt6-qttools-dev libqrencode-dev` |
| Arch (`pacman`)         | `gcc make cmake python boost` | `sqlite` | `capnproto` | `zeromq` | `systemtap` | `qt6-base qt6-tools qt6-wayland qrencode` |
| [FreeBSD](https://www.freebsd.org/security/#sup) (`pkg`)             | `boost-libs cmake git python3` | `sqlite3` | `capnproto` | `libzmq4 pkgconf` | — | `qt6-base qt6-tools libqrencode` |
| [NetBSD](https://www.netbsd.org/releases/) (`pkgin`)                 | `git cmake boost python313` | `sqlite3` | `capnproto pkgconf` | `zeromq pkgconf` | — | `qt6-qtbase qt6-qttools qrencode` |
| [OpenBSD](https://www.openbsd.org/faq/faq5.html#Flavors) (`pkg_add`) | `git cmake boost python` | `sqlite3` | `capnproto` | `zeromq` | — | `qt6-qtbase qt6-qttools libqrencode` |

<a id="note1"></a>1. Some ZMQ packages do not vendor the CMake config files, which requires `pkgconf` or `pkg-config` to be installed.

### Supported Systems

Bitcoin Core is supported on the most recent releases of supported systems.

For Debian "oldstable", or earlier Ubuntu LTS releases, you may need to pick a
later compiler version, according to the [dependencies](/doc/dependencies.md)
documentation.

### IPC

Cap'n Proto is needed for IPC functionality (see [multiprocess.md](multiprocess.md)).
Compile with `-DENABLE_IPC=OFF` if you do not need IPC functionality.

### ZMQ notifications

ZMQ-enabled binaries are compiled with `-DWITH_ZMQ=ON` and require libzmq.

### Tracing

User-Space, Statically Defined Tracing (USDT) requires the systemtap-sdt
development package and must be enabled via `-DWITH_USDT=ON`.

### GUI

Bitcoin Core includes a GUI built with the cross-platform Qt Framework. To compile the GUI, we need to install
the necessary parts of Qt, the libqrencode and pass `-DBUILD_GUI=ON`. Skip if you don't intend to use the GUI.

Additionally, install the Qt Wayland platform plugin for modern desktop environments.

The GUI will be able to encode addresses in QR codes and requires libqrencode.
Otherwise, if you don't need QR encoding support, use the `-DWITH_QRENCODE=OFF` option to disable this feature in order to compile the GUI.

### Disable-wallet mode

When the intention is to only run a P2P node, without a wallet, Bitcoin Core can
be compiled in disable-wallet mode with:

    cmake -B build -DENABLE_WALLET=OFF

In this case there is no dependency on SQLite.

Mining is also possible in disable-wallet mode using the `getblocktemplate` RPC call.

### Test Suite Dependencies

See [`test/README.md`](/test/README.md#dependencies-and-prerequisites).

## Memory Requirements

C++ compilers are memory-hungry. It is recommended to have at least 1.5 GB of
memory available when compiling Bitcoin Core. On systems with less, gcc can be
tuned to conserve memory with additional `CMAKE_CXX_FLAGS`:


    cmake -B build -DCMAKE_CXX_FLAGS="--param ggc-min-expand=1 --param ggc-min-heapsize=32768"

Alternatively, or in addition, debugging information can be skipped for compilation.
For the default build type `RelWithDebInfo`, the default compile flags are
`-O2 -g`, and can be changed with:

    cmake -B build -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O2 -g0"

Finally, clang (often less resource hungry) can be used instead of gcc, which is used by default:

    cmake -B build -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang

### ulimit data size

On some systems (like OpenBSD), the standard ulimit restrictions may be too
strict. Use `ulimit -d` to read and set the data limit to a larger value.
