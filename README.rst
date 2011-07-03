Bitcoin-qt: Qt4 based GUI replacement for Bitcoin
=================================================

**Warning** **Warning** **Warning**

Pre-alpha stuff! I'm using this client myself on the production network, and I haven't noticed any glitches, but remember: always backup your wallet! Testing on the testnet is recommended.

This has been implemented:

- qmake / QtCreator project (.pro)

- All dialogs (main GUI, address book, send coins) and menus

- Taskbar icon/menu

- GUI only functionality (copy to clipboard, select address, address/transaction filter proxys)

- Bitcoin core is made compatible with Qt4

- Send coins dialog: address and input validation

- Address book and transactions views and models

- Options dialog

- Sending coins (including ask for fee when needed)

- Show error messages from core

- Show details dialog for transactions (on double click)

This has to be done:

- Integrate with main bitcoin tree

- Start at system start

- Internationalization (convert WX language files)

- Build on Windows

Build instructions
===================

First, make sure that the required packages for Qt4 development of your
distribution are installed, for Debian and Ubuntu these are:

::

    apt-get install qt4-qmake libqt4-dev build-essential libboost-dev libboost-system-dev \
        libboost-filesystem-dev libboost-program-options-dev libboost-thread-dev \
        libssl-dev libdb4.8++-dev

then execute the following:

::

    qmake
    make

Alternatively, install Qt Creator and open the `bitcoin-qt.pro` file.

An executable named `bitcoin-qt` will be built.

Berkely DB version warning
==========================

A warning for people using the *static binary* version of Bitcoin (tl;dr: **Berkely DB databases are not forward compatible**).

The static binary version of Bitcoin is linked against libdb4.7 or libdb4.8 (see also `this Debian issue`_).

Now the nasty thing is that databases from 5.X are not compatible with 4.X. 

If the globally installed development package of Berkely DB installed on your system is 5.X, any source you
build yourself will be linked against that. The first time you run with a 5.X version the database will be upgraded, 
and 4.X cannot open the new format. This means that you cannot go back to the old statically linked version without
significant hassle!

.. _`this Debian issue`: http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=621425
