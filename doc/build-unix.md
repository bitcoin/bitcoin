UNIX BUILD NOTES
====================
Some notes on how to build Dash Core in Unix.

(For BSD specific instructions, see [build-openbsd.md](build-openbsd.md) and/or
[build-netbsd.md](build-netbsd.md))

Base build dependencies
-----------------------
Building the dependencies and Dash Core requires some essential build tools and libraries to be installed before.

Run the following commands to install required packages:

##### Debian/Ubuntu:
```bash
$ sudo apt-get install curl build-essential libtool autotools-dev automake pkg-config python3 bsdmainutils
```

##### Fedora:
```bash
$ sudo dnf install gcc-c++ libtool make autoconf automake python3 libstdc++-static patch
```

##### Arch Linux:
```bash
$ pacman -S base-devel python3
```

##### Alpine Linux:
```sh
$ sudo apk --update --no-cache add autoconf automake curl g++ gcc libexecinfo-dev libexecinfo-static libtool make perl pkgconfig python3 patch linux-headers
```

##### FreeBSD/OpenBSD:
```bash
pkg_add gmake libtool
pkg_add autoconf # (select highest version, e.g. 2.69)
pkg_add automake # (select highest version, e.g. 1.15)
pkg_add python # (select highest version, e.g. 3.5)
```

Building
--------

Follow the instructions in [build-generic](build-generic.md)

Security
--------
To help make your Dash Core installation more secure by making certain attacks impossible to
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
    The stack and heap are randomly located by default, but this allows the code section to be
    randomly located as well.

    On an AMD64 processor where a library was not compiled with -fPIC, this will cause an error
    such as: "relocation R_X86_64_32 against `......' can not be used when making a shared object;"

    To test that you have built PIE executable, install scanelf, part of paxutils, and use:

    	scanelf -e ./dashd

    The output should contain:

     TYPE
    ET_DYN

* Non-executable Stack
    If the stack is executable then trivial stack-based buffer overflow exploits are possible if
    vulnerable buffers are found. By default, Dash Core should be built with a non-executable stack,
    but if one of the libraries it uses asks for an executable stack or someone makes a mistake
    and uses a compiler extension which requires an executable stack, it will silently build an
    executable without the non-executable stack protection.

    To verify that the stack is non-executable after compiling use:
    `scanelf -e ./dashd`

    The output should contain:
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

**Important**: From OpenBSD 6.2 onwards a C++11-supporting clang compiler is
part of the base image, and while building it is necessary to make sure that this
compiler is used and not ancient g++ 4.2.1. This is done by appending
`CC=cc CXX=c++` to configuration commands. Mixing different compilers
within the same executable will result in linker errors.

```bash
$ cd depends
$ make CC=cc CXX=c++
$ cd ..
$ export AUTOCONF_VERSION=2.69 # replace this with the autoconf version that you installed
$ export AUTOMAKE_VERSION=1.15 # replace this with the automake version that you installed
$ ./autogen.sh
$ ./configure --prefix=<prefix> CC=cc CXX=c++
$ gmake # use -jX here for parallelism
```

OpenBSD Resource limits
-------------------
If the build runs into out-of-memory errors, the instructions in this section
might help.

The standard ulimit restrictions in OpenBSD are very strict:

    data(kbytes)         1572864

This, unfortunately, in some cases not enough to compile some `.cpp` files in the project,
(see issue [#6658](https://github.com/bitcoin/bitcoin/issues/6658)).
If your user is in the `staff` group the limit can be raised with:

    ulimit -d 3000000

The change will only affect the current shell and processes spawned by it. To
make the change system-wide, change `datasize-cur` and `datasize-max` in
`/etc/login.conf`, and reboot.
