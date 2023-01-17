#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
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

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.wallet import MiniWallet


class MempoolSpendCoinbaseTest(SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        wallet = MiniWallet(self.nodes[0])

        # Invalidate two blocks, so that miniwallet has access to a coin that will mature in the next block
        chain_height = 198
        self.nodes[0].invalidateblock(self.nodes[0].getblockhash(chain_height + 1))
        assert_equal(chain_height, self.nodes[0].getblockcount())

        # Coinbase at height chain_height-100+1 ok in mempool, should
        # get mined. Coinbase at height chain_height-100+2 is
        # too immature to spend.
        coinbase_txid = lambda h: self.nodes[0].getblock(self.nodes[0].getblockhash(h))['tx'][0]
        utxo_mature = wallet.get_utxo(txid=coinbase_txid(chain_height - 100 + 1))
        utxo_immature = wallet.get_utxo(txid=coinbase_txid(chain_height - 100 + 2))

        spend_mature_id = wallet.send_self_transfer(from_node=self.nodes[0], utxo_to_spend=utxo_mature)["txid"]

        # other coinbase should be too immature to spend
        immature_tx = wallet.create_self_transfer(utxo_to_spend=utxo_immature)
        assert_raises_rpc_error(-26,
                                "bad-txns-premature-spend-of-coinbase",
                                lambda: self.nodes[0].sendrawtransaction(immature_tx['hex']))

        # mempool should have just the mature one
        assert_equal(self.nodes[0].getrawmempool(), [spend_mature_id])

        # mine a block, mature one should get confirmed
        self.generate(self.nodes[0], 1)
        assert_equal(set(self.nodes[0].getrawmempool()), set())

        # ... and now previously immature can be spent:
        spend_new_id = self.nodes[0].sendrawtransaction(immature_tx['hex'])
        assert_equal(self.nodes[0].getrawmempool(), [spend_new_id])


if __name__ == '__main__':
    MempoolSpendCoinbaseTest().main()
