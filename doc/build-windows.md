WINDOWS BUILD NOTES
====================

Below are some notes on how to build Bitcoin Core for Windows.

Most developers use cross-compilation from Ubuntu to build executables for
Windows. This is also used to build the release binaries.

While there are potentially a number of ways to build on Windows (for example using msys / mingw-w64),
using the Windows Subsystem For Linux is the most straight forward.  If you are building with
an alternative method, please contribute the instructions here for others who are running versions
of Windows that are not compatible with the Windows Subsystem for Linux.

Compiling with the Windows Subsystem For Linux
-------------------

With Windows 10, Microsoft has released a new feature named the
[Windows Subsystem for Linux](https://msdn.microsoft.com/commandline/wsl/about).  This feature allows you to run a bash shell directly on Windows in an Ubuntu based
environment.  Within this environment you can cross compile for Windows without the need for a separate Linux VM or Server.

This feature is not supported in versions of Windows prior to Windows 10 or on Windows Server SKUs.

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

After the bash shell is active, you can follow the instructions below for Windows 64-bit Cross-compilation.
When building dependencies within the 'depends' folder, you may encounter an error building
the protobuf dependency.  If this occurs, re-run the command with sudo.  This is likely
a bug with the Windows Subsystem for Linux feature and may be fixed with a future update.

Cross-compilation
-------------------

These steps can be performed on, for example, an Ubuntu VM. The depends system
will also work on other Linux distributions, however the commands for
installing the toolchain will be different.

Make sure you install the build requirements mentioned in
[build-unix.md](/doc/build-unix.md).
Then, install the toolchains and curl:

    sudo apt-get install g++-mingw-w64-i686 mingw-w64-i686-dev g++-mingw-w64-x86-64 mingw-w64-x86-64-dev curl

To build executables for Windows 32-bit:

    cd depends
    make HOST=i686-w64-mingw32 -j4
    cd ..
    ./autogen.sh # not required when building from tarball
    ./configure --prefix=`pwd`/depends/i686-w64-mingw32
    make

To build executables for Windows 64-bit:

    cd depends
    make HOST=x86_64-w64-mingw32 -j4
    cd ..
    ./autogen.sh # not required when building from tarball
    ./configure --prefix=`pwd`/depends/x86_64-w64-mingw32
    make

For further documentation on the depends system see [README.md](../depends/README.md) in the depends directory.
