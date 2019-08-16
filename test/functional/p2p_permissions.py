#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test p2p permission message.

Test that permissions are correctly calculated and applied
"""

from test_framework.test_node import ErrorMatch
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    connect_nodes,
    p2p_port,
)

class P2PPermissionsTests(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [[],[]]

    def run_test(self):
        self.checkpermission(
        # relay permission added
        ["-whitelist=127.0.0.1", "-whitelistrelay"],
        ["relay", "noban", "mempool"],
        True)

        self.checkpermission(
        # forcerelay and relay permission added
        # Legacy parameter interaction which set whitelistrelay to true
        # if whitelistforcerelay is true
        ["-whitelist=127.0.0.1", "-whitelistforcerelay"],
        ["forcerelay", "relay", "noban", "mempool"],
        True)

        # Let's make sure permissions are merged correctly
        # For this, we need to use whitebind instead of bind
        # by modifying the configuration file.
        ip_port = "127.0.0.1:{}".format(p2p_port(1))
        self.replaceinconfig(1, "bind=127.0.0.1", "whitebind=bloomfilter,forcerelay@" + ip_port)
        self.checkpermission(
        ["-whitelist=noban@127.0.0.1" ],
        # Check parameter interaction forcerelay should activate relay
        ["noban", "bloomfilter", "forcerelay", "relay" ],
        False)
        self.replaceinconfig(1, "whitebind=bloomfilter,forcerelay@" + ip_port, "bind=127.0.0.1")

        self.checkpermission(
        # legacy whitelistrelay should be ignored
        ["-whitelist=noban,mempool@127.0.0.1", "-whitelistrelay"],
        ["noban", "mempool"],
        False)

        self.checkpermission(
        # legacy whitelistforcerelay should be ignored
        ["-whitelist=noban,mempool@127.0.0.1", "-whitelistforcerelay"],
        ["noban", "mempool"],
        False)

        self.checkpermission(
        # missing mempool permission to be considered legacy whitelisted
        ["-whitelist=noban@127.0.0.1"],
        ["noban"],
        False)

        self.checkpermission(
        # all permission added
        ["-whitelist=all@127.0.0.1"],
        ["forcerelay", "noban", "mempool", "bloomfilter", "relay"],
        False)

        self.stop_node(1)
        self.nodes[1].assert_start_raises_init_error(["-whitelist=oopsie@127.0.0.1"], "Invalid P2P permission", match=ErrorMatch.PARTIAL_REGEX)
        self.nodes[1].assert_start_raises_init_error(["-whitelist=noban@127.0.0.1:230"], "Invalid netmask specified in", match=ErrorMatch.PARTIAL_REGEX)
        self.nodes[1].assert_start_raises_init_error(["-whitebind=noban@127.0.0.1/10"], "Cannot resolve -whitebind address", match=ErrorMatch.PARTIAL_REGEX)

    def checkpermission(self, args, expectedPermissions, whitelisted):
        self.restart_node(1, args)
        connect_nodes(self.nodes[0], 1)
        peerinfo = self.nodes[1].getpeerinfo()[0]
        assert_equal(peerinfo['whitelisted'], whitelisted)
        assert_equal(len(expectedPermissions), len(peerinfo['permissions']))
        for p in expectedPermissions:
            if not p in peerinfo['permissions']:
                raise AssertionError("Expected permissions %r is not granted." % p)

    def replaceinconfig(self, nodeid, old, new):
        with open(self.nodes[nodeid].bitcoinconf, encoding="utf8") as f:
            newText=f.read().replace(old, new)
        with open(self.nodes[nodeid].bitcoinconf, 'w', encoding="utf8") as f:
            f.write(newText)

if __name__ == '__main__':
    P2PPermissionsTests().main()
