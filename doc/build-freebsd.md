# FreeBSD Build Guide

**Updated for FreeBSD [12.3](https://www.freebsd.org/releases/12.3R/announce/)**

This guide describes how to build syscoind, command-line utilities, and GUI on FreeBSD.

## Preparation

### 1. Install Required Dependencies
Run the following as root to install the base dependencies for building.

```bash
pkg install autoconf automake boost-libs git gmake libevent libtool pkgconf

```

See [dependencies.md](dependencies.md) for a complete overview.

### 2. Clone Syscoin Repo
Now that `git` and all the required dependencies are installed, let's clone the Syscoin Core repository to a directory. All build scripts and commands will run from this directory.
``` bash
git clone https://github.com/syscoin/syscoin.git
```

### 3. Install Optional Dependencies

#### Wallet Dependencies
It is not necessary to build wallet functionality to run either `syscoind` or `syscoin-qt`.

###### Descriptor Wallet Support

`sqlite3` is required to support [descriptor wallets](descriptors.md).
Skip if you don't intend to use descriptor wallets.
``` bash
pkg install sqlite3
```

###### Legacy Wallet Support
BerkeleyDB is only required if legacy wallet support is required.

It is required to use Berkeley DB 4.8. You **cannot** use the BerkeleyDB library
from ports. However, you can build DB 4.8 yourself [using depends](/depends).

```
gmake -C depends NO_BOOST=1 NO_LIBEVENT=1 NO_QT=1 NO_SQLITE=1 NO_NATPMP=1 NO_UPNP=1 NO_ZMQ=1 NO_USDT=1
```

When the build is complete, the Berkeley DB installation location will be displayed:

```
to: /path/to/syscoin/depends/x86_64-unknown-freebsd[release-number]
```

Finally, set `BDB_PREFIX` to this path according to your shell:

```
csh: setenv BDB_PREFIX [path displayed above]
```

```
sh/bash: export BDB_PREFIX=[path displayed above]
```

#### GUI Dependencies
###### Qt5

Syscoin Core includes a GUI built with the cross-platform Qt Framework. To compile the GUI, we need to install `qt5`. Skip if you don't intend to use the GUI.
```bash
pkg install qt5
```
###### libqrencode

The GUI can encode addresses in a QR Code. To build in QR support for the GUI, install `libqrencode`. Skip if not using the GUI or don't want QR code functionality.
```bash
pkg install libqrencode
```
---

#### Notifications
###### ZeroMQ

Syscoin Core can provide notifications via ZeroMQ. If the package is installed, support will be compiled in.
```bash
pkg install libzmq4
```

#### Test Suite Dependencies
There is an included test suite that is useful for testing code changes when developing.
To run the test suite (recommended), you will need to have Python 3 installed:

```bash
pkg install python3 databases/py-sqlite3
```
---

## Building Syscoin Core

### 1. Configuration

There are many ways to configure Syscoin Core, here are a few common examples:

##### Descriptor Wallet and GUI:
This explicitly enables the GUI and disables legacy wallet support, assuming `sqlite` and `qt` are installed.
```bash
./autogen.sh
./configure --without-bdb --with-gui=yes MAKE=gmake
```

##### Descriptor & Legacy Wallet. No GUI:
This enables support for both wallet types and disables the GUI, assuming
`sqlite3` and `db4` are both installed.
```bash
./autogen.sh
./configure --with-gui=no \
    BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" \
    BDB_CFLAGS="-I${BDB_PREFIX}/include" \
    MAKE=gmake
```

##### No Wallet or GUI
``` bash
./autogen.sh
./configure --without-wallet --with-gui=no MAKE=gmake
```

### 2. Compile
**Important**: Use `gmake` (the non-GNU `make` will exit with an error).

```bash
gmake # use "-j N" for N parallel jobs
gmake check # Run tests if Python 3 is available
```
