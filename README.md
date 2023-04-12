Bitcoin Core integration/staging tree
=====================================

https://bitcoincore.org

For an immediately usable, binary version of the Bitcoin Core software, see
https://bitcoincore.org/en/download/.

What is Bitcoin Core?
---------------------

Bitcoin Core connects to the Bitcoin peer-to-peer network to download and fully
validate blocks and transactions. It also includes a wallet and graphical user
interface, which can be optionally built.

Further information about Bitcoin Core is available in the [doc folder](/doc).

Steps to compile Bitcoin Core in Windows
----------------------------------------
1. Open Command Prompt.   
2. Install `WSL` (Windows Subsystem for Linux) by using command    
   `wsl --install`   
   For more information on WSL refer https://learn.microsoft.com/en-us/windows/wsl/install
3. Install the general dependencies using commands:   
    `sudo apt update`    
    `sudo apt upgrade`      
    `sudo apt install build-essential libtool autotools-dev automake pkg-config bsdmainutils curl git`   
4. If you want to build the windows installer with make deploy you need NSIS:     
    `sudo apt install nsis`
5. Acquire the source in the usual way:   
   `git clone https://github.com/bitcoin/bitcoin.git`  
   `cd bitcoin`   
6. Install the mingw-w64 cross-compilation tool chain:   
   on modern systems (Ubuntu 21.04 Hirsute Hippo or newer, Debian 11 Bullseye or newer) using the command:   
   `sudo apt install g++-mingw-w64-x86-64-posix`   
   on older systems using the command:   
   `sudo apt install g++-mingw-w64-x86-64`   
   
   Once the toolchain is installed the build steps are common:    
   Note that for WSL the Bitcoin Core source path MUST be somewhere in the default mount file system, for example /usr/src/bitcoin, AND not under /mnt/d/. If this is    not the case the dependency autoconf scripts will fail. This means you cannot use a directory that is located directly on the host Windows file system to perform    the build.
7. Build using:   
   `PATH=$(echo "$PATH" | sed -e 's/:\/mnt.*//g') # strip out problematic Windows %PATH% imported var`   
   `sudo bash -c "echo 0 > /proc/sys/fs/binfmt_misc/status" # Disable WSL support for Win32 applications.`   
   `cd depends`   
   `make HOST=x86_64-w64-mingw32`    
   `cd ..`    
   `./autogen.sh`    
   `CONFIG_SITE=$PWD/depends/x86_64-w64-mingw32/share/config.site ./configure --prefix=/`   
   `make # use "-j N" for N parallel jobs`   
   `sudo bash -c "echo 1 > /proc/sys/fs/binfmt_misc/status" # Enable WSL support for Win32 applications.`   
   
 8. After building using the Windows subsystem it can be useful to copy the compiled executables to a directory on the Windows drive in the same directory structure     as they appear in the release .zip archive. This can be done in the following way. This will install to c:\workspace\bitcoin, for example:   
    `make install DESTDIR=/mnt/c/workspace/bitcoin`
 9. You can also create an installer using:   
    `make deploy`
  
License
-------

Bitcoin Core is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

Development Process
-------------------

The `master` branch is regularly built (see `doc/build-*.md` for instructions) and tested, but it is not guaranteed to be
completely stable. [Tags](https://github.com/bitcoin/bitcoin/tags) are created
regularly from release branches to indicate new official, stable release versions of Bitcoin Core.

The https://github.com/bitcoin-core/gui repository is used exclusively for the
development of the GUI. Its master branch is identical in all monotree
repositories. Release branches and tags do not exist, so please do not fork
that repository unless it is for development reasons.

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md)
and useful hints for developers can be found in [doc/developer-notes.md](doc/developer-notes.md).

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write [unit tests](src/test/README.md) for new code, and to
submit new unit tests for old code. Unit tests can be compiled and run
(assuming they weren't disabled in configure) with: `make check`. Further details on running
and extending unit tests can be found in [/src/test/README.md](/src/test/README.md).

There are also [regression and integration tests](/test), written
in Python.
These tests can be run (if the [test dependencies](/test) are installed) with: `test/functional/test_runner.py`

The CI (Continuous Integration) systems make sure that every pull request is built for Windows, Linux, and macOS,
and that unit/sanity tests are run automatically.

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.

Translations
------------

Changes to translations as well as new translations can be submitted to
[Bitcoin Core's Transifex page](https://www.transifex.com/bitcoin/bitcoin/).

Translations are periodically pulled from Transifex and merged into the git repository. See the
[translation process](doc/translation_process.md) for details on how this works.

**Important**: We do not accept translation changes as GitHub pull requests because the next
pull from Transifex would automatically overwrite them again.
