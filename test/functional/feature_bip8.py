#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# Test BIP 8 softforks
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class Bip8Test(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4
        self.setup_clean_chain = True
        # Node 0 has TestDummy inactive
        # Node 1 has a regular activation window
        # Node 2 uses speedy trial.
        # Node 3 ise like Node 1 but with lockinontimeout=true
        self.extra_args = [
            ['-vbparams=testdummy:@-2:@-2'],
            ['-vbparams=testdummy:@144:@{}'.format(144 * 3)],
            ['-vbparams=testdummy:@144:@{}:@{}'.format(144 * 3, 144 * 4)],
            ['-vbparams=testdummy:@144:@{}'.format(144 * 3), "-vblot=testdummy:true"],
        ]

    def test_height(self, height, status, mine_from, nodes):
        if height > self.height:
            self.log.info("Test status at height {}...".format(height))
            self.nodes[mine_from].generate(height - self.height)
            self.sync_blocks(nodes)
        elif height < self.height:
            assert mine_from is None
            self.log.info("Roll back to height {}...".format(height))
            old_block = self.nodes[0].getblockhash(height + 1)
            for node in nodes:
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

    def test_base_bip8(self):
        # Exclude node 3 because it does lockinontimeout which we don't want to test here
        self.stop_node(3)
        nodes = self.nodes[0:3]

        self.test_height(0, [None, "defined", "defined"], None, nodes)

        # BIP 8 state transitions from "defined" to "started" or "failed" after
        # the last block of the retargeting period has been mined. This means
        # any new rules apply to transactions currently in the mempool, which
        # might be mined in the next block.
        #
        # The next retargeting period starts at block 144, so nothing should
        # happen at 142 and the state should change at 143.
        self.test_height(144-2, [None, "defined", "defined"], 0, nodes)
        self.test_height(144-1, [None, "started", "started"], 0, nodes)

        self.log.info("Test status when not signaling...")
        self.test_height(144*2-1, [None, "started", "started"], 0, nodes)
        self.test_height(144*3-1, [None, "failed", "failed"], 0, nodes)

        # The new branch has unique block hashes, because of the signalling and
        # because generate uses a deterministic address that depends on the node
        # index.
        self.log.info("Test status when signaling...")
        self.test_height(144-1, [None, "started", "started"], None, nodes)
        self.test_height(144*2-1, [None, "locked_in", "locked_in"], 2, nodes)
        self.test_height(144*3-1, [None, "active", "locked_in"], 2, nodes)
        self.test_height(144*4-1, [None, "active", "active"], 2, nodes)

    def test_lot(self):
        # Reset all of the nodes, except node 3
        self.test_height(0, [None, "defined", "defined"], None, self.nodes[0:3])

        # Now start node3, connect it, and make sure all of the nodes are in sync
        self.start_node(3, self.extra_args[3])
        self.connect_nodes(2, 3)
        self.connect_nodes(1, 3)
        self.connect_nodes(0, 3)
        self.test_height(1, [None, "defined", "defined", "defined"], 3, self.nodes)

        self.log.info("Test must_signal transition")
        self.test_height(144*2-1, [None, "started", "started", "must_signal"], 0, self.nodes)

        self.log.info("Test must_signal threshold boundary")

        self.log.debug("Check period start, everything is 0")
        self.sync_blocks()
        info = self.nodes[3].getblockchaininfo()
        assert_equal(info["softforks"]["testdummy"]["bip8"]["status"], "must_signal")
        assert_equal(info["softforks"]["testdummy"]["bip8"]["statistics"]["count"], 0)
        assert_equal(info["softforks"]["testdummy"]["bip8"]["statistics"]["elapsed"], 0)

        self.log.debug("Mine 107 (1 under threshold) signaling blocks")
        self.nodes[3].generate(107)
        self.sync_blocks()
        info = self.nodes[3].getblockchaininfo()
        assert_equal(info["softforks"]["testdummy"]["bip8"]["status"], "must_signal")
        assert_equal(info["softforks"]["testdummy"]["bip8"]["statistics"]["count"], 107)
        assert_equal(info["softforks"]["testdummy"]["bip8"]["statistics"]["elapsed"], 107)

        self.log.debug("Mine 36 (up to 1 before end of period) non-signaling blocks")
        self.nodes[0].generate(36)
        self.sync_blocks()
        info = self.nodes[3].getblockchaininfo()
        assert_equal(info["softforks"]["testdummy"]["bip8"]["status"], "must_signal")
        assert_equal(info["softforks"]["testdummy"]["bip8"]["statistics"]["count"], 107)
        assert_equal(info["softforks"]["testdummy"]["bip8"]["statistics"]["elapsed"], 143)

        self.log.debug("Mine 1 signaling block to become locked_in")
        self.nodes[3].generate(1)
        self.sync_blocks()
        for node in self.nodes[1:]:
            info = node.getblockchaininfo()
            assert_equal(info["softforks"]["testdummy"]["bip8"]["status"], "locked_in")
        last_block_in_period = info["bestblockhash"]

        self.log.debug("Invalidate to be able to re-mine the last block in the period")
        for node in self.nodes:
            node.invalidateblock(last_block_in_period)

        self.log.debug("Mine 1 non-signaling block. Node 3 should reject it")
        self.disconnect_nodes(0, 3)
        self.disconnect_nodes(0, 2)
        self.disconnect_nodes(0, 1)
        blockhash = self.nodes[0].generate(1)[0]
        block = self.nodes[0].getblock(blockhash, 0)
        for node in self.nodes[1:3]:
            node.submitblock(block)
            info = node.getblockchaininfo()
            assert_equal(info["softforks"]["testdummy"]["bip8"]["status"], "failed")
        assert_equal(self.nodes[3].submitblock(block), "bad-vbit-unset-testdummy")

    def run_test(self):
        self.height = 0
        self.test_base_bip8()
        self.test_lot()

if __name__ == '__main__':
    Bip8Test().main()
