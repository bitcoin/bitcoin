# OpenBSD Build Guide

**Updated for OpenBSD [7.5](https://www.openbsd.org/75.html)**

This guide describes how to build bitcoind, command-line utilities, and GUI on OpenBSD.

## Preparation

### 1. Install Required Dependencies
Run the following as root to install the base dependencies for building.

```bash
pkg_add git cmake boost libevent
```

See [dependencies.md](dependencies.md) for a complete overview.

### 2. Clone Bitcoin Repo
Clone the Bitcoin Core repository to a directory. All build scripts and commands will run from this directory.
``` bash
git clone https://github.com/bitcoin/bitcoin.git
```

### 3. Install Optional Dependencies

#### Wallet Dependencies

It is not necessary to build wallet functionality to run either `bitcoind` or `bitcoin-qt`.

###### Descriptor Wallet Support

SQLite is required to support [descriptor wallets](descriptors.md).

``` bash
pkg_add sqlite3
```

###### Legacy Wallet Support
BerkeleyDB is only required to support legacy wallets.

It is recommended to use Berkeley DB 4.8. You cannot use the BerkeleyDB library
from ports. However you can build it yourself, [using depends](/depends).

Refer to [depends/README.md](/depends/README.md) for detailed instructions.

```bash
gmake -C depends NO_BOOST=1 NO_LIBEVENT=1 NO_QT=1 NO_SQLITE=1 NO_NATPMP=1 NO_UPNP=1 NO_ZMQ=1 NO_USDT=1
...
to: /path/to/bitcoin/depends/*-unknown-openbsd*
```

Then set `BDB_PREFIX`:

```bash
export BDB_PREFIX="[path displayed above]"
```

#### GUI Dependencies
###### Qt5

Bitcoin Core includes a GUI built with the cross-platform Qt Framework. To compile the GUI, Qt 5 is required.

```bash
pkg_add qtbase qttools
```

#### Test Suite Dependencies
There is an included test suite that is useful for testing code changes when developing.
To run the test suite (recommended), you will need to have Python 3 installed:

```bash
pkg_add install python  # Select the newest version of the package.
```

## Building Bitcoin Core

### 1. Configuration

There are many ways to configure Bitcoin Core, here are a few common examples:

##### Descriptor Wallet and GUI:
This enables descriptor wallet support and the GUI, assuming SQLite and Qt 5 are installed.

```bash
cmake -B build -DWITH_SQLITE=ON -DBUILD_GUI=ON
```

Run `cmake -B build -LH` to see the full list of available options.

##### Descriptor & Legacy Wallet. No GUI:
This enables support for both wallet types:

```bash
cmake -B build -DBerkeleyDB_INCLUDE_DIR:PATH="${BDB_PREFIX}/include"
```

### 2. Compile

```bash
cmake --build build     # Use "-j N" for N parallel jobs.
ctest --test-dir build  # Use "-j N" for N parallel tests. Some tests are disabled if Python 3 is not available.
```

## Resource limits

If the build runs into out-of-memory errors, the instructions in this section
might help.

The standard ulimit restrictions in OpenBSD are very strict:
```bash
data(kbytes)         1572864
```

This is, unfortunately, in some cases not enough to compile some `.cpp` files in the project,
(see issue [#6658](https://github.com/bitcoin/bitcoin/issues/6658)).
If your user is in the `staff` group the limit can be raised with:
```bash
ulimit -d 3000000
```
The change will only affect the current shell and processes spawned by it. To
make the change system-wide, change `datasize-cur` and `datasize-max` in
`/etc/login.conf`, and reboot.
