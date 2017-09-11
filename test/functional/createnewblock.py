#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test transaction selection (CreateNewBlock).

  - Test that block max weight is respected (no blocks too big).
  - Test that if the mempool has a tx that fits in a block, it will always be
    included unless the block is within 4000 of max weight.
  - Test that transaction selection takes into account feerate-with-ancestors
    when creating blocks.
  - Test that recently received transactions aren't selected unless the
    feerate for including them is high.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.blocktools import create_block, create_coinbase
from test_framework.mininode import ToHex
from test_framework.util import (
    random_transaction,
    connect_nodes,
)
from test_framework.authproxy import JSONRPCException
from decimal import Decimal
import random
import time


class CreateNewBlockTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.set_test_params()

    def set_test_params(self):
        self.num_nodes = 4
        self.setup_clean_chain = True

    def setup_network(self, split=False):
        self.cnb_args = [{
            'window': 10000,
            'stalerate': 0.02
        }, {
            'window': 30000,
            'stalerate': 0.1
        }, {
            'window': 50000,
            'stalerate': 0.001
        }]

        # Ancestor/descendant size limits are boosted because this test will
        # populate and sync mempools using unconfirmed transaction chains, and
        # failure to sync the mempool between nodes (eg due to descendant
        # counts differing between nodes that see transactions in different
        # order) will cause test failure.
        # Set blockmaxweight to be low, to require fewer transactions
        # to fill up a block.
        extra_args = [[
            "-debug", "-blockmaxweight=400000", "-minrelaytxfee=0",
            "-limitancestorcount=100", "-limitdescendantcount=100"
        ] for i in range(self.num_nodes)]
        for extra_arg, cnb_arg in zip(extra_args, self.cnb_args):
            extra_arg.extend([
                "-recenttxwindow=%s" % cnb_arg['window'],
                "-recenttxstalerate=%s" % cnb_arg['stalerate']
            ])
        self.nodes = self.start_nodes(self.num_nodes, self.options.tmpdir,
                                      extra_args)

        # Connect the network in a loop
        for i in range(self.num_nodes - 1):
            connect_nodes(self.nodes[i], i + 1)
        connect_nodes(self.nodes[-1], 0)

        self.is_network_split = False

    def populate_mempool(self, desired_transactions):
        num_transactions = 0
        while (num_transactions < desired_transactions):
            try:
                random_amount = random.randint(1, 10) * Decimal('0.1')
                random_transaction(
                    self.nodes,
                    amount=random_amount,
                    min_fee=Decimal("0.00001"),
                    fee_increment=Decimal("0.000005"),
                    fee_variants=5000,
                    confirmations_required=0,
                    allow_segwit=True)
                num_transactions += 1
            except RuntimeError as e:
                # We might run out of funds; just count these as valid attempts
                if "Insufficient funds" in e.error['message']:
                    num_transactions += 1
                else:
                    raise AssertionError("Unexpected run time error: " +
                                         e.error['message'])
            except JSONRPCException as e:
                if "too-long-mempool" in e.error['message']:
                    num_transactions += 1
                else:
                    raise AssertionError("Unexpected JSON-RPC error: " +
                                         e.error['message'])
            except Exception as e:
                raise AssertionError("Unexpected exception raised: " +
                                     type(e).__name__)
        self.sync_all()

    # Approximate weight of the transactions in the mempool
    def get_mempool_weight(self, node):
        mempool = node.getrawmempool(verbose=True)
        weight = 0
        for txid, entry in mempool.items():
            # Scale by the witness multiplier, since we get vsize back
            weight += entry['size'] * 4
        return weight

    # Requires mempool to be populated ahead of time
    def test_max_block_weight(self, node):
        block_max_weight = 400000 - 4000  # 4000 reserved for coinbase
        assert self.get_mempool_weight(node) > block_max_weight

        template = node.getblocktemplate({"rules": ["segwit"]})
        block_weight = 0
        for x in template['transactions']:
            block_weight += x['weight']
        assert (block_weight > block_max_weight - 4000)
        assert (block_weight < block_max_weight)

    def add_empty_block(self, node):
        height = node.getblockcount()
        tip = node.getbestblockhash()
        mtp = node.getblockheader(tip)['mediantime']
        block = create_block(
            int(tip, 16), create_coinbase(height + 1), mtp + 1)
        block.nVersion = 4
        block.solve()
        node.submitblock(ToHex(block))

        # Log the hash to make it easier to figure out where we are in the
        # test, when debugging.
        self.log.debug("Added empty block %s", block.hash)

    # Test that transaction selection is via ancestor feerate, by showing that
    # it's sufficient to bump a child tx's fee to get it and its parent
    # included.
    def test_ancestor_feerate_sort(self, node, recent_tx_window):

        # Advance time to the future, to ensure recent-transaction-filtering
        # isn't affecting the test.
        node.setmocktime(int(time.time()) + (recent_tx_window + 999) // 1000 + 1)
        self.add_empty_block(node)  # bypass the gbt cache

        # Call getblocktemplate.  Find a transaction in the mempool that is not
        # in the block, which has exactly 1 ancestor that is also not in the
        # block.  Call prioritisetransaction on the child tx to boost its
        # ancestor feerate to be above the maximum feerate of transactions in
        # the last ancestor_size space in the block, and verify that the child
        # transaction is in the next block template.
        template = node.getblocktemplate({"rules": ["segwit"]})
        block_txids = [x["txid"] for x in template["transactions"]]
        mempool_txids = node.getrawmempool()

        # Find a transaction that has exactly one ancestor that is also not in
        # the block. Exactly one ancestor is required so that we aren't trying
        # to fee bump something that has, eg, one in-block ancestors and one
        # not-in-block ancestor, because in such a situation the package feerate
        # used in CreateNewBlock will not be the ancestor feerate we get from
        # looking up the transaction in the mempool; it'll be modified by the
        # in-block parent.  Consequently we could bump the feerate by too little
        # fee in the test, causing test failure.
        txid_to_bump = None

        for txid in mempool_txids:
            if txid not in block_txids:
                ancestor_txids = node.getmempoolancestors(
                    txid=txid, verbose=False)
                if len(ancestor_txids) != 1:
                    continue
                if ancestor_txids[0] not in block_txids:
                    txid_to_bump = txid
                    break
            if txid_to_bump is not None:
                break
        if txid_to_bump is None:
            raise AssertionError(
                "Can't find candidate to test ancestor feerate score (test setup bug)"
            )

        # Calculate the ancestor feerate of the candidate transaction.
        mempool_entry = node.getmempoolentry(txid_to_bump)
        ancestor_size = mempool_entry['ancestorsize']
        ancestor_fee = mempool_entry['ancestorfees']
        tx_fee = mempool_entry['modifiedfee']
        tx_size = mempool_entry['size']
        self.log.debug(
                "Found txid to bump: %s, ancestor_size = %d, ancestor_fee = %f (ancestor feerate: %f) size = %d mod_fee = %d (feerate: %f)",
            txid_to_bump, ancestor_size, ancestor_fee, ancestor_fee/ancestor_size, tx_size, tx_fee, tx_fee/tx_size)

        # Determine feerate of the last ancestor_size portion of the block.
        cumulative_weight = 0
        max_feerate = 0
        for block_tx in reversed(template["transactions"]):
            # The fee is the consensus correct one, not the policy modified
            # one, but we haven't prioritized anything yet, so this should be
            # fine.
            self.log.debug("End of block tx: txid %s, fee %d, weight %d",
                    block_tx["txid"], block_tx["fee"], block_tx["weight"])
            if 4 * block_tx["fee"] / block_tx["weight"] > max_feerate:
                max_feerate = 4 * block_tx["fee"] / block_tx["weight"]
            cumulative_weight += block_tx["weight"]
            if cumulative_weight > 4 * ancestor_size:
                break

        # Bump the candidate transaction so that it has a higher feerate than
        # the max feerate observed in the last portion of the block that we
        # seek to replace
        fee_bump = int(max_feerate * ancestor_size - ancestor_fee) + 1
        self.log.debug("Setting fee_bump to %d", fee_bump)

        node.prioritisetransaction(txid=txid_to_bump, fee_delta=fee_bump)

        # Check that the next block template has txid_to_bump in it.
        self.add_empty_block(node)  # bypass the gbt cache
        template = node.getblocktemplate({"rules": ["segwit"]})
        new_block_txids = [x["txid"] for x in template["transactions"]]

        assert txid_to_bump in new_block_txids

        # Get rid of our prioritisation
        node.prioritisetransaction(txid=txid_to_bump, fee_delta=-fee_bump)

    def test_recent_transactions(self):
        # Populate the mempool with some transactions, and compare gbt results
        # across the nodes.  Repeat.
        self.add_empty_block(self.nodes[0])  # Bypass gbt cache.
        self.sync_all()
        recent_tx_block_count = [0, 0, 0]

        max_iterations = 20
        for i in range(max_iterations):
            self.populate_mempool(random.randint(5, 15))  # 5-15 transactions

            # Freeze current time, so that our recent tx window calculations
            # will be consistent.
            cur_time = int(time.time())
            [node.setmocktime(cur_time) for node in self.nodes]

            # Check that total block reward is within stale_rate of node3
            max_income = self.nodes[3].getblocktemplate({
                "rules": ["segwit"]
                })['coinbasevalue']

            # Check that calling gbt on the given node produces a block
            # within the expected tolerance of max_income.
            # Return true if the returned block includes recent transactions;
            # false otherwise
            def check_gbt_results(node, cnb_args):
                template = node.getblocktemplate({"rules": ["segwit"]})
                mempool = node.getrawmempool(verbose=True)
                stale_rate = cnb_args['stalerate']
                # Convert window from milliseconds to seconds
                window = (cnb_args['window'] + 999) // 1000

                # We should always be within stale_rate of best possible block
                assert template['coinbasevalue'] >= stale_rate * max_income

                # Check that if we're including any recent transactions, then
                # we achieve the max_income
                includes_recent_transactions = False
                for x in template['transactions']:
                    if mempool[x['txid']]['time'] > cur_time - window:
                        includes_recent_transactions = True
                        break

                if includes_recent_transactions:
                    assert template['coinbasevalue'] == max_income

                return includes_recent_transactions

            for j in range(3):
                if check_gbt_results(self.nodes[j], self.cnb_args[j]):
                    recent_tx_block_count[j] += 1

            # Drain the mempool if it gets big
            if self.nodes[0].getmempoolinfo()['bytes'] > 400000:
                self.nodes[0].generate(1)
            else:
                [node.setmocktime(0) for node in self.nodes]
                self.add_empty_block(self.nodes[0])
            self.sync_all()

        if sum(recent_tx_block_count) == 3 * max_iterations:
            self.log.warn(
                "Warning: every block contained recent transactions!"
            )
        if sum(recent_tx_block_count) == 0:
            self.log.warn("Warning: no block contained recent transactions!")
        self.log.debug(recent_tx_block_count)

    def run_test(self):
        # Leave IBD and generate some coins to spend.
        # Give everyone plenty of coins
        self.log.info("Generating initial coins for all nodes")
        for i in range(2):
            for x in self.nodes:
                # Building a long blockchain here will make the block reward
                # smaller, compared to the fees -- this is helpful for
                # test_recent_transactions()
                x.generate(201)
                self.sync_all()

        # Run tests...
        self.log.info("Populating mempool with a lot of transactions")
        self.populate_mempool(300)

        self.log.info("Running recent transactions test")
        self.test_recent_transactions()

        self.log.info("Repopulating mempool")
        self.populate_mempool(500)

        self.log.info("Running test_max_block_weight")
        self.test_max_block_weight(self.nodes[0])

        self.sync_all()

        self.log.info("Running test_ancestor_feerate_sort")
        self.test_ancestor_feerate_sort(
            self.nodes[0], recent_tx_window=self.cnb_args[0]['window'])


if __name__ == "__main__":
    CreateNewBlockTest().main()
