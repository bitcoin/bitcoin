#!/usr/bin/env python3
# Copyright (c) 2018 Bradley Denby
# Copyright (c) 2023-2023 The Navcoin Core developers
# Distributed under the MIT software license. See the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test transaction behaviors under the Dandelion spreading policy

Fluffing behavior:
    Stem:  0 --> 1 ... 12 --> 0 where each node supports Dandelion++
    Probe: TestNode --> 12
    Create a Dandelion++ stem phase tx on node 1.
    Assuming the fluff chance of 10%, we should reliably see tx on node 15.
"""

from test_framework.messages import (
        CInv,
        msg_getdata,
        MSG_WTX,
        MSG_DWTX,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet


class DandelionFluffingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 12
        self.extra_args = [
            ["-dandelion", "-whitelist=all@127.0.0.1"],
            ["-dandelion", "-whitelist=all@127.0.0.1"],
            ["-dandelion", "-whitelist=all@127.0.0.1"],
            ["-dandelion", "-whitelist=all@127.0.0.1"],
            ["-dandelion", "-whitelist=all@127.0.0.1"],
            ["-dandelion", "-whitelist=all@127.0.0.1"],
            ["-dandelion", "-whitelist=all@127.0.0.1"],
            ["-dandelion", "-whitelist=all@127.0.0.1"],
            ["-dandelion", "-whitelist=all@127.0.0.1"],
            ["-dandelion", "-whitelist=all@127.0.0.1"],
            ["-dandelion", "-whitelist=all@127.0.0.1"],
            ["-dandelion", "-whitelist=all@127.0.0.1"],
        ]

    # def setup_network(self):
    #     self.setup_nodes()
    #     self.connect_nodes(0, 1)
    #     self.connect_nodes(1, 2)
    #     self.connect_nodes(2, 3)
    #     self.connect_nodes(3, 4)
    #     self.connect_nodes(4, 5)
    #     self.connect_nodes(5, 6)
    #     self.connect_nodes(6, 7)
    #     self.connect_nodes(7, 8)
    #     self.connect_nodes(8, 9)
    #     self.connect_nodes(9, 10)
    #     self.connect_nodes(10, 11)
    #     self.connect_nodes(11, 0)

    def run_test(self):
        # There is a low probability that these tests will fail even if the
        # implementation is correct. Thus, these tests are repeated upon
        # failure. A true bug will result in repeated failures.
        self.log.info("Starting dandelion tests")

        self.log.info("Setting up wallet")
        wallet = MiniWallet(self.nodes[0])

        self.log.info("Create the tx on node 2")
        tx = wallet.send_self_transfer(from_node=self.nodes[0])
        txid = int(tx["wtxid"], 16)
        self.log.info("Sent tx with {}".format(txid))

        # Wait for the nodes to sync mempools
        self.log.info("Sync nodes")
        self.sync_all()

        found = False

        for i in range(self.num_nodes):
            self.log.info("Adding P2PInterface")
            peer = self.nodes[i].add_p2p_connection(P2PInterface())

            # Create and send msg_getdata for the tx
            msg = msg_getdata()
            msg.inv.append(CInv(t=MSG_WTX, h=txid))
            peer.send_and_ping(msg)
            self.log.info("Sending msg_getdata: CInv(MSG_WTX,{})".format(txid))

            if not peer.last_message.get("notfound"):
                found = True
                break

        assert found


if __name__ == "__main__":
    DandelionFluffingTest().main()
