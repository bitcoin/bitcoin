#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from decimal import Decimal

from test_framework.messages import (
    MAX_BIP125_RBF_SEQUENCE,
    WITNESS_SCALE_FACTOR,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import (
    COIN,
    DEFAULT_FEE,
    MiniWallet,
)

MAX_REPLACEMENT_CANDIDATES = 100
V3_MAX_VSIZE = 10000

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
        self.extra_args = [[]]
        self.setup_clean_chain = True

    def check_mempool(self, txids):
        """Assert exact contents of the node's mempool (by txid)."""
        mempool_contents = self.nodes[0].getrawmempool()
        assert_equal(len(txids), len(mempool_contents))
        assert all([txid in txids for txid in mempool_contents])

    @cleanup(extra_args=["-datacarriersize=20000"])
    def test_v3_max_vsize(self):
        node = self.nodes[0]
        self.log.info("Test v3-specific maximum transaction vsize")
        tx_v3_heavy = self.wallet.create_self_transfer(target_weight=(V3_MAX_VSIZE + 1) * WITNESS_SCALE_FACTOR, version=3)
        assert_greater_than_or_equal(tx_v3_heavy["tx"].get_vsize(), V3_MAX_VSIZE)
        expected_error_heavy = f"v3-rule-violation, v3 tx {tx_v3_heavy['txid']} (wtxid={tx_v3_heavy['wtxid']}) is too big"
        assert_raises_rpc_error(-26, expected_error_heavy, node.sendrawtransaction, tx_v3_heavy["hex"])
        self.check_mempool([])

        # Ensure we are hitting the v3-specific limit and not something else
        tx_v2_heavy = self.wallet.send_self_transfer(from_node=node, target_weight=(V3_MAX_VSIZE + 1) * WITNESS_SCALE_FACTOR, version=2)
        self.check_mempool([tx_v2_heavy["txid"]])

    @cleanup(extra_args=["-datacarriersize=1000"])
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

    @cleanup(extra_args=None)
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


    @cleanup(extra_args=None)
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

    @cleanup(extra_args=["-datacarriersize=40000"])
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


    @cleanup(extra_args=["-limitdescendantsize=10", "-datacarriersize=40000"])
    def test_nondefault_package_limits(self):
        """
        Max standard tx size + v3 rules imply the ancestor/descendant rules (at their default
        values), but those checks must not be skipped. Ensure both sets of checks are done by
        changing the ancestor/descendant limit configurations.
        """
        node = self.nodes[0]
        self.log.info("Test that a decreased limitdescendantsize also applies to v3 child")
        parent_target_weight = 9990 * WITNESS_SCALE_FACTOR
        child_target_weight = 500 * WITNESS_SCALE_FACTOR
        tx_v3_parent_large1 = self.wallet.send_self_transfer(
            from_node=node,
            target_weight=parent_target_weight,
            version=3
        )
        tx_v3_child_large1 = self.wallet.create_self_transfer(
            utxo_to_spend=tx_v3_parent_large1["new_utxo"],
            target_weight=child_target_weight,
            version=3
        )

        # Parent and child are within v3 limits, but parent's 10kvB descendant limit is exceeded
        assert_greater_than_or_equal(V3_MAX_VSIZE, tx_v3_parent_large1["tx"].get_vsize())
        assert_greater_than_or_equal(1000, tx_v3_child_large1["tx"].get_vsize())
        assert_greater_than(tx_v3_parent_large1["tx"].get_vsize() + tx_v3_child_large1["tx"].get_vsize(), 10000)

        assert_raises_rpc_error(-26, f"too-long-mempool-chain, exceeds descendant size limit for tx {tx_v3_parent_large1['txid']}", node.sendrawtransaction, tx_v3_child_large1["hex"])
        self.check_mempool([tx_v3_parent_large1["txid"]])
        assert_equal(node.getmempoolentry(tx_v3_parent_large1["txid"])["descendantcount"], 1)
        self.generate(node, 1)

        self.log.info("Test that a decreased limitancestorsize also applies to v3 parent")
        self.restart_node(0, extra_args=["-limitancestorsize=10", "-datacarriersize=40000"])
        tx_v3_parent_large2 = self.wallet.send_self_transfer(
            from_node=node,
            target_weight=parent_target_weight,
            version=3
        )
        tx_v3_child_large2 = self.wallet.create_self_transfer(
            utxo_to_spend=tx_v3_parent_large2["new_utxo"],
            target_weight=child_target_weight,
            version=3
        )

        # Parent and child are within v3 limits
        assert_greater_than_or_equal(V3_MAX_VSIZE, tx_v3_parent_large2["tx"].get_vsize())
        assert_greater_than_or_equal(1000, tx_v3_child_large2["tx"].get_vsize())
        assert_greater_than(tx_v3_parent_large2["tx"].get_vsize() + tx_v3_child_large2["tx"].get_vsize(), 10000)

        assert_raises_rpc_error(-26, f"too-long-mempool-chain, exceeds ancestor size limit", node.sendrawtransaction, tx_v3_child_large2["hex"])
        self.check_mempool([tx_v3_parent_large2["txid"]])

    @cleanup(extra_args=["-datacarriersize=1000"])
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

    @cleanup(extra_args=None)
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

    @cleanup(extra_args=None)
    def test_sibling_eviction_package(self):
        """
        When a transaction has a mempool sibling, it may be eligible for sibling eviction.
        However, this option is only available in single transaction acceptance. It doesn't work in
        a multi-testmempoolaccept (where RBF is disabled) or when doing package CPFP.
        """
        self.log.info("Test v3 sibling eviction in submitpackage and multi-testmempoolaccept")
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

        tx_sibling_1 = self.wallet.create_self_transfer(
            utxo_to_spend=tx_mempool_parent["new_utxos"][1],
            version=3,
            fee_rate=DEFAULT_FEE*100,
        )
        tx_has_mempool_uncle = self.wallet.create_self_transfer(utxo_to_spend=tx_sibling_1["new_utxo"], version=3)

        tx_sibling_2 = self.wallet.create_self_transfer(
            utxo_to_spend=tx_mempool_parent["new_utxos"][0],
            version=3,
            fee_rate=DEFAULT_FEE*200,
        )

        tx_sibling_3 = self.wallet.create_self_transfer(
            utxo_to_spend=tx_mempool_parent["new_utxos"][1],
            version=3,
            fee_rate=0,
        )
        tx_bumps_parent_with_sibling = self.wallet.create_self_transfer(
            utxo_to_spend=tx_sibling_3["new_utxo"],
            version=3,
            fee_rate=DEFAULT_FEE*300,
        )

        # Fails with another non-related transaction via testmempoolaccept
        tx_unrelated = self.wallet.create_self_transfer(version=3)
        result_test_unrelated = node.testmempoolaccept([tx_sibling_1["hex"], tx_unrelated["hex"]])
        assert_equal(result_test_unrelated[0]["reject-reason"], "v3-rule-violation")

        # Fails in a package via testmempoolaccept
        result_test_1p1c = node.testmempoolaccept([tx_sibling_1["hex"], tx_has_mempool_uncle["hex"]])
        assert_equal(result_test_1p1c[0]["reject-reason"], "v3-rule-violation")

        # Allowed when tx is submitted in a package and evaluated individually.
        # Note that the child failed since it would be the 3rd generation.
        result_package_indiv = node.submitpackage([tx_sibling_1["hex"], tx_has_mempool_uncle["hex"]])
        self.check_mempool([tx_mempool_parent["txid"], tx_sibling_1["txid"]])
        expected_error_gen3 = f"v3-rule-violation, tx {tx_has_mempool_uncle['txid']} (wtxid={tx_has_mempool_uncle['wtxid']}) would have too many ancestors"

        assert_equal(result_package_indiv["tx-results"][tx_has_mempool_uncle['wtxid']]['error'], expected_error_gen3)

        # Allowed when tx is submitted in a package with in-mempool parent (which is deduplicated).
        node.submitpackage([tx_mempool_parent["hex"], tx_sibling_2["hex"]])
        self.check_mempool([tx_mempool_parent["txid"], tx_sibling_2["txid"]])

        # Child cannot pay for sibling eviction for parent, as it violates v3 topology limits
        result_package_cpfp = node.submitpackage([tx_sibling_3["hex"], tx_bumps_parent_with_sibling["hex"]])
        self.check_mempool([tx_mempool_parent["txid"], tx_sibling_2["txid"]])
        expected_error_cpfp = f"v3-rule-violation, tx {tx_mempool_parent['txid']} (wtxid={tx_mempool_parent['wtxid']}) would exceed descendant count limit"

        assert_equal(result_package_cpfp["tx-results"][tx_sibling_3['wtxid']]['error'], expected_error_cpfp)


    @cleanup(extra_args=["-datacarriersize=1000"])
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

    @cleanup(extra_args=None)
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

    @cleanup(extra_args=None)
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

    @cleanup(extra_args=None)
    def test_v3_sibling_eviction(self):
        self.log.info("Test sibling eviction for v3")
        node = self.nodes[0]
        tx_v3_parent = self.wallet.send_self_transfer_multi(from_node=node, num_outputs=2, version=3)
        # This is the sibling to replace
        tx_v3_child_1 = self.wallet.send_self_transfer(
            from_node=node, utxo_to_spend=tx_v3_parent["new_utxos"][0], fee_rate=DEFAULT_FEE * 2, version=3
        )
        assert tx_v3_child_1["txid"] in node.getrawmempool()

        self.log.info("Test tx must be higher feerate than sibling to evict it")
        tx_v3_child_2_rule6 = self.wallet.create_self_transfer(
            utxo_to_spend=tx_v3_parent["new_utxos"][1], fee_rate=DEFAULT_FEE, version=3
        )
        rule6_str = f"insufficient fee (including sibling eviction), rejecting replacement {tx_v3_child_2_rule6['txid']}"
        assert_raises_rpc_error(-26, rule6_str, node.sendrawtransaction, tx_v3_child_2_rule6["hex"])
        self.check_mempool([tx_v3_parent['txid'], tx_v3_child_1['txid']])

        self.log.info("Test tx must meet absolute fee rules to evict sibling")
        tx_v3_child_2_rule4 = self.wallet.create_self_transfer(
            utxo_to_spend=tx_v3_parent["new_utxos"][1], fee_rate=2 * DEFAULT_FEE + Decimal("0.00000001"), version=3
        )
        rule4_str = f"insufficient fee (including sibling eviction), rejecting replacement {tx_v3_child_2_rule4['txid']}, not enough additional fees to relay"
        assert_raises_rpc_error(-26, rule4_str, node.sendrawtransaction, tx_v3_child_2_rule4["hex"])
        self.check_mempool([tx_v3_parent['txid'], tx_v3_child_1['txid']])

        self.log.info("Test tx cannot cause more than 100 evictions including RBF and sibling eviction")
        # First add 4 groups of 25 transactions.
        utxos_for_conflict = []
        txids_v2_100 = []
        for _ in range(4):
            confirmed_utxo = self.wallet.get_utxo(confirmed_only=True)
            utxos_for_conflict.append(confirmed_utxo)
            # 25 is within descendant limits
            chain_length = int(MAX_REPLACEMENT_CANDIDATES / 4)
            chain = self.wallet.create_self_transfer_chain(chain_length=chain_length, utxo_to_spend=confirmed_utxo)
            for item in chain:
                txids_v2_100.append(item["txid"])
                node.sendrawtransaction(item["hex"])
        self.check_mempool(txids_v2_100 + [tx_v3_parent["txid"], tx_v3_child_1["txid"]])

        # Replacing 100 transactions is fine
        tx_v3_replacement_only = self.wallet.create_self_transfer_multi(utxos_to_spend=utxos_for_conflict, fee_per_output=4000000)
        # Override maxfeerate - it costs a lot to replace these 100 transactions.
        assert node.testmempoolaccept([tx_v3_replacement_only["hex"]], maxfeerate=0)[0]["allowed"]
        # Adding another one exceeds the limit.
        # TODO: rewrite this test given the new RBF rules
        #utxos_for_conflict.append(tx_v3_parent["new_utxos"][1])
        #tx_v3_child_2_rule5 = self.wallet.create_self_transfer_multi(utxos_to_spend=utxos_for_conflict, fee_per_output=4000000, version=3)
        #rule5_str = f"too many potential replacements (including sibling eviction), rejecting replacement {tx_v3_child_2_rule5['txid']}; too many potential replacements (101 > 100)"
        #assert_raises_rpc_error(-26, rule5_str, node.sendrawtransaction, tx_v3_child_2_rule5["hex"])
        self.check_mempool(txids_v2_100 + [tx_v3_parent["txid"], tx_v3_child_1["txid"]])

        self.log.info("Test sibling eviction is successful if it meets all RBF rules")
        tx_v3_child_2 = self.wallet.create_self_transfer(
            utxo_to_spend=tx_v3_parent["new_utxos"][1], fee_rate=DEFAULT_FEE*10, version=3
        )
        node.sendrawtransaction(tx_v3_child_2["hex"])
        self.check_mempool(txids_v2_100 + [tx_v3_parent["txid"], tx_v3_child_2["txid"]])

        self.log.info("Test that it's possible to do a sibling eviction and RBF at the same time")
        utxo_unrelated_conflict = self.wallet.get_utxo(confirmed_only=True)
        tx_unrelated_replacee = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=utxo_unrelated_conflict)
        assert tx_unrelated_replacee["txid"] in node.getrawmempool()

        fee_to_beat = max(int(tx_v3_child_2["fee"] * COIN), int(tx_unrelated_replacee["fee"]*COIN))

        tx_v3_child_3 = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[tx_v3_parent["new_utxos"][0], utxo_unrelated_conflict], fee_per_output=fee_to_beat*4, version=3
        )
        node.sendrawtransaction(tx_v3_child_3["hex"])
        self.check_mempool(txids_v2_100 + [tx_v3_parent["txid"], tx_v3_child_3["txid"]])

    @cleanup(extra_args=None)
    def test_reorg_sibling_eviction_1p2c(self):
        node = self.nodes[0]
        self.log.info("Test that sibling eviction is not allowed when multiple siblings exist")

        tx_with_multi_children = self.wallet.send_self_transfer_multi(from_node=node, num_outputs=3, version=3, confirmed_only=True)
        self.check_mempool([tx_with_multi_children["txid"]])

        block_to_disconnect = self.generate(node, 1)[0]
        self.check_mempool([])

        tx_with_sibling1 = self.wallet.send_self_transfer(from_node=node, version=3, utxo_to_spend=tx_with_multi_children["new_utxos"][0])
        tx_with_sibling2 = self.wallet.send_self_transfer(from_node=node, version=3, utxo_to_spend=tx_with_multi_children["new_utxos"][1])
        self.check_mempool([tx_with_sibling1["txid"], tx_with_sibling2["txid"]])

        # Create a reorg, bringing tx_with_multi_children back into the mempool with a descendant count of 3.
        node.invalidateblock(block_to_disconnect)
        self.check_mempool([tx_with_multi_children["txid"], tx_with_sibling1["txid"], tx_with_sibling2["txid"]])
        assert_equal(node.getmempoolentry(tx_with_multi_children["txid"])["descendantcount"], 3)

        # Sibling eviction is not allowed because there are two siblings
        tx_with_sibling3 = self.wallet.create_self_transfer(
            version=3,
            utxo_to_spend=tx_with_multi_children["new_utxos"][2],
            fee_rate=DEFAULT_FEE*50
        )
        expected_error_2siblings = f"v3-rule-violation, tx {tx_with_multi_children['txid']} (wtxid={tx_with_multi_children['wtxid']}) would exceed descendant count limit"
        assert_raises_rpc_error(-26, expected_error_2siblings, node.sendrawtransaction, tx_with_sibling3["hex"])

        # However, an RBF (with conflicting inputs) is possible even if the resulting cluster size exceeds 2
        tx_with_sibling3_rbf = self.wallet.send_self_transfer(
            from_node=node,
            version=3,
            utxo_to_spend=tx_with_multi_children["new_utxos"][0],
            fee_rate=DEFAULT_FEE*50
        )
        self.check_mempool([tx_with_multi_children["txid"], tx_with_sibling3_rbf["txid"], tx_with_sibling2["txid"]])


    def run_test(self):
        self.log.info("Generate blocks to create UTXOs")
        node = self.nodes[0]
        self.wallet = MiniWallet(node)
        self.generate(self.wallet, 120)
        self.test_v3_max_vsize()
        self.test_v3_acceptance()
        self.test_v3_replacement()
        self.test_v3_bip125()
        self.test_v3_reorg()
        self.test_nondefault_package_limits()
        self.test_v3_ancestors_package()
        self.test_v3_ancestors_package_and_mempool()
        self.test_sibling_eviction_package()
        self.test_v3_package_inheritance()
        self.test_v3_in_testmempoolaccept()
        self.test_reorg_2child_rbf()
        self.test_v3_sibling_eviction()
        self.test_reorg_sibling_eviction_1p2c()


if __name__ == "__main__":
    MempoolAcceptV3().main()
