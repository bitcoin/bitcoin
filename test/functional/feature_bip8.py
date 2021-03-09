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
            ['-vbparams=testdummy:@-2:@-2'],
            ['-vbparams=testdummy:@144:@{}'.format(144 * 3)],
            ['-vbparams=testdummy:@144:@{}:@{}'.format(144 * 2, 144 * 4)],
        ]

    def test_height(self, height, status, mine_from):
        if height > self.height:
            self.log.info("Test status at height {}...".format(height))
            self.nodes[mine_from].generate(height - self.height)
            self.sync_blocks()
        elif height < self.height:
            assert mine_from is None
            self.log.info("Roll back to height {}...".format(height))
            old_block = self.nodes[0].getblockhash(height + 1)
            for node in self.nodes:
                node.invalidateblock(old_block)
        else:
            assert mine_from is None
            assert height == 0 and self.height == 0
            self.log.info("Test status at genesis...")

        self.height = height

        for (i, node), st in zip(enumerate(self.nodes), status):
            self.log.debug("Node #{}...".format(i))
            info = node.getblockchaininfo()
            assert_equal(info["blocks"], height)
            if st is None:
                assert "testdummy" not in info["softforks"]
            else:
                assert_equal(info["softforks"]["testdummy"]["bip8"]["status"], st)

    def run_test(self):
        self.height = 0
        self.test_height(0, [None, "defined", "defined"], None)

        # BIP 8 state transitions from "defined" to "started" or "failed" after
        # the last block of the retargeting period has been mined. This means
        # any new rules apply to transactions currently in the mempool, which
        # might be mined in the next block.
        #
        # The next retargeting period starts at block 144, so nothing should
        # happen at 142 and the state should change at 143.
        self.test_height(144-2, [None, "defined", "defined"], 0)
        self.test_height(144-1, [None, "started", "started"], 0)

        self.log.info("Test status when not signaling...")
        self.test_height(144*2-1, [None, "started", "failed"], 0)
        self.test_height(144*3-1, [None, "failed", "failed"], 0)

        # The new branch has unique block hashes, because of the signalling and
        # because generate uses a deterministic address that depends on the node
        # index.
        self.log.info("Test status when signaling...")
        self.test_height(144-1, [None, "started", "started"], None)
        self.test_height(144*2-1, [None, "locked_in", "locked_in"], 2)
        self.test_height(144*3-1, [None, "active", "locked_in"], 2)
        self.test_height(144*4-1, [None, "active", "active"], 2)

if __name__ == '__main__':
    Bip8Test().main()
