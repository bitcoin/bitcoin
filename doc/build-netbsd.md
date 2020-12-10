# NetBSD Build Guide

Updated for NetBSD [9.1](https://netbsd.org/releases/formal-9/NetBSD-9.1.html)

This guide describes how to build bitcoind, command-line utilities, and GUI on NetBSD.

## Dependencies

The following dependencies are required:

 Library                                          | Purpose    | Description
 -------------------------------------------------|------------|----------------------
 [autoconf](https://pkgsrc.se/devel/autoconf)     | Build      | Automatically configure software source code
 [automake](https://pkgsrc.se/devel/automake)     | Build      | Generate makefile (requires autoconf)
 [libtool](https://pkgsrc.se/devel/libtool)       | Build      | Shared library support
 [pkg-config](https://pkgsrc.se/devel/pkg-config) | Build      | Configure compiler and linker flags
 [git](https://pkgsrc.se/devel/git)               | Clone      | Version control system
 [gmake](https://pkgsrc.se/devel/gmake)           | Compile    | Generate executables
 [boost](https://pkgsrc.se/meta-pkgs/boost)       | Utility    | Library for threading, data structures, etc
 [libevent](https://pkgsrc.se/devel/libevent)     | Networking | OS-independent asynchronous networking


The following dependencies are optional:

  Library                                            | Purpose          | Description
  ---------------------------------------------------|------------------|----------------------
  [db4](https://pkgsrc.se/databases/db4)             | Berkeley DB      | Wallet storage (only needed when wallet enabled)
  [qt5](https://pkgsrc.se/x11/qt5)                   | GUI              | GUI toolkit (only needed when GUI enabled)
  [qrencode](https://pkgsrc.se/converters/qrencode)  | QR codes in GUI  | Generating QR codes (only needed when GUI enabled)
  [zeromq](https://pkgsrc.se/net/zeromq)             | ZMQ notification | Allows generating ZMQ notifications (requires ZMQ version >= 4.0.0)
  [sqlite3](https://pkgsrc.se/databases/sqlite3)     | SQLite DB        | Wallet storage (only needed when wallet enabled)
  [python37](https://pkgsrc.se/lang/python37)        | Testing          | Python Interpreter (only needed when running the test suite)

  See [dependencies.md](dependencies.md) for a complete overview.


## Preparation

### 1. Install Required Dependencies

Install the required dependencies the usual way you [install software on NetBSD](https://www.netbsd.org/docs/guide/en/chap-boot.html#chap-boot-pkgsrc) -- either with `pkg_add` or `pkgin`. The example commands below use `pkgin` which is the [recommended](https://www.netbsd.org/docs/guide/en/chap-boot.html#chap-boot-pkgsrc) way to install binary packages.

Note: `pkgin` is usually run as `root` or with `sudo`.

```bash
pkgin install autoconf automake libtool pkg-config git gmake boost libevent

```

### 2. Clone Bitcoin Repo

Now that `git` and the required dependencies are installed, let's clone the Bitcoin Core repository to a directory. All build scripts and commands will run from this directory.

```bash
git clone https://github.com/bitcoin/bitcoin.git
```

### 3. Install Optional Dependencies

#### Wallet Dependencies

It is not necessary to build wallet functionality to run bitcoind or the GUI. To enable legacy wallets, you must install Berkeley DB, aka BDB or `db4`. To enable [descriptor wallets](https://github.com/bitcoin/bitcoin/blob/master/doc/descriptors.md), SQLite (`sqlite3`) is required. Skip `db4` if you intend to *exclusively* use descriptor wallets.

###### Legacy Wallet Support

BDB (`db4`) is required to enable support for legacy wallets. Skip if you don't intend to use legacy wallets.

```bash
pkgin install db4
```

###### Descriptor Wallet Support

SQLite (`sqlite3`) is required to enable support for descriptor wallets. Skip if you don't intend to use descriptor wallets.

```bash
pkgin install sqlite3
```
---

#### GUI Dependencies
###### Qt5

Bitcoin Core includes a GUI built with the cross-platform Qt Framework. To compile the GUI, we need to install `qt5`. Skip if you don't intend to use the GUI.

```bash
pkgin install qt5
```
###### qrencode

The GUI can encode addresses in a QR Code. To build in QR support for the GUI, install `qrencode`. Skip if not using the GUI or don't need QR code functionality.

```bash
pkgin install qrencode
```
---

#### Test Suite Dependencies

There is an included test suite that is useful for testing code changes when developing.
To run the test suite (recommended), you will need to have Python 3 installed:

```bash
pkgin install python37
```
---

### Building Bitcoin Core

**Important**: Use `gmake` (the non-GNU `make` will exit with an error).


### 1. Configuration

There are many ways to configure Bitcoin Core. Here are a few common examples:
##### Wallet (BDB + SQlite) Support, No GUI:

This explicitly enables legacy wallet support and disables the GUI. An error will be thrown if `db4` is not installed. If `sqlite3` is installed, then descriptor wallet support will also be built.

```bash
./autogen.sh
./configure --with-gui=no \
    CPPFLAGS="-I/usr/pkg/include" \
    LDFLAGS="-L/usr/pkg/lib" \
    --with-boost-libdir=/usr/pkg/lib \
    MAKE=gmake
```

##### Wallet (only SQlite) and GUI Support:

This explicitly enables the GUI and disables legacy wallet support. If `qt5` is not installed, this will throw an error. If `sqlite3` is installed then descriptor wallet functionality will be built. If `sqlite3` is not installed, then wallet functionality will be disabled.

```bash
./autogen.sh
./configure --without-bdb --with-gui=yes \
    CPPFLAGS="-I/usr/pkg/include" \
    LDFLAGS="-L/usr/pkg/lib" \
    --with-boost-libdir=/usr/pkg/lib \
    MAKE=gmake
```

##### No Wallet or GUI

```bash
./autogen.sh
./configure --without-wallet --with-gui=no \
    CPPFLAGS="-I/usr/pkg/include" \
    LDFLAGS="-L/usr/pkg/lib" \
    --with-boost-libdir=/usr/pkg/lib \
    MAKE=gmake
```


### 2. Compile

Build and run the tests:

```bash
gmake # use "-j N" here for N parallel jobs
gmake check # Run tests if Python 3 is available
```
