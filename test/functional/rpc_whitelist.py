#!/usr/bin/env python3
# Copyright (c) 2017-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
A test for RPC users with restricted permissions
"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    str_to_b64str,
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


def get_permissions(whitelist):
    return [perm for perm in whitelist.split(",") if perm]


class RPCWhitelistTest(BitcoinTestFramework):

    def set_test_params(self):
        self.num_nodes = 1
        self.supports_cli = False

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
            ["strangedude5", "d12c6e962d47a454f962eb41225e6ec8$2dd39635b155536d3c1a2e95d05feff87d5ba55f2d5ff975e6e997a836b717c9", ":getblockcount,getblockcount", "s7R4nG3R7H1nGZ"],
            # Test non-whitelisted user
            ["strangedude6", "67e5583538958883291f6917883eca64$8a866953ef9c5b7d078a62c64754a4eb74f47c2c17821eb4237021d7ef44f991", None, "N4SziYbHmhC1"]
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
                if strangedude[2] is not None:
                    f.write("rpcwhitelist=" + strangedude[0] + strangedude[2] + "\n")
        self.restart_node(0)

        for user in self.users:
            for permission in self.never_allowed:
                self.log.info(f"[{user[0]}]: Testing a non permitted permission ({permission})")
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

        self.test_users_permissions()
        self.test_rpcwhitelistdefault_permissions(0, 200)

        # Replace file configurations
        self.nodes[0].replace_in_config([("rpcwhitelistdefault=0", "rpcwhitelistdefault=1")])
        with open(self.nodes[0].datadir_path / "bitcoin.conf", 'a', encoding='utf8') as f:
            f.write("rpcwhitelist=__cookie__:getblockcount,getblockchaininfo,getmempoolinfo,stop\n")
        self.restart_node(0)

        self.test_users_permissions()
        self.test_rpcwhitelistdefault_permissions(1, 403)

        # Ensure that not specifying -rpcwhitelistdefault is the same as
        # specifying -rpcwhitelistdefault=1. Only explicitly whitelisted users
        # should be allowed.
        self.nodes[0].replace_in_config([("rpcwhitelistdefault=1", "")])
        self.restart_node(0)
        self.test_users_permissions()
        self.test_rpcwhitelistdefault_permissions(1, 403)

    def test_users_permissions(self):
        """
        * Permissions:
            (user1): getbestblockhash,getblockcount
            (user2): getblockcount
        Expected result:  * users can only access whitelisted methods
        """
        for user in self.users:
            permissions = get_permissions(user[2])
            for permission in permissions:
                self.log.info(f"[{user[0]}]: Testing whitelisted user permission ({permission})")
                assert_equal(200, rpccall(self.nodes[0], user, permission).status)
            self.log.info(f"[{user[0]}]: Testing non-permitted permission: getblockchaininfo")
            assert_equal(403, rpccall(self.nodes[0], user, "getblockchaininfo").status)

    def test_rpcwhitelistdefault_permissions(self, default_value, expected_status):
        """
        * rpcwhitelistdefault={default_value}
        * No Permissions defined
        Expected result: strangedude6 (not whitelisted) access is determined by default_value
        When default_value=0: expects 200
        When default_value=1: expects 403
        """
        user = self.strange_users[6]  # strangedude6
        for permission in ["getbestblockhash", "getblockchaininfo"]:
            self.log.info(f"[{user[0]}]: Testing rpcwhitelistdefault={default_value} no specified permission ({permission})")
            assert_equal(expected_status, rpccall(self.nodes[0], user, permission).status)


if __name__ == "__main__":
    RPCWhitelistTest(__file__).main()
