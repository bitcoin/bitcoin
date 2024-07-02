#!/usr/bin/env python3
# Copyright (c) 2019-2022 The Bitcoin Core developers
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
)

class ReorgsRestoreTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 3

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

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
        assert tx_before_reorg["blockhash"] != tx_after_reorg["blockhash"]
        conflicted_after_reorg = self.nodes[1].gettransaction(conflicted_txid)
        # Check that conflicted tx is confirmed again with blockhash different than previously conflicting tx
        assert_equal(conflicted_after_reorg["confirmations"], 1)
        assert conflicting["blockhash"] != conflicted_after_reorg["blockhash"]

if __name__ == '__main__':
    ReorgsRestoreTest(__file__).main()
