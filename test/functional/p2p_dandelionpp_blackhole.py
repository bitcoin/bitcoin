#!/usr/bin/env python3
# Copyright (c) 2018 Bradley Denby
# Copyright (c) 2023-2023 The Navcoin Core developers
# Distributed under the MIT software license. See the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test transaction behaviors under the Dandelion spreading policy

NOTE: check link for basis of this test:
https://github.com/digibyte/digibyte/blob/master/test/functional/p2p_dandelion.py

Resistance to black holes:
    Stem:  0 --> 1 --> 2 --> 0 where each node supports Dandelion++
    Probe: TestNode --> 0
    Wait ~1 second after creating the tx (this should be enough time for the
    tx to propogate through the network with a regular mempool tx), then
    TestNode sends getdata for tx to Node 0
    Assert that Node 0 replies with tx (meaning the tx has fluffed)
"""

import time

from test_framework.messages import (
        CInv,
        msg_getdata,
        MSG_WTX,
        MSG_DWTX,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet

# Max time a tx should spend in stem phase
MAX_STEM_TIME = 60


class DandelionBlackholeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [
            ["-dandelion", "-whitelist=all@127.0.0.1"],
            ["-dandelion", "-whitelist=all@127.0.0.1"],
            ["-dandelion", "-whitelist=all@127.0.0.1"],
        ]

    def run_test(self):
        # There is a low probability that these tests will fail even if the
        # implementation is correct. Thus, these tests are repeated upon
        # failure. A true bug will result in repeated failures.
        self.log.info("Starting dandelion tests")

        self.log.info("Setting up wallet")
        wallet = MiniWallet(self.nodes[0])

        self.log.info("Adding P2PInterface")
        peer = self.nodes[0].add_p2p_connection(P2PInterface())

        self.log.info("Create the tx on node 1")
        tx = wallet.send_self_transfer(from_node=self.nodes[0])
        txid = int(tx['wtxid'], 16)
        self.log.info("Sent tx {}".format(txid))

        # Time travel MAX_STEM_TIME seconds into the future
        # to force embargo to expire and get our tx returned
        mocktime = int(time.time() + MAX_STEM_TIME)
        for hop in range(self.num_nodes):
            for node in self.nodes:
                node.setmocktime(mocktime)

        # Wait for the nodes to sync mempools
        self.log.info("Sync nodes")
        self.sync_all()

        for tx_type in [MSG_WTX, MSG_DWTX]:
            # Create and send msg_getdata for the tx
            msg = msg_getdata()
            msg.inv.append(CInv(t=tx_type, h=txid))
            peer.send_and_ping(msg)
            self.log.info("Sending msg_getdata: CInv({}, {})".format(tx_type, txid))

            assert not peer.last_message.get("notfound")


if __name__ == "__main__":
    DandelionBlackholeTest().main()
