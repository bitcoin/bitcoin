#!/usr/bin/env python3
# Copyright (c) 2022 The Tortoisecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test encrypted v2 p2p proposed in BIP 324
"""
from test_framework.blocktools import (
    create_block,
    create_coinbase,
)
from test_framework.p2p import (
    P2PDataStore,
    P2PInterface,
)
from test_framework.test_framework import TortoisecoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    check_node_connections,
)
from test_framework.crypto.chacha20 import REKEY_INTERVAL


class P2PEncrypted(TortoisecoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [["-v2transport=1"], ["-v2transport=1"]]

    def setup_network(self):
        self.setup_nodes()

    def generate_blocks(self, node, number):
        test_blocks = []
        last_block = node.getbestblockhash()
        tip = int(last_block, 16)
        tipheight = node.getblockcount()
        last_block_time = node.getblock(last_block)['time']
        for _ in range(number):
            # Create some blocks
            block = create_block(tip, create_coinbase(tipheight + 1), last_block_time + 1)
            block.solve()
            test_blocks.append(block)
            tip = block.sha256
            tipheight += 1
            last_block_time += 1
        return test_blocks

    def create_test_block(self, txs):
        block = create_block(self.tip, create_coinbase(self.tipheight + 1), self.last_block_time + 600, txlist=txs)
        block.solve()
        return block

    def run_test(self):
        node0, node1 = self.nodes[0], self.nodes[1]
        self.log.info("Check inbound connection to v2 TestNode from v2 P2PConnection is v2")
        peer1 = node0.add_p2p_connection(P2PInterface(), wait_for_verack=True, supports_v2_p2p=True)
        assert peer1.supports_v2_p2p
        assert_equal(node0.getpeerinfo()[-1]["transport_protocol_type"], "v2")

        self.log.info("Check inbound connection to v2 TestNode from v1 P2PConnection is v1")
        peer2 = node0.add_p2p_connection(P2PInterface(), wait_for_verack=True, supports_v2_p2p=False)
        assert not peer2.supports_v2_p2p
        assert_equal(node0.getpeerinfo()[-1]["transport_protocol_type"], "v1")

        self.log.info("Check outbound connection from v2 TestNode to v1 P2PConnection advertised as v1 is v1")
        peer3 = node0.add_outbound_p2p_connection(P2PInterface(), p2p_idx=0, supports_v2_p2p=False, advertise_v2_p2p=False)
        assert not peer3.supports_v2_p2p
        assert_equal(node0.getpeerinfo()[-1]["transport_protocol_type"], "v1")

        # v2 TestNode performs downgrading here
        self.log.info("Check outbound connection from v2 TestNode to v1 P2PConnection advertised as v2 is v1")
        peer4 = node0.add_outbound_p2p_connection(P2PInterface(), p2p_idx=1, supports_v2_p2p=False, advertise_v2_p2p=True)
        assert not peer4.supports_v2_p2p
        assert_equal(node0.getpeerinfo()[-1]["transport_protocol_type"], "v1")

        self.log.info("Check outbound connection from v2 TestNode to v2 P2PConnection advertised as v2 is v2")
        peer5 = node0.add_outbound_p2p_connection(P2PInterface(), p2p_idx=2, supports_v2_p2p=True, advertise_v2_p2p=True)
        assert peer5.supports_v2_p2p
        assert_equal(node0.getpeerinfo()[-1]["transport_protocol_type"], "v2")

        self.log.info("Check if version is sent and verack is received in inbound/outbound connections")
        assert_equal(len(node0.getpeerinfo()), 5)  # check if above 5 connections are present in node0's getpeerinfo()
        for peer in node0.getpeerinfo():
            assert_greater_than(peer['bytessent_per_msg']['version'], 0)
            assert_greater_than(peer['bytesrecv_per_msg']['verack'], 0)

        self.log.info("Testing whether blocks propagate - check if tips sync when number of blocks >= REKEY_INTERVAL")
        # tests whether rekeying (which happens every REKEY_INTERVAL packets) works correctly
        test_blocks = self.generate_blocks(node0, REKEY_INTERVAL+1)

        for i in range(2):
            peer6 = node0.add_p2p_connection(P2PDataStore(), supports_v2_p2p=True)
            assert peer6.supports_v2_p2p
            assert_equal(node0.getpeerinfo()[-1]["transport_protocol_type"], "v2")

            # Consider: node0 <-- peer6. node0 and node1 aren't connected here.
            # Construct the following topology: node1 <--> node0 <-- peer6
            # and test that blocks produced by peer6 will be received by node1 if sent normally
            # and won't be received by node1 if sent as decoy messages

            # First, check whether blocks produced be peer6 are received by node0 if sent normally
            # and not received by node0 if sent as decoy messages.
            if i:
                # check that node0 receives blocks produced by peer6
                self.log.info("Check if blocks produced by node0's p2p connection is received by node0")
                peer6.send_blocks_and_test(test_blocks, node0, success=True)  # node0's tip advances
            else:
                # check that node0 doesn't receive blocks produced by peer6 since they are sent as decoy messages
                self.log.info("Check if blocks produced by node0's p2p connection sent as decoys aren't received by node0")
                peer6.send_blocks_and_test(test_blocks, node0, success=False, is_decoy=True)  # node0's tip doesn't advance

            # Then, connect node0 and node1 using v2 and check whether the blocks are received by node1
            self.connect_nodes(0, 1, peer_advertises_v2=True)
            self.log.info("Wait for node1 to receive all the blocks from node0")
            self.sync_all()
            self.log.info("Make sure node0 and node1 have same block tips")
            assert_equal(node0.getbestblockhash(), node1.getbestblockhash())

            self.disconnect_nodes(0, 1)

        self.log.info("Check the connections opened as expected")
        check_node_connections(node=node0, num_in=4, num_out=3)

        self.log.info("Check inbound connection to v1 TestNode from v2 P2PConnection is v1")
        self.restart_node(0, ["-v2transport=0"])
        peer1 = node0.add_p2p_connection(P2PInterface(), wait_for_verack=True, supports_v2_p2p=True)
        assert not peer1.supports_v2_p2p
        assert_equal(node0.getpeerinfo()[-1]["transport_protocol_type"], "v1")
        check_node_connections(node=node0, num_in=1, num_out=0)


if __name__ == '__main__':
    P2PEncrypted(__file__).main()
