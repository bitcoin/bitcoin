#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test REQTXRCNCL message
"""

import time
import math

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet

# From txreconciliation.cpp
# Interval the node takes to reconcile with all peers. Each peer is reconciled every 1-nth
RECON_REQUEST_INTERVAL = 30
Q = 0.25
Q_PRECISION = (2 << 14) - 1

class ReqTxrcnclReceiver(P2PInterface):
    def __init__(self):
        super().__init__(support_txrcncl = True)
        self.reqtxrcncl_msg_received = None
        self.received_inv_items = 0

    def on_inv(self, message):
        self.received_inv_items += len(message.inv)
        super().on_inv(message)

    def on_reqtxrcncl(self, message):
        self.reqtxrcncl_msg_received = message

class ReqTxRcnclTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [['-txreconciliation']]

    def run_test(self):
        self.nodes[0].setmocktime(int(time.time()))
        self.generate(self.nodes[0], COINBASE_MATURITY) # mature coinbase UTXO used later

        # Check everything about *sending* REQTXRCNCL.
        # Connecting to an outbound fanout peer does not trigger a reconciliation request
        fanout_peer = self.nodes[0].add_outbound_p2p_connection(ReqTxrcnclReceiver(), wait_for_verack=True, p2p_idx=0)
        self.nodes[0].bumpmocktime(RECON_REQUEST_INTERVAL + 1)
        assert not fanout_peer.reqtxrcncl_msg_received

        # Connecting to an outbound reconciliation peer triggers sending a REQTXRCNCL message after RECON_REQUEST_INTERVAL
        self.log.info('REQTXRCNCL sent to peer 1 (outbound)')
        recon_peer0 = self.nodes[0].add_outbound_p2p_connection(ReqTxrcnclReceiver(), wait_for_verack=True, p2p_idx=1, connection_type="outbound-full-recon")
        self.nodes[0].bumpmocktime(RECON_REQUEST_INTERVAL + 1)
        self.wait_until(lambda: recon_peer0.reqtxrcncl_msg_received)

        # No transaction were created, so we expect the set size to be 0, and Q to be default
        assert_equal(recon_peer0.reqtxrcncl_msg_received.set_size, 0)
        assert_equal(recon_peer0.reqtxrcncl_msg_received.q, int(math.ceil(Q_PRECISION * Q)))

        # Since recon_peer0 has not responded to the reconciliation request, the turn is passed to recon_peer1.
        # Had peer 0 finished the reconciliation process, it would have been peer's 0 turn again, given it was
        # the only registered peer at the time.
        self.log.info('REQTXRCNCL sent to outbound peer 1')
        recon_peer0.reqtxrcncl_msg_received = None
        with self.nodes[0].assert_debug_log(["Register peer=2", "Register peer=3"]):
            recon_peer1 = self.nodes[0].add_outbound_p2p_connection(ReqTxrcnclReceiver(), wait_for_verack=True, p2p_idx=2, connection_type="outbound-full-recon")
            recon_peer2 = self.nodes[0].add_outbound_p2p_connection(ReqTxrcnclReceiver(), wait_for_verack=True, p2p_idx=3, connection_type="outbound-full-recon")

        # The node takes RECON_REQUEST_INTERVAL to send a REQTXRCNCL to the recon_peer1, since the timer is set on the first
        # peer connects.
        self.nodes[0].bumpmocktime(RECON_REQUEST_INTERVAL + 1)
        self.wait_until(lambda: recon_peer1.reqtxrcncl_msg_received)
        assert not recon_peer0.reqtxrcncl_msg_received
        assert not recon_peer2.reqtxrcncl_msg_received

        # After the timer restarts, the interval per peer is now a third, since it is computed over three peers
        self.log.info('REQTXRCNCL sent to an peer 2')
        recon_peer1.reqtxrcncl_msg_received = None
        self.nodes[0].bumpmocktime(math.ceil(RECON_REQUEST_INTERVAL/3) + 1)
        self.wait_until(lambda: recon_peer2.reqtxrcncl_msg_received)
        assert not recon_peer0.reqtxrcncl_msg_received
        assert not recon_peer1.reqtxrcncl_msg_received
        self.nodes[0].disconnect_p2ps()

        self.log.info('Check transactions announced via reconciliation')
        with self.nodes[0].assert_debug_log(["Register peer=4", "Register peer=5"]):
            recon_peer4 = self.nodes[0].add_outbound_p2p_connection(ReqTxrcnclReceiver(), wait_for_verack=True, p2p_idx=4, connection_type="outbound-full-recon")
            recon_peer5 = self.nodes[0].add_outbound_p2p_connection(ReqTxrcnclReceiver(), wait_for_verack=True, p2p_idx=5, connection_type="outbound-full-recon")

        wallet = MiniWallet(self.nodes[0])
        utxos = wallet.get_utxos()
        for utxo in utxos:
            # We want all transactions to be childless.
            wallet.send_self_transfer(from_node=self.nodes[0], utxo_to_spend=utxo)

        # Since we have disconnected all peers, connecting one again will reset the interval to RECON_REQUEST_INTERVAL per peer
        self.nodes[0].bumpmocktime(RECON_REQUEST_INTERVAL + 1)
        self.wait_until(lambda: recon_peer4.reqtxrcncl_msg_received)
        # After one iteration, it will go down to 1/2 again
        self.nodes[0].bumpmocktime(math.ceil(RECON_REQUEST_INTERVAL/2) + 1)
        self.wait_until(lambda: recon_peer5.reqtxrcncl_msg_received)

        for peer in (recon_peer4, recon_peer5):
            assert_equal(peer.reqtxrcncl_msg_received.set_size, len(utxos))
            assert_equal(peer.received_inv_items, 0)

if __name__ == '__main__':
    ReqTxRcnclTest(__file__).main()
