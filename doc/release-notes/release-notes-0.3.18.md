Changes:
* Fixed a wallet.dat compatibility problem if you downgraded from 0.3.17 and then upgraded again
* IsStandard() check to only include known transaction types in blocks
* Jgarzik's optimisation to speed up the initial block download a little

The main addition in this release is the Accounts-Based JSON-RPC commands that Gavin's been working on (more details at http://www.bitcoin.org/smf/index.php?topic=1886.0).  
* getaccountaddress
* sendfrom
* move
* getbalance
* listtransactions
