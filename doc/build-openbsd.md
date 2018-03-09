OpenBSD build guide
======================
(updated for OpenBSD 6.1)

This guide describes how to build bitcoind and command-line utilities on OpenBSD.

As OpenBSD is most common as a server OS, we will not bother with the GUI.

Preparation
-------------

Run the following as root to install the base dependencies for building:

```bash
pkg_add gmake libtool libevent
pkg_add autoconf # (select highest version, e.g. 2.69)
pkg_add automake # (select highest version, e.g. 1.15)
pkg_add python # (select highest version, e.g. 3.5)
```

The default C++ compiler that comes with OpenBSD 5.9 is g++ 4.2. This version is old (from 2007), and is not able to compile the current version of Bitcoin Core, primarily as it has no C++11 support, but even before there were issues. So here we will be installing a newer compiler.

GCC
-------

You can install a newer version of gcc with:

```bash
pkg_add g++ # (select newest 4.x version, e.g. 4.9.3)
```

This compiler will not overwrite the system compiler, it will be installed as `egcc` and `eg++` in `/usr/local/bin`.

### Building boost

Do not use `pkg_add boost`! The boost version installed thus is compiled using the `g++` compiler not `eg++`, which will result in a conflict between `/usr/local/lib/libestdc++.so.XX.0` and `/usr/lib/libstdc++.so.XX.0`, resulting in a test crash:

    test_bitcoin:/usr/lib/libstdc++.so.57.0: /usr/local/lib/libestdc++.so.17.0 : WARNING: symbol(_ZN11__gnu_debug17_S_debug_me ssagesE) size mismatch, relink your program
    ...
    Segmentation fault (core dumped)

This makes it necessary to build boost, or at least the parts used by Bitcoin Core, manually:

```
# Pick some path to install boost to, here we create a directory within the bitcoin directory
BITCOIN_ROOT=$(pwd)
BOOST_PREFIX="${BITCOIN_ROOT}/boost"
mkdir -p $BOOST_PREFIX

# Fetch the source and verify that it is not tampered with
curl -o boost_1_64_0.tar.bz2 https://netcologne.dl.sourceforge.net/project/boost/boost/1.64.0/boost_1_64_0.tar.bz2
echo '7bcc5caace97baa948931d712ea5f37038dbb1c5d89b43ad4def4ed7cb683332 boost_1_64_0.tar.bz2' | sha256 -c
# MUST output: (SHA256) boost_1_64_0.tar.bz2: OK
tar -xjf boost_1_64_0.tar.bz2

# Boost 1.64 needs one small patch for OpenBSD
cd boost_1_64_0
# Also here: https://gist.githubusercontent.com/laanwj/bf359281dc319b8ff2e1/raw/92250de8404b97bb99d72ab898f4a8cb35ae1ea3/patch-boost_test_impl_execution_monitor_ipp.patch
patch -p0 < /usr/ports/devel/boost/patches/patch-boost_test_impl_execution_monitor_ipp

# Build w/ minimum configuration necessary for bitcoin
echo 'using gcc : : eg++ : <cxxflags>"-fvisibility=hidden -fPIC" <linkflags>"" <archiver>"ar" <striper>"strip"  <ranlib>"ranlib" <rc>"" : ;' > user-config.jam
config_opts="runtime-link=shared threadapi=pthread threading=multi link=static variant=release --layout=tagged --build-type=complete --user-config=user-config.jam -sNO_BZIP2=1"
./bootstrap.sh --without-icu --with-libraries=chrono,filesystem,program_options,system,thread,test
./b2 -d2 -j2 -d1 ${config_opts} --prefix=${BOOST_PREFIX} stage
./b2 -d0 -j4 ${config_opts} --prefix=${BOOST_PREFIX} install
```

### Building BerkeleyDB

BerkeleyDB is only necessary for the wallet functionality. To skip this, pass `--disable-wallet` to `./configure`.

See "Berkeley DB" in [build_unix.md](build_unix.md) for instructions on how to build BerkeleyDB 4.8.
You cannot use the BerkeleyDB library from ports, for the same reason as boost above (g++/libstd++ incompatibility).

```bash
# Pick some path to install BDB to, here we create a directory within the bitcoin directory
BITCOIN_ROOT=$(pwd)
BDB_PREFIX="${BITCOIN_ROOT}/db4"
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

This is, unfortunately, no longer enough to compile some `.cpp` files in the project,
at least with gcc 4.9.3 (see issue https://github.com/bitcoin/bitcoin/issues/6658).
If your user is in the `staff` group the limit can be raised with:

    ulimit -d 3000000

The change will only affect the current shell and processes spawned by it. To
make the change system-wide, change `datasize-cur` and `datasize-max` in
`/etc/login.conf`, and reboot.

### Building Bitcoin Core

**Important**: use `gmake`, not `make`. The non-GNU `make` will exit with a horrible error.

Preparation:
```bash
export AUTOCONF_VERSION=2.69 # replace this with the autoconf version that you installed
export AUTOMAKE_VERSION=1.15 # replace this with the automake version that you installed
./autogen.sh
```
Make sure `BDB_PREFIX` and `BOOST_PREFIX` are set to the appropriate paths from the above steps.

To configure with wallet:
```bash
./configure --with-gui=no --with-boost=$BOOST_PREFIX \
    CC=egcc CXX=eg++ CPP=ecpp \
    BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include"
```

To configure without wallet:
```bash
./configure --disable-wallet --with-gui=no --with-boost=$BOOST_PREFIX \
    CC=egcc CXX=eg++ CPP=ecpp
```

Build and run the tests:
```bash
gmake # can use -jX here for parallelism
gmake check
```

Clang (not currently working)
------------------------------

WARNING: This is outdated, needs to be updated for OpenBSD 6.0 and re-tried.

Using a newer g++ results in linking the new code to a new libstdc++.
Libraries built with the old g++, will still import the old library.
This gives conflicts, necessitating rebuild of all C++ dependencies of the application.

With clang this can - at least theoretically - be avoided because it uses the
base system's libstdc++.

```bash
pkg_add llvm boost
```

```bash
./configure --disable-wallet --with-gui=no CC=clang CXX=clang++
gmake
```

However, this does not appear to work. Compilation succeeds, but link fails
with many 'local symbol discarded' errors:

    local symbol 150: discarded in section `.text._ZN10tinyformat6detail14FormatIterator6finishEv' from libbitcoin_util.a(libbitcoin_util_a-random.o)
    local symbol 151: discarded in section `.text._ZN10tinyformat6detail14FormatIterator21streamStateFromFormatERSoRjPKcii' from libbitcoin_util.a(libbitcoin_util_a-random.o)
    local symbol 152: discarded in section `.text._ZN10tinyformat6detail12convertToIntIA13_cLb0EE6invokeERA13_Kc' from libbitcoin_util.a(libbitcoin_util_a-random.o)

According to similar reported errors this is a binutils (ld) issue in 2.15, the
version installed by OpenBSD 5.7:

- http://openbsd-archive.7691.n7.nabble.com/UPDATE-cppcheck-1-65-td248900.html
- https://llvm.org/bugs/show_bug.cgi?id=9758

There is no known workaround for this.
