#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from decimal import Decimal

from test_framework.messages import (
    MAX_BIP125_RBF_SEQUENCE,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import (
    DEFAULT_FEE,
    MiniWallet,
)

def cleanup(extra_args=None):
    def decorator(func):
        def wrapper(self):
            try:
                if extra_args is not None:
                    self.restart_node(0, extra_args=extra_args)
                func(self)
            finally:
                # Clear mempool again after test
                self.generate(self.nodes[0], 1)
                if extra_args is not None:
                    self.restart_node(0)
        return wrapper
    return decorator

class MempoolAcceptV3(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-acceptnonstdtxn=1"]]
        self.setup_clean_chain = True

    def check_mempool(self, txids):
        """Assert exact contents of the node's mempool (by txid)."""
        mempool_contents = self.nodes[0].getrawmempool()
        assert_equal(len(txids), len(mempool_contents))
        assert all([txid in txids for txid in mempool_contents])

    @cleanup(extra_args=["-datacarriersize=1000", "-acceptnonstdtxn=1"])
    def test_v3_acceptance(self):
        node = self.nodes[0]
        self.log.info("Test a child of a v3 transaction cannot be more than 1000vB")
        tx_v3_parent_normal = self.wallet.send_self_transfer(from_node=node, version=3)
        self.check_mempool([tx_v3_parent_normal["txid"]])
        tx_v3_child_heavy = self.wallet.create_self_transfer(
            utxo_to_spend=tx_v3_parent_normal["new_utxo"],
            target_weight=4004,
            version=3
        )
        assert_greater_than_or_equal(tx_v3_child_heavy["tx"].get_vsize(), 1000)
        expected_error_child_heavy = f"v3-rule-violation, v3 child tx {tx_v3_child_heavy['txid']} (wtxid={tx_v3_child_heavy['wtxid']}) is too big"
        assert_raises_rpc_error(-26, expected_error_child_heavy, node.sendrawtransaction, tx_v3_child_heavy["hex"])
        self.check_mempool([tx_v3_parent_normal["txid"]])
        # tx has no descendants
        assert_equal(node.getmempoolentry(tx_v3_parent_normal["txid"])["descendantcount"], 1)

        self.log.info("Test that, during replacements, only the new transaction counts for v3 descendant limit")
        tx_v3_child_almost_heavy = self.wallet.send_self_transfer(
            from_node=node,
            fee_rate=DEFAULT_FEE,
            utxo_to_spend=tx_v3_parent_normal["new_utxo"],
            target_weight=3987,
            version=3
        )
        assert_greater_than_or_equal(1000, tx_v3_child_almost_heavy["tx"].get_vsize())
        self.check_mempool([tx_v3_parent_normal["txid"], tx_v3_child_almost_heavy["txid"]])
        assert_equal(node.getmempoolentry(tx_v3_parent_normal["txid"])["descendantcount"], 2)
        tx_v3_child_almost_heavy_rbf = self.wallet.send_self_transfer(
            from_node=node,
            fee_rate=DEFAULT_FEE * 2,
            utxo_to_spend=tx_v3_parent_normal["new_utxo"],
            target_weight=3500,
            version=3
        )
        assert_greater_than_or_equal(tx_v3_child_almost_heavy["tx"].get_vsize() + tx_v3_child_almost_heavy_rbf["tx"].get_vsize(), 1000)
        self.check_mempool([tx_v3_parent_normal["txid"], tx_v3_child_almost_heavy_rbf["txid"]])
        assert_equal(node.getmempoolentry(tx_v3_parent_normal["txid"])["descendantcount"], 2)

    @cleanup(extra_args=["-acceptnonstdtxn=1"])
    def test_v3_replacement(self):
        node = self.nodes[0]
        self.log.info("Test v3 transactions may be replaced by v3 transactions")
        utxo_v3_bip125 = self.wallet.get_utxo()
        tx_v3_bip125 = self.wallet.send_self_transfer(
            from_node=node,
            fee_rate=DEFAULT_FEE,
            utxo_to_spend=utxo_v3_bip125,
            sequence=MAX_BIP125_RBF_SEQUENCE,
            version=3
        )
        self.check_mempool([tx_v3_bip125["txid"]])

        tx_v3_bip125_rbf = self.wallet.send_self_transfer(
            from_node=node,
            fee_rate=DEFAULT_FEE * 2,
            utxo_to_spend=utxo_v3_bip125,
            version=3
        )
        self.check_mempool([tx_v3_bip125_rbf["txid"]])

        self.log.info("Test v3 transactions may be replaced by V2 transactions")
        tx_v3_bip125_rbf_v2 = self.wallet.send_self_transfer(
            from_node=node,
            fee_rate=DEFAULT_FEE * 3,
            utxo_to_spend=utxo_v3_bip125,
            version=2
        )
        self.check_mempool([tx_v3_bip125_rbf_v2["txid"]])

        self.log.info("Test that replacements cannot cause violation of inherited v3")
        utxo_v3_parent = self.wallet.get_utxo()
        tx_v3_parent = self.wallet.send_self_transfer(
            from_node=node,
            fee_rate=DEFAULT_FEE,
            utxo_to_spend=utxo_v3_parent,
            version=3
        )
        tx_v3_child = self.wallet.send_self_transfer(
            from_node=node,
            fee_rate=DEFAULT_FEE,
            utxo_to_spend=tx_v3_parent["new_utxo"],
            version=3
        )
        self.check_mempool([tx_v3_bip125_rbf_v2["txid"], tx_v3_parent["txid"], tx_v3_child["txid"]])

        tx_v3_child_rbf_v2 = self.wallet.create_self_transfer(
            fee_rate=DEFAULT_FEE * 2,
            utxo_to_spend=tx_v3_parent["new_utxo"],
            version=2
        )
        expected_error_v2_v3 = f"v3-rule-violation, non-v3 tx {tx_v3_child_rbf_v2['txid']} (wtxid={tx_v3_child_rbf_v2['wtxid']}) cannot spend from v3 tx {tx_v3_parent['txid']} (wtxid={tx_v3_parent['wtxid']})"
        assert_raises_rpc_error(-26, expected_error_v2_v3, node.sendrawtransaction, tx_v3_child_rbf_v2["hex"])
        self.check_mempool([tx_v3_bip125_rbf_v2["txid"], tx_v3_parent["txid"], tx_v3_child["txid"]])


    @cleanup(extra_args=["-acceptnonstdtxn=1"])
    def test_v3_bip125(self):
        node = self.nodes[0]
        self.log.info("Test v3 transactions that don't signal BIP125 are replaceable")
        assert_equal(node.getmempoolinfo()["fullrbf"], False)
        utxo_v3_no_bip125 = self.wallet.get_utxo()
        tx_v3_no_bip125 = self.wallet.send_self_transfer(
            from_node=node,
            fee_rate=DEFAULT_FEE,
            utxo_to_spend=utxo_v3_no_bip125,
            sequence=MAX_BIP125_RBF_SEQUENCE + 1,
            version=3
        )

        self.check_mempool([tx_v3_no_bip125["txid"]])
        assert not node.getmempoolentry(tx_v3_no_bip125["txid"])["bip125-replaceable"]
        tx_v3_no_bip125_rbf = self.wallet.send_self_transfer(
            from_node=node,
            fee_rate=DEFAULT_FEE * 2,
            utxo_to_spend=utxo_v3_no_bip125,
            version=3
        )
        self.check_mempool([tx_v3_no_bip125_rbf["txid"]])

    @cleanup(extra_args=["-datacarriersize=40000", "-acceptnonstdtxn=1"])
    def test_v3_reorg(self):
        node = self.nodes[0]
        self.log.info("Test that, during a reorg, v3 rules are not enforced")
        tx_v2_block = self.wallet.send_self_transfer(from_node=node, version=2)
        tx_v3_block = self.wallet.send_self_transfer(from_node=node, version=3)
        tx_v3_block2 = self.wallet.send_self_transfer(from_node=node, version=3)
        self.check_mempool([tx_v3_block["txid"], tx_v2_block["txid"], tx_v3_block2["txid"]])

        block = self.generate(node, 1)
        self.check_mempool([])
        tx_v2_from_v3 = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=tx_v3_block["new_utxo"], version=2)
        tx_v3_from_v2 = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=tx_v2_block["new_utxo"], version=3)
        tx_v3_child_large = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=tx_v3_block2["new_utxo"], target_weight=5000, version=3)
        assert_greater_than(node.getmempoolentry(tx_v3_child_large["txid"])["vsize"], 1000)
        self.check_mempool([tx_v2_from_v3["txid"], tx_v3_from_v2["txid"], tx_v3_child_large["txid"]])
        node.invalidateblock(block[0])
        self.check_mempool([tx_v3_block["txid"], tx_v2_block["txid"], tx_v3_block2["txid"], tx_v2_from_v3["txid"], tx_v3_from_v2["txid"], tx_v3_child_large["txid"]])
        # This is needed because generate() will create the exact same block again.
        node.reconsiderblock(block[0])


    @cleanup(extra_args=["-limitclustercount=1", "-datacarriersize=40000", "-acceptnonstdtxn=1"])
    def test_nondefault_package_limits(self):
        """
        Max standard tx size + v3 rules imply the cluster rules (at their default
        values), but those checks must not be skipped. Ensure both sets of checks are done by
        changing the cluster limit configurations.
        """
        node = self.nodes[0]
        self.log.info("Test that a decreased cluster size limit also applies to v3 child")
        tx_v3_parent_large1 = self.wallet.send_self_transfer(from_node=node, target_weight=99900, version=3)
        tx_v3_child_large1 = self.wallet.create_self_transfer(utxo_to_spend=tx_v3_parent_large1["new_utxo"], version=3)
        # Child is within v3 limits, but parent's descendant limit is exceeded
        assert_greater_than(1000, tx_v3_child_large1["tx"].get_vsize())
        assert_raises_rpc_error(-26, f"too-large-cluster, too many unconfirmed transactions in the cluster [limit: 1]", node.sendrawtransaction, tx_v3_child_large1["hex"])
        self.check_mempool([tx_v3_parent_large1["txid"]])
        assert_equal(node.getmempoolentry(tx_v3_parent_large1["txid"])["descendantcount"], 1)
        self.generate(node, 1)

    @cleanup(extra_args=["-datacarriersize=1000", "-acceptnonstdtxn=1"])
    def test_v3_ancestors_package(self):
        self.log.info("Test that v3 ancestor limits are checked within the package")
        node = self.nodes[0]
        tx_v3_parent_normal = self.wallet.create_self_transfer(
            fee_rate=0,
            target_weight=4004,
            version=3
        )
        tx_v3_parent_2_normal = self.wallet.create_self_transfer(
            fee_rate=0,
            target_weight=4004,
            version=3
        )
        tx_v3_child_multiparent = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[tx_v3_parent_normal["new_utxo"], tx_v3_parent_2_normal["new_utxo"]],
            fee_per_output=10000,
            version=3
        )
        tx_v3_child_heavy = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[tx_v3_parent_normal["new_utxo"]],
            target_weight=4004,
            fee_per_output=10000,
            version=3
        )

        self.check_mempool([])
        result = node.submitpackage([tx_v3_parent_normal["hex"], tx_v3_parent_2_normal["hex"], tx_v3_child_multiparent["hex"]])
        assert_equal(result['package_msg'], f"v3-violation, tx {tx_v3_child_multiparent['txid']} (wtxid={tx_v3_child_multiparent['wtxid']}) would have too many ancestors")
        self.check_mempool([])

        self.check_mempool([])
        result = node.submitpackage([tx_v3_parent_normal["hex"], tx_v3_child_heavy["hex"]])
        # tx_v3_child_heavy is heavy based on weight, not sigops.
        assert_equal(result['package_msg'], f"v3-violation, v3 child tx {tx_v3_child_heavy['txid']} (wtxid={tx_v3_child_heavy['wtxid']}) is too big: {tx_v3_child_heavy['tx'].get_vsize()} > 1000 virtual bytes")
        self.check_mempool([])

        tx_v3_parent = self.wallet.create_self_transfer(version=3)
        tx_v3_child = self.wallet.create_self_transfer(utxo_to_spend=tx_v3_parent["new_utxo"], version=3)
        tx_v3_grandchild = self.wallet.create_self_transfer(utxo_to_spend=tx_v3_child["new_utxo"], version=3)
        result = node.testmempoolaccept([tx_v3_parent["hex"], tx_v3_child["hex"], tx_v3_grandchild["hex"]])
        assert all([txresult["package-error"] == f"v3-violation, tx {tx_v3_grandchild['txid']} (wtxid={tx_v3_grandchild['wtxid']}) would have too many ancestors" for txresult in result])

    @cleanup(extra_args=["-acceptnonstdtxn=1"])
    def test_v3_ancestors_package_and_mempool(self):
        """
        A v3 transaction in a package cannot have 2 v3 parents.
        Test that if we have a transaction graph A -> B -> C, where A, B, C are
        all v3 transactions, that we cannot use submitpackage to get the
        transactions all into the mempool.

        Verify, in particular, that if A is already in the mempool, then
        submitpackage(B, C) will fail.
        """
        node = self.nodes[0]
        self.log.info("Test that v3 ancestor limits include transactions within the package and all in-mempool ancestors")
        # This is our transaction "A":
        tx_in_mempool = self.wallet.send_self_transfer(from_node=node, version=3)

        # Verify that A is in the mempool
        self.check_mempool([tx_in_mempool["txid"]])

        # tx_0fee_parent is our transaction "B"; just create it.
        tx_0fee_parent = self.wallet.create_self_transfer(utxo_to_spend=tx_in_mempool["new_utxo"], fee=0, fee_rate=0, version=3)

        # tx_child_violator is our transaction "C"; create it:
        tx_child_violator = self.wallet.create_self_transfer_multi(utxos_to_spend=[tx_0fee_parent["new_utxo"]], version=3)

        # submitpackage(B, C) should fail
        result = node.submitpackage([tx_0fee_parent["hex"], tx_child_violator["hex"]])
        assert_equal(result['package_msg'], f"v3-violation, tx {tx_child_violator['txid']} (wtxid={tx_child_violator['wtxid']}) would have too many ancestors")
        self.check_mempool([tx_in_mempool["txid"]])

    @cleanup(extra_args=["-acceptnonstdtxn=1"])
    def test_mempool_sibling(self):
        self.log.info("Test that v3 transaction cannot have mempool siblings")
        node = self.nodes[0]
        # Add a parent + child to mempool
        tx_mempool_parent = self.wallet.send_self_transfer_multi(
            from_node=node,
            utxos_to_spend=[self.wallet.get_utxo()],
            num_outputs=2,
            version=3
        )
        tx_mempool_sibling = self.wallet.send_self_transfer(
            from_node=node,
            utxo_to_spend=tx_mempool_parent["new_utxos"][0],
            version=3
        )
        self.check_mempool([tx_mempool_parent["txid"], tx_mempool_sibling["txid"]])

        tx_has_mempool_sibling = self.wallet.create_self_transfer(
            utxo_to_spend=tx_mempool_parent["new_utxos"][1],
            version=3
        )
        expected_error_mempool_sibling = f"v3-rule-violation, tx {tx_mempool_parent['txid']} (wtxid={tx_mempool_parent['wtxid']}) would exceed descendant count limit"
        assert_raises_rpc_error(-26, expected_error_mempool_sibling, node.sendrawtransaction, tx_has_mempool_sibling["hex"])

        tx_has_mempool_uncle = self.wallet.create_self_transfer(utxo_to_spend=tx_has_mempool_sibling["new_utxo"], version=3)

        # Also fails with another non-related transaction via testmempoolaccept
        tx_unrelated = self.wallet.create_self_transfer(version=3)
        result_test_unrelated = node.testmempoolaccept([tx_has_mempool_sibling["hex"], tx_unrelated["hex"]])
        assert_equal(result_test_unrelated[0]["reject-reason"], "v3-rule-violation")

        result_test_1p1c = node.testmempoolaccept([tx_has_mempool_sibling["hex"], tx_has_mempool_uncle["hex"]])
        assert_equal(result_test_1p1c[0]["reject-reason"], "v3-rule-violation")

        # Also fails with a child via submitpackage
        result_submitpackage = node.submitpackage([tx_has_mempool_sibling["hex"], tx_has_mempool_uncle["hex"]])
        assert_equal(result_submitpackage["tx-results"][tx_has_mempool_sibling['wtxid']]['error'], expected_error_mempool_sibling)


    @cleanup(extra_args=["-datacarriersize=1000", "-acceptnonstdtxn=1"])
    def test_v3_package_inheritance(self):
        self.log.info("Test that v3 inheritance is checked within package")
        node = self.nodes[0]
        tx_v3_parent = self.wallet.create_self_transfer(
            fee_rate=0,
            target_weight=4004,
            version=3
        )
        tx_v2_child = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[tx_v3_parent["new_utxo"]],
            fee_per_output=10000,
            version=2
        )
        self.check_mempool([])
        result = node.submitpackage([tx_v3_parent["hex"], tx_v2_child["hex"]])
        assert_equal(result['package_msg'], f"v3-violation, non-v3 tx {tx_v2_child['txid']} (wtxid={tx_v2_child['wtxid']}) cannot spend from v3 tx {tx_v3_parent['txid']} (wtxid={tx_v3_parent['wtxid']})")
        self.check_mempool([])

    @cleanup(extra_args=["-acceptnonstdtxn=1"])
    def test_v3_in_testmempoolaccept(self):
        node = self.nodes[0]

        self.log.info("Test that v3 inheritance is accurately assessed in testmempoolaccept")
        tx_v2 = self.wallet.create_self_transfer(version=2)
        tx_v2_from_v2 = self.wallet.create_self_transfer(utxo_to_spend=tx_v2["new_utxo"], version=2)
        tx_v3_from_v2 = self.wallet.create_self_transfer(utxo_to_spend=tx_v2["new_utxo"], version=3)
        tx_v3 = self.wallet.create_self_transfer(version=3)
        tx_v2_from_v3 = self.wallet.create_self_transfer(utxo_to_spend=tx_v3["new_utxo"], version=2)
        tx_v3_from_v3 = self.wallet.create_self_transfer(utxo_to_spend=tx_v3["new_utxo"], version=3)

        # testmempoolaccept paths don't require child-with-parents topology. Ensure that topology
        # assumptions aren't made in inheritance checks.
        test_accept_v2_and_v3 = node.testmempoolaccept([tx_v2["hex"], tx_v3["hex"]])
        assert all([result["allowed"] for result in test_accept_v2_and_v3])

        test_accept_v3_from_v2 = node.testmempoolaccept([tx_v2["hex"], tx_v3_from_v2["hex"]])
        expected_error_v3_from_v2 = f"v3-violation, v3 tx {tx_v3_from_v2['txid']} (wtxid={tx_v3_from_v2['wtxid']}) cannot spend from non-v3 tx {tx_v2['txid']} (wtxid={tx_v2['wtxid']})"
        assert all([result["package-error"] == expected_error_v3_from_v2 for result in test_accept_v3_from_v2])

        test_accept_v2_from_v3 = node.testmempoolaccept([tx_v3["hex"], tx_v2_from_v3["hex"]])
        expected_error_v2_from_v3 = f"v3-violation, non-v3 tx {tx_v2_from_v3['txid']} (wtxid={tx_v2_from_v3['wtxid']}) cannot spend from v3 tx {tx_v3['txid']} (wtxid={tx_v3['wtxid']})"
        assert all([result["package-error"] == expected_error_v2_from_v3 for result in test_accept_v2_from_v3])

        test_accept_pairs = node.testmempoolaccept([tx_v2["hex"], tx_v3["hex"], tx_v2_from_v2["hex"], tx_v3_from_v3["hex"]])
        assert all([result["allowed"] for result in test_accept_pairs])

        self.log.info("Test that descendant violations are caught in testmempoolaccept")
        tx_v3_independent = self.wallet.create_self_transfer(version=3)
        tx_v3_parent = self.wallet.create_self_transfer_multi(num_outputs=2, version=3)
        tx_v3_child_1 = self.wallet.create_self_transfer(utxo_to_spend=tx_v3_parent["new_utxos"][0], version=3)
        tx_v3_child_2 = self.wallet.create_self_transfer(utxo_to_spend=tx_v3_parent["new_utxos"][1], version=3)
        test_accept_2children = node.testmempoolaccept([tx_v3_parent["hex"], tx_v3_child_1["hex"], tx_v3_child_2["hex"]])
        expected_error_2children = f"v3-violation, tx {tx_v3_parent['txid']} (wtxid={tx_v3_parent['wtxid']}) would exceed descendant count limit"
        assert all([result["package-error"] == expected_error_2children for result in test_accept_2children])

        # Extra v3 transaction does not get incorrectly marked as extra descendant
        test_accept_1child_with_exra = node.testmempoolaccept([tx_v3_parent["hex"], tx_v3_child_1["hex"], tx_v3_independent["hex"]])
        assert all([result["allowed"] for result in test_accept_1child_with_exra])

        # Extra v3 transaction does not make us ignore the extra descendant
        test_accept_2children_with_exra = node.testmempoolaccept([tx_v3_parent["hex"], tx_v3_child_1["hex"], tx_v3_child_2["hex"], tx_v3_independent["hex"]])
        expected_error_extra = f"v3-violation, tx {tx_v3_parent['txid']} (wtxid={tx_v3_parent['wtxid']}) would exceed descendant count limit"
        assert all([result["package-error"] == expected_error_extra for result in test_accept_2children_with_exra])
        # Same result if the parent is already in mempool
        node.sendrawtransaction(tx_v3_parent["hex"])
        test_accept_2children_with_in_mempool_parent = node.testmempoolaccept([tx_v3_child_1["hex"], tx_v3_child_2["hex"]])
        assert all([result["package-error"] == expected_error_extra for result in test_accept_2children_with_in_mempool_parent])

    @cleanup(extra_args=["-acceptnonstdtxn=1"])
    def test_reorg_2child_rbf(self):
        node = self.nodes[0]
        self.log.info("Test that children of a v3 transaction can be replaced individually, even if there are multiple due to reorg")

        ancestor_tx = self.wallet.send_self_transfer_multi(from_node=node, num_outputs=2, version=3)
        self.check_mempool([ancestor_tx["txid"]])

        block = self.generate(node, 1)[0]
        self.check_mempool([])

        child_1 = self.wallet.send_self_transfer(from_node=node, version=3, utxo_to_spend=ancestor_tx["new_utxos"][0])
        child_2 = self.wallet.send_self_transfer(from_node=node, version=3, utxo_to_spend=ancestor_tx["new_utxos"][1])
        self.check_mempool([child_1["txid"], child_2["txid"]])

        self.generate(node, 1)
        self.check_mempool([])

        # Create a reorg, causing ancestor_tx to exceed the 1-child limit
        node.invalidateblock(block)
        self.check_mempool([ancestor_tx["txid"], child_1["txid"], child_2["txid"]])
        assert_equal(node.getmempoolentry(ancestor_tx["txid"])["descendantcount"], 3)

        # Create a replacement of child_1. It does not conflict with child_2.
        child_1_conflict = self.wallet.send_self_transfer(from_node=node, version=3, utxo_to_spend=ancestor_tx["new_utxos"][0], fee_rate=Decimal("0.01"))

        # Ensure child_1 and child_1_conflict are different transactions
        assert (child_1_conflict["txid"] != child_1["txid"])
        self.check_mempool([ancestor_tx["txid"], child_1_conflict["txid"], child_2["txid"]])
        assert_equal(node.getmempoolentry(ancestor_tx["txid"])["descendantcount"], 3)

    def run_test(self):
        self.log.info("Generate blocks to create UTXOs")
        node = self.nodes[0]
        self.wallet = MiniWallet(node)
        self.generate(self.wallet, 110)
        self.test_v3_acceptance()
        self.test_v3_replacement()
        self.test_v3_bip125()
        self.test_v3_reorg()
        self.test_nondefault_package_limits()
        self.test_v3_ancestors_package()
        self.test_v3_ancestors_package_and_mempool()
        self.test_mempool_sibling()
        self.test_v3_package_inheritance()
        self.test_v3_in_testmempoolaccept()
        self.test_reorg_2child_rbf()


if __name__ == "__main__":
    MempoolAcceptV3().main()
