#!/usr/bin/env python3
# Copyright (c) 2018 Bradley Denby
# Copyright (c) 2023-2023 The Navcoin Core developers
# Distributed under the MIT software license. See the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test transaction behaviors under the Dandelion spreading policy

NOTE: check link for basis of this test:
https://github.com/digibyte/digibyte/blob/master/test/functional/p2p_dandelion.py

Loop behavior:
   Stem:  0 --> 1 --> 2 --> 0 where each node has argument "-dandelion=1"
   Probe: TestNode --> 0
   Wait ~5 seconds after Test 1, then TestNode sends getdata for tx to Node 0
   Assert that Node 0 does not reply with tx
"""

import time

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.messages import (
        CInv,
        msg_getdata,
        MSG_TX,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet

# Constants from net_processing
GETDATA_TX_INTERVAL = 60  # seconds
INBOUND_PEER_TX_DELAY = 2  # seconds
TXID_RELAY_DELAY = 2 # seconds

# Python test constants
#MAX_GETDATA_INBOUND_WAIT = GETDATA_TX_INTERVAL + INBOUND_PEER_TX_DELAY + TXID_RELAY_DELAY
MAX_GETDATA_INBOUND_WAIT = 120 #

class DandelionProbingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3

    def setup_network(self):
        self.setup_nodes()
        # Tests 1,2,3: 0 --> 1 --> 2 --> 0
        self.connect_nodes(0, 1)
        self.connect_nodes(1, 2)
        self.connect_nodes(2, 0)

    def run_test(self):
        self.log.info("Setting up")
        wallet = MiniWallet(self.nodes[0])

        self.log.info("Adding P2PInterface")

        # There is a low probability that these tests will fail even if the
        # implementation is correct. Thus, these tests are repeated upon
        # failure. A true bug will result in repeated failures.
        self.log.info("Starting dandelion tests")

        MAX_REPEATS = 10
        self.log.info("Running test up to {} times.".format(MAX_REPEATS))
        for i in range(MAX_REPEATS):
            self.log.info('Run repeat {}'.format(i + 1))

            self.log.info("Create the tx on node 2")
            tx = wallet.send_self_transfer(from_node=self.nodes[1])
            txid = int(tx['txid'], 16)
            self.log.info("Sent tx with txid {}".format(txid))

            mocktime = int(time.time() + MAX_GETDATA_INBOUND_WAIT)

            for hop in range(self.num_nodes):
                for node in self.nodes:
                    node.setmocktime(mocktime + mocktime*hop)
                time.sleep(1)

            peer = self.nodes[0].add_p2p_connection(P2PInterface())
            msg = msg_getdata()
            msg.inv.append(CInv(t=MSG_TX, h=txid))
            peer.send_and_ping(msg)

            assert peer.last_message.get("notfound")

if __name__ == "__main__":
    DandelionProbingTest().main()
