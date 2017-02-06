Crown Core 0.12.0
=====================

Setup
---------------------
[Crown Core](http://crown.tech/wallet) is the original Crown client and it builds the backbone of the network. However, it downloads and stores the entire history of Crowncoin transactions (which is currently several GBs); depending on the speed of your computer and network connection, the synchronization process can take anywhere from a few hours to a day or more. Thankfully you only have to do this once. If you would like the process to go faster you can [download the blockchain directly](http://txexplorer.infernopool.com/CRWbootstrap.zip).

Running
---------------------
The following are some helpful notes on how to run Crown on your native platform.

### Unix

You need the Qt5 run-time libraries to run Crown-Qt. On Debian or Ubuntu:

	sudo apt-get install libqtgui5

Unpack the files into a directory and run:

- bin/32/crown-qt (GUI, 32-bit) or bin/32/crownd (headless, 32-bit)
- bin/64/crown-qt (GUI, 64-bit) or bin/64/crownd (headless, 64-bit)



### Windows

Unpack the files into a directory, and then run crown-qt.exe.

### OSX

Drag Crown-Qt to your applications folder, and then run Crown-Qt.

### Need Help?

* Ask for help on [#crowncoin](http://webchat.freenode.net?channels=crowncoin) on Freenode. If you don't have an IRC client use [webchat here](http://webchat.freenode.net?channels=crowncoin).

Building
---------------------
The following are developer notes on how to build Crown on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [OSX Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows build notes](build-msw.md)

Development
---------------------
The Crown repo's [root README](https://github.com/Crowndev/crowncoin/blob/master/README.md) contains relevant information on the development process and automated testing.

- [Coding Guidelines](coding.md)
- [Multiwallet Qt Development](multiwallet-qt.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- [Unit Tests](unit-tests.md)

### Resources
* Discuss on [#crowncoin](http://webchat.freenode.net/?channels=crowncoin) on Freenode. If you don't have an IRC client use [webchat here](http://webchat.freenode.net/?channels=crowncoin).

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [Files](files.md)
- [Tor Support](tor.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)

License
---------------------
Distributed under the [MIT/X11 software license](http://www.opensource.org/licenses/mit-license.php).
This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](https://www.openssl.org/). This product includes
cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.
