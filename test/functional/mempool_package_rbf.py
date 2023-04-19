#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from decimal import Decimal

from test_framework.messages import (
    COIN,
    MAX_BIP125_RBF_SEQUENCE,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.mempool_util import fill_mempool
from test_framework.util import (
    assert_greater_than_or_equal,
    assert_equal,
)
from test_framework.wallet import (
    DEFAULT_FEE,
    MiniWallet,
)

MAX_REPLACEMENT_CANDIDATES = 100

# Value high enough to cause evictions in each subtest
# for typical cases
DEFAULT_CHILD_FEE = DEFAULT_FEE * 4

class PackageRBFTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        # Required for fill_mempool()
        self.extra_args = [[
            "-datacarriersize=100000",
            "-maxmempool=5",
        ]] * self.num_nodes

    def assert_mempool_contents(self, expected=None):
        """Assert that all transactions in expected are in the mempool,
        and no additional ones exist.
        """
        if not expected:
            expected = []
        mempool = self.nodes[0].getrawmempool(verbose=False)
        assert_equal(len(mempool), len(expected))
        for tx in expected:
            assert tx.rehash() in mempool

    def create_simple_package(self, parent_coin, parent_fee=DEFAULT_FEE, child_fee=DEFAULT_CHILD_FEE, heavy_child=False):
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
        # coins (to create conflicts), and perhaps giving them the same fee, we might accidentally just create the same transaction again.
        # To prevent this, set nSequences to MAX_BIP125_RBF_SEQUENCE - self.ctr.
        self.ctr = 0

        self.log.info("Generate blocks to create UTXOs")
        self.wallet = MiniWallet(self.nodes[0])

        # Make more than enough coins for the sum of all tests,
        # otherwise a wallet rescan is needed later
        self.generate(self.wallet, 300)
        self.coins = self.wallet.get_utxos(mark_as_spent=False)

        self.test_package_rbf_basic()
        self.test_package_rbf_singleton()
        self.test_package_rbf_additional_fees()
        self.test_package_rbf_max_conflicts()
        self.test_too_numerous_ancestors()
        self.test_package_rbf_with_wrong_pkg_size()
        self.test_insufficient_feerate()
        self.test_0fee_package_rbf()
        self.test_child_conflicts_parent_mempool_ancestor()

    def test_package_rbf_basic(self):
        self.log.info("Test that a child can pay to replace its parents' conflicts of cluster size 2")
        node = self.nodes[0]
        # Reuse the same coins so that the transactions conflict with one another.
        parent_coin = self.coins.pop()
        package_hex1, package_txns1 = self.create_simple_package(parent_coin, DEFAULT_FEE, DEFAULT_FEE)
        package_hex2, package_txns2 = self.create_simple_package(parent_coin, DEFAULT_FEE, DEFAULT_CHILD_FEE)
        node.submitpackage(package_hex1)
        self.assert_mempool_contents(expected=package_txns1)

        # Make sure 2nd node gets set up for basic package RBF
        self.sync_all()

        # Test run rejected because conflicts are not allowed in subpackage evaluation
        testres = node.testmempoolaccept(package_hex2)
        assert_equal(testres[0]["reject-reason"], "bip125-replacement-disallowed")

        # But accepted during normal submission
        submitres = node.submitpackage(package_hex2)
        assert_equal(set(submitres["replaced-transactions"]), set([tx.rehash() for tx in package_txns1]))
        self.assert_mempool_contents(expected=package_txns2)

        # Make sure 2nd node gets a basic package RBF over p2p
        self.sync_all()

        self.generate(node, 1)

    def test_package_rbf_singleton(self):
        self.log.info("Test child can pay to replace a parent's single conflicted tx")
        node = self.nodes[0]

        # Make singleton tx to conflict with in next batch
        singleton_coin = self.coins.pop()
        singleton_tx = self.wallet.create_self_transfer(utxo_to_spend=singleton_coin)
        node.sendrawtransaction(singleton_tx["hex"])
        self.assert_mempool_contents(expected=[singleton_tx["tx"]])

        package_hex, package_txns = self.create_simple_package(singleton_coin, DEFAULT_FEE, singleton_tx["fee"] * 2)

        submitres = node.submitpackage(package_hex)
        assert_equal(submitres["replaced-transactions"], [singleton_tx["tx"].rehash()])
        self.assert_mempool_contents(expected=package_txns)

        self.generate(node, 1)

    def test_package_rbf_additional_fees(self):
        self.log.info("Check Package RBF must increase the absolute fee")
        node = self.nodes[0]
        coin = self.coins.pop()

        package_hex1, package_txns1 = self.create_simple_package(coin, parent_fee=DEFAULT_FEE, child_fee=DEFAULT_CHILD_FEE, heavy_child=True)
        assert_greater_than_or_equal(1000, package_txns1[-1].get_vsize())
        node.submitpackage(package_hex1)
        self.assert_mempool_contents(expected=package_txns1)

        PACKAGE_FEE = DEFAULT_FEE + DEFAULT_CHILD_FEE
        PACKAGE_FEE_MINUS_ONE = PACKAGE_FEE - Decimal("0.00000001")

        # Package 2 has a higher feerate but lower absolute fee
        package_hex2, package_txns2 = self.create_simple_package(coin, parent_fee=DEFAULT_FEE, child_fee=DEFAULT_CHILD_FEE - Decimal("0.00000001"))
        pkg_results2 = node.submitpackage(package_hex2)
        assert_equal(f"package RBF failed: insufficient anti-DoS fees, rejecting replacement {package_txns2[1].rehash()}, less fees than conflicting txs; {PACKAGE_FEE_MINUS_ONE} < {PACKAGE_FEE}", pkg_results2["package_msg"])
        self.assert_mempool_contents(expected=package_txns1)

        self.log.info("Check replacement pays for incremental bandwidth")
        _, placeholder_txns3 = self.create_simple_package(coin)
        package_3_size = sum([tx.get_vsize() for tx in placeholder_txns3])
        incremental_sats_required = Decimal(package_3_size) / COIN
        incremental_sats_short = incremental_sats_required - Decimal("0.00000001")
        # Recreate the package with slightly higher fee once we know the size of the new package, but still short of required fee
        failure_package_hex3, failure_package_txns3 = self.create_simple_package(coin, parent_fee=DEFAULT_FEE, child_fee=DEFAULT_CHILD_FEE + incremental_sats_short)
        assert_equal(package_3_size, sum([tx.get_vsize() for tx in failure_package_txns3]))
        pkg_results3 = node.submitpackage(failure_package_hex3)
        assert_equal(f"package RBF failed: insufficient anti-DoS fees, rejecting replacement {failure_package_txns3[1].rehash()}, not enough additional fees to relay; {incremental_sats_short} < {incremental_sats_required}", pkg_results3["package_msg"])
        self.assert_mempool_contents(expected=package_txns1)

        success_package_hex3, success_package_txns3 = self.create_simple_package(coin, parent_fee=DEFAULT_FEE, child_fee=DEFAULT_CHILD_FEE + incremental_sats_required)
        node.submitpackage(success_package_hex3)
        self.assert_mempool_contents(expected=success_package_txns3)
        self.generate(node, 1)

        self.log.info("Check Package RBF must have strict cpfp structure")
        coin = self.coins.pop()
        package_hex4, package_txns4 = self.create_simple_package(coin, parent_fee=DEFAULT_FEE, child_fee=DEFAULT_CHILD_FEE)
        node.submitpackage(package_hex4)
        self.assert_mempool_contents(expected=package_txns4)
        package_hex5, _package_txns5 = self.create_simple_package(coin, parent_fee=DEFAULT_CHILD_FEE, child_fee=DEFAULT_CHILD_FEE)
        pkg_results5 = node.submitpackage(package_hex5)
        assert 'package RBF failed: package feerate is less than or equal to parent feerate' in pkg_results5["package_msg"]
        self.assert_mempool_contents(expected=package_txns4)

        package_hex5_1, package_txns5_1 = self.create_simple_package(coin, parent_fee=DEFAULT_CHILD_FEE, child_fee=DEFAULT_CHILD_FEE + Decimal("0.00000001"))
        node.submitpackage(package_hex5_1)
        self.assert_mempool_contents(expected=package_txns5_1)
        self.generate(node, 1)

    def test_package_rbf_max_conflicts(self):
        node = self.nodes[0]
        self.log.info("Check Package RBF cannot conflict with  more than MAX_REPLACEMENT_CANDIDATES clusters")
        num_coins = 101
        parent_coins = self.coins[:num_coins]
        del self.coins[:num_coins]

        # Original transactions: 101 transactions with 1 descendants each -> 202 total transactions, 101 clusters
        size_two_clusters = []
        for coin in parent_coins:
            size_two_clusters.append(self.wallet.send_self_transfer_chain(from_node=node, chain_length=2, utxo_to_spend=coin))
        expected_txns = [txn["tx"] for parent_child_txns in size_two_clusters for txn in parent_child_txns]
        assert_equal(len(expected_txns), num_coins * 2)
        self.assert_mempool_contents(expected=expected_txns)

        # 101 clusters
        clusters = set([])
        for txn in expected_txns:
            clusters.add(node.getmempoolentry(txn.rehash())["clusterid"])
        assert_equal(len(clusters), num_coins)

        # parent feeerate needs to be high enough for minrelay
        # child feerate needs to be large enough to trigger package rbf with a very large parent and
        # pay for all evicted fees. maxfeerate turned off for all submissions since child feerate
        # is extremely high
        parent_fee_per_conflict = 10000
        child_feerate = 10000 * DEFAULT_FEE

        # Conflict against all transactions by double-spending each parent, causing 101 cluster conflicts
        package_parent = self.wallet.create_self_transfer_multi(utxos_to_spend=parent_coins, fee_per_output=parent_fee_per_conflict)
        package_child = self.wallet.create_self_transfer(fee_rate=child_feerate, utxo_to_spend=package_parent["new_utxos"][0])

        pkg_results = node.submitpackage([package_parent["hex"], package_child["hex"]], maxfeerate=0)
        print(pkg_results["package_msg"])
        assert_equal(f"transaction failed", pkg_results["package_msg"])
        assert_equal(f"too many potential replacements, rejecting replacement {package_parent['txid']}; too many conflicting clusters (101 > 100)\n", pkg_results["tx-results"][package_parent["wtxid"]]["error"])
        self.assert_mempool_contents(expected=expected_txns)

        # Make singleton tx to conflict with in next batch
        singleton_coin = self.coins.pop()
        singleton_tx = self.wallet.create_self_transfer(utxo_to_spend=singleton_coin)
        node.sendrawtransaction(singleton_tx["hex"])
        expected_txns.append(singleton_tx["tx"])

        # Double-spend same set minus last, and double-spend singleton. This is still too many conflicted clusters.
        # N.B. we can't RBF just a child tx in the clusters, as that would make resulting cluster of size 3.
        double_spending_coins = parent_coins[:-1] + [singleton_coin]
        package_parent = self.wallet.create_self_transfer_multi(utxos_to_spend=double_spending_coins, fee_per_output=parent_fee_per_conflict)
        package_child = self.wallet.create_self_transfer(fee_rate=child_feerate, utxo_to_spend=package_parent["new_utxos"][0])
        pkg_results = node.submitpackage([package_parent["hex"], package_child["hex"]], maxfeerate=0)
        assert_equal(f"transaction failed", pkg_results["package_msg"])
        assert_equal(f"too many potential replacements, rejecting replacement {package_parent['txid']}; too many conflicting clusters (101 > 100)\n", pkg_results["tx-results"][package_parent["wtxid"]]["error"])
        self.assert_mempool_contents(expected=expected_txns)

        # Finally, conflict with MAX_REPLACEMENT_CANDIDATES clusters
        package_parent = self.wallet.create_self_transfer_multi(utxos_to_spend=parent_coins[:-1], fee_per_output=parent_fee_per_conflict)
        package_child = self.wallet.create_self_transfer(fee_rate=child_feerate, utxo_to_spend=package_parent["new_utxos"][0])
        pkg_results = node.submitpackage([package_parent["hex"], package_child["hex"]], maxfeerate=0)
        assert_equal(pkg_results["package_msg"], "success")
        self.assert_mempool_contents(expected=[singleton_tx["tx"], size_two_clusters[-1][0]["tx"], size_two_clusters[-1][1]["tx"], package_parent["tx"], package_child["tx"]] )

        self.generate(node, 1)

    def test_too_numerous_ancestors(self):
        self.log.info("Test that package RBF doesn't work with packages larger than 2 due to ancestors")
        node = self.nodes[0]
        coin = self.coins.pop()

        package_hex1, package_txns1 = self.create_simple_package(coin, DEFAULT_FEE, DEFAULT_CHILD_FEE)
        node.submitpackage(package_hex1)
        self.assert_mempool_contents(expected=package_txns1)

        # Double-spends the original package
        self.ctr += 1
        parent_result1 = self.wallet.create_self_transfer(
            fee=DEFAULT_FEE,
            utxo_to_spend=coin,
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        coin2 = self.coins.pop()

        # Added to make package too large for package RBF;
        # it will enter mempool individually
        self.ctr += 1
        parent_result2 = self.wallet.create_self_transfer(
            fee=DEFAULT_FEE,
            utxo_to_spend=coin2,
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        # Child that spends both, violating cluster size rule due
        # to in-mempool ancestry
        self.ctr += 1
        child_result = self.wallet.create_self_transfer_multi(
            fee_per_output=int(DEFAULT_CHILD_FEE * COIN),
            utxos_to_spend=[parent_result1["new_utxo"], parent_result2["new_utxo"]],
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        package_hex2 = [parent_result1["hex"], parent_result2["hex"], child_result["hex"]]
        package_txns2_succeed = [parent_result2["tx"]]

        pkg_result = node.submitpackage(package_hex2)
        assert_equal(pkg_result["package_msg"], 'package RBF failed: new transaction cannot have mempool ancestors')
        self.assert_mempool_contents(expected=package_txns1 + package_txns2_succeed)
        self.generate(node, 1)

    def test_package_rbf_with_wrong_pkg_size(self):
        self.log.info("Test that package RBF doesn't work with packages larger than 2 due to pkg size")
        node = self.nodes[0]
        coin1 = self.coins.pop()
        coin2 = self.coins.pop()

        # Two packages to require multiple direct conflicts, easier to set up illicit pkg size
        package_hex1, package_txns1 = self.create_simple_package(coin1, DEFAULT_FEE, DEFAULT_CHILD_FEE)
        package_hex2, package_txns2 = self.create_simple_package(coin2, DEFAULT_FEE, DEFAULT_CHILD_FEE)

        node.submitpackage(package_hex1)
        node.submitpackage(package_hex2)

        self.assert_mempool_contents(expected=package_txns1 + package_txns2)
        assert_equal(len(node.getrawmempool()), 4)

        # Double-spends the first package
        self.ctr += 1
        parent_result1 = self.wallet.create_self_transfer(
            fee=DEFAULT_FEE,
            utxo_to_spend=coin1,
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        # Double-spends the second package
        self.ctr += 1
        parent_result2 = self.wallet.create_self_transfer(
            fee=DEFAULT_FEE,
            utxo_to_spend=coin2,
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        # Child that spends both, violating cluster size rule due
        # to pkg size
        self.ctr += 1
        child_result = self.wallet.create_self_transfer_multi(
            fee_per_output=int(DEFAULT_CHILD_FEE * COIN),
            utxos_to_spend=[parent_result1["new_utxo"], parent_result2["new_utxo"]],
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        package_hex3 = [parent_result1["hex"], parent_result2["hex"], child_result["hex"]]

        pkg_result = node.submitpackage(package_hex3)
        assert_equal(pkg_result["package_msg"], 'package RBF failed: package must be 1-parent-1-child')
        self.assert_mempool_contents(expected=package_txns1 + package_txns2)
        self.generate(node, 1)

    def test_insufficient_feerate(self):
        self.log.info("Check Package RBF must beat feerate of direct conflict")
        node = self.nodes[0]
        coin = self.coins.pop()

        # Non-cpfp structure
        package_hex1, package_txns1 = self.create_simple_package(coin, parent_fee=DEFAULT_CHILD_FEE, child_fee=DEFAULT_FEE)
        node.submitpackage(package_hex1)
        self.assert_mempool_contents(expected=package_txns1)

        # Package 2 feerate is below the feerate of directly conflicted parent, so it fails even though
        # total fees are higher than the original package
        package_hex2, _package_txns2 = self.create_simple_package(coin, parent_fee=DEFAULT_CHILD_FEE - Decimal("0.00000001"), child_fee=DEFAULT_CHILD_FEE)
        pkg_results2 = node.submitpackage(package_hex2)
        assert_equal(pkg_results2["package_msg"], 'package RBF failed: insufficient feerate: does not improve feerate diagram')
        self.assert_mempool_contents(expected=package_txns1)
        self.generate(node, 1)

    def test_0fee_package_rbf(self):
        self.log.info("Test package RBF: TRUC 0-fee parent + high-fee child replaces parent's conflicts")
        node = self.nodes[0]
        # Reuse the same coins so that the transactions conflict with one another.
        self.wallet.rescan_utxos()
        parent_coin = self.wallet.get_utxo(confirmed_only=True)

        # package1 pays default fee on both transactions
        parent1 = self.wallet.create_self_transfer(utxo_to_spend=parent_coin, version=3)
        child1 = self.wallet.create_self_transfer(utxo_to_spend=parent1["new_utxo"], version=3)
        package_hex1 = [parent1["hex"], child1["hex"]]
        fees_package1 = parent1["fee"] + child1["fee"]
        submitres1 = node.submitpackage(package_hex1)
        assert_equal(submitres1["package_msg"], "success")
        self.assert_mempool_contents([parent1["tx"], child1["tx"]])

        # package2 has a 0-fee parent (conflicting with package1) and very high fee child
        parent2 = self.wallet.create_self_transfer(utxo_to_spend=parent_coin, fee=0, fee_rate=0, version=3)
        child2 = self.wallet.create_self_transfer(utxo_to_spend=parent2["new_utxo"], fee=fees_package1*10, version=3)
        package_hex2 = [parent2["hex"], child2["hex"]]

        submitres2 = node.submitpackage(package_hex2)
        assert_equal(submitres2["package_msg"], "success")
        assert_equal(set(submitres2["replaced-transactions"]), set([parent1["txid"], child1["txid"]]))
        self.assert_mempool_contents([parent2["tx"], child2["tx"]])

        self.generate(node, 1)

    def test_child_conflicts_parent_mempool_ancestor(self):
        fill_mempool(self, self.nodes[0], tx_sync_fun=self.no_op)
        # Reset coins since we filled the mempool with current coins
        self.coins = self.wallet.get_utxos(mark_as_spent=False, confirmed_only=True)

        self.log.info("Test that package RBF doesn't have issues with mempool<->package conflicts via inconsistency")
        node = self.nodes[0]
        coin = self.coins.pop()

        self.ctr += 1
        grandparent_result = self.wallet.create_self_transfer(
            fee=DEFAULT_FEE,
            utxo_to_spend=coin,
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        node.sendrawtransaction(grandparent_result["hex"])

        # Now make package of two descendants that looks
        # like a cpfp where the parent can't get in on its own
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
            fee_per_output=int(DEFAULT_CHILD_FEE * COIN),
            utxos_to_spend=[parent_result["new_utxo"], coin],
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
        )

        pkg_result = node.submitpackage([parent_result["hex"], child_result["hex"]])
        assert_equal(pkg_result["package_msg"], 'package RBF failed: new transaction cannot have mempool ancestors')
        mempool_info = node.getrawmempool()
        assert grandparent_result["txid"] in mempool_info
        assert parent_result["txid"] not in mempool_info
        assert child_result["txid"] not in mempool_info

if __name__ == "__main__":
    PackageRBFTest(__file__).main()
