#!/usr/bin/env python2
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test addressindex generation and fetching
#

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
import binascii

class AddressIndexTest(BitcoinTestFramework):

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 4)

    def setup_network(self):
        self.nodes = []
        # Nodes 0/1 are "wallet" nodes
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug", "-addressindex"]))
        # Nodes 2/3 are used for testing
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug", "-addressindex"]))
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[0], 2)
        connect_nodes(self.nodes[0], 3)

        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        print "Mining blocks..."
        self.nodes[0].generate(105)
        self.sync_all()

        chain_height = self.nodes[1].getblockcount()
        assert_equal(chain_height, 105)
        assert_equal(self.nodes[1].getbalance(), 0)
        assert_equal(self.nodes[2].getbalance(), 0)

        # Check p2pkh and p2sh address indexes
        print "Testing p2pkh and p2sh address index..."

        txid0 = self.nodes[0].sendtoaddress("mo9ncXisMeAoXwqcV5EWuyncbmCcQN4rVs", 10)
        txidb0 = self.nodes[0].sendtoaddress("2N2JD6wb56AfK4tfmM6PwdVmoYk2dCKf4Br", 10)
        self.nodes[0].generate(1)

        txid1 = self.nodes[0].sendtoaddress("mo9ncXisMeAoXwqcV5EWuyncbmCcQN4rVs", 15)
        txidb1 = self.nodes[0].sendtoaddress("2N2JD6wb56AfK4tfmM6PwdVmoYk2dCKf4Br", 15)
        self.nodes[0].generate(1)

        txid2 = self.nodes[0].sendtoaddress("mo9ncXisMeAoXwqcV5EWuyncbmCcQN4rVs", 20)
        txidb2 = self.nodes[0].sendtoaddress("2N2JD6wb56AfK4tfmM6PwdVmoYk2dCKf4Br", 20)
        self.nodes[0].generate(1)

        self.sync_all()

        txids = self.nodes[1].getaddresstxids("mo9ncXisMeAoXwqcV5EWuyncbmCcQN4rVs");
        assert_equal(len(txids), 3);
        assert_equal(txids[0], txid0);
        assert_equal(txids[1], txid1);
        assert_equal(txids[2], txid2);

        txidsb = self.nodes[1].getaddresstxids("2N2JD6wb56AfK4tfmM6PwdVmoYk2dCKf4Br");
        assert_equal(len(txidsb), 3);
        assert_equal(txidsb[0], txidb0);
        assert_equal(txidsb[1], txidb1);
        assert_equal(txidsb[2], txidb2);

        # Check that outputs with the same address will only return one txid
        print "Testing for txid uniqueness..."
        addressHash = "6349a418fc4578d10a372b54b45c280cc8c4382f".decode("hex")
        scriptPubKey = CScript([OP_HASH160, addressHash, OP_EQUAL])
        unspent = self.nodes[0].listunspent()
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(unspent[0]["txid"], 16), unspent[0]["vout"]))]
        tx.vout = [CTxOut(10, scriptPubKey), CTxOut(11, scriptPubKey)]
        tx.rehash()

        signed_tx = self.nodes[0].signrawtransaction(binascii.hexlify(tx.serialize()).decode('utf-8'))
        sent_txid = self.nodes[0].sendrawtransaction(signed_tx['hex'], True)

        self.nodes[0].generate(1)
        self.sync_all()

        txidsmany = self.nodes[1].getaddresstxids("2N2JD6wb56AfK4tfmM6PwdVmoYk2dCKf4Br");
        assert_equal(len(txidsmany), 4);
        assert_equal(txidsmany[3], sent_txid);

        print "Passed\n"


if __name__ == '__main__':
    AddressIndexTest().main()
