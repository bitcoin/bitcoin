#!/usr/bin/env python3
# Copyright (c) 2017-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
A test for RPC users with restricted permissions
"""

import os
import http.client
import urllib.parse
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    get_datadir_path,
    str_to_b64str,
)


def rpccall(node, user, method, password=None):
    url = urllib.parse.urlparse(node.url)
    if password is not None:
        auth_value = str_to_b64str(f'{user}:{password}')
    else:
        auth_value = str_to_b64str(f'{user[0]}:{user[3]}')
    headers = {"Authorization": "Basic " + auth_value}
    conn = http.client.HTTPConnection(url.hostname, url.port)
    conn.connect()
    conn.request('POST', '/', f'{{"method": "{method}"}}', headers)
    resp = conn.getresponse()
    conn.close()
    return resp


class RPCWhitelistTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        # 0 => Username
        # 1 => Password (Hashed)
        # 2 => Permissions
        # 3 => Password Plaintext
        self.users = [
            ["user1", "50358aa884c841648e0700b073c32b2e$b73e95fff0748cc0b517859d2ca47d9bac1aa78231f3e48fa9222b612bd2083e", "getbestblockhash,getblockcount,", "12345"],
            ["user2", "8650ba41296f62092377a38547f361de$4620db7ba063ef4e2f7249853e9f3c5c3592a9619a759e3e6f1c63f2e22f1d21", "getblockcount", "54321"]
        ]
        # For exceptions
        self.strange_users = [
            # Test empty
            ["strangedude", "62d67dffec03836edd698314f1b2be62$c2fb4be29bb0e3646298661123cf2d8629640979cabc268ef05ea613ab54068d", ":", "s7R4nG3R7H1nGZ"],
            ["strangedude2", "575c012c7fe4b1e83b9d809412da3ef7$09f448d0acfc19924dd62ecb96004d3c2d4b91f471030dfe43c6ea64a8f658c1", "", "s7R4nG3R7H1nGZ"],
            # Test trailing comma
            ["strangedude3", "23189c561b5975a56f4cf94030495d61$3a2f6aac26351e2257428550a553c4c1979594e36675bbd3db692442387728c0", ":getblockcount,", "s7R4nG3R7H1nGZ"],
            # Test overwrite
            ["strangedude4", "990c895760a70df83949e8278665e19a$8f0906f20431ff24cb9e7f5b5041e4943bdf2a5c02a19ef4960dcf45e72cde1c", ":getblockcount, getbestblockhash", "s7R4nG3R7H1nGZ"],
            ["strangedude4", "990c895760a70df83949e8278665e19a$8f0906f20431ff24cb9e7f5b5041e4943bdf2a5c02a19ef4960dcf45e72cde1c", ":getblockcount", "s7R4nG3R7H1nGZ"],
            # Testing the same permission twice
            ["strangedude5", "d12c6e962d47a454f962eb41225e6ec8$2dd39635b155536d3c1a2e95d05feff87d5ba55f2d5ff975e6e997a836b717c9", ":getblockcount,getblockcount", "s7R4nG3R7H1nGZ"]
        ]
        # These commands shouldn't be allowed for any user to test failures
        self.never_allowed = ["getnetworkinfo"]
        with open(self.nodes[0].datadir_path / "bitcoin.conf", "a", encoding="utf8") as f:
            f.write("\nrpcwhitelistdefault=0\n")
            for user in self.users:
                f.write("rpcauth=" + user[0] + ":" + user[1] + "\n")
                f.write("rpcwhitelist=" + user[0] + ":" + user[2] + "\n")
            # Special cases
            for strangedude in self.strange_users:
                f.write("rpcauth=" + strangedude[0] + ":" + strangedude[1] + "\n")
                f.write("rpcwhitelist=" + strangedude[0] + strangedude[2] + "\n")
        self.restart_node(0)

        for user in self.users:
            permissions = user[2].replace(" ", "").split(",")
            # Pop all empty items
            i = 0
            while i < len(permissions):
                if permissions[i] == '':
                    permissions.pop(i)

                i += 1
            for permission in permissions:
                self.log.info("[" + user[0] + "]: Testing a permitted permission (" + permission + ")")
                assert_equal(200, rpccall(self.nodes[0], user, permission).status)
            for permission in self.never_allowed:
                self.log.info("[" + user[0] + "]: Testing a non permitted permission (" + permission + ")")
                assert_equal(403, rpccall(self.nodes[0], user, permission).status)
        # Now test the strange users
        for permission in self.never_allowed:
            self.log.info("Strange test 1")
            assert_equal(403, rpccall(self.nodes[0], self.strange_users[0], permission).status)
        for permission in self.never_allowed:
            self.log.info("Strange test 2")
            assert_equal(403, rpccall(self.nodes[0], self.strange_users[1], permission).status)
        self.log.info("Strange test 3")
        assert_equal(200, rpccall(self.nodes[0], self.strange_users[2], "getblockcount").status)
        self.log.info("Strange test 4")
        assert_equal(403, rpccall(self.nodes[0], self.strange_users[3], "getbestblockhash").status)
        self.log.info("Strange test 5")
        assert_equal(200, rpccall(self.nodes[0], self.strange_users[4], "getblockcount").status)

        #rpcwhitelistdefaulttest
        self.test_rpcwhitelist_default_0_without_whitelist()
        self.test_rpcwhitelist_default_0_specific_whitelist()
        self.test_whitelistdefault_1_with_specified_permission()
        self.test_whitelistdefault_1_without_specified_permission()

    def test_rpcwhitelist_default_0_without_whitelist(self):
        """
        Tests that with rpcwhitelistdefault=0 and no entries in rpcwhitelist,
        all authorized users (with rpcauth entries) are allowed to access all RPC methods
        """
        self.log.info("Testing rpcwhitelistdefault=0 with no specified permissions")
        with open(os.path.join(get_datadir_path(self.options.tmpdir, 0), "bitcoin.conf"), 'a', encoding='utf8') as f:
            f.write("rpcwhitelistdefault=0\n")
            f.write("rpcauth=user3:50358aa884c841648e0700b073c32b2e$b73e95fff0748cc0b517859d2ca47d9bac1aa78231f3e48fa9222b612bd2083e\n")
            f.write("rpcauth=user4:6e0499f40b6420da2d6a3baaec1a1268$4c03b56943444ad141e3a2d8389aa22b2ade5b09d6710d5299ade7b2902086f1\n")
        self.restart_node(0)

        assert_equal(200, rpccall(self.nodes[0], "user3", "getbestblockhash", "12345").status)
        assert_equal(200, rpccall(self.nodes[0], "user4", "getbestblockhash", "12345").status)


    def test_rpcwhitelist_default_0_specific_whitelist(self):
        """
        Tests that with rpcwhitelistdefault=0 and a specific whitelist for a user,
        only allowed RPC calls are permitted for that user, while other users retain full access.
        """
        self.log.info("Testing rpcwhitelistdefault=0 with whitelist for getblockcount")
        with open(os.path.join(get_datadir_path(self.options.tmpdir, 0), "bitcoin.conf"), 'a', encoding='utf8') as f:
            f.write("rpcwhitelistdefault=0\n")
            f.write("rpcauth=user3:50358aa884c841648e0700b073c32b2e$b73e95fff0748cc0b517859d2ca47d9bac1aa78231f3e48fa9222b612bd2083e\n")
            f.write("rpcauth=user4:6e0499f40b6420da2d6a3baaec1a1268$4c03b56943444ad141e3a2d8389aa22b2ade5b09d6710d5299ade7b2902086f1\n")
            f.write("rpcwhitelist=user4:getblockcount\n")
        self.restart_node(0)

        assert_equal(200, rpccall(self.nodes[0], "user3", "getnetworkinfo" ,"12345").status)
        assert_equal(403, rpccall(self.nodes[0], "user4", "getbestblockhash","12345").status)
        assert_equal(200, rpccall(self.nodes[0], "user4", "getblockcount", "12345").status)


    def test_whitelistdefault_1_with_specified_permission(self):
        """
        Tests that with rpcwhitelistdefault=1 (whitelist by default), a user with a specific whitelist entry
        can access the whitelisted methods, while users without a whitelist entry are denied access.
        """
        self.log.info("Testing rpcwhitelistdefault=1 with specified permissions")
        with open(os.path.join(get_datadir_path(self.options.tmpdir, 0), "bitcoin.conf"), 'a', encoding='utf8') as f:
            f.write("rpcwhitelistdefault=1\n")
            f.write("rpcauth=user3:50358aa884c841648e0700b073c32b2e$b73e95fff0748cc0b517859d2ca47d9bac1aa78231f3e48fa9222b612bd2083e\n")
            f.write("rpcauth=user4:6e0499f40b6420da2d6a3baaec1a1268$4c03b56943444ad141e3a2d8389aa22b2ade5b09d6710d5299ade7b2902086f1\n")
            f.write("rpcwhitelist=user3:getbestblockhash\n")
        self.restart_node(0)

        assert_equal(200, rpccall(self.nodes[0], "user3","getbestblockhash","12345").status)
        assert_equal(403, rpccall(self.nodes[0], "user4","getbestblockhash","12345").status)


    def test_whitelistdefault_1_without_specified_permission(self):
        """
        Tests that with rpcwhitelistdefault=1  and no entries in rpcwhitelist,
        all RPC methods are denied for users (even those with rpcauth entries).
        """
        self.log.info("Testing rpcwhitelistdefault=1 with no specified permissions")
        with open(os.path.join(get_datadir_path(self.options.tmpdir, 0), "bitcoin.conf"), 'a', encoding='utf8') as f:
            f.write("rpcwhitelistdefault=1\n")
            f.write("rpcauth=user3:50358aa884c841648e0700b073c32b2e$b73e95fff0748cc0b517859d2ca47d9bac1aa78231f3e48fa9222b612bd2083e\n")
            f.write("rpcauth=user4:6e0499f40b6420da2d6a3baaec1a1268$4c03b56943444ad141e3a2d8389aa22b2ade5b09d6710d5299ade7b2902086f1\n")
        self.restart_node(0)

        assert_equal(403, rpccall(self.nodes[0], "user3", "getwalletinfo", "12345").status)
        assert_equal(403, rpccall(self.nodes[0], "user4", "getdifficulty", "12345").status)



if __name__ == "__main__":
    RPCWhitelistTest().main()
