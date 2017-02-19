WINDOWS BUILD NOTES
====================

Below are some notes on how to build Bitcoin Core for Windows.

Most developers use cross-compilation from Ubuntu to build executables for
Windows. This is also used to build the release binaries.

While there are potentially a number of ways to build on Windows (for example using msys / mingw-w64),
using the Windows Subsystem For Linux is the most straightforward. If you are building with
another method, please contribute the instructions here for others who are running versions
of Windows that are not compatible with the Windows Subsystem for Linux.

Compiling with Windows Subsystem For Linux
-------------------------------------------

With Windows 10, Microsoft has released a new feature named the [Windows
Subsystem for Linux](https://msdn.microsoft.com/commandline/wsl/about). This
feature allows you to run a bash shell directly on Windows in an Ubuntu-based
environment. Within this environment you can cross compile for Windows without
the need for a separate Linux VM or server.

This feature is not supported in versions of Windows prior to Windows 10 or on
Windows Server SKUs. In addition, it is available [only for 64-bit versions of
Windows](https://msdn.microsoft.com/en-us/commandline/wsl/install_guide).

To get the bash shell, you must first activate the feature in Windows.

1. Turn on Developer Mode
  * Open Settings -> Update and Security -> For developers
  * Select the Developer Mode radio button
  * Restart if necessary
2. Enable the Windows Subsystem for Linux feature
  * From Start, search for "Turn Windows features on or off" (type 'turn')
  * Select Windows Subsystem for Linux (beta)
  * Click OK
  * Restart if necessary
3. Complete Installation
  * Open a cmd prompt and type "bash"
  * Accept the license
  * Create a new UNIX user account (this is a separate account from your Windows account)

After the bash shell is active, you can follow the instructions below, starting
with the "Cross-compilation" section. Compiling the 64-bit version is
recommended but it is possible to compile the 32-bit version.

Cross-compilation
-------------------

These steps can be performed on, for example, an Ubuntu VM. The depends system
will also work on other Linux distributions, however the commands for
installing the toolchain will be different.

First, install the general dependencies:

    sudo apt-get install build-essential libtool autotools-dev automake pkg-config bsdmainutils curl

A host toolchain (`build-essential`) is necessary because some dependency
packages (such as `protobuf`) need to build host utilities that are used in the
build process.

## Building for 64-bit Windows

To build executables for Windows 64-bit, install the following dependencies:

    sudo apt-get install g++-mingw-w64-x86-64 mingw-w64-x86-64-dev

Then build using:

    cd depends
    make HOST=x86_64-w64-mingw32
    cd ..
    ./autogen.sh # not required when building from tarball
    CONFIG_SITE=$PWD/depends/x86_64-w64-mingw32/share/config.site ./configure --prefix=/
    make

## Building for 32-bit Windows

To build executables for Windows 32-bit, install the following dependencies:

    sudo apt-get install g++-mingw-w64-i686 mingw-w64-i686-dev 

Then build using:

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
executables to a directory on the windows drive in the same directory structure
as they appear in the release `.zip` archive. This can be done in the following
way. This will install to `c:\workspace\bitcoin`, for example:

    make install DESTDIR=/mnt/c/workspace/bitcoin
