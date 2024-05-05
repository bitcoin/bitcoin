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

Berkeley DB is only required for the legacy wallet. Ubuntu and Debian have their own `libdb-dev` and `libdb++-dev` packages,
but these will install Berkeley DB 5.3 or later. This will break binary wallet compatibility with the distributed
executables, which are based on BerkeleyDB 4.8. Otherwise, you can build Berkeley DB [yourself](#berkeley-db).

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

Berkeley DB is only required for the legacy wallet. Fedora releases have only `libdb-devel` and `libdb-cxx-devel` packages, but these will install
Berkeley DB 5.3 or later. This will break binary wallet compatibility with the distributed executables, which
are based on Berkeley DB 4.8. Otherwise, you can build Berkeley DB [yourself](#berkeley-db).

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

### Berkeley DB

The legacy wallet uses Berkeley DB. To ensure backwards compatibility it is
recommended to use Berkeley DB 4.8. If you have to build it yourself, and don't
want to use any other libraries built in depends, you can do:
```bash
make -C depends NO_BOOST=1 NO_LIBEVENT=1 NO_QT=1 NO_SQLITE=1 NO_UPNP=1 NO_ZMQ=1 NO_USDT=1
...
to: /path/to/bitcoin/depends/x86_64-pc-linux-gnu
```
and configure using the following:
```bash
export BDB_PREFIX="/path/to/bitcoin/depends/x86_64-pc-linux-gnu"

cmake -B build -DBerkeleyDB_INCLUDE_DIR:PATH="${BDB_PREFIX}/include" -DWITH_BDB=ON
```

**Note**: Make sure that `BDB_PREFIX` is an absolute path.

**Note**: You only need Berkeley DB if the legacy wallet is enabled (see [*Disable-wallet mode*](#disable-wallet-mode)).

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
