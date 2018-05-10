# Crown Platform 0.12.4

[Crown software](http://crown.tech/wallet) is an implementation of a full Crown Platform node, which can be used both locally, as a wallet for CRW and for running Masternodes and Systemnodes. It is a full node:  it downloads and stores the entire history of Crown transactions (which is currently several GBs); depending on the speed of your computer and network connection, the synchronization process can take anywhere from a few hours to a day or more. If you would like the process to go faster you can _download the blockchain directly_ **(BOOSTRAP LINK NEEDED)**.

## Install and Run Crown Wallet

Here are some helpful notes on how to run Crown on your native platform.

### Windows

Unpack the files into a directory, and then run `crown-qt.exe`.

### OSX

Drag Crown-Qt to your applications folder, and then run `Crown-Qt`.

### Unix

Unpack the files into a directory and run: `bin/crown-qt` or `bin/crownd`. You need the Qt5 run-time libraries to run Crown-Qt. On Debian or Ubuntu:

	sudo apt-get install libqtgui5

### Need Help?

* Ask for help on Mattermost: https://mm.crownlab.eu
* Or at Crown Forum: https://forum.crown.tech
* Or write a mail to [support@crown.tech](email:support@crown.tech)

## Run Masternode or Systemnode

TBD

## Build Crown From Source

The following are developer notes on how to build Crown on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [OSX Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows build notes](build-msw.md)

## Development Documentation

The Crown repo's [root README](https://gitlab.crown.tech/crown/crown-core/blob/master/README.md) contains relevant information on the development process.

- [Coding Guidelines](coding-style.md)
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

## License

Crown is distributed under the terms of the [MIT/X11 software license](http://www.opensource.org/licenses/mit-license.php). 

This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](https://www.openssl.org/). This product includes cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.
- - -

Copyright © 2009-2018, Bitcoin Core Developers

Copyright © 2014-2016, Dash Core Developers

Copyright © 2014-2018, Crown Developers