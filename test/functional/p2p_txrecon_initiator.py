#!/usr/bin/env python3
# Copyright (c) 2021-2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test reconciliation-based transaction relay (node initiates)"""

import time
import math

from test_framework.messages import msg_reconcildiff

from test_framework.wallet import MiniWallet
from test_framework.util import assert_equal
from test_framework.p2p import P2PDataStore
from test_framework.p2p_txrecon import (
    create_sketch, get_short_id, ReconciliationTest,
    TxReconTestP2PConn, Q_PRECISION, RECON_Q,
    MAX_SKETCH_CAPACITY, BYTES_PER_SKETCH_CAPACITY,
    OUTBOUND_INVENTORY_BROADCAST_INTERVAL
)

# Taken from txreconciliation_impl.h
RECON_REQUEST_INTERVAL = 30


class ReconciliationInitiatorTest(ReconciliationTest):
    def set_test_params(self):
        super().set_test_params()

    # Wait for the next REQTXRCNCL message to be received by the
    # given peer. Clear and return it.
    def wait_for_reqtxrcncl(self, peer):
        def received_reqtxrcncl():
            return (len(peer.last_reqtxrcncl) > 0)
        self.wait_until(received_reqtxrcncl)

        return peer.last_reqtxrcncl.pop()

    # Wait for the next RECONCILDIFF message to be received by the
    # given peer. Clear and return it.
    def wait_for_reconcildiff(self, peer):
        def received_reconcildiff():
            return (len(peer.last_reconcildiff) > 0)
        self.wait_until(received_reconcildiff)

        return peer.last_reconcildiff.pop()

    # Creates a Sketch using the provided transactions and capacity
    # and sends it from the given peer.
    # Returns a list of the short ids contained in the Sketch.
    def send_sketch_from(self, peer, unique_wtxids, shared_wtxids, capacity):
        unique_short_txids = [get_short_id(wtxid, peer.combined_salt)
                       for wtxid in unique_wtxids]
        shared_short_txids = [get_short_id(wtxid, peer.combined_salt)
                       for wtxid in shared_wtxids]

        sketch = create_sketch(unique_short_txids + shared_short_txids, capacity)
        peer.send_sketch(sketch)

        return unique_short_txids

    def test_reconciliation_initiator_flow_empty_sketch(self):
        peer = self.test_node.add_outbound_p2p_connection(TxReconTestP2PConn(), p2p_idx=0, connection_type="outbound-full-recon")

        # Generate transaction only on the node's end, so it has something to announce at the end
        _, node_unique_txs, _ = self.generate_txs(self.wallet, 0, 10, 0)

        # Do the reconciliation dance announcing an empty sketch.
        # Wait enough to make sure the node adds the transaction to our tracker
        # and sends us a reconciliation request.
        self.log.info('Testing reconciliation flow sending an empty sketch')
        self.bump_mocktime_past_trickle(OUTBOUND_INVENTORY_BROADCAST_INTERVAL)
        peer.sync_with_ping()
        # We have bumped the time well past RECON_REQUEST_INTERVAL, so the peer should send us a recon request
        self.wait_for_reqtxrcncl(peer)
        # Sketch is empty
        self.send_sketch_from(peer, [], [], 0)
        recon_diff = self.wait_for_reconcildiff(peer)
        # The node's reply is also empty, signaling early exit
        assert_equal(recon_diff.ask_shortids, [])

        # The node simply defaults to announce all the transaction it had for us
        node_unique_wtxids = set([tx.wtxid_int for tx in node_unique_txs])
        self.wait_for_inv(peer, node_unique_wtxids)
        self.request_transactions_from(peer, node_unique_wtxids)
        self.wait_for_txs(peer, node_unique_wtxids)

        # Clear peer
        peer.peer_disconnect()
        peer.wait_for_disconnect()

    def test_reconciliation_initiator_flow_interleaved_txs(self):
        peers = [self.test_node.add_outbound_p2p_connection(TxReconTestP2PConn(), p2p_idx=i, connection_type="outbound-full-recon") for i in range(3)]

        # Generate shared transactions only.
        _, _, shared_txs = self.generate_txs(self.wallet, 0, 0, 2)

        # The rest will be sent in between the REQTXRCNCL and the SKETCH to
        # see how node handle transactions received during reconciliation.
        self.log.info('Testing reconciliation flow sending interleaved transactions')

        # Make sure the node has trickled for all peers
        self.bump_mocktime_past_trickle(OUTBOUND_INVENTORY_BROADCAST_INTERVAL)
        for peer in peers:
            peer.sync_with_ping()

        # Reconciliation requests are scheduled every RECON_REQUEST_INTERVAL / len(peers).
        # Make sure all peers have had time to send us a request
        for peer in peers:
            self.test_node.bumpmocktime(math.ceil(RECON_REQUEST_INTERVAL/len(peers)))
            peer.sync_with_ping()
            node_set_size = self.wait_for_reqtxrcncl(peer).set_size
            # All peers are reconciliation peers, so peer should be trying to reconcile shared_txs with us
            assert_equal(node_set_size, len(shared_txs))

        # Interleave transactions
        # - peer0 will receive all transactions before reconciling, so he will remove all of them from our set
        # - peer1 will receive one transaction, so he should have one transaction to reconcile with us
        # - peer2 will receive no transactions
        shared_wtxids = [tx.wtxid_int for tx in shared_txs]
        peers[0].send_txs_and_test(shared_txs, self.test_node)
        peers[0].shared_wtxids = []
        peers[1].send_txs_and_test(shared_txs[:1], self.test_node)
        peers[1].shared_wtxids = shared_wtxids[1:]
        # Make our set for peer2 have a single tx so they can send the other
        peers[2].shared_wtxids = shared_wtxids[1:]

        # Send the sketches and expect, for all peers, empty ask reconcildiff:
        # - peer0 has local set emptied by receiving both txs: early exit, so no diff
        # - peer1 only has one one tx in the set, which matches out sketch som no diff
        # - peer2: has both txs still in set, we send a sketch with one of them, so no diff
        #   but they will offer us the other transaction
        for peer in peers:
            shared_short_txids = self.send_sketch_from(peer, peer.shared_wtxids, [], len(shared_txs))
            expected_diff = msg_reconcildiff()
            expected_diff.success = bool(shared_short_txids)
            expected_diff.ask_shortids = []

            received_recon_diff = self.wait_for_reconcildiff(peer)
            assert_equal(expected_diff, received_recon_diff)

        # Only the last peer should have received a transaction announcement
        assert(not peers[0].last_inv)
        assert(not peers[1].last_inv)
        self.wait_for_inv(peers[2], shared_wtxids[:1])

        # Clear peers
        self.test_node.disconnect_p2ps()

    def test_reconciliation_initiator_protocol_violations(self):
        # Test disconnect on sending Erlay messages as a non-Erlay peer
        self.log.info('Testing protocol violation: erlay messages as non-erlay peer')
        peer = self.test_node.add_outbound_p2p_connection(P2PDataStore(), p2p_idx=0)
        peer.send_without_ping(msg_reconcildiff())
        peer.wait_for_disconnect()

        # Test disconnect on sending a REQTXRCNCL as a responder
        self.log.info('Testing protocol violation: sending REQTXRCNCL as a responder')
        peer = self.test_node.add_outbound_p2p_connection(TxReconTestP2PConn(), p2p_idx=0, connection_type="outbound-full-recon")
        peer.send_reqtxrcncl(0, int(RECON_Q * Q_PRECISION))
        peer.wait_for_disconnect()

        # Test disconnect on sending a SKETCH out of order
        self.log.info('Testing protocol violation: sending SKETCH out of order')
        peer = self.test_node.add_outbound_p2p_connection(TxReconTestP2PConn(), p2p_idx=0, connection_type="outbound-full-recon")
        peer.send_sketch([])
        peer.wait_for_disconnect()

        # Test disconnect on sending a RECONCILDIFF as a responder
        self.log.info('Testing protocol violation: sending RECONCILDIFF as a responder')
        peer = self.test_node.add_outbound_p2p_connection(TxReconTestP2PConn(), p2p_idx=0, connection_type="outbound-full-recon")
        peer.send_reconcildiff(True, [])
        peer.wait_for_disconnect()

        # Test disconnect on SKETCH that exceeds maximum capacity
        self.log.info('Testing protocol violation: sending SKETCH exceeding the maximum capacity')
        peer = self.test_node.add_outbound_p2p_connection(TxReconTestP2PConn(), p2p_idx=0, connection_type="outbound-full-recon")
        # Do the reconciliation dance until announcing the SKETCH
        self.test_node.bumpmocktime(RECON_REQUEST_INTERVAL)
        peer.sync_with_ping()
        self.wait_for_reqtxrcncl(peer)
        # Send an over-sized sketch (over the maximum allowed capacity)
        peer.send_sketch([0] * ((MAX_SKETCH_CAPACITY + 1) * BYTES_PER_SKETCH_CAPACITY))
        peer.wait_for_disconnect()

    def test_reconciliation_initiator_no_extension(self, n_node, n_mininode, n_shared):
        self.log.info('Testing reconciliation flow without extensions')
        # Use 4 peers (MAX_OUTBOUND_FULL_RECON_CONNECTIONS). All are reconciliation
        peers = [self.test_node.add_outbound_p2p_connection(TxReconTestP2PConn(), p2p_idx=i, connection_type="outbound-full-recon") for i in range(4)]

        # Generate and submit transactions.
        mininode_unique_txs, node_unique_txs, shared_txs = self.generate_txs(
            self.wallet, n_mininode, n_node, n_shared)

        # This bump also covers RECON_REQUEST_INTERVAL (30s), so the first peer gets its REQTXRCNCL here.
        self.bump_mocktime_past_trickle(OUTBOUND_INVENTORY_BROADCAST_INTERVAL)
        for peer in peers:
            peer.sync_with_ping()

        # Reconciliation requests are scheduled every RECON_REQUEST_INTERVAL / len(peers).
        # Make sure all peers have had time to send us a request
        for peer in peers:
            self.test_node.bumpmocktime(math.ceil(RECON_REQUEST_INTERVAL/len(peers)))
            peer.sync_with_ping()
            node_set_size = self.wait_for_reqtxrcncl(peer).set_size
            # All peers are reconciliation peers, so peer should be trying to reconcile shared_txs with us
            assert_equal(node_set_size, n_node + n_shared)

        # Send the corresponding sketches
        for peer in peers:
            unique_wtxids = [tx.wtxid_int for tx in mininode_unique_txs]
            shared_wtxids = [tx.wtxid_int for tx in shared_txs]
            peer.unique_short_txids = self.send_sketch_from(peer, unique_wtxids, shared_wtxids, n_node + n_mininode)

        # Check that we received the expected sketch difference, based on the sketch we have sent.
        # Decoding succeeds and the node asks for all of our unique transactions.
        for peer in peers:
            recon_diff = self.wait_for_reconcildiff(peer)
            expected_diff = msg_reconcildiff()
            expected_diff.success = 1
            expected_diff.ask_shortids = peer.unique_short_txids
            assert_equal(recon_diff, expected_diff)

        # The node announces the transactions we are missing (node_unique + shared).
        node_unique_wtxids = [tx.wtxid_int for tx in node_unique_txs]
        shared_wtxids = [tx.wtxid_int for tx in shared_txs]
        for peer in peers:
            expected_wtxids = set(node_unique_wtxids + shared_wtxids)
            self.wait_for_inv(peer, expected_wtxids)
            self.request_transactions_from(peer, expected_wtxids)
            self.wait_for_txs(peer, expected_wtxids)

            # The reconciliation flow has really finished already, but we should be well behaved
            # and send the node the transactions it asked for in the diff.
            peer.send_txs_and_test(mininode_unique_txs, self.test_node)

    def run_test(self):
        self.test_node = self.nodes[0]
        self.test_node.setmocktime(int(time.time()))
        self.wallet = MiniWallet(self.nodes[0])
        self.generate(self.wallet, 512)

        self.test_reconciliation_initiator_flow_empty_sketch()
        self.test_reconciliation_initiator_flow_interleaved_txs()
        self.test_reconciliation_initiator_protocol_violations()
        self.test_reconciliation_initiator_no_extension(20, 15, 0)

        # TODO: Add more cases, potentially including also extensions
        # if we end up not dropping them from the PR


if __name__ == '__main__':
    ReconciliationInitiatorTest(__file__).main()
