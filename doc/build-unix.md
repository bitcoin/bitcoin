UNIX BUILD NOTES
====================
Some notes on how to build Bitcoin Core in Unix.

(For BSD specific instructions, see `build-*bsd.md` in this directory.)

To Build
---------------------

```bash
cmake -B build
```
Run `cmake -B build -LH` to see the full list of available options.

```bash
cmake --build build    # Append "-j N" for N parallel jobs
cmake --install build  # Optional
```

See below for instructions on how to [install the dependencies on popular Linux
distributions](#linux-distribution-specific-instructions), or the
[dependencies](#dependencies) section for a complete overview.

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

## Linux Distribution Specific Instructions

### Ubuntu & Debian

#### Dependency Build Instructions

Build requirements:

    sudo apt-get install build-essential cmake pkgconf python3

Now, you can either build from self-compiled [depends](#dependencies) or install the required dependencies:

    sudo apt-get install libevent-dev libboost-dev

SQLite is required for the wallet:

    sudo apt install libsqlite3-dev

To build Bitcoin Core without the wallet, see [*Disable-wallet mode*](#disable-wallet-mode)

ZMQ-enabled binaries are compiled with `-DWITH_ZMQ=ON` and require the following dependency:

    sudo apt-get install libzmq3-dev

User-Space, Statically Defined Tracing (USDT) dependencies:

    sudo apt install systemtap-sdt-dev

IPC-enabled binaries are compiled  with `-DENABLE_IPC=ON` and require the following dependencies.
Skip if you do not need IPC functionality.

    sudo apt-get install libcapnp-dev capnproto

GUI dependencies:

Bitcoin Core includes a GUI built with the cross-platform Qt Framework. To compile the GUI, we need to install
the necessary parts of Qt, the libqrencode and pass `-DBUILD_GUI=ON`. Skip if you don't intend to use the GUI.

    sudo apt-get install qt6-base-dev qt6-tools-dev qt6-l10n-tools qt6-tools-dev-tools libgl-dev

For Qt 6.5 and later, the `libxcb-cursor0` package must be installed at runtime.

Additionally, to support Wayland protocol for modern desktop environments:

    sudo apt install qt6-wayland

The GUI will be able to encode addresses in QR codes unless this feature is explicitly disabled. To install libqrencode, run:

    sudo apt-get install libqrencode-dev

Otherwise, if you don't need QR encoding support, use the `-DWITH_QRENCODE=OFF` option to disable this feature in order to compile the GUI.


### Fedora

#### Dependency Build Instructions

Build requirements:

    sudo dnf install gcc-c++ cmake make python3

Now, you can either build from self-compiled [depends](#dependencies) or install the required dependencies:

    sudo dnf install libevent-devel boost-devel

SQLite is required for the wallet:

    sudo dnf install sqlite-devel

To build Bitcoin Core without the wallet, see [*Disable-wallet mode*](#disable-wallet-mode)

ZMQ-enabled binaries are compiled with `-DWITH_ZMQ=ON` and require the following dependency:

    sudo dnf install zeromq-devel

User-Space, Statically Defined Tracing (USDT) dependencies:

    sudo dnf install systemtap-sdt-devel

IPC-enabled binaries are compiled with `-DENABLE_IPC=ON` and require the following dependency.
Skip if you do not need IPC functionality.

    sudo dnf install capnproto

GUI dependencies:

Bitcoin Core includes a GUI built with the cross-platform Qt Framework. To compile the GUI, we need to install
the necessary parts of Qt, the libqrencode and pass `-DBUILD_GUI=ON`. Skip if you don't intend to use the GUI.

    sudo dnf install qt6-qtbase-devel qt6-qttools-devel

For Qt 6.5 and later, the `xcb-util-cursor` package must be installed at runtime.

Additionally, to support Wayland protocol for modern desktop environments:

    sudo dnf install qt6-qtwayland

The GUI will be able to encode addresses in QR codes unless this feature is explicitly disabled. To install libqrencode, run:

    sudo dnf install qrencode-devel

Otherwise, if you don't need QR encoding support, use the `-DWITH_QRENCODE=OFF` option to disable this feature in order to compile the GUI.

### Alpine

#### Dependency Build Instructions

Build requirements:

    apk add build-base cmake linux-headers pkgconf python3

Now, you can either build from self-compiled [depends](#dependencies) or install the required dependencies:

    apk add libevent-dev boost-dev

SQLite is required for the wallet:

    apk add sqlite-dev

To build Bitcoin Core without the wallet, see [*Disable-wallet mode*](#disable-wallet-mode)

ZMQ dependencies (provides ZMQ API):

    apk add zeromq-dev

User-Space, Statically Defined Tracing (USDT) is not supported or tested on Alpine Linux at this time.

GUI dependencies:

Bitcoin Core includes a GUI built with the cross-platform Qt Framework. To compile the GUI, we need to install
the necessary parts of Qt, the libqrencode and pass `-DBUILD_GUI=ON`. Skip if you don't intend to use the GUI.

    apk add qt6-qtbase-dev  qt6-qttools-dev

For Qt 6.5 and later, the `xcb-util-cursor` package must be installed at runtime.

The GUI will be able to encode addresses in QR codes unless this feature is explicitly disabled. To install libqrencode, run:

    apk add libqrencode-dev

Otherwise, if you don't need QR encoding support, use the `-DWITH_QRENCODE=OFF` option to disable this feature in order to compile the GUI.

## Dependencies

See [dependencies.md](dependencies.md) for a complete overview, and
[depends](/depends/README.md) on how to compile them yourself, if you wish to
not use the packages of your Linux distribution.

Disable-wallet mode
--------------------
When the intention is to only run a P2P node, without a wallet, Bitcoin Core can
be compiled in disable-wallet mode with:

    cmake -B build -DENABLE_WALLET=OFF

In this case there is no dependency on SQLite.

Mining is also possible in disable-wallet mode using the `getblocktemplate` RPC call.

Setup and Build Example: Arch Linux
-----------------------------------
This example lists the steps necessary to setup and build a command line only distribution of the latest changes on Arch Linux:

    pacman --sync --needed cmake boost gcc git libevent make python sqlite
    git clone https://github.com/bitcoin/bitcoin.git
    cd bitcoin/
    cmake -B build
    cmake --build build
    ctest --test-dir build
    ./build/bin/bitcoind
    ./build/bin/bitcoin help

