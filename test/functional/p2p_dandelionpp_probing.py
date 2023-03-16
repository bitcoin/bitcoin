#!/usr/bin/env python3
# Copyright (c) 2018 Bradley Denby
# Copyright (c) 2023-2023 The Navcoin Core developers
# Distributed under the MIT software license. See the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test transaction behaviors under the Dandelion spreading policy

NOTE: check link for basis of this test:
https://github.com/digibyte/digibyte/blob/master/test/functional/p2p_dandelion.py

Resistance to active probing:
   Stem:  0 --> 1 --> 2 --> 0 where each node supports dandelion
   Probe: TestNode --> 0
   Node 0 generates a Dandelion transaction "tx": 1.0 BTC from Node 0 to Node 2
   TestNode immediately sends getdata for tx to Node 0
   Assert that Node 0 does not reply with tx
"""

import time

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.messages import (
        CTransaction,
        CInv,

        from_hex,
        msg_getdata,

        MSG_TX,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class DandelionP2PInterface(P2PInterface):
    def send_dandeliontx_getdata(self, tx_hash):
        msg = msg_getdata()
        msg.inv.append(CInv(MSG_TX, tx_hash))
        self.send_message(msg)

class DandelionProbingTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()
        # Tests 1,2,3: 0 --> 1 --> 2 --> 0
        self.connect_nodes(0, 1)
        self.connect_nodes(1, 2)
        self.connect_nodes(2, 0)

    def run_test(self):
        self.log.info("Setting up")

        self.log.info("Mining COINBASE_MATURITY + 1 blocks")
        self.generate(self.nodes[0], COINBASE_MATURITY + 1)

        self.log.info("Adding P2PInterface")
        peer_txer = self.nodes[0].add_p2p_connection(DandelionP2PInterface())

        # There is a low probability that these tests will fail even if the
        # implementation is correct. Thus, these tests are repeated upon
        # failure. A true bug will result in repeated failures.
        self.log.info("Starting dandelion tests")

        # Attempts: 5
        for i in range(5):
            node2_address = self.nodes[2].getnewaddress()
            txid = self.nodes[0].sendtoaddress(node2_address, 1.0)
            tx = from_hex(CTransaction(), self.nodes[0].gettransaction(txid)["hex"])

            # Test 1: Resistance to active probing
            self.log.info("Testing resistance to active probing")
            peer_txer.message_count["notfound"] = 0
            peer_txer.send_dandeliontx_getdata(tx.calc_sha256(True))

            # Give the msg some time to work
            time.sleep(10)

            # Check that our test passed
            assert(peer_txer.message_count["notfound"] == 1)
            self.log.info("Success: resistance to active probing")

if __name__ == "__main__":
    DandelionProbingTest().main()
