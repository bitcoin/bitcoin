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
from test_framework.util import (
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
    assert_equal,
)
from test_framework.wallet import (
    DEFAULT_FEE,
    MiniWallet,
)

class PackageRBFTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

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

        # Package 2 has a higher feerate but lower absolute fee
        package_hex2, package_txns2 = self.create_simple_package(coin, parent_fee=DEFAULT_FEE - fee_delta, child_fee=DEFAULT_FEE + fee_delta - Decimal("0.000000001"))
        pkg_results2 = node.submitpackage(package_hex2)
        assert_equal(pkg_results2["package_msg"], 'package RBF failed: insufficient anti-DoS fees')
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
        package_hex5, package_txns5 = self.create_simple_package(coin, parent_fee=DEFAULT_FEE - fee_delta, child_fee=DEFAULT_FEE + fee_delta + Decimal("0.000000001"))
        pkg_results5 = node.submitpackage(package_hex5)
        assert_equal(pkg_results5["package_msg"], 'package RBF failed: insufficient anti-DoS fees')

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
        assert_equal(pkg_results["package_msg"], "package RBF failed: too many potential replacements")
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
        self.log.info("Check Package RBF must neat feerate of direct conflict")
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
        assert_equal(pkg_results2["package_msg"], 'package RBF failed: insufficient feerate')
        self.assert_mempool_contents(expected=package_txns1, unexpected=package_txns2)
        self.generate(node, 1)

if __name__ == "__main__":
    PackageRBFTest().main()
