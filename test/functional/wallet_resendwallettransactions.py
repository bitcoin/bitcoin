#!/usr/bin/env python3
# Copyright (c) 2017-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that the wallet resends transactions periodically."""
import time

from decimal import Decimal

from test_framework.blocktools import (
    create_block,
    create_coinbase,
)
from test_framework.messages import DEFAULT_MEMPOOL_EXPIRY_HOURS
from test_framework.p2p import P2PTxInvStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    get_fee,
    try_rpc,
)

class ResendWalletTransactionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [["-whitelist=relay,noban@127.0.0.1"], ["-whitelist=relay,noban@127.0.0.1"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes() # Don't connect nodes

    def run_test(self):
        node = self.nodes[0]  # alias

        peer_first = node.add_p2p_connection(P2PTxInvStore())

        self.log.info("Create a new transaction and wait until it's broadcast")
        parent_utxo, indep_utxo = node.listunspent()[:2]
        addr = node.getnewaddress()
        txid = node.send(outputs=[{addr: 1}], inputs=[parent_utxo])["txid"]

        # Can take a few seconds due to transaction trickling
        peer_first.wait_for_broadcast([txid])

        # Add a second peer since txs aren't rebroadcast to the same peer (see m_tx_inventory_known_filter)
        peer_second = node.add_p2p_connection(P2PTxInvStore())

        self.log.info("Create a block")
        # Create and submit a block without the transaction.
        # Transactions are only rebroadcast if there has been a block at least five minutes
        # after the last time we tried to broadcast. Use mocktime and give an extra minute to be sure.
        block_time = int(time.time()) + 6 * 60
        node.setmocktime(block_time)
        block = create_block(int(node.getbestblockhash(), 16), create_coinbase(node.getblockcount() + 1), block_time)
        block.solve()
        node.submitblock(block.serialize().hex())

        # Set correct m_best_block_time, which is used in ResubmitWalletTransactions
        node.syncwithvalidationinterfacequeue()
        now = int(time.time())

        # Transaction should not be rebroadcast within first 12 hours
        # Leave 2 mins for buffer
        twelve_hrs = 12 * 60 * 60
        two_min = 2 * 60
        node.setmocktime(now + twelve_hrs - two_min)
        node.mockscheduler(60)  # Tell scheduler to call MaybeResendWalletTxs now
        assert_equal(int(txid, 16) in peer_second.get_invs(), False)

        self.log.info("Bump time & check that transaction is rebroadcast")
        # Transaction should be rebroadcast approximately 24 hours in the future,
        # but can range from 12-36. So bump 36 hours to be sure.
        with node.assert_debug_log(['resubmit 1 unconfirmed transactions']):
            node.setmocktime(now + 36 * 60 * 60)
            # Tell scheduler to call MaybeResendWalletTxs now.
            node.mockscheduler(60)
        # Give some time for trickle to occur
        node.setmocktime(now + 36 * 60 * 60 + 600)
        peer_second.wait_for_broadcast([txid])

        self.log.info("Chain of unconfirmed not-in-mempool txs are rebroadcast")
        # This tests that the node broadcasts the parent transaction before the child transaction.
        # To test that scenario, we need a method to reliably get a child transaction placed
        # in mapWallet positioned before the parent. We cannot predict the position in mapWallet,
        # but we can observe it using listreceivedbyaddress and other related RPCs.
        #
        # So we will create the child transaction, use listreceivedbyaddress to see what the
        # ordering of mapWallet is, if the child is not before the parent, we will create a new
        # child (via bumpfee) and remove the old child (via removeprunedfunds) until we get the
        # ordering of child before parent.
        child_inputs = [{"txid": txid, "vout": 0}]
        child_txid = node.sendall(recipients=[addr], inputs=child_inputs)["txid"]
        # Get the child tx's info for manual bumping
        child_tx_info = node.gettransaction(txid=child_txid, verbose=True)
        child_output_value = child_tx_info["decoded"]["vout"][0]["value"]
        # Include an additional 1 vbyte buffer to handle when we have a smaller signature
        additional_child_fee = get_fee(child_tx_info["decoded"]["vsize"] + 1, Decimal(0.00001100))
        while True:
            txids = node.listreceivedbyaddress(minconf=0, address_filter=addr)[0]["txids"]
            if txids == [child_txid, txid]:
                break
            # Manually bump the tx
            # The inputs and the output address stay the same, just changing the amount for the new fee
            child_output_value -= additional_child_fee
            bumped_raw = node.createrawtransaction(inputs=child_inputs, outputs=[{addr: child_output_value}])
            bumped = node.signrawtransactionwithwallet(bumped_raw)
            bumped_txid = node.decoderawtransaction(bumped["hex"])["txid"]
            # Sometimes we will get a signature that is a little bit shorter than we expect which causes the
            # feerate to be a bit higher, then the followup to be a bit lower. This results in a replacement
            # that can't be broadcast. We can just skip that and keep grinding.
            if try_rpc(-26, "insufficient fee, rejecting replacement", node.sendrawtransaction, bumped["hex"]):
                continue
            # The scheduler queue creates a copy of the added tx after
            # send/bumpfee and re-adds it to the wallet (undoing the next
            # removeprunedfunds). So empty the scheduler queue:
            node.syncwithvalidationinterfacequeue()
            node.removeprunedfunds(child_txid)
            child_txid = bumped_txid
        entry_time = node.getmempoolentry(child_txid)["time"]

        block_time = entry_time + 6 * 60
        node.setmocktime(block_time)
        block = create_block(int(node.getbestblockhash(), 16), create_coinbase(node.getblockcount() + 1), block_time)
        block.solve()
        node.submitblock(block.serialize().hex())
        # Set correct m_best_block_time, which is used in ResubmitWalletTransactions
        node.syncwithvalidationinterfacequeue()

        evict_time = block_time + 60 * 60 * DEFAULT_MEMPOOL_EXPIRY_HOURS + 5
        # Flush out currently scheduled resubmit attempt now so that there can't be one right between eviction and check.
        with node.assert_debug_log(['resubmit 2 unconfirmed transactions']):
            node.setmocktime(evict_time)
            node.mockscheduler(60)

        # Evict these txs from the mempool
        indep_send = node.send(outputs=[{node.getnewaddress(): 1}], inputs=[indep_utxo])
        node.getmempoolentry(indep_send["txid"])
        assert_raises_rpc_error(-5, "Transaction not in mempool", node.getmempoolentry, txid)
        assert_raises_rpc_error(-5, "Transaction not in mempool", node.getmempoolentry, child_txid)

        # Rebroadcast and check that parent and child are both in the mempool
        with node.assert_debug_log(['resubmit 2 unconfirmed transactions']):
            node.setmocktime(evict_time + 36 * 60 * 60) # 36 hrs is the upper limit of the resend timer
            node.mockscheduler(60)
        node.getmempoolentry(txid)
        node.getmempoolentry(child_txid)

        self.log.info("Test rebroadcast of transactions received by others")
        # clear mempool
        self.generate(node, 1, sync_fun=self.no_op)
        # Sync node1's mocktime to node0's before connecting so it accepts node0's blocks
        node1 = self.nodes[1]
        node1.setmocktime(evict_time + 36 * 60 * 60)
        self.connect_nodes(1, 0)
        self.sync_all()

        self.log.info("node0 sends a tx to node1 and disconnects")
        recv_addr = node1.getnewaddress()
        recv_txid = node.sendtoaddress(recv_addr, 1)
        self.sync_mempools()
        node1.syncwithvalidationinterfacequeue()

        wallet_tx = node1.gettransaction(recv_txid)
        assert_equal(wallet_tx["confirmations"], 0)
        recv_wtxid = node1.getmempoolentry(recv_txid)["wtxid"]
        self.disconnect_nodes(0, 1)

        self.log.info("Create a block without the transaction")
        node1.bumpmocktime(6 * 60)
        block = create_block(int(node1.getbestblockhash(), 16), create_coinbase(node1.getblockcount() + 1), node1.mocktime)
        block.solve()
        node1.submitblock(block.serialize().hex())
        node1.syncwithvalidationinterfacequeue()

        self.log.info("Connect p2p who hasn't seen the tx")
        peer = node1.add_p2p_connection(P2PTxInvStore())

        self.log.info("Check that rebroadcast happens after 36 hours")
        with node1.assert_debug_log(['resubmit 1 unconfirmed transactions']):
            node1.bumpmocktime(36 * 60 * 60)
            node1.mockscheduler(60)

        peer.wait_for_broadcast([recv_wtxid])



if __name__ == '__main__':
    ResendWalletTransactionsTest(__file__).main()
