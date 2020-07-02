NetBSD build guide
======================
(updated for NetBSD 9.0)

This guide describes how to build bitcoind and command-line utilities on NetBSD.

This guide does not contain instructions for building the GUI.

Preparation
-------------

You will need the following packages, which can be installed via pkgsrc or pkgin:

```
devel/autoconf
devel/automake
devel/boost-libs
devel/git
devel/gmake
devel/libevent
devel/libtool
devel/pkg-config
lang/python37

git clone https://github.com/bitcoin/bitcoin.git
```

See [dependencies.md](dependencies.md) for a complete overview.

### Building BerkeleyDB

BerkeleyDB is only necessary for the wallet functionality. To skip this, pass
`--disable-wallet` to `./configure` and skip to the next section.

It is recommended to use Berkeley DB 4.8. For maximum compatibility, use the
bundled [installation script included in contrib/](/contrib/install_db4.sh) like so:

```bash
./contrib/install_db4.sh `pwd`
```

from the root of the repository. Then set `BDB_PREFIX` for the next section:

```bash
export BDB_PREFIX="$PWD/db4"
```

### Building Bitcoin Core

**Important**: Use `gmake` (the non-GNU `make` will exit with an error).

With wallet:
```bash
./autogen.sh

MAKE="gmake" \
CPPFLAGS="-I/usr/pkg/include" LDFLAGS="-L/usr/pkg/lib -Wl,-R/usr/pkg/lib" \
BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include" \
./configure --with-gui=no --with-boost=/usr/pkg
```

Without wallet:
```bash
./autogen.sh

MAKE="gmake" \
CPPFLAGS="-I/usr/pkg/include" LDFLAGS="-L/usr/pkg/lib -Wl,-R/usr/pkg/lib" \
BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include" \
./configure --with-gui=no --with-boost=/usr/pkg --disable-wallet
```

Build and run the tests (very important!):
```bash
gmake # use -jX here for parallelism
gmake check
```
