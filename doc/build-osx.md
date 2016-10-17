Mac OS X Build Instructions and Notes
====================================
<<<<<<< HEAD
This guide will show you how to build dashd (headless client) for OSX.
=======
This guide will show you how to build crowncoind(headless client) for OSX.
>>>>>>> origin/dirty-merge-dash-0.11.0

Notes
-----

* Tested on OS X 10.7 through 10.10 on 64-bit Intel processors only.

* All of the commands should be executed in a Terminal application. The
built-in one is located in `/Applications/Utilities`.

Preparation
-----------

You need to install XCode with all the options checked so that the compiler
and everything is available in /usr not just /Developer. XCode should be
available on your OS X installation media, but if not, you can get the
current version from https://developer.apple.com/xcode/. If you install
Xcode 4.3 or later, you'll need to install its command line tools. This can
be done in `Xcode > Preferences > Downloads > Components` and generally must
be re-done or updated every time Xcode is updated.

There's also an assumption that you already have `git` installed. If
not, it's the path of least resistance to install [Github for Mac](https://mac.github.com/)
(OS X 10.7+) or
[Git for OS X](https://code.google.com/p/git-osx-installer/). It is also
available via Homebrew.

You will also need to install [Homebrew](http://brew.sh) in order to install library
dependencies.

The installation of the actual dependencies is covered in the Instructions
sections below.

Instructions: MacPorts
----------------------

### Install dependencies

Installing the dependencies using MacPorts is very straightforward.

    sudo port install boost db48@+no_java openssl miniupnpc autoconf pkgconfig automake

Optional: install Qt4

    sudo port install qt4-mac qrencode protobuf-cpp

### Building `crowncoind`

1. Clone the github tree to get the source code and go into the directory.

        git clone git@github.com:crowncoin/crowncoin.git crowncoin
        cd crowncoin

2.  Build crowncoind (and Crowncoin-Qt, if configured):

        ./autogen.sh
        ./configure
        make

3.  It is a good idea to build and run the unit tests, too:

        make check

Instructions: Homebrew
----------------------

#### Install dependencies using Homebrew

        brew install autoconf automake libtool boost miniupnpc openssl pkg-config protobuf qt

#### Installing berkeley-db4 using Homebrew

The homebrew package for berkeley-db4 has been broken for some time.  It will install without Java though.

Running this command takes you into brew's interactive mode, which allows you to configure, make, and install by hand:
```
$ brew install https://raw.github.com/mxcl/homebrew/master/Library/Formula/berkeley-db4.rb -–without-java 
```

The rest of these commands are run inside brew interactive mode:
```
/private/tmp/berkeley-db4-UGpd0O/db-4.8.30 $ cd ..
/private/tmp/berkeley-db4-UGpd0O $ db-4.8.30/dist/configure --prefix=/usr/local/Cellar/berkeley-db4/4.8.30 --mandir=/usr/local/Cellar/berkeley-db4/4.8.30/share/man --enable-cxx
/private/tmp/berkeley-db4-UGpd0O $ make
/private/tmp/berkeley-db4-UGpd0O $ make install
/private/tmp/berkeley-db4-UGpd0O $ exit
```

After exiting, you'll get a warning that the install is keg-only, which means it wasn't symlinked to `/usr/local`.  You don't need it to link it to build dash, but if you want to, here's how:

    $ brew link --force berkeley-db4


<<<<<<< HEAD
### Building `dashd`

1. Clone the github tree to get the source code and go into the directory.

        git clone https://github.com/dashpay/dash.git
        cd dash

2.  Build dashd:
=======
### Building `crowncoind`

1. Clone the github tree to get the source code and go into the directory.

        git clone https://github.com/Climbee/crowncoin.git
        cd crowncoin

2.  Build crowncoind:
>>>>>>> origin/dirty-merge-dash-0.11.0

        ./autogen.sh
        ./configure
        make

3.  It is also a good idea to build and run the unit tests:

        make check

4.  (Optional) You can also install dashd to your path:

        make install

Use Qt Creator as IDE
------------------------
You can use Qt Creator as IDE, for debugging and for manipulating forms, etc.
Download Qt Creator from http://www.qt.io/download/. Download the "community edition" and only install Qt Creator (uncheck the rest during the installation process).

1. Make sure you installed everything through homebrew mentioned above 
2. Do a proper ./configure --with-gui=qt5 --enable-debug
3. In Qt Creator do "New Project" -> Import Project -> Import Existing Project
4. Enter "dash-qt" as project name, enter src/qt as location
5. Leave the file selection as it is
6. Confirm the "summary page"
7. In the "Projects" tab select "Manage Kits..."
8. Select the default "Desktop" kit and select "Clang (x86 64bit in /usr/bin)" as compiler
9. Select LLDB as debugger (you might need to set the path to your installtion)
10. Start debugging with Qt Creator

Creating a release build
------------------------
<<<<<<< HEAD
You can ignore this section if you are building `dashd` for your own use.

dashd/dash-cli binaries are not included in the Dash-Qt.app bundle.

If you are building `dashd` or `Dash-Qt` for others, your build machine should be set up
=======
You can ignore this section if you are building `crowncoind` for your own use.

crowncoind/crowncoin-cli binaries are not included in the Crowncoin-Qt.app bundle.

If you are building `crowncoind` or `Crowncoin-Qt` for others, your build machine should be set up
>>>>>>> origin/dirty-merge-dash-0.11.0
as follows for maximum compatibility:

All dependencies should be compiled with these flags:

 -mmacosx-version-min=10.7
 -arch x86_64
 -isysroot $(xcode-select --print-path)/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk

<<<<<<< HEAD
Once dependencies are compiled, see release-process.md for how the Dash-Qt.app
=======
For MacPorts, that means editing your macports.conf and setting
`macosx_deployment_target` and `build_arch`:

    macosx_deployment_target=10.6
    build_arch=x86_64

... and then uninstalling and re-installing, or simply rebuilding, all ports.

As of December 2012, the `boost` port does not obey `macosx_deployment_target`.
Download `http://gavinandresen-crowncoin.s3.amazonaws.com/boost_macports_fix.zip`
for a fix.

Once dependencies are compiled, see release-process.md for how the Crowncoin-Qt.app
>>>>>>> origin/dirty-merge-dash-0.11.0
bundle is packaged and signed to create the .dmg disk image that is distributed.

Running
-------

<<<<<<< HEAD
It's now available at `./dashd`, provided that you are still in the `src`
directory. We have to first create the RPC configuration file, though.

Run `./dashd` to get the filename where it should be put, or just try these
commands:

    echo -e "rpcuser=dashrpc\nrpcpassword=$(xxd -l 16 -p /dev/urandom)" > "/Users/${USER}/Library/Application Support/Dash/dash.conf"
    chmod 600 "/Users/${USER}/Library/Application Support/Dash/dash.conf"
=======
It's now available at `./crowncoind`, provided that you are still in the `src`
directory. We have to first create the RPC configuration file, though.

Run `./crowncoind` to get the filename where it should be put, or just try these
commands:

    echo -e "rpcuser=crowncoinrpc\nrpcpassword=$(xxd -l 16 -p /dev/urandom)" > "/Users/${USER}/Library/Application Support/Crowncoin/crowncoin.conf"
    chmod 600 "/Users/${USER}/Library/Application Support/Crowncoin/crowncoin.conf"
>>>>>>> origin/dirty-merge-dash-0.11.0

The next time you run it, it will start downloading the blockchain, but it won't
output anything while it's doing this. This process may take several hours;
you can monitor its process by looking at the debug.log file, like this:

<<<<<<< HEAD
    tail -f $HOME/Library/Application\ Support/Dash/debug.log
=======
    tail -f $HOME/Library/Application\ Support/Crowncoin/debug.log
>>>>>>> origin/dirty-merge-dash-0.11.0

Other commands:
-------

<<<<<<< HEAD
    ./dashd -daemon # to start the dash daemon.
    ./dash-cli --help  # for a list of command-line options.
    ./dash-cli help    # When the daemon is running, to get a list of RPC commands
=======
    ./crowncoind -daemon # to start the crowncoin daemon.
    ./crowncoin-cli --help  # for a list of command-line options.
    ./crowncoin-cli help    # When the daemon is running, to get a list of RPC commands
>>>>>>> origin/dirty-merge-dash-0.11.0
