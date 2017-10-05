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

Windows 64bit/32bit Cross-compilation
-------------------------------
Cross-compiling to Windows requires a few additional packages to be installed:

```bash
$ sudo apt-get install nsis wine-stable wine64 bc
```

For Windows 64bit, install :
```bash
$ sudo apt-get install g++-mingw-w64-x86-64
```

If you're building on Ubuntu 17.04 or later, run these two commands, selecting the 'posix' variant for both,
to work around issues with mingw-w64. See issue [8732](https://github.com/bitcoin/bitcoin/issues/8732) for more information.
This also fixes linker issues related to std::thread and other threading related standard C++ libraries.
```
sudo update-alternatives --config x86_64-w64-mingw32-g++
sudo update-alternatives --config x86_64-w64-mingw32-gcc
```

For Windows 32bit, install:
```bash
$ sudo apt-get install g++-mingw-w64-i686
```

If you're building on Ubuntu 17.04 or later, run these two commands, selecting the 'posix' variant for both,
to fix linker issues related to std::thread and other threading related standard C++ libraries.
```
sudo update-alternatives --config x86_64-w64-mingw32-g++
sudo update-alternatives --config x86_64-w64-mingw32-gcc
```

Before building for Windows 32bit or 64bit, run

```
$ PATH=$(echo "$PATH" | sed -e 's/:\/mnt.*//g') # strip out problematic Windows %PATH% imported var
```

When building the dependencies, as described in [build-generic](build-generic.md), use

```bash
$ make HOST=x86_64-w64-mingw32 -j4
```

When building Dash Core, use

```bash
$ ./configure --prefix=`pwd`/depends/x86_64-w64-mingw32
```

These commands will build for Windows 64bit. If you want to compile for 32bit,
replace `x86_64-w64-mingw32` with `i686-w64-mingw32`.

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
