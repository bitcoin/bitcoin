#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Test that wallet correctly tracks transactions that have been conflicted by blocks, particularly during reorgs.
"""

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    find_vout_for_address,
)

class TxConflicts(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 3

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def get_utxo_of_value(self, from_tx_id, search_value):
        return next(tx_out["vout"] for tx_out in self.nodes[0].gettransaction(from_tx_id)["details"] if tx_out["amount"] == Decimal(f"{search_value}"))

    def run_test(self):
        self.log.info("Send tx from which to conflict outputs later")
        txid_conflict_from_1 = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), Decimal("10"))
        txid_conflict_from_2 = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), Decimal("10"))
        self.generate(self.nodes[0], 1)
        self.sync_blocks()

        self.log.info("Disconnect nodes to broadcast conflicts on their respective chains")
        self.disconnect_nodes(0, 1)
        self.disconnect_nodes(2, 1)

        self.log.info("Create transactions that conflict with each other")
        output_A = self.get_utxo_of_value(from_tx_id=txid_conflict_from_1, search_value=10)
        output_B = self.get_utxo_of_value(from_tx_id=txid_conflict_from_2, search_value=10)

        # First create a transaction that consumes both A and B outputs.
        #
        # | tx1 |  ----->  |                |         |               |
        #                  |  AB_parent_tx  |  ---->  |   Child_Tx    |
        # | tx2 |  ----->  |                |         |               |
        #
        inputs_tx_AB_parent = [{"txid": txid_conflict_from_1, "vout": output_A}, {"txid": txid_conflict_from_2, "vout": output_B}]
        tx_AB_parent = self.nodes[0].signrawtransactionwithwallet(self.nodes[0].createrawtransaction(inputs_tx_AB_parent, {self.nodes[0].getnewaddress(): Decimal("19.99998")}))

        # Secondly, create two transactions: One consuming output_A, and another one consuming output_B
        #
        # | tx1 |  ----->  |     Tx_A_1     |
        #                   ----------------
        # | tx2 |  ----->  |     Tx_B_1     |
        #
        inputs_tx_A_1 = [{"txid": txid_conflict_from_1, "vout": output_A}]
        inputs_tx_B_1 = [{"txid": txid_conflict_from_2, "vout": output_B}]
        tx_A_1 = self.nodes[0].signrawtransactionwithwallet(self.nodes[0].createrawtransaction(inputs_tx_A_1, {self.nodes[0].getnewaddress(): Decimal("9.99998")}))
        tx_B_1 = self.nodes[0].signrawtransactionwithwallet(self.nodes[0].createrawtransaction(inputs_tx_B_1, {self.nodes[0].getnewaddress(): Decimal("9.99998")}))

        self.log.info("Broadcast conflicted transaction")
        txid_AB_parent = self.nodes[0].sendrawtransaction(tx_AB_parent["hex"])
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)

        # Now that 'AB_parent_tx' was broadcast, build 'Child_Tx'
        output_c = self.get_utxo_of_value(from_tx_id=txid_AB_parent, search_value=19.99998)
        inputs_tx_C_child = [({"txid": txid_AB_parent, "vout": output_c})]

        tx_C_child = self.nodes[0].signrawtransactionwithwallet(self.nodes[0].createrawtransaction(inputs_tx_C_child, {self.nodes[0].getnewaddress() : Decimal("19.99996")}))
        tx_C_child_txid = self.nodes[0].sendrawtransaction(tx_C_child["hex"])
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)

        self.log.info("Broadcast conflicting tx to node 1 and generate a longer chain")
        conflicting_txid_A = self.nodes[1].sendrawtransaction(tx_A_1["hex"])
        self.generate(self.nodes[1], 4, sync_fun=self.no_op)
        conflicting_txid_B = self.nodes[1].sendrawtransaction(tx_B_1["hex"])
        self.generate(self.nodes[1], 4, sync_fun=self.no_op)

        self.log.info("Connect nodes 0 and 1, trigger reorg and ensure that the tx is effectively conflicted")
        self.connect_nodes(0, 1)
        self.sync_blocks([self.nodes[0], self.nodes[1]])
        conflicted_AB_tx = self.nodes[0].gettransaction(txid_AB_parent)
        tx_C_child = self.nodes[0].gettransaction(tx_C_child_txid)
        conflicted_A_tx = self.nodes[0].gettransaction(conflicting_txid_A)

        self.log.info("Verify, after the reorg, that Tx_A was accepted, and tx_AB and its Child_Tx are conflicting now")
        # Tx A was accepted, Tx AB was not.
        assert conflicted_AB_tx["confirmations"] < 0
        assert conflicted_A_tx["confirmations"] > 0

        # Conflicted tx should have confirmations set to the confirmations of the most conflicting tx
        assert_equal(-conflicted_AB_tx["confirmations"], conflicted_A_tx["confirmations"])
        # Child should inherit conflicted state from parent
        assert_equal(-tx_C_child["confirmations"], conflicted_A_tx["confirmations"])
        # Check the confirmations of the conflicting transactions
        assert_equal(conflicted_A_tx["confirmations"], 8)
        assert_equal(self.nodes[0].gettransaction(conflicting_txid_B)["confirmations"], 4)

        self.log.info("Now generate a longer chain that does not contain any tx")
        # Node2 chain without conflicts
        self.generate(self.nodes[2], 15, sync_fun=self.no_op)

        # Connect node0 and node2 and wait reorg
        self.connect_nodes(0, 2)
        self.sync_blocks()
        conflicted = self.nodes[0].gettransaction(txid_AB_parent)
        tx_C_child = self.nodes[0].gettransaction(tx_C_child_txid)

        self.log.info("Test that formerly conflicted transaction are inactive after reorg")
        # Former conflicted tx should be unconfirmed as it hasn't been yet rebroadcast
        assert_equal(conflicted["confirmations"], 0)
        # Former conflicted child tx should be unconfirmed as it hasn't been rebroadcast
        assert_equal(tx_C_child["confirmations"], 0)
        # Rebroadcast former conflicted tx and check it confirms smoothly
        self.nodes[2].sendrawtransaction(conflicted["hex"])
        self.generate(self.nodes[2], 1)
        self.sync_blocks()
        former_conflicted = self.nodes[0].gettransaction(txid_AB_parent)
        assert_equal(former_conflicted["confirmations"], 1)
        assert_equal(former_conflicted["blockheight"], 217)

        self.test_unknown_parent_conflict()

    def test_unknown_parent_conflict(self):
        self.log.info("Test that a conflict with parent not belonging to the wallet causes in-wallet children to also conflict")
        self.nodes[0].createwallet("parent_conflict")
        wallet = self.nodes[0].get_wallet_rpc("parent_conflict")
        def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        # one utxo to target wallet
        addr = wallet.getnewaddress()
        addr_desc = wallet.getaddressinfo(addr)["desc"]
        def_wallet.sendtoaddress(addr, 10)
        self.generate(self.nodes[0], 1)
        utxo = wallet.listunspent()[0]

        # Make utxo in grandparent that will be double spent
        gp_addr = def_wallet.getnewaddress()
        gp_txid = def_wallet.sendtoaddress(gp_addr, 7)
        gp_utxo = {"txid": gp_txid, "vout": find_vout_for_address(self.nodes[0], gp_txid, gp_addr)}
        self.generate(self.nodes[0], 1)

        assert_equal(self.nodes[0].getrawmempool(), [])

        # make unconfirmed parent tx
        parent_addr = def_wallet.getnewaddress()
        parent_txid = def_wallet.send(outputs=[{parent_addr: 5}], inputs=[gp_utxo])["txid"]
        parent_utxo = {"txid": parent_txid, "vout": find_vout_for_address(self.nodes[0], parent_txid, parent_addr)}
        parent_me = self.nodes[0].getmempoolentry(parent_txid)
        parent_feerate = parent_me["fees"]["base"] * 100000000 / parent_me["vsize"]
        assert_equal(self.nodes[0].getrawmempool(), [parent_txid])

        # make child tx that has one input belonging to target wallet, and one input not
        psbt = def_wallet.walletcreatefundedpsbt(inputs=[utxo, parent_utxo], outputs=[{def_wallet.getnewaddress(): 13}], solving_data={"descriptors":[addr_desc]})["psbt"]
        psbt = def_wallet.walletprocesspsbt(psbt)["psbt"]
        psbt_proc = wallet.walletprocesspsbt(psbt)
        psbt = psbt_proc["psbt"]
        assert_equal(psbt_proc["complete"], True)
        child_txid = self.nodes[0].sendrawtransaction(psbt_proc["hex"])
        assert_equal(set(self.nodes[0].getrawmempool()), {child_txid, parent_txid})

        assert_equal(def_wallet.gettransaction(parent_txid)["confirmations"], 0)
        assert_equal(wallet.gettransaction(child_txid)["confirmations"], 0)

        # Make a conflict spending parent
        conflict_psbt = def_wallet.walletcreatefundedpsbt(inputs=[gp_utxo], outputs=[{def_wallet.getnewaddress(): 2}], fee_rate=parent_feerate*3)["psbt"]
        conflict_proc = def_wallet.walletprocesspsbt(conflict_psbt)
        assert_equal(conflict_proc["complete"], True)
        conflict_txid = self.nodes[0].sendrawtransaction(conflict_proc["hex"])
        assert_equal(set(self.nodes[0].getrawmempool()), {conflict_txid})

        self.generate(self.nodes[0], 1)

        assert_equal(def_wallet.gettransaction(parent_txid)["confirmations"], -1)
        assert_equal(wallet.gettransaction(child_txid)["confirmations"], -1) # Currently fails here

if __name__ == '__main__':
    TxConflicts().main()
