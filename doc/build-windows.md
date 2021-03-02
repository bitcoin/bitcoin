WINDOWS BUILD NOTES
====================

Below are some notes on how to build Dash Core for Windows.

The options known to work for building Dash Core on Windows are:

* On Linux, using the [Mingw-w64](https://mingw-w64.org/doku.php) cross compiler tool chain. Ubuntu Focal 20.04 is required
and is the platform used to build the Dash Core Windows release binaries.
* On Windows, using [Windows
Subsystem for Linux (WSL)](https://docs.microsoft.com/windows/wsl/about) and the Mingw-w64 cross compiler tool chain.
* On Windows, using a native compiler tool chain such as [Visual Studio](https://www.visualstudio.com).

Other options which may work, but which have not been extensively tested are (please contribute instructions):

* On Windows, using a POSIX compatibility layer application such as [cygwin](https://www.cygwin.com/) or [msys2](https://www.msys2.org/).

Installing Windows Subsystem for Linux
---------------------------------------

With Windows 10, Microsoft has released a new feature named the [Windows
Subsystem for Linux (WSL)](https://docs.microsoft.com/windows/wsl/about). This
feature allows you to run a bash shell directly on Windows in an Ubuntu-based
environment. Within this environment you can cross compile for Windows without
the need for a separate Linux VM or server. Note that while WSL can be installed with
other Linux variants, such as OpenSUSE, the following instructions have only been
tested with Ubuntu.

This feature is not supported in versions of Windows prior to Windows 10 or on
Windows Server SKUs. In addition, it is available [only for 64-bit versions of
Windows](https://docs.microsoft.com/windows/wsl/install-win10).

Full instructions to install WSL are available on the above link.
To install WSL on Windows 10 with Fall Creators Update installed (version >= 16215.0) do the following:

1. Enable the Windows Subsystem for Linux feature
  * Open the Windows Features dialog (`OptionalFeatures.exe`)
  * Enable 'Windows Subsystem for Linux'
  * Click 'OK' and restart if necessary
2. Install Ubuntu
  * Open Microsoft Store and search for "Ubuntu 20.04" or use [this link](https://www.microsoft.com/store/productId/9MTTCL66CPXJ)
  * Click Install
3. Complete Installation
  * Open a cmd prompt and type "Ubuntu2004"
  * Create a new UNIX user account (this is a separate account from your Windows account)

After the bash shell is active, you can follow the instructions below, starting
with the "Cross-compilation" section. Compiling the 64-bit version is
recommended, but it is possible to compile the 32-bit version.

Cross-compilation
-------------------

These steps can be performed on, for example, an Ubuntu VM. The depends system
will also work on other Linux distributions, however the commands for
installing the toolchain will be different.

First, install the general dependencies:

    sudo apt-get install build-essential libtool autotools-dev automake pkg-config bsdmainutils bison curl

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

Additional WSL Note: WSL support for [launching Win32 applications](https://docs.microsoft.com/en-us/archive/blogs/wsl/windows-and-ubuntu-interoperability#launching-win32-applications-from-within-wsl)
results in `Autoconf` configure scripts being able to execute Windows Portable Executable files. This can cause
unexpected behaviour during the build, such as Win32 error dialogs for missing libraries. The recommended approach
is to temporarily disable WSL support for Win32 applications.

Then build using:

    sudo bash -c "echo 0 > /proc/sys/fs/binfmt_misc/status" # Disable WSL support for Win32 applications.
    cd depends
    make HOST=i686-w64-mingw32
    cd ..
    ./autogen.sh
    CONFIG_SITE=$PWD/depends/i686-w64-mingw32/share/config.site ./configure --prefix=/
    make
    sudo bash -c "echo 1 > /proc/sys/fs/binfmt_misc/status" # Enable WSL support for Win32 applications.

## Depends system

For further documentation on the depends system see [README.md](../depends/README.md) in the depends directory.

Installation
-------------

After building using the Windows subsystem it can be useful to copy the compiled
executables to a directory on the Windows drive in the same directory structure
as they appear in the release `.zip` archive. This can be done in the following
way. This will install to `c:\workspace\dash`, for example:

    make install DESTDIR=/mnt/c/workspace/dash

You can also create an installer using:

    make deploy
