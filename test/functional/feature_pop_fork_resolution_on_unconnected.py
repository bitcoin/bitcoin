#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2021 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

"""
Start 2 nodes.
Disconnect node[1].
Mine 100 orphans on node[0].
node[1] mines 2 blocks, total height is 202 (fork B)
node[1] is connected to node[0]
send header of the block 201
send block 202

After sync has been completed, expect FindBestChain called with 100 candidates
"""

from test_framework.mininode import (
    P2PInterface,
)

from test_framework.blocktools import create_block, create_coinbase, create_tx_with_script
from test_framework.pop import mine_until_pop_active, PopMiningContext
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    connect_nodes, disconnect_nodes, assert_equal, wait_until
)
from test_framework.messages import (
    CBlock, CBlockHeader, msg_headers, msg_sendheaders, msg_block
)


class BaseNode(P2PInterface):
    def __init__(self, log=None):
        super().__init__()
        self.log = log

class PopFrUnconnected(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-txindex"]]
        self.extra_args = [x + ['-debug=pop'] for x in self.extra_args]
        self.orphans_to_generate = 100

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypoptools()

    def setup_network(self):
        self.setup_nodes()
        mine_until_pop_active(self.nodes[0])
        self.popctx = PopMiningContext(self.nodes[0])

    def get_best_block(self, node):
        hash = node.getbestblockhash()
        return node.getblock(hash)

    def _find_best_chain_on_unconnected_block(self):
        self.log.warning("starting _find_best_chain_on_unconnected_block()")
        lastblock = self.nodes[0].getblockcount()

        candidates = []
        for i in range(self.orphans_to_generate):
            addr1 = self.nodes[0].getnewaddress()
            hash = self.nodes[0].generatetoaddress(nblocks=1, address=addr1)[-1]
            candidates.append(hash)
            self.invalidatedheight = lastblock + 1
            self.invalidated = self.nodes[0].getblockhash(self.invalidatedheight)
            self.nodes[0].invalidateblock(self.invalidated)
            new_lastblock = self.nodes[0].getblockcount()
            assert new_lastblock == lastblock

        for c in candidates:
            self.nodes[0].reconsiderblock(c)

        self.log.info("node0 generated {} orphans".format(self.orphans_to_generate))
        assert self.get_best_block(self.nodes[0])['height'] == lastblock + 1

        compares_before = self.nodes[0].getpopscorestats()['stats']['popScoreComparisons']

        # connect to fake node
        bn = BaseNode(self.log)
        self.nodes[0].add_p2p_connection(bn)

        # generate 2 blocks to send from the fake node

        block_to_connect_hash = self.nodes[0].getblockhash(lastblock)
        block_to_connect = self.nodes[0].getblock(block_to_connect_hash)
        tip = int(block_to_connect_hash, 16)
        height = block_to_connect["height"] + 1
        block_time = block_to_connect["time"] + 1

        block1 = create_block(self.popctx, tip, create_coinbase(height), block_time)
        block1.solve()

        headers_message = msg_headers()
        headers_message.headers = [CBlockHeader(block1)]
        self.nodes[0].p2p.send_and_ping(headers_message)
        self.popctx.accept_block(height, block1.hash, block_to_connect_hash)

        tip = int(block1.hash, 16)
        height = height + 1
        block_time = block_time + 1

        block2 = create_block(self.popctx, tip, create_coinbase(height + 1), block_time + 1)
        block2.solve()

        block_message = msg_block(block2)
        self.nodes[0].p2p.send_and_ping(block_message)

        prevbest = self.nodes[0].getblockhash(lastblock + 1)
        newbest = self.nodes[0].getbestblockhash()
        assert newbest == prevbest, "bad tip. \n\tExpected : {}\n\tGot      : {}".format(prevbest, newbest)

        compares_after = self.nodes[0].getpopscorestats()['stats']['popScoreComparisons']
        test_comparisons = compares_after - compares_before
        assert test_comparisons == self.orphans_to_generate, "Expected {} comparisons, got {}".format(self.orphans_to_generate, test_comparisons)
        self.log.info("node0 made {} POP score comparisons".format(test_comparisons))

        assert self.get_best_block(self.nodes[0])['height'] == lastblock + 1
        self.log.warning("_find_best_chain_on_unconnected_block() succeeded!")

    def run_test(self):
        """Main test logic"""

        from pypoptools.pypopminer import MockMiner
        self.apm = MockMiner()

        self._find_best_chain_on_unconnected_block()


if __name__ == '__main__':
    PopFrUnconnected().main()
