WINDOWS BUILD NOTES
====================

Below are some notes on how to build Bitcoin Core for Windows.

The options known to work for building Bitcoin Core on Windows are:

* On Linux, using the [Mingw-w64](https://www.mingw-w64.org/) cross compiler tool chain.
* On Windows, using [Windows Subsystem for Linux (WSL)](https://docs.microsoft.com/windows/wsl/about) and Mingw-w64.
* On Windows, using [Microsoft Visual Studio](https://www.visualstudio.com). See [README.md](/build_msvc/README.md).

Other options which may work, but which have not been extensively tested are (please contribute instructions):

* On Windows, using a POSIX compatibility layer application such as [cygwin](https://www.cygwin.com/) or [msys2](https://www.msys2.org/).

Installing Windows Subsystem for Linux
---------------------------------------

Follow the upstream installation instructions, available [here](https://docs.microsoft.com/windows/wsl/install-win10).

Cross-compilation for Ubuntu and Windows Subsystem for Linux
------------------------------------------------------------

The steps below can be performed on Ubuntu or WSL. The depends system
will also work on other Linux distributions, however the commands for
installing the toolchain will be different.

First, install the general dependencies:

```sh
  sudo apt update
```

```sh
  sudo apt upgrade
```

```sh
  sudo apt install build-essential libtool autotools-dev automake pkg-config bsdmainutils curl git
```

A host toolchain (`build-essential`) is necessary because some dependency
packages need to build host utilities that are used in the build process.

See [dependencies.md](dependencies.md) for a complete overview.

If you want to build the windows installer with `make deploy` you need [NSIS](https://nsis.sourceforge.io/Main_Page):

```sh
  sudo apt install nsis
```

Acquire the source in the usual way:

```sh
  git clone https://github.com/bitcoin/bitcoin.git
```

```sh
  cd bitcoin
```

## Building for 64-bit Windows

The first step is to install the mingw-w64 cross-compilation tool chain:
  - on modern systems (Ubuntu 21.04 Hirsute Hippo or newer, Debian 11 Bullseye or newer):

```sh
  sudo apt install g++-mingw-w64-x86-64-posix
```

  - on older systems:

```sh
  sudo apt install g++-mingw-w64-x86-64
```

Once the toolchain is installed the build steps are common:

Note that for WSL the Bitcoin Core source path MUST be somewhere in the default mount file system, for
example /usr/src/bitcoin, AND not under /mnt/d/. If this is not the case the dependency autoconf scripts will fail.
This means you cannot use a directory that is located directly on the host Windows file system to perform the build.

Additional WSL Note: WSL support for [launching Win32 applications](https://docs.microsoft.com/en-us/archive/blogs/wsl/windows-and-ubuntu-interoperability#launching-win32-applications-from-within-wsl)
results in `Autoconf` configure scripts being able to execute Windows Portable Executable files. This can cause
unexpected behaviour during the build, such as Win32 error dialogs for missing libraries. The recommended approach
is to temporarily disable WSL support for Win32 applications.

Build using:

```sh
  PATH=$(echo "$PATH" | sed -e 's/:\/mnt.*//g') # strip out problematic Windows %PATH% imported var
```

```sh
  sudo bash -c "echo 0 > /proc/sys/fs/binfmt_misc/status" # Disable WSL support for Win32 applications.
```

```sh
  cd depends
```

```sh
  make HOST=x86_64-w64-mingw32
```

```sh
  cd ..
```

```sh
  ./autogen.sh
```

```sh
  CONFIG_SITE=$PWD/depends/x86_64-w64-mingw32/share/config.site ./configure --prefix=/
```

```sh
  make # use "-j N" for N parallel jobs
```

```sh
  sudo bash -c "echo 1 > /proc/sys/fs/binfmt_misc/status" # Enable WSL support for Win32 applications.
```

## Depends system

For further documentation on the depends system see [README.md](../depends/README.md) in the depends directory.

Installation
-------------

After building using the Windows subsystem it can be useful to copy the compiled
executables to a directory on the Windows drive in the same directory structure
as they appear in the release `.zip` archive. This can be done in the following
way. This will install to `c:\workspace\bitcoin`, for example:

```sh
  make install DESTDIR=/mnt/c/workspace/bitcoin
```

You can also create an installer using:

```sh
  make deploy
```

Compilation for 64-bit Windows with msys2
------------------------------------------------------------

Please download the installer here: http://msys2.github.io/

#### Execute:
```sh
  C:\msys64\msys2.exe
```

#### Install
```sh
  pacman -Sy
```

```sh
  pacman -Su
```

```sh
  pacman -S --needed filesystem msys2-runtime bash
```

```sh
  pacman -S base-devel gcc make cmake git
```

```sh
  pacman -S autoconf autotools automake libtool
```

```sh
  pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-pkgconf
```

```sh
  pacman -S mingw-w64-x86_64-boost mingw-w64-x86_64-nsis
```

```sh
  pacman -S mingw-w64-x86_64-libevent
```

```sh
  pacman -S mingw-w64-x86_64-db
```

```sh
  pacman -S mingw-w64-x86_64-miniupnpc
```

```sh
  pacman -S mingw-w64-x86_64-ccache
```

#### Execute:
```sh
  C:\msys64\mingw64.exe
```

#### Build using:
```sh
  ./autogen.sh
```

```sh
  ./configure --enable-ccache --without-gui
```

```sh
  make -J $(nproc)
```

```sh
  make check
```

```sh
  make install DESTDIR=/mnt/c/workspace/bitcoin
```
