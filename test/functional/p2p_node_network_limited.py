#!/usr/bin/env python3
# Copyright (c) 2017-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests NODE_NETWORK_LIMITED.

Tests that a node configured with -prune=550 signals NODE_NETWORK_LIMITED correctly
and that it responds to getdata requests for blocks correctly:
    - send a block within 288 + 2 of the tip
    - disconnect peers who request blocks older than that."""
from test_framework.messages import (
    CInv,
    MSG_BLOCK,
    NODE_NETWORK_LIMITED,
    NODE_P2P_V2,
    NODE_WITNESS,
    msg_getdata,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    try_rpc
)

# Minimum blocks required to signal NODE_NETWORK_LIMITED #
NODE_NETWORK_LIMITED_MIN_BLOCKS = 288

class P2PIgnoreInv(P2PInterface):
    firstAddrnServices = 0
    def on_inv(self, message):
        # The node will send us invs for other blocks. Ignore them.
        pass
    def on_addr(self, message):
        self.firstAddrnServices = message.addrs[0].nServices
    def wait_for_addr(self, timeout=5):
        test_function = lambda: self.last_message.get("addr")
        self.wait_until(test_function, timeout=timeout)
    def send_getdata_for_block(self, blockhash):
        getdata_request = msg_getdata()
        getdata_request.inv.append(CInv(MSG_BLOCK, int(blockhash, 16)))
        self.send_message(getdata_request)

class NodeNetworkLimitedTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [['-prune=550'], [], []]

    def disconnect_all(self):
        self.disconnect_nodes(0, 1)
        self.disconnect_nodes(0, 2)
        self.disconnect_nodes(1, 2)

    def setup_network(self):
        self.add_nodes(self.num_nodes, self.extra_args)
        self.start_nodes()

    def test_avoid_requesting_historical_blocks(self):
        self.log.info("Test full node does not request blocks beyond the limited peer threshold")
        pruned_node = self.nodes[0]
        miner = self.nodes[1]
        full_node = self.nodes[2]

        # Connect and generate block to ensure IBD=false
        self.connect_nodes(1, 0)
        self.connect_nodes(1, 2)
        self.generate(miner, 1)

        # Verify peers are out of IBD
        for node in self.nodes:
            assert not node.getblockchaininfo()['initialblockdownload']

        # Isolate full_node (the node will remain out of IBD)
        full_node.setnetworkactive(False)
        self.wait_until(lambda: len(full_node.getpeerinfo()) == 0)

        # Mine blocks and sync the pruned node. Surpass the NETWORK_NODE_LIMITED threshold.
        # Blocks deeper than the threshold are considered "historical blocks"
        num_historial_blocks = 12
        self.generate(miner, NODE_NETWORK_LIMITED_MIN_BLOCKS + num_historial_blocks, sync_fun=self.no_op)
        self.sync_blocks([miner, pruned_node])

        # Connect full_node to prune_node and check peers don't disconnect right away.
        # (they will disconnect if full_node, which is chain-wise behind, request blocks
        # older than NODE_NETWORK_LIMITED_MIN_BLOCKS)
        start_height_full_node = full_node.getblockcount()
        full_node.setnetworkactive(True)
        self.connect_nodes(2, 0)
        assert_equal(len(full_node.getpeerinfo()), 1)

        # Wait until the full_node is headers-wise sync
        best_block_hash = pruned_node.getbestblockhash()
        default_value = {'status': ''}  # No status
        self.wait_until(lambda: next(filter(lambda x: x['hash'] == best_block_hash, full_node.getchaintips()), default_value)['status'] == "headers-only")

        # Now, since the node aims to download a window of 1024 blocks,
        # ensure it requests the blocks below the threshold only (with a
        # 2-block buffer). And also, ensure it does not request any
        # historical block.
        tip_height = pruned_node.getblockcount()
        limit_buffer = 2
        # Prevent races by waiting for the tip to arrive first
        self.wait_until(lambda: not try_rpc(-1, "Block not available (not fully downloaded)", full_node.getblock, pruned_node.getbestblockhash()))
        for height in range(start_height_full_node + 1, tip_height + 1):
            if height <= tip_height - (NODE_NETWORK_LIMITED_MIN_BLOCKS - limit_buffer):
                assert_raises_rpc_error(-1, "Block not available (not fully downloaded)", full_node.getblock, pruned_node.getblockhash(height))
            else:
                full_node.getblock(pruned_node.getblockhash(height))  # just assert it does not throw an exception

        # Lastly, ensure the full_node is not sync and verify it can get synced by
        # establishing a connection with another full node capable of providing them.
        assert_equal(full_node.getblockcount(), start_height_full_node)
        self.connect_nodes(2, 1)
        self.sync_blocks([miner, full_node])

    def run_test(self):
        node = self.nodes[0].add_p2p_connection(P2PIgnoreInv())

        expected_services = NODE_WITNESS | NODE_NETWORK_LIMITED
        if self.options.v2transport:
            expected_services |= NODE_P2P_V2

        self.log.info("Check that node has signalled expected services.")
        assert_equal(node.nServices, expected_services)

        self.log.info("Check that the localservices is as expected.")
        assert_equal(int(self.nodes[0].getnetworkinfo()['localservices'], 16), expected_services)

        self.log.info("Mine enough blocks to reach the NODE_NETWORK_LIMITED range.")
        self.connect_nodes(0, 1)
        blocks = self.generate(self.nodes[1], 292, sync_fun=lambda: self.sync_blocks([self.nodes[0], self.nodes[1]]))

        self.log.info("Make sure we can max retrieve block at tip-288.")
        node.send_getdata_for_block(blocks[1])  # last block in valid range
        node.wait_for_block(int(blocks[1], 16), timeout=3)

        self.log.info("Requesting block at height 2 (tip-289) must fail (ignored).")
        node.send_getdata_for_block(blocks[0])  # first block outside of the 288+2 limit
        node.wait_for_disconnect(timeout=5)
        self.nodes[0].disconnect_p2ps()

        # connect unsynced node 2 with pruned NODE_NETWORK_LIMITED peer
        # because node 2 is in IBD and node 0 is a NODE_NETWORK_LIMITED peer, sync must not be possible
        self.connect_nodes(0, 2)
        try:
            self.sync_blocks([self.nodes[0], self.nodes[2]], timeout=5)
        except Exception:
            pass
        # node2 must remain at height 0
        assert_equal(self.nodes[2].getblockheader(self.nodes[2].getbestblockhash())['height'], 0)

        # now connect also to node 1 (non pruned)
        self.connect_nodes(1, 2)

        # sync must be possible
        self.sync_blocks()

        # disconnect all peers
        self.disconnect_all()

        # mine 10 blocks on node 0 (pruned node)
        self.generate(self.nodes[0], 10, sync_fun=self.no_op)

        # connect node1 (non pruned) with node0 (pruned) and check if the can sync
        self.connect_nodes(0, 1)

        # sync must be possible, node 1 is no longer in IBD and should therefore connect to node 0 (NODE_NETWORK_LIMITED)
        self.sync_blocks([self.nodes[0], self.nodes[1]])

        self.test_avoid_requesting_historical_blocks()

if __name__ == '__main__':
    NodeNetworkLimitedTest(__file__).main()
