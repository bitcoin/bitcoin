UNIX BUILD NOTES
====================
Some notes on how to build Bitcoin Core in Unix.

(For BSD specific instructions, see `build-*bsd.md` in this directory.)

To Build
---------------------

```bash
cmake -B build
cmake --build build    # use "-j N" for N parallel jobs
cmake --install build  # optional
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

    sudo apt-get install build-essential cmake pkg-config python3

Now, you can either build from self-compiled [depends](#dependencies) or install the required dependencies:

    sudo apt-get install libevent-dev libboost-dev

SQLite is required for the descriptor wallet:

    sudo apt install libsqlite3-dev

To build Bitcoin Core without wallet, see [*Disable-wallet mode*](#disable-wallet-mode)

Optional port mapping library (see: `-DWITH_MINIUPNPC=ON`):

    sudo apt install libminiupnpc-dev

ZMQ dependencies (provides ZMQ API):

    sudo apt-get install libzmq3-dev

User-Space, Statically Defined Tracing (USDT) dependencies:

    sudo apt install systemtap-sdt-dev

GUI dependencies:

Bitcoin Core includes a GUI built with the cross-platform Qt Framework. To compile the GUI, we need to install
the necessary parts of Qt, the libqrencode and pass `-DBUILD_GUI=ON`. Skip if you don't intend to use the GUI.

    sudo apt-get install qtbase5-dev qttools5-dev qttools5-dev-tools

Additionally, to support Wayland protocol for modern desktop environments:

    sudo apt install qtwayland5

The GUI will be able to encode addresses in QR codes unless this feature is explicitly disabled. To install libqrencode, run:

    sudo apt-get install libqrencode-dev

Otherwise, if you don't need QR encoding support, use the `-DWITH_QRENCODE=OFF` option to disable this feature in order to compile the GUI.


### Fedora

#### Dependency Build Instructions

Build requirements:

    sudo dnf install gcc-c++ cmake make python3

Now, you can either build from self-compiled [depends](#dependencies) or install the required dependencies:

    sudo dnf install libevent-devel boost-devel

SQLite is required for the descriptor wallet:

    sudo dnf install sqlite-devel

To build Bitcoin Core without wallet, see [*Disable-wallet mode*](#disable-wallet-mode)

Optional port mapping library (see: `-DWITH_MINIUPNPC=ON`):

    sudo dnf install miniupnpc-devel

ZMQ dependencies (provides ZMQ API):

    sudo dnf install zeromq-devel

User-Space, Statically Defined Tracing (USDT) dependencies:

    sudo dnf install systemtap-sdt-devel

GUI dependencies:

Bitcoin Core includes a GUI built with the cross-platform Qt Framework. To compile the GUI, we need to install
the necessary parts of Qt, the libqrencode and pass `-DBUILD_GUI=ON`. Skip if you don't intend to use the GUI.

    sudo dnf install qt5-qttools-devel qt5-qtbase-devel

Additionally, to support Wayland protocol for modern desktop environments:

    sudo dnf install qt5-qtwayland

The GUI will be able to encode addresses in QR codes unless this feature is explicitly disabled. To install libqrencode, run:

    sudo dnf install qrencode-devel

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

In this case there is no dependency on SQLite or Berkeley DB.

Mining is also possible in disable-wallet mode using the `getblocktemplate` RPC call.

Additional Configure Flags
--------------------------
A list of additional configure flags can be displayed with:

    cmake -B build -LH


Setup and Build Example: Arch Linux
-----------------------------------
This example lists the steps necessary to setup and build a command line only distribution of the latest changes on Arch Linux:

    pacman --sync --needed cmake boost gcc git libevent make pkgconf python sqlite
    git clone https://github.com/bitcoin/bitcoin.git
    cd bitcoin/
    cmake -B build
    cmake --build build
    ctest --test-dir build
    ./build/src/bitcoind

If you intend to work with legacy Berkeley DB wallets, see [Berkeley DB](#berkeley-db) section.
