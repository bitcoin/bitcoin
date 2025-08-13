UNIX BUILD NOTES
====================
Some notes on how to build Dash Core in Unix.

(For BSD specific instructions, see `build-*bsd.md` in this directory.)

Note
---------------------
Always use absolute paths to configure and compile Dash Core and the dependencies.
For example, when specifying the path of the dependency:

```sh
../dist/configure --enable-cxx --disable-shared --with-pic --prefix=$BDB_PREFIX
```

Here `BDB_PREFIX` must be an absolute path - it is defined using $(pwd) which ensures
the usage of the absolute path.

To Build
---------------------

```sh
./autogen.sh
./configure
make # use "-j N" for N parallel jobs
make install # optional
```

This will build dash-qt as well, if the dependencies are met.

See [dependencies.md](dependencies.md) for a complete overview.

Memory Requirements
--------------------

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

Now, you can either build from self-compiled [depends](/depends/README.md) or install the required dependencies:

```sh
sudo apt-get install libevent-dev libboost-dev
```

SQLite is required for the descriptor wallet:

```sh
sudo apt-get install libsqlite3-dev
```

Berkeley DB is required for the legacy wallet. Ubuntu and Debian have their own `libdb-dev` and `libdb++-dev` packages,
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

Now, you can either build from self-compiled [depends](/depends/README.md) or install the required dependencies:

```sh
sudo dnf install libevent-devel boost-devel
```

SQLite is required for the descriptor wallet:

```sh
sudo dnf install sqlite-devel
```

Berkeley DB is required for the legacy wallet:

```sh
sudo dnf install libdb4-devel libdb4-cxx-devel
```

Newer Fedora releases, since Fedora 33, have only `libdb-devel` and `libdb-cxx-devel` packages, but these will install
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

Notes
-----
The release is built with GCC and then "strip dashd" to strip the debug
symbols, which reduces the executable size by about 90%.


miniupnpc
---------

[miniupnpc](https://miniupnp.tuxfamily.org) may be used for UPnP port mapping.  It can be downloaded from [here](
https://miniupnp.tuxfamily.org/files/).  UPnP support is compiled in and
turned off by default.

libnatpmp
---------

[libnatpmp](https://miniupnp.tuxfamily.org/libnatpmp.html) may be used for NAT-PMP port mapping. It can be downloaded
from [here](https://miniupnp.tuxfamily.org/files/). NAT-PMP support is compiled in and
turned off by default.

Berkeley DB
-----------

The legacy wallet uses Berkeley DB. To ensure backwards compatibility it is
recommended to use Berkeley DB 4.8. If you have to build it yourself, you can
use [the installation script included in contrib/](/contrib/install_db4.sh)
like so:

```sh
./contrib/install_db4.sh `pwd`
```

from the root of the repository.

Otherwise, you can build Dash Core from self-compiled [depends](/depends/README.md).

**Note**: You only need Berkeley DB if the wallet is enabled (see [*Disable-wallet mode*](#disable-wallet-mode)).

Disable-wallet mode
--------------------
When the intention is to run only a P2P node without a wallet, Dash Core may be compiled in
disable-wallet mode with:

    ./configure --disable-wallet

In this case there is no dependency on Berkeley DB 4.8 and SQLite.

Mining is also possible in disable-wallet mode using the `getblocktemplate` RPC call.

Additional Configure Flags
--------------------------
A list of additional configure flags can be displayed with:

```sh
./configure --help
```


Setup and Build Example: Arch Linux
-----------------------------------
This example lists the steps necessary to setup and build a command line only, non-wallet distribution of the latest changes on Arch Linux:

```sh
pacman -S git base-devel boost libevent python
git clone https://github.com/dashpay/dash.git
cd dash/
./autogen.sh
./configure --disable-wallet --without-gui --without-miniupnpc
make check
```

Note:
Enabling wallet support requires either compiling against a Berkeley DB newer than 4.8 (package `db`) using `--with-incompatible-bdb`,
or building and depending on a local version of Berkeley DB 4.8. The readily available Arch Linux packages are currently built using
`--with-incompatible-bdb` according to the [PKGBUILD](https://github.com/archlinux/svntogit-community/blob/packages/bitcoin/trunk/PKGBUILD).
As mentioned above, when maintaining portability of the wallet between the standard Dash Core distributions and independently built
node software is desired, Berkeley DB 4.8 must be used.
