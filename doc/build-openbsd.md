# OpenBSD build guide

This guide describes how to build Bitcoin Core on OpenBSD.
It has been tested on OpenBSD-current/amd64 as of Dec 25, 2017.

## Preparations

### The compiler

On some platforms, the system compiler that comes with OpenBSD is clang 5.0.0,
which compiles C++11 just fine. On these system, `c++` is `clang++`

```shell
$ ls -li /usr/bin/c++ /usr/bin/clang++
32869 -r-xr-xr-x  6 root  bin  42331608 Dec 18 18:55 /usr/bin/c++
32869 -r-xr-xr-x  6 root  bin  42331608 Dec 18 18:55 /usr/bin/clang++
```

but `g++` might still be the old GCC:

```shell
$ g++ -v
Reading specs from /usr/lib/gcc-lib/amd64-unknown-openbsd6.2/4.2.1/specs
Target: amd64-unknown-openbsd6.2
Configured with: OpenBSD/amd64 system compiler
Thread model: posix
gcc version 4.2.1 20070719
```

On other platforms, the system compiler is GCC 4.2.1 which is not able to compile
the current version of Bitcoin Core, as it has no C++11 support.
In that case (find out with `c++ -v`), you need to install a newer compiler
with `pkg_add -i g++`. This compiler does not overwrite the system compiler,
it gets installed as `egcc` and `eg++` in `/usr/local/bin`.
Similarly, you can install clang with `pkg_add -i llvm`.

### Dependencies

Run the following as root to install the dependencies:
Note that the OpenBSD package of Berkeley DB is version 4.6,
while Bitcoin Core requires version 4.8 to build the wallet.

```shell
# pkg_add -i autoconf automake libtool  # select highest version
# pkg_add boost libevent                # required libraries
# pkg_add gmake python                  # required build tools
# pkg_add qt5                           # needed for the GUI
# pkg_add git
```

### BerkeleyDB

BerkeleyDB is only needed for the wallet functionality.
This can be disabled with `./configure --disable-wallet`.

It is recommended to use Berkeley DB 4.8.
As mentioned above, the OpenBSD ports only have version 4.6
If you have to build it yourself, you can use
[the installation script included in contrib/](contrib/install_db4.sh):

```
./contrib/install_db4.sh `pwd` CC=egcc CXX=eg++ CPP=ecpp
```

### The source

Clone the Bitcoin Core github repository:

```
$ mkdir -p ~/src && cd ~/src
$ git clone https://github.com/bitcoin/bitcoin.git
```

## Configure Bitcoin Core

As a start, this configures Bitcoin Core without the wallet and without GUI.
Make sure to point `CXX` and friends to your C++11 capable compiler.
Note that you might need to specify `CC=cc CXX=c++` even on clang systems,
because `./configure` will autodetect `gcc` and `g++` (which is the old 4.2.1)
before `cc` and `c++` (which is clang).

```
env AUTOCONF_VERSION=2.69 AUTOMAKE_VERSION=1.15 ./autogen.sh
./configure --prefix=$HOME --with-mandir=$HOME/man --disable-ccache --disable-silent-rules --disable-upnp-default --enable-tests --disable-bench --disable-hardening --disable-reduce-exports --disable-glibc-back-compat --disable-experimental-asm --disable-zmq --enable-man --disable-debug --enable-werror --enable-largefile --without-miniupnpc --without-system-univalue --with-utils --with-libs --with-daemon --with-gui=no --disable-wallet CC=egcc CXX=eg++ CPP=ecpp
```

/*
FIXME muniupnpc, protobuf, and zmq do not seem to be required.
We explictily disable --without-miniupnpc and --disable-zmq  for now.
There is no --option for protobuf; it gets picked up when present.
*/

To configure with wallet, replace `--enable-wallet=no` with `--enable-wallet`
and add the following to the `./configure` line:

```
BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include"
```

Edit `BDB_PREFIX` to the path where your BDB 4.8 is installed.

/*
FIXME: doesn't configure pick the db package (4.6) anyway,
if present in /usr/local, which comes first in both -I and -L?
*/

## Build Bitcoin Core

The Makefiles used by Bitcoin Core need to be processed with GNU make,
not with the standard BSD make. Make sure to use `gmake`, not `make`.

```
gmake		# build Bitcoin Core
gmake check	# run the tests
gmake install
```
