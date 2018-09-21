Mac OS X Build Instructions and Notes
====================================
The commands in this guide should be executed in a Terminal application.
The built-in one is located in `/Applications/Utilities/Terminal.app`.

Preparation
-----------
Install the OS X command line tools:

`xcode-select --install`

When the popup appears, click `Install`.

Then install [Homebrew](https://brew.sh).

Dependencies
----------------------

    brew install automake berkeley-db4 libtool boost --c++11 miniupnpc openssl pkg-config protobuf python3 qt libevent

See [dependencies.md](dependencies.md) for a complete overview.

If you want to build the disk image with `make deploy` (.dmg / optional), you need RSVG

    brew install librsvg

NOTE: Building with Qt4 is still supported, however, could result in a broken UI. Building with Qt5 is recommended.

NOTE: At this time it is highly recommended that developers wishing to compile the Raven Core binaries **DO NOT** upgrade to 
OS X Mojave Beta 10.14.  Currently there is a compatibility issue with OS X Mojave, Command-Line-Tools 10.0.0 (clang), and 
Berkeley-db version 4.8.3.  Binaries compiled using this combination will crash with a segmentation-fault during initialization. 
Binaries compiled by previous versions will run on OS X Mojave with no-known issues.  It is possible to work-around this issue by 
upgrading Berkeley-db to version 18.1.25 or newer (currently 18.1.25 is the only known version to work).  To compile and run with 
newer versions of Berkeley-db it is recommended that Berkeley-db 4.8.3 be uninstalled and the latest version installed.  There are 
unknown wallet compatability ramifications to this solution so it is highly recommended that any local wallets be backed-up before
opening them using binaries compiled with this solution.

Use the following commands to compile a working version of Raven Core on Mojave (assuming that the instructions in the section "Build 
Raven Core" has already been followed).  Uninstall Berkeley-db 4.8.3, install the latest version, and _configure_ with the 
incompatible-bdb flag:

    brew remove berkeley-db@4
    brew install bekeley-db
    ./autogen.sh
    ./configure --with-incompatible-bdb
    make


Build Raven Core
------------------------

1. Clone the raven source code and cd into `raven`

        git clone https://github.com/RavenProject/Ravencoin
        cd Ravencoin

2.  Build raven-core:

    Configure and build the headless raven binaries as well as the GUI (if Qt is found).

    You can disable the GUI build by passing `--without-gui` to configure.

        ./autogen.sh
        ./configure
        make

3.  It is recommended to build and run the unit tests:

        make check

4.  You can also create a .dmg that contains the .app bundle (optional):

        make deploy

Running
-------

Raven Core is now available at `./src/ravend`

Before running, it's recommended you create an RPC configuration file.

    echo -e "rpcuser=ravenrpc\nrpcpassword=$(xxd -l 16 -p /dev/urandom)" > "/Users/${USER}/Library/Application Support/Raven/raven.conf"

    chmod 600 "/Users/${USER}/Library/Application Support/Raven/raven.conf"

The first time you run ravend, it will start downloading the blockchain. This process could take several hours.

You can monitor the download process by looking at the debug.log file:

    tail -f $HOME/Library/Application\ Support/Raven/debug.log

Other commands:
-------

    ./src/ravend -daemon # Starts the raven daemon.
    ./src/raven-cli --help # Outputs a list of command-line options.
    ./src/raven-cli help # Outputs a list of RPC commands when the daemon is running.

Using Qt Creator as IDE
------------------------
You can use Qt Creator as an IDE, for raven development.
Download and install the community edition of [Qt Creator](https://www.qt.io/download/).
Uncheck everything except Qt Creator during the installation process.

1. Make sure you installed everything through Homebrew mentioned above
2. Do a proper ./configure --enable-debug
3. In Qt Creator do "New Project" -> Import Project -> Import Existing Project
4. Enter "raven-qt" as project name, enter src/qt as location
5. Leave the file selection as it is
6. Confirm the "summary page"
7. In the "Projects" tab select "Manage Kits..."
8. Select the default "Desktop" kit and select "Clang (x86 64bit in /usr/bin)" as compiler
9. Select LLDB as debugger (you might need to set the path to your installation)
10. Start debugging with Qt Creator

Notes
-----

* Tested on OS X 10.8 through 10.14 on 64-bit Intel processors only.

* Building with downloaded Qt binaries is not officially supported. 

* autoreconf (boost issue)
