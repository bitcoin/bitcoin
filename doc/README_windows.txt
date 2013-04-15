NovaCoin 0.4.0 BETA

Copyright (c) 2013 NovaCoin Developers
Copyright (c) 2011-2013 PPCoin Developers
Distributed under the MIT/X11 software license, see the accompanying
file license.txt or http://www.opensource.org/licenses/mit-license.php.
This product includes software developed by the OpenSSL Project for use in
the OpenSSL Toolkit (http://www.openssl.org/).  This product includes
cryptographic software written by Eric Young (eay@cryptsoft.com).


Intro
-----
NovaCoin is a free open source project derived from PPCoin, with
the goal of providing a long-term energy-efficient scrypt-based crypto-currency.
Built on the foundation of Bitcoin, innovations such as proof-of-stake and scrypt
help further advance the field of crypto-currency.


Setup
-----
After completing windows setup then run windows command line (cmd)
  cd daemon
  novacoind
You would need to create a configuration file novacoin.conf in the default
wallet directory. Grant access to novacoind.exe in anti-virus and firewall
applications if necessary.

The software automatically finds other nodes to connect to.  You can
enable Universal Plug and Play (UPnP) with your router/firewall
or forward port 9901 (TCP) to your computer so you can receive
incoming connections.  PPCoin works without incoming connections,
but allowing incoming connections helps the PPCoin network.


Upgrade
-------
All you existing coins/transactions should be intact with the upgrade.
To upgrade from 0.2.x, first backup wallet
novacoind backupwallet <destination_backup_file>
Then shutdown ppcoind by
novacoind stop
Start up the new novacoind (0.3.0).


------------------
Bitcoin 0.6.3 BETA

Copyright (c) 2009-2012 Bitcoin Developers
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


See the bitcoin wiki at:
  https://en.bitcoin.it/wiki/Main_Page
for more help and information.
