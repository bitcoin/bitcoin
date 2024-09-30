#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool limiting together/eviction with the wallet."""

from decimal import Decimal

from test_framework.mempool_util import (
    fill_mempool,
)
from test_framework.p2p import P2PTxInvStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_fee_amount,
    assert_greater_than,
    assert_raises_rpc_error,
)
from test_framework.wallet import (
    COIN,
    DEFAULT_FEE,
    MiniWallet,
)


class MempoolLimitTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [[
            "-datacarriersize=100000",
            "-maxmempool=5",
        ]]
        self.supports_cli = False

    def test_rbf_carveout_disallowed(self):
        node = self.nodes[0]

        self.log.info("Check that individually-evaluated transactions in a package don't increase package limits for other subpackage parts")

        # We set chain limits to 2 ancestors, 1 descendant, then try to get a parents-and-child chain of 2 in mempool
        #
        # A: Solo transaction to be RBF'd (to bump descendant limit for package later)
        # B: First transaction in package, RBFs A by itself under individual evaluation, which would give it +1 descendant limit
        # C: Second transaction in package, spends B. If the +1 descendant limit persisted, would make it into mempool

        self.restart_node(0, extra_args=self.extra_args[0] + ["-limitancestorcount=2", "-limitdescendantcount=1"])

        # Generate a confirmed utxo we will double-spend
        rbf_utxo = self.wallet.send_self_transfer(
            from_node=node,
            confirmed_only=True
        )["new_utxo"]
        self.generate(node, 1)

        # tx_A needs to be RBF'd, set minfee at set size
        A_vsize = 250
        mempoolmin_feerate = node.getmempoolinfo()["mempoolminfee"]
        tx_A = self.wallet.send_self_transfer(
            from_node=node,
            fee_rate=mempoolmin_feerate,
            target_vsize=A_vsize,
            utxo_to_spend=rbf_utxo,
            confirmed_only=True
        )

        # RBF's tx_A, is not yet submitted
        tx_B = self.wallet.create_self_transfer(
            fee=tx_A["fee"] * 4,
            target_vsize=A_vsize,
            utxo_to_spend=rbf_utxo,
            confirmed_only=True
        )

        # Spends tx_B's output, too big for cpfp carveout (because that would also increase the descendant limit by 1)
        non_cpfp_carveout_vsize = 10001  # EXTRA_DESCENDANT_TX_SIZE_LIMIT + 1
        tx_C = self.wallet.create_self_transfer(
            target_vsize=non_cpfp_carveout_vsize,
            fee_rate=mempoolmin_feerate,
            utxo_to_spend=tx_B["new_utxo"],
            confirmed_only=True
        )
        res = node.submitpackage([tx_B["hex"], tx_C["hex"]])
        assert_equal(res["package_msg"], "transaction failed")
        assert "too-long-mempool-chain" in res["tx-results"][tx_C["wtxid"]]["error"]

    def test_mid_package_eviction(self):
        node = self.nodes[0]
        self.log.info("Check a package where each parent passes the current mempoolminfee but would cause eviction before package submission terminates")

        self.restart_node(0, extra_args=self.extra_args[0])

        # Restarting the node resets mempool minimum feerate
        assert_equal(node.getmempoolinfo()['minrelaytxfee'], Decimal('0.00001000'))
        assert_equal(node.getmempoolinfo()['mempoolminfee'], Decimal('0.00001000'))

        fill_mempool(self, node)
        current_info = node.getmempoolinfo()
        mempoolmin_feerate = current_info["mempoolminfee"]

        package_hex = []
        # UTXOs to be spent by the ultimate child transaction
        parent_utxos = []

        evicted_vsize = 2000
        # Mempool transaction which is evicted due to being at the "bottom" of the mempool when the
        # mempool overflows and evicts by descendant score. It's important that the eviction doesn't
        # happen in the middle of package evaluation, as it can invalidate the coins cache.
        mempool_evicted_tx = self.wallet.send_self_transfer(
            from_node=node,
            fee_rate=mempoolmin_feerate,
            target_vsize=evicted_vsize,
            confirmed_only=True
        )
        # Already in mempool when package is submitted.
        assert mempool_evicted_tx["txid"] in node.getrawmempool()

        # This parent spends the above mempool transaction that exists when its inputs are first
        # looked up, but disappears later. It is rejected for being too low fee (but eligible for
        # reconsideration), and its inputs are cached. When the mempool transaction is evicted, its
        # coin is no longer available, but the cache could still contains the tx.
        cpfp_parent = self.wallet.create_self_transfer(
            utxo_to_spend=mempool_evicted_tx["new_utxo"],
            fee_rate=mempoolmin_feerate - Decimal('0.00001'),
            confirmed_only=True)
        package_hex.append(cpfp_parent["hex"])
        parent_utxos.append(cpfp_parent["new_utxo"])
        assert_equal(node.testmempoolaccept([cpfp_parent["hex"]])[0]["reject-reason"], "mempool min fee not met")

        self.wallet.rescan_utxos()

        # Series of parents that don't need CPFP and are submitted individually. Each one is large and
        # high feerate, which means they should trigger eviction but not be evicted.
        parent_vsize = 25000
        num_big_parents = 3
        # Need to be large enough to trigger eviction
        # (note that the mempool usage of a tx is about three times its vsize)
        assert_greater_than(parent_vsize * num_big_parents * 3, current_info["maxmempool"] - current_info["bytes"])
        parent_feerate = 100 * mempoolmin_feerate

        big_parent_txids = []
        for i in range(num_big_parents):
            parent = self.wallet.create_self_transfer(fee_rate=parent_feerate, target_vsize=parent_vsize, confirmed_only=True)
            parent_utxos.append(parent["new_utxo"])
            package_hex.append(parent["hex"])
            big_parent_txids.append(parent["txid"])
            # There is room for each of these transactions independently
            assert node.testmempoolaccept([parent["hex"]])[0]["allowed"]

        # Create a child spending everything, bumping cpfp_parent just above mempool minimum
        # feerate. It's important not to bump too much as otherwise mempool_evicted_tx would not be
        # evicted, making this test much less meaningful.
        approx_child_vsize = self.wallet.create_self_transfer_multi(utxos_to_spend=parent_utxos)["tx"].get_vsize()
        cpfp_fee = (mempoolmin_feerate / 1000) * (cpfp_parent["tx"].get_vsize() + approx_child_vsize) - cpfp_parent["fee"]
        # Specific number of satoshis to fit within a small window. The parent_cpfp + child package needs to be
        # - When there is mid-package eviction, high enough feerate to meet the new mempoolminfee
        # - When there is no mid-package eviction, low enough feerate to be evicted immediately after submission.
        magic_satoshis = 1200
        cpfp_satoshis = int(cpfp_fee * COIN) + magic_satoshis

        child = self.wallet.create_self_transfer_multi(utxos_to_spend=parent_utxos, fee_per_output=cpfp_satoshis)
        package_hex.append(child["hex"])

        # Package should be submitted, temporarily exceeding maxmempool, and then evicted.
        with node.assert_debug_log(expected_msgs=["rolling minimum fee bumped"]):
            assert_equal(node.submitpackage(package_hex)["package_msg"], "transaction failed")

        # Maximum size must never be exceeded.
        assert_greater_than(node.getmempoolinfo()["maxmempool"], node.getmempoolinfo()["bytes"])

        # Evicted transaction and its descendants must not be in mempool.
        resulting_mempool_txids = node.getrawmempool()
        assert mempool_evicted_tx["txid"] not in resulting_mempool_txids
        assert cpfp_parent["txid"] not in resulting_mempool_txids
        assert child["txid"] not in resulting_mempool_txids
        for txid in big_parent_txids:
            assert txid in resulting_mempool_txids

    def test_mid_package_replacement(self):
        node = self.nodes[0]
        self.log.info("Check a package where an early tx depends on a later-replaced mempool tx")

        self.restart_node(0, extra_args=self.extra_args[0])

        # Restarting the node resets mempool minimum feerate
        assert_equal(node.getmempoolinfo()['minrelaytxfee'], Decimal('0.00001000'))
        assert_equal(node.getmempoolinfo()['mempoolminfee'], Decimal('0.00001000'))

        fill_mempool(self, node)
        current_info = node.getmempoolinfo()
        mempoolmin_feerate = current_info["mempoolminfee"]

        # Mempool transaction which is evicted due to being at the "bottom" of the mempool when the
        # mempool overflows and evicts by descendant score. It's important that the eviction doesn't
        # happen in the middle of package evaluation, as it can invalidate the coins cache.
        double_spent_utxo = self.wallet.get_utxo(confirmed_only=True)
        replaced_tx = self.wallet.send_self_transfer(
            from_node=node,
            utxo_to_spend=double_spent_utxo,
            fee_rate=mempoolmin_feerate,
            confirmed_only=True
        )
        # Already in mempool when package is submitted.
        assert replaced_tx["txid"] in node.getrawmempool()

        # This parent spends the above mempool transaction that exists when its inputs are first
        # looked up, but disappears later. It is rejected for being too low fee (but eligible for
        # reconsideration), and its inputs are cached. When the mempool transaction is evicted, its
        # coin is no longer available, but the cache could still contain the tx.
        cpfp_parent = self.wallet.create_self_transfer(
            utxo_to_spend=replaced_tx["new_utxo"],
            fee_rate=mempoolmin_feerate - Decimal('0.00001'),
            confirmed_only=True)

        self.wallet.rescan_utxos()

        # Parent that replaces the parent of cpfp_parent.
        replacement_tx = self.wallet.create_self_transfer(
            utxo_to_spend=double_spent_utxo,
            fee_rate=10*mempoolmin_feerate,
            confirmed_only=True
        )
        parent_utxos = [cpfp_parent["new_utxo"], replacement_tx["new_utxo"]]

        # Create a child spending everything, CPFPing the low-feerate parent.
        approx_child_vsize = self.wallet.create_self_transfer_multi(utxos_to_spend=parent_utxos)["tx"].get_vsize()
        cpfp_fee = (2 * mempoolmin_feerate / 1000) * (cpfp_parent["tx"].get_vsize() + approx_child_vsize) - cpfp_parent["fee"]
        child = self.wallet.create_self_transfer_multi(utxos_to_spend=parent_utxos, fee_per_output=int(cpfp_fee * COIN))
        # It's very important that the cpfp_parent is before replacement_tx so that its input (from
        # replaced_tx) is first looked up *before* replacement_tx is submitted.
        package_hex = [cpfp_parent["hex"], replacement_tx["hex"], child["hex"]]

        # Package should be submitted, temporarily exceeding maxmempool, and then evicted.
        res = node.submitpackage(package_hex)
        assert_equal(res["package_msg"], "transaction failed")
        assert len([tx_res for _, tx_res in res["tx-results"].items() if "error" in tx_res and tx_res["error"] == "bad-txns-inputs-missingorspent"])

        # Maximum size must never be exceeded.
        assert_greater_than(node.getmempoolinfo()["maxmempool"], node.getmempoolinfo()["bytes"])

        resulting_mempool_txids = node.getrawmempool()
        # The replacement should be successful.
        assert replacement_tx["txid"] in resulting_mempool_txids
        # The replaced tx and all of its descendants must not be in mempool.
        assert replaced_tx["txid"] not in resulting_mempool_txids
        assert cpfp_parent["txid"] not in resulting_mempool_txids
        assert child["txid"] not in resulting_mempool_txids


    def run_test(self):
        node = self.nodes[0]
        self.wallet = MiniWallet(node)
        miniwallet = self.wallet

        # Generate coins needed to create transactions in the subtests (excluding coins used in fill_mempool).
        self.generate(miniwallet, 20)

        relayfee = node.getnetworkinfo()['relayfee']
        self.log.info('Check that mempoolminfee is minrelaytxfee')
        assert_equal(node.getmempoolinfo()['minrelaytxfee'], Decimal('0.00001000'))
        assert_equal(node.getmempoolinfo()['mempoolminfee'], Decimal('0.00001000'))

        fill_mempool(self, node)

        # Deliberately try to create a tx with a fee less than the minimum mempool fee to assert that it does not get added to the mempool
        self.log.info('Create a mempool tx that will not pass mempoolminfee')
        assert_raises_rpc_error(-26, "mempool min fee not met", miniwallet.send_self_transfer, from_node=node, fee_rate=relayfee)

        self.log.info("Check that submitpackage allows cpfp of a parent below mempool min feerate")
        node = self.nodes[0]
        peer = node.add_p2p_connection(P2PTxInvStore())

        # Package with 2 parents and 1 child. One parent has a high feerate due to modified fees,
        # another is below the mempool minimum feerate but bumped by the child.
        tx_poor = miniwallet.create_self_transfer(fee_rate=relayfee)
        tx_rich = miniwallet.create_self_transfer(fee=0, fee_rate=0)
        node.prioritisetransaction(tx_rich["txid"], 0, int(DEFAULT_FEE * COIN))
        package_txns = [tx_rich, tx_poor]
        coins = [tx["new_utxo"] for tx in package_txns]
        tx_child = miniwallet.create_self_transfer_multi(utxos_to_spend=coins, fee_per_output=10000) #DEFAULT_FEE
        package_txns.append(tx_child)

        submitpackage_result = node.submitpackage([tx["hex"] for tx in package_txns])
        assert_equal(submitpackage_result["package_msg"], "success")

        rich_parent_result = submitpackage_result["tx-results"][tx_rich["wtxid"]]
        poor_parent_result = submitpackage_result["tx-results"][tx_poor["wtxid"]]
        child_result = submitpackage_result["tx-results"][tx_child["tx"].getwtxid()]
        assert_fee_amount(poor_parent_result["fees"]["base"], tx_poor["tx"].get_vsize(), relayfee)
        assert_equal(rich_parent_result["fees"]["base"], 0)
        assert_equal(child_result["fees"]["base"], DEFAULT_FEE)
        # The "rich" parent does not require CPFP so its effective feerate is just its individual feerate.
        assert_fee_amount(DEFAULT_FEE, tx_rich["tx"].get_vsize(), rich_parent_result["fees"]["effective-feerate"])
        assert_equal(rich_parent_result["fees"]["effective-includes"], [tx_rich["wtxid"]])
        # The "poor" parent and child's effective feerates are the same, composed of their total
        # fees divided by their combined vsize.
        package_fees = poor_parent_result["fees"]["base"] + child_result["fees"]["base"]
        package_vsize = tx_poor["tx"].get_vsize() + tx_child["tx"].get_vsize()
        assert_fee_amount(package_fees, package_vsize, poor_parent_result["fees"]["effective-feerate"])
        assert_fee_amount(package_fees, package_vsize, child_result["fees"]["effective-feerate"])
        assert_equal([tx_poor["wtxid"], tx_child["tx"].getwtxid()], poor_parent_result["fees"]["effective-includes"])
        assert_equal([tx_poor["wtxid"], tx_child["tx"].getwtxid()], child_result["fees"]["effective-includes"])

        # The node will broadcast each transaction, still abiding by its peer's fee filter
        peer.wait_for_broadcast([tx["tx"].getwtxid() for tx in package_txns])

        self.log.info("Check a package that passes mempoolminfee but is evicted immediately after submission")
        mempoolmin_feerate = node.getmempoolinfo()["mempoolminfee"]
        current_mempool = node.getrawmempool(verbose=False)
        worst_feerate_btcvb = Decimal("21000000")
        for txid in current_mempool:
            entry = node.getmempoolentry(txid)
            worst_feerate_btcvb = min(worst_feerate_btcvb, entry["fees"]["descendant"] / entry["descendantsize"])
        # Needs to be large enough to trigger eviction
        # (note that the mempool usage of a tx is about three times its vsize)
        target_vsize_each = 50000
        assert_greater_than(target_vsize_each * 2 * 3, node.getmempoolinfo()["maxmempool"] - node.getmempoolinfo()["bytes"])
        # Should be a true CPFP: parent's feerate is just below mempool min feerate
        parent_feerate = mempoolmin_feerate - Decimal("0.000001")  # 0.1 sats/vbyte below min feerate
        # Parent + child is above mempool minimum feerate
        child_feerate = (worst_feerate_btcvb * 1000) - Decimal("0.000001")  # 0.1 sats/vbyte below worst feerate
        # However, when eviction is triggered, these transactions should be at the bottom.
        # This assertion assumes parent and child are the same size.
        miniwallet.rescan_utxos()
        tx_parent_just_below = miniwallet.create_self_transfer(fee_rate=parent_feerate, target_vsize=target_vsize_each)
        tx_child_just_above = miniwallet.create_self_transfer(utxo_to_spend=tx_parent_just_below["new_utxo"], fee_rate=child_feerate, target_vsize=target_vsize_each)
        # This package ranks below the lowest descendant package in the mempool
        package_fee = tx_parent_just_below["fee"] + tx_child_just_above["fee"]
        package_vsize = tx_parent_just_below["tx"].get_vsize() + tx_child_just_above["tx"].get_vsize()
        assert_greater_than(worst_feerate_btcvb, package_fee / package_vsize)
        assert_greater_than(mempoolmin_feerate, tx_parent_just_below["fee"] / (tx_parent_just_below["tx"].get_vsize()))
        assert_greater_than(package_fee / package_vsize, mempoolmin_feerate / 1000)
        res = node.submitpackage([tx_parent_just_below["hex"], tx_child_just_above["hex"]])
        for wtxid in [tx_parent_just_below["wtxid"], tx_child_just_above["wtxid"]]:
            assert_equal(res["tx-results"][wtxid]["error"], "mempool full")

        self.log.info('Test passing a value below the minimum (5 MB) to -maxmempool throws an error')
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(["-maxmempool=4"], "Error: -maxmempool must be at least 5 MB")

        self.test_mid_package_replacement()
        self.test_mid_package_eviction()
        self.test_rbf_carveout_disallowed()


if __name__ == '__main__':
    MempoolLimitTest(__file__).main()
