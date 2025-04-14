#!/usr/bin/env python3
# Copyright (c) 2021-2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test reconciliation-based transaction relay (node responds)"""

import time

from test_framework.messages import msg_reqtxrcncl
from test_framework.p2p import P2PDataStore
from test_framework.p2p_txrecon import (
    create_sketch, get_short_id, estimate_capacity,
    Q_PRECISION, RECON_Q, ReconciliationTest, TxReconTestP2PConn
)
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet

# Taken from net_processing.cpp
INBOUND_INVENTORY_BROADCAST_INTERVAL = 5


class ReconciliationResponderTest(ReconciliationTest):
    def set_test_params(self):
        super().set_test_params()

    # Wait for the next SKETCH message to be received by the
    # given peer. Clear and return it.
    def wait_for_sketch(self, peer):
        def received_sketch():
            return (len(peer.last_sketch) > 0)
        self.wait_until(received_sketch, timeout=2)

        return peer.last_sketch.pop()

    # Check that the node announced the exact sketch we expected (of the expected capacity
    # and over the expected transactions)
    def check_sketch(self, peer, skdata, expected_wtxids, local_set_size):
        expected_short_ids = [get_short_id(wtxid, peer.combined_salt)
                        for wtxid in expected_wtxids]

        if len(expected_wtxids) == 0:
            expected_capacity = 0
        else:
            expected_capacity = estimate_capacity(len(expected_wtxids), local_set_size)
        expected_sketch = create_sketch(expected_short_ids, expected_capacity)

        assert_equal(skdata, expected_sketch)

    # Send a RECONCILDIFF message from the given peer, including a sketch of
    # the given transactions.
    def send_reconcildiff_from(self, peer, success, wtxids_to_request, sync_with_ping=False):
        ask_shortids = [get_short_id(wtxid, peer.combined_salt)
                        for wtxid in wtxids_to_request]
        peer.send_reconcildiff(success, ask_shortids, sync_with_ping)

    def test_reconciliation_responder_flow_empty_sketch(self):
        self.log.info('Testing reconciliation flow sending an empty REQRXRCNCL')
        peer = self.test_node.add_p2p_connection(TxReconTestP2PConn())
        # Send a reconciliation request without creating any transactions
        peer.send_reqtxrcncl(0, int(RECON_Q * Q_PRECISION))

        # We need to make sure the node has trickled for inbounds. Waiting bumping for 20x the expected
        # time gives us a 1/1000000000 chances of failing
        self.test_node.bumpmocktime(INBOUND_INVENTORY_BROADCAST_INTERVAL * 20)
        peer.sync_with_ping()

        # Node sends us an empty sketch
        received_sketch = self.wait_for_sketch(peer)
        assert_equal(received_sketch.skdata, [])

        # It doesn't really matter what we send them here as our diff, given they have no
        # transaction for us, so nothing will match their local set and the node will simply terminate.
        self.send_reconcildiff_from(peer, True, [], sync_with_ping=True)

        # We can check this is the case by sending another reconciliation request, and check
        # how they reply to it (the node won't reply if the previous reconciliation was still pending)
        peer.send_reqtxrcncl(0, int(RECON_Q * Q_PRECISION))
        self.test_node.bumpmocktime(INBOUND_INVENTORY_BROADCAST_INTERVAL * 20)
        peer.sync_with_ping()
        received_sketch = self.wait_for_sketch(peer)

        # Clear peer
        peer.peer_disconnect()
        peer.wait_for_disconnect()

    def test_reconciliation_responder_protocol_violations(self):
        # Test disconnect on sending Erlay messages as a non-Erlay peer
        self.log.info('Testing protocol violation: erlay messages as non-erlay peer')
        peer = self.test_node.add_p2p_connection(P2PDataStore())
        peer.send_without_ping(msg_reqtxrcncl())
        peer.wait_for_disconnect()

        # Test disconnect on sending multiple REQTXRCNCL without receiving a response
        self.log.info('Testing protocol violation: sending multiple REQTXRCNCL without waiting for a response')
        peer = self.test_node.add_p2p_connection(TxReconTestP2PConn())
        peer.send_reqtxrcncl(0, int(RECON_Q * Q_PRECISION))
        peer.send_reqtxrcncl(0, int(RECON_Q * Q_PRECISION))
        peer.wait_for_disconnect()

        # Test disconnect on sending SKETCH as initiator
        self.log.info('Testing protocol violation: sending SKETCH as initiator')
        peer = self.test_node.add_p2p_connection(TxReconTestP2PConn())
        peer.send_sketch([])
        peer.wait_for_disconnect()

        # Test disconnect on sending a RECONCILDIFF out-of-order
        self.log.info('Testing protocol violation: sending RECONCILDIFF out of order')
        peer = self.test_node.add_p2p_connection(TxReconTestP2PConn())
        self.send_reconcildiff_from(peer, True, [])
        peer.wait_for_disconnect()

    def test_reconciliation_responder_flow_no_extension(self, n_mininode, n_node):
        self.log.info('Testing reconciliation flow without extensions')
        peer = self.test_node.add_p2p_connection(TxReconTestP2PConn())
        # Generate and submit transactions.
        mininode_unique_txs, node_unique_txs,  _ = self.generate_txs(self.wallet, n_mininode, n_node, 0)
        node_unique_wtxids = [tx.calc_sha256(True) for tx in node_unique_txs]

        # Send a reconciliation request. The request will be queued and replied on the next inbound trickle
        peer.send_reqtxrcncl(n_mininode, int(RECON_Q * Q_PRECISION))

        # We need to make sure the node has trickled for inbounds. Waiting bumping for 20x the expected
        # time gives us a 1/1000000000 chances of failing
        self.test_node.bumpmocktime(INBOUND_INVENTORY_BROADCAST_INTERVAL * 20)

        received_sketch = self.wait_for_sketch(peer)
        self.check_sketch(peer, received_sketch.skdata, node_unique_wtxids, n_mininode)

        # Diff should be all the node has that they don't have (their unique txs)
        self.send_reconcildiff_from(peer, True, node_unique_wtxids)

        self.wait_for_inv(peer, set(node_unique_wtxids))
        self.request_transactions_from(peer, node_unique_wtxids)
        self.wait_for_txs(peer, node_unique_wtxids)

        # Send our bit
        peer.send_txs_and_test(mininode_unique_txs, self.test_node)

        # Clear peer
        peer.peer_disconnect()
        peer.wait_for_disconnect()

    def run_test(self):
        self.test_node = self.nodes[0]
        self.test_node.setmocktime(int(time.time()))
        self.wallet = MiniWallet(self.nodes[0])
        self.generate(self.wallet, 512)

        # These node will consume some of the low-fanout announcements.
        self.outbound_peers = [self.test_node.add_p2p_connection(TxReconTestP2PConn()) for _ in range(4)]

        self.test_reconciliation_responder_flow_empty_sketch()
        self.test_reconciliation_responder_protocol_violations()
        self.test_reconciliation_responder_flow_no_extension(20, 15)

        # TODO: Add more cases, potentially including also extensions
        # if we end up not dropping them from the PR


if __name__ == '__main__':
    ReconciliationResponderTest(__file__).main()
