#!/usr/bin/env python3
# Copyright (c) 2018 Bradley Denby
# Copyright (c) 2023-2023 The Navio Core developers
# Distributed under the MIT software license. See the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test transaction behaviors under the Dandelion spreading policy

Loop behavior:
   Probe: TestNode --> 0
   Node0 generates a Dandelion++ transaction "tx"
   Make sure that only stem route peer can get the INV message
   with msg_mempool requests.
"""

import time

from test_framework.messages import (
        msg_mempool,
        MSG_WTX,
        MSG_DWTX,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet

# Time in the future that tx is 100% fluffed
MAX_STEM_TIME = 999


class DandelionLoopTest(BitcoinTestFramework):
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

        self.log.info("Adding stem P2PInterface")
        with self.nodes[0].assert_debug_log(["Shuffled stem peers (found=1"]):
            stem_peer = self.nodes[0].add_p2p_connection(P2PInterface())

        self.log.info("Create the tx on node 1")
        tx = wallet.send_self_transfer(from_node=self.nodes[0])
        txid = int(tx["wtxid"], 16)
        self.log.info("Sent tx {}".format(txid))

        # Time travel into the future to make the embargo expire
        self.nodes[0].setmocktime(int(time.time() + MAX_STEM_TIME))

        # Create and send msg_mempool to node
        self.log.info("Sending msg_mempool from stem_peer")
        stem_peer.send_and_ping(msg_mempool())

        # Make sure that we got the INV as a reply from msg_mempool
        inv = stem_peer.last_message.get("inv")
        assert inv and len(inv.inv) == 1

        # Make sure that inv0.hash is the same as txid
        inv0 = inv.inv[0]
        self.log.info("Got CInv({}, {})".format(inv0.type, inv0.hash))
        assert inv0.hash == txid and inv0.type == MSG_WTX

        self.log.info("Adding P2PInterface")
        peer = self.nodes[0].add_p2p_connection(P2PInterface())

        # Create and send msg_mempool to node
        self.log.info("Sending msg_mempool from peer")
        peer.send_and_ping(msg_mempool())

        # Make sure that we got the INV as a reply from msg_mempool
        inv = peer.last_message.get("inv")
        assert inv and len(inv.inv) == 1

        # Make sure that inv0.hash is the same as txid
        inv0 = inv.inv[0]
        self.log.info("Got CInv({}, {})".format(inv0.type, inv0.hash))
        assert inv0.hash == txid and inv0.type == MSG_WTX

        self.log.info("Create the tx on node 1")
        tx = wallet.send_self_transfer(from_node=self.nodes[0])
        txid = int(tx["wtxid"], 16)
        self.log.info("Sent tx {}".format(txid))

        # Create and send msg_mempool to node
        self.log.info("Sending msg_mempool from stem_peer")
        stem_peer.send_and_ping(msg_mempool())

        # Make sure that we got the INV as a reply from msg_mempool
        inv = stem_peer.last_message.get("inv")
        assert inv and len(inv.inv) == 2

        # Make sure that inv0.hash is the same as txid
        inv0 = inv.inv[0]
        self.log.info("Got CInv({}, {})".format(inv0.type, inv0.hash))
        assert inv0.hash == txid and inv0.type == MSG_DWTX

        # Create and send msg_mempool to node
        self.log.info("Sending msg_mempool from peer")
        peer.send_and_ping(msg_mempool())

        # Make sure we did not get the new embargoed tx from mempool request
        inv = peer.last_message.get("inv")
        assert inv and len(inv.inv) == 1


if __name__ == "__main__":
    DandelionLoopTest().main()
