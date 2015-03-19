#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test resurrection of transactions when
# the blockchain is re-organized.
#

from test_framework import BitcoinTestFramework
from bitcoinrpc.authproxy import AuthServiceProxy, JSONRPCException
from util import *
import os
import shutil

class MempoolCoinbaseTest(BitcoinTestFramework):

    def setup_network(self):
        # Need three nodes for this test
        args = ["-checkmempool", "-debug=mempool", "-debug=net"]
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, args))
        self.nodes.append(start_node(1, self.options.tmpdir, args))
        self.nodes.append(start_node(2, self.options.tmpdir, args))
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[0], 2)
        connect_nodes(self.nodes[2], 1)
        self.is_network_split = False

    def create_tx(self, from_txid, to_address, amount, which_node=0):
        inputs = [{ "txid" : from_txid, "vout" : 0}]
        outputs = { to_address : amount }
        rawtx = self.nodes[which_node].createrawtransaction(inputs, outputs)
        signresult = self.nodes[which_node].signrawtransaction(rawtx)
        assert_equal(signresult["complete"], True)
        return signresult["hex"]

    def run_test(self):
        node0_address = self.nodes[0].getnewaddress()
        node1_address = self.nodes[1].getnewaddress()

        #---------------------------------------------------------------
        # Spend block 1/2/3's coinbase transactions
        # Mine a block.
        # Create three more transactions, spending the spends
        # Mine another block.
        # ... make sure all the transactions are confirmed
        # Invalidate both blocks
        # ... make sure all the transactions are put back in the mempool
        # Mine a new block
        # ... make sure all the transactions are confirmed again.

        b = [ self.nodes[0].getblockhash(n) for n in range(1, 4) ]
        coinbase_txids = [ self.nodes[0].getblock(h)['tx'][0] for h in b ]
        spends1_raw = [ self.create_tx(txid, node0_address, 50) for txid in coinbase_txids ]
        spends1_id = [ self.nodes[0].sendrawtransaction(tx) for tx in spends1_raw ]

        blocks = []
        blocks.extend(self.nodes[0].generate(1))

        spends2_raw = [ self.create_tx(txid, node0_address, 49.99) for txid in spends1_id ]
        spends2_id = [ self.nodes[0].sendrawtransaction(tx) for tx in spends2_raw ]

        blocks.extend(self.nodes[0].generate(1))

        # mempool should be empty, all txns confirmed
        assert_equal(set(self.nodes[0].getrawmempool()), set())
        for txid in spends1_id+spends2_id:
            tx = self.nodes[0].gettransaction(txid)
            assert(tx["confirmations"] > 0)

        # Use invalidateblock to re-org back; all transactions should
        # end up unconfirmed and back in the mempool
        sync_blocks(self.nodes)
        for node in self.nodes:
            node.invalidateblock(blocks[0])

        # mempool should be empty, all txns confirmed
        assert_equal(set(self.nodes[0].getrawmempool()), set(spends1_id+spends2_id))
        for txid in spends1_id+spends2_id:
            tx = self.nodes[0].gettransaction(txid)
            assert(tx["confirmations"] == 0)

        # Generate another block, they should all get mined
        self.nodes[0].generate(1)
        # mempool should be empty, all txns confirmed
        assert_equal(set(self.nodes[0].getrawmempool()), set())
        for txid in spends1_id+spends2_id:
            tx = self.nodes[0].gettransaction(txid)
            assert(tx["confirmations"] > 0)

        #---------------------------------------------------------------
        # Test the code that makes sure both the sender and receiver
        # of transactions that get dropped from the mempool because
        # of a long re-org put them back in the mempool.
        #
        # Test scenario is:
        # node[0] sends node[1] a transaction in block N
        # node[1] sends it back in block N+1
        # ... 8 more block are mined, then a 10-deep re-org
        # is triggered (with node[3] doing all the mining).
        # Then node 1 resends its wallet transactions.
        # EXPECT: node[3] ends up with the both transactions in
        # its mempool.
        blocks = []
        blocks.extend(self.nodes[2].generate(1))

        tx1_raw = self.create_tx(spends2_id[0], node1_address, 49.98, 0)
        tx1_id = self.nodes[2].sendrawtransaction(tx1_raw)
        blocks.extend(self.nodes[2].generate(1))
        sync_blocks(self.nodes)

        tx2_raw = self.create_tx(tx1_id, node0_address, 49.97, 1)
        tx2_id = self.nodes[2].sendrawtransaction(tx2_raw)

        blocks.extend(self.nodes[2].generate(8))
        sync_blocks(self.nodes)

        # Both transactions should be confirmed:
        for txid in (tx1_id, tx2_id):
            tx = self.nodes[1].gettransaction(txid)
            assert(tx["confirmations"] > 0)

        # Re-org away from that chain:
        for node in self.nodes:
            node.invalidateblock(blocks[0])
        new_chain = self.nodes[2].generate(10)
        sync_blocks(self.nodes)

        txset = set([tx1_id, tx2_id])
        assert_equal(set(self.nodes[0].resendwallettransactions()), txset)
        sync_mempools(self.nodes)

        assert_equal(set(self.nodes[2].getrawmempool()), txset)

if __name__ == '__main__':
    MempoolCoinbaseTest().main()
