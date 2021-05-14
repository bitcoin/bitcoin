# FreeBSD Build Guide

**Updated for FreeBSD [12.2](https://www.freebsd.org/releases/12.2R/announce.html)**

This guide describes how to build syscoind, command-line utilities, and GUI on FreeBSD.

## Dependencies

The following dependencies are **required**:

 Library                                                               | Purpose    | Description
 ----------------------------------------------------------------------|------------|----------------------
 [autoconf](https://svnweb.freebsd.org/ports/head/devel/autoconf/)     | Build      | Automatically configure software source code
 [automake](https://svnweb.freebsd.org/ports/head/devel/automake/)     | Build      | Generate makefile (requires autoconf)
 [libtool](https://svnweb.freebsd.org/ports/head/devel/libtool/)       | Build      | Shared library support
 [pkgconf](https://svnweb.freebsd.org/ports/head/devel/pkgconf/)       | Build      | Configure compiler and linker flags
 [git](https://svnweb.freebsd.org/ports/head/devel/git/)               | Clone      | Version control system
 [gmake](https://svnweb.freebsd.org/ports/head/devel/gmake/)           | Compile    | Generate executables
 [boost-libs](https://svnweb.freebsd.org/ports/head/devel/boost-libs/) | Utility    | Library for threading, data structures, etc
 [libevent](https://svnweb.freebsd.org/ports/head/devel/libevent/)     | Networking | OS independent asynchronous networking


The following dependencies are **optional**:

  Library                                                                    | Purpose          | Description
  ---------------------------------------------------------------------------|------------------|----------------------
  [db5](https://svnweb.freebsd.org/ports/head/databases/db5/)                | Berkeley DB      | Wallet storage (only needed when wallet enabled)
  [qt5](https://svnweb.freebsd.org/ports/head/devel/qt5/)                    | GUI              | GUI toolkit (only needed when GUI enabled)
  [libqrencode](https://svnweb.freebsd.org/ports/head/graphics/libqrencode/) | QR codes in GUI  | Generating QR codes (only needed when GUI enabled)
  [libzmq4](https://svnweb.freebsd.org/ports/head/net/libzmq4/)              | ZMQ notification | Allows generating ZMQ notifications (requires ZMQ version >= 4.0.0)
  [sqlite3](https://svnweb.freebsd.org/ports/head/databases/sqlite3/)        | SQLite DB        | Wallet storage (only needed when wallet enabled)
  [python3](https://svnweb.freebsd.org/ports/head/lang/python3/)             | Testing          | Python Interpreter (only needed when running the test suite)

  See [dependencies.md](dependencies.md) for a complete overview.

## Preparation

### 1. Install Required Dependencies
Install the required dependencies the usual way you [install software on FreeBSD](https://www.freebsd.org/doc/en/books/handbook/ports.html) - either with `pkg` or via the Ports collection. The example commands below use `pkg` which is usually run as `root` or via `sudo`. If you want to use `sudo`, and you haven't set it up: [use this guide](http://www.freebsdwiki.net/index.php/Sudo%2C_configuring) to setup `sudo` access on FreeBSD.

```bash
pkg install autoconf automake boost-libs git gmake libevent libtool pkgconf

```

### 2. Clone Syscoin Repo
Now that `git` and all the required dependencies are installed, let's clone the Syscoin Core repository to a directory. All build scripts and commands will run from this directory.
``` bash
git clone https://github.com/syscoin/syscoin.git
```

### 3. Install Optional Dependencies

#### Wallet Dependencies
It is not necessary to build wallet functionality to run syscoind or the GUI. To enable legacy wallets, you must install `db5`. To enable [descriptor wallets](https://github.com/syscoin/syscoin/blob/master/doc/descriptors.md), `sqlite3` is required. Skip `db5` if you intend to *exclusively* use descriptor wallets

###### Legacy Wallet Support
`db5` is required to enable support for legacy wallets. Skip if you don't intend to use legacy wallets

```bash
pkg install db5
```

###### Descriptor Wallet Support

`sqlite3` is required to enable support for descriptor wallets. Skip if you don't intend to use descriptor wallets.
``` bash
pkg install sqlite3
```
---

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

#### Test Suite Dependencies
There is an included test suite that is useful for testing code changes when developing.
To run the test suite (recommended), you will need to have Python 3 installed:

```bash
pkg install python3
```
---

## Building Syscoin Core

### 1. Configuration

There are many ways to configure Syscoin Core, here are a few common examples:
##### Wallet (BDB + SQlite) Support, No GUI:
This explicitly enables legacy wallet support and disables the GUI. If `sqlite3` is installed, then descriptor wallet support will be built.
```bash
./autogen.sh
./configure --with-gui=no --with-incompatible-bdb \
    BDB_LIBS="-ldb_cxx-5" \
    BDB_CFLAGS="-I/usr/local/include/db5" \
    MAKE=gmake
```

##### Wallet (only SQlite) and GUI Support:
This explicitly enables the GUI and disables legacy wallet support. If `qt5` is not installed, this will throw an error. If `sqlite3` is installed then descriptor wallet functionality will be built. If `sqlite3` is not installed, then wallet functionality will be disabled.
```bash
./autogen.sh
./configure --without-bdb --with-gui=yes MAKE=gmake
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
