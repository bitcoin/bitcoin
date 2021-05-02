XBit Core
=============

Setup
---------------------
XBit Core is the original XBit client and it builds the backbone of the network. It downloads and, by default, stores the entire history of XBit transactions, which requires a few hundred gigabytes of disk space. Depending on the speed of your computer and network connection, the synchronization process can take anywhere from a few hours to a day or more.

To download XBit Core, visit [xbitcore.org](https://xbitcore.org/en/download/).

Running
---------------------
The following are some helpful notes on how to run XBit Core on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/xbit-qt` (GUI) or
- `bin/xbitd` (headless)

### Windows

Unpack the files into a directory, and then run xbit-qt.exe.

### macOS

Drag XBit Core to your applications folder, and then run XBit Core.

### Need Help?

* See the documentation at the [XBit Wiki](https://en.xbit.it/wiki/Main_Page)
for help and more information.
* Ask for help on [#xbit](https://webchat.freenode.net/#xbit) on Freenode. If you don't have an IRC client, use [webchat here](https://webchat.freenode.net/#xbit).
* Ask for help on the [XBitTalk](https://xbittalk.org/) forums, in the [Technical Support board](https://xbittalk.org/index.php?board=4.0).

Building
---------------------
The following are developer notes on how to build XBit Core on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [Dependencies](dependencies.md)
- [macOS Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-windows.md)
- [FreeBSD Build Notes](build-freebsd.md)
- [OpenBSD Build Notes](build-openbsd.md)
- [NetBSD Build Notes](build-netbsd.md)
- [Gitian Building Guide (External Link)](https://github.com/xbit-core/docs/blob/master/gitian-building.md)

Development
---------------------
The XBit repo's [root README](/README.md) contains relevant information on the development process and automated testing.

- [Developer Notes](developer-notes.md)
- [Productivity Notes](productivity.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- [Source Code Documentation (External Link)](https://doxygen.xbitcore.org/)
- [Translation Process](translation_process.md)
- [Translation Strings Policy](translation_strings_policy.md)
- [JSON-RPC Interface](JSON-RPC-interface.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Shared Libraries](shared-libraries.md)
- [BIPS](bips.md)
- [Dnsseed Policy](dnsseed-policy.md)
- [Benchmarking](benchmarking.md)

### Resources
* Discuss on the [XBitTalk](https://xbittalk.org/) forums, in the [Development & Technical Discussion board](https://xbittalk.org/index.php?board=6.0).
* Discuss project-specific development on #xbit-core-dev on Freenode. If you don't have an IRC client, use [webchat here](https://webchat.freenode.net/#xbit-core-dev).
* Discuss general XBit development on #xbit-dev on Freenode. If you don't have an IRC client, use [webchat here](https://webchat.freenode.net/#xbit-dev).

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [xbit.conf Configuration File](xbit-conf.md)
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
