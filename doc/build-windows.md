WINDOWS BUILD NOTES
====================

Below are some notes on how to build Bitcoin Core for Windows.

The options known to work for building Bitcoin Core on Windows are:

* On Linux, using the [Mingw-w64](https://www.mingw-w64.org/) cross compiler tool chain.
* On Windows, using [Windows Subsystem for Linux (WSL)](https://learn.microsoft.com/en-us/windows/wsl/about) and Mingw-w64.
* On Windows, using [Microsoft Visual Studio](https://visualstudio.microsoft.com). See [`build-windows-msvc.md`](./build-windows-msvc.md).

Other options which may work, but which have not been extensively tested are (please contribute instructions):

* On Windows, using a POSIX compatibility layer application such as [cygwin](https://www.cygwin.com/) or [msys2](https://www.msys2.org/).

The instructions below work on Ubuntu and Debian. Make sure the distribution's `g++-mingw-w64-x86-64-posix`
package meets the minimum required `g++` version specified in [dependencies.md](dependencies.md).

Installing Windows Subsystem for Linux
---------------------------------------

Follow the upstream installation instructions, available [here](https://learn.microsoft.com/en-us/windows/wsl/install).

Cross-compilation for Ubuntu and Windows Subsystem for Linux
------------------------------------------------------------

The steps below can be performed on Ubuntu or WSL. The depends system
will also work on other Linux distributions, however the commands for
installing the toolchain will be different.

First, install the general dependencies:

    sudo apt update
    sudo apt upgrade
    sudo apt install cmake curl g++ git make pkg-config

A host toolchain (`g++`) is necessary because some dependency
packages need to build host utilities that are used in the build process.

See [dependencies.md](dependencies.md) for a complete overview.

If you want to build the Windows installer using the `deploy` build target, you will need [NSIS](https://nsis.sourceforge.io/Main_Page):

    sudo apt install nsis

Acquire the source in the usual way:

    git clone https://github.com/bitcoin/bitcoin.git
    cd bitcoin

## Building for 64-bit Windows

The first step is to install the mingw-w64 cross-compilation toolchain:

```sh
sudo apt install g++-mingw-w64-x86-64-posix
```

Once the toolchain is installed the build steps are common:

Note that for WSL the Bitcoin Core source path MUST be somewhere in the default mount file system, for
example /usr/src/bitcoin, AND not under /mnt/d/. If this is not the case the dependency autoconf scripts will fail.
This means you cannot use a directory that is located directly on the host Windows file system to perform the build.

Build using:

    gmake -C depends HOST=x86_64-w64-mingw32  # Use "-j N" for N parallel jobs.
    cmake -B build --toolchain depends/x86_64-w64-mingw32/toolchain.cmake
    cmake --build build     # Use "-j N" for N parallel jobs.

## Depends system

For further documentation on the depends system see [README.md](../depends/README.md) in the depends directory.

Installation
-------------

After building using the Windows subsystem it can be useful to copy the compiled
executables to a directory on the Windows drive in the same directory structure
as they appear in the release `.zip` archive. This can be done in the following
way. This will install to `c:\workspace\bitcoin`, for example:

    cmake --install build --prefix /mnt/c/workspace/bitcoin

You can also create an installer using:

    cmake --build build --target deploy
