#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# Test BIP 8 softforks
from collections import namedtuple
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

RestartedNodeInfo = namedtuple("RestartedNodeInfo", ("node", "extra_args", "status"))

class Bip8Test(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4
        self.setup_clean_chain = True
        self.extra_args = [
            ['-vbparams=testdummy:@-2:@-2'], # Node 0 has TestDummy inactive
            ['-vbparams=testdummy:@144:@{}'.format(144 * 3)], # Node 1 has regular activation window
            ['-vbparams=testdummy:@144:@{}:@{}'.format(144 * 3, 144 * 5)], # Node 2 has minimum activation height
            ['-vbparams=testdummy:@-2:@-2'], # Node 3 has TestDummy inactive, but will be restarted with different params
        ]

    def setup_network(self):
        self.setup_nodes()

    def test_height(self, height, status, mine_from, restart):
        if height > self.height:
            self.log.info("Test status at height {}...".format(height))
            while self.height < height:
                blockhash = self.nodes[mine_from].generate(1)[0]
                block = self.nodes[mine_from].getblock(blockhash, 0)
                for node in self.nodes:
                    node.submitblock(block)
                    assert_equal(node.getbestblockhash(), blockhash)
                self.height += 1
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

        if restart:
            # Restart this node and check that the status is what we expect
            self.restart_node(restart.node, restart.extra_args)
            info = self.nodes[restart.node].getblockchaininfo()
            assert_equal(info["blocks"], height)
            if restart.status is None:
                assert "testdummy" not in info["softforks"]
            else:
                assert_equal(info["softforks"]["testdummy"]["bip8"]["status"], restart.status)

            # Now restart again to go back to the original settings
            self.restart_node(restart.node, self.extra_args[restart.node])
            info = self.nodes[restart.node].getblockchaininfo()
            assert_equal(info["blocks"], height)
            if status[restart.node] is None:
                assert "testdummy" not in info["softforks"]
            else:
                assert_equal(info["softforks"]["testdummy"]["bip8"]["status"], status[restart.node])

    def run_test(self):
        self.log.info("Checking -vbparams")
        self.stop_node(3)
        self.nodes[3].assert_start_raises_init_error(extra_args=["-vbparams=testdummy:@-2:@1"], expected_msg="Error: When one of startheight or timeoutheight is -2, both must be -2")
        self.nodes[3].assert_start_raises_init_error(extra_args=["-vbparams=testdummy:@1:@-2"], expected_msg="Error: When one of startheight or timeoutheight is -2, both must be -2")
        self.nodes[3].assert_start_raises_init_error(extra_args=["-vbparams=testdummy:@1:@1"], expected_msg="Error: Invalid startheight (1), must be a multiple of 144")
        self.nodes[3].assert_start_raises_init_error(extra_args=["-vbparams=testdummy:@144:@1"], expected_msg="Error: Invalid timeoutheight (1), must be a multiple of 144")
        self.nodes[3].assert_start_raises_init_error(extra_args=["-vbparams=testdummy:@144:@144:@-1"], expected_msg="Error: Invalid minimum activation height (-1), cannot be negative")
        self.nodes[3].assert_start_raises_init_error(extra_args=["-vbparams=testdummy:@144:@144:@1"], expected_msg="Error: Invalid minimum activation height (1), must be a multiple of 144")
        self.nodes[3].assert_start_raises_init_error(extra_args=["-vbparams=testdummy:@288:@144"], expected_msg="Error: Invalid timeoutheight (144), must be at least two periods greater than the startheight (288)")
        self.nodes[3].assert_start_raises_init_error(extra_args=["-vbparams=testdummy:@-3:@144"], expected_msg="Error: Invalid startheight (-3), cannot be negative (except for never or always active special cases)")
        self.nodes[3].assert_start_raises_init_error(extra_args=["-vbparams=testdummy:@144:@-1"], expected_msg="Error: Invalid timeoutheight (-1), cannot be negative (except for never or always active special cases)")
        self.nodes[3].assert_start_raises_init_error(extra_args=["-vbparams=testdummy:@{}:@144".format(0x7fffffff + 1)], expected_msg="Error: Invalid startheight (@{})".format(0x7fffffff + 1))
        self.nodes[3].assert_start_raises_init_error(extra_args=["-vbparams=testdummy:@144:@{}".format(0x7fffffff + 1)], expected_msg="Error: Invalid timeoutheight (@{})".format(0x7fffffff + 1))
        self.nodes[3].assert_start_raises_init_error(extra_args=["-vbparams=testdummy:@{}:@144".format(-(0x7fffffff + 2))], expected_msg="Error: Invalid startheight (@{})".format(-(0x7fffffff + 2)))
        self.nodes[3].assert_start_raises_init_error(extra_args=["-vbparams=testdummy:@144:@{}".format(-(0x7fffffff + 2))], expected_msg="Error: Invalid timeoutheight (@{})".format(-(0x7fffffff + 2)))
        self.start_node(3, self.extra_args[3])

        self.height = 0
        self.test_height(0, [None, "defined", "defined", None], None, RestartedNodeInfo(3, ["-vbparams=testdummy:@144:@{}".format(144*3)], "defined"))

        # BIP 8 state transitions from "defined" to "started" or "failed" after
        # the last block of the retargeting period has been mined. This means
        # any new rules apply to transactions currently in the mempool, which
        # might be mined in the next block.
        #
        # The next retargeting period starts at block 144, so nothing should
        # happen at 142 and the state should change at 143.
        self.test_height(144-2, [None, "defined", "defined", None], 0, RestartedNodeInfo(3, ["-vbparams=testdummy:@144:@{}".format(144*3)], "defined"))
        self.test_height(144-1, [None, "started", "started", None], 0, RestartedNodeInfo(3, ["-vbparams=testdummy:@144:@{}".format(144*3)], "started"))

        self.log.info("Test status when not signaling...")
        self.test_height(144*2-1, [None, "started", "started", None], 0, RestartedNodeInfo(3, ["-vbparams=testdummy:@144:@{}".format(144*3)], "started"))
        self.test_height(144*3-1, [None, "failed", "failed", None], 0, RestartedNodeInfo(3, ["-vbparams=testdummy:@144:@{}".format(144*3)], "failed"))

        # The new branch has unique block hashes, because of the signalling and
        # because generate uses a deterministic address that depends on the node
        # index.
        # Also tests that if the last period meets the threshold, the fork activates.
        self.log.info("Test status when signaling...")
        self.test_height(144-1, [None, "started", "started", None], None, RestartedNodeInfo(3, ["-vbparams=testdummy:@144:@{}".format(144*3)], "started"))
        self.test_height(144*2-1, [None, "started", "started", None], 3, RestartedNodeInfo(3, ["-vbparams=testdummy:@144:@{}".format(144*3)], "started"))
        self.test_height(144*3-1, [None, "locked_in", "locked_in", None], 1, RestartedNodeInfo(3, ["-vbparams=testdummy:@144:@{}".format(144*3)], "locked_in"))
        self.test_height(144*4-1, [None, "active", "locked_in", None], 1, RestartedNodeInfo(3, ["-vbparams=testdummy:@144:@{}".format(144*3)], "active"))
        self.test_height(144*5-1, [None, "active", "active", None], 1, RestartedNodeInfo(3, ["-vbparams=testdummy:@144:@{}".format(144*3)], "active"))

if __name__ == '__main__':
    Bip8Test().main()
