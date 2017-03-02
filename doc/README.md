Peercoin (PPCoin) 0.6.0 BETA

Copyright (c) 2011-2017 Peercoin (PPCoin) Developers
Distributed under the MIT/X11 software license, see the accompanying
file license.txt or http://www.opensource.org/licenses/mit-license.php.
This product includes software developed by the OpenSSL Project for use in
the OpenSSL Toolkit (http://www.openssl.org/).  This product includes
cryptographic software written by Eric Young (eay@cryptsoft.com).


Intro
-----
PPCoin is a free open source project derived from Bitcoin, with
the goal of providing a long-term energy-efficient crypto-currency.
Built on the foundation of Bitcoin, innovations such as proof-of-stake
help further advance the field of crypto-currency.


Setup
-----
Unpack the files into a directory and run:
 bin/32/ppcoin-qt (GUI, 32-bit)
 bin/32/ppcoind (headless, 32-bit)
 bin/64/ppcoin-qt (GUI, 64-bit)
 bin/64/ppcoind (headless, 64-bit)

The software automatically finds other nodes to connect to.  You can
enable Universal Plug and Play (UPnP) with your router/firewall
or forward port 9901 (TCP) to your computer so you can receive
incoming connections.  PPCoin works without incoming connections,
but allowing incoming connections helps the PPCoin network.


Upgrade
-------
All your existing coins/transactions should be intact with the upgrade.
To upgrade from 0.5, first backup wallet
ppcoind backupwallet <destination_backup_file>
Then shutdown ppcoind by
ppcoind stop
Uninstall v0.5 client, download and install v0.6 client.
Remove all files and subdirectory in your wallet directory EXCEPT FOR
wallet.dat (wallet file) and ppcoin.conf (configuration file).
Start up the new ppcoind (0.6).
For this upgrade blockchain re-download is required.


See the documentation/wiki at github:
  http://github.com/peercoin/peercoin
for help and more information.


Other Pages
---------------------
- [Unix Build Notes](build-unix.md)
- [OSX Build Notes](build-osx.md)
- [Windows Build Notes](build-msw.md)
- [Coding Guidelines](coding.md)
- [Release Process](release-process.md)
- [Release Notes](release-notes.md)
- [Multiwallet Qt Development](multiwallet-qt.md)
- [Unit Tests](unit-tests.md)
- [Translation Process](translation_process.md)




Bitcoin 0.8.6 BETA
====================

Copyright (c) 2009-2013 Bitcoin Developers

Distributed under the MIT/X11 software license, see the accompanying
file COPYING or http://www.opensource.org/licenses/mit-license.php.
This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](http://www.openssl.org/). This product includes
cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.


Intro
---------------------
Bitcoin is a free open source peer-to-peer electronic cash system that is
completely decentralized, without the need for a central server or trusted
parties.  Users hold the crypto keys to their own money and transact directly
with each other, with the help of a P2P network to check for double-spending.


Setup
---------------------
You need the Qt4 run-time libraries to run Bitcoin-Qt. On Debian or Ubuntu:
	`sudo apt-get install libqtgui4`

Unpack the files into a directory and run:

- bin/32/bitcoin-qt (GUI, 32-bit)
- bin/32/bitcoind (headless, 32-bit)
- bin/64/bitcoin-qt (GUI, 64-bit)
- bin/64/bitcoind (headless, 64-bit)

See the documentation at the [Bitcoin Wiki](https://en.bitcoin.it/wiki/Main_Page)
for help and more information.
