#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.blocktools import (
    create_block,
    create_coinbase,
    add_witness_commitment,
)
from test_framework.p2p import P2PInterface, msg_block
from test_framework.test_framework import BitcoinTestFramework

class AssumeValidPruningTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.rpc_timeout = 120
        self.blocks = []

    def setup_network(self):
        self.add_nodes(3)
        self.start_node(0)

    def create_blocks(self, num):
        # Build the blockchain
        tip = int(self.nodes[0].getbestblockhash(), 16)
        block_time = self.nodes[0].getblock(
            self.nodes[0].getbestblockhash())['time'] + 1

        for i in range(num):
            # TODO create "large" blocks that contain txs with witness data
            block = create_block(tip, create_coinbase(height=i+1), block_time)
            add_witness_commitment(block, 0)
            block.solve()
            self.blocks.append(block)
            tip = block.sha256
            block_time += 1

    def run_test(self):
        self.create_blocks(num=2500)

        node0 = self.nodes[0].add_p2p_connection(P2PInterface())
        for block in self.blocks:
            node0.send_message(msg_block(block))
        node0.sync_with_ping(960)

        assume_valid_height = 100
        minimum_work_height = len(self.blocks) - 1
        # Start node 1 assume-valid and manual pruning
        self.start_node(1, extra_args=[
            "-assumevalid=" + hex(self.blocks[assume_valid_height-1].sha256),
            # TODO if we don't set the min. work, we might end up failing to
            # validate a block because we request it with witnesses but when
            # the block arrives and our best header advanced the block would be
            # considered assumed-valid (leading to unexpected witnesses).
            "-minimumchainwork=" + self.nodes[0].getblock(self.blocks[minimum_work_height].hash, 1)['chainwork'],
            "-prune=1",
        ])

        # Trigger IBD for node 1 and wait for it to finish.
        self.connect_nodes(0, 1)
        self.sync_blocks(
            nodes=[self.nodes[0], self.nodes[1]],
            timeout=960
        )

        # Request all blocks from node 1 and check that only the blocks below the
        # assume-valid point do not have witnesses.
        for i in range(len(self.blocks)):
            block = self.nodes[1].getblock(blockhash=self.blocks[i].hash, verbosity=3)
            for tx in block['tx']:
                if i < assume_valid_height:
                    assert ('txinwitness' not in tx_input for tx_input in tx['vin'])
                else:
                    assert ('txinwitness' in tx_input for tx_input in tx['vin'])

        # Check that restarting node 1 raises an init error for having pruned
        # witnesses.
        self.stop_node(1)
        self.nodes[1].assert_start_raises_init_error(extra_args=['-prune=0'])

        # Start node 2 assume-valid and manual pruning
        self.start_node(2, extra_args=[
            "-assumevalid=" + hex(self.blocks[assume_valid_height-1].sha256),
            "-minimumchainwork=" + self.nodes[0].getblock(self.blocks[minimum_work_height].hash, 1)['chainwork'],
            "-prune=1",
            # We will be changing the assume-valid hash at this height
            "-stopatheight=" + str(assume_valid_height + 10),
        ])

        self.connect_nodes(0, 2)
        self.nodes[2].wait_until_stopped(timeout=960)

        # Restart node 2 with a different assume-valid hash
        self.start_node(2, extra_args=[
            "-assumevalid=" + hex(self.blocks[assume_valid_height + 100 - 1].sha256),
            "-minimumchainwork=" + self.nodes[0].getblock(self.blocks[minimum_work_height].hash, 1)['chainwork'],
            "-prune=1",
            # We will be changing the assume-valid hash at this height
            "-stopatheight=200",
        ])

        self.connect_nodes(0, 2)
        self.nodes[2].wait_until_stopped(timeout=960)

        # Restart node 2 with a different assume-valid hash
        self.start_node(2, extra_args=[
            "-assumevalid=0",
            "-minimumchainwork=" + self.nodes[0].getblock(self.blocks[minimum_work_height].hash, 1)['chainwork'],
            "-prune=1",
        ])

        self.connect_nodes(0, 2)
        self.sync_blocks(nodes=[self.nodes[0], self.nodes[2]], timeout=960)

        for i in range(len(self.blocks)):
            block = self.nodes[2].getblock(blockhash=self.blocks[i].hash, verbosity=3)
            for tx in block['tx']:
                if i < assume_valid_height or (i >= assume_valid_height + 10 and i < assume_valid_height + 100):
                    assert ('txinwitness' not in tx_input for tx_input in tx['vin'])
                else:
                    assert ('txinwitness' in tx_input for tx_input in tx['vin'])

if __name__ == '__main__':
    AssumeValidPruningTest().main()
