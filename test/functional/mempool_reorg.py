#!/usr/bin/env python3
# Copyright (c) 2014-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool re-org scenarios.

Test re-org scenarios with a mempool that contains transactions
that spend (directly or indirectly) coinbase transactions.
"""

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.wallet import MiniWallet

class MempoolCoinbaseTest(SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [
            [
                '-whitelist=noban@127.0.0.1',  # immediate tx relay
            ],
            []
        ]

    def run_test(self):
        wallet = MiniWallet(self.nodes[0])

        # Start with a 200 block chain
        assert_equal(self.nodes[0].getblockcount(), 200)

        self.log.info("Add 4 coinbase utxos to the miniwallet")
        # Block 76 contains the first spendable coinbase txs.
        first_block = 76
        wallet.scan_blocks(start=first_block, num=4)

        # Three scenarios for re-orging coinbase spends in the memory pool:
        # 1. Direct coinbase spend  :  spend_1
        # 2. Indirect (coinbase spend in chain, child in mempool) : spend_2 and spend_2_1
        # 3. Indirect (coinbase and child both in chain) : spend_3 and spend_3_1
        # Use invalidateblock to make all of the above coinbase spends invalid (immature coinbase),
        # and make sure the mempool code behaves correctly.
        b = [self.nodes[0].getblockhash(n) for n in range(first_block, first_block+4)]
        coinbase_txids = [self.nodes[0].getblock(h)['tx'][0] for h in b]
        utxo_1 = wallet.get_utxo(txid=coinbase_txids[1])
        utxo_2 = wallet.get_utxo(txid=coinbase_txids[2])
        utxo_3 = wallet.get_utxo(txid=coinbase_txids[3])
        self.log.info("Create three transactions spending from coinbase utxos: spend_1, spend_2, spend_3")
        spend_1 = wallet.create_self_transfer(from_node=self.nodes[0], utxo_to_spend=utxo_1)
        spend_2 = wallet.create_self_transfer(from_node=self.nodes[0], utxo_to_spend=utxo_2)
        spend_3 = wallet.create_self_transfer(from_node=self.nodes[0], utxo_to_spend=utxo_3)

        self.log.info("Create another transaction which is time-locked to two blocks in the future")
        utxo = wallet.get_utxo(txid=coinbase_txids[0])
        timelock_tx = wallet.create_self_transfer(
            from_node=self.nodes[0],
            utxo_to_spend=utxo,
            mempool_valid=False,
            locktime=self.nodes[0].getblockcount() + 2
        )['hex']

        self.log.info("Check that the time-locked transaction is too immature to spend")
        assert_raises_rpc_error(-26, "non-final", self.nodes[0].sendrawtransaction, timelock_tx)

        self.log.info("Broadcast and mine spend_2 and spend_3")
        wallet.sendrawtransaction(from_node=self.nodes[0], tx_hex=spend_2['hex'])
        wallet.sendrawtransaction(from_node=self.nodes[0], tx_hex=spend_3['hex'])
        self.log.info("Generate a block")
        self.nodes[0].generate(1)
        self.log.info("Check that time-locked transaction is still too immature to spend")
        assert_raises_rpc_error(-26, 'non-final', self.nodes[0].sendrawtransaction, timelock_tx)

        self.log.info("Create spend_2_1 and spend_3_1")
        spend_2_utxo = wallet.get_utxo(txid=spend_2['txid'])
        spend_2_1 = wallet.create_self_transfer(from_node=self.nodes[0], utxo_to_spend=spend_2_utxo)
        spend_3_utxo = wallet.get_utxo(txid=spend_3['txid'])
        spend_3_1 = wallet.create_self_transfer(from_node=self.nodes[0], utxo_to_spend=spend_3_utxo)

        self.log.info("Broadcast and mine spend_3_1")
        spend_3_1_id = self.nodes[0].sendrawtransaction(spend_3_1['hex'])
        self.log.info("Generate a block")
        last_block = self.nodes[0].generate(1)
        # Sync blocks, so that peer 1 gets the block before timelock_tx
        # Otherwise, peer 1 would put the timelock_tx in recentRejects
        self.sync_all()

        self.log.info("The time-locked transaction can now be spent")
        timelock_tx_id = self.nodes[0].sendrawtransaction(timelock_tx)

        self.log.info("Add spend_1 and spend_2_1 to the mempool")
        spend_1_id = self.nodes[0].sendrawtransaction(spend_1['hex'])
        spend_2_1_id = self.nodes[0].sendrawtransaction(spend_2_1['hex'])

        assert_equal(set(self.nodes[0].getrawmempool()), {spend_1_id, spend_2_1_id, timelock_tx_id})
        self.sync_all()

        self.log.info("invalidate the last block")
        for node in self.nodes:
            node.invalidateblock(last_block[0])
        self.log.info("The time-locked transaction is now too immature and has been removed from the mempool")
        self.log.info("spend_3_1 has been re-orged out of the chain and is back in the mempool")
        assert_equal(set(self.nodes[0].getrawmempool()), {spend_1_id, spend_2_1_id, spend_3_1_id})

        self.log.info("Use invalidateblock to re-org back and make all those coinbase spends immature/invalid")
        b = self.nodes[0].getblockhash(first_block + 100)
        for node in self.nodes:
            node.invalidateblock(b)

        self.log.info("Check that the mempool is empty")
        assert_equal(set(self.nodes[0].getrawmempool()), set())
        self.sync_all()


if __name__ == '__main__':
    MempoolCoinbaseTest().main()
