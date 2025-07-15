#!/usr/bin/env python3
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test tx status in case of reorgs while wallet being shutdown.

Wallet txn status rely on block connection/disconnection for its
accuracy. In case of reorgs happening while wallet being shutdown
block updates are not going to be received. At wallet loading, we
check against chain if confirmed txn are still in chain and change
their status if block in which they have been included has been
disconnected.
"""

from decimal import Decimal
import shutil

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
        assert_equal,
        assert_greater_than,
        assert_not_equal,
        assert_raises_rpc_error
)

class ReorgsRestoreTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_coinbase_automatic_abandon_during_startup(self):
        ##########################################################################################################
        # Verify the wallet marks coinbase transactions, and their descendants, as abandoned during startup when #
        # the block is no longer part of the best chain.                                                         #
        ##########################################################################################################
        self.log.info("Test automatic coinbase abandonment during startup")
        # Test setup: Sync nodes for the coming test, ensuring both are at the same block, then disconnect them to
        # generate two competing chains. After disconnection, verify no other peer connection exists.
        self.connect_nodes(1, 0)
        self.sync_blocks(self.nodes[:2])
        self.disconnect_nodes(1, 0)
        assert all(len(node.getpeerinfo()) == 0 for node in self.nodes[:2])

        # Create a new block in node0, coinbase going to wallet0
        self.nodes[0].createwallet(wallet_name="w0", load_on_startup=True)
        wallet0 = self.nodes[0].get_wallet_rpc("w0")
        self.generatetoaddress(self.nodes[0], 1, wallet0.getnewaddress(), sync_fun=self.no_op)
        node0_coinbase_tx_hash = wallet0.getblock(wallet0.getbestblockhash(), verbose=1)['tx'][0]

        # Mine 100 blocks on top to mature the coinbase and create a descendant
        self.generate(self.nodes[0], 101, sync_fun=self.no_op)
        # Make descendant, send-to-self
        descendant_tx_id = wallet0.sendtoaddress(wallet0.getnewaddress(), 1)

        # Verify balance
        wallet0.syncwithvalidationinterfacequeue()
        assert(wallet0.getbalances()['mine']['trusted'] > 0)

        # Now create a fork in node1. This will be used to replace node0's chain later.
        self.nodes[1].createwallet(wallet_name="w1", load_on_startup=True)
        wallet1 = self.nodes[1].get_wallet_rpc("w1")
        self.generatetoaddress(self.nodes[1], 1, wallet1.getnewaddress(), sync_fun=self.no_op)
        wallet1.syncwithvalidationinterfacequeue()

        # Verify both nodes are on a different chain
        block0_best_hash, block1_best_hash = wallet0.getbestblockhash(), wallet1.getbestblockhash()
        assert(block0_best_hash != block1_best_hash)

        # Stop both nodes and replace node0 chain entirely for the node1 chain
        self.stop_nodes()
        for path in ["chainstate", "blocks"]:
            shutil.rmtree(self.nodes[0].chain_path / path)
            shutil.copytree(self.nodes[1].chain_path / path, self.nodes[0].chain_path / path)

        # Start node0 and verify that now it has node1 chain and no info about its previous best block
        self.start_node(0)
        wallet0 = self.nodes[0].get_wallet_rpc("w0")
        assert_equal(wallet0.getbestblockhash(), block1_best_hash)
        assert_raises_rpc_error(-5, "Block not found", wallet0.getblock, block0_best_hash)

        # Verify the coinbase tx was marked as abandoned and balance correctly computed
        tx_info = wallet0.gettransaction(node0_coinbase_tx_hash)['details'][0]
        assert_equal(tx_info['abandoned'], True)
        assert_equal(tx_info['category'], 'orphan')
        assert(wallet0.getbalances()['mine']['trusted'] == 0)
        # Verify the coinbase descendant was also marked as abandoned
        assert_equal(wallet0.gettransaction(descendant_tx_id)['details'][0]['abandoned'], True)

    def test_reorg_handling_during_unclean_shutdown(self):
        self.log.info("Test that wallet transactions are un-abandoned in case of temporarily invalidated blocks and wallet doesn't crash due to a duplicate block disconnection event after an unclean shutdown")
        node = self.nodes[0]
        # Receive coinbase reward on a new wallet
        node.createwallet(wallet_name="reorg_crash", load_on_startup=True)
        wallet = node.get_wallet_rpc("reorg_crash")
        self.generatetoaddress(node, 1, wallet.getnewaddress(), sync_fun=self.no_op)

        # Restart to ensure node and wallet are flushed
        self.restart_node(0)
        wallet = node.get_wallet_rpc("reorg_crash")
        assert_greater_than(wallet.getbalances()["mine"]["immature"], 0)

        # Disconnect tip and sync wallet state
        tip = wallet.getbestblockhash()
        tip_height = wallet.getblockstats(hash_or_height=self.convert_to_json_for_cli(tip))["height"]
        wallet.invalidateblock(tip)
        wallet.syncwithvalidationinterfacequeue()

        # Tip was disconnected, ensure coinbase has been abandoned
        assert_equal(wallet.getbalances()["mine"]["immature"], 0)
        coinbase_tx_id = wallet.getblock(tip, verbose=1)["tx"][0]
        assert_equal(wallet.gettransaction(coinbase_tx_id)['details'][0]['abandoned'], True)

        # Abort process abruptly to mimic an unclean shutdown (no chain state flush to disk)
        node.kill_process()

        # Restart the node and confirm that it has not persisted the last chain state changes to disk
        # that leads to a rescan by the wallet
        with self.nodes[0].assert_debug_log(expected_msgs=[f"Rescanning last 1 blocks (from block {tip_height - 1})...\n"]):
            self.start_node(0)
        assert_equal(node.getbestblockhash(), tip)

        # After disconnecting the block, the wallet should record the new best block.
        # Upon reload after the crash, since the chainstate was not flushed, the tip contains the previously abandoned
        # coinbase. This was rescanned and now un-abandoned.
        wallet = node.get_wallet_rpc("reorg_crash")
        assert_equal(wallet.gettransaction(coinbase_tx_id)['details'][0]['abandoned'], False)
        assert_greater_than(wallet.getbalances()["mine"]["immature"], 0)

        # Previously, a bug caused the node to crash if two block disconnection events occurred consecutively.
        # Ensure this is no longer the case by simulating a new reorg.
        node.invalidateblock(tip)
        assert(node.getbestblockhash() != tip)
        # Ensure wallet state is consistent now
        assert_equal(wallet.gettransaction(coinbase_tx_id)['details'][0]['abandoned'], True)
        assert_equal(wallet.getbalances()["mine"]["immature"], 0)

        # And finally, verify the state if the block ends up being into the best chain again
        node.reconsiderblock(tip)
        assert_equal(wallet.gettransaction(coinbase_tx_id)['details'][0]['abandoned'], False)
        assert_greater_than(wallet.getbalances()["mine"]["immature"], 0)

    def run_test(self):
        # Send a tx from which to conflict outputs later
        txid_conflict_from = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), Decimal("10"))
        self.generate(self.nodes[0], 1)

        # Disconnect node1 from others to reorg its chain later
        self.disconnect_nodes(0, 1)
        self.disconnect_nodes(1, 2)
        self.connect_nodes(0, 2)

        # Send a tx to be unconfirmed later
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), Decimal("10"))
        tx = self.nodes[0].gettransaction(txid)
        self.generate(self.nodes[0], 4, sync_fun=self.no_op)
        self.sync_blocks([self.nodes[0], self.nodes[2]])
        tx_before_reorg = self.nodes[0].gettransaction(txid)
        assert_equal(tx_before_reorg["confirmations"], 4)

        # Disconnect node0 from node2 to broadcast a conflict on their respective chains
        self.disconnect_nodes(0, 2)
        nA = next(tx_out["vout"] for tx_out in self.nodes[0].gettransaction(txid_conflict_from)["details"] if tx_out["amount"] == Decimal("10"))
        inputs = []
        inputs.append({"txid": txid_conflict_from, "vout": nA})
        outputs_1 = {}
        outputs_2 = {}

        # Create a conflicted tx broadcast on node0 chain and conflicting tx broadcast on node1 chain. Both spend from txid_conflict_from
        outputs_1[self.nodes[0].getnewaddress()] = Decimal("9.99998")
        outputs_2[self.nodes[0].getnewaddress()] = Decimal("9.99998")
        conflicted = self.nodes[0].signrawtransactionwithwallet(self.nodes[0].createrawtransaction(inputs, outputs_1))
        conflicting = self.nodes[0].signrawtransactionwithwallet(self.nodes[0].createrawtransaction(inputs, outputs_2))

        conflicted_txid = self.nodes[0].sendrawtransaction(conflicted["hex"])
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)
        conflicting_txid = self.nodes[2].sendrawtransaction(conflicting["hex"])
        self.generate(self.nodes[2], 9, sync_fun=self.no_op)

        # Reconnect node0 and node2 and check that conflicted_txid is effectively conflicted
        self.connect_nodes(0, 2)
        self.sync_blocks([self.nodes[0], self.nodes[2]])
        conflicted = self.nodes[0].gettransaction(conflicted_txid)
        conflicting = self.nodes[0].gettransaction(conflicting_txid)
        assert_equal(conflicted["confirmations"], -9)
        assert_equal(conflicted["walletconflicts"][0], conflicting["txid"])

        # Node0 wallet is shutdown
        self.restart_node(0)

        # The block chain re-orgs and the tx is included in a different block
        self.generate(self.nodes[1], 9, sync_fun=self.no_op)
        self.nodes[1].sendrawtransaction(tx["hex"])
        self.generate(self.nodes[1], 1, sync_fun=self.no_op)
        self.nodes[1].sendrawtransaction(conflicted["hex"])
        self.generate(self.nodes[1], 1, sync_fun=self.no_op)

        # Node0 wallet file is loaded on longest sync'ed node1
        self.stop_node(1)
        self.nodes[0].backupwallet(self.nodes[0].datadir_path / 'wallet.bak')
        shutil.copyfile(self.nodes[0].datadir_path / 'wallet.bak', self.nodes[1].chain_path / self.default_wallet_name / self.wallet_data_filename)
        self.start_node(1)
        tx_after_reorg = self.nodes[1].gettransaction(txid)
        # Check that normal confirmed tx is confirmed again but with different blockhash
        assert_equal(tx_after_reorg["confirmations"], 2)
        assert_not_equal(tx_before_reorg["blockhash"], tx_after_reorg["blockhash"])
        conflicted_after_reorg = self.nodes[1].gettransaction(conflicted_txid)
        # Check that conflicted tx is confirmed again with blockhash different than previously conflicting tx
        assert_equal(conflicted_after_reorg["confirmations"], 1)
        assert_not_equal(conflicting["blockhash"], conflicted_after_reorg["blockhash"])

        # Verify we mark coinbase txs, and their descendants, as abandoned during startup
        self.test_coinbase_automatic_abandon_during_startup()

        # Verify reorg behavior during an unclean shutdown
        self.test_reorg_handling_during_unclean_shutdown()


if __name__ == '__main__':
    ReorgsRestoreTest(__file__).main()
