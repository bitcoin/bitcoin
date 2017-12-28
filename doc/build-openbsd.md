OpenBSD build guide
======================
This guide describes how to build Bitcoin Core on OpenBSD 6.2.


Preparation
-------------

Run the following as root to install the base dependencies for building:

```bash
pkg_add git gmake libevent libtool
pkg_add autoconf # (select highest version, e.g. 2.69)
pkg_add automake # (select highest version, e.g. 1.15)
pkg_add python # (select highest version, e.g. 3.6)
pkg_add boost

git clone https://github.com/bitcoin/bitcoin.git
```

Optional: To build with Qt 5 support, you also need qt5 and protobuf:
```bash
pkg_add protobuf
pkg_add qt5
```

See [dependencies.md](dependencies.md) for a complete overview.


### Building BerkeleyDB

BerkeleyDB is only necessary for the wallet functionality. To skip this,
pass `--disable-wallet` to `./configure`.

It is recommended to use Berkeley DB 4.8. The Berkley DB library available via the OpenBSD packages is unfortunately too old.
You can use [the installation script included
in contrib/](contrib/install_db4.sh) to build it yourself:

```shell
./contrib/install_db4.sh `pwd` CC=cc CXX=c++
```

from the root of the repository. Then set `BDB_PREFIX` for the next section:

```shell
export BDB_PREFIX="$PWD/db4"
```

### Building Bitcoin Core

**Important**: use `gmake`, not `make`. The non-GNU `make` will exit with a horrible error.

Preparation:
```bash
export AUTOCONF_VERSION=2.69 # replace this with the autoconf version that you installed
export AUTOMAKE_VERSION=1.15 # replace this with the automake version that you installed
./autogen.sh
```
Make sure `BDB_PREFIX` is set to the appropriate path from the above steps.

To configure with wallet and Qt 5:
```bash
./configure --with-gui=qt5 CC=cc CXX=c++ \
    LDFLAGS="-L${BDB_PREFIX}/lib -L/usr/X11R6/lib" \
    CPPFLAGS="-I${BDB_PREFIX}/include -I/usr/X11R6/include"
```

To configure with wallet:
```bash
./configure --with-gui=no CC=cc CXX=c++ \
    LDFLAGS="-L${BDB_PREFIX}/lib" CPPFLAGS="-I${BDB_PREFIX}/include"
```

To configure without wallet:
```bash
./configure --disable-wallet --with-gui=no CC=cc CXX=c++
```

Build and run the tests:
```bash
gmake # use -jX here for parallelism
gmake check
gmake install
```
