#!/usr/bin/env python3
# Copyright (c) 2018 Bradley Denby
# Copyright (c) 2023-2023 The Navio Core developers
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
        MSG_WTX,
        MSG_DWTX,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet


class DandelionProbingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-dandelion", "-whitelist=all@127.0.0.1"]]

    def run_test(self):
        # There is a low probability that these tests will fail even if the
        # implementation is correct. Thus, these tests are repeated upon
        # failure. A true bug will result in repeated failures.
        self.log.info("Starting dandelion tests")

        self.log.info("Setting up wallet")
        wallet = MiniWallet(self.nodes[0])

        self.log.info("Create the tx on node 1")
        tx = wallet.send_self_transfer(from_node=self.nodes[0])
        txid = int(tx['wtxid'], 16)
        self.log.info("Sent tx with {}".format(txid))

        self.log.info("Adding P2PInterface")
        self.nodes[0].add_p2p_connection(P2PInterface())  # Fake dandelion peer
        peer = self.nodes[0].add_p2p_connection(P2PInterface()) # Probing peer

        for tx_type in [MSG_WTX, MSG_DWTX]:
            # Create and send msg_mempool to node to bypass mempool request
            # security
            peer.send_and_ping(msg_mempool())

            # Create and send msg_getdata for the tx
            msg = msg_getdata()
            msg.inv.append(CInv(t=tx_type, h=txid))
            peer.send_and_ping(msg)
            self.log.info("Sending msg_getdata: CInv({}, {})".format(tx_type, txid))

            assert peer.last_message.get("notfound")


if __name__ == "__main__":
    DandelionProbingTest().main()
