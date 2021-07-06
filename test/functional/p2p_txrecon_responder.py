#!/usr/bin/env python3
# Copyright (c) 2021-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test reconciliation-based transaction relay (node responds)"""

import random
import time

from test_framework.messages import (
    msg_reqtxrcncl, msg_reqsketchext, msg_reconcildiff,
)

from test_framework.util import assert_equal

from test_framework.p2p_txrecon import (
    BYTES_PER_SKETCH_CAPACITY, create_sketch, get_short_id, Q_PRECISION,
    estimate_capacity, RECON_Q, ReconciliationTest, TxReconTestP2PConn
)

# Taken from net_processing.cpp
RECON_RESPONSE_INTERVAL = 1
# A caller should allow a lot of extra time to handle Poisson delays.
INVENTORY_BROADCAST_INTERVAL = 2


class TestTxReconInitiatorP2PConn(TxReconTestP2PConn):
    # This object simulates a reconciliation initiator, which will communicate with
    # the Bitcoin Core node being tested.
    def __init__(self):
        super().__init__()
        self.last_sketch = []

    def send_reqtxrcncl(self, set_size, q):
        msg = msg_reqtxrcncl()
        msg.set_size = set_size
        msg.q = q
        self.send_message(msg)

    def on_sketch(self, message):
        self.last_sketch.append(message)

    def send_reqsketchext(self):
        msg = msg_reqsketchext()
        self.send_message(msg)

    def send_reconcildiff(self, success, ask_shortids):
        msg = msg_reconcildiff()
        msg.success = success
        msg.ask_shortids = ask_shortids
        self.send_message(msg)


class ReconciliationResponderTest(ReconciliationTest):
    def set_test_params(self):
        super().set_test_params()

    # Check that the node announced a sketch, and return an estimate of how many transaction
    # the node had it the set, based on:
    # - the sketch capacity
    # - reconciliation request it handled
    def expect_sketch(self, n_mininode):
        # Give time to receive request and start the response delay timer.
        self.proceed_in_time(RECON_RESPONSE_INTERVAL + 2)
        time.sleep(0.1)

        def received_sketch():
            # We might have received several unintended sketches before. Consider the latest.
            return (len(self.test_node.last_sketch) == 1)
        self.wait_until(received_sketch, timeout=30)

        skdata = self.test_node.last_sketch[0].skdata
        if skdata == []:
            return 0

        capacity = len(skdata) / BYTES_PER_SKETCH_CAPACITY
        # Infer sketched txs based on the diff formula
        # capacity = abs(their - ours) + RECON_Q * min(theirs, ours) + 1
        # Note that it's impossible to infer this 100% accurate and sometimes the result is
        # off-by-one. This should be handled later on.
        sketched_txs = capacity - 1 - int(RECON_Q * n_mininode) + n_mininode
        return sketched_txs

    # Check that the node announced the exact sketch we expected (of the expected capacity,
    # over the expected transactions, and initial/extension)
    def check_sketch(self, n_mininode, expected_transactions, extension):
        expected_short_ids = [get_short_id(
            tx, self.compute_salt()) for tx in expected_transactions]

        if n_mininode == 0 or len(expected_transactions) == 0:
            # A peer should send us an empty sketch to trigger reconciliation termination.
            # This should never happen in the extension phase.
            assert not extension
            expected_capacity = 0
        else:
            expected_capacity = estimate_capacity(
                n_mininode, len(expected_transactions))

        if extension:
            expected_sketch = create_sketch(
                expected_short_ids, expected_capacity * 2)[
                int(expected_capacity * BYTES_PER_SKETCH_CAPACITY):]
        else:
            expected_sketch = create_sketch(
                expected_short_ids, expected_capacity)

        assert_equal(
            self.test_node.last_sketch[0].skdata, expected_sketch)
        self.test_node.last_sketch = []

    # Check that the node announced N transactions as we expected (or N+1, see below).
    # Return the announced transactions.
    def handle_announcements(self, n_expected):
        if n_expected <= 0:
            return []

        def received_inv():
            # The second condition handles the case where we can't accurately infer how many
            # transactions were sketched based on the capacity, and thus how many are fanouted here.
            return len(self.test_node.last_inv) == n_expected or len(self.test_node.last_inv) == n_expected + 1

        self.wait_until(received_inv, timeout=30)
        announced_invs = self.test_node.last_inv
        self.test_node.last_inv = []
        return announced_invs

    # Send reconciliation finalization message to the node, and request some of its transactions
    # based on their shortids.
    def finalize_reconciliation(self, success, txs_to_request):
        ask_shortids = [get_short_id(tx, self.compute_salt())
                        for tx in txs_to_request]
        self.test_node.send_reconcildiff(success, ask_shortids)

    #
    # Actual test cases
    #

    def reconciliation_responder_flow(self, n_mininode, n_node, initial_result,
                                      result):
        # Generate transactions and let them trigger announcement (low-fanout or adding
        # to recon set).
        _, node_txs, _ = self.generate_txs(n_mininode, n_node, 0)
        self.proceed_in_time(INVENTORY_BROADCAST_INTERVAL + 10)

        # We will announce extra transactions which should not be lost during the
        # reconciliation round, and should be relayed along or in the next round.
        more_node_txs = []
        # We track transactions which are announced during the reconciliation round.
        announced_invs = []

        self.test_node.send_reqtxrcncl(n_mininode, int(RECON_Q * Q_PRECISION))
        time.sleep(0.1)
        # The typical check consists of 3 parts:
        # 1. Receive a sketch of some size N: expect_sketch.
        # 2. Based on the capacity of that sketch, infer how many transactions should be announced
        #    by the node, as opposed to be included in that sketch (1).
        #    Receive them in handle_announcements.
        # 3. Check that the sketch we received in (1) corresponds to all the transactions the node
        #    had except for those announced in (2): check_sketch.
        n_sketched_txs = self.expect_sketch(n_mininode)

        # Some transactions would be announced via fanout right away, so they won't be included in
        # the sketch. This block handles only cases where both nodes had transactions, because
        # we infer how many transactions should be relayed based on the sketch. In cases when there
        # are no transactions, the sketch is always empty, but that doesn't mean all transactions
        # are fanouted.
        #
        # It's still possible some transactions are announced, but we will handle that later.
        if n_mininode != 0 and n_node != 0:
            announced_invs += self.handle_announcements(
                n_node - n_sketched_txs)
            node_txs = [tx for tx in node_txs if tx.calc_sha256(
                with_witness=True) not in announced_invs]

        self.check_sketch(n_mininode, node_txs, False)

        more_node_txs.extend(self.generate_txs(0, 8, 0)[1])
        self.proceed_in_time(INVENTORY_BROADCAST_INTERVAL + 10)

        if not initial_result and n_mininode != 0 and n_node != 0:
            # Request sketch extension and check that we receive what was expected.
            # Does not work when one of the sets is empty, because the node knows it's pointless.
            self.test_node.send_reqsketchext()
            self.expect_sketch(n_mininode)
            self.check_sketch(n_mininode, node_txs, True)

        more_node_txs.extend(self.generate_txs(0, 8, 0)[1])
        self.proceed_in_time(INVENTORY_BROADCAST_INTERVAL + 10)

        expected_txs_announced = []
        if result:
            # In the success case, we query the node for a subset of transactions they have
            # within the finalization message.
            expected_txs_announced = random.sample(node_txs, 3)
            self.finalize_reconciliation(True, expected_txs_announced)
        else:
            # In the failure case, the node should announce all transactions it has.
            self.finalize_reconciliation(False, [])
            expected_txs_announced = node_txs

        # Receive expected announcements from the node.
        expected_wtxids = [tx.calc_sha256(
            with_witness=True) for tx in expected_txs_announced]

        def received_inv():
            # These may include extra transactions: in addition to the expected txs,
            # a peer might INV us extra transactions they received during this round.
            return (set(expected_wtxids).issubset(set(self.test_node.last_inv)))
        self.wait_until(received_inv, timeout=30)
        announced_invs += self.test_node.last_inv
        self.test_node.last_inv = []

        # Check those additional transactions are not lost.
        # First, filter those which were already announced.
        more_node_txs = [tx for tx in more_node_txs if tx.calc_sha256(
            with_witness=True) not in announced_invs]

        # Trigger the regular reconciliation event at the node.
        self.test_node.send_reqtxrcncl(1, int(RECON_Q * Q_PRECISION))
        time.sleep(0.1)

        n_sketched_txs = self.expect_sketch(1)
        announced_invs += self.handle_announcements(
            len(more_node_txs) - n_sketched_txs)

        more_node_txs = [tx for tx in more_node_txs if tx.calc_sha256(
            with_witness=True) not in announced_invs]
        self.check_sketch(1, more_node_txs, False)
        self.finalize_reconciliation(True, txs_to_request=[])

    def test_recon_responder(self):
        # These node will consume some of the low-fanout announcements.
        for _ in range(4):
            fanout_consumer = self.nodes[0].add_p2p_connection(TestTxReconInitiatorP2PConn())
            fanout_consumer.wait_for_verack()

        self.test_node = self.nodes[0].add_p2p_connection(TestTxReconInitiatorP2PConn())
        self.test_node.wait_for_verack()

        # Early exit, expect empty sketch.
        self.reconciliation_responder_flow(0, 15, False, False)
        # Early exit, expect empty sketch.
        self.reconciliation_responder_flow(20, 0, False, False)
        # # Initial reconciliation succeeds
        self.reconciliation_responder_flow(3, 15, True, False)
        # # Initial reconciliation fails, extension succeeds
        self.reconciliation_responder_flow(3, 15, False, True)
        # # Initial reconciliation fails, extension fails
        self.reconciliation_responder_flow(3, 15, False, False)

        # Test disconnect on RECONCILDIFF violation
        self.test_node = self.nodes[0].add_p2p_connection(TestTxReconInitiatorP2PConn())
        self.test_node.wait_for_verack()
        self.finalize_reconciliation(True, [])
        self.test_node.wait_for_disconnect()

    def run_test(self):
        super().run_test()
        self.test_recon_responder()


if __name__ == '__main__':
    ReconciliationResponderTest().main()
