#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool re-org scenarios.

Test re-org scenarios with a mempool that contains transactions
that spend (directly or indirectly) coinbase transactions.
"""

import time

from test_framework.messages import (
    CInv,
    MSG_WTX,
    msg_getdata,
)
from test_framework.p2p import (
    P2PTxInvStore,
    p2p_lock,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.wallet import MiniWallet

class MempoolCoinbaseTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [
            [
                '-whitelist=noban@127.0.0.1',  # immediate tx relay
            ],
            []
        ]

    def test_reorg_relay(self):
        self.log.info("Test that transactions from disconnected blocks are available for relay immediately")
        # Prevent time from moving forward
        self.nodes[1].setmocktime(int(time.time()))
        self.connect_nodes(0, 1)
        self.generate(self.wallet, 3)

        # Disconnect node0 and node1 to create different chains.
        self.disconnect_nodes(0, 1)
        # Connect a peer to node1, which doesn't have immediate tx relay
        peer1 = self.nodes[1].add_p2p_connection(P2PTxInvStore())

        # Create a transaction that is included in a block.
        tx_disconnected = self.wallet.send_self_transfer(from_node=self.nodes[1])
        self.generate(self.nodes[1], 1, sync_fun=self.no_op)

        # Create a transaction and submit it to node1's mempool.
        tx_before_reorg = self.wallet.send_self_transfer(from_node=self.nodes[1])

        # Create a child of that transaction and submit it to node1's mempool.
        tx_child = self.wallet.send_self_transfer(utxo_to_spend=tx_disconnected["new_utxo"], from_node=self.nodes[1])
        assert_equal(self.nodes[1].getmempoolentry(tx_child["txid"])["ancestorcount"], 1)
        assert_equal(len(peer1.get_invs()), 0)

        # node0 has a longer chain in which tx_disconnected was not confirmed.
        self.generate(self.nodes[0], 3, sync_fun=self.no_op)

        # Reconnect the nodes and sync chains. node0's chain should win.
        self.connect_nodes(0, 1)
        self.sync_blocks()

        # Child now has an ancestor from the disconnected block
        assert_equal(self.nodes[1].getmempoolentry(tx_child["txid"])["ancestorcount"], 2)
        assert_equal(self.nodes[1].getmempoolentry(tx_before_reorg["txid"])["ancestorcount"], 1)

        # peer1 should not have received an inv for any of the transactions during this time, as no
        # mocktime has elapsed for those transactions to be announced. Likewise, it cannot
        # request very recent, unanounced transactions.
        assert_equal(len(peer1.get_invs()), 0)
        # It's too early to request these two transactions
        requests_too_recent = msg_getdata([CInv(t=MSG_WTX, h=int(tx["tx"].getwtxid(), 16)) for tx in [tx_before_reorg, tx_child]])
        peer1.send_and_ping(requests_too_recent)
        for _ in range(len(requests_too_recent.inv)):
            peer1.sync_with_ping()
        with p2p_lock:
            assert "tx" not in peer1.last_message
            assert "notfound" in peer1.last_message

        # Request the tx from the disconnected block
        request_disconnected_tx = msg_getdata([CInv(t=MSG_WTX, h=int(tx_disconnected["tx"].getwtxid(), 16))])
        peer1.send_and_ping(request_disconnected_tx)

        # The tx from the disconnected block was never announced, and it entered the mempool later
        # than the transactions that are too recent.
        assert_equal(len(peer1.get_invs()), 0)
        with p2p_lock:
            # However, the node will answer requests for the tx from the recently-disconnected block.
            assert_equal(peer1.last_message["tx"].tx.getwtxid(),tx_disconnected["tx"].getwtxid())

        self.nodes[1].setmocktime(int(time.time()) + 300)
        peer1.sync_with_ping()
        # the transactions are now announced
        assert_equal(len(peer1.get_invs()), 3)
        for _ in range(3):
            # make sure all tx requests have been responded to
            peer1.sync_with_ping()
        last_tx_received = peer1.last_message["tx"]

        tx_after_reorg = self.wallet.send_self_transfer(from_node=self.nodes[1])
        request_after_reorg = msg_getdata([CInv(t=MSG_WTX, h=int(tx_after_reorg["tx"].getwtxid(), 16))])
        assert tx_after_reorg["txid"] in self.nodes[1].getrawmempool()
        peer1.send_and_ping(request_after_reorg)
        with p2p_lock:
            assert_equal(peer1.last_message["tx"], last_tx_received)

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])
        wallet = self.wallet

        # Start with a 200 block chain
        assert_equal(self.nodes[0].getblockcount(), 200)

        self.log.info("Add 4 coinbase utxos to the miniwallet")
        # Block 76 contains the first spendable coinbase txs.
        first_block = 76

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
        spend_1 = wallet.create_self_transfer(utxo_to_spend=utxo_1)
        spend_2 = wallet.create_self_transfer(utxo_to_spend=utxo_2)
        spend_3 = wallet.create_self_transfer(utxo_to_spend=utxo_3)

        self.log.info("Create another transaction which is time-locked to two blocks in the future")
        utxo = wallet.get_utxo(txid=coinbase_txids[0])
        timelock_tx = wallet.create_self_transfer(
            utxo_to_spend=utxo,
            locktime=self.nodes[0].getblockcount() + 2,
        )['hex']

        self.log.info("Check that the time-locked transaction is too immature to spend")
        assert_raises_rpc_error(-26, "non-final", self.nodes[0].sendrawtransaction, timelock_tx)

        self.log.info("Broadcast and mine spend_2 and spend_3")
        wallet.sendrawtransaction(from_node=self.nodes[0], tx_hex=spend_2['hex'])
        wallet.sendrawtransaction(from_node=self.nodes[0], tx_hex=spend_3['hex'])
        self.log.info("Generate a block")
        self.generate(self.nodes[0], 1)
        self.log.info("Check that time-locked transaction is still too immature to spend")
        assert_raises_rpc_error(-26, 'non-final', self.nodes[0].sendrawtransaction, timelock_tx)

        self.log.info("Create spend_2_1 and spend_3_1")
        spend_2_1 = wallet.create_self_transfer(utxo_to_spend=spend_2["new_utxo"])
        spend_3_1 = wallet.create_self_transfer(utxo_to_spend=spend_3["new_utxo"])

        self.log.info("Broadcast and mine spend_3_1")
        spend_3_1_id = self.nodes[0].sendrawtransaction(spend_3_1['hex'])
        self.log.info("Generate a block")
        last_block = self.generate(self.nodes[0], 1)
        # generate() implicitly syncs blocks, so that peer 1 gets the block before timelock_tx
        # Otherwise, peer 1 would put the timelock_tx in m_lazy_recent_rejects

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

        self.test_reorg_relay()


if __name__ == '__main__':
    MempoolCoinbaseTest(__file__).main()
