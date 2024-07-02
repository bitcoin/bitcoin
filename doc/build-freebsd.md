# FreeBSD Build Guide

**Updated for FreeBSD [14.0](https://www.freebsd.org/releases/14.0R/announce/)**

This guide describes how to build bitcoind, command-line utilities, and GUI on FreeBSD.

## Preparation

### 1. Install Required Dependencies
Run the following as root to install the base dependencies for building.

```bash
pkg install boost-libs cmake git libevent pkgconf
```

See [dependencies.md](dependencies.md) for a complete overview.

### 2. Clone Bitcoin Repo
Now that `git` and all the required dependencies are installed, let's clone the Bitcoin Core repository to a directory. All build scripts and commands will run from this directory.
```bash
git clone https://github.com/bitcoin/bitcoin.git
```

### 3. Install Optional Dependencies

#### Wallet Dependencies
It is not necessary to build wallet functionality to run either `bitcoind` or `bitcoin-qt`.

###### Descriptor Wallet Support

`sqlite3` is required to support [descriptor wallets](descriptors.md).
Skip if you don't intend to use descriptor wallets.
```bash
pkg install sqlite3
```

###### Legacy Wallet Support
BerkeleyDB is only required if legacy wallet support is required.

It is required to use Berkeley DB 4.8. You **cannot** use the BerkeleyDB library
from ports. However, you can build DB 4.8 yourself [using depends](/depends).

```bash
pkg install gmake
gmake -C depends NO_BOOST=1 NO_LIBEVENT=1 NO_QT=1 NO_SQLITE=1 NO_NATPMP=1 NO_UPNP=1 NO_ZMQ=1 NO_USDT=1
```

When the build is complete, the Berkeley DB installation location will be displayed:

```
to: /path/to/bitcoin/depends/x86_64-unknown-freebsd[release-number]
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
This disables legacy wallet support and enables the GUI, assuming `sqlite` and `qt` are installed.
```bash
cmake -B build -DWITH_BDB=OFF -DBUILD_GUI=ON
```

Run `cmake -B build -LH` to see the full list of available options.

##### Descriptor & Legacy Wallet. No GUI:
This enables support for both wallet types, assuming
`sqlite3` and `db4` are both installed.
```bash
cmake -B build -DBerkeleyDB_INCLUDE_DIR:PATH="${BDB_PREFIX}/include"
```

##### No Wallet or GUI
```bash
cmake -B build -DENABLE_WALLET=OFF
```

### 2. Compile

```bash
cmake --build build     # Use "-j N" for N parallel jobs.
ctest --test-dir build  # Use "-j N" for N parallel tests. Some tests are disabled if Python 3 is not available.
```
