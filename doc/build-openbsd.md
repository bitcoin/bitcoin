# OpenBSD Build Guide

**Updated for OpenBSD [7.8](https://www.openbsd.org/78.html)**

This guide describes how to build bitcoind, command-line utilities, and GUI on OpenBSD.

## Preparation

### 1. Install Required Dependencies
Run the following as root to install the base dependencies for building.

```bash
pkg_add git cmake boost libevent
```

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

#### Notifications
###### ZeroMQ

Bitcoin Core can provide notifications via ZeroMQ. If the package is installed, support will be compiled in.
```bash
pkg_add zeromq
```

#### Test Suite Dependencies
There is an included test suite that is useful for testing code changes when developing.
To run the test suite (recommended), you will need to have Python 3 installed:

```bash
pkg_add python py3-zmq  # Select the newest version of the python package if necessary.
```

## Building Bitcoin Core

### 1. Configuration

There are many ways to configure Bitcoin Core, here are a few common examples:

Run `cmake -B build -LH` to see the full list of available options.

### 2. Compile

```bash
cmake --build build     # Append "-j N" for N parallel jobs.
ctest --test-dir build  # Append "-j N" for N parallel tests.
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
