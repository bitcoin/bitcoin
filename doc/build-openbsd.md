OpenBSD build guide
======================
(updated for OpenBSD 6.2)

This guide describes how to build litecoind and command-line utilities on OpenBSD.

OpenBSD is most commonly used as a server OS, so this guide does not contain instructions for building the GUI.

Preparation
-------------

Run the following as root to install the base dependencies for building:

```bash
pkg_add git gmake libevent libtool
pkg_add autoconf # (select highest version, e.g. 2.69)
pkg_add automake # (select highest version, e.g. 1.15)
pkg_add python # (select highest version, e.g. 3.6)
pkg_add boost

git clone https://github.com/litecoin-project/litecoin.git
```

See [dependencies.md](dependencies.md) for a complete overview.

GCC
-------

The default C++ compiler that comes with OpenBSD 6.2 is g++ 4.2.1. This version is old (from 2007), and is not able to compile the current version of Litecoin Core because it has no C++11 support. We'll install a newer version of GCC:

```bash
 pkg_add g++
 ```

 This compiler will not overwrite the system compiler, it will be installed as `egcc` and `eg++` in `/usr/local/bin`.

### Building BerkeleyDB

BerkeleyDB is only necessary for the wallet functionality. To skip this, pass `--disable-wallet` to `./configure`.

See "Berkeley DB" in [build-unix.md](build-unix.md#berkeley-db) for instructions on how to build BerkeleyDB 4.8.
You cannot use the BerkeleyDB library from ports, for the same reason as boost above (g++/libstd++ incompatibility).

```bash
# Pick some path to install BDB to, here we create a directory within the litecoin directory
LITECOIN_ROOT=$(pwd)
BDB_PREFIX="${LITECOIN_ROOT}/db4"
mkdir -p $BDB_PREFIX

# Fetch the source and verify that it is not tampered with
curl -o db-4.8.30.NC.tar.gz 'http://download.oracle.com/berkeley-db/db-4.8.30.NC.tar.gz'
echo '12edc0df75bf9abd7f82f821795bcee50f42cb2e5f76a6a281b85732798364ef  db-4.8.30.NC.tar.gz' | sha256 -c
# MUST output: (SHA256) db-4.8.30.NC.tar.gz: OK
tar -xzf db-4.8.30.NC.tar.gz

# Build the library and install to specified prefix
cd db-4.8.30.NC/build_unix/
#  Note: Do a static build so that it can be embedded into the executable, instead of having to find a .so at runtime
../dist/configure --enable-cxx --disable-shared --with-pic --prefix=$BDB_PREFIX CC=egcc CXX=eg++ CPP=ecpp
make install # do NOT use -jX, this is broken
```

### Resource limits

The standard ulimit restrictions in OpenBSD are very strict:

    data(kbytes)         1572864

This, unfortunately, may no longer be enough to compile some `.cpp` files in the project,
at least with GCC 4.9.4 (see issue [#6658](https://github.com/bitcoin/bitcoin/issues/6658)).
If your user is in the `staff` group the limit can be raised with:

    ulimit -d 3000000

The change will only affect the current shell and processes spawned by it. To
make the change system-wide, change `datasize-cur` and `datasize-max` in
`/etc/login.conf`, and reboot.

### Building Litecoin Core

**Important**: use `gmake`, not `make`. The non-GNU `make` will exit with a horrible error.

Preparation:
```bash
export AUTOCONF_VERSION=2.69 # replace this with the autoconf version that you installed
export AUTOMAKE_VERSION=1.15 # replace this with the automake version that you installed
./autogen.sh
```
Make sure `BDB_PREFIX` is set to the appropriate path from the above steps.

To configure with wallet:
```bash
./configure --with-gui=no CC=egcc CXX=eg++ CPP=ecpp \
    BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include"
```

To configure without wallet:
```bash
./configure --disable-wallet --with-gui=no CC=egcc CXX=eg++ CPP=ecpp
```

Build and run the tests:
```bash
gmake # use -jX here for parallelism
gmake check
```

Clang
------------------------------

```bash
pkg_add llvm

./configure --disable-wallet --with-gui=no CC=clang CXX=clang++
gmake # use -jX here for parallelism
gmake check
```
