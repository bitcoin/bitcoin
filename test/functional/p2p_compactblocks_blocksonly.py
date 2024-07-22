#!/usr/bin/env python3
# Copyright (c) 2021-2021 The Bitcoin Core developers
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
    msg_block,
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
        block.rehash()
        return block

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
        assert_equal(int(self.nodes[0].getbestblockhash(), 16), block0.sha256)
        assert_equal(p2p_conn_blocksonly.message_count['sendcmpct'], 1)
        assert_equal(p2p_conn_blocksonly.last_message['sendcmpct'].announce, False)

        # A normal node participating in transaction relay should request BIP152
        # high bandwidth mode upon receiving a new valid block at the tip.
        p2p_conn_high_bw.send_and_ping(msg_block(block0))
        assert_equal(int(self.nodes[1].getbestblockhash(), 16), block0.sha256)
        p2p_conn_high_bw.wait_until(lambda: p2p_conn_high_bw.message_count['sendcmpct'] == 2)
        assert_equal(p2p_conn_high_bw.last_message['sendcmpct'].announce, True)

        # Don't send a block from the p2p_conn_low_bw so the low bandwidth node
        # doesn't select it for BIP152 high bandwidth relay.
        self.nodes[3].submitblock(block0.serialize().hex())

        self.log.info("Test that -blocksonly nodes send getdata(BLOCK) instead"
                      " of getdata(CMPCT) in BIP152 low bandwidth mode")

        block1 = self.build_block_on_tip()

        p2p_conn_blocksonly.send_message(msg_headers(headers=[CBlockHeader(block1)]))
        p2p_conn_blocksonly.sync_with_ping()
        assert_equal(p2p_conn_blocksonly.last_message['getdata'].inv, [CInv(MSG_BLOCK | MSG_WITNESS_FLAG, block1.sha256)])

        p2p_conn_high_bw.send_message(msg_headers(headers=[CBlockHeader(block1)]))
        p2p_conn_high_bw.sync_with_ping()
        assert_equal(p2p_conn_high_bw.last_message['getdata'].inv, [CInv(MSG_CMPCT_BLOCK, block1.sha256)])

        self.log.info("Test that getdata(CMPCT) is still sent on BIP152 low bandwidth connections"
                      " when no -blocksonly nodes are involved")

        p2p_conn_low_bw.send_and_ping(msg_headers(headers=[CBlockHeader(block1)]))
        p2p_conn_low_bw.sync_with_ping()
        assert_equal(p2p_conn_low_bw.last_message['getdata'].inv, [CInv(MSG_CMPCT_BLOCK, block1.sha256)])

        self.log.info("Test that -blocksonly nodes still serve compact blocks")

        def test_for_cmpctblock(block):
            if 'cmpctblock' not in p2p_conn_blocksonly.last_message:
                return False
            return p2p_conn_blocksonly.last_message['cmpctblock'].header_and_shortids.header.rehash() == block.sha256

        p2p_conn_blocksonly.send_message(msg_getdata([CInv(MSG_CMPCT_BLOCK, block0.sha256)]))
        p2p_conn_blocksonly.wait_until(lambda: test_for_cmpctblock(block0))

        # Request BIP152 high bandwidth mode from the -blocksonly node.
        p2p_conn_blocksonly.send_and_ping(msg_sendcmpct(announce=True, version=2))

        block2 = self.build_block_on_tip()
        self.nodes[0].submitblock(block1.serialize().hex())
        self.nodes[0].submitblock(block2.serialize().hex())
        p2p_conn_blocksonly.wait_until(lambda: test_for_cmpctblock(block2))

if __name__ == '__main__':
    P2PCompactBlocksBlocksOnly(__file__).main()
