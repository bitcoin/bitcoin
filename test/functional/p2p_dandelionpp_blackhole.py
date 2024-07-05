#!/usr/bin/env python3
# Copyright (c) 2018 Bradley Denby
# Copyright (c) 2023-2023 The Navio Core developers
# Distributed under the MIT software license. See the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test transaction behaviors under the Dandelion spreading policy

NOTE: check link for basis of this test:
https://github.com/digibyte/digibyte/blob/master/test/functional/p2p_dandelion.py

Resistance to black holes:
    Stem:  0 --> 1 --> 2 --> 0 where each node supports Dandelion++
    Probe: TestNode --> 0
    Wait ~60 seconds after creating the tx (this should be enough time this
    should expired the embargo on the tx), then TestNode sends getdata for tx
    to Node 0.
    Assert that Node 0 replies with tx (meaning the tx has fluffed)
"""

import time

from test_framework.messages import msg_mempool
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet

# Time in the future that tx is 100% fluffed
MAX_STEM_TIME = 999


class DandelionBlackholeTest(BitcoinTestFramework):
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

        self.log.info("Adding P2PInterface stem")
        with self.nodes[0].assert_debug_log(["Shuffled stem peers (found=1"]):
            self.nodes[0].add_p2p_connection(P2PInterface())  # Fake stem peer

        self.log.info("Create the tx on node 1")
        tx = wallet.send_self_transfer(from_node=self.nodes[0])
        txid = int(tx['wtxid'], 16)
        self.log.info("Sent tx {}".format(txid))

        self.log.info("Adding P2PInterface probe")
        peer = self.nodes[0].add_p2p_connection(P2PInterface())  # Probing peer

        # Time travel MAX_STEM_TIME seconds into the future
        # to force embargo to expire and get our tx returned
        self.nodes[0].setmocktime(int(time.time() + MAX_STEM_TIME))

        res_tx = peer.last_message.get("tx")
        if not res_tx:
            # Create and send msg_mempool to node to bypass mempool request
            # security
            peer.send_and_ping(msg_mempool())
            res_tx = peer.last_message.get("tx")

        self.log.info(res_tx)
        assert res_tx and res_tx.tx

        self.log.info("Got tx {}".format(int(res_tx.tx.getwtxid(), 16)))
        assert int(res_tx.tx.getwtxid(), 16) == txid


if __name__ == "__main__":
    DandelionBlackholeTest().main()
