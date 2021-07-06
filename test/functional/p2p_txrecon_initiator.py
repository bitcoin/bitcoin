#!/usr/bin/env python3
# Copyright (c) 2021-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test reconciliation-based transaction relay (node initiates)"""

import time

from test_framework.messages import (
    msg_getdata, msg_sketch, MSG_WTX, CInv,
)

from test_framework.util import assert_equal

from test_framework.p2p_txrecon import (
    BYTES_PER_SKETCH_CAPACITY, create_sketch, get_short_id,
    ReconciliationTest, TxReconTestP2PConn
)

# Taken from net_processing.cpp
INVENTORY_BROADCAST_INTERVAL = 1
RECON_REQUEST_INTERVAL = 1

EXTRA_FANOUT_CANDIDATES = 4


class TestTxReconResponderP2PConn(TxReconTestP2PConn):
    # This object simulates a reconciliation responder, which will be queried by
    # the Bitcoin Core node being tested.
    def __init__(self):
        super().__init__(False)
        self.last_sendrecon = []
        self.last_tx = []
        self.last_reqreconcil = []
        self.last_reconcildiff = []
        self.last_reqsketchext = []

    def on_tx(self, message):
        self.last_tx.append(message.tx.calc_sha256(True))

    def on_reqrecon(self, message):
        self.last_reqreconcil.append(message)

    def on_reqsketchext(self, message):
        self.last_reqsketchext.append(message)

    def on_reconcildiff(self, message):
        self.last_reconcildiff.append(message)

    def send_sketch(self, skdata):
        msg = msg_sketch()
        msg.skdata = skdata
        self.send_message(msg)

    def send_getdata(self, ask_wtxids):
        msg = msg_getdata(inv=[CInv(MSG_WTX, h=wtxid) for wtxid in ask_wtxids])
        self.send_message(msg)


class ReconciliationInitiatorTest(ReconciliationTest):

    def set_test_params(self):
        super().set_test_params()

    def make_or_reset_reconciliation_conn(self):
        self.test_node = self.nodes[0].add_p2p_connection(
            TestTxReconResponderP2PConn(), inbound=True)
        self.test_node.wait_for_verack()

    # Returns False if we received an empty sketch instead of the expected non-empty sketch, likely
    # because the transactions were added to the set after the reconciliation initiation.
    def receive_reqreconcil(self, expected_set_size):
        for _ in range(EXTRA_FANOUT_CANDIDATES + 1):
            time.sleep(0.1)  # give time to issue other recon requests
            self.proceed_in_time(RECON_REQUEST_INTERVAL + 1)

        def received_reqreconcil():
            return (len(self.test_node.last_reqreconcil) >= 1)
        self.wait_until(received_reqreconcil, timeout=30)

        # Some of the transactions are set for low-fanout to the mininode, so they won't be
        # in the set. The caller should check that they are indeed announced later.
        node_set_size = self.test_node.last_reqreconcil[-1].set_size
        assert(node_set_size <= expected_set_size)
        self.test_node.last_reqreconcil = []
        return node_set_size

    def transmit_sketch(self, txs_to_sketch, extension, capacity):
        short_txids = [get_short_id(tx, self.compute_salt())
                       for tx in txs_to_sketch]
        if extension:
            sketch = create_sketch(
                short_txids, capacity * 2)[int(capacity * BYTES_PER_SKETCH_CAPACITY):]
        else:
            self.test_node.last_full_mininode_size = len(txs_to_sketch)
            sketch = create_sketch(short_txids, capacity)
        self.test_node.send_sketch(sketch)

    def handle_extension_request(self):
        def received_reqsketchext():
            return (len(self.test_node.last_reqsketchext) == 1)
        self.wait_until(received_reqsketchext, timeout=30)
        self.test_node.last_reqsketchext = []

    def handle_reconciliation_finalization(self, expected_success,
                                           expected_requested_txs,
                                           might_be_requested_txs):
        expected_requested_shortids = [get_short_id(
            tx, self.compute_salt()) for tx in expected_requested_txs]

        def received_reconcildiff():
            return (len(self.test_node.last_reconcildiff) == 1)
        self.wait_until(received_reconcildiff, timeout=30)
        success = self.test_node.last_reconcildiff[0].success
        assert_equal(success, int(expected_success))
        # They could ask for more, if they didn't add some transactions to the reconciliation set,
        # but set it to low-fanout to us instead.
        assert(set(expected_requested_shortids).
               issubset(set(self.test_node.last_reconcildiff[0].ask_shortids)))
        extra_requested_shortids = set(
            self.test_node.last_reconcildiff[0].ask_shortids) - set(expected_requested_shortids)
        might_be_requested_shortids = [get_short_id(
            tx, self.compute_salt()) for tx in might_be_requested_txs]
        assert(extra_requested_shortids.issubset(might_be_requested_shortids))
        self.test_node.last_reconcildiff = []

    def request_transactions(self, txs_to_request):
        # Make sure there were no unexpected transactions received before
        assert_equal(self.test_node.last_tx, [])

        wtxids_to_request = [tx.calc_sha256(
            with_witness=True) for tx in txs_to_request]
        self.test_node.send_getdata(wtxids_to_request)

        def received_tx():
            return (len(self.test_node.last_tx) == len(txs_to_request))
        self.wait_until(received_tx, timeout=30)
        assert_equal(set([tx.calc_sha256(True)
                     for tx in txs_to_request]), set(self.test_node.last_tx))
        self.test_node.last_tx = []

    def expect_announcements(self, expected_txs, extra_candidates_txs):
        expected_wtxids = [tx.calc_sha256(True)
                           for tx in expected_txs]
        extra_candidates_wtxids = [tx.calc_sha256(True)
                                   for tx in extra_candidates_txs]
        self.wait_until(lambda: (set(expected_wtxids).issubset(
            set(self.test_node.last_inv))), timeout=30)
        extra_received_wtxids = set(
            self.test_node.last_inv) - set(expected_wtxids)
        assert(extra_received_wtxids.issubset(extra_candidates_wtxids))
        self.test_node.last_inv = []
        return extra_received_wtxids

    #
    # Actual test cases
    #

    def reconciliation_initiator_flow(self, n_node, n_mininode, n_shared,
                                      capacity, terminate_after_initial,
                                      expected_success):

        # Generate and submit transactions.
        mininode_unique_txs, node_unique_txs, shared_txs = self.generate_txs(
            n_mininode, n_node, n_shared)
        mininode_txs = mininode_unique_txs + shared_txs
        node_txs = node_unique_txs + shared_txs
        time.sleep(0.1)
        self.proceed_in_time(INVENTORY_BROADCAST_INTERVAL + 3)

        # First, check that the node sends a reconciliation request, claiming to have some
        # transactions in their set. Sending out a request always happens after adding to the set
        # per net_processing.cpp.
        self.receive_reqreconcil(expected_set_size=len(node_txs))

        # We check that transactions received by the node during the reconciliation round
        # are not lost, and will be announced either during the reconciliation (low-fanout) or
        # after it (next reconciliation round).
        more_node_txs = []
        extra_received_wtxids = []
        if terminate_after_initial:
            # Respond to the node with a sketch of a given capacity over given transactions.
            self.transmit_sketch(txs_to_sketch=mininode_txs,
                                 extension=False, capacity=capacity)

            # Add extra transactions, and let them trigger adding to recon sets or low-fanout.
            more_node_txs.extend(self.generate_txs(0, 10, 0)[1])
            time.sleep(0.1)
            self.proceed_in_time(INVENTORY_BROADCAST_INTERVAL + 3)

            if expected_success:
                # Expect the node to guess and request the transactions which it is missing,
                # and possibly some of the shared transactions which were set for low-fanout by
                # the node, and thus indicated as missing by reconciliation.
                self.handle_reconciliation_finalization(expected_success=True,
                                                        expected_requested_txs=mininode_unique_txs,
                                                        might_be_requested_txs=shared_txs)
            else:
                # This happens only if one of the sets (or both) was empty.
                # The node can't guess what it's missing, so it won't explicitly request anything.
                self.handle_reconciliation_finalization(expected_success=False,
                                                        expected_requested_txs=[],
                                                        might_be_requested_txs=[])

            # A node would also announce transactions it has for us (the mininode). It could
            # also announce extra transactions here:
            # - shared transactions which were set for low-fanout instead of reconciliation
            # - transactions received during the reconciliation round and set for low-fanout
            # These two types of announcements are returned.
            extra_received_wtxids = self.expect_announcements(
                node_unique_txs, shared_txs + more_node_txs)
        else:
            # Respond to the node with a sketch of a given capacity over given transactions.
            self.transmit_sketch(txs_to_sketch=mininode_txs,
                                 extension=False, capacity=capacity)

            # Add extra transactions, and let them trigger adding to recon sets or low-fanout.
            more_node_txs.extend(self.generate_txs(0, 4, 0)[1])
            time.sleep(0.1)
            self.proceed_in_time(INVENTORY_BROADCAST_INTERVAL + 3)

            # Expect the node to request sketch extension, because the sketch we sent to it
            # was insufficient.
            self.handle_extension_request()

            # Add extra transactions, and let them trigger adding to recon sets or low-fanout.
            more_node_txs.extend(self.generate_txs(0, 4, 0)[1])
            time.sleep(0.1)
            self.proceed_in_time(INVENTORY_BROADCAST_INTERVAL + 3)

            # Send a sketch extension to the node.
            self.transmit_sketch(txs_to_sketch=mininode_txs,
                                 extension=True, capacity=capacity)
            if expected_success:
                # Expect the node to guess and request the transactions which it is missing,
                # and possibly some of the shared transactions which were set for low-fanout by
                # the node, and thus indicated as missing by reconciliation.
                self.handle_reconciliation_finalization(expected_success=True,
                                                        expected_requested_txs=mininode_unique_txs,
                                                        might_be_requested_txs=shared_txs)

                # A node would also announce transactions it has for us (the mininode). It could
                # also announce extra transactions here:
                # - shared transactions which were set for low-fanout instead of reconciliation
                # - transactions received during the reconciliation round and set for low-fanout
                # These two types of announcements are returned.
                extra_received_wtxids = self.expect_announcements(
                    node_unique_txs, shared_txs + more_node_txs)
            else:
                # The node can't guess what it's missing, so it won't explicitly request anything.
                self.handle_reconciliation_finalization(expected_success=False,
                                                        expected_requested_txs=[],
                                                        might_be_requested_txs=[])

                # A node would also announce transactions it has for us (the mininode). It could
                # also announce extra transactions here:
                # - transactions received during the reconciliation round and set for low-fanout
                # These announcements are returned.
                extra_received_wtxids = self.expect_announcements(
                    node_txs, more_node_txs)

        # Request the transactions the node announced.
        self.request_transactions(node_unique_txs)

        # Check those additional transactions are not lost. First, filter out those which were
        # already announced during the reconciliation round.
        more_node_txs = [tx for tx in more_node_txs if tx.calc_sha256(
            with_witness=True) not in extra_received_wtxids]

        if more_node_txs != []:
            # Make a regular early-exit reconciliation round, which would trigger all transactions
            # from the set to be announced
            self.receive_reqreconcil(expected_set_size=len(more_node_txs))
            self.transmit_sketch(txs_to_sketch=[], extension=False, capacity=0)
            self.handle_reconciliation_finalization(expected_success=False,
                                                    expected_requested_txs=[],
                                                    might_be_requested_txs=[])

            # Check that all of those extra transactions are announced (either via low-fanout,
            # or via either of the 2 possible reconciliation rounds). We also check that no
            # other transactions were announced (second parameter).
            self.expect_announcements(more_node_txs, [])

    def test_recon_initiator(self):
        # These node will consume some of the low-fanout announcements, and add to the
        # reconciliation peers queue.
        for _ in range(EXTRA_FANOUT_CANDIDATES):
            fanout_destination = self.nodes[0].add_p2p_connection(
                TestTxReconResponderP2PConn(), inbound=True)
            fanout_destination.wait_for_verack()

        self.make_or_reset_reconciliation_conn()
        # 20 at node, 0 at mininode, 0 shared, early exit.
        self.reconciliation_initiator_flow(20, 0, 0, 0, True, False)
        # 0 at node, 20 at mininode, 0 shared, early exit.
        self.reconciliation_initiator_flow(0, 20, 0, 0, True, False)
        # 20 at node, 20 at mininode, 10 shared, initial reconciliation succeeds
        self.reconciliation_initiator_flow(20, 20, 10, 54, True, True)
        # 20 at node, 20 at mininode, 10 shared, initial reconciliation fails,
        # extension succeeds
        self.reconciliation_initiator_flow(20, 20, 10, 27, False, True)
        # 20 at node, 20 at mininode, 10 shared, initial reconciliation fails,
        # extension fails
        self.reconciliation_initiator_flow(20, 20, 10, 10, False, False)

        # Test disconnect on SKETCH violation by exceeding max sketch capacity
        MAX_SKETCH_CAPACITY = 2 << 12
        self.make_or_reset_reconciliation_conn()
        serialized_size = (MAX_SKETCH_CAPACITY) * 4 + 1
        self.test_node.send_sketch([1 * serialized_size])
        self.test_node.wait_for_disconnect()

    def run_test(self):
        super().run_test()
        self.test_recon_initiator()


if __name__ == '__main__':
    ReconciliationInitiatorTest().main()
