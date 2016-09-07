#!/usr/bin/env python2
# Copyright (c) 2016 Tom Zander <tomz@freedommail.ch>
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test creation of a v4 transaction, mining it and forwarding it to other nodes.
#

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class Transactionv4(BitcoinTestFramework):

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 4)

    def setup_network(self):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug"]))
        connect_nodes(self.nodes[0], 1)

        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        print "Mining blocks..."
        self.nodes[0].wallet.generate(105)
        self.sync_all()

        # chain_height = self.nodes[1].getblockcount()

        node0utxos = self.nodes[0].listunspent(1)
        print "Creating v4 tx..."
        tx1 = self.nodes[0].createrawtransaction([node0utxos.pop()], {self.nodes[1].getnewaddress(): 50}, 0, 4)
        txid1 = self.nodes[0].sendrawtransaction(self.nodes[0].signrawtransaction(tx1)["hex"])
        self.sync_all()
        print "Checking for propagation"
        self.nodes[1].wallet.generate(1)
        self.sync_all()
        block = self.nodes[0].getblock(self.nodes[0].getbestblockhash())
        assert_equal(block["height"], 106)
        tx_list = block["tx"]
        assert_equal(len(tx_list), 2)
        assert_equal(tx_list[1], txid1)

if __name__ == '__main__':
    Transactionv4().main()
