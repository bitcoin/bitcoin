#!/usr/bin/env python3
# Copyright (c) 2017-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that the wallet resends transactions periodically."""
import time

from test_framework.blocktools import (
    create_block,
)
from test_framework.messages import DEFAULT_MEMPOOL_EXPIRY_HOURS
from test_framework.p2p import P2PTxInvStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

# 36 hours is the upper limit of the resend timer, see CWallet::SetNextResend()
RESEND_TIMER_LIMIT = 36 * 60 * 60


class ResendWalletTransactionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.noban_tx_relay = True  # Needed due to mocktime

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()  # Don't connect nodes

    def mine_empty_block(self, node):
        # Create and submit a block without transactions.
        # Transactions are only rebroadcast if there has been a block at least five minutes
        # after the last time we tried to broadcast. Use mocktime and give an extra minute to be sure.
        if node.mocktime is None:
            node.setmocktime(int(time.time()))
        node.bumpmocktime(6 * 60)
        block = create_block(int(node.getbestblockhash(), 16), height=node.getblockcount() + 1, ntime=node.mocktime)
        block.solve()
        node.submitblock(block.serialize().hex())

    def test_resubmit_timer(self):
        self.log.info("Test periodic rebroadcast of sent transaction")

        node = self.nodes[0]  # alias
        node.createwallet("resubmit")
        wallet = node.get_wallet_rpc("resubmit")

        self.default_wallet.sendtoaddress(wallet.getnewaddress(), 5)
        self.generate(node, 1, sync_fun=self.no_op)
        parent_utxo = wallet.listunspent()[0]

        peer_first = node.add_p2p_connection(P2PTxInvStore())

        self.log.debug("Create a new transaction and wait until it's broadcast")
        txid = wallet.send(outputs=[{self.default_wallet.getnewaddress(): 1}], inputs=[parent_utxo])["txid"]
        wtxid = wallet.gettransaction(txid)["wtxid"]

        # Can take a few seconds due to transaction trickling
        peer_first.wait_for_broadcast([wtxid])

        # Add a second peer since txs aren't rebroadcast to the same peer (see m_tx_inventory_known_filter)
        peer_second = node.add_p2p_connection(P2PTxInvStore())

        self.mine_empty_block(node)

        # Set correct m_best_block_time, which is used in ResubmitWalletTransactions
        node.syncwithvalidationinterfacequeue()

        # Transaction should not be rebroadcast within first 12 hours
        # Leave 2 mins for buffer
        with node.assert_debug_log(expected_msgs=[], unexpected_msgs=["resubmit 1 unconfirmed transactions"]):
            twelve_hrs = 12 * 60 * 60
            two_min = 2 * 60
            node.bumpmocktime(twelve_hrs - two_min)
            node.mockscheduler(60)  # Tell scheduler to call MaybeResendWalletTxs now
            assert_equal(int(wtxid, 16) in peer_second.get_invs(), False)

        # Transaction should be rebroadcast approximately 24 hours in the future,
        # but can range from 12-36. So bump 36 hours to be sure.
        with node.assert_debug_log(['resubmit 1 unconfirmed transactions']):
            node.bumpmocktime(RESEND_TIMER_LIMIT)
            # Tell scheduler to call MaybeResendWalletTxs now.
            node.mockscheduler(60)
            peer_second.wait_for_broadcast([wtxid])

        self.generate(node, 1, sync_fun=self.no_op)

    def test_chained_tx_resubmission(self):
        self.log.info("Test that chain of unconfirmed not-in-mempool txs are rebroadcast")
        node = self.nodes[0]  # alias
        node.createwallet("chained")
        wallet = node.get_wallet_rpc("chained")

        # We cannot predict the ordering in mapWallet of parent and child, so
        # try a few times to get both.
        for _ in range(10):
            self.default_wallet.sendtoaddress(wallet.getnewaddress(), 1)
            self.generate(node, 1, sync_fun=self.no_op)
            parent_txid = wallet.sendall(recipients=[wallet.getnewaddress()])["txid"]
            parent_utxo = {"txid": parent_txid, "vout": 0}

            child_txid = wallet.sendall(recipients=[wallet.getnewaddress()], inputs=[parent_utxo])["txid"]

            self.mine_empty_block(node)
            node.syncwithvalidationinterfacequeue()

            evict_time = node.mocktime + 60 * 60 * DEFAULT_MEMPOOL_EXPIRY_HOURS + 5
            # Flush out currently scheduled resubmit attempt now so that there can't be one right between eviction and check.
            with node.assert_debug_log(['resubmit 2 unconfirmed transactions'], timeout=2):
                node.setmocktime(evict_time)
                node.mockscheduler(60)

            # Evict these txs from the mempool
            indep_send = self.default_wallet.send(outputs=[{self.default_wallet.getnewaddress(): 1}])
            node.getmempoolentry(indep_send["txid"])
            assert_raises_rpc_error(-5, "Transaction not in mempool", node.getmempoolentry, parent_txid)
            assert_raises_rpc_error(-5, "Transaction not in mempool", node.getmempoolentry, child_txid)

            # Rebroadcast and check that parent and child are both in the mempool
            with node.assert_debug_log(['resubmit 2 unconfirmed transactions'], timeout=2):
                node.setmocktime(evict_time + RESEND_TIMER_LIMIT)
                node.mockscheduler(60)
            node.getmempoolentry(parent_txid)
            node.getmempoolentry(child_txid)

    def test_received_rebroadcast(self):
        self.log.info("Test rebroadcast of transactions received by others")
        self.nodes[1].createwallet("otherrecv")
        wallet = self.nodes[1].get_wallet_rpc("otherrecv")

        # clear mempool
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)

        # Sync node1's mocktime to node0's before connecting so it accepts node0's blocks
        node1 = self.nodes[1]
        node1.setmocktime(self.nodes[0].mocktime)
        self.connect_nodes(1, 0)
        self.sync_all()

        self.log.debug("node0 sends a tx to node1 and disconnects")
        recv_txid = self.default_wallet.sendtoaddress(wallet.getnewaddress(), 1)
        self.sync_mempools()
        node1.syncwithvalidationinterfacequeue()

        wallet_tx = wallet.gettransaction(recv_txid)
        assert_equal(wallet_tx["confirmations"], 0)
        recv_wtxid = node1.getmempoolentry(recv_txid)["wtxid"]
        self.disconnect_nodes(0, 1)

        self.mine_empty_block(node1)
        node1.syncwithvalidationinterfacequeue()

        self.log.info("Connect p2p who hasn't seen the tx")
        peer = node1.add_p2p_connection(P2PTxInvStore())

        self.log.info("Check that rebroadcast happens after 36 hours")
        with node1.assert_debug_log(['resubmit 1 unconfirmed transactions']):
            node1.bumpmocktime(RESEND_TIMER_LIMIT)
            node1.mockscheduler(60)
            peer.wait_for_broadcast([recv_wtxid])

    def test_resubmit_on_load(self):
        self.log.info("Test loading a wallet only resubmits to the mempool and does not broadcast")
        self.nodes[0].createwallet("loading_resubmit")
        wallet = self.nodes[0].get_wallet_rpc("loading_resubmit")

        self.default_wallet.sendtoaddress(wallet.getnewaddress(), 1)
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)

        txid1 = wallet.sendtoaddress(self.default_wallet.getnewaddress(), 0.5)
        txid2 = self.default_wallet.sendtoaddress(wallet.getnewaddress(), 1)

        self.restart_node(0, extra_args=self.nodes[0].extra_args + ["-persistmempool=0", "-nowallet", f"-mocktime={self.nodes[0].mocktime}"])
        assert_equal(self.nodes[0].getrawmempool(), [])
        peer = self.nodes[0].add_p2p_connection(P2PTxInvStore())

        self.nodes[0].loadwallet("loading_resubmit")
        mempool = self.nodes[0].getrawmempool()
        assert txid1 in mempool
        assert txid2 in mempool

        self.nodes[0].loadwallet(self.default_wallet_name)
        self.default_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        # Send an unrelated transaction to make sure the unconfirmed txs in the wallet are not broadcast
        broadcast_txid = self.default_wallet.sendtoaddress(self.default_wallet.getnewaddress(), 1)
        broadcast_wtxid = self.default_wallet.gettransaction(broadcast_txid)["wtxid"]
        peer.wait_for_broadcast([broadcast_wtxid])
        invs = peer.get_invs()
        assert int(txid1, 16) not in invs
        assert int(txid2, 16) not in invs

        self.generate(self.nodes[0], 1, sync_fun=self.no_op)

    def run_test(self):
        self.default_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        self.test_resubmit_timer()
        self.test_chained_tx_resubmission()
        self.test_received_rebroadcast()
        self.test_resubmit_on_load()

if __name__ == '__main__':
    ResendWalletTransactionsTest(__file__).main()
