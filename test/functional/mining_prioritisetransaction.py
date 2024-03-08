#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the prioritisetransaction mining RPC."""

from decimal import Decimal
import time

from test_framework.messages import (
    COIN,
    MAX_BLOCK_WEIGHT,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    create_lots_of_big_transactions,
    gen_return_txouts,
)
from test_framework.wallet import MiniWallet


class PrioritiseTransactionTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[
            "-printpriority=1",
            "-datacarriersize=100000",
        ]] * self.num_nodes
        self.supports_cli = False

    def clear_prioritisation(self, node):
        for txid, info in node.getprioritisedtransactions().items():
            delta = info["fee_delta"]
            node.prioritisetransaction(txid, 0, -delta)
        assert_equal(node.getprioritisedtransactions(), {})

    def test_replacement(self):
        self.log.info("Test tx prioritisation stays after a tx is replaced")
        conflicting_input = self.wallet.get_utxo()
        tx_replacee = self.wallet.create_self_transfer(utxo_to_spend=conflicting_input, fee_rate=Decimal("0.0001"))
        tx_replacement = self.wallet.create_self_transfer(utxo_to_spend=conflicting_input, fee_rate=Decimal("0.005"))
        # Add 1 satoshi fee delta to replacee
        self.nodes[0].prioritisetransaction(tx_replacee["txid"], 0, 100)
        assert_equal(self.nodes[0].getprioritisedtransactions(), { tx_replacee["txid"] : { "fee_delta" : 100, "in_mempool" : False}})
        self.nodes[0].sendrawtransaction(tx_replacee["hex"])
        assert_equal(self.nodes[0].getprioritisedtransactions(), { tx_replacee["txid"] : { "fee_delta" : 100, "in_mempool" : True, "modified_fee": int(tx_replacee["fee"] * COIN + 100)}})
        self.nodes[0].sendrawtransaction(tx_replacement["hex"])
        assert tx_replacee["txid"] not in self.nodes[0].getrawmempool()
        assert_equal(self.nodes[0].getprioritisedtransactions(), { tx_replacee["txid"] : { "fee_delta" : 100, "in_mempool" : False}})

        # PrioritiseTransaction is additive
        self.nodes[0].prioritisetransaction(tx_replacee["txid"], 0, COIN)
        self.nodes[0].sendrawtransaction(tx_replacee["hex"])
        assert_equal(self.nodes[0].getprioritisedtransactions(), { tx_replacee["txid"] : { "fee_delta" : COIN + 100, "in_mempool" : True, "modified_fee": int(tx_replacee["fee"] * COIN + COIN + 100)}})
        self.generate(self.nodes[0], 1)
        assert_equal(self.nodes[0].getprioritisedtransactions(), {})

    def test_diamond(self):
        self.log.info("Test diamond-shape package with priority")
        mock_time = int(time.time())
        self.nodes[0].setmocktime(mock_time)

        #      tx_a
        #      / \
        #     /   \
        #   tx_b  tx_c
        #     \   /
        #      \ /
        #      tx_d

        tx_o_a = self.wallet.send_self_transfer_multi(
            from_node=self.nodes[0],
            num_outputs=2,
        )
        txid_a = tx_o_a["txid"]

        tx_o_b, tx_o_c = [self.wallet.send_self_transfer(
            from_node=self.nodes[0],
            utxo_to_spend=u,
        ) for u in tx_o_a["new_utxos"]]
        txid_b = tx_o_b["txid"]
        txid_c = tx_o_c["txid"]

        tx_o_d = self.wallet.send_self_transfer_multi(
            from_node=self.nodes[0],
            utxos_to_spend=[
                self.wallet.get_utxo(txid=txid_b),
                self.wallet.get_utxo(txid=txid_c),
            ],
        )
        txid_d = tx_o_d["txid"]

        self.log.info("Test priority while txs are in mempool")
        raw_before = self.nodes[0].getrawmempool(verbose=True)
        fee_delta_b = Decimal(9999) / COIN
        fee_delta_c_1 = Decimal(-1234) / COIN
        fee_delta_c_2 = Decimal(8888) / COIN
        self.nodes[0].prioritisetransaction(txid=txid_b, fee_delta=int(fee_delta_b * COIN))
        self.nodes[0].prioritisetransaction(txid=txid_c, fee_delta=int(fee_delta_c_1 * COIN))
        self.nodes[0].prioritisetransaction(txid=txid_c, fee_delta=int(fee_delta_c_2 * COIN))
        raw_before[txid_a]["fees"]["descendant"] += fee_delta_b + fee_delta_c_1 + fee_delta_c_2
        raw_before[txid_b]["fees"]["modified"] += fee_delta_b
        raw_before[txid_b]["fees"]["ancestor"] += fee_delta_b
        raw_before[txid_b]["fees"]["descendant"] += fee_delta_b
        raw_before[txid_c]["fees"]["modified"] += fee_delta_c_1 + fee_delta_c_2
        raw_before[txid_c]["fees"]["ancestor"] += fee_delta_c_1 + fee_delta_c_2
        raw_before[txid_c]["fees"]["descendant"] += fee_delta_c_1 + fee_delta_c_2
        raw_before[txid_d]["fees"]["ancestor"] += fee_delta_b + fee_delta_c_1 + fee_delta_c_2
        raw_after = self.nodes[0].getrawmempool(verbose=True)
        for txid in [txid_a, txid_b, txid_c, txid_d]:
            del raw_before[txid]["fees"]["chunk"]
            del raw_before[txid]["chunksize"]
            del raw_before[txid]["clusterid"]
        for txid in [txid_a, txid_b, txid_c, txid_d]:
            del raw_after[txid]["fees"]["chunk"]
            del raw_after[txid]["chunksize"]
            del raw_after[txid]["clusterid"]
        assert_equal(raw_before[txid_a], raw_after[txid_a])
        assert_equal(raw_before, raw_after)
        assert_equal(self.nodes[0].getprioritisedtransactions(), {txid_b: {"fee_delta" : fee_delta_b*COIN, "in_mempool" : True, "modified_fee": int(fee_delta_b*COIN + COIN * tx_o_b["fee"])}, txid_c: {"fee_delta" : (fee_delta_c_1 + fee_delta_c_2)*COIN, "in_mempool" : True, "modified_fee": int((fee_delta_c_1 + fee_delta_c_2 ) * COIN + COIN * tx_o_c["fee"])}})
        # Clear prioritisation, otherwise the transactions' fee deltas are persisted to mempool.dat and loaded again when the node
        # is restarted at the end of this subtest. Deltas are removed when a transaction is mined, but only at that time. We do
        # not check whether mapDeltas transactions were mined when loading from mempool.dat.
        self.clear_prioritisation(node=self.nodes[0])

        self.log.info("Test priority while txs are not in mempool")
        self.restart_node(0, extra_args=["-nopersistmempool"])
        self.nodes[0].setmocktime(mock_time)
        assert_equal(self.nodes[0].getmempoolinfo()["size"], 0)
        self.nodes[0].prioritisetransaction(txid=txid_b, fee_delta=int(fee_delta_b * COIN))
        self.nodes[0].prioritisetransaction(txid=txid_c, fee_delta=int(fee_delta_c_1 * COIN))
        self.nodes[0].prioritisetransaction(txid=txid_c, fee_delta=int(fee_delta_c_2 * COIN))
        assert_equal(self.nodes[0].getprioritisedtransactions(), {txid_b: {"fee_delta" : fee_delta_b*COIN, "in_mempool" : False}, txid_c: {"fee_delta" : (fee_delta_c_1 + fee_delta_c_2)*COIN, "in_mempool" : False}})
        for t in [tx_o_a["hex"], tx_o_b["hex"], tx_o_c["hex"], tx_o_d["hex"]]:
            self.nodes[0].sendrawtransaction(t)
        raw_after = self.nodes[0].getrawmempool(verbose=True)
        for txid in [txid_a, txid_b, txid_c, txid_d]:
            del raw_after[txid]["fees"]["chunk"]
            del raw_after[txid]["chunksize"]
            del raw_after[txid]["clusterid"]
        assert_equal(raw_before[txid_a], raw_after[txid_a])
        assert_equal(raw_before, raw_after)
        assert_equal(self.nodes[0].getprioritisedtransactions(), {txid_b: {"fee_delta" : fee_delta_b*COIN, "in_mempool" : True, "modified_fee": int(fee_delta_b*COIN + COIN * tx_o_b["fee"])}, txid_c: {"fee_delta" : (fee_delta_c_1 + fee_delta_c_2)*COIN, "in_mempool" : True, "modified_fee": int((fee_delta_c_1 + fee_delta_c_2 ) * COIN + COIN * tx_o_c["fee"])}})

        # Clear mempool
        self.generate(self.nodes[0], 1)
        # Prioritisation for transactions is automatically deleted after they are mined.
        assert_equal(self.nodes[0].getprioritisedtransactions(), {})

        # Use default extra_args
        self.restart_node(0)
        assert_equal(self.nodes[0].getprioritisedtransactions(), {})

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])

        # Test `prioritisetransaction` required parameters
        assert_raises_rpc_error(-1, "prioritisetransaction", self.nodes[0].prioritisetransaction)
        assert_raises_rpc_error(-1, "prioritisetransaction", self.nodes[0].prioritisetransaction, '')
        assert_raises_rpc_error(-1, "prioritisetransaction", self.nodes[0].prioritisetransaction, '', 0)

        # Test `prioritisetransaction` invalid extra parameters
        assert_raises_rpc_error(-1, "prioritisetransaction", self.nodes[0].prioritisetransaction, '', 0, 0, 0)

        # Test `getprioritisedtransactions` invalid parameters
        assert_raises_rpc_error(-1, "getprioritisedtransactions",
                self.nodes[0].getprioritisedtransactions, True)

        # Test `prioritisetransaction` invalid `txid`
        assert_raises_rpc_error(-8, "txid must be of length 64 (not 3, for 'foo')", self.nodes[0].prioritisetransaction, txid='foo', fee_delta=0)
        assert_raises_rpc_error(-8, "txid must be hexadecimal string (not 'Zd1d4e24ed99057e84c3f80fd8fbec79ed9e1acee37da269356ecea000000000')", self.nodes[0].prioritisetransaction, txid='Zd1d4e24ed99057e84c3f80fd8fbec79ed9e1acee37da269356ecea000000000', fee_delta=0)

        # Test `prioritisetransaction` invalid `dummy`
        txid = '1d1d4e24ed99057e84c3f80fd8fbec79ed9e1acee37da269356ecea000000000'
        assert_raises_rpc_error(-3, "JSON value of type string is not of expected type number", self.nodes[0].prioritisetransaction, txid, 'foo', 0)
        assert_raises_rpc_error(-8, "Priority is no longer supported, dummy argument to prioritisetransaction must be 0.", self.nodes[0].prioritisetransaction, txid, 1, 0)

        # Test `prioritisetransaction` invalid `fee_delta`
        assert_raises_rpc_error(-3, "JSON value of type string is not of expected type number", self.nodes[0].prioritisetransaction, txid=txid, fee_delta='foo')

        self.test_replacement()
        self.test_diamond()

        self.txouts = gen_return_txouts()
        self.relayfee = self.nodes[0].getnetworkinfo()['relayfee']

        utxo_count = 90
        utxos = self.wallet.send_self_transfer_multi(from_node=self.nodes[0], num_outputs=utxo_count)['new_utxos']
        self.generate(self.wallet, 1)
        assert_equal(len(self.nodes[0].getrawmempool()), 0)

        base_fee = self.relayfee*100 # our transactions are smaller than 100kb
        txids = []

        # Create 3 batches of transactions at 3 different fee rate levels
        range_size = utxo_count // 3
        for i in range(3):
            txids.append([])
            start_range = i * range_size
            end_range = start_range + range_size
            txids[i] = create_lots_of_big_transactions(
                self.wallet,
                self.nodes[0],
                (i+1) * base_fee,
                end_range - start_range,
                self.txouts,
                utxos[start_range:end_range])

        # Make sure that the size of each group of transactions exceeds
        # MAX_BLOCK_WEIGHT // 4 -- otherwise the test needs to be revised to
        # create more transactions.
        mempool = self.nodes[0].getrawmempool(True)
        sizes = [0, 0, 0]
        for i in range(3):
            for j in txids[i]:
                assert j in mempool
                sizes[i] += mempool[j]['vsize']
            assert sizes[i] > MAX_BLOCK_WEIGHT // 4  # Fail => raise utxo_count

        assert_equal(self.nodes[0].getprioritisedtransactions(), {})
        # add a fee delta to something in the cheapest bucket and make sure it gets mined
        # also check that a different entry in the cheapest bucket is NOT mined
        self.nodes[0].prioritisetransaction(txid=txids[0][0], fee_delta=int(3*base_fee*COIN))
        assert_equal(self.nodes[0].getprioritisedtransactions(), {txids[0][0] : { "fee_delta" : 3*base_fee*COIN, "in_mempool" : True, "modified_fee": int(3*base_fee*COIN + COIN * 1 * base_fee)}})

        # Priority disappears when prioritisetransaction is called with an inverse value...
        self.nodes[0].prioritisetransaction(txid=txids[0][0], fee_delta=int(-3*base_fee*COIN))
        assert txids[0][0] not in self.nodes[0].getprioritisedtransactions()
        # ... and reappears when prioritisetransaction is called again.
        self.nodes[0].prioritisetransaction(txid=txids[0][0], fee_delta=int(3*base_fee*COIN))
        assert txids[0][0] in self.nodes[0].getprioritisedtransactions()

        self.generate(self.nodes[0], 1)

        mempool = self.nodes[0].getrawmempool()
        self.log.info("Assert that prioritised transaction was mined")
        assert txids[0][0] not in mempool
        assert txids[0][1] in mempool

        high_fee_tx = None
        for x in txids[2]:
            if x not in mempool:
                high_fee_tx = x

        # Something high-fee should have been mined!
        assert high_fee_tx is not None

        # Add a prioritisation before a tx is in the mempool (de-prioritising a
        # high-fee transaction so that it's now low fee).
        self.nodes[0].prioritisetransaction(txid=high_fee_tx, fee_delta=-int(2*base_fee*COIN))
        assert_equal(self.nodes[0].getprioritisedtransactions()[high_fee_tx], { "fee_delta" : -2*base_fee*COIN, "in_mempool" : False})

        # Add everything back to mempool
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())

        # Check to make sure our high fee rate tx is back in the mempool
        mempool = self.nodes[0].getrawmempool()
        assert high_fee_tx in mempool

        # Now verify the modified-high feerate transaction isn't mined before
        # the other high fee transactions. Keep mining until our mempool has
        # decreased by all the high fee size that we calculated above.
        while (self.nodes[0].getmempoolinfo()['bytes'] > sizes[0] + sizes[1]):
            self.generate(self.nodes[0], 1, sync_fun=self.no_op)

        # High fee transaction should not have been mined, but other high fee rate
        # transactions should have been.
        mempool = self.nodes[0].getrawmempool()
        self.log.info("Assert that de-prioritised transaction is still in mempool")
        assert high_fee_tx in mempool
        assert_equal(self.nodes[0].getprioritisedtransactions()[high_fee_tx], { "fee_delta" : -2*base_fee*COIN, "in_mempool" : True, "modified_fee": int(-2*base_fee*COIN + COIN * 3 * base_fee)})
        for x in txids[2]:
            if (x != high_fee_tx):
                assert x not in mempool


        self.log.info("Assert that 0 delta is never added to mapDeltas")
        tx_id_zero_del = self.wallet.create_self_transfer()['txid']
        self.nodes[0].prioritisetransaction(txid=tx_id_zero_del, fee_delta=0)
        assert tx_id_zero_del not in self.nodes[0].getprioritisedtransactions()

        # Create a free transaction.  Should be rejected.
        tx_res = self.wallet.create_self_transfer(fee_rate=0)
        tx_hex = tx_res['hex']
        tx_id = tx_res['txid']

        # This will raise an exception due to min relay fee not being met
        assert_raises_rpc_error(-26, "min relay fee not met", self.nodes[0].sendrawtransaction, tx_hex)
        assert tx_id not in self.nodes[0].getrawmempool()

        # This is a less than 1000-byte transaction, so just set the fee
        # to be the minimum for a 1000-byte transaction and check that it is
        # accepted.
        self.nodes[0].prioritisetransaction(txid=tx_id, fee_delta=int(self.relayfee*COIN))
        assert_equal(self.nodes[0].getprioritisedtransactions()[tx_id], { "fee_delta" : self.relayfee*COIN, "in_mempool" : False})

        self.log.info("Assert that prioritised free transaction is accepted to mempool")
        assert_equal(self.nodes[0].sendrawtransaction(tx_hex), tx_id)
        assert tx_id in self.nodes[0].getrawmempool()
        assert_equal(self.nodes[0].getprioritisedtransactions()[tx_id], { "fee_delta" : self.relayfee*COIN, "in_mempool" : True, "modified_fee": int(self.relayfee*COIN + COIN * tx_res["fee"])})

        # Test that calling prioritisetransaction is sufficient to trigger
        # getblocktemplate to (eventually) return a new block.
        mock_time = int(time.time())
        self.nodes[0].setmocktime(mock_time)
        template = self.nodes[0].getblocktemplate({'rules': ['segwit']})
        self.nodes[0].prioritisetransaction(txid=tx_id, fee_delta=-int(self.relayfee*COIN))

        # Calling prioritisetransaction with the inverse amount should delete its prioritisation entry
        assert tx_id not in self.nodes[0].getprioritisedtransactions()

        self.nodes[0].setmocktime(mock_time+10)
        new_template = self.nodes[0].getblocktemplate({'rules': ['segwit']})

        assert template != new_template

if __name__ == '__main__':
    PrioritiseTransactionTest().main()
