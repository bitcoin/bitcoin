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
        self.num_nodes = 1

    def run_test(self):
        self.log.info("Setting up")
        wallet = MiniWallet(self.nodes[0])

        self.log.info("Adding P2PInterface")
        peer = self.nodes[0].add_p2p_connection(P2PInterface())

        # There is a low probability that these tests will fail even if the
        # implementation is correct. Thus, these tests are repeated upon
        # failure. A true bug will result in repeated failures.
        self.log.info("Starting dandelion tests")

        MAX_REPEATS = 10
        self.log.info("Running test up to {} times.".format(MAX_REPEATS))
        for i in range(MAX_REPEATS):
            self.log.info('Run repeat {}'.format(i + 1))

            self.log.info("Create the tx")
            tx = wallet.send_self_transfer(from_node=self.nodes[0])
            txid = int(tx['txid'], 16)
            self.log.info("Sent tx with txid {}".format(txid))

            self.nodes[0].setmocktime(int(time.time() + MAX_GETDATA_INBOUND_WAIT))

            msg = msg_getdata()
            msg.inv.append(CInv(t=MSG_TX, h=txid))
            peer.send_and_ping(msg)

            assert peer.last_message.get("notfound")

if __name__ == "__main__":
    DandelionProbingTest().main()
