Bitcoin Core
=============

Setup
---------------------
Bitcoin Core is the original Bitcoin client and it builds the backbone of the network. It downloads and, by default, stores the entire history of Bitcoin transactions (which is currently more than 150 GBs); depending on the speed of your computer and network connection, the synchronization process can take anywhere from a few hours to a day or more.

Bitcoin Core is fully open source and welcomes Pull Requests and Contributions at the official [GitHub Repository](https://github.com/bitcoin/bitcoin/). Bug reports and Feature Suggestions can be filed at https://github.com/bitcoin/bitcoin/issues.

Setup
---------------------
Unpack the files into a directory and run:

- `bin/bitcoin-qt` for the GUI or
- `bin/bitcoind` for the daemon

Files
---------------------
The functionality of some files bundled with the release is provided below:

- `bin/bitcoin-cli` allows you to send RPC command to bitcoind from the command line. For usage, ./bitcoin-cli help
- `bin/bitcoind` is the Bitcoin daemon and provides a full node, queryable by RPC calls
- `bin/bitcoin-tx` can be used to create a raw bitcoin transaction, which can then be relayed via `bitcoind`
- `bin/bitcoin-qt`provides a combination full Bitcoin peer and wallet frontend
- `bin/test_bitcoin` can be used to validate proper functioning of the Bitcoin client

Development
---------------------
The Bitcoin repo's [root README](/README.md) contains relevant information on the development process and automated testing.

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


### Need Help?

* See the documentation at the [Bitcoin Wiki](https://en.bitcoin.it/wiki/Main_Page)
for help and more information.
* Ask for help on [#bitcoin](http://webchat.freenode.net?channels=bitcoin) on Freenode. If you don't have an IRC client use [webchat here](http://webchat.freenode.net?channels=bitcoin).
* Ask for help on the [BitcoinTalk](https://bitcointalk.org/) forums, in the [Technical Support board](https://bitcointalk.org/index.php?board=4.0).

License
---------------------
Distributed under the [MIT software license](/COPYING).
This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](https://www.openssl.org/). This product includes
cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.
