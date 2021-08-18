#!/usr/bin/env python3
# Copyright (c) 2017-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test logic for setting nMinimumChainWork on command line.

Nodes don't consider themselves out of "initial block download" until
their active chain has more work than nMinimumChainWork.

Nodes don't download blocks from a peer unless the peer's best known block
has more work than nMinimumChainWork.

While in initial block download, nodes won't relay blocks to their peers, so
test that this parameter functions as intended by verifying that block relay
only succeeds past a given node once its nMinimumChainWork has been exceeded.
"""

import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

# 2 hashes required per regtest block (with no difficulty adjustment)
REGTEST_WORK_PER_BLOCK = 2

class MinimumChainWorkTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3

        self.extra_args = [[], ["-minimumchainwork=0x65"], ["-minimumchainwork=0x65"]]
        self.node_min_work = [0, 101, 101]

    def setup_network(self):
        # This test relies on the chain setup being:
        # node0 <- node1 <- node2
        # Before leaving IBD, nodes prefer to download blocks from outbound
        # peers, so ensure that we're mining on an outbound peer and testing
        # block relay to inbound peers.
        self.setup_nodes()
        for i in range(self.num_nodes-1):
            self.connect_nodes(i+1, i)

    def run_test(self):
        # Start building a chain on node0.  node2 shouldn't be able to sync until node1's
        # minchainwork is exceeded
        starting_chain_work = REGTEST_WORK_PER_BLOCK # Genesis block's work
        self.log.info(f"Testing relay across node 1 (minChainWork = {self.node_min_work[1]})")

        starting_blockcount = self.nodes[2].getblockcount()

        num_blocks_to_generate = int((self.node_min_work[1] - starting_chain_work) / REGTEST_WORK_PER_BLOCK)
        self.log.info(f"Generating {num_blocks_to_generate} blocks on node0")
        hashes = self.nodes[0].generatetoaddress(num_blocks_to_generate,
                                                 self.nodes[0].get_deterministic_priv_key().address)

        self.log.info(f"Node0 current chain work: {self.nodes[0].getblockheader(hashes[-1])['chainwork']}")

        # Sleep a few seconds and verify that node2 didn't get any new blocks
        # or headers.  We sleep, rather than sync_blocks(node0, node1) because
        # it's reasonable either way for node1 to get the blocks, or not get
        # them (since they're below node1's minchainwork).
        time.sleep(3)

        self.log.info("Verifying node 2 has no more blocks than before")
        self.log.info(f"Blockcounts: {[n.getblockcount() for n in self.nodes]}")
        # Node2 shouldn't have any new headers yet, because node1 should not
        # have relayed anything.
        assert_equal(len(self.nodes[2].getchaintips()), 1)
        assert_equal(self.nodes[2].getchaintips()[0]['height'], 0)

        assert self.nodes[1].getbestblockhash() != self.nodes[0].getbestblockhash()
        assert_equal(self.nodes[2].getblockcount(), starting_blockcount)

        self.log.info("Generating one more block")
        self.nodes[0].generatetoaddress(1, self.nodes[0].get_deterministic_priv_key().address)

        self.log.info("Verifying nodes are all synced")

        # Because nodes in regtest are all manual connections (eg using
        # addnode), node1 should not have disconnected node0. If not for that,
        # we'd expect node1 to have disconnected node0 for serving an
        # insufficient work chain, in which case we'd need to reconnect them to
        # continue the test.

        self.sync_all()
        self.log.info(f"Blockcounts: {[n.getblockcount() for n in self.nodes]}")

if __name__ == '__main__':
    MinimumChainWorkTest().main()
