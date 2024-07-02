#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Test that wallet correctly tracks transactions that have been conflicted by blocks, particularly during reorgs.
"""

from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.messages import COIN
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
        """
        The following tests check the behavior of the wallet when
        transaction conflicts are created. These conflicts are created
        using raw transaction RPCs that double-spend UTXOs and have more
        fees, replacing the original transaction.
        """

        self.test_block_conflicts()
        self.test_unknown_parent_conflict()
        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 7, self.nodes[2].getnewaddress())
        self.test_mempool_conflict()
        self.test_mempool_and_block_conflicts()
        self.test_descendants_with_mempool_conflicts()

    def test_block_conflicts(self):
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

    def test_mempool_conflict(self):
        self.nodes[0].createwallet("alice")
        alice = self.nodes[0].get_wallet_rpc("alice")

        bob = self.nodes[1]

        self.nodes[2].send(outputs=[{alice.getnewaddress() : 25} for _ in range(3)])
        self.generate(self.nodes[2], 1)

        self.log.info("Test a scenario where a transaction has a mempool conflict")

        unspents = alice.listunspent()
        assert_equal(len(unspents), 3)
        assert all([tx["amount"] == 25 for tx in unspents])

        # tx1 spends unspent[0] and unspent[1]
        raw_tx = alice.createrawtransaction(inputs=[unspents[0], unspents[1]], outputs=[{bob.getnewaddress() : 49.9999}])
        tx1 = alice.signrawtransactionwithwallet(raw_tx)['hex']

        # tx2 spends unspent[1] and unspent[2], conflicts with tx1
        raw_tx = alice.createrawtransaction(inputs=[unspents[1], unspents[2]], outputs=[{bob.getnewaddress() : 49.99}])
        tx2 = alice.signrawtransactionwithwallet(raw_tx)['hex']

        # tx3 spends unspent[2], conflicts with tx2
        raw_tx = alice.createrawtransaction(inputs=[unspents[2]], outputs=[{bob.getnewaddress() : 24.9899}])
        tx3 = alice.signrawtransactionwithwallet(raw_tx)['hex']

        # broadcast tx1
        tx1_txid = alice.sendrawtransaction(tx1)

        assert_equal(alice.listunspent(), [unspents[2]])
        assert_equal(alice.getbalance(), 25)

        # broadcast tx2, replaces tx1 in mempool
        tx2_txid = alice.sendrawtransaction(tx2)

        # Check that unspent[0] is now available because the transaction spending it has been replaced in the mempool
        assert_equal(alice.listunspent(), [unspents[0]])
        assert_equal(alice.getbalance(), 25)

        assert_equal(alice.gettransaction(tx1_txid)["mempoolconflicts"], [tx2_txid])

        self.log.info("Test scenario where a mempool conflict is removed")

        # broadcast tx3, replaces tx2 in mempool
        # Now that tx1's conflict has been removed, tx1 is now
        # not conflicted, and instead is inactive until it is
        # rebroadcasted. Now unspent[0] is not available, because
        # tx1 is no longer conflicted.
        alice.sendrawtransaction(tx3)

        assert_equal(alice.gettransaction(tx1_txid)["mempoolconflicts"], [])
        assert tx1_txid not in self.nodes[0].getrawmempool()

        # now all of alice's outputs should be considered spent
        # unspent[0]: spent by inactive tx1
        # unspent[1]: spent by inactive tx1
        # unspent[2]: spent by active tx3
        assert_equal(alice.listunspent(), [])
        assert_equal(alice.getbalance(), 0)

        # Clean up for next test
        bob.sendall([self.nodes[2].getnewaddress()])
        self.generate(self.nodes[2], 1)

        alice.unloadwallet()

    def test_mempool_and_block_conflicts(self):
        self.nodes[0].createwallet("alice_2")
        alice = self.nodes[0].get_wallet_rpc("alice_2")
        bob = self.nodes[1]

        self.nodes[2].send(outputs=[{alice.getnewaddress() : 25} for _ in range(3)])
        self.generate(self.nodes[2], 1)

        self.log.info("Test a scenario where a transaction has both a block conflict and a mempool conflict")
        unspents = [{"txid" : element["txid"], "vout" : element["vout"]} for element in alice.listunspent()]

        assert_equal(bob.getbalances()["mine"]["untrusted_pending"], 0)

        # alice and bob nodes are disconnected so that transactions can be
        # created by alice, but broadcasted from bob so that alice's wallet
        # doesn't know about them
        self.disconnect_nodes(0, 1)

        # Sends funds to bob
        raw_tx = alice.createrawtransaction(inputs=[unspents[0]], outputs=[{bob.getnewaddress() : 24.99999}])
        raw_tx1 = alice.signrawtransactionwithwallet(raw_tx)['hex']
        tx1_txid = bob.sendrawtransaction(raw_tx1) # broadcast original tx spending unspents[0] only to bob

        # create a conflict to previous tx (also spends unspents[0]), but don't broadcast, sends funds back to alice
        raw_tx = alice.createrawtransaction(inputs=[unspents[0], unspents[2]], outputs=[{alice.getnewaddress() : 49.999}])
        tx1_conflict = alice.signrawtransactionwithwallet(raw_tx)['hex']

        # Sends funds to bob
        raw_tx = alice.createrawtransaction(inputs=[unspents[1]], outputs=[{bob.getnewaddress() : 24.9999}])
        raw_tx2 = alice.signrawtransactionwithwallet(raw_tx)['hex']
        tx2_txid = bob.sendrawtransaction(raw_tx2) # broadcast another original tx spending unspents[1] only to bob

        # create a conflict to previous tx (also spends unspents[1]), but don't broadcast, sends funds to alice
        raw_tx = alice.createrawtransaction(inputs=[unspents[1]], outputs=[{alice.getnewaddress() : 24.9999}])
        tx2_conflict = alice.signrawtransactionwithwallet(raw_tx)['hex']

        bob_unspents = [{"txid" : element, "vout" : 0} for element in [tx1_txid, tx2_txid]]

        # tx1 and tx2 are now in bob's mempool, and they are unconflicted, so bob has these funds
        assert_equal(bob.getbalances()["mine"]["untrusted_pending"], Decimal("49.99989000"))

        # spend both of bob's unspents, child tx of tx1 and tx2
        raw_tx = bob.createrawtransaction(inputs=[bob_unspents[0], bob_unspents[1]], outputs=[{bob.getnewaddress() : 49.999}])
        raw_tx3 = bob.signrawtransactionwithwallet(raw_tx)['hex']
        tx3_txid = bob.sendrawtransaction(raw_tx3) # broadcast tx only to bob

        # alice knows about 0 txs, bob knows about 3
        assert_equal(len(alice.getrawmempool()), 0)
        assert_equal(len(bob.getrawmempool()), 3)

        assert_equal(bob.getbalances()["mine"]["untrusted_pending"], Decimal("49.99900000"))

        # bob broadcasts tx_1 conflict
        tx1_conflict_txid = bob.sendrawtransaction(tx1_conflict)
        assert_equal(len(alice.getrawmempool()), 0)
        assert_equal(len(bob.getrawmempool()), 2) # tx1_conflict kicks out both tx1, and its child tx3

        assert tx2_txid in bob.getrawmempool()
        assert tx1_conflict_txid in bob.getrawmempool()

        assert_equal(bob.gettransaction(tx1_txid)["mempoolconflicts"], [tx1_conflict_txid])
        assert_equal(bob.gettransaction(tx2_txid)["mempoolconflicts"], [])
        assert_equal(bob.gettransaction(tx3_txid)["mempoolconflicts"], [tx1_conflict_txid])

        # check that tx3 is now conflicted, so the output from tx2 can now be spent
        assert_equal(bob.getbalances()["mine"]["untrusted_pending"], Decimal("24.99990000"))

        # we will be disconnecting this block in the future
        alice.sendrawtransaction(tx2_conflict)
        assert_equal(len(alice.getrawmempool()), 1) # currently alice's mempool is only aware of tx2_conflict
        # 11 blocks are mined so that when they are invalidated, tx_2
        # does not get put back into the mempool
        blk = self.generate(self.nodes[0], 11, sync_fun=self.no_op)[0]
        assert_equal(len(alice.getrawmempool()), 0) # tx2_conflict is now mined

        self.connect_nodes(0, 1)
        self.sync_blocks()
        assert_equal(alice.getbestblockhash(), bob.getbestblockhash())

        # now that tx2 has a block conflict, tx1_conflict should be the only tx in bob's mempool
        assert tx1_conflict_txid in bob.getrawmempool()
        assert_equal(len(bob.getrawmempool()), 1)

        # tx3 should now also be block-conflicted by tx2_conflict
        assert_equal(bob.gettransaction(tx3_txid)["confirmations"], -11)
        # bob has no pending funds, since tx1, tx2, and tx3 are all conflicted
        assert_equal(bob.getbalances()["mine"]["untrusted_pending"], 0)
        bob.invalidateblock(blk) # remove tx2_conflict
        # bob should still have no pending funds because tx1 and tx3 are still conflicted, and tx2 has not been re-broadcast
        assert_equal(bob.getbalances()["mine"]["untrusted_pending"], 0)
        assert_equal(len(bob.getrawmempool()), 1)
        # check that tx3 is no longer block-conflicted
        assert_equal(bob.gettransaction(tx3_txid)["confirmations"], 0)

        bob.sendrawtransaction(raw_tx2)
        assert_equal(bob.getbalances()["mine"]["untrusted_pending"], Decimal("24.99990000"))

        # create a conflict to previous tx (also spends unspents[2]), but don't broadcast, sends funds back to alice
        raw_tx = alice.createrawtransaction(inputs=[unspents[2]], outputs=[{alice.getnewaddress() : 24.99}])
        tx1_conflict_conflict = alice.signrawtransactionwithwallet(raw_tx)['hex']

        bob.sendrawtransaction(tx1_conflict_conflict) # kick tx1_conflict out of the mempool
        bob.sendrawtransaction(raw_tx1) #re-broadcast tx1 because it is no longer conflicted

        # Now bob has no pending funds because tx1 and tx2 are spent by tx3, which hasn't been re-broadcast yet
        assert_equal(bob.getbalances()["mine"]["untrusted_pending"], 0)

        bob.sendrawtransaction(raw_tx3)
        assert_equal(len(bob.getrawmempool()), 4) # The mempool contains: tx1, tx2, tx1_conflict_conflict, tx3
        assert_equal(bob.getbalances()["mine"]["untrusted_pending"], Decimal("49.99900000"))

        # Clean up for next test
        bob.reconsiderblock(blk)
        assert_equal(alice.getbestblockhash(), bob.getbestblockhash())
        self.sync_mempools()
        self.generate(self.nodes[2], 1)

        alice.unloadwallet()

    def test_descendants_with_mempool_conflicts(self):
        self.nodes[0].createwallet("alice_3")
        alice = self.nodes[0].get_wallet_rpc("alice_3")

        self.nodes[2].send(outputs=[{alice.getnewaddress() : 25} for _ in range(2)])
        self.generate(self.nodes[2], 1)

        self.nodes[1].createwallet("bob_1")
        bob = self.nodes[1].get_wallet_rpc("bob_1")

        self.nodes[2].createwallet("carol")
        carol = self.nodes[2].get_wallet_rpc("carol")

        self.log.info("Test a scenario where a transaction's parent has a mempool conflict")

        unspents = alice.listunspent()
        assert_equal(len(unspents), 2)
        assert all([tx["amount"] == 25 for tx in unspents])

        assert_equal(alice.getrawmempool(), [])

        # Alice spends first utxo to bob in tx1
        raw_tx = alice.createrawtransaction(inputs=[unspents[0]], outputs=[{bob.getnewaddress() : 24.9999}])
        tx1 = alice.signrawtransactionwithwallet(raw_tx)['hex']
        tx1_txid = alice.sendrawtransaction(tx1)

        self.sync_mempools()

        assert_equal(alice.getbalance(), 25)
        assert_equal(bob.getbalances()["mine"]["untrusted_pending"], Decimal("24.99990000"))

        assert_equal(bob.gettransaction(tx1_txid)["mempoolconflicts"],  [])

        raw_tx = bob.createrawtransaction(inputs=[bob.listunspent(minconf=0)[0]], outputs=[{carol.getnewaddress() : 24.999}])
        # Bob creates a child to tx1
        tx1_child = bob.signrawtransactionwithwallet(raw_tx)['hex']
        tx1_child_txid = bob.sendrawtransaction(tx1_child)

        self.sync_mempools()

        # Currently neither tx1 nor tx1_child should have any conflicts
        assert_equal(bob.gettransaction(tx1_txid)["mempoolconflicts"],  [])
        assert_equal(bob.gettransaction(tx1_child_txid)["mempoolconflicts"],  [])
        assert tx1_txid in bob.getrawmempool()
        assert tx1_child_txid in bob.getrawmempool()
        assert_equal(len(bob.getrawmempool()), 2)

        assert_equal(bob.getbalances()["mine"]["untrusted_pending"], 0)
        assert_equal(carol.getbalances()["mine"]["untrusted_pending"], Decimal("24.99900000"))

        # Alice spends first unspent again, conflicting with tx1
        raw_tx = alice.createrawtransaction(inputs=[unspents[0], unspents[1]], outputs=[{carol.getnewaddress() : 49.99}])
        tx1_conflict = alice.signrawtransactionwithwallet(raw_tx)['hex']
        tx1_conflict_txid = alice.sendrawtransaction(tx1_conflict)

        self.sync_mempools()

        assert_equal(bob.getbalances()["mine"]["untrusted_pending"], 0)
        assert_equal(carol.getbalances()["mine"]["untrusted_pending"], Decimal("49.99000000"))

        assert tx1_txid not in bob.getrawmempool()
        assert tx1_child_txid not in bob.getrawmempool()
        assert tx1_conflict_txid in bob.getrawmempool()
        assert_equal(len(bob.getrawmempool()), 1)

        # Now both tx1 and tx1_child are conflicted by tx1_conflict
        assert_equal(bob.gettransaction(tx1_txid)["mempoolconflicts"],  [tx1_conflict_txid])
        assert_equal(bob.gettransaction(tx1_child_txid)["mempoolconflicts"],  [tx1_conflict_txid])

        # Now create a conflict to tx1_conflict, so that it gets kicked out of the mempool
        raw_tx = alice.createrawtransaction(inputs=[unspents[1]], outputs=[{carol.getnewaddress() : 24.9895}])
        tx1_conflict_conflict = alice.signrawtransactionwithwallet(raw_tx)['hex']
        tx1_conflict_conflict_txid = alice.sendrawtransaction(tx1_conflict_conflict)

        self.sync_mempools()

        # Now that tx1_conflict has been removed, both tx1 and tx1_child
        assert_equal(bob.gettransaction(tx1_txid)["mempoolconflicts"],  [])
        assert_equal(bob.gettransaction(tx1_child_txid)["mempoolconflicts"],  [])

        # Both tx1 and tx1_child are still not in the mempool because they have not be re-broadcasted
        assert tx1_txid not in bob.getrawmempool()
        assert tx1_child_txid not in bob.getrawmempool()
        assert tx1_conflict_txid not in bob.getrawmempool()
        assert tx1_conflict_conflict_txid in bob.getrawmempool()
        assert_equal(len(bob.getrawmempool()), 1)

        assert_equal(alice.getbalance(), 0)
        assert_equal(bob.getbalances()["mine"]["untrusted_pending"], 0)
        assert_equal(carol.getbalances()["mine"]["untrusted_pending"], Decimal("24.98950000"))

        # Both tx1 and tx1_child can now be re-broadcasted
        bob.sendrawtransaction(tx1)
        bob.sendrawtransaction(tx1_child)
        assert_equal(len(bob.getrawmempool()), 3)

        alice.unloadwallet()
        bob.unloadwallet()
        carol.unloadwallet()

    def test_unknown_parent_conflict(self):
        self.log.info("Test that a conflict with parent not belonging to the wallet causes in-wallet children to also conflict")

        self.nodes[0].createwallet("parent_conflict")
        self.nodes[1].createwallet("external_wallet_conflict")
        wallet = self.nodes[0].get_wallet_rpc("parent_conflict")
        external_wallet = self.nodes[1].get_wallet_rpc("external_wallet_conflict")
        def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        child_txids = []
        for current_wallet in [wallet, external_wallet]:
            # one utxo to target wallet
            addr = current_wallet.getnewaddress()
            addr_desc = current_wallet.getaddressinfo(addr)["desc"]
            def_wallet.sendtoaddress(addr, 10)

            self.generate(self.nodes[0], 1)

            utxo = current_wallet.listunspent()[0]

            # Make utxo in grandparent that will be double spent
            gp_addr = def_wallet.getnewaddress()
            gp_txid = def_wallet.sendtoaddress(gp_addr, 7)
            gp_utxo = {"txid": gp_txid, "vout": find_vout_for_address(self.nodes[0], gp_txid, gp_addr)}

            self.generate(self.nodes[0], 1)
            assert_equal(self.nodes[0].getrawmempool(), [])
            assert_equal(self.nodes[1].getrawmempool(), [])

            # make unconfirmed parent tx
            parent_addr = def_wallet.getnewaddress()
            parent_txid = def_wallet.send(outputs=[{parent_addr: 5}], inputs=[gp_utxo])["txid"]
            parent_utxo = {"txid": parent_txid, "vout": find_vout_for_address(self.nodes[0], parent_txid, parent_addr)}
            parent_me = self.nodes[0].getmempoolentry(parent_txid)
            parent_feerate = parent_me["fees"]["base"] * COIN / parent_me["vsize"]
            self.nodes[1].sendrawtransaction(def_wallet.gettransaction(parent_txid)["hex"])

            assert_equal(self.nodes[0].getrawmempool(), [parent_txid])
            assert_equal(self.nodes[1].getrawmempool(), [parent_txid])

            # make child tx that has one input belonging to target wallet, and one input not
            psbt = def_wallet.walletcreatefundedpsbt(inputs=[utxo, parent_utxo], outputs=[{def_wallet.getnewaddress(): 13}], solving_data={"descriptors":[addr_desc]})["psbt"]
            psbt = def_wallet.walletprocesspsbt(psbt)["psbt"]
            psbt_proc = current_wallet.walletprocesspsbt(psbt)
            psbt = psbt_proc["psbt"]
            assert_equal(psbt_proc["complete"], True)
            child_txid = self.nodes[0].sendrawtransaction(psbt_proc["hex"])
            self.nodes[1].sendrawtransaction(psbt_proc["hex"])
            child_txids.append(child_txid)

            assert_equal(set(self.nodes[0].getrawmempool()), {child_txid, parent_txid})
            assert_equal(set(self.nodes[1].getrawmempool()), {child_txid, parent_txid})

            assert_equal(def_wallet.gettransaction(parent_txid)["confirmations"], 0)
            assert_equal(current_wallet.gettransaction(child_txid)["confirmations"], 0)

            # Make a conflict spending parent
            conflict_psbt = def_wallet.walletcreatefundedpsbt(inputs=[gp_utxo], outputs=[{def_wallet.getnewaddress(): 2}], fee_rate="{0:.8}".format(Decimal(parent_feerate*3)))["psbt"]
            conflict_proc = def_wallet.walletprocesspsbt(conflict_psbt)
            assert_equal(conflict_proc["complete"], True)
            # Only send conflict to node 0
            conflict_txid = self.nodes[0].sendrawtransaction(conflict_proc["hex"])
            assert_equal(set(self.nodes[0].getrawmempool()), {conflict_txid})

            self.generate(self.nodes[0], 1, sync_fun=self.no_op)
            assert_equal(def_wallet.gettransaction(parent_txid)["confirmations"], -1)

        assert_equal(wallet.gettransaction(child_txids[0])["confirmations"], -4)

        # Sync Blocks so that node 1 gets conflicted block
        self.sync_blocks([self.nodes[0], self.nodes[1]])
        assert_equal(external_wallet.gettransaction(child_txids[1])["confirmations"], -1)
        external_wallet.unloadwallet()

if __name__ == '__main__':
    TxConflicts().main()
