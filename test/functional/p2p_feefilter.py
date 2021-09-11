#!/usr/bin/env python3
# Copyright (c) 2016-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test processing of feefilter messages."""

from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.messages import MSG_TX, MSG_WTX, msg_feefilter
from test_framework.p2p import P2PInterface, p2p_lock
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


class FeefilterConn(P2PInterface):
    feefilter_received = False

    def on_feefilter(self, message):
        self.feefilter_received = True

    def assert_feefilter_received(self, recv: bool):
        with p2p_lock:
            assert_equal(self.feefilter_received, recv)


class TestP2PConn(P2PInterface):
    def __init__(self):
        super().__init__()
        self.txinvs = []

    def on_inv(self, message):
        for i in message.inv:
            if (i.type == MSG_TX) or (i.type == MSG_WTX):
                self.txinvs.append('{:064x}'.format(i.hash))

    def wait_for_invs_to_match(self, invs_expected):
        invs_expected.sort()
        self.wait_until(lambda: invs_expected == sorted(self.txinvs))

    def clear_invs(self):
        with p2p_lock:
            self.txinvs = []


class FeeFilterTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        # We lower the various required feerates for this test
        # to catch a corner-case where feefilter used to slightly undercut
        # mempool and wallet feerate calculation based on GetFee
        # rounding down 3 places, leading to stranded transactions.
        # See issue #16499
        # grant noban permission to all peers to speed up tx relay / mempool sync
        self.extra_args = [[
            "-minrelaytxfee=0.00000100",
            "-mintxfee=0.00000100",
            "-whitelist=noban@127.0.0.1",
        ]] * self.num_nodes

    def run_test(self):
        self.test_feefilter_forcerelay()
        self.test_feefilter()
        self.test_feefilter_blocksonly()

    def test_feefilter_forcerelay(self):
        self.log.info('Check that peers without forcerelay permission (default) get a feefilter message')
        self.nodes[0].add_p2p_connection(FeefilterConn()).assert_feefilter_received(True)

        self.log.info('Check that peers with forcerelay permission do not get a feefilter message')
        self.restart_node(0, extra_args=['-whitelist=forcerelay@127.0.0.1'])
        self.nodes[0].add_p2p_connection(FeefilterConn()).assert_feefilter_received(False)

        # Restart to disconnect peers and load default extra_args
        self.restart_node(0)
        self.connect_nodes(1, 0)

    def test_feefilter(self):
        node1 = self.nodes[1]
        node0 = self.nodes[0]
        miniwallet = MiniWallet(node1)
        # Add enough mature utxos to the wallet, so that all txs spend confirmed coins
        self.generate(miniwallet, 5)
        self.generate(node1, COINBASE_MATURITY)

        conn = self.nodes[0].add_p2p_connection(TestP2PConn())

        self.log.info("Test txs paying 0.2 sat/byte are received by test connection")
        txids = [miniwallet.send_self_transfer(fee_rate=Decimal('0.00000200'), from_node=node1)['wtxid'] for _ in range(3)]
        conn.wait_for_invs_to_match(txids)
        conn.clear_invs()

        # Set a fee filter of 0.15 sat/byte on test connection
        conn.send_and_ping(msg_feefilter(150))

        self.log.info("Test txs paying 0.15 sat/byte are received by test connection")
        txids = [miniwallet.send_self_transfer(fee_rate=Decimal('0.00000150'), from_node=node1)['wtxid'] for _ in range(3)]
        conn.wait_for_invs_to_match(txids)
        conn.clear_invs()

        self.log.info("Test txs paying 0.1 sat/byte are no longer received by test connection")
        txids = [miniwallet.send_self_transfer(fee_rate=Decimal('0.00000100'), from_node=node1)['wtxid'] for _ in range(3)]
        self.sync_mempools()  # must be sure node 0 has received all txs

        # Send one transaction from node0 that should be received, so that we
        # we can sync the test on receipt (if node1's txs were relayed, they'd
        # be received by the time this node0 tx is received). This is
        # unfortunately reliant on the current relay behavior where we batch up
        # to 35 entries in an inv, which means that when this next transaction
        # is eligible for relay, the prior transactions from node1 are eligible
        # as well.
        txids = [miniwallet.send_self_transfer(fee_rate=Decimal('0.00020000'), from_node=node0)['wtxid'] for _ in range(1)]
        conn.wait_for_invs_to_match(txids)
        conn.clear_invs()
        self.sync_mempools()  # must be sure node 1 has received all txs

        self.log.info("Remove fee filter and check txs are received again")
        conn.send_and_ping(msg_feefilter(0))
        txids = [miniwallet.send_self_transfer(fee_rate=Decimal('0.00020000'), from_node=node1)['wtxid'] for _ in range(3)]
        conn.wait_for_invs_to_match(txids)
        conn.clear_invs()

    def test_feefilter_blocksonly(self):
        """Test that we don't send fee filters to block-relay-only peers and when we're in blocksonly mode."""
        self.log.info("Check that we don't send fee filters to block-relay-only peers.")
        feefilter_peer = self.nodes[0].add_outbound_p2p_connection(FeefilterConn(), p2p_idx=0, connection_type="block-relay-only")
        feefilter_peer.sync_with_ping()
        feefilter_peer.assert_feefilter_received(False)

        self.log.info("Check that we don't send fee filters when in blocksonly mode.")
        self.restart_node(0, ["-blocksonly"])
        feefilter_peer = self.nodes[0].add_p2p_connection(FeefilterConn())
        feefilter_peer.sync_with_ping()
        feefilter_peer.assert_feefilter_received(False)


if __name__ == '__main__':
    FeeFilterTest().main()
