#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test BIP324 v2 transport
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class P2PV2Test(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 5
        self.extra_args = [["-v2transport=1"], ["-v2transport=1"], ["-v2transport=0"], ["-v2transport=0"], ["-v2transport=0"]]

    def run_test(self):
        sending_ellswift = "sending 64 byte v2 p2p ellswift key to peer"
        downgrading_to_v1 = "downgrading to v1 transport protocol for peer"
        self.disconnect_nodes(0, 1)
        self.disconnect_nodes(1, 2)
        self.disconnect_nodes(2, 3)
        self.disconnect_nodes(3, 4)

        # V2 nodes can sync with V2 nodes
        assert_equal(self.nodes[0].getblockcount(), 0)
        assert_equal(self.nodes[1].getblockcount(), 0)
        self.nodes[0].generatetoaddress(5, "bcrt1q0yq2azut8gn2xu3y2g0xucf8pny6w8uxmyf220", invalid_call=False)
        assert_equal(self.nodes[0].getblockcount(), 5)
        assert_equal(self.nodes[1].getblockcount(), 0)
        with self.nodes[0].assert_debug_log(expected_msgs=[sending_ellswift],
                                            unexpected_msgs=[downgrading_to_v1]):
            self.connect_nodes(0, 1, True)
        # sync_all() verifies that the block tips match
        self.sync_all(self.nodes[0:2])
        assert_equal(self.nodes[1].getblockcount(), 5)
        peerinfo_0 = self.nodes[0].getpeerinfo()
        peerinfo_1 = self.nodes[1].getpeerinfo()
        assert_equal(peerinfo_0[0]["transport_protocol_type"], "v2")
        assert_equal(peerinfo_1[0]["transport_protocol_type"], "v2")
        assert_equal(len(peerinfo_0[0]["v2_session_id"]), 64)
        assert_equal(peerinfo_0[0]["v2_session_id"], peerinfo_1[0]["v2_session_id"])

        # V1 nodes can sync with each other
        assert_equal(self.nodes[2].getblockcount(), 0)
        assert_equal(self.nodes[3].getblockcount(), 0)
        self.nodes[2].generatetoaddress(8, "bcrt1qyr5lnc2g8aa3qa9c4th9d46n5uu4y0m9nvq2cv", invalid_call=False)
        assert_equal(self.nodes[2].getblockcount(), 8)
        assert_equal(self.nodes[3].getblockcount(), 0)
        with self.nodes[2].assert_debug_log(expected_msgs=[],
                                            unexpected_msgs=[sending_ellswift, downgrading_to_v1]):
            self.connect_nodes(2, 3, False)
        self.sync_all(self.nodes[2:4])
        assert_equal(self.nodes[3].getblockcount(), 8)
        assert self.nodes[0].getbestblockhash() != self.nodes[2].getbestblockhash()
        peerinfo_2 = self.nodes[2].getpeerinfo()
        assert_equal(peerinfo_2[0]["transport_protocol_type"], "v1")
        assert "v2_session_id" not in peerinfo_2[0]

        # V1 nodes can sync with V2 nodes
        self.disconnect_nodes(0, 1)
        self.disconnect_nodes(2, 3)
        with self.nodes[2].assert_debug_log(expected_msgs=[],
                                            unexpected_msgs=[sending_ellswift, downgrading_to_v1]):
            self.connect_nodes(2, 1, True)
        self.sync_all(self.nodes[1:3])
        assert_equal(self.nodes[1].getblockcount(), 8)
        assert self.nodes[0].getbestblockhash() != self.nodes[1].getbestblockhash()
        assert_equal(self.nodes[2].getpeerinfo()[0]["transport_protocol_type"], "v1")

        # V2 nodes can sync with V1 nodes
        self.disconnect_nodes(1, 2)
        with self.nodes[0].assert_debug_log(expected_msgs=[],
                                            unexpected_msgs=[sending_ellswift, downgrading_to_v1]):
            self.connect_nodes(0, 3, False)
        self.sync_all([self.nodes[0], self.nodes[3]])
        assert_equal(self.nodes[0].getblockcount(), 8)
        assert_equal(self.nodes[0].getpeerinfo()[0]["transport_protocol_type"], "v1")

        # V2 node mines another block and everyone gets it
        self.connect_nodes(0, 1, True)
        self.connect_nodes(1, 2, False)
        self.nodes[1].generatetoaddress(1, "bcrt1q3zsxn3qx0cqyyxgv90k7j6786mpe543wc4vy2v", invalid_call=False)
        self.sync_all(self.nodes[0:4])
        assert_equal(self.nodes[0].getblockcount(), 9) # sync_all() verifies tip hashes match

        # V1 node mines another block and everyone gets it
        self.nodes[3].generatetoaddress(2, "bcrt1q3zsxn3qx0cqyyxgv90k7j6786mpe543wc4vy2v", invalid_call=False)
        self.sync_all(self.nodes[0:4])
        assert_equal(self.nodes[2].getblockcount(), 11) # sync_all() verifies tip hashes match

        assert_equal(self.nodes[4].getblockcount(), 0)
        # Peer 4 is v1 p2p, but is falsely advertised as v2.
        with self.nodes[1].assert_debug_log(expected_msgs=[sending_ellswift, downgrading_to_v1]):
            self.connect_nodes(1, 4, True)
        self.sync_all()
        assert_equal(self.nodes[4].getblockcount(), 11)
        assert_equal(self.nodes[4].getpeerinfo()[0]["transport_protocol_type"], "v1")


if __name__ == '__main__':
    P2PV2Test().main()
