### P2P and Network Changes

Support for the legacy ElGamal (type 0) encryption type when creating I2P
sessions is being sunset by the I2P network and will be removed from bitcoind on
or before v34. Nodes using I2P with versions of Bitcoin Core earlier than v26.1
(PRs #29200, #29209) will soon only be able to connect to other legacy ElGamal
I2P peers and will be increasingly isolated from the rest of the network, with a
reduced anonymity set. See #35696 for details.
