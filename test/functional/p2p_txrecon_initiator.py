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
    MAX_SKETCH_CAPACITY, BYTES_PER_SKETCH_CAPACITY
)

# Taken from net_processing.cpp
OUTBOUND_INVENTORY_BROADCAST_INTERVAL = 2

# Taken from txreconciliation.h
OUTBOUND_FANOUT_THRESHOLD = 4
RECON_REQUEST_INTERVAL = 8


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
        peer = self.test_node.add_outbound_p2p_connection(TxReconTestP2PConn(), p2p_idx=0)

        # Generate transaction only on the node's end, so it has something to announce at the end
        _, node_unique_txs, _ = self.generate_txs(self.wallet, 0, 10, 0)

        # Do the reconciliation dance announcing an empty sketch
        # Wait enough to make sure the node adds the transaction to our tracker
        # And sends us a reconciliation request
        self.log.info('Testing reconciliation flow sending an empty sketch')
        self.test_node.bumpmocktime(OUTBOUND_INVENTORY_BROADCAST_INTERVAL * 20)
        peer.sync_with_ping()
        self.test_node.bumpmocktime(RECON_REQUEST_INTERVAL)
        peer.sync_with_ping()
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
        peers = [self.test_node.add_outbound_p2p_connection(TxReconTestP2PConn(), p2p_idx=i) for i in range(8)]

        # Generate shared transaction transactions only.
        _, _, shared_txs = self.generate_txs(self.wallet, 0, 0, 2)

        # Do the reconciliation dance announcing only a single transaction.
        # The other one will be sent in between the Sketch and the diff to see
        # how the node behaves.
        self.log.info('Testing reconciliation flow sending interleaved transactions')

        # Make sure the node has trickle for all peers
        self.test_node.bumpmocktime(OUTBOUND_INVENTORY_BROADCAST_INTERVAL * 20)
        for peer in peers:
            peer.sync_with_ping()

        # Tick for as many peers as test_node has, so all of them receive a reconciliation request
        for peer in peers:
            self.test_node.bumpmocktime(math.ceil(RECON_REQUEST_INTERVAL/len(peers)))
            peer.sync_with_ping()

        empty_recon_requests = 0
        recon_peers = []
        for peer in peers:
            # The node should be trying to reconcile with a single peer only.
            node_set_size = self.wait_for_reqtxrcncl(peer).set_size
            if (node_set_size == 0):
                empty_recon_requests+=1
                peer.chosen_for_fanout = True
            else:
                assert_equal(node_set_size, len(shared_txs))
                recon_peers.append(peer)

        assert_equal(empty_recon_requests, OUTBOUND_FANOUT_THRESHOLD + 1)

        # From the 3 reconciliation peers do the following:
        # - Pick one to send BOTH transactions via fanout before reconciling
        # - Pick another to announce ONE transaction before reconciling
        # - Leave one as is
        shared_wtxids = [tx.wtxid_int for tx in shared_txs]
        recon_peers[0].send_txs_and_test(shared_txs, self.test_node)
        recon_peers[0].shared_wtxids = []
        recon_peers[1].send_txs_and_test(shared_txs[:1], self.test_node)
        recon_peers[1].shared_wtxids = shared_wtxids[1:]
        recon_peers[2].shared_wtxids = shared_wtxids[1:]

        # Send the sketches and expect, for all peers, empty diffs.
        # - For the first peer, the reconciliation shortcuts, since we sent an empty sketch
        # - For the second peer, the reconciliation succeeds with no differences
        # - For the last peer, the reconciliation succeeds with no differences, and we get one announcement
        for recon_peer in recon_peers:
            shared_short_txids = self.send_sketch_from(recon_peer, recon_peer.shared_wtxids, [], len(shared_txs))
            expected_diff = msg_reconcildiff()
            expected_diff.success = bool(shared_short_txids)
            expected_diff.ask_shortids = []

            received_recon_diff = self.wait_for_reconcildiff(recon_peer)
            assert_equal(expected_diff, received_recon_diff)

        # Only the last recon peer should have received a transaction announcement
        assert(not recon_peers[0].last_inv)
        assert(not recon_peers[1].last_inv)
        self.wait_for_inv(recon_peers[2], shared_wtxids[:1])

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
        peer = self.test_node.add_outbound_p2p_connection(TxReconTestP2PConn(), p2p_idx=0)
        peer.send_reqtxrcncl(0, int(RECON_Q * Q_PRECISION))
        peer.wait_for_disconnect()

        # Test disconnect on sending a SKETCH out of order
        self.log.info('Testing protocol violation: sending SKETCH out of order')
        peer = self.test_node.add_outbound_p2p_connection(TxReconTestP2PConn(), p2p_idx=0)
        peer.send_sketch([])
        peer.wait_for_disconnect()

        # Test disconnect on sending a RECONCILDIFF as a responder
        self.log.info('Testing protocol violation: sending RECONCILDIFF as a responder')
        peer = self.test_node.add_outbound_p2p_connection(TxReconTestP2PConn(), p2p_idx=0)
        peer.send_reconcildiff(True, [])
        peer.wait_for_disconnect()

        # Test disconnect on SKETCH that exceeds maximum capacity
        self.log.info('Testing protocol violation: sending SKETCH exceeding the maximum capacity')
        peer = self.test_node.add_outbound_p2p_connection(TxReconTestP2PConn(), p2p_idx=0)
        # Do the reconciliation dance until announcing the SKETCH
        self.test_node.bumpmocktime(RECON_REQUEST_INTERVAL)
        peer.sync_with_ping()
        self.wait_for_reqtxrcncl(peer)
        # Send an over-sized sketch (over the maximum allowed capacity)
        peer.send_sketch([0] * ((MAX_SKETCH_CAPACITY + 1) * BYTES_PER_SKETCH_CAPACITY))
        peer.wait_for_disconnect()

    def test_reconciliation_initiator_no_extension(self, n_node, n_mininode, n_shared):
        self.log.info('Testing reconciliation flow without extensions')
        peers = [self.test_node.add_outbound_p2p_connection(TxReconTestP2PConn(), p2p_idx=i) for i in range(8)]

        # Generate and submit transactions.
        mininode_unique_txs, node_unique_txs, shared_txs = self.generate_txs(
            self.wallet, n_mininode, n_node, n_shared)

        # For transactions to have been added to the reconciliation sets of the node's outbound peers, we need no make sure
        # that the Poisson timer for all of them has ticked. Each timer ticks every 2 seconds on average. However, given the
        # nature of the test we have no way of checking if the timer has ticked, but we can work around this by making sure we
        # have waited long enough. By bumping the time ~20 times the expected value, we have a 1/100000000 chances of any of the
        # timers not ticking (i.e. failing the test later on), which should be more than acceptable
        self.test_node.bumpmocktime(OUTBOUND_INVENTORY_BROADCAST_INTERVAL * 20)
        for peer in peers:
            peer.sync_with_ping()

        # Tick for as many peers as test_node has, so all of them receive a reconciliation request
        for peer in peers:
            self.test_node.bumpmocktime(int(RECON_REQUEST_INTERVAL/len(peers)))
            peer.sync_with_ping()

        empty_recon_requests = 0
        for peer in peers:
            # Check we have received a reconciliation request. The request contains either no
            # elements (the node has been picked for fanout) or as many elements as transactions
            # where created by the node (n_node + n_shared)
            node_set_size = self.wait_for_reqtxrcncl(peer).set_size
            if (node_set_size == 0):
                empty_recon_requests+=1
                peer.chosen_for_fanout = True
            else:
                assert_equal(node_set_size, n_node + n_shared)
                peer.chosen_for_fanout = False

        # For outbound peers, if the transaction was created by the node, or receive via fanout,
        # it will be fanout to up to OUTBOUND_FANOUT_THRESHOLD. We will be reconciling with the rest
        assert_equal(empty_recon_requests, OUTBOUND_FANOUT_THRESHOLD + 1)

        for peer in peers:
            # If we received an empty request we can simply respond with an empty sketch
            # the node will shortcircuit and send us all transactions via fanout
            capacity = 0 if peer.chosen_for_fanout else  n_node + n_mininode
            unique_wtxids = [tx.wtxid_int for tx in mininode_unique_txs]
            shared_wtxids = [tx.wtxid_int for tx in shared_txs]
            peer.unique_short_txids = self.send_sketch_from(peer, unique_wtxids, shared_wtxids, capacity)

        # Check that we received the expected sketch difference, based on the sketch we have sent
        for peer in peers:
            recon_diff = self.wait_for_reconcildiff(peer)
            expected_diff = msg_reconcildiff()
            if peer.chosen_for_fanout:
                # If we replied with an empty sketch, they will flag failure and reply with an
                # empty diff to signal an early exit and default to fanout
                assert_equal(recon_diff, expected_diff)
            else:
                # Otherwise, we expect the decoding to succeed and a request of all out transactions
                # (given there were no shared transaction)
                expected_diff.success = 1
                expected_diff.ask_shortids = peer.unique_short_txids
                assert_equal(recon_diff, expected_diff)

        # If we were chosen for reconciliation, the node will announce only the transaction we are missing (node_unique)
        # Otherwise, it will announce all the ones it has (node_unique + shared)
        node_unique_wtxids = [tx.wtxid_int for tx in node_unique_txs]
        shared_wtxids = [tx.wtxid_int for tx in shared_txs]
        for peer in peers:
            expected_wtxids = set(node_unique_wtxids + shared_wtxids) if peer.chosen_for_fanout else set(node_unique_wtxids)
            self.wait_for_inv(peer, expected_wtxids)
            self.request_transactions_from(peer, expected_wtxids)
            self.wait_for_txs(peer, expected_wtxids)

            if not peer.chosen_for_fanout:
                # If we received a populated diff, the node will be expecting
                # some transactions in return. The reconciliation flow has really
                # finished already, but we should be well behaved
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
