#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# Test BIP 8 softforks
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class Bip8Test(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.setup_clean_chain = True
        # Node 0 has TestDummy inactive
        # Node 1 has a regular activation window
        # Node 2 uses speedy trial.
        self.extra_args = [
            ['-vbparams=testdummy:@1:@1'],
            ['-vbparams=testdummy:@144:@{}'.format(144 * 3)],
            ['-vbparams=testdummy:@144:@{}:@{}'.format(144 * 2, 144 * 4)],
        ]

    def run_test(self):
        self.log.info("Test status at genesis...")
        for i, node in enumerate(self.nodes):
            self.log.debug('Node #{}...'.format(i))
            info = node.getblockchaininfo()
            assert_equal(info['blocks'], 0)
            assert_equal(info["softforks"]["testdummy"]["bip8"]["status"], "defined")

        # BIP 8 state transitions from "defined" to "started" or "failed" after
        # the last block of the retargeting period has been mined. This means
        # any new rules apply to transactions currently in the mempool, which
        # might be mined in the next block.
        #
        # The next retargeting period starts at block 144, so nothing should
        # happen at 142 and the state should change at 143.
        self.log.info("Test status at height 142...")
        self.nodes[0].generate(142)
        self.sync_blocks()
        for i, node in enumerate(self.nodes):
            self.log.debug('Node #{}...'.format(i))
            info = node.getblockchaininfo()
            assert_equal(info['blocks'], 142)
            assert_equal(info["softforks"]["testdummy"]["bip8"]["status"], "defined")

        self.log.info("Test status at height 143...")
        self.nodes[0].generate(1)
        self.sync_blocks()
        for i, node in enumerate(self.nodes):
            self.log.debug('Node #{}...'.format(i))
            info = node.getblockchaininfo()
            assert_equal(info['blocks'], 143)
            status = info["softforks"]["testdummy"]["bip8"]["status"]
            assert_equal(status, "failed" if i == 0 else "started")

        height = 144 * 2 - 1
        self.log.info("Test status at height {} when not signalling...".format(height))
        self.nodes[0].generate(144)
        self.sync_blocks()
        for i, node in enumerate(self.nodes):
            self.log.debug('Node #{}...'.format(i))
            info = node.getblockchaininfo()
            assert_equal(info['blocks'], height)
            status = info["softforks"]["testdummy"]["bip8"]["status"]
            assert_equal(status, "started" if i == 1 else "failed")

        height = 144 * 3 - 1
        self.log.info("Test status at height {} when not signalling...".format(height))
        self.nodes[0].generate(144)
        self.sync_blocks()
        for i, node in enumerate(self.nodes):
            self.log.debug('Node #{}...'.format(i))
            info = node.getblockchaininfo()
            assert_equal(info['blocks'], height)
            status = info["softforks"]["testdummy"]["bip8"]["status"]
            assert_equal(status, "failed")

        height = 144 - 1
        self.log.info("Roll back to {}...".format(height))
        old_block = self.nodes[0].getblockhash(height + 1)
        for node in self.nodes:
            node.invalidateblock(old_block)
            info = node.getblockchaininfo()
            assert_equal(info['blocks'], height)

        height = 144 * 2 - 1
        self.log.info("Test status at height {} when signalling...".format(height))
        # The new branch has unique block hashes, because of the signalling and
        # because generate uses a deterministic address that depends on the node
        # index.
        self.nodes[2].generate(144)
        self.sync_blocks()

        for i, node in enumerate(self.nodes):
            self.log.debug('Node #{}...'.format(i))
            info = node.getblockchaininfo()
            assert_equal(info['blocks'], height)
            status = info["softforks"]["testdummy"]["bip8"]["status"]
            assert_equal(status, "failed" if i == 0 else "locked_in")

        height = 144 * 3 - 1
        self.log.info("Test status at height {} when signalling...".format(height))
        self.nodes[2].generate(144)
        self.sync_blocks()

        for i, node in enumerate(self.nodes):
            self.log.debug('Node #{}...'.format(i))
            info = node.getblockchaininfo()
            assert_equal(info['blocks'], height)
            status = info["softforks"]["testdummy"]["bip8"]["status"]
            assert_equal(status, "failed" if i == 0 else "active" if i == 1 else "locked_in")

        height = 144 * 4 - 1
        self.log.info("Test status at height {} when signalling...".format(height))
        self.nodes[2].generate(144)
        self.sync_blocks()

        for i, node in enumerate(self.nodes):
            self.log.debug('Node #{}...'.format(i))
            info = node.getblockchaininfo()
            assert_equal(info['blocks'], height)
            status = info["softforks"]["testdummy"]["bip8"]["status"]
            assert_equal(status, "failed" if i == 0 else "active")

if __name__ == '__main__':
    Bip8Test().main()
