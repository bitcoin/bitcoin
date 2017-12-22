Mac OS X Build Instructions and Notes
====================================

Note that Bitcoin Core up to 0.15.1 is available as a MacOS package
and Bitcoin Core 0.14.1 is present in [MacPorts](https://www.macports.org).
These instructions are for those who _want_ to build from source.

The commands in this guide should be executed in a Terminal application.
The built-in one is located in `/Applications/Utilities/Terminal.app`.

Preparation
-----------
Install the OS X command line tools:

`xcode-select --install`

When the popup appears, click `Install`.

Dependencies
----------------------

There are several dependencies that need to be installed
before you try to build Bitcoin Core itself.
See [dependencies.md](dependencies.md) for a complete overview.
If you want to build the disk image with `make deploy`,
you will also need RSVG (librsvg).

You might want to use one of the packaging systems
available for MacOS, such as
[MacPorts](https://www.macports.org),
[Homebrew](https://brew.sh) or
[Fink](http://www.finkproject.org).
to install these dependencies.

For example, this is how you would install the required packages
with MacPort's `port` and Homebrew's `brew`, respectively:

```shell
sudo port install autoconf automake db48 libtool boost miniupnpc openssl pkgconfig protobuf-cpp python36 qt5 libevent zmq
```

```shell
brew install automake berkeley-db4 libtool boost --c++11 miniupnpc openssl pkg-config protobuf python3 qt libevent
```

**Note**: Building with Qt4 is still supported, however,
could result in a broken UI. Building with Qt5 is recommended.

Berkeley DB
-----------
It is recommended to use Berkeley DB 4.8.
If you have to build it yourself, you can use
[the installation script included in contrib/](/contrib/install_db4.sh).
Run `./contrib/install_db4.sh` from the root of the repository.

**Note**: You only need Berkeley DB if the wallet is enabled.

Build Bitcoin Core
------------------------

1. Clone the bitcoin source code and cd into `bitcoin`

        git clone https://github.com/bitcoin/bitcoin
        cd bitcoin

2.  Build bitcoin-core:

    Configure and build the headless bitcoin binaries
    as well as the GUI (if Qt is found).
    You can disable the GUI build by passing `--without-gui` to `./configure`,
    and you can disable the wallet with `--disable-wallet`.
    The defaut `--prefix` is `/usr/local`.
    Check the other options with `./configure --help`.

        ./autogen.sh
        ./configure
        make

3.  It is recommended to build and run the unit tests:

        make check

4.  Install the compiled software:

	sudo make install

5.  You can also create a .dmg that contains the .app bundle (optional):

	sudo port install librsvg
        make deploy

Running
-------

Bitcoin Core is now available as `bitcoind`.
Before running, it's recommended you create an RPC configuration file.
in `${HOME}/Library/Application Support/Bitcoin/bitcoin.conf`
(remember to `chmod 600` the file) containing

```
rpcuser=bitcoinrpc
rpcpassword=PASSWORD
```

You can get a good password by running e.g. `xxd -l 16 -p /dev/random`.

The first time you run bitcoind, it will start downloading the blockchain.
This process could take several hours or days. You can monitor the download
process by looking at the debug.log file:

    tail -f $HOME/Library/Application\ Support/Bitcoin/debug.log

Other commands:
-------

    ./src/bitcoind -daemon # Starts the bitcoin daemon.
    ./src/bitcoin-cli --help # Outputs a list of command-line options.
    ./src/bitcoin-cli help # Outputs a list of RPC commands when the daemon is running.

Using Qt Creator as IDE
------------------------
You can use Qt Creator as an IDE, for bitcoin development.
Download and install the community edition of [Qt Creator](https://www.qt.io/download/).
Uncheck everything except Qt Creator during the installation process.

1. Make sure you installed the dependencies mentioned above
2. Do a proper ./configure --enable-debug
3. In Qt Creator do "New Project" -> Import Project -> Import Existing Project
4. Enter "bitcoin-qt" as project name, enter src/qt as location
5. Leave the file selection as it is
6. Confirm the "summary page"
7. In the "Projects" tab select "Manage Kits..."
8. Select the default "Desktop" kit and select "Clang (x86 64bit in /usr/bin)" as compiler
9. Select LLDB as debugger (you might need to set the path to your installation)
10. Start debugging with Qt Creator

Notes
-----

* Tested on OS X 10.8 through 10.13 on 64-bit Intel processors only.

* Building with downloaded Qt binaries is not officially supported. See the notes in [#7714](https://github.com/bitcoin/bitcoin/issues/7714)
