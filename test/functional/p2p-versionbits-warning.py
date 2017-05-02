#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test version bits warning system.

Generate chains with block versions that appear to be signalling unknown
soft-forks, and test that warning alerts are generated.
"""

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
import re
from test_framework.blocktools import create_block, create_coinbase

VB_PERIOD = 144 # versionbits period length for regtest
VB_THRESHOLD = 108 # versionbits activation threshold for regtest
VB_TOP_BITS = 0x20000000
VB_UNKNOWN_BIT = 27 # Choose a bit unassigned to any deployment

WARN_UNKNOWN_RULES_MINED = "Unknown block versions being mined! It's possible unknown rules are in effect"
WARN_UNKNOWN_RULES_ACTIVE = "unknown new rules activated (versionbit {})".format(VB_UNKNOWN_BIT)
VB_PATTERN = re.compile("^Warning.*versionbit")

class TestNode(NodeConnCB):
    def on_inv(self, conn, message):
        pass

class VersionBitsWarningTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self):
        self.alert_filename = os.path.join(self.options.tmpdir, "alert.txt")
        # Open and close to create zero-length file
        with open(self.alert_filename, 'w', encoding='utf8') as _:
            pass
        self.extra_args = [["-alertnotify=echo %s >> \"" + self.alert_filename + "\""]]
        self.setup_nodes()

    # Send numblocks blocks via peer with nVersionToUse set.
    def send_blocks_with_version(self, peer, numblocks, nVersionToUse):
        tip = self.nodes[0].getbestblockhash()
        height = self.nodes[0].getblockcount()
        block_time = self.nodes[0].getblockheader(tip)["time"]+1
        tip = int(tip, 16)

        for _ in range(numblocks):
            block = create_block(tip, create_coinbase(height+1), block_time)
            block.nVersion = nVersionToUse
            block.solve()
            peer.send_message(msg_block(block))
            block_time += 1
            height += 1
            tip = block.sha256
        peer.sync_with_ping()

    def test_versionbits_in_alert_file(self):
        with open(self.alert_filename, 'r', encoding='utf8') as f:
            alert_text = f.read()
        assert(VB_PATTERN.match(alert_text))

    def run_test(self):
        # Setup the p2p connection and start up the network thread.
        test_node = TestNode()

        connections = []
        connections.append(NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], test_node))
        test_node.add_connection(connections[0])

        NetworkThread().start() # Start up network handling in another thread

        # Test logic begins here
        test_node.wait_for_verack()

        # 1. Have the node mine one period worth of blocks
        self.nodes[0].generate(VB_PERIOD)

        # 2. Now build one period of blocks on the tip, with < VB_THRESHOLD
        # blocks signaling some unknown bit.
        nVersion = VB_TOP_BITS | (1<<VB_UNKNOWN_BIT)
        self.send_blocks_with_version(test_node, VB_THRESHOLD-1, nVersion)

        # Fill rest of period with regular version blocks
        self.nodes[0].generate(VB_PERIOD - VB_THRESHOLD + 1)
        # Check that we're not getting any versionbit-related errors in
        # get*info()
        assert(not VB_PATTERN.match(self.nodes[0].getinfo()["errors"]))
        assert(not VB_PATTERN.match(self.nodes[0].getmininginfo()["errors"]))
        assert(not VB_PATTERN.match(self.nodes[0].getnetworkinfo()["warnings"]))

        # 3. Now build one period of blocks with >= VB_THRESHOLD blocks signaling
        # some unknown bit
        self.send_blocks_with_version(test_node, VB_THRESHOLD, nVersion)
        self.nodes[0].generate(VB_PERIOD - VB_THRESHOLD)
        # Might not get a versionbits-related alert yet, as we should
        # have gotten a different alert due to more than 51/100 blocks
        # being of unexpected version.
        # Check that get*info() shows some kind of error.
        assert(WARN_UNKNOWN_RULES_MINED in self.nodes[0].getinfo()["errors"])
        assert(WARN_UNKNOWN_RULES_MINED in self.nodes[0].getmininginfo()["errors"])
        assert(WARN_UNKNOWN_RULES_MINED in self.nodes[0].getnetworkinfo()["warnings"])

        # Mine a period worth of expected blocks so the generic block-version warning
        # is cleared, and restart the node. This should move the versionbit state
        # to ACTIVE.
        self.nodes[0].generate(VB_PERIOD)
        stop_nodes(self.nodes)
        # Empty out the alert file
        with open(self.alert_filename, 'w', encoding='utf8') as _:
            pass
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, self.extra_args)

        # Connecting one block should be enough to generate an error.
        self.nodes[0].generate(1)
        assert(WARN_UNKNOWN_RULES_ACTIVE in self.nodes[0].getinfo()["errors"])
        assert(WARN_UNKNOWN_RULES_ACTIVE in self.nodes[0].getmininginfo()["errors"])
        assert(WARN_UNKNOWN_RULES_ACTIVE in self.nodes[0].getnetworkinfo()["warnings"])
        stop_nodes(self.nodes)
        self.test_versionbits_in_alert_file()

        # Test framework expects the node to still be running...
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, self.extra_args)

if __name__ == '__main__':
    VersionBitsWarningTest().main()
