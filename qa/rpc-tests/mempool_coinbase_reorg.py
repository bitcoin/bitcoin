#!/usr/bin/env python
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test re-org scenarios with a mempool that contains transactions
# that spend (directly or indirectly) coinbase transactions.
#

from test_framework import BitcoinTestFramework
from util import *

class MempoolCoinbaseReorgTest(BitcoinTestFramework):

    def setup_network(self):
        args = ["-checkmempool", "-debug=mempool"]
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, args))
        self.nodes.append(start_node(1, self.options.tmpdir, args))
        connect_nodes(self.nodes[1], 0)
        self.is_network_split = False
        self.sync_all

    def create_tx(self, from_txid, to_address, amount):
        inputs = [{ "txid" : from_txid, "vout" : 0}]
        outputs = { to_address : amount }
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
        signresult = self.nodes[0].signrawtransaction(rawtx)
        assert_equal(signresult["complete"], True)
        return signresult["hex"]

    def run_test(self):
        # At start, chain height is 200. Next block will be 201,
        # so coinbase 101 is allowed into the mempool.

        # Mine two blocks. Height is 202, so coinbases 102 and 103
        # can be in mempool.
        new_blocks = self.nodes[1].setgenerate(True, 2)
        self.sync_all()

        node0_address = self.nodes[0].getnewaddress()
        node1_address = self.nodes[1].getnewaddress()

        # Three scenarios for re-orging coinbase spends in the memory pool:
        # 1. Direct coinbase spend  :  spend_101
        # 2. Indirect (coinbase spend in chain, child in mempool) : spend_102 and spend_102_1
        # 3. Indirect (coinbase and child both in chain) : spend_103 and spend_103_1
        # Use invalidatblock to make all of the above coinbase spends invalid (immature coinbase),
        # and make sure the mempool code behaves correctly.
        b = [ self.nodes[0].getblockhash(n) for n in range(101, 104) ]
        coinbase_txids = [ self.nodes[0].getblock(h)['tx'][0] for h in b ]
        spend_101_raw = self.create_tx(coinbase_txids[0], node1_address, 50)
        spend_102_raw = self.create_tx(coinbase_txids[1], node0_address, 50)
        spend_103_raw = self.create_tx(coinbase_txids[2], node0_address, 50)

        # Broadcast and mine spend_102 and 103:
        spend_102_id = self.nodes[0].sendrawtransaction(spend_102_raw)
        spend_103_id = self.nodes[0].sendrawtransaction(spend_103_raw)
        self.nodes[0].setgenerate(True, 1)
        
        # Create 102_1 and 103_1:
        spend_102_1_raw = self.create_tx(spend_102_id, node1_address, 50)
        spend_103_1_raw = self.create_tx(spend_103_id, node1_address, 50)

        # Broadcast and mine 103_1:
        spend_103_1_id = self.nodes[0].sendrawtransaction(spend_103_1_raw)
        self.nodes[0].setgenerate(True, 1)

        # ... now put spend_101 and spend_102_1 in memory pools:
        spend_101_id = self.nodes[0].sendrawtransaction(spend_101_raw)
        spend_102_1_id = self.nodes[0].sendrawtransaction(spend_102_1_raw)

        self.sync_all()

        assert_equal(set(self.nodes[0].getrawmempool()), set([ spend_101_id, spend_102_1_id ]))

        # Use invalidateblock to re-org; only spend_101 should be left:
        for node in self.nodes:
            node.invalidateblock(new_blocks[0])

        self.sync_all()

        assert_equal(self.nodes[0].getrawmempool(), [ spend_101_id ])

if __name__ == '__main__':
    MempoolCoinbaseReorgTest().main()
