#!/usr/bin/env python3
# Copyright (c) 2021-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that a node in blocksonly mode does not request compact blocks."""

from test_framework.messages import (
    MSG_BLOCK,
    MSG_CMPCT_BLOCK,
    MSG_WITNESS_FLAG,
    CBlock,
    CBlockHeader,
    CInv,
    from_hex,
    HeaderAndShortIDs,
    msg_block,
    msg_cmpctblock,
    msg_getdata,
    msg_headers,
    msg_sendcmpct,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class P2PCompactBlocksBlocksOnly(BitcoinTestFramework):
    def set_test_params(self):
        self.extra_args = [["-blocksonly"], [], [], []]
        self.num_nodes = 4

    def setup_network(self):
        self.setup_nodes()
        # Start network with everyone disconnected
        self.sync_all()

    def build_block_on_tip(self):
        blockhash = self.generate(self.nodes[2], 1, sync_fun=self.no_op)[0]
        block_hex = self.nodes[2].getblock(blockhash=blockhash, verbosity=0)
        block = from_hex(CBlock(), block_hex)
        return block

    def ignores_cmpctblock(self, node_id, conn, solicited=False):
        # Briefly connect to mining node, sync, and disconnect, just to make
        # sure the node is on the same tip as the miner so that compact block
        # reconstruction works.
        self.connect_nodes(2, node_id)
        self.sync_blocks([self.nodes[2], self.nodes[node_id]], timeout=10)
        self.disconnect_nodes(2, node_id)

        block = self.build_block_on_tip()
        if solicited:
            conn.send_without_ping(msg_headers([block]))
            conn.wait_for_getdata([block.hash_int], timeout=10)
        cmpct_block = HeaderAndShortIDs()
        cmpct_block.initialize_from_block(block, use_witness=True)
        msg = msg_cmpctblock(cmpct_block.to_p2p())
        conn.send_and_ping(msg)

        cmpct_block_received = self.nodes[node_id].getbestblockhash() == cmpct_block.header.hash_hex
        if not cmpct_block_received:
            # Compact block was ignored, send the full block to keep in sync.
            conn.send_and_ping(msg_block(block))

        return not cmpct_block_received

    def run_test(self):
        # Nodes will only request hb compact blocks mode when they're out of IBD
        for node in self.nodes:
            assert not node.getblockchaininfo()['initialblockdownload']

        p2p_conn_blocksonly = self.nodes[0].add_p2p_connection(P2PInterface())
        p2p_conn_high_bw = self.nodes[1].add_p2p_connection(P2PInterface())
        p2p_conn_low_bw = self.nodes[3].add_p2p_connection(P2PInterface())
        for conn in [p2p_conn_blocksonly, p2p_conn_high_bw, p2p_conn_low_bw]:
            assert_equal(conn.message_count['sendcmpct'], 1)
            conn.send_and_ping(msg_sendcmpct(announce=False, version=2))

        # Nodes:
        #   0 -> blocksonly
        #   1 -> high bandwidth
        #   2 -> miner
        #   3 -> low bandwidth
        #
        # Topology:
        #   p2p_conn_blocksonly ---> node0
        #   p2p_conn_high_bw    ---> node1
        #   p2p_conn_low_bw     ---> node3
        #   node2 (no connections)
        #
        # node2 produces blocks that are passed to the rest of the nodes
        # through the respective p2p connections.

        self.log.info("Test that -blocksonly nodes do not select peers for BIP152 high bandwidth mode")

        block0 = self.build_block_on_tip()

        # A -blocksonly node should not request BIP152 high bandwidth mode upon
        # receiving a new valid block at the tip.
        p2p_conn_blocksonly.send_and_ping(msg_block(block0))
        assert_equal(self.nodes[0].getbestblockhash(), block0.hash_hex)
        assert_equal(p2p_conn_blocksonly.message_count['sendcmpct'], 1)
        assert_equal(p2p_conn_blocksonly.last_message['sendcmpct'].announce, False)

        # A normal node participating in transaction relay should request BIP152
        # high bandwidth mode upon receiving a new valid block at the tip.
        p2p_conn_high_bw.send_and_ping(msg_block(block0))
        assert_equal(self.nodes[1].getbestblockhash(), block0.hash_hex)
        p2p_conn_high_bw.wait_until(lambda: p2p_conn_high_bw.message_count['sendcmpct'] == 2)
        assert_equal(p2p_conn_high_bw.last_message['sendcmpct'].announce, True)

        # Don't send a block from the p2p_conn_low_bw so the low bandwidth node
        # doesn't select it for BIP152 high bandwidth relay.
        self.nodes[3].submitblock(block0.serialize().hex())

        self.log.info("Test that -blocksonly nodes send getdata(BLOCK) instead"
                      " of getdata(CMPCT) in BIP152 low bandwidth mode")

        block1 = self.build_block_on_tip()

        p2p_conn_blocksonly.send_and_ping(msg_headers(headers=[CBlockHeader(block1)]))
        assert_equal(p2p_conn_blocksonly.last_message['getdata'].inv, [CInv(MSG_BLOCK | MSG_WITNESS_FLAG, block1.hash_int)])

        p2p_conn_high_bw.send_and_ping(msg_headers(headers=[CBlockHeader(block1)]))
        assert_equal(p2p_conn_high_bw.last_message['getdata'].inv, [CInv(MSG_CMPCT_BLOCK, block1.hash_int)])

        # Send the block to avoid stalling the peer later in the test.
        comp_block = HeaderAndShortIDs()
        comp_block.initialize_from_block(block1, use_witness=True)
        block1_cb_msg = msg_cmpctblock(comp_block.to_p2p())
        p2p_conn_high_bw.send_and_ping(block1_cb_msg)

        self.log.info("Test that getdata(CMPCT) is still sent on BIP152 low bandwidth connections"
                      " when no -blocksonly nodes are involved")

        p2p_conn_low_bw.send_and_ping(msg_headers(headers=[CBlockHeader(block1)]))
        assert_equal(p2p_conn_low_bw.last_message['getdata'].inv, [CInv(MSG_CMPCT_BLOCK, block1.hash_int)])
        # Send the block to avoid stalling the peer later in the test.
        p2p_conn_low_bw.send_and_ping(block1_cb_msg)

        self.log.info("Test that -blocksonly nodes still serve compact blocks")

        def test_for_cmpctblock(block):
            if 'cmpctblock' not in p2p_conn_blocksonly.last_message:
                return False
            return p2p_conn_blocksonly.last_message['cmpctblock'].header_and_shortids.header.hash_int == block.hash_int

        p2p_conn_blocksonly.send_without_ping(msg_getdata([CInv(MSG_CMPCT_BLOCK, block0.hash_int)]))
        p2p_conn_blocksonly.wait_until(lambda: test_for_cmpctblock(block0))

        # Request BIP152 high bandwidth mode from the -blocksonly node.
        p2p_conn_blocksonly.send_and_ping(msg_sendcmpct(announce=True, version=2))

        block2 = self.build_block_on_tip()
        self.nodes[0].submitblock(block1.serialize().hex())
        self.nodes[0].submitblock(block2.serialize().hex())
        p2p_conn_blocksonly.wait_until(lambda: test_for_cmpctblock(block2))

        # This is redundant with other tests, and is here as a test-of-the-test
        self.log.info("Test that normal nodes don't ignore CMPCTBLOCK messages from HB peers")
        assert not self.ignores_cmpctblock(1, p2p_conn_high_bw, solicited=False)

        self.log.info("Test that -blocksonly nodes ignore CMPCTBLOCK messages")
        assert self.ignores_cmpctblock(0, p2p_conn_blocksonly, solicited=False)

        self.log.info("Test that low bandwidth nodes listen to CMPCTBLOCK messages when the block is requested")
        assert not self.ignores_cmpctblock(3, p2p_conn_low_bw, solicited=True)

        self.log.info("Test that -blocksonly nodes ignore CMPCTBLOCK messages even when the block is requested")
        assert self.ignores_cmpctblock(0, p2p_conn_blocksonly, solicited=True)

if __name__ == '__main__':
    P2PCompactBlocksBlocksOnly(__file__).main()
