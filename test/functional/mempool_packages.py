#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test descendant package tracking code."""

from decimal import Decimal

from test_framework.messages import (
    DEFAULT_ANCESTOR_LIMIT,
    DEFAULT_DESCENDANT_LIMIT,
)
from test_framework.p2p import P2PTxInvStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)
from test_framework.wallet import MiniWallet

# custom limits for node1
CUSTOM_DESCENDANT_LIMIT = 10

class MempoolPackagesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        # whitelist peers to speed up tx relay / mempool sync
        self.noban_tx_relay = True
        self.extra_args = [
            [
            ],
            [
                "-limitdescendantcount={}".format(CUSTOM_DESCENDANT_LIMIT),
                "-limitclustercount={}".format(CUSTOM_DESCENDANT_LIMIT),
            ],
        ]

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])
        self.wallet.rescan_utxos()

        peer_inv_store = self.nodes[0].add_p2p_connection(P2PTxInvStore()) # keep track of invs

        # DEFAULT_ANCESTOR_LIMIT transactions off a confirmed tx should be fine
        chain = self.wallet.create_self_transfer_chain(chain_length=DEFAULT_ANCESTOR_LIMIT)
        witness_chain = [t["wtxid"] for t in chain]
        ancestor_vsize = 0
        ancestor_fees = Decimal(0)

        for i, t in enumerate(chain):
            ancestor_vsize += t["tx"].get_vsize()
            ancestor_fees += t["fee"]
            self.wallet.sendrawtransaction(from_node=self.nodes[0], tx_hex=t["hex"])

        # Wait until mempool transactions have passed initial broadcast (sent inv and received getdata)
        # Otherwise, getrawmempool may be inconsistent with getmempoolentry if unbroadcast changes in between
        peer_inv_store.wait_for_broadcast(witness_chain)

        # Check mempool has DEFAULT_ANCESTOR_LIMIT transactions in it, and descendant and ancestor
        # count and fees should look correct
        mempool = self.nodes[0].getrawmempool(True)
        assert_equal(len(mempool), DEFAULT_ANCESTOR_LIMIT)
        descendant_count = 1
        descendant_fees = 0
        descendant_vsize = 0

        assert_equal(ancestor_vsize, sum([mempool[tx]['vsize'] for tx in mempool]))
        ancestor_count = DEFAULT_ANCESTOR_LIMIT
        assert_equal(ancestor_fees, sum([mempool[tx]['fees']['base'] for tx in mempool]))

        descendants = []
        ancestors = [t["txid"] for t in chain]
        chain = [t["txid"] for t in chain]
        for x in reversed(chain):
            # Check that getmempoolentry is consistent with getrawmempool
            entry = self.nodes[0].getmempoolentry(x)
            assert_equal(entry, mempool[x])

            # Check that gettxspendingprevout is consistent with getrawmempool
            witnesstx = self.nodes[0].getrawtransaction(txid=x, verbose=True)
            for tx_in in witnesstx["vin"]:
                spending_result = self.nodes[0].gettxspendingprevout([ {'txid' : tx_in["txid"], 'vout' : tx_in["vout"]} ])
                assert_equal(spending_result, [ {'txid' : tx_in["txid"], 'vout' : tx_in["vout"], 'spendingtxid' : x} ])

            # Check that the descendant calculations are correct
            assert_equal(entry['descendantcount'], descendant_count)
            descendant_fees += entry['fees']['base']
            assert_equal(entry['fees']['modified'], entry['fees']['base'])
            assert_equal(entry['fees']['descendant'], descendant_fees)
            descendant_vsize += entry['vsize']
            assert_equal(entry['descendantsize'], descendant_vsize)
            descendant_count += 1

            # Check that ancestor calculations are correct
            assert_equal(entry['ancestorcount'], ancestor_count)
            assert_equal(entry['fees']['ancestor'], ancestor_fees)
            assert_equal(entry['ancestorsize'], ancestor_vsize)
            ancestor_vsize -= entry['vsize']
            ancestor_fees -= entry['fees']['base']
            ancestor_count -= 1

            # Check that parent/child list is correct
            assert_equal(entry['spentby'], descendants[-1:])
            assert_equal(entry['depends'], ancestors[-2:-1])

            # Check that getmempooldescendants is correct
            assert_equal(sorted(descendants), sorted(self.nodes[0].getmempooldescendants(x)))

            # Check getmempooldescendants verbose output is correct
            for descendant, dinfo in self.nodes[0].getmempooldescendants(x, True).items():
                assert_equal(dinfo['depends'], [chain[chain.index(descendant)-1]])
                if dinfo['descendantcount'] > 1:
                    assert_equal(dinfo['spentby'], [chain[chain.index(descendant)+1]])
                else:
                    assert_equal(dinfo['spentby'], [])
            descendants.append(x)

            # Check that getmempoolancestors is correct
            ancestors.remove(x)
            assert_equal(sorted(ancestors), sorted(self.nodes[0].getmempoolancestors(x)))

            # Check that getmempoolancestors verbose output is correct
            for ancestor, ainfo in self.nodes[0].getmempoolancestors(x, True).items():
                assert_equal(ainfo['spentby'], [chain[chain.index(ancestor)+1]])
                if ainfo['ancestorcount'] > 1:
                    assert_equal(ainfo['depends'], [chain[chain.index(ancestor)-1]])
                else:
                    assert_equal(ainfo['depends'], [])


        # Check that getmempoolancestors/getmempooldescendants correctly handle verbose=true
        v_ancestors = self.nodes[0].getmempoolancestors(chain[-1], True)
        assert_equal(len(v_ancestors), len(chain)-1)
        for x in v_ancestors.keys():
            assert_equal(mempool[x], v_ancestors[x])
        assert chain[-1] not in v_ancestors.keys()

        v_descendants = self.nodes[0].getmempooldescendants(chain[0], True)
        assert_equal(len(v_descendants), len(chain)-1)
        for x in v_descendants.keys():
            assert_equal(mempool[x], v_descendants[x])
        assert chain[0] not in v_descendants.keys()

        # Check that ancestor modified fees includes fee deltas from
        # prioritisetransaction
        self.nodes[0].prioritisetransaction(txid=chain[0], fee_delta=1000)
        ancestor_fees = 0
        for x in chain:
            entry = self.nodes[0].getmempoolentry(x)
            ancestor_fees += entry['fees']['base']
            assert_equal(entry['fees']['ancestor'], ancestor_fees + Decimal('0.00001'))

        # Undo the prioritisetransaction for later tests
        self.nodes[0].prioritisetransaction(txid=chain[0], fee_delta=-1000)

        # Check that descendant modified fees includes fee deltas from
        # prioritisetransaction
        self.nodes[0].prioritisetransaction(txid=chain[-1], fee_delta=1000)

        descendant_fees = 0
        for x in reversed(chain):
            entry = self.nodes[0].getmempoolentry(x)
            descendant_fees += entry['fees']['base']
            assert_equal(entry['fees']['descendant'], descendant_fees + Decimal('0.00001'))

        # Check that prioritising a tx before it's added to the mempool works
        # First clear the mempool by mining a block.
        self.generate(self.nodes[0], 1)
        assert_equal(len(self.nodes[0].getrawmempool()), 0)
        # Prioritise a transaction that has been mined, then add it back to the
        # mempool by using invalidateblock.
        self.nodes[0].prioritisetransaction(txid=chain[-1], fee_delta=2000)
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())
        # Keep node1's tip synced with node0
        self.nodes[1].invalidateblock(self.nodes[1].getbestblockhash())

        # Now check that the transaction is in the mempool, with the right modified fee
        descendant_fees = 0
        for x in reversed(chain):
            entry = self.nodes[0].getmempoolentry(x)
            descendant_fees += entry['fees']['base']
            if (x == chain[-1]):
                assert_equal(entry['fees']['modified'], entry['fees']['base'] + Decimal("0.00002"))
            assert_equal(entry['fees']['descendant'], descendant_fees + Decimal("0.00002"))

        # Now test descendant chain limits
        tx_children = []
        # First create one parent tx with 10 children
        tx_with_children = self.wallet.send_self_transfer_multi(from_node=self.nodes[0], num_outputs=10)
        parent_transaction = tx_with_children["txid"]
        transaction_package = tx_with_children["new_utxos"]

        # Sign and send up to MAX_DESCENDANT transactions chained off the parent tx
        chain = [] # save sent txs for the purpose of checking node1's mempool later (see below)
        for _ in range(DEFAULT_DESCENDANT_LIMIT - 1):
            utxo = transaction_package.pop(0)
            new_tx = self.wallet.send_self_transfer_multi(from_node=self.nodes[0], num_outputs=10, utxos_to_spend=[utxo])
            txid = new_tx["txid"]
            chain.append(txid)
            if utxo['txid'] is parent_transaction:
                tx_children.append(txid)
            transaction_package.extend(new_tx["new_utxos"])

        mempool = self.nodes[0].getrawmempool(True)
        assert_equal(mempool[parent_transaction]['descendantcount'], DEFAULT_DESCENDANT_LIMIT)
        assert_equal(sorted(mempool[parent_transaction]['spentby']), sorted(tx_children))

        for child in tx_children:
            assert_equal(mempool[child]['depends'], [parent_transaction])

        # Check that node1's mempool is as expected, containing:
        # - parent tx for descendant test
        # - txs chained off parent tx (-> custom descendant limit)
        self.wait_until(lambda: len(self.nodes[1].getrawmempool()) == 2*CUSTOM_DESCENDANT_LIMIT, timeout=10)
        mempool0 = self.nodes[0].getrawmempool(False)
        mempool1 = self.nodes[1].getrawmempool(False)
        assert set(mempool1).issubset(set(mempool0))
        assert parent_transaction in mempool1
        for tx in chain:
            if tx in mempool1:
                entry0 = self.nodes[0].getmempoolentry(tx)
                entry1 = self.nodes[1].getmempoolentry(tx)
                assert not entry0['unbroadcast']
                assert not entry1['unbroadcast']
                assert entry1["descendantcount"] <= CUSTOM_DESCENDANT_LIMIT
                assert_equal(entry1['fees']['base'], entry0['fees']['base'])
                assert_equal(entry1['vsize'], entry0['vsize'])
                assert_equal(entry1['depends'], entry0['depends'])

        # Test reorg handling
        # First, the basics:
        self.generate(self.nodes[0], 1)
        self.nodes[1].invalidateblock(self.nodes[0].getbestblockhash())
        self.nodes[1].reconsiderblock(self.nodes[0].getbestblockhash())

        # Now test the case where node1 has a transaction T in its mempool that
        # depends on transactions A and B which are in a mined block, and the
        # block containing A and B is disconnected, AND B is not accepted back
        # into node1's mempool because its ancestor count is too high.

        # Create 8 transactions, like so:
        # Tx0 -> Tx1 (vout0)
        #   \--> Tx2 (vout1) -> Tx3 -> Tx4 -> Tx5 -> Tx6 -> Tx7
        #
        # Mine them in the next block, then generate a new tx8 that spends
        # Tx1 and Tx7, and add to node1's mempool, then disconnect the
        # last block.

        # Create tx0 with 2 outputs
        tx0 = self.wallet.send_self_transfer_multi(from_node=self.nodes[0], num_outputs=2)

        # Create tx1
        tx1 = self.wallet.send_self_transfer(from_node=self.nodes[0], utxo_to_spend=tx0["new_utxos"][0])

        # Create tx2-7
        tx7 = self.wallet.send_self_transfer_chain(from_node=self.nodes[0], utxo_to_spend=tx0["new_utxos"][1], chain_length=6)[-1]

        # Mine these in a block
        self.generate(self.nodes[0], 1)

        # Now generate tx8, with a big fee
        self.wallet.send_self_transfer_multi(from_node=self.nodes[0], utxos_to_spend=[tx1["new_utxo"], tx7["new_utxo"]], fee_per_output=40000)
        self.sync_mempools()

        # Now try to disconnect the tip on each node...
        self.nodes[1].invalidateblock(self.nodes[1].getbestblockhash())
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())
        self.sync_blocks()

if __name__ == '__main__':
    MempoolPackagesTest().main()
