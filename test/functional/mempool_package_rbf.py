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

    def create_simple_package(self, parent_coin, parent_fee=0, child_fee=DEFAULT_FEE, heavy_child=False, version=3):
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
            version=version
        )

        num_child_outputs = 10 if heavy_child else 1
        child_result = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[parent_result["new_utxo"]],
            num_outputs=num_child_outputs,
            fee_per_output=int(child_fee * COIN // num_child_outputs),
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
            version=version
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
        self.test_package_rbf_signaling()
        self.test_package_rbf_additional_fees()
        self.test_package_rbf_max_conflicts()
        self.test_package_rbf_conflicting_conflicts()
        self.test_package_rbf_partial()

    def test_package_rbf_basic(self):
        self.log.info("Test that a child can pay to replace its parents' conflicts")
        node = self.nodes[0]
        # Reuse the same coins so that the transactions conflict with one another.
        parent_coin = self.coins.pop()
        package_hex1, package_txns1 = self.create_simple_package(parent_coin, DEFAULT_FEE, DEFAULT_FEE)
        package_hex2, package_txns2 = self.create_simple_package(parent_coin, 0, DEFAULT_FEE * 5)
        node.submitpackage(package_hex1)
        self.assert_mempool_contents(expected=package_txns1, unexpected=package_txns2)

        submitres = node.submitpackage(package_hex2)
        submitres["replaced-transactions"] == [tx.rehash() for tx in package_txns1]
        self.assert_mempool_contents(expected=package_txns2, unexpected=package_txns1)
        self.generate(node, 1)

    def test_package_rbf_signaling(self):
        node = self.nodes[0]
        self.log.info("Test that V3 transactions not signaling BIP125 are replaceable")
        # Create single transaction that doesn't signal BIP125 but has nVersion=3
        coin = self.coins.pop()

        tx_v3_no_bip125 = self.wallet.create_self_transfer(
            fee=DEFAULT_FEE,
            utxo_to_spend=coin,
            sequence=MAX_BIP125_RBF_SEQUENCE + 1,
            version=3
        )
        node.sendrawtransaction(tx_v3_no_bip125["hex"])
        self.assert_mempool_contents(expected=[tx_v3_no_bip125["tx"]])

        self.log.info("Test that non-V3 transactions signaling BIP125 are replaceable")
        coin = self.coins[0]
        del self.coins[0]
        # This transaction signals BIP125 but isn't V3
        tx_bip125_v2 = self.wallet.create_self_transfer(
            fee=DEFAULT_FEE,
            utxo_to_spend=coin,
            version=2
        )
        node.sendrawtransaction(tx_bip125_v2["hex"])

        self.assert_mempool_contents(expected=[tx_bip125_v2["tx"]])
        assert node.getmempoolentry(tx_bip125_v2["tx"].rehash())["bip125-replaceable"]
        assert tx_bip125_v2["tx"].nVersion == 2
        package_hex_v3, package_txns_v3 = self.create_simple_package(coin, parent_fee=0, child_fee=DEFAULT_FEE * 3, version=3)
        assert all([tx.nVersion == 3 for tx in package_txns_v3])
        node.submitpackage(package_hex_v3)
        self.assert_mempool_contents(expected=package_txns_v3, unexpected=[tx_bip125_v2["tx"]])
        self.generate(node, 1)

    def test_package_rbf_additional_fees(self):
        self.log.info("Check Package RBF must increase the absolute fee")
        node = self.nodes[0]
        coin = self.coins.pop()
        package_hex1, package_txns1 = self.create_simple_package(coin, parent_fee=DEFAULT_FEE, child_fee=DEFAULT_FEE, heavy_child=True)
        assert_greater_than_or_equal(1000, package_txns1[-1].get_vsize())
        node.submitpackage(package_hex1)
        self.assert_mempool_contents(expected=package_txns1)
        # Package 2 has a higher feerate but lower absolute fee
        package_fees1 = DEFAULT_FEE * 2
        package_hex2, package_txns2 = self.create_simple_package(coin, parent_fee=0, child_fee=package_fees1 - Decimal("0.000000001"))
        assert_raises_rpc_error(-25, "package RBF failed: insufficient fee", node.submitpackage, package_hex2)
        self.assert_mempool_contents(expected=package_txns1, unexpected=package_txns2)
        # Package 3 has a higher feerate and absolute fee
        package_hex3, package_txns3 = self.create_simple_package(coin, parent_fee=0, child_fee=package_fees1 * 3)
        node.submitpackage(package_hex3)
        self.assert_mempool_contents(expected=package_txns3, unexpected=package_txns1 + package_txns2)
        self.generate(node, 1)

        self.log.info("Check Package RBF must pay for the entire package's bandwidth")
        coin = self.coins.pop()
        package_hex1, package_txns1 = self.create_simple_package(coin, parent_fee=DEFAULT_FEE, child_fee=DEFAULT_FEE)
        package_fees1 = 2 * DEFAULT_FEE
        node.submitpackage(package_hex1)
        self.assert_mempool_contents(expected=package_txns1, unexpected=[])
        package_hex2, package_txns2 = self.create_simple_package(coin, child_fee=package_fees1 + Decimal("0.000000001"))
        assert_raises_rpc_error(-25, "package RBF failed: insufficient fee", node.submitpackage, package_hex2)
        self.assert_mempool_contents(expected=package_txns1, unexpected=package_txns2)
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
        package_parent = self.wallet.create_self_transfer_multi(utxos_to_spend=parent_coins, version=3)
        package_child = self.wallet.create_self_transfer(fee_rate=50*DEFAULT_FEE, utxo_to_spend=package_parent["new_utxos"][0], version=3)

        assert_raises_rpc_error(-25, "package RBF failed: too many potential replacements",
                node.submitpackage, [package_parent["hex"], package_child["hex"]])
        self.generate(node, 1)

    def test_package_rbf_conflicting_conflicts(self):
        node = self.nodes[0]
        self.log.info("Check that different package transactions cannot share the same conflicts")
        coin = self.coins.pop()
        package_hex1, package_txns1 = self.create_simple_package(coin, DEFAULT_FEE, DEFAULT_FEE)
        package_hex2, package_txns2 = self.create_simple_package(coin, Decimal("0.00009"), DEFAULT_FEE * 2)
        package_hex3, package_txns3 = self.create_simple_package(coin, 0, DEFAULT_FEE * 5)
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
        submitres2["replaced-transactions"] == [tx.rehash() for tx in package_txns1]
        self.assert_mempool_contents(expected=package_txns2, unexpected=package_txns1)
        submitres3 = node.submitpackage(package_hex3)
        submitres3["replaced-transactions"] == [tx.rehash() for tx in package_txns2]
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

if __name__ == "__main__":
    PackageRBFTest().main()
