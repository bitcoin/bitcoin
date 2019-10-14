#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test conflicts tracking with multilple txn conflicting a conflicted tx."""

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
        assert_equal,
        connect_nodes,
        disconnect_nodes,
)

class TxConflicts(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # Send tx from which to conflict outputs later
        txid_conflict_from_1 = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), Decimal("10"))
        txid_conflict_from_2 = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), Decimal("10"))
        self.nodes[0].generate(1)
        self.sync_blocks()

        # Disconnect to broadcast conflicts on their respective chains
        disconnect_nodes(self.nodes[0], 1)
        disconnect_nodes(self.nodes[2], 1)
        disconnect_nodes(self.nodes[2], 0)

        # Create txn, with conflicted txn spending outpoint A & B, A is going to be spent by conflicting_tx_A and B is going to be spent by conflicting_tx_B
        nA = next(tx_out["vout"] for tx_out in self.nodes[0].gettransaction(txid_conflict_from_1)["details"] if tx_out["amount"] == Decimal("10"))
        nB = next(tx_out["vout"] for tx_out in self.nodes[0].gettransaction(txid_conflict_from_2)["details"] if tx_out["amount"] == Decimal("10"))
        inputs_conflicted_tx = []
        inputs_conflicted_tx.append({"txid": txid_conflict_from_1, "vout": nA})
        inputs_conflicted_tx.append({"txid": txid_conflict_from_2, "vout": nB})
        inputs_conflicting_tx_A = []
        inputs_conflicting_tx_A.append({"txid": txid_conflict_from_1, "vout": nA})
        inputs_conflicting_tx_B = []
        inputs_conflicting_tx_B.append({"txid": txid_conflict_from_2, "vout": nB})

        outputs_conflicted_tx = {}
        outputs_conflicted_tx[self.nodes[0].getnewaddress()] = Decimal("19.99998")
        outputs_conflicting_tx_A = {}
        outputs_conflicting_tx_A[self.nodes[0].getnewaddress()] = Decimal("9.99998")
        outputs_conflicting_tx_B = {}
        outputs_conflicting_tx_B[self.nodes[0].getnewaddress()] = Decimal("9.99998")

        conflicted = self.nodes[0].signrawtransactionwithwallet(self.nodes[0].createrawtransaction(inputs_conflicted_tx, outputs_conflicted_tx))
        conflicting_tx_A = self.nodes[0].signrawtransactionwithwallet(self.nodes[0].createrawtransaction(inputs_conflicting_tx_A, outputs_conflicting_tx_A))
        conflicting_tx_B = self.nodes[0].signrawtransactionwithwallet(self.nodes[0].createrawtransaction(inputs_conflicting_tx_B, outputs_conflicting_tx_B))

        # Broadcast conflicted tx
        conflicted_txid = self.nodes[0].sendrawtransaction(conflicted["hex"])
        self.nodes[0].generate(1)

        # Build child conflicted tx
        nA = next(tx_out["vout"] for tx_out in self.nodes[0].gettransaction(conflicted_txid)["details"] if tx_out["amount"] == Decimal("19.99998"))
        inputs_child_conflicted = []
        inputs_child_conflicted.append({"txid": conflicted_txid, "vout": nA})
        outputs_child_conflicted = {}
        outputs_child_conflicted[self.nodes[0].getnewaddress()] = Decimal("19.99996")
        child_conflicted = self.nodes[0].signrawtransactionwithwallet(self.nodes[0].createrawtransaction(inputs_child_conflicted, outputs_child_conflicted))
        child_conflicted_txid = self.nodes[0].sendrawtransaction(child_conflicted["hex"])
        self.nodes[0].generate(1)

        # Broadcast conflicting txn on node1 chain
        conflicting_txid_A = self.nodes[1].sendrawtransaction(conflicting_tx_A["hex"])
        self.nodes[1].generate(4)
        self.nodes[1].sendrawtransaction(conflicting_tx_B["hex"])
        self.nodes[1].generate(4)

        # Reconnect node0 and node1 and check that conflicted_txid is effectively conflicted
        connect_nodes(self.nodes[0], 1)
        self.sync_blocks([self.nodes[0], self.nodes[1]])
        conflicted = self.nodes[0].gettransaction(conflicted_txid)
        child_conflicted = self.nodes[0].gettransaction(child_conflicted_txid)
        conflicting = self.nodes[0].gettransaction(conflicting_txid_A)
        # Conflicted tx should have confirmations set to the confirmations of the most conflicting tx
        assert_equal(conflicted["confirmations"], -conflicting["confirmations"])
        # Child should inherit conflicted state from parent
        assert_equal(child_conflicted["confirmations"], -conflicting["confirmations"])

        # Node2 chain without conflicts
        self.nodes[2].generate(15)

        # Connect node0 and node2 and wait reorg
        connect_nodes(self.nodes[0], 2)
        self.sync_blocks([self.nodes[0], self.nodes[2]])
        conflicted = self.nodes[0].gettransaction(conflicted_txid)
        child_conflicted = self.nodes[0].gettransaction(child_conflicted_txid)
        # Former conflicted tx should be unoncifmred as it hasn't been yet rebroadcast
        assert_equal(conflicted["confirmations"], 0)
        # Former conflicted child tx should be unconfirmed as it hasn't been rebroadcast
        assert_equal(child_conflicted["confirmations"], 0)
        # Rebroadcast former conflicted tx and check it confirms smoothly
        self.nodes[2].sendrawtransaction(conflicted["hex"])
        self.nodes[2].generate(1)
        self.sync_blocks([self.nodes[0], self.nodes[2]])
        former_conflicted = self.nodes[0].gettransaction(conflicted_txid)
        assert_equal(former_conflicted["confirmations"], 1)
        assert_equal(former_conflicted["blockheight"], 217)

if __name__ == '__main__':
    TxConflicts().main()
