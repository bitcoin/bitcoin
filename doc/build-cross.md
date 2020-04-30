Cross-compiliation of Dash Core
===============================

Dash Core can be cross-compiled on Linux to all other supported host systems. This is done by changing
the `HOST` parameter when building the dependencies and then specifying another `--prefix` directory when building Dash.

The following instructions are only tested on Debian Stretch and Ubuntu Bionic.

MacOSX Cross-compilation
------------------------
Cross-compiling to MacOSX requires a few additional packages to be installed:

```bash
$ sudo apt-get install python3-setuptools libcap-dev zlib1g-dev libbz2-dev
```

Additionally, the Mac OSX SDK must be downloaded and extracted manually:

```bash
$ mkdir -p depends/sdk-sources
$ mkdir -p depends/SDKs
$ curl https://bitcoincore.org/depends-sources/sdks/MacOSX10.11.sdk.tar.gz -o depends/sdk-sources/MacOSX10.11.sdk.tar.gz
$ tar -C depends/SDKs -xf depends/sdk-sources/MacOSX10.11.sdk.tar.gz
```

When building the dependencies, as described in [build-generic](build-generic.md), use

```bash
$ make HOST=x86_64-apple-darwin11 -j4
```

When building Dash Core, use

```bash
$ ./configure --prefix=`pwd`/depends/x86_64-apple-darwin11
```

Windows 64bit Cross-compilation
-------------------------------
The steps below can be performed on Ubuntu (including in a VM) or WSL. The depends system
will also work on other Linux distributions, however the commands for
installing the toolchain will be different.

First, install the general dependencies:

    sudo apt update
    sudo apt upgrade
    sudo apt install build-essential libtool autotools-dev automake pkg-config bsdmainutils curl git python3 cmake

A host toolchain (`build-essential`) is necessary because some dependency
packages need to build host utilities that are used in the build process.

See [dependencies.md](dependencies.md) for a complete overview.

If you want to build the windows installer with `make deploy` you need [NSIS](https://nsis.sourceforge.io/Main_Page):

    sudo apt install nsis

Acquire the source in the usual way:

    git clone https://github.com/dashpay/dash.git
    cd dash

### Building for 64-bit Windows

The first step is to install the mingw-w64 cross-compilation tool chain:

    sudo apt install g++-mingw-w64-x86-64

Ubuntu Bionic 18.04 <sup>[1](#footnote1)</sup>:

    sudo update-alternatives --config x86_64-w64-mingw32-g++ # Set the default mingw32 g++ compiler option to posix.

Once the toolchain is installed the build steps are common:

Note that for WSL the Dash Core source path MUST be somewhere in the default mount file system, for
example /usr/src/dash, AND not under /mnt/d/. If this is not the case the dependency autoconf scripts will fail.
This means you cannot use a directory that is located directly on the host Windows file system to perform the build.

Build using:

    PATH=$(echo "$PATH" | sed -e 's/:\/mnt.*//g') # strip out problematic Windows %PATH% imported var
    cd depends
    make HOST=x86_64-w64-mingw32
    cd ..
    ./autogen.sh # not required when building from tarball
    CONFIG_SITE=$PWD/depends/x86_64-w64-mingw32/share/config.site ./configure --prefix=/
    make

### Depends system

For further documentation on the depends system see [README.md](../depends/README.md) in the depends directory.

ARM-Linux Cross-compilation
-------------------
Cross-compiling to ARM-Linux requires a few additional packages to be installed:

```bash
$ sudo apt-get install g++-arm-linux-gnueabihf
```

When building the dependencies, as described in [build-generic](build-generic.md), use

```bash
$ make HOST=arm-linux-gnueabihf -j4
```

When building Dash Core, use

```bash
$ ./configure --prefix=`pwd`/depends/arm-linux-gnueabihf
```

Footnotes
---------

<a name="footnote1">1</a>: Starting from Ubuntu Xenial 16.04, both the 32 and 64 bit Mingw-w64 packages install two different
compiler options to allow a choice between either posix or win32 threads. The default option is win32 threads which is the more
efficient since it will result in binary code that links directly with the Windows kernel32.lib. Unfortunately, the headers
required to support win32 threads conflict with some of the classes in the C++11 standard library, in particular std::mutex.
It's not possible to build the Dash Core code using the win32 version of the Mingw-w64 cross compilers (at least not without
modifying headers in the Dash Core source code).
