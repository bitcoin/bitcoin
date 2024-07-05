#!/usr/bin/env python3
# Copyright (c) 2018 Bradley Denby
# Copyright (c) 2023-2023 The Navio Core developers
# Distributed under the MIT software license. See the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test transaction behaviors under the Dandelion spreading policy

Fluffing behavior:
    Stem:  0 --> 1 where each node supports Dandelion++
    Probe: TestNode --> 1
    Create a Dandelion++ stem phase tx on node 1.
    Assuming the fluff chance of 10%, we should reliably see tx on node 1
    about 10% of the time.
"""

from test_framework.messages import (
        CInv,
        msg_getdata,
        msg_mempool,
        MSG_WTX,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet


class DandelionFluffingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.max_attempts = 100
        self.tx_count = 5
        self.extra_args = [
            ["-dandelion", "-whitelist=all@127.0.0.1"],
            ["-dandelion", "-whitelist=all@127.0.0.1"],
        ]

    def setup_network(self):
        self.setup_nodes()
        self.connect_nodes(0, 1)

    def run_test(self):
        # There is a low probability that these tests will fail even if the
        # implementation is correct. Thus, these tests are repeated upon
        # failure. A true bug will result in repeated failures.
        self.log.info("Starting dandelion tests")

        self.log.info("Setting up wallet")
        wallet = MiniWallet(self.nodes[0])

        self.log.info("Adding P2PInterface")
        peer = self.nodes[1].add_p2p_connection(P2PInterface())

        # Wait for the nodes to sync mempools
        self.log.info("Sync nodes")
        self.sync_all()

        # Track attempts and fluffs
        attempt = 0
        fluffed = 0

        while attempt < self.max_attempts:
            attempt += 1
            self.log.info("Attempt {}".format(attempt))

            txids = []
            for i in range(self.tx_count):
                tx = wallet.send_self_transfer(from_node=self.nodes[0])
                txids += [int(tx["wtxid"], 16)]
                self.log.info("Sent tx {}".format(txids[i]))

            # Wait for the nodes to sync mempools
            self.log.info("Sync nodes")
            self.sync_all()

            for i in range(self.tx_count):
                # Create and send msg_mempool to node to bypass mempool request
                # security
                peer.send_and_ping(msg_mempool())

                # Create and send msg_getdata for the tx
                msg = msg_getdata()
                msg.inv.append(CInv(t=MSG_WTX, h=txids[i]))
                peer.send_and_ping(msg)
                self.log.info("Sending msg_getdata: CInv(MSG_WTX,{})".format(txids[i]))

                last_tx = peer.last_message.get("tx")
                if last_tx and int(last_tx.tx.getwtxid(), 16) == txids[i]:
                    self.log.info("Tx {} fluffed on node 2".format(txids[i]))
                    fluffed += 1

            if fluffed > 0:
                break

        self.log.info("Fluffed tx {}%".format((fluffed / (attempt * self.tx_count)) * 100))
        assert fluffed > 0


if __name__ == "__main__":
    DandelionFluffingTest().main()
