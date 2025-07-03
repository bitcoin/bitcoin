#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import time

from test_framework.blocktools import (
    create_block,
)
from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    msg_block,
)
from test_framework.p2p import (
    P2PInterface,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    softfork_active,
)
class CompactBlockInvalidForward(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def test_invalid_cb_stall(self):
        self.log.info("Test that a compact block sent for invalid block results in a getdata")

        test_node = self.nodes[0].add_p2p_connection(P2PInterface())

        now = int(time.time())
        for node in self.nodes:
            node.setmocktime(now)

        # Make blocks to set chaintip time to "now"
        self.generate(self.nodes[0], 12)

        # Make 200+ minutes pass with wiggle room added
        # to disallow direct fetch of block
        for node in self.nodes:
            node.bumpmocktime(200 * 60 * 2)

        # "Mine" an invalid block that passes PoW and merkle root checks
        # send it to node0 via p2p to have it forward via cb to node1
        # but node1 will deem it not directly fetchable due to 200m+ passing
        missing_input = CTransaction()
        missing_input.vin.append(CTxIn(COutPoint(0, 0), b""))
        missing_input.vout.append(CTxOut())
        block = create_block(tmpl=self.nodes[0].getblocktemplate({"rules": ["segwit"]}), txlist=[missing_input])
        block.solve()
        test_node.send_without_ping(msg_block(block))

        # Both peers now have knowledge of the invalid block, and have not disconnected
        for node in self.nodes:
            self.wait_until(lambda: len(node.getchaintips()) == 2)
            assert {"height": 13, "hash": block.hash, "branchlen": 1, "status": "invalid"} in node.getchaintips()

        assert_equal(len(self.nodes[1].getpeerinfo()), 1)


    def run_test(self):

        assert softfork_active(self.nodes[0], "segwit")

        self.test_invalid_cb_stall()

if __name__ == '__main__':
    CompactBlockInvalidForward(__file__).main()
