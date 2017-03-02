Peercoin (PPCoin) 0.6.0 BETA

Copyright (c) 2011-2017 Peercoin (PPCoin) Developers
Distributed under the MIT/X11 software license, see the accompanying
file COPYING or http://www.opensource.org/licenses/mit-license.php.
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
After completing windows setup then run ppcoin-qt.
Alternatively you can run windows command line (cmd) in ppcoin program dir.
  cd daemon
  ppcoind
You would need to create a configuration file ppcoin.conf in the default
wallet directory. Grant access to ppcoind/ppcoin-qt in anti-virus and firewall
applications if necessary.

The software automatically finds other nodes to connect to.  You can
enable Universal Plug and Play (UPnP) with your router/firewall
or forward port 9901 (TCP) to your computer so you can receive
incoming connections.  PPCoin works without incoming connections,
but allowing incoming connections helps the PPCoin network.


Upgrade
-------
All your existing coins/transactions should be intact with the upgrade.
To upgrade from 0.5, first backup wallet in qt menu or by command line
ppcoind backupwallet <destination_backup_file>
Then shutdown ppcoind by
ppcoind stop
Uninstall v0.5 client, download and install v0.6 client.
Remove all files and subdirectory in your wallet directory EXCEPT FOR
wallet.dat (wallet file) and ppcoin.conf (configuration file).
For this upgrade blockchain re-download is required.


See the documentation/wiki at github:
  http://github.com/peercoin/peercoin
for help and more information.


------------------
Bitcoin 0.8.6 BETA

Copyright (c) 2009-2013 Bitcoin Developers
Distributed under the MIT/X11 software license, see the accompanying
file license.txt or http://www.opensource.org/licenses/mit-license.php.
This product includes software developed by the OpenSSL Project for use in
the OpenSSL Toolkit (http://www.openssl.org/).  This product includes
cryptographic software written by Eric Young (eay@cryptsoft.com).


Intro
-----
Bitcoin is a free open source peer-to-peer electronic cash system that is
completely decentralized, without the need for a central server or trusted
parties.  Users hold the crypto keys to their own money and transact directly
with each other, with the help of a P2P network to check for double-spending.

Unpack the files into a directory and run bitcoin-qt.exe.

Bitcoin-Qt is the original Bitcoin client and it builds the backbone of the network.
However, it downloads and stores the entire history of Bitcoin transactions;
depending on the speed of your computer and network connection, the synchronization
process can take anywhere from a few hours to a day or more.

See the bitcoin wiki at:
  https://en.bitcoin.it/wiki/Main_Page
for more help and information.
