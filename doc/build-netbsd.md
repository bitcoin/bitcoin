# NetBSD Build Guide

**Updated for NetBSD [10.1](https://netbsd.org/releases/formal-10/NetBSD-10.1.html)**

This guide describes how to build bitcoind, command-line utilities, and GUI on NetBSD.

## Preparation

### 1. Install Required Dependencies

Install the required dependencies the usual way you [install software on NetBSD](https://www.netbsd.org/docs/guide/en/chap-boot.html#chap-boot-pkgsrc).
The example commands below use `pkgin`.

```bash
pkgin install git cmake pkg-config boost libevent
```

NetBSD currently ships with an older version of `gcc` than is needed to build. You should upgrade your `gcc` and then pass this new version to the CMake configuration.

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

Cap'n Proto is needed for IPC functionality (see [multiprocess.md](multiprocess.md)):

```bash
pkgin install capnproto
```

Compile with `-DENABLE_IPC=OFF` if you do not need IPC functionality.

See [dependencies.md](dependencies.md) for a complete overview.

### 2. Clone Bitcoin Repo

Clone the Bitcoin Core repository to a directory. All build scripts and commands will run from this directory.

```bash
git clone https://github.com/bitcoin/bitcoin.git
```

### 3. Install Optional Dependencies

#### Notifications
###### ZeroMQ

Bitcoin Core can provide notifications via ZeroMQ. If the package is installed, support will be compiled in.
```bash
pkgin install zeromq
```

#### Test Suite Dependencies

There is an included test suite that is useful for testing code changes when developing.
To run the test suite (recommended), you will need to have Python 3 installed:

```bash
pkgin install python313 py313-zmq
```

## Building Bitcoin Core

### 1. Configuration

Run `cmake -B build -LH` to see the full list of available options.

### 2. Compile

Build and run the tests:

```bash
cmake --build build     # Append "-j N" for N parallel jobs.
ctest --test-dir build  # Append "-j N" for N parallel tests.
```
