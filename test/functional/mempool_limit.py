#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool limiting together/eviction with the wallet."""

from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.p2p import P2PTxInvStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_fee_amount,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
    create_lots_of_big_transactions,
    gen_return_txouts,
)
from test_framework.wallet import (
    COIN,
    DEFAULT_FEE,
    MiniWallet,
)


class MempoolLimitTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [[
            "-datacarriersize=100000",
            "-maxmempool=5",
        ],
        [
            "-datacarriersize=100000",
        ]]
        self.supports_cli = False

    def run_test(self):
        txouts = gen_return_txouts()
        node = self.nodes[0]
        miniwallet = MiniWallet(node)
        relayfee = node.getnetworkinfo()['relayfee']
        all_transactions = []

        self.log.info('Check that mempoolminfee is minrelaytxfee')
        assert_equal(node.getmempoolinfo()['minrelaytxfee'], Decimal('0.00001000'))
        assert_equal(node.getmempoolinfo()['mempoolminfee'], Decimal('0.00001000'))

        # Batch size 1 is required to ensure every tx has a different feerate, which means the
        # selection of transaction(s) for eviction is identical across nodes.
        tx_batch_size = 1
        num_of_batches = 75
        # Generate UTXOs to flood the mempool
        # 1 to create a tx initially that will be evicted from the mempool later
        # 75 transactions, each with a fee rate much higher than the previous one
        # And 1 more to verify that this tx does not get added to the mempool with a fee rate less than the mempoolminfee
        # And 2 more for the package cpfp test
        # And 2 more for the package mempool full test
        self.generate(miniwallet, 1 + (num_of_batches * tx_batch_size) + 1 + 2)

        # Mine 99 blocks so that the UTXOs are allowed to be spent
        self.generate(node, COINBASE_MATURITY - 1)
        self.disconnect_nodes(0, 1)

        self.log.info('Create a mempool tx that will be evicted')
        tx_to_be_evicted = miniwallet.send_self_transfer(from_node=node, fee_rate=relayfee)
        tx_to_be_evicted_id = tx_to_be_evicted["txid"]
        all_transactions.append(tx_to_be_evicted["hex"])

        # Increase the tx fee rate to give the subsequent transactions a higher priority in the mempool
        # The tx has an approx. vsize of 65k, i.e. multiplying the previous fee rate (in sats/kvB)
        # by 130 should result in a fee that corresponds to 2x of that fee rate
        base_fee = relayfee * 130

        self.log.info("Fill up the mempool with txs with higher fee rate")
        for batch_of_txid in range(num_of_batches):
            fee = (batch_of_txid + 1) * base_fee
            all_transactions.extend(create_lots_of_big_transactions(miniwallet, node, fee, tx_batch_size, txouts)[1])

        self.log.info('The tx should be evicted by now')
        # The number of transactions created should be greater than the ones present in the mempool
        assert_greater_than(tx_batch_size * num_of_batches, len(node.getrawmempool()))
        # Initial tx created should not be present in the mempool anymore as it had a lower fee rate
        assert tx_to_be_evicted_id not in node.getrawmempool()

        self.log.info('Check that mempoolminfee is larger than minrelaytxfee')
        assert_equal(node.getmempoolinfo()['minrelaytxfee'], Decimal('0.00001000'))
        assert_greater_than(node.getmempoolinfo()['mempoolminfee'], Decimal('0.00001000'))

        # Deliberately try to create a tx with a fee less than the minimum mempool fee to assert that it does not get added to the mempool
        self.log.info('Create a mempool tx that will not pass mempoolminfee')
        tx_below_mempoolmin = miniwallet.create_self_transfer(fee_rate=relayfee)
        assert_raises_rpc_error(-26, "mempool min fee not met", node.sendrawtransaction, tx_below_mempoolmin["hex"])
        all_transactions.append(tx_below_mempoolmin["hex"])

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
        assert_greater_than_or_equal(poor_parent_result["fees"]["effective-feerate"], Decimal("0.0002"))

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
        target_weight_each = 200000
        assert_greater_than(target_weight_each * 2, node.getmempoolinfo()["maxmempool"] - node.getmempoolinfo()["bytes"])
        # Should be a true CPFP: parent's feerate is just below mempool min feerate
        parent_fee = (mempoolmin_feerate / 1000) * (target_weight_each // 4) - Decimal("0.00001")
        # Parent + child is above mempool minimum feerate
        child_fee = (worst_feerate_btcvb) * (target_weight_each // 4) - Decimal("0.00001")
        # However, when eviction is triggered, these transactions should be at the bottom.
        # This assertion assumes parent and child are the same size.
        miniwallet.rescan_utxos()
        tx_parent_just_below = miniwallet.create_self_transfer(fee=parent_fee, target_weight=target_weight_each)
        tx_child_just_above = miniwallet.create_self_transfer(utxo_to_spend=tx_parent_just_below["new_utxo"], fee=child_fee, target_weight=target_weight_each)
        # This package ranks below the lowest descendant package in the mempool
        assert_greater_than(worst_feerate_btcvb, (parent_fee + child_fee) / (tx_parent_just_below["tx"].get_vsize() + tx_child_just_above["tx"].get_vsize()))
        assert_greater_than(mempoolmin_feerate, (parent_fee) / (tx_parent_just_below["tx"].get_vsize()))
        assert_greater_than((parent_fee + child_fee) / (tx_parent_just_below["tx"].get_vsize() + tx_child_just_above["tx"].get_vsize()), mempoolmin_feerate / 1000)
        assert_raises_rpc_error(-26, "mempool full", node.submitpackage, [tx_parent_just_below["hex"], tx_child_just_above["hex"]])
        kept_txids = set(node.getrawmempool())

        self.log.info('Test restarting with smaller maxmempool persists the correct transactions.')
        # All transactions would make it in to a default size mempool:
        self.nodes[1].prioritisetransaction(tx_rich["txid"], 0, int(DEFAULT_FEE * COIN))
        for txhex in all_transactions:
            self.nodes[1].sendrawtransaction(txhex)
        self.nodes[1].submitpackage([tx["hex"] for tx in package_txns])
        node1_info = self.nodes[1].getmempoolinfo()
        assert_equal(node1_info["mempoolminfee"], node1_info["minrelaytxfee"])
        self.stop_node(1)
        # Upon restart, the mempool min fee will rise above 1sat/vB due to the limited capacity.
        self.restart_node(1, extra_args=["-maxmempool=5","-datacarriersize=100000"])
        node1_info_after_restart = self.nodes[1].getmempoolinfo()
        assert_greater_than(node1_info_after_restart["mempoolminfee"], node1_info_after_restart["minrelaytxfee"])
        assert_equal(set(self.nodes[1].getrawmempool()), kept_txids)

        txids_above_20 = set()
        for txid in kept_txids:
            entry = self.nodes[1].getmempoolentry(txid)
            descendant_feerate = Decimal(entry["fees"]["descendant"] / entry["descendantsize"])
            assert_greater_than_or_equal(descendant_feerate * 1000, node1_info_after_restart["mempoolminfee"])
            if (descendant_feerate * 1000 >= Decimal("0.0002")):
                txids_above_20.add(txid)
        assert all([tx["txid"] in txids_above_20 for tx in package_txns])

        self.log.info('Test restarting with higher -minrelaytxfee persists the correct transactions.')
        self.stop_node(1)
        self.restart_node(1, extra_args=["-minrelaytxfee=0.0002","-datacarriersize=100000"])
        assert_equal(set(self.nodes[1].getrawmempool()), txids_above_20)


        self.log.info('Test passing a value below the minimum (5 MB) to -maxmempool throws an error')
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(["-maxmempool=4"], "Error: -maxmempool must be at least 5 MB")


if __name__ == '__main__':
    MempoolLimitTest().main()
