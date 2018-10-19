OpenBSD build guide
======================
(updated for OpenBSD 6.4)

This guide describes how to build bitcoind and command-line utilities on OpenBSD.

OpenBSD is most commonly used as a server OS, so this guide does not contain instructions for building the GUI.

Preparation
-------------

Run the following as root to install the base dependencies for building:

```bash
pkg_add git gmake libevent libtool boost
pkg_add autoconf # (select highest version, e.g. 2.69)
pkg_add automake # (select highest version, e.g. 1.16)
pkg_add python # (select highest version, e.g. 3.6)

git clone https://github.com/bitcoin/bitcoin.git
```

See [dependencies.md](dependencies.md) for a complete overview.

**Important**: From OpenBSD 6.2 onwards a C++11-supporting clang compiler is
part of the base image, and while building it is necessary to make sure that this
compiler is used and not ancient g++ 4.2.1. This is done by appending
`CC=cc CXX=c++` to configuration commands. Mixing different compilers
within the same executable will result in linker errors.

### Building BerkeleyDB

BerkeleyDB is only necessary for the wallet functionality. To skip this, pass
`--disable-wallet` to `./configure` and skip to the next section.

It is recommended to use Berkeley DB 4.8. You cannot use the BerkeleyDB library
from ports, for the same reason as boost above (g++/libstd++ incompatibility).
If you have to build it yourself, you can use [the installation script included
in contrib/](/contrib/install_db4.sh) like so

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

# Replace this with the autoconf version that you installed. Include only
# the major and minor parts of the version: use "2.69" for "autoconf-2.69p2".
export AUTOCONF_VERSION=2.69

# Replace this with the automake version that you installed. Include only
# the major and minor parts of the version: use "1.16" for "automake-1.16.1".
export AUTOMAKE_VERSION=1.16

./autogen.sh
```
Make sure `BDB_PREFIX` is set to the appropriate path from the above steps.

To configure with wallet:
```bash
./configure --with-gui=no CC=cc CXX=c++ \
    BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include"
```

To configure without wallet:
```bash
./configure --disable-wallet --with-gui=no CC=cc CXX=c++
```

Build and run the tests:
```bash
gmake # use -jX here for parallelism
gmake check
```

Resource limits
-------------------

If the build runs into out-of-memory errors, the instructions in this section
might help.

The standard ulimit restrictions in OpenBSD are very strict:

    data(kbytes)         1572864

This, unfortunately, in some cases not enough to compile some `.cpp` files in the project,
(see issue [#6658](https://github.com/bitcoin/bitcoin/issues/6658)).
If your user is in the `staff` group the limit can be raised with:

    ulimit -d 3000000

The change will only affect the current shell and processes spawned by it. To
make the change system-wide, change `datasize-cur` and `datasize-max` in
`/etc/login.conf`, and reboot.

