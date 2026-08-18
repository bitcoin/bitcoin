#!/usr/bin/env python3
# Copyright (c) 2021-2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test reconciliation-based transaction relay (node responds)"""

import time

from decimal import Decimal

from test_framework.messages import COIN, msg_feefilter, msg_reqtxrcncl
from test_framework.p2p import P2PDataStore
from test_framework.p2p_txrecon import (
    create_sketch, get_short_id, estimate_sketch_capacity, wire_q,
    ReconciliationTest, TxReconTestP2PConn, Q_PRECISION,
    INBOUND_INVENTORY_BROADCAST_INTERVAL
)
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet

class ReconciliationResponderTest(ReconciliationTest):
    def set_test_params(self):
        super().set_test_params()

    # Wait for the next SKETCH message to be received by the
    # given peer. Clear and return it.
    def wait_for_sketch(self, peer):
        def received_sketch():
            return (len(peer.last_sketch) > 0)
        self.wait_until(received_sketch, timeout=5)

        return peer.last_sketch.pop()

    # Check that the node announced the exact sketch we expected (of the expected capacity
    # and over the expected transactions)
    def check_sketch(self, peer, skdata, expected_wtxids, local_set_size):
        expected_short_ids = [get_short_id(wtxid, peer.combined_salt)
                        for wtxid in expected_wtxids]

        if len(expected_wtxids) == 0:
            expected_capacity = 0
        else:
            expected_capacity = estimate_sketch_capacity(len(expected_wtxids), local_set_size)
        expected_sketch = create_sketch(expected_short_ids, expected_capacity)

        assert_equal(skdata, expected_sketch)

    # Send a RECONCILDIFF message from the given peer, including a sketch of
    # the given transactions.
    def send_reconcildiff_from(self, peer, success, wtxids_to_request, sync_with_ping=False):
        ask_shortids = [get_short_id(wtxid, peer.combined_salt)
                        for wtxid in wtxids_to_request]
        peer.send_reconcildiff(success, ask_shortids, sync_with_ping)

    def test_reconciliation_responder_flow_empty_sketch(self):
        self.log.info('Testing reconciliation flow sending an empty REQTXRCNCL')
        peer = self.test_node.add_p2p_connection(TxReconTestP2PConn())

        # Give the node a transaction, so the empty sketch below is the result of us announcing an
        # empty set rather than of the node having nothing to offer.
        self.generate_txs(self.wallet, 0, 1, 0)
        self.bump_mocktime_past_trickle(INBOUND_INVENTORY_BROADCAST_INTERVAL)
        peer.sync_with_ping()

        # Send a reconciliation request announcing an empty set
        peer.send_reqtxrcncl(0, wire_q())

        # Node sends us an empty sketch
        received_sketch = self.wait_for_sketch(peer)
        assert_equal(received_sketch.skdata, [])

        # Having received no sketch we could not have decoded anything, so we terminate with a failure
        self.send_reconcildiff_from(peer, False, [], sync_with_ping=True)

        # We can check this is the case by sending another reconciliation request, and check
        # how they reply to it (the node won't reply if the previous reconciliation was still pending)
        peer.send_reqtxrcncl(0, wire_q())
        self.bump_mocktime_past_trickle(INBOUND_INVENTORY_BROADCAST_INTERVAL)
        peer.sync_with_ping()
        received_sketch = self.wait_for_sketch(peer)

        peer.peer_disconnect()
        peer.wait_for_disconnect()

    def test_reconciliation_responder_flow_interleaved_txs(self):
        self.log.info('Testing reconciliation flow sending interleaved transactions')

        # A second, non-Erlay inbound, used further down to hand the node a transaction
        # from somewhere other than the peer we are reconciling with.
        tx_submitter = self.test_node.add_p2p_connection(P2PDataStore())
        peer = self.test_node.add_p2p_connection(TxReconTestP2PConn())

        # Most of both sets is shared, so the difference is small enough for a sketch of the
        # estimated capacity to decode, as it would be between two real peers.
        n_mininode, n_node, n_shared = 3, 3, 20
        mininode_unique_txs, node_unique_txs, shared_txs = self.generate_txs(self.wallet, n_mininode, n_node, n_shared)
        node_set_wtxids = [tx.wtxid_int for tx in node_unique_txs + shared_txs]
        node_unique_wtxids = [tx.wtxid_int for tx in node_unique_txs]
        mininode_set_size = n_mininode + n_shared

        # Bump well past the trickle interval so the peer has all their
        # transactions in our reconciliation set before requesting reconciliation
        self.bump_mocktime_past_trickle(INBOUND_INVENTORY_BROADCAST_INTERVAL)
        tx_submitter.sync_with_ping()
        peer.sync_with_ping()

        peer.send_reqtxrcncl(mininode_set_size, wire_q())

        # Receive their sketch, and check it matches our expectations
        received_sketch = self.wait_for_sketch(peer)
        self.check_sketch(peer, received_sketch.skdata, node_set_wtxids, mininode_set_size)

        # Send one of out transactions to the node via another node. We want them to store
        # data on our set once the Sketch has already been sent, to make sure data is not lost
        # when the reconciliation is finished
        target_wtxid = mininode_unique_txs[0].wtxid_int
        tx_submitter.send_txs_and_test(mininode_unique_txs[:1], self.test_node, success=True)

        # Request the transactions the decoded difference reveals as only theirs
        self.send_reconcildiff_from(peer, True, node_unique_wtxids)
        self.wait_for_inv(peer, node_unique_wtxids)
        self.request_transactions_from(peer, node_unique_wtxids)
        self.wait_for_txs(peer, node_unique_wtxids)

        # Send our bit
        peer.send_txs_and_test(mininode_unique_txs[1:], self.test_node)

        # Ask to reconcile again, see if they offer the missing transaction.
        # Bump past the trickle interval first so the target tx is added to
        # the peer's set before requesting reconciliation
        self.bump_mocktime_past_trickle(INBOUND_INVENTORY_BROADCAST_INTERVAL)
        peer.sync_with_ping()
        peer.send_reqtxrcncl(mininode_set_size, wire_q())
        received_sketch = self.wait_for_sketch(peer)
        self.check_sketch(peer, received_sketch.skdata, [target_wtxid], mininode_set_size)

        # Ask for the missing transaction and check that it is sent
        self.send_reconcildiff_from(peer, True, [target_wtxid])
        self.wait_for_inv(peer, [target_wtxid])
        self.request_transactions_from(peer, [target_wtxid])
        self.wait_for_txs(peer, [target_wtxid])

        # Clear peers
        tx_submitter.peer_disconnect()
        tx_submitter.wait_for_disconnect()
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
        peer.send_reqtxrcncl(0, wire_q())
        peer.send_reqtxrcncl(0, wire_q())
        peer.wait_for_disconnect()

        # Test disconnect on sending a REQTXRCNCL with a q larger than the wire format allows
        self.log.info('Testing protocol violation: sending REQTXRCNCL with an out-of-range q')
        peer = self.test_node.add_p2p_connection(TxReconTestP2PConn())
        with self.test_node.assert_debug_log(['Peer is requesting reconciliation with an out-of-range q']):
            peer.send_reqtxrcncl(0, Q_PRECISION + 1)
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

        # Test disconnect on claiming a successful decode of a sketch that was never sent
        self.log.info('Testing protocol violation: claiming success after an empty sketch')
        peer = self.test_node.add_p2p_connection(TxReconTestP2PConn())
        peer.send_reqtxrcncl(0, wire_q())
        assert_equal(self.wait_for_sketch(peer).skdata, [])
        with self.test_node.assert_debug_log(['claiming a successful reconciliation after we sent an empty sketch']):
            self.send_reconcildiff_from(peer, True, [])
            peer.wait_for_disconnect()

    def test_reconciliation_responder_flow_no_extension(self, n_mininode, n_node, n_shared):
        self.log.info('Testing reconciliation flow without extensions')
        peer = self.test_node.add_p2p_connection(TxReconTestP2PConn())

        # Generate and submit transactions. Sharing most of them keeps the difference decodable.
        mininode_unique_txs, node_unique_txs, shared_txs = self.generate_txs(self.wallet, n_mininode, n_node, n_shared)
        node_set_wtxids = [tx.wtxid_int for tx in node_unique_txs + shared_txs]
        node_unique_wtxids = [tx.wtxid_int for tx in node_unique_txs]
        mininode_set_size = n_mininode + n_shared

        # Bump well past the trickle interval so the peer has all their
        # transactions in our reconciliation set before requesting reconciliation
        self.bump_mocktime_past_trickle(INBOUND_INVENTORY_BROADCAST_INTERVAL)
        peer.sync_with_ping()

        peer.send_reqtxrcncl(mininode_set_size, wire_q())
        received_sketch = self.wait_for_sketch(peer)
        self.check_sketch(peer, received_sketch.skdata, node_set_wtxids, mininode_set_size)

        # Diff should be all the node has that they don't have (their unique txs)
        self.send_reconcildiff_from(peer, True, node_unique_wtxids)

        self.wait_for_inv(peer, set(node_unique_wtxids))
        self.request_transactions_from(peer, node_unique_wtxids)
        self.wait_for_txs(peer, node_unique_wtxids)

        # Send our bit
        peer.send_txs_and_test(mininode_unique_txs, self.test_node)

        peer.peer_disconnect()
        peer.wait_for_disconnect()

    def test_reconciliation_responder_mempool_removal(self):
        self.log.info('Testing that transactions leaving the mempool leave the reconciliation set')
        # Two peers receive the same transactions. The first acts normally. The second never
        # completes a round, so nothing but pruning can empty its set.
        peer0 = self.test_node.add_p2p_connection(TxReconTestP2PConn())
        peer1 = self.test_node.add_p2p_connection(TxReconTestP2PConn())

        _, node_txs, _ = self.generate_txs(self.wallet, 0, 5, 0)
        node_wtxids = [tx.wtxid_int for tx in node_txs]
        self.bump_mocktime_past_trickle(INBOUND_INVENTORY_BROADCAST_INTERVAL)
        peer0.sync_with_ping()
        peer1.sync_with_ping()

        # The transactions are in both sets
        peer0.send_reqtxrcncl(len(node_txs), wire_q())
        self.check_sketch(peer0, self.wait_for_sketch(peer0).skdata, node_wtxids, len(node_txs))

        # Mining them takes them out of the mempool, so they must leave the sets too
        self.generate(self.wallet, 1)
        peer1.sync_with_ping()

        # A non-zero claimed set size, so an empty sketch can only mean our own set is empty
        peer1.send_reqtxrcncl(len(node_txs), wire_q())
        assert_equal(self.wait_for_sketch(peer1).skdata, [])

        self.send_reconcildiff_from(peer1, False, [], sync_with_ping=True)
        for peer in (peer0, peer1):
            peer.peer_disconnect()
            peer.wait_for_disconnect()

    def test_reconciliation_responder_feefilter(self):
        self.log.info('Testing that the fee filter is applied to post-reconciliation announcements')
        peer = self.test_node.add_p2p_connection(TxReconTestP2PConn())

        # Create three transactions, one below the filter, one at the filter, and one above the filter.
        # Make sure we are above the minimum relay fee.
        vsize = 200
        filter_fee = 5 * vsize
        below = self.wallet.create_self_transfer(target_vsize=vsize, fee=Decimal(3 * vsize) / COIN)["tx"]
        at_limit = self.wallet.create_self_transfer(target_vsize=vsize, fee=Decimal(filter_fee) / COIN)["tx"]
        above = self.wallet.create_self_transfer(target_vsize=vsize, fee=Decimal(10 * vsize) / COIN)["tx"]

        # A low fee parent with a high fee child. Topology puts the parent first, so a filtered
        # out transaction must not stop the ones behind it.
        parent_utxo = self.wallet.create_self_transfer(target_vsize=vsize, fee=Decimal(3 * vsize) / COIN)
        parent = parent_utxo["tx"]
        child = self.wallet.create_self_transfer(utxo_to_spend=parent_utxo["new_utxo"], target_vsize=vsize,
                                                fee=Decimal(20 * vsize) / COIN)["tx"]

        all_txs = [below, at_limit, above, parent, child]
        tx_submitter = self.test_node.add_p2p_connection(P2PDataStore())
        tx_submitter.send_txs_and_test(all_txs, self.test_node, success=True)
        tx_submitter.peer_disconnect()
        # All of them must be in the mempool, or the ones under the filter are never tested
        mempool = set(self.test_node.getrawmempool())
        assert all(tx.txid_hex in mempool for tx in all_txs)

        # Let the transactions reach the peer's reconciliation set before announcing the filter,
        # so they are filtered when reconciliation announces them rather than on the way in.
        self.bump_mocktime_past_trickle(INBOUND_INVENTORY_BROADCAST_INTERVAL)
        peer.sync_with_ping()
        peer.send_without_ping(msg_feefilter(feerate=5000))
        peer.sync_with_ping()

        all_wtxids = [tx.wtxid_int for tx in all_txs]
        peer.send_reqtxrcncl(1, wire_q())
        peer.sync_with_ping()
        received_sketch = self.wait_for_sketch(peer)
        self.check_sketch(peer, received_sketch.skdata, all_wtxids, 1)

        # Ask for everything they hold. Only the transactions paying at least the filter rate
        # should come back.
        self.send_reconcildiff_from(peer, True, all_wtxids)
        self.wait_for_inv(peer, [at_limit.wtxid_int, above.wtxid_int, child.wtxid_int])

        peer.peer_disconnect()
        peer.wait_for_disconnect()

    def run_test(self):
        self.test_node = self.nodes[0]
        self.test_node.setmocktime(int(time.time()))
        self.wallet = MiniWallet(self.nodes[0])
        self.generate(self.wallet, 512)

        self.test_reconciliation_responder_flow_empty_sketch()
        self.test_reconciliation_responder_flow_interleaved_txs()
        self.test_reconciliation_responder_protocol_violations()
        self.test_reconciliation_responder_flow_no_extension(3, 3, 20)
        self.test_reconciliation_responder_mempool_removal()
        self.test_reconciliation_responder_feefilter()

        # TODO: Add more cases, potentially including also extensions
        # if we end up not dropping them from the PR


if __name__ == '__main__':
    ReconciliationResponderTest(__file__).main()
