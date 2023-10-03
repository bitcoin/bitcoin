#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_not_equal,
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
    get_fee,
)
from test_framework.wallet import (
    COIN,
    DEFAULT_FEE,
    MiniWallet,
)

MAX_REPLACEMENT_CANDIDATES = 100
TRUC_MAX_VSIZE = 10000
TRUC_CHILD_MAX_VSIZE = 1000

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

class MempoolTRUC(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[]]
        self.setup_clean_chain = True

    def check_mempool(self, txids):
        """Assert exact contents of the node's mempool (by txid)."""
        mempool_contents = self.nodes[0].getrawmempool()
        assert_equal(len(txids), len(mempool_contents))
        assert all([txid in txids for txid in mempool_contents])

    @cleanup()
    def test_truc_max_vsize(self):
        node = self.nodes[0]
        self.log.info("Test TRUC-specific maximum transaction vsize")
        tx_v3_heavy = self.wallet.create_self_transfer(target_vsize=TRUC_MAX_VSIZE + 1, version=3)
        assert_greater_than_or_equal(tx_v3_heavy["tx"].get_vsize(), TRUC_MAX_VSIZE)
        expected_error_heavy = f"TRUC-violation, version=3 tx {tx_v3_heavy['txid']} (wtxid={tx_v3_heavy['wtxid']}) is too big"
        assert_raises_rpc_error(-26, expected_error_heavy, node.sendrawtransaction, tx_v3_heavy["hex"])
        self.check_mempool([])

        # Ensure we are hitting the TRUC-specific limit and not something else
        tx_v2_heavy = self.wallet.send_self_transfer(from_node=node, target_vsize=TRUC_MAX_VSIZE + 1, version=2)
        self.check_mempool([tx_v2_heavy["txid"]])

    @cleanup()
    def test_truc_acceptance(self):
        node = self.nodes[0]
        self.log.info("Test a child of a TRUC transaction cannot be more than 1000vB")
        tx_v3_parent_normal = self.wallet.send_self_transfer(from_node=node, version=3)
        self.check_mempool([tx_v3_parent_normal["txid"]])
        tx_v3_child_heavy = self.wallet.create_self_transfer(
            utxo_to_spend=tx_v3_parent_normal["new_utxo"],
            target_vsize=TRUC_CHILD_MAX_VSIZE + 1,
            version=3
        )
        assert_greater_than_or_equal(tx_v3_child_heavy["tx"].get_vsize(), TRUC_CHILD_MAX_VSIZE)
        expected_error_child_heavy = f"TRUC-violation, version=3 child tx {tx_v3_child_heavy['txid']} (wtxid={tx_v3_child_heavy['wtxid']}) is too big"
        assert_raises_rpc_error(-26, expected_error_child_heavy, node.sendrawtransaction, tx_v3_child_heavy["hex"])
        self.check_mempool([tx_v3_parent_normal["txid"]])
        # tx has no descendants
        assert_equal(node.getmempoolentry(tx_v3_parent_normal["txid"])["descendantcount"], 1)

        self.log.info("Test that, during replacements, only the new transaction counts for TRUC descendant limit")
        tx_v3_child_almost_heavy = self.wallet.send_self_transfer(
            from_node=node,
            fee_rate=DEFAULT_FEE,
            utxo_to_spend=tx_v3_parent_normal["new_utxo"],
            target_vsize=TRUC_CHILD_MAX_VSIZE - 3,
            version=3
        )
        assert_greater_than_or_equal(TRUC_CHILD_MAX_VSIZE, tx_v3_child_almost_heavy["tx"].get_vsize())
        self.check_mempool([tx_v3_parent_normal["txid"], tx_v3_child_almost_heavy["txid"]])
        assert_equal(node.getmempoolentry(tx_v3_parent_normal["txid"])["descendantcount"], 2)
        tx_v3_child_almost_heavy_rbf = self.wallet.send_self_transfer(
            from_node=node,
            fee_rate=DEFAULT_FEE * 2,
            utxo_to_spend=tx_v3_parent_normal["new_utxo"],
            target_vsize=875,
            version=3
        )
        assert_greater_than_or_equal(tx_v3_child_almost_heavy["tx"].get_vsize() + tx_v3_child_almost_heavy_rbf["tx"].get_vsize(),
                                     TRUC_CHILD_MAX_VSIZE)
        self.check_mempool([tx_v3_parent_normal["txid"], tx_v3_child_almost_heavy_rbf["txid"]])
        assert_equal(node.getmempoolentry(tx_v3_parent_normal["txid"])["descendantcount"], 2)

    @cleanup(extra_args=None)
    def test_truc_replacement(self):
        node = self.nodes[0]
        self.log.info("Test TRUC transactions may be replaced by TRUC transactions")
        utxo_v3_bip125 = self.wallet.get_utxo()
        tx_v3_bip125 = self.wallet.send_self_transfer(
            from_node=node,
            fee_rate=DEFAULT_FEE,
            utxo_to_spend=utxo_v3_bip125,
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

        self.log.info("Test TRUC transactions may be replaced by non-TRUC (BIP125) transactions")
        tx_v3_bip125_rbf_v2 = self.wallet.send_self_transfer(
            from_node=node,
            fee_rate=DEFAULT_FEE * 3,
            utxo_to_spend=utxo_v3_bip125,
            version=2
        )
        self.check_mempool([tx_v3_bip125_rbf_v2["txid"]])

        self.log.info("Test that replacements cannot cause violation of inherited TRUC")
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
        expected_error_v2_v3 = f"TRUC-violation, non-version=3 tx {tx_v3_child_rbf_v2['txid']} (wtxid={tx_v3_child_rbf_v2['wtxid']}) cannot spend from version=3 tx {tx_v3_parent['txid']} (wtxid={tx_v3_parent['wtxid']})"
        assert_raises_rpc_error(-26, expected_error_v2_v3, node.sendrawtransaction, tx_v3_child_rbf_v2["hex"])
        self.check_mempool([tx_v3_bip125_rbf_v2["txid"], tx_v3_parent["txid"], tx_v3_child["txid"]])


    @cleanup()
    def test_truc_reorg(self):
        node = self.nodes[0]
        self.log.info("Test that, during a reorg, TRUC rules are not enforced")
        self.check_mempool([])

        # Testing 2<-3 versions allowed
        tx_v2_block = self.wallet.create_self_transfer(version=2)

        # Testing 3<-2 versions allowed
        tx_v3_block = self.wallet.create_self_transfer(version=3)

        # Testing overly-large child size
        tx_v3_block2 = self.wallet.create_self_transfer(version=3)

        # Also create a linear chain of 3 TRUC transactions that will be directly mined, followed by one v2 in-mempool after block is made
        tx_chain_1 = self.wallet.create_self_transfer(version=3)
        tx_chain_2 = self.wallet.create_self_transfer(utxo_to_spend=tx_chain_1["new_utxo"], version=3)
        tx_chain_3 = self.wallet.create_self_transfer(utxo_to_spend=tx_chain_2["new_utxo"], version=3)

        tx_to_mine = [tx_v3_block["hex"], tx_v2_block["hex"], tx_v3_block2["hex"], tx_chain_1["hex"], tx_chain_2["hex"], tx_chain_3["hex"]]
        block = self.generateblock(node, output="raw(42)", transactions=tx_to_mine)

        self.check_mempool([])
        tx_v2_from_v3 = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=tx_v3_block["new_utxo"], version=2)
        tx_v3_from_v2 = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=tx_v2_block["new_utxo"], version=3)
        tx_v3_child_large = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=tx_v3_block2["new_utxo"], target_vsize=1250, version=3)
        assert_greater_than(node.getmempoolentry(tx_v3_child_large["txid"])["vsize"], TRUC_CHILD_MAX_VSIZE)
        tx_chain_4 = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=tx_chain_3["new_utxo"], version=2)
        self.check_mempool([tx_v2_from_v3["txid"], tx_v3_from_v2["txid"], tx_v3_child_large["txid"], tx_chain_4["txid"]])

        # Reorg should have all block transactions re-accepted, ignoring TRUC enforcement
        node.invalidateblock(block["hash"])
        self.check_mempool([tx_v3_block["txid"], tx_v2_block["txid"], tx_v3_block2["txid"], tx_v2_from_v3["txid"], tx_v3_from_v2["txid"], tx_v3_child_large["txid"], tx_chain_1["txid"], tx_chain_2["txid"], tx_chain_3["txid"], tx_chain_4["txid"]])

    @cleanup(extra_args=["-limitclustercount=1"])
    def test_nondefault_package_limits(self):
        """
        Max standard tx size + TRUC rules imply the cluster rules (at their default
        values), but those checks must not be skipped. Ensure both sets of checks are done by
        changing the cluster limit configurations.
        """
        node = self.nodes[0]
        self.log.info("Test that a decreased cluster count limit also applies to TRUC child")
        parent_target_vsize = 9990
        child_target_vsize = 500
        tx_v3_parent_large1 = self.wallet.send_self_transfer(
            from_node=node,
            target_vsize=parent_target_vsize,
            version=3
        )
        tx_v3_child_large1 = self.wallet.create_self_transfer(
            utxo_to_spend=tx_v3_parent_large1["new_utxo"],
            target_vsize=child_target_vsize,
            version=3
        )

        # Parent and child are within v3 limits, but cluster count limit is exceeded.
        assert_greater_than_or_equal(TRUC_MAX_VSIZE, tx_v3_parent_large1["tx"].get_vsize())
        assert_greater_than_or_equal(TRUC_CHILD_MAX_VSIZE, tx_v3_child_large1["tx"].get_vsize())

        assert_raises_rpc_error(-26, "too-large-cluster", node.sendrawtransaction, tx_v3_child_large1["hex"])
        self.check_mempool([tx_v3_parent_large1["txid"]])
        assert_equal(node.getmempoolentry(tx_v3_parent_large1["txid"])["descendantcount"], 1)
        self.generate(node, 1)

        self.log.info("Test that a decreased limitclustersize also applies to TRUC child")
        self.restart_node(0, extra_args=["-limitclustersize=10", "-acceptnonstdtxn=1"])
        tx_v3_parent_large2 = self.wallet.send_self_transfer(from_node=node, target_vsize=parent_target_vsize, version=3)
        tx_v3_child_large2 = self.wallet.create_self_transfer(utxo_to_spend=tx_v3_parent_large2["new_utxo"], target_vsize=child_target_vsize, version=3)
        # Parent and child are within TRUC limits
        assert_greater_than_or_equal(TRUC_MAX_VSIZE, tx_v3_parent_large2["tx"].get_vsize())
        assert_greater_than_or_equal(TRUC_CHILD_MAX_VSIZE, tx_v3_child_large2["tx"].get_vsize())
        assert_raises_rpc_error(-26, "too-large-cluster", node.sendrawtransaction, tx_v3_child_large2["hex"])
        self.check_mempool([tx_v3_parent_large2["txid"]])

    @cleanup()
    def test_truc_ancestors_package(self):
        self.log.info("Test that TRUC ancestor limits are checked within the package")
        node = self.nodes[0]
        tx_v3_parent_normal = self.wallet.create_self_transfer(
            fee_rate=0,
            target_vsize=1001,
            version=3
        )
        tx_v3_parent_2_normal = self.wallet.create_self_transfer(
            fee_rate=0,
            target_vsize=1001,
            version=3
        )
        tx_v3_child_multiparent = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[tx_v3_parent_normal["new_utxo"], tx_v3_parent_2_normal["new_utxo"]],
            fee_per_output=10000,
            version=3
        )
        tx_v3_child_heavy = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[tx_v3_parent_normal["new_utxo"]],
            target_vsize=TRUC_CHILD_MAX_VSIZE + 1,
            fee_per_output=10000,
            version=3
        )

        self.check_mempool([])
        result = node.submitpackage([tx_v3_parent_normal["hex"], tx_v3_parent_2_normal["hex"], tx_v3_child_multiparent["hex"]])
        assert_equal(result['package_msg'], f"TRUC-violation, tx {tx_v3_child_multiparent['txid']} (wtxid={tx_v3_child_multiparent['wtxid']}) would have too many ancestors")
        self.check_mempool([])

        self.check_mempool([])
        result = node.submitpackage([tx_v3_parent_normal["hex"], tx_v3_child_heavy["hex"]])
        # tx_v3_child_heavy is heavy based on vsize, not sigops.
        assert_equal(result['package_msg'], f"TRUC-violation, version=3 child tx {tx_v3_child_heavy['txid']} (wtxid={tx_v3_child_heavy['wtxid']}) is too big: {tx_v3_child_heavy['tx'].get_vsize()} > 1000 virtual bytes")
        self.check_mempool([])

        tx_v3_parent = self.wallet.create_self_transfer(version=3)
        tx_v3_child = self.wallet.create_self_transfer(utxo_to_spend=tx_v3_parent["new_utxo"], version=3)
        tx_v3_grandchild = self.wallet.create_self_transfer(utxo_to_spend=tx_v3_child["new_utxo"], version=3)
        result = node.testmempoolaccept([tx_v3_parent["hex"], tx_v3_child["hex"], tx_v3_grandchild["hex"]])
        assert all([txresult["package-error"] == f"TRUC-violation, tx {tx_v3_grandchild['txid']} (wtxid={tx_v3_grandchild['wtxid']}) would have too many ancestors" for txresult in result])

    @cleanup(extra_args=None)
    def test_truc_ancestors_package_and_mempool(self):
        """
        A TRUC transaction in a package cannot have 2 TRUC parents.
        Test that if we have a transaction graph A -> B -> C, where A, B, C are
        all TRUC transactions, that we cannot use submitpackage to get the
        transactions all into the mempool.

        Verify, in particular, that if A is already in the mempool, then
        submitpackage(B, C) will fail.
        """
        node = self.nodes[0]
        self.log.info("Test that TRUC ancestor limits include transactions within the package and all in-mempool ancestors")
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
        assert_equal(result['package_msg'], f"TRUC-violation, tx {tx_child_violator['txid']} (wtxid={tx_child_violator['wtxid']}) would have too many ancestors")
        self.check_mempool([tx_in_mempool["txid"]])

    @cleanup(extra_args=None)
    def test_sibling_eviction_package(self):
        """
        When a transaction has a mempool sibling, it may be eligible for sibling eviction.
        However, this option is only available in single transaction acceptance. It doesn't work in
        a multi-testmempoolaccept (where RBF is disabled) or when doing package CPFP.
        """
        self.log.info("Test TRUC sibling eviction in submitpackage and multi-testmempoolaccept")
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
        assert_equal(result_test_unrelated[0]["reject-reason"], "TRUC-violation")

        # Fails in a package via testmempoolaccept
        result_test_1p1c = node.testmempoolaccept([tx_sibling_1["hex"], tx_has_mempool_uncle["hex"]])
        assert_equal(result_test_1p1c[0]["reject-reason"], "TRUC-violation")

        # Allowed when tx is submitted in a package and evaluated individually.
        # Note that the child failed since it would be the 3rd generation.
        result_package_indiv = node.submitpackage([tx_sibling_1["hex"], tx_has_mempool_uncle["hex"]])
        self.check_mempool([tx_mempool_parent["txid"], tx_sibling_1["txid"]])
        expected_error_gen3 = f"TRUC-violation, tx {tx_has_mempool_uncle['txid']} (wtxid={tx_has_mempool_uncle['wtxid']}) would have too many ancestors"

        assert_equal(result_package_indiv["tx-results"][tx_has_mempool_uncle['wtxid']]['error'], expected_error_gen3)

        # Allowed when tx is submitted in a package with in-mempool parent (which is deduplicated).
        node.submitpackage([tx_mempool_parent["hex"], tx_sibling_2["hex"]])
        self.check_mempool([tx_mempool_parent["txid"], tx_sibling_2["txid"]])

        # Child cannot pay for sibling eviction for parent, as it violates TRUC topology limits
        result_package_cpfp = node.submitpackage([tx_sibling_3["hex"], tx_bumps_parent_with_sibling["hex"]])
        self.check_mempool([tx_mempool_parent["txid"], tx_sibling_2["txid"]])
        expected_error_cpfp = f"TRUC-violation, tx {tx_mempool_parent['txid']} (wtxid={tx_mempool_parent['wtxid']}) would exceed descendant count limit"

        assert_equal(result_package_cpfp["tx-results"][tx_sibling_3['wtxid']]['error'], expected_error_cpfp)


    @cleanup()
    def test_truc_package_inheritance(self):
        self.log.info("Test that TRUC inheritance is checked within package")
        node = self.nodes[0]
        tx_v3_parent = self.wallet.create_self_transfer(
            fee_rate=0,
            target_vsize=1001,
            version=3
        )
        tx_v2_child = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[tx_v3_parent["new_utxo"]],
            fee_per_output=10000,
            version=2
        )
        self.check_mempool([])
        result = node.submitpackage([tx_v3_parent["hex"], tx_v2_child["hex"]])
        assert_equal(result['package_msg'], f"TRUC-violation, non-version=3 tx {tx_v2_child['txid']} (wtxid={tx_v2_child['wtxid']}) cannot spend from version=3 tx {tx_v3_parent['txid']} (wtxid={tx_v3_parent['wtxid']})")
        self.check_mempool([])

    @cleanup(extra_args=None)
    def test_truc_in_testmempoolaccept(self):
        node = self.nodes[0]

        self.log.info("Test that TRUC inheritance is accurately assessed in testmempoolaccept")
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
        expected_error_v3_from_v2 = f"TRUC-violation, version=3 tx {tx_v3_from_v2['txid']} (wtxid={tx_v3_from_v2['wtxid']}) cannot spend from non-version=3 tx {tx_v2['txid']} (wtxid={tx_v2['wtxid']})"
        assert all([result["package-error"] == expected_error_v3_from_v2 for result in test_accept_v3_from_v2])

        test_accept_v2_from_v3 = node.testmempoolaccept([tx_v3["hex"], tx_v2_from_v3["hex"]])
        expected_error_v2_from_v3 = f"TRUC-violation, non-version=3 tx {tx_v2_from_v3['txid']} (wtxid={tx_v2_from_v3['wtxid']}) cannot spend from version=3 tx {tx_v3['txid']} (wtxid={tx_v3['wtxid']})"
        assert all([result["package-error"] == expected_error_v2_from_v3 for result in test_accept_v2_from_v3])

        test_accept_pairs = node.testmempoolaccept([tx_v2["hex"], tx_v3["hex"], tx_v2_from_v2["hex"], tx_v3_from_v3["hex"]])
        assert all([result["allowed"] for result in test_accept_pairs])

        self.log.info("Test that descendant violations are caught in testmempoolaccept")
        tx_v3_independent = self.wallet.create_self_transfer(version=3)
        tx_v3_parent = self.wallet.create_self_transfer_multi(num_outputs=2, version=3)
        tx_v3_child_1 = self.wallet.create_self_transfer(utxo_to_spend=tx_v3_parent["new_utxos"][0], version=3)
        tx_v3_child_2 = self.wallet.create_self_transfer(utxo_to_spend=tx_v3_parent["new_utxos"][1], version=3)
        test_accept_2children = node.testmempoolaccept([tx_v3_parent["hex"], tx_v3_child_1["hex"], tx_v3_child_2["hex"]])
        expected_error_2children = f"TRUC-violation, tx {tx_v3_parent['txid']} (wtxid={tx_v3_parent['wtxid']}) would exceed descendant count limit"
        assert all([result["package-error"] == expected_error_2children for result in test_accept_2children])

        # Extra TRUC transaction does not get incorrectly marked as extra descendant
        test_accept_1child_with_exra = node.testmempoolaccept([tx_v3_parent["hex"], tx_v3_child_1["hex"], tx_v3_independent["hex"]])
        assert all([result["allowed"] for result in test_accept_1child_with_exra])

        # Extra TRUC transaction does not make us ignore the extra descendant
        test_accept_2children_with_exra = node.testmempoolaccept([tx_v3_parent["hex"], tx_v3_child_1["hex"], tx_v3_child_2["hex"], tx_v3_independent["hex"]])
        expected_error_extra = f"TRUC-violation, tx {tx_v3_parent['txid']} (wtxid={tx_v3_parent['wtxid']}) would exceed descendant count limit"
        assert all([result["package-error"] == expected_error_extra for result in test_accept_2children_with_exra])
        # Same result if the parent is already in mempool
        node.sendrawtransaction(tx_v3_parent["hex"])
        test_accept_2children_with_in_mempool_parent = node.testmempoolaccept([tx_v3_child_1["hex"], tx_v3_child_2["hex"]])
        assert all([result["package-error"] == expected_error_extra for result in test_accept_2children_with_in_mempool_parent])

    @cleanup(extra_args=None)
    def test_reorg_2child_rbf(self):
        node = self.nodes[0]
        self.log.info("Test that children of a TRUC transaction can be replaced individually, even if there are multiple due to reorg")

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
        assert_not_equal(child_1_conflict["txid"], child_1["txid"])
        self.check_mempool([ancestor_tx["txid"], child_1_conflict["txid"], child_2["txid"]])
        assert_equal(node.getmempoolentry(ancestor_tx["txid"])["descendantcount"], 3)

    @cleanup(extra_args=None)
    def test_truc_sibling_eviction(self):
        self.log.info("Test sibling eviction for TRUC")
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
        expected_error_2siblings = f"TRUC-violation, tx {tx_with_multi_children['txid']} (wtxid={tx_with_multi_children['wtxid']}) would exceed descendant count limit"
        assert_raises_rpc_error(-26, expected_error_2siblings, node.sendrawtransaction, tx_with_sibling3["hex"])

        # However, an RBF (with conflicting inputs) is possible even if the resulting cluster size exceeds 2
        tx_with_sibling3_rbf = self.wallet.send_self_transfer(
            from_node=node,
            version=3,
            utxo_to_spend=tx_with_multi_children["new_utxos"][0],
            fee_rate=DEFAULT_FEE*50
        )
        self.check_mempool([tx_with_multi_children["txid"], tx_with_sibling3_rbf["txid"], tx_with_sibling2["txid"]])

    @cleanup(extra_args=None)
    def test_minrelay_in_package_combos(self):
        node = self.nodes[0]
        self.log.info("Test that only TRUC transactions can be under minrelaytxfee for various settings...")

        for minrelay_setting in (0, 5, 10, 100, 500, 1000, 5000, 333333, 2500000):
            self.log.info(f"-> Test -minrelaytxfee={minrelay_setting}sat/kvB...")
            setting_decimal = minrelay_setting / Decimal(COIN)
            self.restart_node(0, extra_args=[f"-minrelaytxfee={setting_decimal:.8f}", "-persistmempool=0"])
            minrelayfeerate = node.getmempoolinfo()["minrelaytxfee"]
            high_feerate = minrelayfeerate * 50

            tx_v3_0fee_parent = self.wallet.create_self_transfer(fee=0, fee_rate=0, confirmed_only=True, version=3)
            tx_v3_child = self.wallet.create_self_transfer(utxo_to_spend=tx_v3_0fee_parent["new_utxo"], fee_rate=high_feerate, version=3)
            total_v3_fee = tx_v3_child["fee"] + tx_v3_0fee_parent["fee"]
            total_v3_size = tx_v3_child["tx"].get_vsize() + tx_v3_0fee_parent["tx"].get_vsize()
            assert_greater_than_or_equal(total_v3_fee, get_fee(total_v3_size, minrelayfeerate))
            if minrelayfeerate > 0:
                assert_greater_than(get_fee(tx_v3_0fee_parent["tx"].get_vsize(), minrelayfeerate), 0)
                # Always need to pay at least 1 satoshi for entry, even if minimum feerate is very low
                assert_greater_than(total_v3_fee, 0)
                # Also create a version where the child is at minrelaytxfee
                tx_v3_child_minrelay = self.wallet.create_self_transfer(utxo_to_spend=tx_v3_0fee_parent["new_utxo"], fee_rate=minrelayfeerate, version=3)
                result_truc_minrelay = node.submitpackage([tx_v3_0fee_parent["hex"], tx_v3_child_minrelay["hex"]])
                assert_equal(result_truc_minrelay["package_msg"], "transaction failed")

            tx_v2_0fee_parent = self.wallet.create_self_transfer(fee=0, fee_rate=0, confirmed_only=True, version=2)
            tx_v2_child = self.wallet.create_self_transfer(utxo_to_spend=tx_v2_0fee_parent["new_utxo"], fee_rate=high_feerate, version=2)
            total_v2_fee = tx_v2_child["fee"] + tx_v2_0fee_parent["fee"]
            total_v2_size = tx_v2_child["tx"].get_vsize() + tx_v2_0fee_parent["tx"].get_vsize()
            assert_greater_than_or_equal(total_v2_fee, get_fee(total_v2_size, minrelayfeerate))
            if minrelayfeerate > 0:
                assert_greater_than(get_fee(tx_v2_0fee_parent["tx"].get_vsize(), minrelayfeerate), 0)
                # Always need to pay at least 1 satoshi for entry, even if minimum feerate is very low
                assert_greater_than(total_v2_fee, 0)

            result_truc = node.submitpackage([tx_v3_0fee_parent["hex"], tx_v3_child["hex"]], maxfeerate=0)
            assert_equal(result_truc["package_msg"], "success")

            result_non_truc = node.submitpackage([tx_v2_0fee_parent["hex"], tx_v2_child["hex"]], maxfeerate=0)
            if minrelayfeerate > 0:
                assert_equal(result_non_truc["package_msg"], "transaction failed")
                min_fee_parent = int(get_fee(tx_v2_0fee_parent["tx"].get_vsize(), minrelayfeerate) * COIN)
                assert_equal(result_non_truc["tx-results"][tx_v2_0fee_parent["wtxid"]]["error"], f"min relay fee not met, 0 < {min_fee_parent}")
                self.check_mempool([tx_v3_0fee_parent["txid"], tx_v3_child["txid"]])
            else:
                assert_equal(result_non_truc["package_msg"], "success")
                self.check_mempool([tx_v2_0fee_parent["txid"], tx_v2_child["txid"], tx_v3_0fee_parent["txid"], tx_v3_child["txid"]])


    def run_test(self):
        self.log.info("Generate blocks to create UTXOs")
        node = self.nodes[0]
        self.wallet = MiniWallet(node)
        self.generate(self.wallet, 200)
        self.test_truc_max_vsize()
        self.test_truc_acceptance()
        self.test_truc_replacement()
        self.test_truc_reorg()
        self.test_nondefault_package_limits()
        self.test_truc_ancestors_package()
        self.test_truc_ancestors_package_and_mempool()
        self.test_sibling_eviction_package()
        self.test_truc_package_inheritance()
        self.test_truc_in_testmempoolaccept()
        self.test_reorg_2child_rbf()
        self.test_truc_sibling_eviction()
        self.test_reorg_sibling_eviction_1p2c()
        self.test_minrelay_in_package_combos()


if __name__ == "__main__":
    MempoolTRUC(__file__).main()
