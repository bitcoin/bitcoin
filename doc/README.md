Chaincoin Core
=============

Setup
---------------------
Chaincoin Core is the original Chaincoin client and it builds the backbone of the network. It downloads and, by default, stores the entire history of Chaincoin transactions (which is currently more than 1 GB); depending on the speed of your computer and network connection, the synchronization process can take anywhere from a few hours to a day or more.

To download Chaincoin Core, visit [chaincoin.org](https://www.chaincoin.org/chaincoin-wallet/).

Running
---------------------
The following are some helpful notes on how to run Chaincoin on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/chaincoin-qt` (GUI) or
- `bin/chaincoind` (headless)

### Windows

Unpack the files into a directory, and then run chaincoin-qt.exe.

### macOS

Drag Chaincoin-Core to your applications folder, and then run Chaincoin-Core.

Building
---------------------
The following are developer notes on how to build Chaincoin on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [Dependencies](dependencies.md)
- [macOS Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-windows.md)
- [OpenBSD Build Notes](build-openbsd.md)
- [NetBSD Build Notes](build-netbsd.md)
- [Gitian Building Guide](gitian-building.md)

Development
---------------------
The Chaincoin repo's [root README](/README.md) contains relevant information on the development process and automated testing.

- [Developer Notes](developer-notes.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- [Source Code Documentation (External Link)](https://dev.visucore.com/bitcoin/doxygen/)
- [Translation Process](translation_process.md)
- [Translation Strings Policy](translation_strings_policy.md)
- [Travis CI](travis-ci.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Shared Libraries](shared-libraries.md)
- [BIPS](bips.md)
- [Dnsseed Policy](dnsseed-policy.md)
- [Benchmarking](benchmarking.md)

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [Files](files.md)
- [Fuzz-testing](fuzzing.md)
- [Reduce Traffic](reduce-traffic.md)
- [Tor Support](tor.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)
- [ZMQ](zmq.md)

License
---------------------
Distributed under the [MIT software license](/COPYING).
This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](https://www.openssl.org/). This product includes
cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.
