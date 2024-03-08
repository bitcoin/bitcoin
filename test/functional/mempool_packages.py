#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test mempool clusters"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet

# custom limits for node1
CUSTOM_CLUSTER_LIMIT = 10
DEFAULT_CLUSTER_LIMIT = 100

class MempoolPackagesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [
            [
                "-maxorphantx=1000",
                "-whitelist=noban@127.0.0.1",  # immediate tx relay
            ],
            [
                "-maxorphantx=1000",
                "-limitclustercount={}".format(CUSTOM_CLUSTER_LIMIT),
            ],
        ]

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])
        self.wallet.rescan_utxos()

        # First create 5 parent txs with 10 children
        txs_with_children = []
        NUM_PARENT_TXS = 5
        for i in range(NUM_PARENT_TXS):
            txs_with_children.append(self.wallet.send_self_transfer_multi(from_node=self.nodes[0], num_outputs=10))

        parent_transactions = [ t["txid"] for t in txs_with_children ]
        transaction_packages = []
        for t in txs_with_children:
            transaction_packages.extend(t["new_utxos"])

        # Sign and send up to CLUSTER_LIMIT transactions
        chain = [] # save sent txs for the purpose of checking node1's mempool later (see below)
        for _ in range(DEFAULT_CLUSTER_LIMIT - NUM_PARENT_TXS - 1):
            utxo = transaction_packages.pop(0)
            new_tx = self.wallet.send_self_transfer_multi(from_node=self.nodes[0], num_outputs=1, utxos_to_spend=[utxo])
            txid = new_tx["txid"]
            chain.append(txid)
            transaction_packages.extend(new_tx["new_utxos"])

        # Create one more transaction that spends all remaining utxos
        new_tx = self.wallet.send_self_transfer_multi(from_node=self.nodes[0], num_outputs=10, utxos_to_spend=transaction_packages)
        chain.append(new_tx["txid"])

        mempool = self.nodes[0].getrawmempool(True)

        for tx in chain:
            assert tx in mempool

        #assert self.nodes[0].getmempoolinfo()["maxclustercount"] == DEFAULT_CLUSTER_LIMIT

        self.wait_until(lambda: len(self.nodes[1].getrawmempool()) == CUSTOM_CLUSTER_LIMIT*NUM_PARENT_TXS, timeout=10)
        # Check that node1's mempool is as expected, containing:
        # - parent tx for descendant test
        # - txs chained off parent tx (-> custom descendant limit)
        #assert self.nodes[1].getmempoolinfo()["maxclustercount"] == CUSTOM_CLUSTER_LIMIT
        #assert self.nodes[1].getmempoolinfo()["numberofclusters"] == NUM_PARENT_TXS

        mempool0 = self.nodes[0].getrawmempool(False)
        mempool1 = self.nodes[1].getrawmempool(False)
        assert set(mempool1).issubset(set(mempool0))

        for p in parent_transactions:
            assert p in mempool1
            entry = self.nodes[1].getmempoolentry(p)
            cluster = self.nodes[1].getmempoolcluster(entry["clusterid"])
            assert cluster["txcount"] == CUSTOM_CLUSTER_LIMIT

        # Test reorg handling
        # First, the basics:
        self.generate(self.nodes[0], 1)
        self.nodes[1].invalidateblock(self.nodes[0].getbestblockhash())
        self.nodes[1].reconsiderblock(self.nodes[0].getbestblockhash())

        # Now test the case where node1 has a transaction T in its mempool that
        # depends on transactions A and B which are in a mined block, and the
        # block containing A and B is disconnected, AND B is not accepted back
        # into node1's mempool because its cluster count is too high.

        # Create 10 transactions, like so:
        # Tx0 -> Tx1 (vout0)
        #   \--> Tx2 (vout1) -> Tx3 -> Tx4 -> Tx5 -> Tx6 -> Tx7 -> Tx8 -> Tx9
        #
        # Mine them in the next block, then generate a new tx8 that spends
        # Tx1 and Tx7, and add to node1's mempool, then disconnect the
        # last block.

        # Create tx0 with 2 outputs
        tx0 = self.wallet.send_self_transfer_multi(from_node=self.nodes[0], num_outputs=2)

        # Create tx1
        tx1 = self.wallet.send_self_transfer(from_node=self.nodes[0], utxo_to_spend=tx0["new_utxos"][0])

        # Create tx2-9
        txs = self.wallet.send_self_transfer_chain(from_node=self.nodes[0], utxo_to_spend=tx0["new_utxos"][1], chain_length=8)
        tx9 = txs[-1]

        # Check the ancestors and descendants look right.
        ancestors = self.nodes[0].getmempoolancestors(tx9["txid"])
        expected_ancestors = [t["txid"] for t in [tx0] + txs[0:-1]]
        assert sorted(ancestors) == sorted(expected_ancestors)

        descendants = self.nodes[0].getmempooldescendants(tx0["txid"])
        expected_descendants = [t["txid"] for t in [tx1] + txs]
        assert sorted(descendants) == sorted(expected_descendants)

        # Check the fee/size chunks match the totals
        chunks = self.nodes[0].getmempoolfeesize()
        # Check that the sum of the fees matches the total
        fee_sum = sum([c["fee"] for c in chunks])
        assert fee_sum == self.nodes[0].getmempoolinfo()["total_fee"]

        # Mine these in a block
        self.generate(self.nodes[0], 1)

        # Now generate tx10, with a big fee
        tx10 = self.wallet.send_self_transfer_multi(from_node=self.nodes[0], utxos_to_spend=[tx1["new_utxo"], tx9["new_utxo"]], fee_per_output=40000)
        self.sync_mempools()

        # Now try to disconnect the tip on each node...
        self.nodes[1].invalidateblock(self.nodes[1].getbestblockhash())
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())
        self.sync_blocks()

        # ensure that the cluster limits are still enforced on node1.
        #assert self.nodes[1].getmempoolinfo()["maxclustercount"] == CUSTOM_CLUSTER_LIMIT
        entry  = self.nodes[1].getmempoolentry(tx0["txid"])
        cluster = self.nodes[1].getmempoolcluster(entry["clusterid"])
        assert cluster["txcount"] == CUSTOM_CLUSTER_LIMIT
        assert tx10["txid"] not in self.nodes[1].getrawmempool()
        assert tx10["txid"] in self.nodes[0].getrawmempool()

if __name__ == '__main__':
    MempoolPackagesTest().main()
