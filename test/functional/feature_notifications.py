#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Copyright (c) 2023 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the -alertnotify, -blocknotify, -chainlocknotify, -instantsendnotify and -walletnotify options."""
import os

from test_framework.address import ADDRESS_BCRT1_UNSPENDABLE
from test_framework.test_framework import DashTestFramework
from test_framework.util import (
    assert_equal,
    force_finish_mnsync,
    wait_until,
)

# Linux allow all characters other than \x00
# Windows disallow control characters (0-31) and /\?%:|"<>
FILE_CHAR_START = 32 if os.name == 'nt' else 1
FILE_CHAR_END = 128
FILE_CHARS_DISALLOWED = '/\\?%*:|"<>' if os.name == 'nt' else '/'


def notify_outputname(walletname, txid):
    return txid if os.name == 'nt' else '{}_{}'.format(walletname, txid)


class NotificationsTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(6, 4, fast_dip3_enforcement=True)

    def setup_network(self):
        self.wallet = ''.join(chr(i) for i in range(FILE_CHAR_START, FILE_CHAR_END) if chr(i) not in FILE_CHARS_DISALLOWED)
        self.alertnotify_dir = os.path.join(self.options.tmpdir, "alertnotify")
        self.blocknotify_dir = os.path.join(self.options.tmpdir, "blocknotify")
        self.walletnotify_dir = os.path.join(self.options.tmpdir, "walletnotify")
        self.chainlocknotify_dir = os.path.join(self.options.tmpdir, "chainlocknotify")
        self.instantsendnotify_dir = os.path.join(self.options.tmpdir, "instantsendnotify")
        os.mkdir(self.alertnotify_dir)
        os.mkdir(self.blocknotify_dir)
        os.mkdir(self.walletnotify_dir)
        os.mkdir(self.chainlocknotify_dir)
        os.mkdir(self.instantsendnotify_dir)

        # -alertnotify and -blocknotify on node0, walletnotify on node1
        self.extra_args[0].append("-alertnotify=echo > {}".format(os.path.join(self.alertnotify_dir, '%s')))
        self.extra_args[0].append("-blocknotify=echo > {}".format(os.path.join(self.blocknotify_dir, '%s')))
        self.extra_args[1].append("-blockversion=211")
        self.extra_args[1].append("-rescan")
        self.extra_args[1].append("-walletnotify=echo > {}".format(os.path.join(self.walletnotify_dir, notify_outputname('%w', '%s'))))

        # -chainlocknotify on node0, -instantsendnotify on node1
        self.extra_args[0].append("-chainlocknotify=echo > {}".format(os.path.join(self.chainlocknotify_dir, '%s')))
        self.extra_args[1].append("-instantsendnotify=echo > {}".format(os.path.join(self.instantsendnotify_dir, notify_outputname('%w', '%s'))))

        super().setup_network()

    def run_test(self):
        # remove files created during network setup
        for block_file in os.listdir(self.blocknotify_dir):
            os.remove(os.path.join(self.blocknotify_dir, block_file))
        for tx_file in os.listdir(self.walletnotify_dir):
            os.remove(os.path.join(self.walletnotify_dir, tx_file))

        if self.is_wallet_compiled():
            self.nodes[1].createwallet(wallet_name=self.wallet, load_on_startup=True)

        self.log.info("test -blocknotify")
        block_count = 10
        blocks = self.nodes[1].generatetoaddress(block_count, self.nodes[1].getnewaddress() if self.is_wallet_compiled() else ADDRESS_BCRT1_UNSPENDABLE)

        # wait at most 10 seconds for expected number of files before reading the content
        wait_until(lambda: len(os.listdir(self.blocknotify_dir)) == block_count, timeout=10)

        # directory content should equal the generated blocks hashes
        assert_equal(sorted(blocks), sorted(os.listdir(self.blocknotify_dir)))

        if self.is_wallet_compiled():
            self.log.info("test -walletnotify")
            # wait at most 10 seconds for expected number of files before reading the content
            wait_until(lambda: len(os.listdir(self.walletnotify_dir)) == block_count, timeout=10)

            # directory content should equal the generated transaction hashes
            txids_rpc = list(map(lambda t: notify_outputname(self.wallet, t['txid']), self.nodes[1].listtransactions("*", block_count)))
            assert_equal(sorted(txids_rpc), sorted(os.listdir(self.walletnotify_dir)))
            self.stop_node(1)

            for tx_file in os.listdir(self.walletnotify_dir):
                os.remove(os.path.join(self.walletnotify_dir, tx_file))

            self.log.info("test -walletnotify after rescan")
            # restart node to rescan to force wallet notifications
            self.start_node(1)
            force_finish_mnsync(self.nodes[1])
            self.connect_nodes(0, 1)

            wait_until(lambda: len(os.listdir(self.walletnotify_dir)) == block_count, timeout=10)

            # directory content should equal the generated transaction hashes
            txids_rpc = list(map(lambda t: notify_outputname(self.wallet, t['txid']), self.nodes[1].listtransactions("*", block_count)))
            assert_equal(sorted(txids_rpc), sorted(os.listdir(self.walletnotify_dir)))

        self.log.info("test -chainlocknotify")

        self.activate_v19(expected_activation_height=900)
        self.log.info("Activated v19 at height:" + str(self.nodes[0].getblockcount()))

        self.activate_dip8()
        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.nodes[0].sporkupdate("SPORK_19_CHAINLOCKS_ENABLED", 4070908800)
        self.wait_for_sporks_same()
        self.move_to_next_cycle()
        self.log.info("Cycle H height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H+C height:" + str(self.nodes[0].getblockcount()))
        self.move_to_next_cycle()
        self.log.info("Cycle H+2C height:" + str(self.nodes[0].getblockcount()))

        (quorum_info_i_0, quorum_info_i_1) = self.mine_cycle_quorum(llmq_type_name='llmq_test_dip0024', llmq_type=103)
        self.nodes[0].sporkupdate("SPORK_19_CHAINLOCKS_ENABLED", 0)
        self.wait_for_sporks_same()

        self.log.info("Mine single block, wait for chainlock")
        self.bump_mocktime(1)
        tip = self.nodes[0].generate(1)[-1]
        self.wait_for_chainlocked_block_all_nodes(tip)
        # directory content should equal the chainlocked block hash
        assert_equal([tip], sorted(os.listdir(self.chainlocknotify_dir)))

        if self.is_wallet_compiled():
            self.log.info("test -instantsendnotify")
            assert_equal(len(os.listdir(self.instantsendnotify_dir)), 0)

            tx_count = 10
            for _ in range(tx_count):
                txid = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1)
                self.wait_for_instantlock(txid, self.nodes[1])

            # wait at most 10 seconds for expected number of files before reading the content
            wait_until(lambda: len(os.listdir(self.instantsendnotify_dir)) == tx_count, timeout=10)

            # directory content should equal the generated transaction hashes
            txids_rpc = list(map(lambda t: notify_outputname(self.wallet, t['txid']), self.nodes[1].listtransactions("*", tx_count)))
            assert_equal(sorted(txids_rpc), sorted(os.listdir(self.instantsendnotify_dir)))

        # TODO: add test for `-alertnotify` large fork notifications

if __name__ == '__main__':
    NotificationsTest().main()
