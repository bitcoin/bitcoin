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

Build requirements for the latest Debian "stable" release, or the latest Ubuntu LTS release:

    sudo apt-get install build-essential cmake pkgconf python3

For Debian "oldstable", or earlier Ubuntu LTS releases, you may need to pick a
later compiler version, according to the [dependencies](/doc/dependencies.md)
documentation.

Now, you can either build from self-compiled [depends](#dependencies) or install the required dependencies:

    sudo apt-get install libevent-dev libboost-dev

Cap'n Proto is needed for IPC functionality (see [multiprocess.md](multiprocess.md)):

    sudo apt-get install libcapnp-dev capnproto

Compile with `-DENABLE_IPC=OFF` if you do not need IPC functionality.

ZMQ-enabled binaries are compiled with `-DWITH_ZMQ=ON` and require the following dependency:

    sudo apt-get install libzmq3-dev

User-Space, Statically Defined Tracing (USDT) dependencies:

    sudo apt install systemtap-sdt-dev


### Fedora

#### Dependency Build Instructions

Build requirements:

    sudo dnf install gcc-c++ cmake make python3

Now, you can either build from self-compiled [depends](#dependencies) or install the required dependencies:

    sudo dnf install libevent-devel boost-devel

ZMQ-enabled binaries are compiled with `-DWITH_ZMQ=ON` and require the following dependency:

    sudo dnf install zeromq-devel

User-Space, Statically Defined Tracing (USDT) dependencies:

    sudo dnf install systemtap-sdt-devel

Cap'n Proto is needed for IPC functionality (see [multiprocess.md](multiprocess.md)):

    sudo dnf install capnproto capnproto-devel

Compile with `-DENABLE_IPC=OFF` if you do not need IPC functionality.


### Alpine

#### Dependency Build Instructions

Build requirements:

    apk add build-base cmake linux-headers pkgconf python3

Now, you can either build from self-compiled [depends](#dependencies) or install the required dependencies:

    apk add libevent-dev boost-dev

Cap'n Proto is needed for IPC functionality (see [multiprocess.md](multiprocess.md)):

    apk add capnproto capnproto-dev

Compile with `-DENABLE_IPC=OFF` if you do not need IPC functionality.

ZMQ dependencies (provides ZMQ API):

    apk add zeromq-dev

User-Space, Statically Defined Tracing (USDT) is not supported or tested on Alpine Linux at this time.

## Dependencies

See [dependencies.md](dependencies.md) for a complete overview, and
[depends](/depends/README.md) on how to compile them yourself, if you wish to
not use the packages of your Linux distribution.

Setup and Build Example: Arch Linux
-----------------------------------
This example lists the steps necessary to setup and build a command line only distribution of the latest changes on Arch Linux:

    pacman --sync --needed capnproto cmake boost gcc git libevent make python
    git clone https://github.com/bitcoin/bitcoin.git
    cd bitcoin/
    cmake -B build
    cmake --build build
    ctest --test-dir build
    ./build/bin/bitcoind
    ./build/bin/bitcoin help

