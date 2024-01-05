# FreeBSD Build Guide

**Updated for FreeBSD [14.0](https://www.freebsd.org/releases/14.0R/announce/)**

This guide describes how to build bitcoind, command-line utilities, and GUI on FreeBSD.

## Preparation

### 1. Install Required Dependencies
Run the following as root to install the base dependencies for building.

```bash
pkg install autoconf automake boost-libs git gmake libevent libtool pkgconf

```

See [dependencies.md](dependencies.md) for a complete overview.

### 2. Clone Bitcoin Repo
Now that `git` and all the required dependencies are installed, let's clone the Bitcoin Core repository to a directory. All build scripts and commands will run from this directory.
``` bash
git clone https://github.com/bitcoin/bitcoin.git
```

### 3. Install Optional Dependencies

#### Wallet Dependencies
It is not necessary to build wallet functionality to run either `bitcoind` or `bitcoin-qt`.

###### Descriptor Wallet Support

`sqlite3` is required to support [descriptor wallets](descriptors.md).
Skip if you don't intend to use descriptor wallets.
``` bash
pkg install sqlite3
```

#### GUI Dependencies
###### Qt5

Bitcoin Core includes a GUI built with the cross-platform Qt Framework. To compile the GUI, we need to install the necessary parts of Qt. Skip if you don't intend to use the GUI.
```bash
pkg install qt5-buildtools qt5-core qt5-gui qt5-linguisttools qt5-testlib qt5-widgets
```

###### libqrencode

The GUI can encode addresses in a QR Code. To build in QR support for the GUI, install `libqrencode`. Skip if not using the GUI or don't want QR code functionality.
```bash
pkg install libqrencode
```
---

#### Notifications
###### ZeroMQ

Bitcoin Core can provide notifications via ZeroMQ. If the package is installed, support will be compiled in.
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

## Building Bitcoin Core

### 1. Configuration

There are many ways to configure Bitcoin Core, here are a few common examples:

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
