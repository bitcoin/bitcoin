Bitcoin-qt: Qt4 based GUI replacement for Bitcoin
=================================================

**Warning** **Warning** **Warning**

Alpha version! I'm using this client myself on the production network, and I haven't noticed any glitches, but remember: always backup your wallet.
Testing on the testnet is recommended.

This has been implemented:

- qmake / QtCreator project (.pro)

- Compatibility with Linux (both GNOME and KDE), MacOSX and Windows

- All functionality of the original client, including taskbar icon/menu

- Splash screen

- Tabbed interface

- Overview page with current balance, unconfirmed balance, and such

- Better transaction list with status icons, real-time filtering and a context menu

- Asks for confirmation before sending coins, for your own safety

- CSV export of transactions and address book (for Excel bookkeeping)
 
- Shows alternative icon when connected to testnet, so you never accidentally send real coins during testing

- Shows a progress bar on initial block download, so that you don't have to wonder how many blocks it needs to download to be up to date

- Sendmany support, send to multiple recipients at the same time

- Multiple unit support, can show subdivided bitcoins (uBTC, mBTC) for users that like large numbers

- Support for English, German and Dutch languages

- Address books and transaction table can be sorted by any column

This has to be done:

- Start at system start

- Support more languages

Build instructions 
===================

Debian
-------

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


Windows
--------

Windows build instructions:

- Download the `QT Windows SDK`_ and install it. You don't need the Symbian stuff, just the desktop Qt.

- Download and extract the `dependencies archive`_  [#]_, or compile openssl, boost and dbcxx yourself.

- Copy the contents of the folder "deps" to "X:\\QtSDK\\mingw", replace X:\\ with the location where you installed the Qt SDK. Make sure that the contents of "deps\\include" end up in the current "include" directory.

- Open the .pro file in QT creator and build as normal (ctrl-B)

.. _`QT Windows SDK`: http://qt.nokia.com/downloads/sdk-windows-cpp
.. _`dependencies archive`: http://download.visucore.com/bitcoin/qtgui_deps_1.zip
.. [#] PGP signature: http://download.visucore.com/bitcoin/qtgui_deps_1.zip.sig (signed with RSA key ID `610945D0`_)
.. _`610945D0`: http://pgp.mit.edu:11371/pks/lookup?op=get&search=0x610945D0

Berkely DB version warning
==========================

A warning for people using the *static binary* version of Bitcoin on a Linux/UNIX-ish system (tl;dr: **Berkely DB databases are not forward compatible**).

The static binary version of Bitcoin is linked against libdb4.7 or libdb4.8 (see also `this Debian issue`_).

Now the nasty thing is that databases from 5.X are not compatible with 4.X. 

If the globally installed development package of Berkely DB installed on your system is 5.X, any source you
build yourself will be linked against that. The first time you run with a 5.X version the database will be upgraded, 
and 4.X cannot open the new format. This means that you cannot go back to the old statically linked version without
significant hassle!

.. _`this Debian issue`: http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=621425
