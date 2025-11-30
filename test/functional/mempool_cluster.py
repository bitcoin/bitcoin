#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test cluster mempool accessors and limits"""

from decimal import Decimal

from test_framework.mempool_util import (
    DEFAULT_CLUSTER_LIMIT,
    DEFAULT_CLUSTER_SIZE_LIMIT_KVB,
)
from test_framework.messages import (
    COIN,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import (
    MiniWallet,
)
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
)

def weight_to_vsize(weight):
    # Divide by 4, round up
    return (weight + 3) // 4

def cleanup(func):
    def wrapper(self, *args, **kwargs):
        try:
            func(self, *args, **kwargs)
        finally:
            # Mine blocks to clear the mempool and replenish the wallet's confirmed UTXOs.
            while (len(self.nodes[0].getrawmempool()) > 0):
                self.generate(self.nodes[0], 1)
            self.wallet.rescan_utxos(include_mempool=True)
    return wrapper

class MempoolClusterTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def add_chain_cluster(self, node, cluster_count, target_vsize=None):
        """Create a cluster of transactions, with the count specified.
        The topology is a chain: the i'th transaction depends on the (i-1)'th transaction.
        Optionally provide a target_vsize for each transaction.
        """
        parent_tx = self.wallet.send_self_transfer(from_node=node, confirmed_only=True, target_vsize=target_vsize)
        utxo_to_spend = parent_tx["new_utxo"]
        all_txids = [parent_tx["txid"]]
        all_results = [parent_tx]

        while len(all_results) < cluster_count:
            next_tx = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=utxo_to_spend, target_vsize=target_vsize)
            assert next_tx["txid"] in node.getrawmempool()

            # Confirm that each transaction is in the same cluster as the first.
            assert_equal(node.getmempoolcluster(next_tx['txid']), node.getmempoolcluster(parent_tx['txid']))

            # Confirm that the ancestors are what we expect
            mempool_ancestors = node.getmempoolancestors(next_tx['txid'])
            assert_equal(sorted(mempool_ancestors), sorted(all_txids))

            # Confirm that each successive transaction is added as a descendant.
            assert all([ next_tx["txid"] in node.getmempooldescendants(x) for x in all_txids ])

            # Update for next iteration
            all_results.append(next_tx)
            all_txids.append(next_tx["txid"])
            utxo_to_spend = next_tx["new_utxo"]

        assert node.getmempoolcluster(parent_tx['txid'])['txcount'] == cluster_count
        return all_results

    def check_feerate_diagram(self, node):
        """Sanity check the feerate diagram."""
        feeratediagram = node.getmempoolfeeratediagram()
        last_val = {"weight": 0, "fee": 0}
        for x in feeratediagram:
            # The weight is always positive, except for the first iteration
            assert x['weight'] > 0 or x['fee'] == 0
            # Monotonically decreasing fee per weight
            assert_greater_than_or_equal(last_val['fee'] * x['weight'], x['fee'] * last_val['weight'])
            last_val = x

    def test_limit_enforcement(self, cluster_submitted, target_vsize_per_tx=None):
        """
        the cluster may change as a result of these transactions, so cluster_submitted is mutated accordingly
        """
        # Cluster has already been submitted and has at least 3 transactions, otherwise this test won't work.
        assert_greater_than_or_equal(len(cluster_submitted), 3)
        node = self.nodes[0]
        last_result = cluster_submitted[-1]

        # Test that adding one more transaction to the cluster will fail.
        bad_tx = self.wallet.create_self_transfer(utxo_to_spend=last_result["new_utxo"], target_vsize=target_vsize_per_tx)
        assert_raises_rpc_error(-26, "too-large-cluster", node.sendrawtransaction, bad_tx["hex"])

        # It should also limit cluster sizes during replacement
        utxo_to_double_spend = self.wallet.get_utxo(confirmed_only=True)
        fee = Decimal("0.000001")
        tx_to_replace = self.wallet.create_self_transfer(utxo_to_spend=utxo_to_double_spend, fee=fee)
        node.sendrawtransaction(tx_to_replace["hex"])

        # Multiply fee by 5, which should easily cover the cost to replace (but
        # is still too large a cluster). Otherwise, use the target vsize at
        # 10sat/vB
        fee_to_use = target_vsize_per_tx * 10 if target_vsize_per_tx is not None else int(fee * COIN * 5)
        bad_tx_also_replacement = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[last_result["new_utxo"], utxo_to_double_spend],
            target_vsize=target_vsize_per_tx,
            fee_per_output=fee_to_use,
        )
        assert_raises_rpc_error(-26, "too-large-cluster", node.sendrawtransaction, bad_tx_also_replacement["hex"])

        # Replace the last transaction. We are extending the cluster by one, but also removing one: 64 + 1 - 1 = 64
        # In the case of vsize, it should similarly cancel out.
        second_to_last_utxo = cluster_submitted[-2]["new_utxo"]
        fee_to_beat = cluster_submitted[-1]["fee"]
        vsize_to_use = cluster_submitted[-1]["tx"].get_vsize() if target_vsize_per_tx is not None else None
        good_tx_replacement = self.wallet.create_self_transfer(utxo_to_spend=second_to_last_utxo, fee=fee_to_beat * 5, target_vsize=vsize_to_use)
        node.sendrawtransaction(good_tx_replacement["hex"], maxfeerate=0)

        cluster_submitted[-1] = good_tx_replacement

    def test_limit_enforcement_package(self, cluster_submitted):
        node = self.nodes[0]
        # Create a package from the second to last transaction. This shouldn't work because the effect is 64 + 2 - 1 = 65
        last_utxo = cluster_submitted[-2]["new_utxo"]
        fee_to_beat = cluster_submitted[-1]["fee"]
        # We do not use package RBF here because it has additional restrictions on mempool ancestors.
        parent_tx_bad = self.wallet.create_self_transfer(utxo_to_spend=last_utxo, fee=fee_to_beat * 5)
        child_tx_bad = self.wallet.create_self_transfer(utxo_to_spend=parent_tx_bad["new_utxo"])
        # The parent should be submitted, but the child rejected.
        result_parent_only = node.submitpackage([parent_tx_bad["hex"], child_tx_bad["hex"]])

        assert parent_tx_bad["txid"] in node.getrawmempool()
        assert child_tx_bad["txid"] not in node.getrawmempool()
        assert_equal(result_parent_only["package_msg"], "transaction failed")
        assert_equal(result_parent_only["tx-results"][child_tx_bad["wtxid"]]["error"], "too-large-cluster")

        # Now, create a package from the second to last transaction. This should work because the effect is 64 + 2 - 2 = 64
        third_to_last_utxo = cluster_submitted[-3]["new_utxo"]
        parent_tx_good = self.wallet.create_self_transfer(utxo_to_spend=third_to_last_utxo)
        child_tx_good = self.wallet.create_self_transfer(utxo_to_spend=parent_tx_good["new_utxo"], fee=fee_to_beat * 5)
        result_both_good = node.submitpackage([parent_tx_good["hex"], child_tx_good["hex"]], maxfeerate=0)
        assert_equal(result_both_good["package_msg"], "success")
        assert parent_tx_good["txid"] in node.getrawmempool()
        assert child_tx_good["txid"] in node.getrawmempool()

    @cleanup
    def test_cluster_count_limit(self, max_cluster_count):
        node = self.nodes[0]
        cluster_submitted = self.add_chain_cluster(node, max_cluster_count)
        self.check_feerate_diagram(node)
        for result in cluster_submitted:
            assert_equal(node.getmempoolcluster(result["txid"])['txcount'], max_cluster_count)

        self.log.info("Test that cluster count limit is enforced")
        self.test_limit_enforcement(cluster_submitted)
        self.log.info("Test that the resulting cluster count is correctly calculated in a package")
        self.test_limit_enforcement_package(cluster_submitted)

    @cleanup
    def test_cluster_size_limit(self, max_cluster_size_vbytes):
        node = self.nodes[0]
        # This number should be smaller than the cluster count limit.
        num_txns = 10
        # Leave some buffer so it is possible to add a reasonably-sized transaction.
        target_vsize_per_tx = int((max_cluster_size_vbytes - 500) / num_txns)
        cluster_submitted = self.add_chain_cluster(node, num_txns, target_vsize_per_tx)

        vsize_remaining = max_cluster_size_vbytes - weight_to_vsize(node.getmempoolcluster(cluster_submitted[0]["txid"])['clusterweight'])
        self.log.info("Test that cluster size limit is enforced")
        self.test_limit_enforcement(cluster_submitted, target_vsize_per_tx=vsize_remaining + 4)

        # Try another cluster and add a small transaction: it should succeed
        last_result = cluster_submitted[-1]
        small_tx = self.wallet.create_self_transfer(utxo_to_spend=last_result["new_utxo"], target_vsize=vsize_remaining)
        node.sendrawtransaction(small_tx["hex"])

    @cleanup
    def test_cluster_merging(self, max_cluster_count):
        node = self.nodes[0]

        self.log.info(f"Test merging 2 clusters with transaction counts totaling {max_cluster_count}")
        for num_txns_cluster1 in [1, 5, 10]:
            # Create a chain of transactions
            cluster1 = self.add_chain_cluster(node, num_txns_cluster1)
            for result in cluster1:
                node.sendrawtransaction(result["hex"])
            utxo_from_cluster1 = cluster1[-1]["new_utxo"]

            # Make the next cluster, which contains the remaining transactions
            assert_greater_than(max_cluster_count, num_txns_cluster1)
            num_txns_cluster2 = max_cluster_count - num_txns_cluster1
            cluster2 = self.add_chain_cluster(node, num_txns_cluster2)
            for result in cluster2:
                node.sendrawtransaction(result["hex"])
            utxo_from_cluster2 = cluster2[-1]["new_utxo"]

            # Now create a transaction that spends from both clusters, which would merge them.
            tx_merger = self.wallet.create_self_transfer_multi(utxos_to_spend=[utxo_from_cluster1, utxo_from_cluster2])
            assert_raises_rpc_error(-26, "too-large-cluster", node.sendrawtransaction, tx_merger["hex"])

            # Spending from the clusters independently should work
            tx_spending_cluster1 = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=utxo_from_cluster1)
            tx_spending_cluster2 = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=utxo_from_cluster2)
            assert tx_spending_cluster1["txid"] in node.getrawmempool()
            assert tx_spending_cluster2["txid"] in node.getrawmempool()

        self.log.info(f"Test merging {max_cluster_count} clusters with 1 transaction spending from all of them")
        utxos_to_merge = []
        for _ in range(max_cluster_count):
            # Use a confirmed utxo to ensure distinct clusters
            confirmed_utxo = self.wallet.get_utxo(confirmed_only=True)
            singleton = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=confirmed_utxo)
            assert singleton["txid"] in node.getrawmempool()
            utxos_to_merge.append(singleton["new_utxo"])

        assert_equal(len(utxos_to_merge), max_cluster_count)
        tx_merger = self.wallet.create_self_transfer_multi(utxos_to_spend=utxos_to_merge)
        assert_raises_rpc_error(-26, "too-large-cluster", node.sendrawtransaction, tx_merger["hex"])

        # Spending from 1 fewer cluster should work
        tx_merger_all_but_one = self.wallet.create_self_transfer_multi(utxos_to_spend=utxos_to_merge[:-1])
        node.sendrawtransaction(tx_merger_all_but_one["hex"])
        assert tx_merger_all_but_one["txid"] in node.getrawmempool()

    @cleanup
    def test_cluster_merging_size(self, max_cluster_size_vbytes):
        node = self.nodes[0]

        self.log.info(f"Test merging clusters with sizes totaling {max_cluster_size_vbytes} vB")
        num_txns = 10
        # Leave some buffer so it is possible to add a reasonably-sized transaction.
        utxos_to_merge = []
        vsize_remaining = max_cluster_size_vbytes
        for _ in range(num_txns):
            confirmed_utxo = self.wallet.get_utxo(confirmed_only=True)
            singleton = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=confirmed_utxo)
            assert singleton["txid"] in node.getrawmempool()
            utxos_to_merge.append(singleton["new_utxo"])
            vsize_remaining -= singleton["tx"].get_vsize()

        assert_greater_than_or_equal(vsize_remaining, 500)

        # Create a transaction spending from all clusters that exceeds the cluster size limit.
        tx_merger_too_big = self.wallet.create_self_transfer_multi(utxos_to_spend=utxos_to_merge, target_vsize=vsize_remaining + 4, fee_per_output=10000)
        assert_raises_rpc_error(-26, "too-large-cluster", node.sendrawtransaction, tx_merger_too_big["hex"])

        # A transaction that is slightly smaller should work.
        tx_merger_small = self.wallet.create_self_transfer_multi(utxos_to_spend=utxos_to_merge[:-1], target_vsize=vsize_remaining - 4, fee_per_output=10000)
        node.sendrawtransaction(tx_merger_small["hex"])
        assert tx_merger_small["txid"] in node.getrawmempool()

    @cleanup
    def test_cluster_limit_rbf(self, max_cluster_count):
        node = self.nodes[0]

        # Use min feerate for the to-be-replaced transactions. There are many, so replacement cost can be expensive.
        min_feerate = node.getmempoolinfo()["mempoolminfee"]

        self.log.info("Test that cluster size calculation takes RBF into account")
        utxos_created_by_parents = []
        fees_rbf_sats = 0
        for _ in range(max_cluster_count - 1):
            parent_tx = self.wallet.send_self_transfer(from_node=node, confirmed_only=True)
            utxo_to_replace = parent_tx["new_utxo"]
            child_tx = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=utxo_to_replace, fee_rate=min_feerate)

            fees_rbf_sats += int(child_tx["fee"] * COIN)
            utxos_created_by_parents.append(utxo_to_replace)

        # This transaction would create a cluster of size max_cluster_count
        # Importantly, the node should account for the fact that half of the transactions will be replaced.
        tx_merger_replacer = self.wallet.create_self_transfer_multi(utxos_to_spend=utxos_created_by_parents, fee_per_output=fees_rbf_sats * 2)
        node.sendrawtransaction(tx_merger_replacer["hex"])
        assert tx_merger_replacer["txid"] in node.getrawmempool()
        assert_equal(node.getmempoolcluster(tx_merger_replacer["txid"])['txcount'], max_cluster_count)

        self.log.info("Test that cluster size calculation takes package RBF into account")
        utxos_to_replace = []
        fee_rbf_decimal = 0
        for _ in range(max_cluster_count):
            confirmed_utxo = self.wallet.get_utxo(confirmed_only=True)
            tx_to_replace = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=confirmed_utxo, fee_rate=min_feerate)
            fee_rbf_decimal += tx_to_replace["fee"]
            utxos_to_replace.append(confirmed_utxo)

        tx_replacer = self.wallet.create_self_transfer_multi(utxos_to_spend=utxos_to_replace)
        tx_replacer_sponsor = self.wallet.create_self_transfer(utxo_to_spend=tx_replacer["new_utxos"][0], fee=fee_rbf_decimal * 2)
        node.submitpackage([tx_replacer["hex"], tx_replacer_sponsor["hex"]], maxfeerate=0)
        assert tx_replacer["txid"] in node.getrawmempool()
        assert tx_replacer_sponsor["txid"] in node.getrawmempool()
        assert_equal(node.getmempoolcluster(tx_replacer["txid"])['txcount'], 2)

    @cleanup
    def test_getmempoolcluster(self):
        node = self.nodes[0]

        self.log.info("Testing getmempoolcluster")

        assert_equal(node.getrawmempool(), [])

        # Not in-mempool
        not_mempool_tx = self.wallet.create_self_transfer()
        assert_raises_rpc_error(-5, "Transaction not in mempool", node.getmempoolcluster, not_mempool_tx["txid"])

        # Test that chunks are being recomputed properly

        # One chunk with one tx
        first_chunk_tx = self.wallet.send_self_transfer(from_node=node)
        first_chunk_info = node.getmempoolcluster(first_chunk_tx["txid"])
        assert_equal(first_chunk_info, {'clusterweight': first_chunk_tx["tx"].get_weight(), 'txcount': 1, 'chunks': [{'chunkfee': first_chunk_tx["fee"], 'chunkweight': first_chunk_tx["tx"].get_weight(), 'txs': [first_chunk_tx["txid"]]}]})

        # Another unconnected tx, nothing should change
        self.wallet.send_self_transfer(from_node=node)
        first_chunk_info = node.getmempoolcluster(first_chunk_tx["txid"])
        assert_equal(first_chunk_info, {'clusterweight': first_chunk_tx["tx"].get_weight(), 'txcount': 1, 'chunks': [{'chunkfee': first_chunk_tx["fee"], 'chunkweight': first_chunk_tx["tx"].get_weight(), 'txs': [first_chunk_tx["txid"]]}]})

        # Second connected tx, makes one chunk still with high enough fee
        second_chunk_tx = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=first_chunk_tx["new_utxo"], fee_rate=Decimal("0.01"))
        first_chunk_info = node.getmempoolcluster(first_chunk_tx["txid"])
        # output is same across same cluster transactions
        assert_equal(first_chunk_info, node.getmempoolcluster(second_chunk_tx["txid"]))
        chunkweight = first_chunk_tx["tx"].get_weight() + second_chunk_tx["tx"].get_weight()
        chunkfee = first_chunk_tx["fee"] + second_chunk_tx["fee"]
        assert_equal(first_chunk_info, {'clusterweight': chunkweight, 'txcount': 2, 'chunks': [{'chunkfee': chunkfee, 'chunkweight': chunkweight, 'txs': [first_chunk_tx["txid"], second_chunk_tx["txid"]]}]})

        # Third connected tx, makes one chunk still with high enough fee
        third_chunk_tx = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=second_chunk_tx["new_utxo"], fee_rate=Decimal("0.1"))
        first_chunk_info = node.getmempoolcluster(first_chunk_tx["txid"])
        # output is same across same cluster transactions
        assert_equal(first_chunk_info, node.getmempoolcluster(third_chunk_tx["txid"]))
        chunkweight = first_chunk_tx["tx"].get_weight() + second_chunk_tx["tx"].get_weight() + third_chunk_tx["tx"].get_weight()
        chunkfee = first_chunk_tx["fee"] + second_chunk_tx["fee"] + third_chunk_tx["fee"]
        assert_equal(first_chunk_info, {'clusterweight': chunkweight, 'txcount': 3, 'chunks': [{'chunkfee': chunkfee, 'chunkweight': chunkweight, 'txs': [first_chunk_tx["txid"], second_chunk_tx["txid"], third_chunk_tx["txid"]]}]})

        # Now test single cluster with each tx being its own chunk

        # One chunk with one tx
        first_chunk_tx = self.wallet.send_self_transfer(from_node=node)
        first_chunk_info = node.getmempoolcluster(first_chunk_tx["txid"])
        assert_equal(first_chunk_info, {'clusterweight': first_chunk_tx["tx"].get_weight(), 'txcount': 1, 'chunks': [{'chunkfee': first_chunk_tx["fee"], 'chunkweight': first_chunk_tx["tx"].get_weight(), 'txs': [first_chunk_tx["txid"]]}]})

        # Second connected tx, lower fee
        second_chunk_tx = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=first_chunk_tx["new_utxo"], fee_rate=Decimal("0.000002"))
        first_chunk_info = node.getmempoolcluster(first_chunk_tx["txid"])
        # output is same across same cluster transactions
        assert_equal(first_chunk_info, node.getmempoolcluster(second_chunk_tx["txid"]))
        first_chunkweight = first_chunk_tx["tx"].get_weight()
        second_chunkweight = second_chunk_tx["tx"].get_weight()
        assert_equal(first_chunk_info, {'clusterweight': first_chunkweight + second_chunkweight, 'txcount': 2, 'chunks': [{'chunkfee': first_chunk_tx["fee"], 'chunkweight': first_chunkweight, 'txs': [first_chunk_tx["txid"]]}, {'chunkfee': second_chunk_tx["fee"], 'chunkweight': second_chunkweight, 'txs': [second_chunk_tx["txid"]]}]})

        # Third connected tx, even lower fee
        third_chunk_tx = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=second_chunk_tx["new_utxo"], fee_rate=Decimal("0.000001"))
        first_chunk_info = node.getmempoolcluster(first_chunk_tx["txid"])
        # output is same across same cluster transactions
        assert_equal(first_chunk_info, node.getmempoolcluster(third_chunk_tx["txid"]))
        first_chunkweight = first_chunk_tx["tx"].get_weight()
        second_chunkweight = second_chunk_tx["tx"].get_weight()
        third_chunkweight = third_chunk_tx["tx"].get_weight()
        chunkfee = first_chunk_tx["fee"] + second_chunk_tx["fee"] + third_chunk_tx["fee"]
        assert_equal(first_chunk_info, {'clusterweight': first_chunkweight + second_chunkweight + third_chunkweight, 'txcount': 3, 'chunks': [{'chunkfee': first_chunk_tx["fee"], 'chunkweight': first_chunkweight, 'txs': [first_chunk_tx["txid"]]}, {'chunkfee': second_chunk_tx["fee"], 'chunkweight': second_chunkweight, 'txs': [second_chunk_tx["txid"]]}, {'chunkfee': third_chunk_tx["fee"], 'chunkweight': third_chunkweight, 'txs': [third_chunk_tx["txid"]]}]})

        # If we prioritise the last transaction it can join the second transaction's chunk.
        node.prioritisetransaction(third_chunk_tx["txid"], 0, int(third_chunk_tx["fee"]*COIN) + 1)
        first_chunk_info = node.getmempoolcluster(first_chunk_tx["txid"])
        assert_equal(first_chunk_info, {'clusterweight': first_chunkweight + second_chunkweight + third_chunkweight, 'txcount': 3, 'chunks': [{'chunkfee': first_chunk_tx["fee"], 'chunkweight': first_chunkweight, 'txs': [first_chunk_tx["txid"]]}, {'chunkfee': second_chunk_tx["fee"] + 2*third_chunk_tx["fee"] + Decimal("0.00000001"), 'chunkweight': second_chunkweight + third_chunkweight, 'txs': [second_chunk_tx["txid"], third_chunk_tx["txid"]]}]})

    def run_test(self):
        node = self.nodes[0]
        self.wallet = MiniWallet(node)
        self.generate(self.wallet, 400)

        self.test_getmempoolcluster()

        self.test_cluster_limit_rbf(DEFAULT_CLUSTER_LIMIT)

        for cluster_size_limit_kvb in [10, 20, 33, 100, DEFAULT_CLUSTER_SIZE_LIMIT_KVB]:
            self.log.info(f"-> Resetting node with -limitclustersize={cluster_size_limit_kvb}")
            self.restart_node(0, extra_args=[f"-limitclustersize={cluster_size_limit_kvb}"])

            cluster_size_limit = cluster_size_limit_kvb * 1000
            self.test_cluster_size_limit(cluster_size_limit)
            self.test_cluster_merging_size(cluster_size_limit)

        for cluster_count_limit in [4, 10, 16, 32, DEFAULT_CLUSTER_LIMIT]:
            self.log.info(f"-> Resetting node with -limitclustercount={cluster_count_limit}")
            self.restart_node(0, extra_args=[f"-limitclustercount={cluster_count_limit}"])

            self.test_cluster_count_limit(cluster_count_limit)
            if cluster_count_limit > 10:
                self.test_cluster_merging(cluster_count_limit)


if __name__ == '__main__':
    MempoolClusterTest(__file__).main()
