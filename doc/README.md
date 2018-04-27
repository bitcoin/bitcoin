Ravencoin Core
==============

Setup
---------------------
Ravencoin Core is the original Ravencoin client and it builds the backbone of the network. It downloads and, by default, stores the entire history of Ravencoin transactions; depending on the speed of your computer and network connection, the synchronization process is typically complete in under an hour.

To download Ravencion Core code, visit [GitHub](https://github.com/RavenProject/Ravencoin/releases).

Running
---------------------
The following are some helpful notes on how to run Ravencoin on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/raven-qt` (GUI) or
- `bin/ravend` (headless)

### Windows

Unpack the files into a directory, and then run raven-qt.exe.

### OS X

Drag Raven-Core to your applications folder, and then run Raven-Core.

### Need Help?

* See the documentation at the [Ravencoin Wiki](https://raven.wiki/wiki/Ravencoin_Wiki)
for help and more information.
* Ask for help on [Discord](https://discord.gg/DUkcBst) or [Telegram](https://t.me/RavencoinDev).
* Ask for help on the [RavencoinTalk](https://www.ravencointalk.org/) forums, in the [Development and Technical discussion board](https://www.ravencointalk.org/?forum=661517).

Building
---------------------
The following are developer notes on how to build the Ravencoin core software on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [Dependencies](dependencies.md)
- [OS X Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-windows.md)
- [OpenBSD Build Notes](build-openbsd.md)
- [Gitian Building Guide](gitian-building.md)

Development
---------------------
Ravencoin repo's [root README](/README.md) contains relevant information on the development process and automated testing.

- [Developer Notes](developer-notes.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- [Source Code Documentation (External Link)](https://dev.visucore.com/raven/doxygen/)
- [Translation Process](translation_process.md)
- [Translation Strings Policy](translation_strings_policy.md)
- [Travis CI](travis-ci.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Shared Libraries](shared-libraries.md)
- [BIPS](bips.md)
- [Dnsseed Policy](dnsseed-policy.md)
- [Benchmarking](benchmarking.md)

### Resources
* Discuss on the [RavencoinTalk](https://www.ravencointalk.org/) forums, in the [Development & Technical Discussion board](https://raventalk.org/index.php?board=6.0).
* Discuss on chat [Discord](https://discord.gg/DUkcBst) or [Telegram](https://t.me/RavencoinDev)
* Find out more on the [Ravencoin Wiki](https://raven.wiki/wiki/Ravencoin_Wiki)
* Visit the project home [Ravencoin.org](https://ravencoin.org)

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
