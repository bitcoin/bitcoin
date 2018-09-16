#!/usr/bin/env python3
# Copyright (c) 2016-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test version bits warning system.

Generate chains with block versions that appear to be signalling unknown
soft-forks, and test that warning alerts are generated.
"""
import os
import re

from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import msg_block
from test_framework.mininode import P2PInterface, mininode_lock
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import wait_until

VB_PERIOD = 144           # versionbits period length for regtest
VB_THRESHOLD = 108        # versionbits activation threshold for regtest
VB_TOP_BITS = 0x20000000
VB_UNKNOWN_BIT = 27       # Choose a bit unassigned to any deployment
VB_UNKNOWN_VERSION = VB_TOP_BITS | (1 << VB_UNKNOWN_BIT)

WARN_UNKNOWN_RULES_MINED = "Unknown block versions being mined! It's possible unknown rules are in effect"
WARN_UNKNOWN_RULES_ACTIVE = "unknown new rules activated (versionbit {})".format(VB_UNKNOWN_BIT)
VB_PATTERN = re.compile("Warning: unknown new rules activated.*versionbit")

class VersionBitsWarningTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self):
        self.alert_filename = os.path.join(self.options.tmpdir, "alert.txt")
        # Open and close to create zero-length file
        with open(self.alert_filename, 'w', encoding='utf8'):
            pass
        self.extra_args = [["-alertnotify=echo %s >> \"" + self.alert_filename + "\""]]
        self.setup_nodes()

    def send_blocks_with_version(self, peer, numblocks, version):
        """Send numblocks blocks to peer with version set"""
        tip = self.nodes[0].getbestblockhash()
        height = self.nodes[0].getblockcount()
        block_time = self.nodes[0].getblockheader(tip)["time"] + 1
        tip = int(tip, 16)

        for _ in range(numblocks):
            block = create_block(tip, create_coinbase(height + 1), block_time)
            block.nVersion = version
            block.solve()
            peer.send_message(msg_block(block))
            block_time += 1
            height += 1
            tip = block.sha256
        peer.sync_with_ping()

    def versionbits_in_alert_file(self):
        """Test that the versionbits warning has been written to the alert file."""
        alert_text = open(self.alert_filename, 'r', encoding='utf8').read()
        return VB_PATTERN.search(alert_text) is not None

    def run_test(self):
        node = self.nodes[0]
        node.add_p2p_connection(P2PInterface())

        node_deterministic_address = node.get_deterministic_priv_key().address
        # Mine one period worth of blocks
        node.generatetoaddress(VB_PERIOD, node_deterministic_address)

        self.log.info("Check that there is no warning if previous VB_BLOCKS have <VB_THRESHOLD blocks with unknown versionbits version.")
        # Build one period of blocks with < VB_THRESHOLD blocks signaling some unknown bit
        self.send_blocks_with_version(node.p2p, VB_THRESHOLD - 1, VB_UNKNOWN_VERSION)
        node.generatetoaddress(VB_PERIOD - VB_THRESHOLD + 1, node_deterministic_address)

        # Check that we're not getting any versionbit-related errors in get*info()
        assert(not VB_PATTERN.match(node.getmininginfo()["warnings"]))
        assert(not VB_PATTERN.match(node.getnetworkinfo()["warnings"]))

        self.log.info("Check that there is a warning if >50 blocks in the last 100 were an unknown version")
        # Build one period of blocks with VB_THRESHOLD blocks signaling some unknown bit
        self.send_blocks_with_version(node.p2p, VB_THRESHOLD, VB_UNKNOWN_VERSION)
        node.generatetoaddress(VB_PERIOD - VB_THRESHOLD, node_deterministic_address)

        # Check that get*info() shows the 51/100 unknown block version error.
        assert(WARN_UNKNOWN_RULES_MINED in node.getmininginfo()["warnings"])
        assert(WARN_UNKNOWN_RULES_MINED in node.getnetworkinfo()["warnings"])

        self.log.info("Check that there is a warning if previous VB_BLOCKS have >=VB_THRESHOLD blocks with unknown versionbits version.")
        # Mine a period worth of expected blocks so the generic block-version warning
        # is cleared. This will move the versionbit state to ACTIVE.
        node.generatetoaddress(VB_PERIOD, node_deterministic_address)

        # Stop-start the node. This is required because bitcoind will only warn once about unknown versions or unknown rules activating.
        self.restart_node(0)

        # Generating one block guarantees that we'll get out of IBD
        node.generatetoaddress(1, node_deterministic_address)
        wait_until(lambda: not node.getblockchaininfo()['initialblockdownload'], timeout=10, lock=mininode_lock)
        # Generating one more block will be enough to generate an error.
        node.generatetoaddress(1, node_deterministic_address)
        # Check that get*info() shows the versionbits unknown rules warning
        assert(WARN_UNKNOWN_RULES_ACTIVE in node.getmininginfo()["warnings"])
        assert(WARN_UNKNOWN_RULES_ACTIVE in node.getnetworkinfo()["warnings"])
        # Check that the alert file shows the versionbits unknown rules warning
        wait_until(lambda: self.versionbits_in_alert_file(), timeout=60)

if __name__ == '__main__':
    VersionBitsWarningTest().main()
