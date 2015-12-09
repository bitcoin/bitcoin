Dash Core 0.12.0
=====================

This is the official reference wallet for Dash digital currency and comprises the backbone of the Dash peer-to-peer network. You can [download Dash Core](https://www.dash.org/downloads/) or [build it yourself](#building) using the guides below.

Running
---------------------
The following are some helpful notes on how to run Dash on your native platform.

### Unix

You need the Qt4 run-time libraries to run Dash-Qt. On Debian or Ubuntu:

	sudo apt-get install libqtgui4

Unpack the files into a directory and run:

- bin/32/dash-qt (GUI, 32-bit) or bin/32/dashd (headless, 32-bit)
- bin/64/dash-qt (GUI, 64-bit) or bin/64/dashd (headless, 64-bit)



### Windows

Unpack the files into a directory, and then run dash-qt.exe.

### OSX

Drag Dash-Qt to your applications folder, and then run Dash-Qt.

### Need Help?

* See the [Dash documentation](https://dashpay.atlassian.net/wiki/display/DOC)
for help and more information.
* Ask for help on [#dashpay](http://webchat.freenode.net?channels=dashpay) on Freenode. If you don't have an IRC client use [webchat here](http://webchat.freenode.net?channels=dashpay).
* Ask for help on the [DashTalk](https://dashtalk.org/) forums.

Building
---------------------
The following are developer notes on how to build Dash on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [OSX Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)

Development
---------------------
The Dash repo's [root README](https://github.com/dashpay/dash/blob/master/README.md) contains relevant information on the development process and automated testing.

- [Coding Guidelines](coding.md)
- [Multiwallet Qt Development](multiwallet-qt.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- Source Code Documentation ***TODO***
- [Translation Process](translation_process.md)
- [Unit Tests](unit-tests.md)

### Resources
* Discuss on the [DashTalk](https://dashtalk.org/) forums, in the Development & Technical Discussion board.
* Discuss on [#dashpay](http://webchat.freenode.net/?channels=dashpay) on Freenode. If you don't have an IRC client use [webchat here](http://webchat.freenode.net/?channels=dashpay).

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
