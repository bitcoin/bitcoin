Dash Core staging tree 0.11
===============================

http://www.dashpay.io

Copyright (c) 2009-2015 Bitcoin Core Developers

Copyright (c) 2014-2015 Dash Core Developers


What is Dash?
----------------

Dash is an experimental new digital currency that enables anonymous, instant
payments to anyone, anywhere in the world. Dash uses peer-to-peer technology
to operate with no central authority: managing transactions and issuing money
are carried out collectively by the network. Dash Core is the name of open
source software which enables the use of this currency.

For more information, as well as an immediately useable, binary version of
the Dash Core software, see http://www.dashpay.io/downloads.


License
-------

Dash Core is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see http://opensource.org/licenses/MIT.


Building process
-----------------

**compiling Dash from git**

Use the autogen script to prepare the build environment.

    ./autogen.sh
    ./configure
    make

**precompiled binaries**

Precompiled binaries are available at github, see
https://github.com/dashproject/dash-binaries

Always verify the signatures and checksums.


Development tips and tricks
---------------------------

**compiling for debugging**

Run configure with the --enable-debug option, then make. Or run configure with
CXXFLAGS="-g -ggdb -O0" or whatever debug flags you need.

**debug.log**

If the code is behaving strangely, take a look in the debug.log file in the data directory;
error and debugging message are written there.

The -debug=... command-line option controls debugging; running with just -debug will turn
on all categories (and give you a very large debug.log file).

The Qt code routes qDebug() output to debug.log under category "qt": run with -debug=qt
to see it.

**testnet and regtest modes**

Run with the -testnet option to run with "play dashs" on the test network, if you
are testing multi-machine code that needs to operate across the internet.

If you are testing something that can run on one machine, run with the -regtest option.
In regression test mode blocks can be created on-demand; see qa/rpc-tests/ for tests
that run in -regtest mode.

**DEBUG_LOCKORDER**

Dash Core is a multithreaded application, and deadlocks or other multithreading bugs
can be very difficult to track down. Compiling with -DDEBUG_LOCKORDER (configure
CXXFLAGS="-DDEBUG_LOCKORDER -g") inserts run-time checks to keep track of what locks
are held, and adds warning to the debug.log file if inconsistencies are detected.
