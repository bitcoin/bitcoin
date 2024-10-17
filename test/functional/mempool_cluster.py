#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test cluster mempool accessors and limits"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import (
    MiniWallet,
)
from test_framework.util import (
    assert_raises_rpc_error,
)

MAX_CLUSTER_COUNT = 100

class MempoolClusterTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        self.wallet = MiniWallet(node)

        node = self.nodes[0]
        parent_tx = self.wallet.send_self_transfer(from_node=node)
        utxo_to_spend = parent_tx["new_utxo"]
        ancestors = [parent_tx["txid"]]
        while len(node.getrawmempool()) < MAX_CLUSTER_COUNT:
            next_tx = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=utxo_to_spend)
            # Confirm that each transaction is in the same cluster as the first.
            assert node.getmempoolentry(next_tx['txid'])['clusterid'] == node.getmempoolentry(parent_tx['txid'])['clusterid']
            # Confirm that there is only one cluster in the mempool.
            assert node.getmempoolinfo()['numberofclusters'] == 1

            # Confirm that the ancestors are what we expect
            mempool_ancestors = node.getmempoolancestors(next_tx['txid'])
            assert sorted(mempool_ancestors) == sorted(ancestors)

            # Confirm that each successive transaction is added as a descendant.
            assert all([ next_tx["txid"] in node.getmempooldescendants(x) for x in ancestors ])

            # Update for next iteration
            ancestors.append(next_tx["txid"])
            utxo_to_spend = next_tx["new_utxo"]

        assert node.getmempoolinfo()['numberofclusters'] == 1
        assert node.getmempoolinfo()['maxclustercount'] == MAX_CLUSTER_COUNT
        assert node.getmempoolcluster(node.getmempoolentry(parent_tx['txid'])['clusterid'])['txcount'] == MAX_CLUSTER_COUNT
        feeratediagram = node.getmempoolfeeratediagram()
        last_val = [0, 0]
        for x in feeratediagram:
            assert x['size'] > 0
            assert last_val[0]*x['fee'] >= last_val[1]*x['size']
            last_val = [x['size'], x['fee']]

        # Test that adding one more transaction to the cluster will fail.
        bad_tx = self.wallet.create_self_transfer(utxo_to_spend=utxo_to_spend)
        assert_raises_rpc_error(-26, "too-large-cluster", node.sendrawtransaction, bad_tx["hex"])

        # TODO: verify that the size limits are also enforced.
        # TODO: add tests that exercise rbf, package submission, and package
        # rbf and verify that cluster limits are enforced.

if __name__ == '__main__':
    MempoolClusterTest(__file__).main()
