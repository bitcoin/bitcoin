#!/usr/bin/env python3
# Copyright (c) 2016-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test processing of feefilter messages."""

from decimal import Decimal
import time

from test_framework.messages import msg_feefilter
from test_framework.mininode import mininode_lock, P2PInterface
from test_framework.test_framework import BitcoinTestFramework


def hashToHex(hash):
    return format(hash, '064x')

# Wait up to 60 secs to see if the testnode has received all the expected invs
def allInvsMatch(invsExpected, testnode):
    for x in range(60):
        with mininode_lock:
            if (sorted(invsExpected) == sorted(testnode.txinvs)):
                return True
        time.sleep(1)
    return False

class TestP2PConn(P2PInterface):
    def __init__(self):
        super().__init__()
        self.txinvs = []

    def on_inv(self, message):
        for i in message.inv:
            if (i.type == 1):
                self.txinvs.append(hashToHex(i.hash))

    def clear_invs(self):
        with mininode_lock:
            self.txinvs = []

class FeeFilterTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        # We lower the various required feerates for this test
        # to catch a corner-case where feefilter used to slightly undercut
        # mempool and wallet feerate calculation based on GetFee
        # rounding down 3 places, leading to stranded transactions.
        # See issue #16499
        self.extra_args = [["-minrelaytxfee=0.00000100", "-mintxfee=0.00000100"]]*self.num_nodes

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node1 = self.nodes[1]
        node0 = self.nodes[0]
        # Get out of IBD
        node1.generate(1)
        self.sync_blocks()

        self.nodes[0].add_p2p_connection(TestP2PConn())

        # Test that invs are received by test connection for all txs at
        # feerate of .2 sat/byte
        node1.settxfee(Decimal("0.00000200"))
        txids = [node1.sendtoaddress(node1.getnewaddress(), 1) for x in range(3)]
        assert allInvsMatch(txids, self.nodes[0].p2p)
        self.nodes[0].p2p.clear_invs()

        # Set a filter of .15 sat/byte on test connection
        self.nodes[0].p2p.send_and_ping(msg_feefilter(150))

        # Test that txs are still being received by test connection (paying .15 sat/byte)
        node1.settxfee(Decimal("0.00000150"))
        txids = [node1.sendtoaddress(node1.getnewaddress(), 1) for x in range(3)]
        assert allInvsMatch(txids, self.nodes[0].p2p)
        self.nodes[0].p2p.clear_invs()

        # Change tx fee rate to .1 sat/byte and test they are no longer received
        # by the test connection
        node1.settxfee(Decimal("0.00000100"))
        [node1.sendtoaddress(node1.getnewaddress(), 1) for x in range(3)]
        self.sync_mempools() # must be sure node 0 has received all txs

        # Send one transaction from node0 that should be received, so that we
        # we can sync the test on receipt (if node1's txs were relayed, they'd
        # be received by the time this node0 tx is received). This is
        # unfortunately reliant on the current relay behavior where we batch up
        # to 35 entries in an inv, which means that when this next transaction
        # is eligible for relay, the prior transactions from node1 are eligible
        # as well.
        node0.settxfee(Decimal("0.00020000"))
        txids = [node0.sendtoaddress(node0.getnewaddress(), 1)]
        assert allInvsMatch(txids, self.nodes[0].p2p)
        self.nodes[0].p2p.clear_invs()

        # Remove fee filter and check that txs are received again
        self.nodes[0].p2p.send_and_ping(msg_feefilter(0))
        txids = [node1.sendtoaddress(node1.getnewaddress(), 1) for x in range(3)]
        assert allInvsMatch(txids, self.nodes[0].p2p)
        self.nodes[0].p2p.clear_invs()

if __name__ == '__main__':
    FeeFilterTest().main()
