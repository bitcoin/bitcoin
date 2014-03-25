Bitcredit 0.9.99.0 BETA
=====================

Copyright (c) 2013-2015 Bitcredit Developers


Setup
---------------------
Bitcredit Core is the original Bitcredit client and it builds the backbone of the network. However, it downloads and stores the entire history of Bitcredit transactions (which is currently several GBs); depending on the speed of your computer and network connection, the synchronization process can take anywhere from a few hours to a day or more. Thankfully you only have to do this once. If you would like the process to go faster you can [download the blockchain directly](bootstrap.md).

Running
---------------------
The following are some helpful notes on how to run Bitcredit on your native platform. 

### Unix

You need the Qt4 run-time libraries to run Bitcredit-Qt. On Debian or Ubuntu:

	sudo apt-get install libqtgui4

Unpack the files into a directory and run:

- bin/32/bitcredit-qt (GUI, 32-bit) or bin/32/bitcreditd (headless, 32-bit)
- bin/64/bitcredit-qt (GUI, 64-bit) or bin/64/bitcreditd (headless, 64-bit)



### Windows

Unpack the files into a directory, and then run bitcredit-qt.exe.

### OSX

Drag Bitcredit-Qt to your applications folder, and then run Bitcredit-Qt.

### Need Help?

* Ask for help on the [BitcreditTalk](https://bitcredit-currency.org/) forums.

Building
---------------------
The following are developer notes on how to build Bitcredit on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [OSX Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-msw.md)

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [Files](files.md)
- [Tor Support](tor.md)

License
---------------------
Distributed under the [MIT/X11 software license](http://www.opensource.org/licenses/mit-license.php).
This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](http://www.openssl.org/). This product includes
cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.
