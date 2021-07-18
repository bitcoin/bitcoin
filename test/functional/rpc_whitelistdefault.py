#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
A test for rpcwhitelistdefault states.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    get_datadir_path,
    str_to_b64str
)

import os
import http.client
import urllib.parse

def rpccall(node, user, password, method):
    url = urllib.parse.urlparse(node.url)
    headers = {"Authorization": "Basic " + str_to_b64str('{}:{}'.format(user, password))}
    conn = http.client.HTTPConnection(url.hostname, url.port)
    conn.connect()
    conn.request('POST', '/', '{{"method": "{}"}}'.format(method), headers)
    resp = conn.getresponse()
    conn.close()
    return resp.code

class RPCWhitelistDefaultTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4

    def setup_chain(self):
        super().setup_chain()
        """
        node0 specs:
        * rpcwhitelistdefault=0
        * No Permissions defined
        Expected result: Any command for any user should work
        """
        with open(os.path.join(get_datadir_path(self.options.tmpdir, 0), "bitcoin.conf"), 'a', encoding='utf8') as f:
            f.write("rpcwhitelistdefault=0\n")
            f.write("rpcauth=user1:50358aa884c841648e0700b073c32b2e$b73e95fff0748cc0b517859d2ca47d9bac1aa78231f3e48fa9222b612bd2083e\n")
            f.write("rpcauth=user2:6e0499f40b6420da2d6a3baaec1a1268$4c03b56943444ad141e3a2d8389aa22b2ade5b09d6710d5299ade7b2902086f1\n")
        """
        node1 specs:
        * rpcwhitelistdefault=0
        * Permissions (user1): getbestblockhash
        Expected result: Only getbestblockhash for user1 should work, user2 should execute anything
        """
        with open(os.path.join(get_datadir_path(self.options.tmpdir, 1), "bitcoin.conf"), 'a', encoding='utf8') as f:
            f.write("rpcwhitelistdefault=0\n")
            f.write("rpcauth=user1:50358aa884c841648e0700b073c32b2e$b73e95fff0748cc0b517859d2ca47d9bac1aa78231f3e48fa9222b612bd2083e\n")
            f.write("rpcauth=user2:6e0499f40b6420da2d6a3baaec1a1268$4c03b56943444ad141e3a2d8389aa22b2ade5b09d6710d5299ade7b2902086f1\n")
            f.write("rpcwhitelist=user1:getbestblockhash\n")
        """
        node2 specs:
        * rpcwhitelistdefault=1
        * No Permissions defined
        Expected result: No user should be able to execute anything
        """
        with open(os.path.join(get_datadir_path(self.options.tmpdir, 2), "bitcoin.conf"), 'a', encoding='utf8') as f:
            f.write("rpcwhitelistdefault=1\n")
            f.write("rpcauth=user1:50358aa884c841648e0700b073c32b2e$b73e95fff0748cc0b517859d2ca47d9bac1aa78231f3e48fa9222b612bd2083e\n")
            f.write("rpcauth=user2:6e0499f40b6420da2d6a3baaec1a1268$4c03b56943444ad141e3a2d8389aa22b2ade5b09d6710d5299ade7b2902086f1\n")
            # We need to permit the __cookie__ user for the Bitcoin Test Framework
            f.write("rpcwhitelist=__cookie__:getblockcount,getwalletinfo,importprivkey,getblockchaininfo,submitblock,addnode,getpeerinfo,getbestblockhash,getrawmempool,syncwithvalidationinterfacequeue,stop\n")
        """
        node3 specs:
        * rpcwhitelistdefault=1
        * Permissions (user1): getbestblockhash
        Expected result: user1 should do getbestblockhash and user2 nothing
        """
        with open(os.path.join(get_datadir_path(self.options.tmpdir, 3), "bitcoin.conf"), 'a', encoding='utf8') as f:
            f.write("rpcwhitelistdefault=1\n")
            f.write("rpcauth=user1:50358aa884c841648e0700b073c32b2e$b73e95fff0748cc0b517859d2ca47d9bac1aa78231f3e48fa9222b612bd2083e\n")
            f.write("rpcauth=user2:6e0499f40b6420da2d6a3baaec1a1268$4c03b56943444ad141e3a2d8389aa22b2ade5b09d6710d5299ade7b2902086f1\n")
            f.write("rpcwhitelist=user1:getbestblockhash\n")
            # We need to permit the __cookie__ user for the Bitcoin Test Framework
            f.write("rpcwhitelist=__cookie__:getblockcount,getwalletinfo,importprivkey,getblockchaininfo,submitblock,addnode,getpeerinfo,getbestblockhash,getrawmempool,syncwithvalidationinterfacequeue,stop\n")

    def run_test(self):
        self.log.info("Testing rpcwhitelistdefault=0 with no specified permissions")
        assert_equal(200, rpccall(self.nodes[0], "user1", "12345", "getbestblockhash"))
        assert_equal(200, rpccall(self.nodes[0], "user2", "12345", "getbestblockhash"))
        self.log.info("Testing rpcwhitelistdefault=0 with specified permissions")
        assert_equal(200, rpccall(self.nodes[1], "user1", "12345", "getbestblockhash"))
        assert_equal(403, rpccall(self.nodes[1], "user1", "12345", "getblockcount"))
        assert_equal(200, rpccall(self.nodes[1], "user2", "12345", "getblockcount"))
        self.log.info("Testing rpcwhitelistdefault=1 with no specified permissions")
        assert_equal(403, rpccall(self.nodes[2], "user1", "12345", "getblockcount"))
        assert_equal(403, rpccall(self.nodes[2], "user2", "12345", "getblockcount"))
        self.log.info("Testing rpcwhitelistdefault=1 with specified permissions")
        assert_equal(200, rpccall(self.nodes[3], "user1", "12345", "getbestblockhash"))
        assert_equal(403, rpccall(self.nodes[3], "user2", "12345", "getbestblockhash"))

if __name__ == "__main__":
    RPCWhitelistDefaultTest().main()

