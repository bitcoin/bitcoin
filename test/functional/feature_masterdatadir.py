#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the master datadir feature.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    connect_nodes,
    get_datadir_path,
    mine_large_block,
)
import os

# Copied from feature_pruning.py; should probably be moved into util.py
def calc_usage(blockdir):
    return sum(os.path.getsize(blockdir+f) for f in os.listdir(blockdir) if os.path.isfile(os.path.join(blockdir, f))) / (1024. * 1024.)

class MasterDatadirTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_nodes(self):
        self.masterdatadir = get_datadir_path(self.options.tmpdir, 0)
        # Create node 0 to mine.
        # Create node 1 to test -masterdatadir.
        self.extra_args = [[],
                           ["-masterdatadir=" + self.masterdatadir]]
        self.add_nodes(2, self.extra_args)
        self.start_node(0)

        self.blocks0 = os.path.join(self.nodes[0].datadir, 'regtest', 'blocks', '')
        self.blocks1 = os.path.join(self.nodes[1].datadir, 'regtest', 'blocks', '')

    def setup_network(self):
        self.setup_nodes()

    def run_test(self):
        self.log.info("Mining large blocks until we have more than one blk file")
        # Make stuff
        self.nodes[0].generate(150)
        assert_equal(150, self.nodes[0].getblockcount())
        # We want more than one block file, so make a few large blocks
        for i in range(150):
            mine_large_block(self.nodes[0])
            if i % 50 == 0:
                self.log.info("... %s / 150", str(i))
        assert_equal(300, self.nodes[0].getblockcount())
        # Check datadir space use
        node0use = calc_usage(self.blocks0)
        self.log.info("Node uses %s MB", str(node0use))
        assert 140. < node0use < 160.
        self.log.info("Basic setup test")
        # Shutdown
        self.stop_node(0)
        # Start up with master dir
        self.start_node(1, ["-masterdatadir=" + self.masterdatadir])
        # Get block count
        assert_equal(300, self.nodes[1].getblockcount())
        # Check datadir space use
        node1use = calc_usage(self.blocks1)
        assert node1use < 20.
        # Generate blocks
        self.nodes[1].generate(100)
        assert_equal(400, self.nodes[1].getblockcount())
        # Check datadir space use
        node1use = calc_usage(self.blocks1)
        assert node1use < 20.
        # Go back to node 0
        self.stop_node(1)
        self.start_node(0)
        assert_equal(300, self.nodes[0].getblockcount())
        # Get both online and synced up
        self.start_node(1, ["-masterdatadir=" + self.masterdatadir])
        connect_nodes(self.nodes[0], 1)
        self.sync_all()
        assert_equal(400, self.nodes[0].getblockcount())
        assert_equal(400, self.nodes[1].getblockcount())
        node0use = calc_usage(self.blocks0)
        node1use = calc_usage(self.blocks1)
        assert 140. < node0use < 160.
        assert node1use < 20.

        # Test case where master mines an additional block file while slave is
        # offline - slave should never reference new master blocks file
        self.log.info("Post-init master block file increase test")

        # Shut down node 1
        self.stop_node(1)
        # Make more large blocks
        for i in range(150):
            mine_large_block(self.nodes[0])
            if i % 50 == 0:
                self.log.info("... %s / 150", str(i))
        node0use = calc_usage(self.blocks0)
        self.log.info("Node uses %s MB", str(node0use))
        for f in os.listdir(self.blocks0):
            print(f)
        # Start node 1 back up
        self.start_node(1, ["-masterdatadir=" + self.masterdatadir])
        connect_nodes(self.nodes[0], 1)
        self.sync_all()
        assert_equal(self.nodes[0].getblockcount(), self.nodes[1].getblockcount())
        node1use = calc_usage(self.blocks1)
        assert node1use < 180.

        # Reindex the blockchain as node 1; it should directly refer to master
        # blocks and its own blocks as appropriate
        self.stop_node(1)
        self.start_node(1, ["-masterdatadir=" + self.masterdatadir, "-reindex", "-reindex-chainstate"])
        connect_nodes(self.nodes[0], 1)
        self.sync_all()
        node1use = calc_usage(self.blocks1)
        assert node1use < 180.

if __name__ == '__main__':
    MasterDatadirTest().main()
