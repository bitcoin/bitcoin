Bitcoin Core 
=====================================

https://bitcoincore.org

What is Bitcoin?
----------------

Bitcoin is an experimental digital currency that enables instant payments to
anyone, anywhere in the world. Bitcoin uses peer-to-peer technology to operate
with no central authority: managing transactions and issuing money are carried
out collectively by the network. Bitcoin Core is the name of open source
software which enables the use of this currency.

For more information, as well as an immediately useable, binary version of
the Bitcoin Core software, see https://bitcoincore.org/en/download/, or read the
[original whitepaper](https://bitcoincore.org/bitcoin.pdf).

License
-------

Bitcoin Core is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

Setup
---------------------
Bitcoin Core is the original Bitcoin client and it builds the backbone of the network. It downloads and, by default, stores the entire history of Bitcoin transactions (which is currently more than 100 GBs); depending on the speed of your computer and network connection, the synchronization process can take anywhere from a few hours to a day or more.

To download Bitcoin Core, visit [bitcoincore.org](https://bitcoincore.org/en/releases/).

Running
---------------------

Unpack the files into a directory and run:

- `bin/bitcoin-qt` (GUI) or
- `bin/bitcoind` (headless)

Note: bitcoin-qt is missing in arm tars.

### Files
```
./bin/bitcoin-cli (Command Line Interface)
./bin/bitcoind (headless daemon)
./bin/bitcoin-qt (GUI)
./bin/bitcoin-tx (hex-encoded transaction tool)
./bin/test_bitcoin (runs the unit tests)
./include/bitcoinconsensus.h (lib header)
./lib/libbitcoinconsensus.so.0.0.0 (shared lib)
./share/man/man1/bitcoin-cli.1 (man pages)
./share/man/man1/bitcoind.1
./share/man/man1/bitcoin-qt.1
./share/man/man1/bitcoin-tx.1
```
### Need Help?

See the documentation at the [Bitcoin Wiki](https://en.bitcoin.it/wiki/Main_Page)


### Contributing

You are welcome to [contribute](https://bitcoincore.org/en/contribute/) to the project! 
