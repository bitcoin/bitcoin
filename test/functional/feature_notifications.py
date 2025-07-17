#!/usr/bin/env python3
# Copyright (c) 2014-2020 The Bitcoin Core developers
# Copyright (c) 2023-2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the -alertnotify, -blocknotify, -chainlocknotify, -instantsendnotify and -walletnotify options."""
import os

from test_framework.address import ADDRESS_BCRT1_UNSPENDABLE

from test_framework.test_framework import DashTestFramework
from test_framework.util import (
    assert_equal,
    force_finish_mnsync,
)

# Linux allow all characters other than \x00
# Windows disallow control characters (0-31) and /\?%:|"<>
FILE_CHAR_START = 32 if os.name == 'nt' else 1
FILE_CHAR_END = 128
FILE_CHARS_DISALLOWED = '/\\?%*:|"<>' if os.name == 'nt' else '/'
UNCONFIRMED_HASH_STRING = 'unconfirmed'

def notify_outputname(walletname, txid):
    return txid if os.name == 'nt' else f'{walletname}_{txid}'


class NotificationsTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(6, 4)

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
        # -chainlocknotify on node0, -instantsendnotify on node1
        self.extra_args = [[
            f"-alertnotify=echo > {os.path.join(self.alertnotify_dir, '%s')}",
            f"-blocknotify=echo > {os.path.join(self.blocknotify_dir, '%s')}",
            f"-chainlocknotify=echo > {os.path.join(self.chainlocknotify_dir, '%s')}",
        ], [
            "-rescan",
            f"-walletnotify=echo %h_%b > {os.path.join(self.walletnotify_dir, notify_outputname('%w', '%s'))}",
            f"-instantsendnotify=echo > {os.path.join(self.instantsendnotify_dir, notify_outputname('%w', '%s'))}",
        ],
        [], [], [], []]

        self.wallet_names = [self.default_wallet_name, self.wallet]
        super().setup_network()

    def run_test(self):
        # remove files created during network setup
        for block_file in os.listdir(self.blocknotify_dir):
            os.remove(os.path.join(self.blocknotify_dir, block_file))
        for tx_file in os.listdir(self.walletnotify_dir):
            os.remove(os.path.join(self.walletnotify_dir, tx_file))

        self.log.info("test -blocknotify")
        block_count = 10
        blocks = self.generatetoaddress(self.nodes[1], block_count, self.nodes[1].getnewaddress() if self.is_wallet_compiled() else ADDRESS_BCRT1_UNSPENDABLE)

        # wait at most 10 seconds for expected number of files before reading the content
        self.wait_until(lambda: len(os.listdir(self.blocknotify_dir)) == block_count, timeout=10)

        # directory content should equal the generated blocks hashes
        assert_equal(sorted(blocks), sorted(os.listdir(self.blocknotify_dir)))

        if self.is_wallet_compiled():
            self.log.info("test -walletnotify")
            # wait at most 10 seconds for expected number of files before reading the content
            self.wait_until(lambda: len(os.listdir(self.walletnotify_dir)) == block_count, timeout=10)

            # directory content should equal the generated transaction hashes
            tx_details = list(map(lambda t: (t['txid'], t['blockheight'], t['blockhash']), self.nodes[1].listtransactions("*", block_count)))
            self.stop_node(1)
            self.expect_wallet_notify(tx_details)

            self.log.info("test -walletnotify after rescan")
            # restart node to rescan to force wallet notifications
            self.start_node(1)
            force_finish_mnsync(self.nodes[1])
            self.connect_nodes(0, 1)

            self.wait_until(lambda: len(os.listdir(self.walletnotify_dir)) == block_count, timeout=10)

            # directory content should equal the generated transaction hashes
            tx_details = list(map(lambda t: (t['txid'], t['blockheight'], t['blockhash']), self.nodes[1].listtransactions("*", block_count)))
            self.expect_wallet_notify(tx_details)


        self.log.info("test -chainlocknotify")

        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.nodes[0].sporkupdate("SPORK_19_CHAINLOCKS_ENABLED", 4070908800)
        self.wait_for_sporks_same()
        self.log.info("Mine quorum for InstantSend")
        (quorum_info_i_0, quorum_info_i_1) = self.mine_cycle_quorum()
        self.log.info("Mine quorum for ChainLocks")
        if len(self.nodes[0].quorum('list')['llmq_test']) == 0:
            self.mine_quorum(llmq_type_name='llmq_test', llmq_type=104)
        else:
            self.log.info("Quorum `llmq_test` already exist")
        self.nodes[0].sporkupdate("SPORK_19_CHAINLOCKS_ENABLED", 0)
        self.wait_for_sporks_same()

        self.log.info("Mine single block, wait for chainlock")
        self.bump_mocktime(1)
        pre_tip = self.nodes[0].getbestblockhash()
        tip = self.generate(self.nodes[0], 1, sync_fun=self.no_op)[-1]
        self.wait_for_chainlocked_block_all_nodes(tip)
        # directory content should equal the chainlocked block hash
        cl_hashes = sorted(os.listdir(self.chainlocknotify_dir))
        if len(cl_hashes) <= 1:
            assert_equal([tip], cl_hashes)
        else:
            assert_equal(sorted([tip, pre_tip]), cl_hashes)

        if self.is_wallet_compiled():
            self.log.info("test -instantsendnotify")
            assert_equal(len(os.listdir(self.instantsendnotify_dir)), 0)

            tx_count = 10
            for _ in range(tx_count):
                txid = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1)
                self.bump_mocktime(30)
                self.wait_for_instantlock(txid, self.nodes[0])

            # wait at most 10 seconds for expected number of files before reading the content
            self.bump_mocktime(30)
            self.wait_until(lambda: len(os.listdir(self.instantsendnotify_dir)) == tx_count, timeout=10)

            # directory content should equal the generated transaction hashes
            txids_rpc = list(map(lambda t: notify_outputname(self.wallet, t['txid']), self.nodes[1].listtransactions("*", tx_count)))
            assert_equal(sorted(txids_rpc), sorted(os.listdir(self.instantsendnotify_dir)))

        # TODO: add test for `-alertnotify` large fork notifications

    def expect_wallet_notify(self, tx_details):
        self.wait_until(lambda: len(os.listdir(self.walletnotify_dir)) >= len(tx_details), timeout=10)
        # Should have no more and no less files than expected
        assert_equal(sorted(notify_outputname(self.wallet, tx_id) for tx_id, _, _ in tx_details), sorted(os.listdir(self.walletnotify_dir)))
        # Should now verify contents of each file
        for tx_id, blockheight, blockhash in tx_details:
            fname = os.path.join(self.walletnotify_dir, notify_outputname(self.wallet, tx_id))
            # Wait for the cached writes to hit storage
            self.wait_until(lambda: os.path.getsize(fname) > 0, timeout=10)
            with open(fname, 'rt', encoding='utf-8') as f:
                text = f.read()
                # Universal newline ensures '\n' on 'nt'
                assert_equal(text[-1], '\n')
                text = text[:-1]
                if os.name == 'nt':
                    # On Windows, echo as above will append a whitespace
                    assert_equal(text[-1], ' ')
                    text = text[:-1]
                expected = str(blockheight) + '_' + blockhash
                assert_equal(text, expected)

        for tx_file in os.listdir(self.walletnotify_dir):
            os.remove(os.path.join(self.walletnotify_dir, tx_file))

if __name__ == '__main__':
    NotificationsTest().main()
