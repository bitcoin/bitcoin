vBitcoin Core
=============

Setup
---------------------
vBitcoin Core is the original vBitcoin client and it builds the backbone of the network. It downloads and, by default, stores the entire history of vBitcoin transactions, which requires a few gigabytes of disk space. Depending on the speed of your computer and network connection, the synchronization process can take anywhere from a few hours to a day or more.

To download vBitcoin Core, visit [veriblock.org](https://veriblock.org/).

Running
---------------------
The following are some helpful notes on how to run vBitcoin Core on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/vbitcoin-qt` (GUI) or
- `bin/vbitcoind` (headless)

### Windows

Unpack the files into a directory, and then run vbitcoin-qt.exe.

### macOS

Drag vBitcoin Core to your applications folder, and then run vBitcoin Core.

### Need Help?

* Join our discord channel @ VeriBlock (https://discord.gg/wJZEjry).

You can also seek general help related to Bitcoin itself below:
* See the documentation at the [Bitcoin Wiki](https://en.bitcoin.it/wiki/Main_Page)
for help and more information.
* Ask for help on [#bitcoin](http://webchat.freenode.net?channels=bitcoin) on Freenode. If you don't have an IRC client, use [webchat here](http://webchat.freenode.net?channels=bitcoin).
* Ask for help on the [BitcoinTalk](https://bitcointalk.org/) forums, in the [Technical Support board](https://bitcointalk.org/index.php?board=4.0).

Building
---------------------
The following are developer notes on how to build vBitcoin Core on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [Dependencies](dependencies.md)
- [macOS Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-windows.md)
- [FreeBSD Build Notes](build-freebsd.md)
- [OpenBSD Build Notes](build-openbsd.md)
- [NetBSD Build Notes](build-netbsd.md)
- [Gitian Building Guide (External Link)](https://github.com/bitcoin-core/docs/blob/master/gitian-building.md)

Development
---------------------
The vBitcoin repo's [root README](/README.md) contains relevant information on the development process and automated testing.

- [Developer Notes](developer-notes.md)
- [Productivity Notes](productivity.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- [Source Code Documentation (External Link)](https://doxygen.bitcoincore.org/)
- [Translation Process](translation_process.md)
- [Translation Strings Policy](translation_strings_policy.md)
- [JSON-RPC Interface](JSON-RPC-interface.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Shared Libraries](shared-libraries.md)
- [BIPS](bips.md)
- [Dnsseed Policy](dnsseed-policy.md)
- [Benchmarking](benchmarking.md)

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [vbitcoin.conf Configuration File](vbitcoin-conf.md)
- [Files](files.md)
- [Fuzz-testing](fuzzing.md)
- [Reduce Memory](reduce-memory.md)
- [Reduce Traffic](reduce-traffic.md)
- [Tor Support](tor.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)
- [ZMQ](zmq.md)
- [PSBT support](psbt.md)

License
---------------------
Distributed under the [MIT software license](/COPYING).
