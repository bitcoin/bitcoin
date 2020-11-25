#!/usr/bin/env python3
# Copyright (c) 2017-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test VeriBlock PoP merkle root calculation"""

# Avoid wildcard * imports
from test_framework.pop import mine_until_pop_enabled
from test_framework.blocktools import (create_block, create_coinbase)
from test_framework.mininode import (
    P2PInterface,
    msg_block,
)
from test_framework.test_framework import BitcoinTestFramework


class PoPMerkleRootTest(BitcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self):
        self.add_nodes(self.num_nodes)
        self.start_node(0)
        # POP should be enabled because merkle root calculation differs for non-POP blocks
        mine_until_pop_enabled(self.nodes[0])

    def run_test(self):
        """Main test logic"""
        lastblock = self.nodes[0].getblockcount()
        self.nodes[0].add_p2p_connection(P2PInterface())

        # Generating a block on one of the nodes will get us out of IBD
        blockhashhex = self.nodes[0].generate(nblocks=1)[0]
        block = self.nodes[0].getblock(blockhashhex)
        height = block['height']
        blocktime = block['time']

        # create a block
        assert self.nodes[0].getblockchaininfo()['softforks']['pop_security']['active'], "POP is not activated"
        block = create_block(self.nodes[0], int(blockhashhex, 16), create_coinbase(height + 1), blocktime + 1)
        block.solve()
        block_message = msg_block(block)
        # Send message is used to send a P2P message to the node over our P2PInterface
        self.nodes[0].p2p.send_message(block_message)
        self.nodes[0].waitforblockheight(lastblock + 2)
        newbest = self.nodes[0].getbestblockhash()
        assert newbest == block.hash, "bad tip. \n\tExpected : {}\n\tGot      : {}".format(block, newbest)


if __name__ == '__main__':
    PoPMerkleRootTest().main()
