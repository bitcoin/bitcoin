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
   Probe: TestNode --> 0
   Node0 generates a Dandelion++ transaction "tx"
   TestNode immediately sends getdata for tx to Node0
   Assert that Node 0 does not reply with tx
"""

from test_framework.messages import (
        CInv,
        msg_getdata,
        msg_mempool,
        MSG_TX,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet

class DandelionProbingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # Make sure we are whitelisted
        self.extra_args = ["-whitelist=all@127.0.0.1"]

    def run_test(self):
        self.log.info("Setting up")
        wallet = MiniWallet(self.nodes[0])

        self.log.info("Adding P2PInterface")
        peer = self.nodes[0].add_p2p_connection(P2PInterface())

        # There is a low probability that these tests will fail even if the
        # implementation is correct. Thus, these tests are repeated upon
        # failure. A true bug will result in repeated failures.
        self.log.info("Starting dandelion tests")

        self.log.info("Create the tx")
        tx = wallet.send_self_transfer(from_node=self.nodes[0])
        txid = int(tx['txid'], 16)
        self.log.info("Sent tx with txid {}".format(txid))

        # Request for the mempool update
        peer.send_and_ping(msg_mempool())

        # Create and send msg_getdata for the tx
        msg = msg_getdata()
        msg.inv.append(CInv(t=MSG_TX, h=txid))
        peer.send_and_ping(msg)

        assert peer.last_message.get("notfound")

if __name__ == "__main__":
    DandelionProbingTest().main()
