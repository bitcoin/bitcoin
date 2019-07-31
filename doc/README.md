Bitcointalkcoin Core
=============

Setup
---------------------
Bitcointalkcoin Core is the original Bitcointalkcoin client and it builds the backbone of the network. It downloads and, by default, stores the entire history of Bitcointalkcoin transactions, which requires a few hundred gigabytes of disk space. Depending on the speed of your computer and network connection, the synchronization process can take anywhere from a few hours to a day or more.

To download Bitcointalkcoin Core, visit [bitcointalkcoincore.org](https://bitcointalkcoincore.org/en/download/).

Running
---------------------
The following are some helpful notes on how to run Bitcointalkcoin Core on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/bitcointalkcoin-qt` (GUI) or
- `bin/bitcointalkcoind` (headless)

### Windows

Unpack the files into a directory, and then run bitcointalkcoin-qt.exe.

### macOS

Drag Bitcointalkcoin Core to your applications folder, and then run Bitcointalkcoin Core.

### Need Help?

* See the documentation at the [Bitcointalkcoin Wiki](https://en.bitcointalkcoin.it/wiki/Main_Page)
for help and more information.
* Ask for help on [#bitcointalkcoin](http://webchat.freenode.net?channels=bitcointalkcoin) on Freenode. If you don't have an IRC client, use [webchat here](http://webchat.freenode.net?channels=bitcointalkcoin).
* Ask for help on the [BitcointalkcoinTalk](https://bitcointalkcointalk.org/) forums, in the [Technical Support board](https://bitcointalkcointalk.org/index.php?board=4.0).

Building
---------------------
The following are developer notes on how to build Bitcointalkcoin Core on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [Dependencies](dependencies.md)
- [macOS Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-windows.md)
- [FreeBSD Build Notes](build-freebsd.md)
- [OpenBSD Build Notes](build-openbsd.md)
- [NetBSD Build Notes](build-netbsd.md)
- [Gitian Building Guide (External Link)](https://github.com/bitcointalkcoin-core/docs/blob/master/gitian-building.md)

Development
---------------------
The Bitcointalkcoin repo's [root README](/README.md) contains relevant information on the development process and automated testing.

- [Developer Notes](developer-notes.md)
- [Productivity Notes](productivity.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- [Source Code Documentation (External Link)](https://dev.visucore.com/bitcointalkcoin/doxygen/)
- [Translation Process](translation_process.md)
- [Translation Strings Policy](translation_strings_policy.md)
- [JSON-RPC Interface](JSON-RPC-interface.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Shared Libraries](shared-libraries.md)
- [BIPS](bips.md)
- [Dnsseed Policy](dnsseed-policy.md)
- [Benchmarking](benchmarking.md)

### Resources
* Discuss on the [BitcointalkcoinTalk](https://bitcointalkcointalk.org/) forums, in the [Development & Technical Discussion board](https://bitcointalkcointalk.org/index.php?board=6.0).
* Discuss project-specific development on #bitcointalkcoin-core-dev on Freenode. If you don't have an IRC client, use [webchat here](http://webchat.freenode.net/?channels=bitcointalkcoin-core-dev).
* Discuss general Bitcointalkcoin development on #bitcointalkcoin-dev on Freenode. If you don't have an IRC client, use [webchat here](http://webchat.freenode.net/?channels=bitcointalkcoin-dev).

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [bitcointalkcoin.conf Configuration File](bitcointalkcoin-conf.md)
- [Files](files.md)
- [Fuzz-testing](fuzzing.md)
- [Reduce Traffic](reduce-traffic.md)
- [Tor Support](tor.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)
- [ZMQ](zmq.md)
- [PSBT support](psbt.md)

License
---------------------
Distributed under the [MIT software license](/COPYING).
This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](https://www.openssl.org/). This product includes
cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.
