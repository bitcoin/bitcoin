#!/usr/bin/env python2
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test merkleblock fetch/validation
#

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class MerkleBlockTest(BitcoinTestFramework):

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 4)

    def setup_network(self):
        self.nodes = []
        # Nodes 0/1 are "wallet" nodes
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug"]))
        # Nodes 2/3 are used for testing
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug", "-txindex"]))
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

        node0utxos = self.nodes[0].listunspent(1)
        tx1 = self.nodes[0].createrawtransaction([node0utxos.pop()], {self.nodes[1].getnewaddress(): 500})
        txid1 = self.nodes[0].sendrawtransaction(self.nodes[0].signrawtransaction(tx1)["hex"])
        tx2 = self.nodes[0].createrawtransaction([node0utxos.pop()], {self.nodes[1].getnewaddress(): 500})
        txid2 = self.nodes[0].sendrawtransaction(self.nodes[0].signrawtransaction(tx2)["hex"])
        assert_raises(JSONRPCException, self.nodes[0].gettxoutproof, [txid1])

        self.nodes[0].generate(1)
        blockhash = self.nodes[0].getblockhash(chain_height + 1)
        self.sync_all()

        txlist = []
        blocktxn = self.nodes[0].getblock(blockhash, True)["tx"]
        txlist.append(blocktxn[1])
        txlist.append(blocktxn[2])

        assert_equal(self.nodes[2].verifytxoutproof(self.nodes[2].gettxoutproof([txid1])), [txid1])
        assert_equal(self.nodes[2].verifytxoutproof(self.nodes[2].gettxoutproof([txid1, txid2])), txlist)
        assert_equal(self.nodes[2].verifytxoutproof(self.nodes[2].gettxoutproof([txid1, txid2], blockhash)), txlist)

        txin_spent = self.nodes[1].listunspent(1).pop()
        tx3 = self.nodes[1].createrawtransaction([txin_spent], {self.nodes[0].getnewaddress(): 500})
        self.nodes[0].sendrawtransaction(self.nodes[1].signrawtransaction(tx3)["hex"])
        self.nodes[0].generate(1)
        self.sync_all()

        txid_spent = txin_spent["txid"]
        txid_unspent = txid1 if txin_spent["txid"] != txid1 else txid2

        # We can't find the block from a fully-spent tx
        # Doesn't apply to Dash Core - we have txindex always on
        # assert_raises(JSONRPCException, self.nodes[2].gettxoutproof, [txid_spent])
        # ...but we can if we specify the block
        assert_equal(self.nodes[2].verifytxoutproof(self.nodes[2].gettxoutproof([txid_spent], blockhash)), [txid_spent])
        # ...or if the first tx is not fully-spent
        assert_equal(self.nodes[2].verifytxoutproof(self.nodes[2].gettxoutproof([txid_unspent])), [txid_unspent])
        try:
            assert_equal(self.nodes[2].verifytxoutproof(self.nodes[2].gettxoutproof([txid1, txid2])), txlist)
        except JSONRPCException:
            assert_equal(self.nodes[2].verifytxoutproof(self.nodes[2].gettxoutproof([txid2, txid1])), txlist)
        # ...or if we have a -txindex
        assert_equal(self.nodes[2].verifytxoutproof(self.nodes[3].gettxoutproof([txid_spent])), [txid_spent])

if __name__ == '__main__':
    MerkleBlockTest().main()
