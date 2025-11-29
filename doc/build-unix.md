UNIX BUILD NOTES
====================
Some notes on how to build Dash Core in Unix.

(For BSD specific instructions, see `build-*bsd.md` in this directory.)

To Build
---------------------

```sh
./autogen.sh
./configure
make # use "-j N" for N parallel jobs
make install # optional
```

See below for instructions on how to [install the dependencies on popular Linux
distributions](#linux-distribution-specific-instructions), or the
[dependencies](#dependencies) section for a complete overview.

## Memory Requirements

C++ compilers are memory-hungry. It is recommended to have at least 1.5 GB of
memory available when compiling Dash Core. On systems with less, gcc can be
tuned to conserve memory with additional CXXFLAGS:


```sh
./configure CXXFLAGS="--param ggc-min-expand=1 --param ggc-min-heapsize=32768"
```


## Linux Distribution Specific Instructions

### Ubuntu & Debian

#### Dependency Build Instructions

Build requirements:

```sh
sudo apt-get install build-essential libtool autotools-dev automake pkg-config bsdmainutils bison python3
```

Now, you can either build from self-compiled [depends](#dependencies) or install the required dependencies:

```sh
sudo apt-get install libbacktrace-dev libevent-dev libboost-dev
```

Note: libbacktrace-dev is available in Debian 13 (Trixie) and Ubuntu 25.04+.
For older releases, use the /depends/README.md which includes all required libraries.

SQLite is required for the descriptor wallet:

```sh
sudo apt-get install libsqlite3-dev
```

Berkeley DB is only required for the legacy wallet. Ubuntu and Debian have their own `libdb-dev` and `libdb++-dev` packages,
but these will install Berkeley DB 5.1 or later. This will break binary wallet compatibility with the distributed
executables, which are based on BerkeleyDB 4.8. If you do not care about wallet compatibility, pass
`--with-incompatible-bdb` to configure. Otherwise, you can build Berkeley DB [yourself](#berkeley-db).

To build Dash Core without wallet, see [*Disable-wallet mode*](#disable-wallet-mode)

Optional port mapping libraries (see: `--with-miniupnpc` and `--with-natpmp`):

```sh
sudo apt-get install libminiupnpc-dev libnatpmp-dev
```

ZMQ dependencies (provides ZMQ API):

```sh
sudo apt-get install libzmq3-dev
```

GMP dependencies (provides platform-optimized routines):

```sh
sudo apt-get install libgmp-dev
```

User-Space, Statically Defined Tracing (USDT) dependencies:

```sh
sudo apt install systemtap-sdt-dev
```

GUI dependencies:

If you want to build dash-qt, make sure that the required packages for Qt development
are installed. Qt 5 is necessary to build the GUI.
To build without GUI pass `--without-gui`.

To build with Qt 5 you need the following:

```sh
sudo apt-get install qtbase5-dev qttools5-dev qttools5-dev-tools
```

Additionally, to support Wayland protocol for modern desktop environments:

```sh
sudo apt-get install qtwayland5
```

libqrencode (optional) can be installed with:

```sh
sudo apt-get install libqrencode-dev
```

Once these are installed, they will be found by configure and a dash-qt executable will be
built by default.


### Fedora

#### Dependency Build Instructions

Build requirements:

```sh
sudo dnf install gcc-c++ libtool make autoconf automake python3
```

Now, you can either build from self-compiled [depends](#dependencies) or install the required dependencies:

```sh
sudo dnf install libevent-devel boost-devel
```

Note: Fedora repositories do not include libbacktrace. To build Dash Core without stack trace support, configure with `--disable-stacktraces`.

SQLite is required for the descriptor wallet:

```sh
sudo dnf install sqlite-devel
```

Berkeley DB is required for the legacy wallet:

```sh
sudo dnf install libdb4-devel libdb4-cxx-devel
```

Berkeley DB is only required for the legacy wallet. Newer Fedora releases have only `libdb-devel` and `libdb-cxx-devel` packages, but these will install
Berkeley DB 5.3 or later. This will break binary wallet compatibility with the distributed executables, which
are based on Berkeley DB 4.8. If you do not care about wallet compatibility,
pass `--with-incompatible-bdb` to configure. Otherwise, you can build Berkeley DB [yourself](#berkeley-db).

To build Dash Core without wallet, see [*Disable-wallet mode*](#disable-wallet-mode)

Optional port mapping libraries (see: `--with-miniupnpc` and `--with-natpmp`):

```sh
sudo dnf install miniupnpc-devel libnatpmp-devel
```

ZMQ dependencies (provides ZMQ API):

```sh
sudo dnf install zeromq-devel
```

GMP dependencies (provides platform-optimized routines):

```sh
sudo dnf install gmp-devel
```

User-Space, Statically Defined Tracing (USDT) dependencies:

```sh
sudo dnf install systemtap-sdt-devel
```

GUI dependencies:

If you want to build dash-qt, make sure that the required packages for Qt development
are installed. Qt 5 is necessary to build the GUI.
To build without GUI pass `--without-gui`.

To build with Qt 5 you need the following:

```sh
sudo dnf install qt5-qttools-devel qt5-qtbase-devel
```

Additionally, to support Wayland protocol for modern desktop environments:

```sh
sudo dnf install qt5-qtwayland
```

libqrencode (optional) can be installed with:

```sh
sudo dnf install qrencode-devel
```

Once these are installed, they will be found by configure and a dash-qt executable will be
built by default.

## Dependencies

See [dependencies.md](dependencies.md) for a complete overview, and
[depends](/depends/README.md) on how to compile them yourself, if you wish to
not use the packages of your Linux distribution.

### Berkeley DB

The legacy wallet uses Berkeley DB. To ensure backwards compatibility it is
recommended to use Berkeley DB 4.8. If you have to build it yourself, and don't
want to use any other libraries built in depends, you can do:
```bash
make -C depends NO_BOOST=1 NO_LIBEVENT=1 NO_QT=1 NO_SQLITE=1 NO_NATPMP=1 NO_UPNP=1 NO_ZMQ=1 NO_USDT=1
...
to: /path/to/dash/depends/x86_64-pc-linux-gnu
```
and configure using the following:
```bash
export BDB_PREFIX="/path/to/dash/depends/x86_64-pc-linux-gnu"

./configure \
    BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" \
    BDB_CFLAGS="-I${BDB_PREFIX}/include"
```

**Note**: Make sure that `BDB_PREFIX` is an absolute path.

**Note**: You only need Berkeley DB if the legacy wallet is enabled (see [*Disable-wallet mode*](#disable-wallet-mode)).

Disable-wallet mode
--------------------
When the intention is to only run a P2P node, without a wallet, Dash Core can
be compiled in disable-wallet mode with:

    ./configure --disable-wallet

In this case there is no dependency on SQLite or Berkeley DB.

Mining is also possible in disable-wallet mode using the `getblocktemplate` RPC call.

Additional Configure Flags
--------------------------
A list of additional configure flags can be displayed with:

```sh
./configure --help
```


Setup and Build Example: Arch Linux
-----------------------------------
This example lists the steps necessary to setup and build a command line only distribution of the latest changes on Arch Linux:

```sh
pacman --sync --needed autoconf automake boost gcc git libbacktrace libevent libtool make pkgconf python sqlite
git clone https://github.com/dashpay/dash.git
cd dash/
./autogen.sh
./configure
make check
./src/dashd
```

If you intend to work with legacy Berkeley DB wallets, see [Berkeley DB](#berkeley-db) section.
