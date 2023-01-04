WINDOWS BUILD NOTES
====================

Below are some notes on how to build Dash Core for Windows.

The options known to work for building Dash Core on Windows are:

* On Linux, using the [Mingw-w64](https://www.mingw-w64.org/) cross compiler tool chain.
* On Windows, using [Windows Subsystem for Linux (WSL)](https://learn.microsoft.com/en-us/windows/wsl/about) and Mingw-w64.

Other options which may work, but which have not been extensively tested are (please contribute instructions):

* On Windows, using a POSIX compatibility layer application such as [cygwin](https://www.cygwin.com/) or [msys2](https://www.msys2.org/).

The instructions below work on Ubuntu and Debian. Make sure the distribution's `g++-mingw-w64-x86-64-posix`
package meets the minimum required `g++` version specified in [dependencies.md](dependencies.md).

Installing Windows Subsystem for Linux
---------------------------------------

Follow the upstream installation instructions, available [here](https://learn.microsoft.com/en-us/windows/wsl/install).

Cross-compilation
-------------------

The steps below can be performed on Ubuntu or WSL. The depends system
will also work on other Linux distributions, however the commands for
installing the toolchain will be different.

See [README.md](../depends/README.md) in the depends directory for which
dependencies to install and [dependencies.md](dependencies.md) for a complete overview.

Note that for WSL the Dash Core source path MUST be somewhere in the default mount file system, for
example /usr/src/dash, AND not under /mnt/d/. If this is not the case the dependency autoconf scripts will fail.
This means you cannot use a directory that is located directly on the host Windows file system to perform the build.

Additional WSL Note: WSL support for [launching Win32 applications](https://learn.microsoft.com/en-us/archive/blogs/wsl/windows-and-ubuntu-interoperability#launching-win32-applications-from-within-wsl)
results in `Autoconf` configure scripts being able to execute Windows Portable Executable files. This can cause
unexpected behaviour during the build, such as Win32 error dialogs for missing libraries. The recommended approach
is to temporarily disable WSL support for Win32 applications.

Build using:

    sudo bash -c "echo 0 > /proc/sys/fs/binfmt_misc/status" # Disable WSL support for Win32 applications.
    cd depends
    make HOST=x86_64-w64-mingw32
    cd ..
    ./autogen.sh
    CONFIG_SITE=$PWD/depends/x86_64-w64-mingw32/share/config.site ./configure --prefix=/
    make # use "-j N" for N parallel jobs
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
