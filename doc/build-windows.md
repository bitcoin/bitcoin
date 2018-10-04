WINDOWS BUILD NOTES
====================

Below are some notes on how to build Bitcoin Core for Windows.

The options known to work for building Bitcoin Core on Windows are:

* On Linux using the [Mingw-w64](https://mingw-w64.org/doku.php) cross compiler tool chain. Ubuntu Bionic 18.04 is required
and is the platform used to build the Bitcoin Core Windows release binaries.
* On Windows using [Windows
Subsystem for Linux (WSL)](https://msdn.microsoft.com/commandline/wsl/about) and the Mingw-w64 cross compiler tool chain.

Other options which may work, but which have not been extensively tested are (please contribute instructions):

* On Windows using a POSIX compatibility layer application such as [cygwin](http://www.cygwin.com/) or [msys2](http://www.msys2.org/).
* On Windows using a native compiler tool chain such as [Visual Studio](https://www.visualstudio.com).

Installing Windows Subsystem for Linux
---------------------------------------

With Windows 10, Microsoft has released a new feature named the [Windows
Subsystem for Linux (WSL)](https://msdn.microsoft.com/commandline/wsl/about). This
feature allows you to run a bash shell directly on Windows in an Ubuntu-based
environment. Within this environment you can cross compile for Windows without
the need for a separate Linux VM or server. Note that while WSL can be installed with
other Linux variants, such as OpenSUSE, the following instructions have only been
tested with Ubuntu.

This feature is not supported in versions of Windows prior to Windows 10 or on
Windows Server SKUs. In addition, it is available [only for 64-bit versions of
Windows](https://msdn.microsoft.com/en-us/commandline/wsl/install_guide).

Full instructions to install WSL are available on the above link.
To install WSL on Windows 10 with Fall Creators Update installed (version >= 16215.0) do the following:

1. Enable the Windows Subsystem for Linux feature
  * Open the Windows Features dialog (`OptionalFeatures.exe`)
  * Enable 'Windows Subsystem for Linux'
  * Click 'OK' and restart if necessary
2. Install Ubuntu
  * Open Microsoft Store and search for "Ubuntu 18.04" or use [this link](https://www.microsoft.com/store/productId/9N9TNGVNDL3Q)
  * Click Install
3. Complete Installation
  * Open a cmd prompt and type "Ubuntu1804"
  * Create a new UNIX user account (this is a separate account from your Windows account)

After the bash shell is active, you can follow the instructions below, starting
with the "Cross-compilation" section. Compiling the 64-bit version is
recommended, but it is possible to compile the 32-bit version.

Cross-compilation for Ubuntu and Windows Subsystem for Linux
------------------------------------------------------------

The steps below can be performed on Ubuntu (including in a VM) or WSL. The depends system
will also work on other Linux distributions, however the commands for
installing the toolchain will be different.

First, install the general dependencies:

    sudo apt update
    sudo apt upgrade
    sudo apt install build-essential libtool autotools-dev automake pkg-config bsdmainutils curl git

A host toolchain (`build-essential`) is necessary because some dependency
packages (such as `protobuf`) need to build host utilities that are used in the
build process.

See also: [dependencies.md](dependencies.md).

## Building for 64-bit Windows

The first step is to install the mingw-w64 cross-compilation tool chain.

    sudo apt install g++-mingw-w64-x86-64

Ubuntu Bionic 18.04 <sup>[1](#footnote1)</sup>:

    sudo update-alternatives --config x86_64-w64-mingw32-g++ # Set the default mingw32 g++ compiler option to posix.

Once the toolchain is installed the build steps are common:

Note that for WSL the Bitcoin Core source path MUST be somewhere in the default mount file system, for
example /usr/src/bitcoin, AND not under /mnt/d/. If this is not the case the dependency autoconf scripts will fail.
This means you cannot use a directory that located directly on the host Windows file system to perform the build.

Acquire the source in the usual way:

    git clone https://github.com/bitcoin/bitcoin.git

Once the source code is ready the build steps are below.

    PATH=$(echo "$PATH" | sed -e 's/:\/mnt.*//g') # strip out problematic Windows %PATH% imported var
    cd depends
    make HOST=x86_64-w64-mingw32
    cd ..
    ./autogen.sh # not required when building from tarball
    CONFIG_SITE=$PWD/depends/x86_64-w64-mingw32/share/config.site ./configure --prefix=/
    make

## Building for 32-bit Windows

To build executables for Windows 32-bit, install the following dependencies:

    sudo apt install g++-mingw-w64-i686 mingw-w64-i686-dev

For Ubuntu Bionic 18.04 and Windows Subsystem for Linux <sup>[1](#footnote1)</sup>:

    sudo update-alternatives --config i686-w64-mingw32-g++  # Set the default mingw32 g++ compiler option to posix.

Note that for WSL the Bitcoin Core source path MUST be somewhere in the default mount file system, for
example /usr/src/bitcoin, AND not under /mnt/d/. If this is not the case the dependency autoconf scripts will fail.
This means you cannot use a directory that located directly on the host Windows file system to perform the build.

Acquire the source in the usual way:

    git clone https://github.com/bitcoin/bitcoin.git

Then build using:

    PATH=$(echo "$PATH" | sed -e 's/:\/mnt.*//g') # strip out problematic Windows %PATH% imported var
    cd depends
    make HOST=i686-w64-mingw32
    cd ..
    ./autogen.sh # not required when building from tarball
    CONFIG_SITE=$PWD/depends/i686-w64-mingw32/share/config.site ./configure --prefix=/
    make

## Depends system

For further documentation on the depends system see [README.md](../depends/README.md) in the depends directory.

Installation
-------------

After building using the Windows subsystem it can be useful to copy the compiled
executables to a directory on the Windows drive in the same directory structure
as they appear in the release `.zip` archive. This can be done in the following
way. This will install to `c:\workspace\bitcoin`, for example:

    make install DESTDIR=/mnt/c/workspace/bitcoin

Footnotes
---------

<a name="footnote1">1</a>: Starting from Ubuntu Xenial 16.04 both the 32 and 64 bit Mingw-w64 packages install two different
compiler options to allow a choice between either posix or win32 threads. The default option is win32 threads which is the more
efficient since it will result in binary code that links directly with the Windows kernel32.lib. Unfortunately, the headers
required to support win32 threads conflict with some of the classes in the C++11 standard library in particular std::mutex.
It's not possible to build the Bitcoin Core code using the win32 version of the Mingw-w64 cross compilers (at least not without
modifying headers in the Bitcoin Core source code).
