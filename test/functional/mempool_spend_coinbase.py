#!/usr/bin/env python3
# Copyright (c) 2014-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test spending coinbase transactions.

The coinbase transaction in block N can appear in block
N+100... so is valid in the mempool when the best block
height is N+99.
This test makes sure coinbase spends that will be mature
in the next block are accepted into the memory pool,
but less mature coinbase spends are NOT.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.wallet import MiniWallet


class MempoolSpendCoinbaseTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        wallet = MiniWallet(self.nodes[0])

        wallet.generate(200)
        chain_height = self.nodes[0].getblockcount()
        assert_equal(chain_height, 200)

        # Coinbase at height chain_height-100+1 ok in mempool, should
        # get mined. Coinbase at height chain_height-100+2 is
        # too immature to spend.
        b = [self.nodes[0].getblockhash(n) for n in range(101, 103)]
        coinbase_txids = [self.nodes[0].getblock(h)['tx'][0] for h in b]
        utxo_101 = wallet.get_utxo(txid=coinbase_txids[0])
        utxo_102 = wallet.get_utxo(txid=coinbase_txids[1])

        spend_101_id = wallet.send_self_transfer(from_node=self.nodes[0], utxo_to_spend=utxo_101)["txid"]

        # coinbase at height 102 should be too immature to spend
        assert_raises_rpc_error(-26,
                                "bad-txns-premature-spend-of-coinbase",
                                lambda: wallet.send_self_transfer(from_node=self.nodes[0], utxo_to_spend=utxo_102))

        # mempool should have just spend_101:
        assert_equal(self.nodes[0].getrawmempool(), [spend_101_id])

        # mine a block, spend_101 should get confirmed
        self.nodes[0].generate(1)
        assert_equal(set(self.nodes[0].getrawmempool()), set())

        # ... and now height 102 can be spent:
        spend_102_id = wallet.send_self_transfer(from_node=self.nodes[0], utxo_to_spend=utxo_102)["txid"]
        assert_equal(self.nodes[0].getrawmempool(), [spend_102_id])


if __name__ == '__main__':
    MempoolSpendCoinbaseTest().main()
