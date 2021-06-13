#!/usr/bin/env python3
# Copyright (c) 2014-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test descendant package tracking carve-out allowing one final transaction in
   an otherwise-full package as long as it has only one parent and is <= 10k in
   size.
"""

from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    chain_transaction,
)

MAX_ANCESTORS = 25
MAX_DESCENDANTS = 25

class MempoolPackagesTest(SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-maxorphantx=1000"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # Mine some blocks and have them mature.
        self.nodes[0].generate(COINBASE_MATURITY + 1)
        utxo = self.nodes[0].listunspent(10)
        txid = utxo[0]['txid']
        vout = utxo[0]['vout']
        value = utxo[0]['amount']

        fee = Decimal("0.0002")
        # MAX_ANCESTORS transactions off a confirmed tx should be fine
        chain = []
        for _ in range(4):
            (txid, sent_value) = chain_transaction(self.nodes[0], [txid], [vout], value, fee, 2)
            vout = 0
            value = sent_value
            chain.append([txid, value])
        for _ in range(MAX_ANCESTORS - 4):
            (txid, sent_value) = chain_transaction(self.nodes[0], [txid], [0], value, fee, 1)
            value = sent_value
            chain.append([txid, value])
        (second_chain, second_chain_value) = chain_transaction(self.nodes[0], [utxo[1]['txid']], [utxo[1]['vout']], utxo[1]['amount'], fee, 1)

        # Check mempool has MAX_ANCESTORS + 1 transactions in it
        assert_equal(len(self.nodes[0].getrawmempool(True)), MAX_ANCESTORS + 1)

        # Adding one more transaction on to the chain should fail.
        assert_raises_rpc_error(-26, "too-long-mempool-chain, too many unconfirmed ancestors [limit: 25]", chain_transaction, self.nodes[0], [txid], [0], value, fee, 1)
        # ...even if it chains on from some point in the middle of the chain.
        assert_raises_rpc_error(-26, "too-long-mempool-chain, too many descendants", chain_transaction, self.nodes[0], [chain[2][0]], [1], chain[2][1], fee, 1)
        assert_raises_rpc_error(-26, "too-long-mempool-chain, too many descendants", chain_transaction, self.nodes[0], [chain[1][0]], [1], chain[1][1], fee, 1)
        # ...even if it chains on to two parent transactions with one in the chain.
        assert_raises_rpc_error(-26, "too-long-mempool-chain, too many descendants", chain_transaction, self.nodes[0], [chain[0][0], second_chain], [1, 0], chain[0][1] + second_chain_value, fee, 1)
        # ...especially if its > 40k weight
        assert_raises_rpc_error(-26, "too-long-mempool-chain, too many descendants", chain_transaction, self.nodes[0], [chain[0][0]], [1], chain[0][1], fee, 350)
        # But not if it chains directly off the first transaction
        (replacable_txid, replacable_orig_value) = chain_transaction(self.nodes[0], [chain[0][0]], [1], chain[0][1], fee, 1)
        # and the second chain should work just fine
        chain_transaction(self.nodes[0], [second_chain], [0], second_chain_value, fee, 1)

        # Make sure we can RBF the chain which used our carve-out rule
        second_tx_outputs = {self.nodes[0].getrawtransaction(replacable_txid, True)["vout"][0]['scriptPubKey']['address']: replacable_orig_value - (Decimal(1) / Decimal(100))}
        second_tx = self.nodes[0].createrawtransaction([{'txid': chain[0][0], 'vout': 1}], second_tx_outputs)
        signed_second_tx = self.nodes[0].signrawtransactionwithwallet(second_tx)
        self.nodes[0].sendrawtransaction(signed_second_tx['hex'])

        # Finally, check that we added two transactions
        assert_equal(len(self.nodes[0].getrawmempool(True)), MAX_ANCESTORS + 3)

if __name__ == '__main__':
    MempoolPackagesTest().main()
