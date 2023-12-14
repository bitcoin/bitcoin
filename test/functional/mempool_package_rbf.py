#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.messages import (
    COIN,
    MAX_BIP125_RBF_SEQUENCE,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
    assert_equal,
    assert_greater_than,
    create_lots_of_big_transactions,
    gen_return_txouts,
)
from test_framework.wallet import (
    DEFAULT_FEE,
    MiniWallet,
)

class PackageRBFTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        # Required for fill_mempool()
        self.extra_args = [[
            "-datacarriersize=100000",
            "-maxmempool=5",
        ]]

    def fill_mempool(self):
        """Fill mempool until eviction."""
        self.log.info("Fill the mempool until eviction is triggered and the mempoolminfee rises")
        txouts = gen_return_txouts()
        node = self.nodes[0]
        miniwallet = self.wallet
        relayfee = node.getnetworkinfo()['relayfee']

        tx_batch_size = 1
        num_of_batches = 75
        # Generate UTXOs to flood the mempool
        # 1 to create a tx initially that will be evicted from the mempool later
        # 75 transactions each with a fee rate higher than the previous one
        # And 1 more to verify that this tx does not get added to the mempool with a fee rate less than the mempoolminfee
        # And 2 more for the package cpfp test
        self.generate(miniwallet, 1 + (num_of_batches * tx_batch_size))

        # Mine 99 blocks so that the UTXOs are allowed to be spent
        self.generate(node, COINBASE_MATURITY - 1)

        self.log.debug("Create a mempool tx that will be evicted")
        tx_to_be_evicted_id = miniwallet.send_self_transfer(from_node=node, fee_rate=relayfee)["txid"]

        # Increase the tx fee rate to give the subsequent transactions a higher priority in the mempool
        # The tx has an approx. vsize of 65k, i.e. multiplying the previous fee rate (in sats/kvB)
        # by 130 should result in a fee that corresponds to 2x of that fee rate
        base_fee = relayfee * 130

        self.log.debug("Fill up the mempool with txs with higher fee rate")
        with node.assert_debug_log(["rolling minimum fee bumped"]):
            for batch_of_txid in range(num_of_batches):
                fee = (batch_of_txid + 1) * base_fee
                create_lots_of_big_transactions(miniwallet, node, fee, tx_batch_size, txouts)

        self.log.debug("The tx should be evicted by now")
        # The number of transactions created should be greater than the ones present in the mempool
        assert_greater_than(tx_batch_size * num_of_batches, len(node.getrawmempool()))
        # Initial tx created should not be present in the mempool anymore as it had a lower fee rate
        assert tx_to_be_evicted_id not in node.getrawmempool()

        self.log.debug("Check that mempoolminfee is larger than minrelaytxfee")
        assert_equal(node.getmempoolinfo()['minrelaytxfee'], Decimal('0.00001000'))
        assert_greater_than(node.getmempoolinfo()['mempoolminfee'], Decimal('0.00001000'))

    def assert_mempool_contents(self, expected=None, unexpected=None):
        """Assert that all transactions in expected are in the mempool,
        and all transactions in unexpected are not in the mempool.
        """
        if not expected:
            expected = []
        if not unexpected:
            unexpected = []
        assert set(unexpected).isdisjoint(expected)
        mempool = self.nodes[0].getrawmempool(verbose=False)
        for tx in expected:
            assert tx.rehash() in mempool
        for tx in unexpected:
            assert tx.rehash() not in mempool

    def create_simple_package(self, parent_coin, parent_fee=DEFAULT_FEE, child_fee=DEFAULT_FEE, heavy_child=False):
        """Create a 1 parent 1 child package using the coin passed in as the parent's input. The
        parent has 1 output, used to fund 1 child transaction.
        All transactions signal BIP125 replaceability, but nSequence changes based on self.ctr. This
        prevents identical txids between packages when the parents spend the same coin and have the
        same fee (i.e. 0sat).

        returns tuple (hex serialized txns, CTransaction objects)
        """
        self.ctr += 1
        # Use fee_rate=0 because create_self_transfer will use the default fee_rate value otherwise.
        # Passing in fee>0 overrides fee_rate, so this still works for non-zero parent_fee.
        parent_result = self.wallet.create_self_transfer(
            fee_rate=0,
            fee=parent_fee,
            utxo_to_spend=parent_coin,
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        num_child_outputs = 10 if heavy_child else 1
        child_result = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[parent_result["new_utxo"]],
            num_outputs=num_child_outputs,
            fee_per_output=int(child_fee * COIN // num_child_outputs),
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )
        package_hex = [parent_result["hex"], child_result["hex"]]
        package_txns = [parent_result["tx"], child_result["tx"]]
        return package_hex, package_txns

    def run_test(self):
        # Counter used to count the number of times we constructed packages. Since we're constructing parent transactions with the same
        # coins (to create conflicts), and giving them the same fee (i.e. 0, since their respective children are paying), we might
        # accidentally just create the exact same transaction again. To prevent this, set nSequences to MAX_BIP125_RBF_SEQUENCE - self.ctr.
        self.ctr = 0

        self.log.info("Generate blocks to create UTXOs")
        node = self.nodes[0]
        self.wallet = MiniWallet(node)
        self.generate(self.wallet, 160)
        self.coins = self.wallet.get_utxos(mark_as_spent=False)
        # Mature coinbase transactions
        self.generate(self.wallet, 100)
        self.address = self.wallet.get_address()

        self.test_package_rbf_basic()
        self.test_package_rbf_additional_fees()
        self.test_package_rbf_max_conflicts()
        self.test_package_rbf_conflicting_conflicts()
        self.test_package_rbf_partial()
        self.test_too_numerous_ancestors()
        self.test_too_numerous_pkg()
        self.test_insufficient_feerate()
        self.test_wrong_conflict_cluster_size_linear()
        self.test_wrong_conflict_cluster_size_parents_child()
        self.test_wrong_conflict_cluster_size_parent_children()
        self.test_child_conflicts_parent_mempool_ancestor()

    def test_package_rbf_basic(self):
        self.log.info("Test that a child can pay to replace its parents' conflicts")
        node = self.nodes[0]
        # Reuse the same coins so that the transactions conflict with one another.
        parent_coin = self.coins.pop()
        package_hex1, package_txns1 = self.create_simple_package(parent_coin, DEFAULT_FEE, DEFAULT_FEE)
        package_hex2, package_txns2 = self.create_simple_package(parent_coin, DEFAULT_FEE, DEFAULT_FEE * 5)
        node.submitpackage(package_hex1)
        self.assert_mempool_contents(expected=package_txns1, unexpected=package_txns2)

        submitres = node.submitpackage(package_hex2)
        submitres["replaced-transactions"] == [tx.rehash() for tx in package_txns1]
        self.assert_mempool_contents(expected=package_txns2, unexpected=package_txns1)
        self.generate(node, 1)

    def test_package_rbf_additional_fees(self):
        self.log.info("Check Package RBF must increase the absolute fee")
        node = self.nodes[0]
        coin = self.coins.pop()

        # avoid parent pays for child anti-DoS checks
        fee_delta = DEFAULT_FEE / 2

        package_hex1, package_txns1 = self.create_simple_package(coin, parent_fee=DEFAULT_FEE - fee_delta, child_fee=DEFAULT_FEE + fee_delta, heavy_child=True)
        assert_greater_than_or_equal(1000, package_txns1[-1].get_vsize())
        node.submitpackage(package_hex1)
        self.assert_mempool_contents(expected=package_txns1)

        # Package 2 has a higher feerate but lower absolute fee (which diagram check covers)
        package_hex2, package_txns2 = self.create_simple_package(coin, parent_fee=DEFAULT_FEE - fee_delta, child_fee=DEFAULT_FEE + fee_delta - Decimal("0.00000001"))
        pkg_results2 = node.submitpackage(package_hex2)
        assert_equal(pkg_results2["package_msg"], 'package RBF failed: insufficient feerate: does not improve feerate diagram')
        self.assert_mempool_contents(expected=package_txns1, unexpected=package_txns2)
        # Package 3 has a higher feerate and absolute fee
        package_hex3, package_txns3 = self.create_simple_package(coin, parent_fee=DEFAULT_FEE, child_fee=DEFAULT_FEE * 3)
        node.submitpackage(package_hex3)
        self.assert_mempool_contents(expected=package_txns3, unexpected=package_txns1 + package_txns2)
        self.generate(node, 1)

        self.log.info("Check Package RBF must pay for the entire package's bandwidth")
        coin = self.coins.pop()
        package_hex4, package_txns4 = self.create_simple_package(coin, parent_fee=DEFAULT_FEE, child_fee=DEFAULT_FEE)
        node.submitpackage(package_hex4)
        self.assert_mempool_contents(expected=package_txns4, unexpected=[])
        package_hex5, package_txns5 = self.create_simple_package(coin, parent_fee=DEFAULT_FEE - fee_delta, child_fee=DEFAULT_FEE + fee_delta + Decimal("0.00000001"))
        pkg_results5 = node.submitpackage(package_hex5)
        assert_equal(f"package RBF failed: insufficient anti-DoS fees, rejecting replacement {package_txns5[1].rehash()}, not enough additional fees to relay; 0.00000001 < 0.00000{sum([tx.get_vsize() for tx in package_txns5])}", pkg_results5["package_msg"])


        self.assert_mempool_contents(expected=package_txns4, unexpected=package_txns5)
        self.generate(node, 1)

        self.log.info("Check Package RBF must have strict cpfp structure")
        coin = self.coins.pop()
        package_hex5, package_txns5 = self.create_simple_package(coin, parent_fee=DEFAULT_FEE, child_fee=DEFAULT_FEE)
        node.submitpackage(package_hex5)
        self.assert_mempool_contents(expected=package_txns5, unexpected=[])
        package_hex6, package_txns6 = self.create_simple_package(coin, parent_fee=DEFAULT_FEE + fee_delta, child_fee=DEFAULT_FEE - (fee_delta / 2))
        pkg_results6 = node.submitpackage(package_hex6)
        assert_equal(pkg_results6["package_msg"], 'package RBF failed: parent paying for child anti-DoS')

        self.assert_mempool_contents(expected=package_txns5, unexpected=package_txns6)
        self.generate(node, 1)


    def test_package_rbf_max_conflicts(self):
        node = self.nodes[0]
        self.log.info("Check Package RBF cannot replace more than 100 transactions")
        num_coins = 5
        parent_coins = self.coins[:num_coins]
        del self.coins[:num_coins]
        # Original transactions: 5 transactions with 24 descendants each.
        for coin in parent_coins:
            self.wallet.send_self_transfer_chain(from_node=node, chain_length=25, utxo_to_spend=coin)

        # Replacement package: 1 parent which conflicts with 5 * (1 + 24) = 125 mempool transactions.
        package_parent = self.wallet.create_self_transfer_multi(utxos_to_spend=parent_coins)
        package_child = self.wallet.create_self_transfer(fee_rate=50*DEFAULT_FEE, utxo_to_spend=package_parent["new_utxos"][0])

        pkg_results = node.submitpackage([package_parent["hex"], package_child["hex"]])
        assert_equal(f"package RBF failed: too many potential replacements, rejecting replacement {package_parent['tx'].rehash()}; too many potential replacements (125 > 100)\n", pkg_results["package_msg"])
        self.generate(node, 1)

    def test_package_rbf_conflicting_conflicts(self):
        node = self.nodes[0]
        self.log.info("Check that different package transactions cannot share the same conflicts")
        coin = self.coins.pop()
        package_hex1, package_txns1 = self.create_simple_package(coin, DEFAULT_FEE, DEFAULT_FEE)
        package_hex2, package_txns2 = self.create_simple_package(coin, Decimal("0.00009"), DEFAULT_FEE * 2)
        package_hex3, package_txns3 = self.create_simple_package(coin, DEFAULT_FEE, DEFAULT_FEE * 5)
        node.submitpackage(package_hex1)
        self.assert_mempool_contents(expected=package_txns1)
        # The first two transactions have the same conflicts
        package_duplicate_conflicts_hex = [package_hex2[0]] + package_hex3
        # Note that this won't actually go into the RBF logic, because static package checks will
        # detect that two package transactions conflict with each other. Either way, this must fail.
        assert_raises_rpc_error(-25, "package topology disallowed", node.submitpackage, package_duplicate_conflicts_hex)
        self.assert_mempool_contents(expected=package_txns1, unexpected=package_txns2 + package_txns3)
        # The RBFs should otherwise work.
        submitres2 = node.submitpackage(package_hex2)
        assert_equal(sorted(submitres2["replaced-transactions"]),sorted( [tx.rehash() for tx in package_txns1]))
        self.assert_mempool_contents(expected=package_txns2, unexpected=package_txns1)
        submitres3 = node.submitpackage(package_hex3)
        assert_equal(sorted(submitres3["replaced-transactions"]), sorted([tx.rehash() for tx in package_txns2]))
        self.assert_mempool_contents(expected=package_txns3, unexpected=package_txns2)

    def test_package_rbf_partial(self):
        self.log.info("Test that package RBF works when a transaction was already submitted")
        node = self.nodes[0]
        coin = self.coins.pop()
        package_hex1, package_txns1 = self.create_simple_package(coin, DEFAULT_FEE, DEFAULT_FEE)
        package_hex2, package_txns2 = self.create_simple_package(coin, DEFAULT_FEE * 3, DEFAULT_FEE * 3)
        node.submitpackage(package_hex1)
        self.assert_mempool_contents(expected=package_txns1, unexpected=package_txns2)
        # Submit parent on its own. It should have no trouble replacing the previous
        # transaction(s) because the fee is tripled.
        node.sendrawtransaction(package_hex2[0])
        node.submitpackage(package_hex2)
        self.assert_mempool_contents(expected=package_txns2, unexpected=package_txns1)
        self.generate(node, 1)

    def test_too_numerous_ancestors(self):
        self.log.info("Test that package RBF doesn't work with packages larger than 2 due to ancestors")
        node = self.nodes[0]
        coin = self.coins.pop()
        package_hex1, package_txns1 = self.create_simple_package(coin, DEFAULT_FEE, DEFAULT_FEE)
        node.submitpackage(package_hex1)
        self.assert_mempool_contents(expected=package_txns1, unexpected=[])

        # Double-spends the original package
        self.ctr += 1
        parent_result1 = self.wallet.create_self_transfer(
            fee_rate=0,
            fee=DEFAULT_FEE,
            utxo_to_spend=coin,
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        coin2 = self.coins.pop()

        # Added to make package too large for package RBF;
        # it will enter mempool individually
        self.ctr += 1
        parent_result2 = self.wallet.create_self_transfer(
            fee_rate=0,
            fee=DEFAULT_FEE,
            utxo_to_spend=coin2,
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        # Child that spends both, violating cluster size rule due
        # to in-mempool ancestry
        self.ctr += 1
        child_result = self.wallet.create_self_transfer_multi(
            fee_per_output=int(DEFAULT_FEE * 5 * COIN),
            utxos_to_spend=[parent_result1["new_utxo"], parent_result2["new_utxo"]],
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        package_hex2 = [parent_result1["hex"], parent_result2["hex"], child_result["hex"]]
        package_txns2_fail = [parent_result1["tx"], child_result["tx"]]
        package_txns2_succeed = [parent_result2["tx"]]

        pkg_result = node.submitpackage(package_hex2)
        assert_equal(pkg_result["package_msg"], 'package RBF failed: replacing cluster with ancestors not size two')
        self.assert_mempool_contents(expected=package_txns1 + package_txns2_succeed, unexpected=package_txns2_fail)
        self.generate(node, 1)

    def test_wrong_conflict_cluster_size_linear(self):
        self.log.info("Test that conflicting with a cluster not sized two is rejected: linear chain")
        node = self.nodes[0]

        # Coins we will conflict with
        coin1 = self.coins.pop()
        coin2 = self.coins.pop()
        coin3 = self.coins.pop()

        # Three transactions chained; package RBF against any of these
        # should be rejected
        self.ctr += 1
        parent_result = self.wallet.create_self_transfer(
            fee_rate=0,
            fee=DEFAULT_FEE,
            utxo_to_spend=coin1,
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        self.ctr += 1
        child_result = self.wallet.create_self_transfer_multi(
            fee_per_output=int(DEFAULT_FEE * COIN),
            utxos_to_spend=[parent_result["new_utxo"], coin2],
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        self.ctr += 1
        grandchild_result = self.wallet.create_self_transfer_multi(
            fee_per_output=int(DEFAULT_FEE * COIN),
            utxos_to_spend=[child_result["new_utxos"][0], coin3],
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        node.sendrawtransaction(parent_result["tx"].serialize().hex())
        node.sendrawtransaction(child_result["tx"].serialize().hex())
        node.sendrawtransaction(grandchild_result["tx"].serialize().hex())
        assert_equal(len(node.getrawmempool()), 3)

        # Now make conflicting packages for each coin
        package_hex1, package_txns1 = self.create_simple_package(coin1, DEFAULT_FEE, DEFAULT_FEE + Decimal("0.00000001"))

        package_result = node.submitpackage(package_hex1)
        assert_equal(f"package RBF failed: {parent_result['tx'].rehash()} has 2 descendants, max 1 allowed", package_result["package_msg"])

        package_hex2, package_txns2 = self.create_simple_package(coin2, DEFAULT_FEE, DEFAULT_FEE + Decimal("0.00000001"))
        package_result = node.submitpackage(package_hex2)
        assert_equal(f"package RBF failed: {child_result['tx'].rehash()} has both ancestor and descendant, exceeding cluster limit of 2", package_result["package_msg"])

        package_hex3, package_txns3 = self.create_simple_package(coin3, DEFAULT_FEE, DEFAULT_FEE + Decimal("0.00000001"))
        package_result = node.submitpackage(package_hex3)
        assert_equal(f"package RBF failed: {grandchild_result['tx'].rehash()} has 2 ancestors, max 1 allowed", package_result["package_msg"])


        # Check that replacements were actually rejected
        self.assert_mempool_contents(expected=[], unexpected=package_txns1 + package_txns2 + package_txns3)
        self.generate(node, 1)

    def test_wrong_conflict_cluster_size_parents_child(self):
        self.log.info("Test that conflicting with a cluster not sized two is rejected: two parents one child")
        node = self.nodes[0]

        # Coins we will conflict with
        coin1 = self.coins.pop()
        coin2 = self.coins.pop()
        coin3 = self.coins.pop()

        self.ctr += 1
        parent1_result = self.wallet.create_self_transfer(
            fee_rate=0,
            fee=DEFAULT_FEE,
            utxo_to_spend=coin1,
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        self.ctr += 1
        parent2_result = self.wallet.create_self_transfer_multi(
            fee_per_output=int(DEFAULT_FEE * COIN),
            utxos_to_spend=[coin2],
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        self.ctr += 1
        child_result = self.wallet.create_self_transfer_multi(
            fee_per_output=int(DEFAULT_FEE * COIN),
            utxos_to_spend=[parent1_result["new_utxo"], parent2_result["new_utxos"][0], coin3],
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        node.sendrawtransaction(parent1_result["tx"].serialize().hex())
        node.sendrawtransaction(parent2_result["tx"].serialize().hex())
        node.sendrawtransaction(child_result["tx"].serialize().hex())
        assert_equal(len(node.getrawmempool()), 3)

        # Now make conflicting packages for each coin
        package_hex1, package_txns1 = self.create_simple_package(coin1, DEFAULT_FEE, DEFAULT_FEE + Decimal("0.00000001"))
        package_result = node.submitpackage(package_hex1)
        assert_equal(f"package RBF failed: {parent1_result['tx'].rehash()} is not the only parent of child {child_result['tx'].rehash()}", package_result["package_msg"])

        package_hex2, package_txns2 = self.create_simple_package(coin2, DEFAULT_FEE, DEFAULT_FEE + Decimal("0.00000001"))
        package_result = node.submitpackage(package_hex2)
        assert_equal(f"package RBF failed: {parent2_result['tx'].rehash()} is not the only parent of child {child_result['tx'].rehash()}", package_result["package_msg"])

        package_hex3, package_txns3 = self.create_simple_package(coin3, DEFAULT_FEE, DEFAULT_FEE + Decimal("0.00000001"))
        package_result = node.submitpackage(package_hex3)
        assert_equal(f"package RBF failed: {child_result['tx'].rehash()} has 2 ancestors, max 1 allowed", package_result["package_msg"])

        # Check that replacements were actually rejected
        self.assert_mempool_contents(expected=[], unexpected=package_txns1 + package_txns2 + package_txns3)
        self.generate(node, 1)

    def test_wrong_conflict_cluster_size_parent_children(self):
        self.log.info("Test that conflicting with a cluster not sized two is rejected: one parent two children")
        node = self.nodes[0]

        # Coins we will conflict with
        coin1 = self.coins.pop()
        coin2 = self.coins.pop()
        coin3 = self.coins.pop()

        self.ctr += 1
        parent_result = self.wallet.create_self_transfer_multi(
            fee_per_output=int(DEFAULT_FEE * COIN),
            num_outputs=2,
            utxos_to_spend=[coin1],
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        self.ctr += 1
        child1_result = self.wallet.create_self_transfer_multi(
            fee_per_output=int(DEFAULT_FEE * COIN),
            utxos_to_spend=[parent_result["new_utxos"][0], coin2],
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        self.ctr += 1
        child2_result = self.wallet.create_self_transfer_multi(
            fee_per_output=int(DEFAULT_FEE * COIN),
            utxos_to_spend=[parent_result["new_utxos"][1], coin3],
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        node.sendrawtransaction(parent_result["tx"].serialize().hex())
        node.sendrawtransaction(child1_result["tx"].serialize().hex())
        node.sendrawtransaction(child2_result["tx"].serialize().hex())
        assert_equal(len(node.getrawmempool()), 3)

        # Now make conflicting packages for each coin
        package_hex1, package_txns1 = self.create_simple_package(coin1, DEFAULT_FEE, DEFAULT_FEE + Decimal("0.00000001"))
        package_result = node.submitpackage(package_hex1)
        assert_equal(f"package RBF failed: {parent_result['tx'].rehash()} has 2 descendants, max 1 allowed", package_result["package_msg"])

        package_hex2, package_txns2 = self.create_simple_package(coin2, DEFAULT_FEE, DEFAULT_FEE + Decimal("0.00000001"))
        package_result = node.submitpackage(package_hex2)
        assert_equal(f"package RBF failed: {child1_result['tx'].rehash()} is not the only child of parent {parent_result['tx'].rehash()}", package_result["package_msg"])

        package_hex3, package_txns3 = self.create_simple_package(coin3, DEFAULT_FEE, DEFAULT_FEE + Decimal("0.00000001"))
        package_result = node.submitpackage(package_hex3)
        assert_equal(f"package RBF failed: {child2_result['tx'].rehash()} is not the only child of parent {parent_result['tx'].rehash()}", package_result["package_msg"])

        # Check that replacements were actually rejected
        self.assert_mempool_contents(expected=[], unexpected=package_txns1 + package_txns2 + package_txns3)
        self.generate(node, 1)

    def test_too_numerous_pkg(self):
        self.log.info("Test that package RBF doesn't work with packages larger than 2 due to pkg size")
        node = self.nodes[0]
        coin1 = self.coins.pop()
        coin2 = self.coins.pop()

        # Two packages to require multiple direct conflicts, easier to set up illicit pkg size
        package_hex1, package_txns1 = self.create_simple_package(coin1, DEFAULT_FEE, DEFAULT_FEE)
        package_hex2, package_txns2 = self.create_simple_package(coin2, DEFAULT_FEE, DEFAULT_FEE)

        node.submitpackage(package_hex1)
        node.submitpackage(package_hex2)

        self.assert_mempool_contents(expected=package_txns1 + package_txns2, unexpected=[])
        assert_equal(len(node.getrawmempool()), 4)

        # Double-spends the first package
        self.ctr += 1
        parent_result1 = self.wallet.create_self_transfer(
            fee_rate=0,
            fee=DEFAULT_FEE,
            utxo_to_spend=coin1,
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        # Double-spends the second package
        self.ctr += 1
        parent_result2 = self.wallet.create_self_transfer(
            fee_rate=0,
            fee=DEFAULT_FEE,
            utxo_to_spend=coin2,
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        # Child that spends both, violating cluster size rule due
        # to pkg size
        self.ctr += 1
        child_result = self.wallet.create_self_transfer_multi(
            fee_per_output=int(DEFAULT_FEE * 5 * COIN),
            utxos_to_spend=[parent_result1["new_utxo"], parent_result2["new_utxo"]],
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        package_hex3 = [parent_result1["hex"], parent_result2["hex"], child_result["hex"]]
        package_txns3_fail = [parent_result1["tx"], parent_result2["tx"], child_result["tx"]]

        pkg_result = node.submitpackage(package_hex3)
        assert_equal(pkg_result["package_msg"], 'package RBF failed: replacing cluster not size two')
        self.assert_mempool_contents(expected=package_txns1, unexpected=package_txns3_fail)
        self.generate(node, 1)

    def test_insufficient_feerate(self):
        self.log.info("Check Package RBF must beat feerate of direct conflict")
        node = self.nodes[0]
        coin = self.coins.pop()

        # avoid parent pays for child anti-DoS checks
        fee_delta = DEFAULT_FEE / 2

        package_hex1, package_txns1 = self.create_simple_package(coin, parent_fee=DEFAULT_FEE + fee_delta, child_fee=DEFAULT_FEE - fee_delta)
        node.submitpackage(package_hex1)
        self.assert_mempool_contents(expected=package_txns1)

        # Package 2 feerate is below the rate of directly conflicted parent, even though
        # total fees are higher than the original package
        package_hex2, package_txns2 = self.create_simple_package(coin, parent_fee=DEFAULT_FEE, child_fee=DEFAULT_FEE + fee_delta)
        pkg_results2 = node.submitpackage(package_hex2)
        assert_equal(pkg_results2["package_msg"], 'package RBF failed: insufficient feerate: does not improve feerate diagram')
        self.assert_mempool_contents(expected=package_txns1, unexpected=package_txns2)
        self.generate(node, 1)

    def test_child_conflicts_parent_mempool_ancestor(self):
        self.fill_mempool()
        # Reset coins since we filled the mempool with current coins
        self.coins = self.wallet.get_utxos(mark_as_spent=False, confirmed_only=True)

        self.log.info("Test that package RBF doesn't have issues with mempool<->package conflicts via inconsistency")
        node = self.nodes[0]
        coin = self.coins.pop()

        # Put simple tx in mempool to chain off of
        self.ctr += 1
        grandparent_result = self.wallet.create_self_transfer(
            fee_rate=0,
            fee=DEFAULT_FEE,
            utxo_to_spend=coin,
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        node.sendrawtransaction(grandparent_result["hex"])

        # Now make package of two descendants that looks
        # like a cpfp where the parent can't get in on its own
        assert_greater_than(node.getmempoolinfo()["mempoolminfee"], Decimal('0.00001000'))

        self.ctr += 1
        parent_result = self.wallet.create_self_transfer(
            fee_rate=Decimal('0.00001000'),
            utxo_to_spend=grandparent_result["new_utxo"],
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )
        # Last tx double-spends grandparent's coin,
        # which is not inside the current package
        self.ctr += 1
        child_result = self.wallet.create_self_transfer_multi(
            fee_per_output=int(DEFAULT_FEE * 5 * COIN),
            utxos_to_spend=[parent_result["new_utxo"], coin],
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        pkg_result = node.submitpackage([parent_result["hex"], child_result["hex"]])
        assert_equal(pkg_result["package_msg"], 'package RBF failed: replacing cluster with ancestors not size two')
        mempool_info = node.getrawmempool()
        assert grandparent_result["txid"] in mempool_info
        assert parent_result["txid"] not in mempool_info
        assert child_result["txid"] not in mempool_info
        self.generate(node, 1)

if __name__ == "__main__":
    PackageRBFTest().main()
