#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the -alertnotify, -blocknotify and -walletnotify options."""
import os
import platform

from test_framework.address import ADDRESS_BCRT1_UNSPENDABLE
from test_framework.descriptors import descsum_create
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)

# Linux allow all characters other than \x00
# Windows disallow control characters (0-31) and /\?%:|"<>
FILE_CHAR_START = 32 if platform.system() == 'Windows' else 1
FILE_CHAR_END = 128
FILE_CHARS_DISALLOWED = '/\\?%*:|"<>' if platform.system() == 'Windows' else '/'
UNCONFIRMED_HASH_STRING = 'unconfirmed'

def notify_outputname(walletname, txid):
    return txid if platform.system() == 'Windows' else f'{walletname}_{txid}'


class NotificationsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.uses_wallet = None

    def setup_network(self):
        self.wallet = ''.join(chr(i) for i in range(FILE_CHAR_START, FILE_CHAR_END) if chr(i) not in FILE_CHARS_DISALLOWED)
        self.alertnotify_dir = os.path.join(self.options.tmpdir, "alertnotify")
        self.blocknotify_dir = os.path.join(self.options.tmpdir, "blocknotify")
        self.walletnotify_dir = os.path.join(self.options.tmpdir, "walletnotify")
        self.shutdownnotify_dir = os.path.join(self.options.tmpdir, "shutdownnotify")
        self.shutdownnotify_file = os.path.join(self.shutdownnotify_dir, "shutdownnotify.txt")
        os.mkdir(self.alertnotify_dir)
        os.mkdir(self.blocknotify_dir)
        os.mkdir(self.walletnotify_dir)
        os.mkdir(self.shutdownnotify_dir)

        # -alertnotify and -blocknotify on node0, walletnotify on node1
        self.extra_args = [[
            f"-alertnotify=echo > {os.path.join(self.alertnotify_dir, '%s')}",
            f"-blocknotify=echo > {os.path.join(self.blocknotify_dir, '%s')}",
            f"-shutdownnotify=echo > {self.shutdownnotify_file}",
        ], [
            f"-walletnotify=echo %h_%b > {os.path.join(self.walletnotify_dir, notify_outputname('%w', '%s'))}",
        ]]
        self.wallet_names = [self.default_wallet_name, self.wallet]
        super().setup_network()

    def run_test(self):
        if self.is_wallet_compiled():
            # Setup the descriptors to be imported to the wallet
            xpriv = "tprv8ZgxMBicQKsPfHCsTwkiM1KT56RXbGGTqvc2hgqzycpwbHqqpcajQeMRZoBD35kW4RtyCemu6j34Ku5DEspmgjKdt2qe4SvRch5Kk8B8A2v"
            desc_imports = [{
                "desc": descsum_create(f"wpkh({xpriv}/0/*)"),
                "timestamp": 0,
                "active": True,
                "keypool": True,
            },{
                "desc": descsum_create(f"wpkh({xpriv}/1/*)"),
                "timestamp": 0,
                "active": True,
                "keypool": True,
                "internal": True,
            }]
            # Make the wallets and import the descriptors
            # Ensures that node 0 and node 1 share the same wallet for the conflicting transaction tests below.
            for i, name in enumerate(self.wallet_names):
                self.nodes[i].createwallet(wallet_name=name, blank=True, load_on_startup=True)
                self.nodes[i].importdescriptors(desc_imports)

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
            self.expect_wallet_notify(tx_details)

            self.log.info("test -walletnotify after rescan")
            # rescan to force wallet notifications
            self.nodes[1].rescanblockchain()
            self.wait_until(lambda: len(os.listdir(self.walletnotify_dir)) == block_count, timeout=10)

            self.connect_nodes(0, 1)

            # directory content should equal the generated transaction hashes
            tx_details = list(map(lambda t: (t['txid'], t['blockheight'], t['blockhash']), self.nodes[1].listtransactions("*", block_count)))
            self.expect_wallet_notify(tx_details)

            # Conflicting transactions tests.
            # Generate spends from node 0, and check notifications
            # triggered by node 1
            self.log.info("test -walletnotify with conflicting transactions")
            self.nodes[0].rescanblockchain()
            self.generatetoaddress(self.nodes[0], 100, ADDRESS_BCRT1_UNSPENDABLE)

            # Generate transaction on node 0, sync mempools, and check for
            # notification on node 1.
            tx1 = self.nodes[0].sendtoaddress(address=ADDRESS_BCRT1_UNSPENDABLE, amount=1, replaceable=True)
            assert_equal(tx1 in self.nodes[0].getrawmempool(), True)
            self.sync_mempools()
            self.expect_wallet_notify([(tx1, -1, UNCONFIRMED_HASH_STRING)])

            # Generate bump transaction, sync mempools, and check for bump1
            # notification. In the future, per
            # https://github.com/bitcoin/bitcoin/pull/9371, it might be better
            # to have notifications for both tx1 and bump1.
            bump1 = self.nodes[0].bumpfee(tx1)["txid"]
            assert_equal(bump1 in self.nodes[0].getrawmempool(), True)
            self.sync_mempools()
            self.expect_wallet_notify([(bump1, -1, UNCONFIRMED_HASH_STRING)])

            # Add bump1 transaction to new block, checking for a notification
            # and the correct number of confirmations.
            blockhash1 = self.generatetoaddress(self.nodes[0], 1, ADDRESS_BCRT1_UNSPENDABLE)[0]
            blockheight1 = self.nodes[0].getblockcount()
            self.sync_blocks()
            self.expect_wallet_notify([(bump1, blockheight1, blockhash1)])
            assert_equal(self.nodes[1].gettransaction(bump1)["confirmations"], 1)

            # Generate a second transaction to be bumped.
            tx2 = self.nodes[0].sendtoaddress(address=ADDRESS_BCRT1_UNSPENDABLE, amount=1, replaceable=True)
            assert_equal(tx2 in self.nodes[0].getrawmempool(), True)
            self.sync_mempools()
            self.expect_wallet_notify([(tx2, -1, UNCONFIRMED_HASH_STRING)])

            # Bump tx2 as bump2 and generate a block on node 0 while
            # disconnected, then reconnect and check for notifications on node 1
            # about newly confirmed bump2 and newly conflicted tx2.
            self.disconnect_nodes(0, 1)
            bump2 = self.nodes[0].bumpfee(tx2)["txid"]
            blockhash2 = self.generatetoaddress(self.nodes[0], 1, ADDRESS_BCRT1_UNSPENDABLE, sync_fun=self.no_op)[0]
            blockheight2 = self.nodes[0].getblockcount()
            assert_equal(self.nodes[0].gettransaction(bump2)["confirmations"], 1)
            assert_equal(tx2 in self.nodes[1].getrawmempool(), True)
            self.connect_nodes(0, 1)
            self.sync_blocks()
            self.expect_wallet_notify([(bump2, blockheight2, blockhash2), (tx2, -1, UNCONFIRMED_HASH_STRING)])
            assert_equal(self.nodes[1].gettransaction(bump2)["confirmations"], 1)

        # TODO: add test for `-alertnotify` large fork notifications

        self.log.info("test -shutdownnotify")
        self.stop_nodes()
        self.wait_until(lambda: os.path.isfile(self.shutdownnotify_file), timeout=10)

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
                if platform.system() == 'Windows':
                    # On Windows, echo as above will append a whitespace
                    assert_equal(text[-1], ' ')
                    text = text[:-1]
                expected = str(blockheight) + '_' + blockhash
                assert_equal(text, expected)

        for tx_file in os.listdir(self.walletnotify_dir):
            os.remove(os.path.join(self.walletnotify_dir, tx_file))


if __name__ == '__main__':
    NotificationsTest().main()
