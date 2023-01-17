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
        self.num_nodes = 1
        self.extra_args = [[
            "-datacarriersize=100000",
            "-maxmempool=5",
        ]]
        self.supports_cli = False

    def run_test(self):
        txouts = gen_return_txouts()
        node = self.nodes[0]
        miniwallet = MiniWallet(node)
        relayfee = node.getnetworkinfo()['relayfee']

        self.log.info('Check that mempoolminfee is minrelaytxfee')
        assert_equal(node.getmempoolinfo()['minrelaytxfee'], Decimal('0.00001000'))
        assert_equal(node.getmempoolinfo()['mempoolminfee'], Decimal('0.00001000'))

        tx_batch_size = 25
        num_of_batches = 3
        # Generate UTXOs to flood the mempool
        # 1 to create a tx initially that will be evicted from the mempool later
        # 3 batches of multiple transactions with a fee rate much higher than the previous UTXO
        # And 1 more to verify that this tx does not get added to the mempool with a fee rate less than the mempoolminfee
        # And 2 more for the package cpfp test
        self.generate(miniwallet, 1 + (num_of_batches * tx_batch_size) + 1 + 2)

        # Mine 99 blocks so that the UTXOs are allowed to be spent
        self.generate(node, COINBASE_MATURITY - 1)

        self.log.info('Create a mempool tx that will be evicted')
        tx_to_be_evicted_id = miniwallet.send_self_transfer(from_node=node, fee_rate=relayfee)["txid"]

        # Increase the tx fee rate to give the subsequent transactions a higher priority in the mempool
        # The tx has an approx. vsize of 65k, i.e. multiplying the previous fee rate (in sats/kvB)
        # by 130 should result in a fee that corresponds to 2x of that fee rate
        base_fee = relayfee * 130

        self.log.info("Fill up the mempool with txs with higher fee rate")
        for batch_of_txid in range(num_of_batches):
            fee = (batch_of_txid + 1) * base_fee
            create_lots_of_big_transactions(miniwallet, node, fee, tx_batch_size, txouts)

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
        self.generate(node, 1)

        self.log.info('Test passing a value below the minimum (5 MB) to -maxmempool throws an error')
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(["-maxmempool=4"], "Error: -maxmempool must be at least 5 MB")


if __name__ == '__main__':
    MempoolLimitTest().main()
