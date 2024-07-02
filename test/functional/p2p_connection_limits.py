#!/usr/bin/env python3
# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import (
    msg_version,
    msg_filterload
)
from test_framework.p2p import (
    P2PInterface,
    P2P_SERVICES,
    P2P_SUBVERSION,
    P2P_VERSION,
)


class P2PConnectionLimits(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # scenario : we have 2 inbound slots and allow a maximum of 1 tx-relaying inbound peer
        self.extra_args = [['-maxconnections=13']]  # 11 slots are reserved for outbounds, leaving 2 inbound slots

    def run_test(self):
        self.test_inbound_limits()

    def create_blocks_only_version(self):
        no_txrelay_version_msg = msg_version()
        no_txrelay_version_msg.nVersion = P2P_VERSION
        no_txrelay_version_msg.strSubVer = P2P_SUBVERSION
        no_txrelay_version_msg.nServices = P2P_SERVICES
        no_txrelay_version_msg.relay = 0
        return no_txrelay_version_msg

    def test_inbound_limits(self):
        node = self.nodes[0]

        self.log.info('Test with 2 inbound slots, one of which allows tx-relay')
        node.add_p2p_connection(P2PInterface())

        self.log.info('Connect a full-relay inbound peer - test that eviction is triggered')
        # Since there is no unprotected peer to evict here, the new peer is dropped instead.
        with node.assert_debug_log(['failed to find a tx-relaying eviction candidate - connection dropped']):
            self.nodes[0].add_p2p_connection(P2PInterface(), expect_success=False, wait_for_verack=False)
        self.wait_until(lambda: len(node.getpeerinfo()) == 1)
        node.disconnect_p2ps()

        self.log.info('Connect a block-relay inbound peer - test that second full relay peer is accepted')
        peer1 = self.nodes[0].add_p2p_connection(P2PInterface(), send_version=False, wait_for_verack=False)
        peer1.send_message(self.create_blocks_only_version())
        peer1.wait_for_verack()

        node.add_p2p_connection(P2PInterface())
        self.wait_until(lambda: len(node.getpeerinfo()) == 2)

        self.log.info('Connecting another full-relay peer triggers non-specific eviction')
        with node.assert_debug_log(['failed to find an eviction candidate - connection dropped (full)']):
            self.nodes[0].add_p2p_connection(P2PInterface(), send_version=False, wait_for_verack=False, expect_success=False)
        self.wait_until(lambda: len(node.getpeerinfo()) == 2)

        self.log.info('Run with bloom filter support and check that a switch to tx relay during runtime can trigger eviction')
        self.restart_node(0, ['-maxconnections=13', '-peerbloomfilters'])
        peer1 = self.nodes[0].add_p2p_connection(P2PInterface(), send_version=False, wait_for_verack=False)
        peer1.send_message(self.create_blocks_only_version())
        peer1.wait_for_verack()

        node.add_p2p_connection(P2PInterface())
        self.wait_until(lambda: len(node.getpeerinfo()) == 2)
        with node.assert_debug_log(['filterload received, but no capacity for tx-relay and no other peer to evict. disconnecting peer']):
            peer1.send_message(msg_filterload(data=b'\xbb'*(100)))
        self.wait_until(lambda: len(node.getpeerinfo()) == 1)

        self.log.info('Test different values of inboundrelaypercent')
        self.restart_node(0, ['-maxconnections=13', '-inboundrelaypercent=0'])
        with node.assert_debug_log(['failed to find a tx-relaying eviction candidate - connection dropped']):
            self.nodes[0].add_p2p_connection(P2PInterface(), expect_success=False, wait_for_verack=False)

        self.restart_node(0, ['-maxconnections=13', '-inboundrelaypercent=100'])
        node.add_p2p_connection(P2PInterface())
        node.add_p2p_connection(P2PInterface())
        self.wait_until(lambda: len(node.getpeerinfo()) == 2)


if __name__ == '__main__':
    P2PConnectionLimits().main()
