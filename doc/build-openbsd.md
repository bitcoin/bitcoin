# OpenBSD Build Guide

Bitcoin Core is supported on [the two most recent OpenBSD releases](https://www.openbsd.org/faq/faq5.html#Flavors).

This guide describes how to build bitcoind, command-line utilities, and GUI on the latest release.

## Preparation

### 1. Install Required Dependencies
Run the following as root to install the base dependencies for building.

```bash
pkg_add git cmake boost
```

SQLite is required for the wallet:

```bash
pkg_add sqlite3
```

To build Bitcoin Core without the wallet, use `-DENABLE_WALLET=OFF`.

Cap'n Proto is needed for IPC functionality (see [multiprocess.md](multiprocess.md)):

```bash
pkg_add capnproto
```

Compile with `-DENABLE_IPC=OFF` if you do not need IPC functionality.

See [dependencies.md](dependencies.md) for a complete overview.

### 2. Clone Bitcoin Repo
Clone the Bitcoin Core repository to a directory. All build scripts and commands will run from this directory.
``` bash
git clone https://github.com/bitcoin/bitcoin.git
```

### 3. Install Optional Dependencies

#### GUI Dependencies
###### Qt6

Bitcoin Core includes a GUI built with the cross-platform Qt Framework. To compile the GUI, we need to install
the necessary parts of Qt, the libqrencode and pass `-DBUILD_GUI=ON`. Skip if you don't intend to use the GUI.

```bash
pkg_add qt6-qtbase qt6-qttools
```

###### libqrencode

The GUI will be able to encode addresses in QR codes unless this feature is explicitly disabled. To install libqrencode, run:

```bash
pkg_add libqrencode
```

Otherwise, if you don't need QR encoding support, use the `-DWITH_QRENCODE=OFF` option to disable this feature in order to compile the GUI.

---

#### Notifications
###### ZeroMQ

Bitcoin Core can provide notifications via ZeroMQ. To compile ZMQ support, install the following dependency and pass `-DWITH_ZMQ=ON` when configuring.
```bash
pkg_add zeromq
```

#### Test Suite Dependencies

See [`test/README.md`](/test/README.md#dependencies-and-prerequisites).

## Building Bitcoin Core

### 1. Configuration

There are many ways to configure Bitcoin Core, here are a few common examples:

##### Wallet and GUI:
This enables wallet support and the GUI, assuming SQLite and Qt 6 are installed.

```bash
cmake -B build -DBUILD_GUI=ON
```

Run `cmake -B build -LH` to see the full list of available options.

### 2. Compile

```bash
cmake --build build     # Append "-j N" for N parallel jobs.
ctest --test-dir build  # Append "-j N" for N parallel tests.
```

