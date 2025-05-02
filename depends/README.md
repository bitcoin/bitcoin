# Depends build

This is a system of building and caching dependencies necessary for building
Bitcoin Core. It supports cross-compilation. For more details see [description.md](description.md),
as well as [packages.md](packages.md) for how to add packages.

## Usage

### Ubuntu & Debian

    apt install cmake curl make patch

Skip the following packages if you don't intend to use the GUI and will build with [`NO_QT=1`](#dependency-options):

    apt install bison g++ ninja-build pkgconf python3 xz-utils

To build dependencies for the current arch+OS:

    make

### macOS

Install Xcode Command Line Tools and Homebrew Package Manager,
see [build-osx.md](../doc/build-osx.md).

    brew install cmake make ninja

To build dependencies for the current arch+OS:

    gmake

### FreeBSD

    pkg install bash

To build dependencies for the current arch+OS:

    gmake

### NetBSD

    pkgin install bash gmake

To build dependencies for the current arch+OS:

    gmake

### OpenBSD

    pkg_add bash gmake gtar

To build dependencies for the current arch+OS:

    gmake

## Configuring Bitcoin Core

**When configuring Bitcoin Core, CMake by default will ignore the depends output.** In
order for it to pick up libraries, tools, and settings from the depends build,
you must specify the toolchain file.
In the above example for Ubuntu, a file named `depends/x86_64-pc-linux-gnu/toolchain.cmake` will be
created. To use it during configuring Bitcoin Core:

    cmake -B build --toolchain depends/x86_64-pc-linux-gnu/toolchain.cmake

## Dependency Options

The following can be set when running make: `make FOO=bar`

- `SOURCES_PATH`: Downloaded sources will be placed here
- `BASE_CACHE`: Built packages will be placed here
- `SDK_PATH`: Path where SDKs can be found (used by macOS)
- `FALLBACK_DOWNLOAD_PATH`: If a source file can't be fetched, try here before giving up
- `C_STANDARD`: Set the C standard version used. Defaults to `c11`.
- `CXX_STANDARD`: Set the C++ standard version used. Defaults to `c++20`.
- `NO_BOOST`: Don't download/build/cache Boost
- `NO_LIBEVENT`: Don't download/build/cache Libevent
- `NO_QT`: Don't download/build/cache Qt and its dependencies
- `NO_QR`: Don't download/build/cache packages needed for enabling qrencode
- `NO_ZMQ`: Don't download/build/cache packages needed for enabling ZeroMQ
- `NO_WALLET`: Don't download/build/cache libs needed to enable the wallet (SQLite)
- `NO_BDB`: Don't download/build/cache BerkeleyDB
- `NO_USDT`: Don't download/build/cache packages needed for enabling USDT tracepoints
- `MULTIPROCESS`: Build libmultiprocess (experimental)
- `DEBUG`: Disable some optimizations and enable more runtime checking
- `HOST_ID_SALT`: Optional salt to use when generating host package ids
- `BUILD_ID_SALT`: Optional salt to use when generating build package ids
- `LOG`: Use file-based logging for individual packages. During a package build its log file
  resides in the `depends` directory, and the log file is printed out automatically in case
  of build error. After successful build log files are moved along with package archives
- `LTO`: Enable options needed for LTO. Does not add `-flto` related options to *FLAGS.

If some packages are not built, for example `make NO_WALLET=1`, the appropriate CMake cache
variables will be set when generating the Bitcoin Core buildsystem. In this case, `-DENABLE_WALLET=OFF`.

## Cross compilation

To build for another arch/OS:

    make HOST=host-platform-triplet

For example:

    make HOST=x86_64-w64-mingw32 -j4

Common `host-platform-triplet`s for cross compilation are:

- `i686-pc-linux-gnu` for Linux x86 32 bit
- `x86_64-pc-linux-gnu` for Linux x86 64 bit
- `x86_64-w64-mingw32` for Win64
- `x86_64-apple-darwin` for macOS
- `arm64-apple-darwin` for ARM macOS
- `arm-linux-gnueabihf` for Linux ARM 32 bit
- `aarch64-linux-gnu` for Linux ARM 64 bit
- `powerpc64-linux-gnu` for Linux POWER 64 bit (big endian)
- `powerpc64le-linux-gnu` for Linux POWER 64 bit (little endian)
- `riscv32-linux-gnu` for Linux RISC-V 32 bit
- `riscv64-linux-gnu` for Linux RISC-V 64 bit
- `s390x-linux-gnu` for Linux S390X

The paths are automatically configured and no other options are needed.

#### For macOS cross compilation

    apt install clang lld llvm zip

Clang 18 or later is required. You must also obtain the macOS SDK before
proceeding with a cross-compile. Under the depends directory, create a
subdirectory named `SDKs`. Then, place the extracted SDK under this new directory.
For more information, see [SDK Extraction](../contrib/macdeploy/README.md#sdk-extraction).

#### For Win64 cross compilation

    apt install g++-mingw-w64-x86-64-posix

#### For linux (including i386, ARM) cross compilation

Common linux dependencies:

    sudo apt-get install g++-multilib binutils

For linux ARM cross compilation:

    sudo apt-get install g++-arm-linux-gnueabihf binutils-arm-linux-gnueabihf

For linux AARCH64 cross compilation:

    sudo apt-get install g++-aarch64-linux-gnu binutils-aarch64-linux-gnu

For linux POWER 64-bit cross compilation (there are no packages for 32-bit):

    sudo apt-get install g++-powerpc64-linux-gnu binutils-powerpc64-linux-gnu g++-powerpc64le-linux-gnu binutils-powerpc64le-linux-gnu

For linux RISC-V 64-bit cross compilation (there are no packages for 32-bit):

    sudo apt-get install g++-riscv64-linux-gnu binutils-riscv64-linux-gnu

For linux S390X cross compilation:

    sudo apt-get install g++-s390x-linux-gnu binutils-s390x-linux-gnu

### Additional targets

    download: run 'make download' to fetch all sources without building them
    download-osx: run 'make download-osx' to fetch all sources needed for macOS builds
    download-win: run 'make download-win' to fetch all sources needed for win builds
    download-linux: run 'make download-linux' to fetch all sources needed for linux builds
