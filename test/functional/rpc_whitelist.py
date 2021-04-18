#!/usr/bin/env python3
# Copyright (c) 2017-2019 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
A test for RPC users with restricted permissions
"""
from test_framework.test_framework import WidecoinTestFramework
import os
from test_framework.util import (
    get_datadir_path,
    assert_equal,
    str_to_b64str
)
import http.client
import urllib.parse

def rpccall(node, user, method):
    url = urllib.parse.urlparse(node.url)
    headers = {"Authorization": "Basic " + str_to_b64str('{}:{}'.format(user[0], user[3]))}
    conn = http.client.HTTPConnection(url.hostname, url.port)
    conn.connect()
    conn.request('POST', '/', '{"method": "' + method + '"}', headers)
    resp = conn.getresponse()
    conn.close()
    return resp


class RPCWhitelistTest(WidecoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def setup_chain(self):
        super().setup_chain()
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
        with open(os.path.join(get_datadir_path(self.options.tmpdir, 0), "widecoin.conf"), 'a', encoding='utf8') as f:
            f.write("\nrpcwhitelistdefault=0\n")
            for user in self.users:
                f.write("rpcauth=" + user[0] + ":" + user[1] + "\n")
                f.write("rpcwhitelist=" + user[0] + ":" + user[2] + "\n")
            # Special cases
            for strangedude in self.strange_users:
                f.write("rpcauth=" + strangedude[0] + ":" + strangedude[1] + "\n")
                f.write("rpcwhitelist=" + strangedude[0] + strangedude[2] + "\n")


    def run_test(self):
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

if __name__ == "__main__":
    RPCWhitelistTest().main()
