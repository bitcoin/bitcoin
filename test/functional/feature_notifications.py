#!/usr/bin/env python3
# Copyright (c) 2014-2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the -alertnotify, -blocknotify and -walletnotify options."""
import os

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, wait_until, connect_nodes_bi, sync_blocks

class NotificationsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    def setup_network(self):
        self.alert_filename = os.path.join(self.options.tmpdir, "alert.txt")
        self.block_filename = os.path.join(self.options.tmpdir, "blocks.txt")
        self.tx_filename = os.path.join(self.options.tmpdir, "transactions.txt")

        # -alertnotify and -blocknotify on node0, walletnotify on node1
        self.extra_args = [["-blockversion=2",
                            "-alertnotify=echo %%s >> %s" % self.alert_filename,
                            "-blocknotify=echo %%s >> %s" % self.block_filename],
                           ["-blockversion=211",
                            "-rescan",
                            "-walletnotify=echo %%s >> %s" % self.tx_filename]]
        super().setup_network()

    def run_test(self):
        self.log.info("test -blocknotify")
        block_count = 10
        blocks = self.nodes[1].generate(block_count)

        # wait at most 10 seconds for expected file size before reading the content
        wait_until(lambda: os.path.isfile(self.block_filename) and os.stat(self.block_filename).st_size >= (block_count * 65), timeout=10)

        # file content should equal the generated blocks hashes
        with open(self.block_filename, 'r') as f:
            assert_equal(sorted(blocks), sorted(f.read().splitlines()))

        self.log.info("test -walletnotify")
        # wait at most 10 seconds for expected file size before reading the content
        wait_until(lambda: os.path.isfile(self.tx_filename) and os.stat(self.tx_filename).st_size >= (block_count * 65), timeout=10)

        # file content should equal the generated transaction hashes
        txids_rpc = list(map(lambda t: t['txid'], self.nodes[1].listtransactions("*", block_count)))
        with open(self.tx_filename, 'r') as f:
            assert_equal(sorted(txids_rpc), sorted(f.read().splitlines()))
        os.remove(self.tx_filename)

        self.log.info("test -walletnotify after rescan")
        # restart node to rescan to force wallet notifications
        self.restart_node(1)
        connect_nodes_bi(self.nodes, 0, 1)

        wait_until(lambda: os.path.isfile(self.tx_filename) and os.stat(self.tx_filename).st_size >= (block_count * 65), timeout=10)

        # file content should equal the generated transaction hashes
        txids_rpc = list(map(lambda t: t['txid'], self.nodes[1].listtransactions("*", block_count)))
        with open(self.tx_filename, 'r') as f:
            assert_equal(sorted(txids_rpc), sorted(f.read().splitlines()))
        os.remove(self.tx_filename)

        # Mine another 41 up-version blocks. -alertnotify should trigger on the 51st.
        self.log.info("test -alertnotify")
        self.nodes[1].generate(41)
        self.sync_all()

        # Give bitcoind 10 seconds to write the alert notification
        wait_until(lambda: os.path.isfile(self.alert_filename) and os.path.getsize(self.alert_filename), timeout=10)

        with open(self.alert_filename, 'r', encoding='utf8') as f:
            alert_text = f.read()

        # Mine more up-version blocks, should not get more alerts:
        self.nodes[1].generate(2)
        self.sync_all()

        with open(self.alert_filename, 'r', encoding='utf8') as f:
            alert_text2 = f.read()

        self.log.info("-alertnotify should not continue notifying for more unknown version blocks")
        assert_equal(alert_text, alert_text2)

        self.log.info("test -walletnotify with -walletnotifyconfirmations > 1")
        n_confirmations = 6

        # first have a different node generate blocks that we shouldn't see confirmations for
        self.nodes[0].generate(n_confirmations + 1)
        self.sync_all()
        os.remove(self.tx_filename)

        # now generate some blocks but don't bury any enough to be confirmed (they'll be confirmed later)
        generated_before_restart = self.nodes[1].generate(n_confirmations - 2)
        confirmed_txids = [t['txid'] for t in self.nodes[1].listtransactions("*", 100) if t['blockhash'] in generated_before_restart]

        # restart node with notify on confirmations enabled
        self.restart_node(1, ["-walletnotifyconfirmations=%s" % n_confirmations,
                              "-walletnotify=echo %%s >> %s" % self.tx_filename])

        # mine some more blocks (we should receive notifications on the ones above plus all but the last n_confirmations - 1 of these)
        generated_after_restart = self.nodes[1].generate(block_count + n_confirmations - 1)[:-n_confirmations + 1]
        wait_until(lambda: os.path.isfile(self.tx_filename) and os.stat(self.tx_filename).st_size >= (block_count * 65), timeout=10)

        # file content should equal the generated transaction hashes that are confirmed
        confirmed_txids += [t['txid'] for t in self.nodes[1].listtransactions("*", 1000) if t['blockhash'] in generated_after_restart]

        with open(self.tx_filename, 'r') as f:
            notified = f.read().splitlines()
            assert_equal(sorted(confirmed_txids), sorted(notified))

        # test reorgs of various heights
        os.remove(self.tx_filename)
        for i in range(1, n_confirmations):
            self.test_reorg(n_confirmations, i)

    def test_reorg(self, n_confirmations, depth):
        connect_nodes_bi(self.nodes, 0, 1)
        sync_blocks(self.nodes, timeout=5)

        # Ensure -walletnotifyconfirmations behaves correctly when a transaction
        # is introduced which is later reorg'd out.
        #
        # 1. Mine a block on node1 whose coinbase is added to be notified on after two
        #    additional confirmations. (height = n + 0)
        #
        # 2. Reorg with a longer chain (height = n + 2) which makes the transaction
        #    we introduced in (1) stale.
        #
        # 3. Assert that we haven't notified on the stale transaction.
        #
        self.log.info("Test correctness of -walletnotifyconfirmations during a reorg nconfirmations=%s depth=%s",
                      n_confirmations, depth)
        self.restart_node(0)
        self.restart_node(1, ["-walletnotifyconfirmations=%s" % n_confirmations,
                              "-walletnotify=echo %%s >> %s" % self.tx_filename])
        # Leave nodes unconnected to enable a reorg from node0 onto node1 later.

        # 1. Generate a block only seen by node1.
        #
        # We should receive a notification about the coinbase txn in this block
        # after 2 more block discoveries.
        #
        block_1_hash = self.nodes[1].generate(depth)[-1]
        block_1 = self.nodes[1].getblock(block_1_hash)

        assert_equal(self.nodes[1].getbestblockhash(), block_1_hash)

        # 2. Now, trigger a reorg that will remove the blocks that were just mined
        fork_blockhashes = self.nodes[0].generate(n_confirmations)
        connect_nodes_bi(self.nodes, 0, 1)
        sync_blocks(self.nodes, timeout=5)

        # Ensure the reorg has happened.
        for num in (0, 1):
            assert_equal(self.nodes[num].getbestblockhash(), fork_blockhashes[-1])

        # Ensure that no confirmation notifications were generated
        assert not os.path.isfile(self.tx_filename)

        # Ensure that block 1 has been reorg'd out of the main chain.
        assert self.nodes[1].getblock(block_1_hash)['confirmations'] == -1


if __name__ == '__main__':
    NotificationsTest().main()
