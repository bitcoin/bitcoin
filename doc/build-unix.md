UNIX BUILD NOTES
====================
Some notes on how to build Dash Core in Unix.

(for OpenBSD specific instructions, see [build-openbsd.md](build-openbsd.md))

Base build dependencies
-----------------------
Building the dependencies and Dash Core requires some essential build tools and libraries to be installed before.

Run the following commands to install required packages:

##### Debian/Ubuntu:
```bash
$ sudo apt-get install curl build-essential libtool autotools-dev automake pkg-config python3 bsdmainutils cmake
```

##### Fedora:
```bash
$ sudo dnf install gcc-c++ libtool make autoconf automake python3 cmake libstdc++-static patch
```

##### Arch Linux:
```bash
$ pacman -S base-devel python3 cmake
```

##### FreeBSD/OpenBSD:
```bash
pkg_add gmake cmake libtool
pkg_add autoconf # (select highest version, e.g. 2.69)
pkg_add automake # (select highest version, e.g. 1.15)
pkg_add python # (select highest version, e.g. 3.5)
```

Building
--------

Follow the instructions in [build-generic](build-generic.md)

Security
--------
To help make your Dash installation more secure by making certain attacks impossible to
exploit even if a vulnerability is found, binaries are hardened by default.
This can be disabled with:

Hardening Flags:

	./configure --prefix=<prefix> --enable-hardening
	./configure --prefix=<prefix> --disable-hardening


Hardening enables the following features:

* Position Independent Executable
    Build position independent code to take advantage of Address Space Layout Randomization
    offered by some kernels. Attackers who can cause execution of code at an arbitrary memory
    location are thwarted if they don't know where anything useful is located.
    The stack and heap are randomly located by default but this allows the code section to be
    randomly located as well.

    On an AMD64 processor where a library was not compiled with -fPIC, this will cause an error
    such as: "relocation R_X86_64_32 against `......' can not be used when making a shared object;"

    To test that you have built PIE executable, install scanelf, part of paxutils, and use:

    	scanelf -e ./dashd

    The output should contain:

     TYPE
    ET_DYN

* Non-executable Stack
    If the stack is executable then trivial stack based buffer overflow exploits are possible if
    vulnerable buffers are found. By default, Dash Core should be built with a non-executable stack
    but if one of the libraries it uses asks for an executable stack or someone makes a mistake
    and uses a compiler extension which requires an executable stack, it will silently build an
    executable without the non-executable stack protection.

    To verify that the stack is non-executable after compiling use:
    `scanelf -e ./dashd`

    the output should contain:
	STK/REL/PTL
	RW- R-- RW-

    The STK RW- means that the stack is readable and writeable but not executable.

Disable-wallet mode
--------------------
When the intention is to run only a P2P node without a wallet, Dash Core may be compiled in
disable-wallet mode with:

    ./configure --prefix=<prefix> --disable-wallet

In this case there is no dependency on Berkeley DB 4.8.

Mining is also possible in disable-wallet mode, but only using the `getblocktemplate` RPC
call not `getwork`.

Additional Configure Flags
--------------------------
A list of additional configure flags can be displayed with:

    ./configure --help

Building on FreeBSD
--------------------

(TODO, this is untested, please report if it works and if changes to this documentation are needed)

Building on FreeBSD is basically the same as on Linux based systems, with the difference that you have to use `gmake`
instead of `make`.

*Note on debugging*: The version of `gdb` installed by default is [ancient and considered harmful](https://wiki.freebsd.org/GdbRetirement).
It is not suitable for debugging a multi-threaded C++ program, not even for getting backtraces. Please install the package `gdb` and
use the versioned gdb command e.g. `gdb7111`.

Building on OpenBSD
-------------------

(TODO, this is untested, please report if it works and if changes to this documentation are needed)
(TODO, clang might also be an option. Old documentation reported it to to not work due to linking errors, but we're building all dependencies now as part of the depends system, so this might have changed)

Building on OpenBSD might require installation of a newer GCC version. If needed, do this with:

```bash
$ pkg_add g++ # (select newest 6.x version)
```

This compiler will not overwrite the system compiler, it will be installed as `egcc` and `eg++` in `/usr/local/bin`.

Add `CC=egcc CXX=eg++ CPP=ecpp` to the dependencies build and the Dash Core build:
```bash
$ cd depends
$ make CC=egcc CXX=eg++ CPP=ecpp # do not use -jX, this is broken
$ cd ..
$ export AUTOCONF_VERSION=2.69 # replace this with the autoconf version that you installed
$ export AUTOMAKE_VERSION=1.15 # replace this with the automake version that you installed
$ ./autogen.sh
$ ./configure --prefix=<prefix> CC=egcc CXX=eg++ CPP=ecpp
$ gmake # do not use -jX, this is broken
```
