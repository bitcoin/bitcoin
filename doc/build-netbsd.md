# NetBSD Build Guide

**Updated for NetBSD [10.0](https://netbsd.org/releases/formal-10/NetBSD-10.0.html)**

This guide describes how to build bitcoind, command-line utilities, and GUI on NetBSD.

## Preparation

### 1. Install Required Dependencies

Install the required dependencies the usual way you [install software on NetBSD](https://www.netbsd.org/docs/guide/en/chap-boot.html#chap-boot-pkgsrc).
The example commands below use `pkgin`.

```bash
pkgin install git cmake pkg-config boost-headers libevent
```

NetBSD currently ships with an older version of `gcc` than is needed to build. You should upgrade your `gcc` and then pass this new version to the configure script.

For example, grab `gcc12`:
```
pkgin install gcc12
```

Then, when configuring, pass the following:
```bash
cmake -B build
    ...
    -DCMAKE_C_COMPILER="/usr/pkg/gcc12/bin/gcc" \
    -DCMAKE_CXX_COMPILER="/usr/pkg/gcc12/bin/g++" \
    ...
```

See [dependencies.md](dependencies.md) for a complete overview.

### 2. Clone Bitcoin Repo

Clone the Bitcoin Core repository to a directory. All build scripts and commands will run from this directory.

```bash
git clone https://github.com/bitcoin/bitcoin.git
```

### 3. Install Optional Dependencies

#### Wallet Dependencies

It is not necessary to build wallet functionality to run bitcoind or the GUI.

###### Descriptor Wallet Support

`sqlite3` is required to enable support for [descriptor wallets](https://github.com/bitcoin/bitcoin/blob/master/doc/descriptors.md).

```bash
pkgin install sqlite3
```

###### Legacy Wallet Support

`db4` is required to enable support for legacy wallets.

```bash
pkgin install db4
```

#### GUI Dependencies
###### Qt5

Bitcoin Core includes a GUI built with the cross-platform Qt Framework. To compile the GUI, we need to install
the necessary parts of Qt, the libqrencode and pass `-DBUILD_GUI=ON`. Skip if you don't intend to use the GUI.

```bash
pkgin install qt5-qtbase qt5-qttools
```

###### libqrencode

The GUI will be able to encode addresses in QR codes unless this feature is explicitly disabled. To install libqrencode, run:

```bash
pkgin install qrencode
```

Otherwise, if you don't need QR encoding support, use the `-DWITH_QRENCODE=OFF` option to disable this feature in order to compile the GUI.


#### Test Suite Dependencies

There is an included test suite that is useful for testing code changes when developing.
To run the test suite (recommended), you will need to have Python 3 installed:

```bash
pkgin install python39
```

### Building Bitcoin Core

### 1. Configuration

There are many ways to configure Bitcoin Core. Here is an example that
explicitly disables the wallet and GUI:

```bash
cmake -B build -DENABLE_WALLET=OFF -DBUILD_GUI=OFF
```

Run `cmake -B build -LH` to see the full list of available options.

### 2. Compile

Build and run the tests:

```bash
cmake --build build     # Use "-j N" for N parallel jobs.
ctest --test-dir build  # Use "-j N" for N parallel tests. Some tests are disabled if Python 3 is not available.
```
