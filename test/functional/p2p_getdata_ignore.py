#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test node responses to invalid transactions.

In this test we connect to one node over p2p, and test tx requests."""
from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import (
    CInv,
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    msg_inv,
)
from test_framework.mininode import P2PDataStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)
import time

class InvalidTxRequestTest(BitcoinTestFramework):
    SCRIPT_PUB_KEY_OP_TRUE = b'\x51\x75' * 15 + b'\x51'

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.withheld_tx_offset = 0

    def bootstrap_p2p(self, *, num_connections=1):
        """Add a P2P connection to the node.

        Helper to connect and wait for version handshake."""
        for _ in range(num_connections):
            self.nodes[0].add_p2p_connection(P2PDataStore())

    def reconnect_p2p(self, **kwargs):
        """Tear down and bootstrap the P2P connection to the node.

        The node gets disconnected several times in this test. This helper
        method reconnects the p2p and restarts the network thread."""
        self.nodes[0].disconnect_p2ps()
        self.bootstrap_p2p(**kwargs)

    def p2p_withheld_tx(self, num):
        node = self.nodes[0]  # convenience reference to the node
        for i in range(num):
            tx_withhold = CTransaction()
            tx_withhold.vin.append(CTxIn(outpoint=COutPoint(self.block1.vtx[0].sha256, 0)))
            tx_withhold.vout.append(CTxOut(nValue=50 * COIN - 12000 + self.withheld_tx_offset + i, scriptPubKey=self.SCRIPT_PUB_KEY_OP_TRUE))
            tx_withhold.calc_sha256()
            self.log.debug("Withheld tx %d %s" % (self.withheld_tx_offset+i+1, tx_withhold.hash))
            node.p2p.send_message(msg_inv(inv=[CInv(1, tx_withhold.sha256)]))
        self.withheld_tx_offset += num
        time.sleep(5) # give it time to actually do some getdata requests

    def run_test(self):
        node = self.nodes[0]  # convenience reference to the node
        mt = int(time.time())
        node.setmocktime(mt)

        self.bootstrap_p2p()  # Add one p2p connection to the node

        best_block = self.nodes[0].getbestblockhash()
        tip = int(best_block, 16)
        best_block_time = self.nodes[0].getblock(best_block)['time']
        block_time = best_block_time + 1

        self.log.info("Create a new block with an anyone-can-spend coinbase.")
        height = 1
        block = create_block(tip, create_coinbase(height), block_time)
        block.solve()
        # Save the coinbase for later
        self.block1 = block
        tip = block.sha256
        node.p2p.send_blocks_and_test([block], node, success=True)

        self.log.info("Mature the block.")
        self.nodes[0].generatetoaddress(100, self.nodes[0].get_deterministic_priv_key().address)

        self.log.info('Test withheld transaction handling ... ')
        # Create a root transaction that we withhold until all dependent transactions
        # are sent out and in the orphan cache

        time.sleep(1) # catch up
        node.p2p.getdata_requests = [] # clear this
        self.p2p_withheld_tx(120)

        assert_equal(0, node.getmempoolinfo()['size'])  # Mempool should be empty
        assert_equal(1, len(node.getpeerinfo()))  # still connected
        assert_equal(len(node.p2p.getdata_requests), 100) # stop at >100 getdata requests
        self.log.info("Correctly asked for 100 transactions out of 120!")
        node.p2p.getdata_requests = [] # clear again

        self.log.info("Have 40 unasked-for transactions queued at 2min...")
        node.setmocktime(mt+2*60)
        self.p2p_withheld_tx(20)
        assert_equal(len(node.p2p.getdata_requests), 0)
        assert_equal(node.getpeerinfo()[0]["tx_process"], 40)

        self.log.info("Have 60 unasked-for transactions queued at 28min...")
        node.setmocktime(mt+28*60)
        self.p2p_withheld_tx(20)
        assert_equal(len(node.p2p.getdata_requests), 0)
        assert_equal(node.getpeerinfo()[0]["tx_process"], 60)

        self.log.info("After 30 minutes, have disconnected")
        node.setmocktime(mt+32*60)
        time.sleep(1)
        assert_equal(len(node.p2p.getdata_requests), 0)
        assert_equal(0, node.getmempoolinfo()['size'])  # Mempool should be empty
        assert_equal(0, len(node.getpeerinfo()))  # disconnected

if __name__ == '__main__':
    InvalidTxRequestTest().main()
