#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that commands submitted by the platform user are filtered."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import str_to_b64str, assert_equal

import http.client
import json
import os
import urllib.parse


class HTTPBasicsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def setup_chain(self):
        super().setup_chain()
        # Append rpcauth to dash.conf before initialization
        rpcauthplatform = "rpcauth=platform-user:dd88fd676186f48553775d6fb5a2d344$bc1f7898698ead19c6ec7ff47055622dd7101478f1ff6444103d3dc03cd77c13"
        # rpcuser : platform-user
        # rpcpassword : password123
        rpcauthoperator = "rpcauth=operator:e9b45dd0b61a7be72155535435365a3a$8fb7470bc6f74d8ceaf9a23f49b06127723bd563b3ed5d9cea776ef01803d191"
        # rpcuser : operator
        # rpcpassword : otherpassword

        masternodeblskey="masternodeblsprivkey=58af6e39bb4d86b22bda1a02b134c2f5b71caffa1377540b02f7f1ad122f59e0"

        with open(os.path.join(self.options.tmpdir+"/node0", "dash.conf"), 'a', encoding='utf8') as f:
            f.write(masternodeblskey+"\n")
            f.write(rpcauthplatform+"\n")
            f.write(rpcauthoperator+"\n")

    def run_test(self):

        url = urllib.parse.urlparse(self.nodes[0].url)

        def test_command(method, params, auth, expexted_status, should_not_match=False):
            conn = http.client.HTTPConnection(url.hostname, url.port)
            conn.connect()
            body = {"method": method}
            if len(params):
                body["params"] = params
            conn.request('POST', '/', json.dumps(body), {"Authorization": "Basic " + str_to_b64str(auth)})
            resp = conn.getresponse()
            if should_not_match:
                assert(resp.status != expexted_status)
            else:
                assert_equal(resp.status, expexted_status)
            conn.close()

        whitelisted = ["getbestblockhash",
                       "getblockhash",
                       "getblockcount",
                       "getbestchainlock",
                       "quorum",
                       "verifyislock"]

        help_output = self.nodes[0].help().split('\n')
        nonwhitelisted = set()

        for line in help_output:
            line = line.strip()

            # Ignore blanks, headers and whitelisted
            if line and not line.startswith('='):
                command = line.split()[0]
                if not command in whitelisted:
                    nonwhitelisted.add(command)

        rpcuser_authpair_platform = "platform-user:password123"
        rpcuser_authpair_operator = "operator:otherpassword"
        rpcuser_authpair_wrong = "platform-user:rpcpasswordwrong"

        self.log.info('Try using a incorrect password for platform-user...')
        test_command("getbestblockhash", [], rpcuser_authpair_wrong, 401)

        self.log.info('Try using a correct password for platform-user and running all whitelisted commands...')
        test_command("getbestblockhash", [], rpcuser_authpair_platform, 200)
        test_command("getblockhash", [0], rpcuser_authpair_platform, 200)
        test_command("getblockcount", [], rpcuser_authpair_platform, 200)
        test_command("getbestchainlock", [], rpcuser_authpair_platform, 500)
        test_command("quorum", ["sign", 100], rpcuser_authpair_platform, 500)
        test_command("quorum", ["sign", 100, "0000000000000000000000000000000000000000000000000000000000000000",
                                "0000000000000000000000000000000000000000000000000000000000000001"],
                                rpcuser_authpair_platform, 200)
        test_command("quorum", ["verify"], rpcuser_authpair_platform, 500)
        test_command("verifyislock", [], rpcuser_authpair_platform, 500)

        self.log.info('Try using some invalid combinations for platform-user')
        test_command("quorum", [], rpcuser_authpair_platform, 403)
        test_command("quorum", ["sign"], rpcuser_authpair_platform, 403)
        test_command("quorum", ["sign", 102], rpcuser_authpair_platform, 403)
        test_command("quorum", ["sign", "100"], rpcuser_authpair_platform, 403)
        test_command("quorum", ["dkgsimerror"], rpcuser_authpair_platform, 403)

        self.log.info('Try running all non-whitelisted commands as each user...')
        for command in nonwhitelisted:
            test_command(command, [], rpcuser_authpair_platform, 403)
            if command != "stop":  # avoid stopping the node while testing
                # we don't care about the exact status here, should simply be anything else but 403
                test_command(command, [], rpcuser_authpair_operator, 403, True)

        self.log.info('Try running a not whitelisted command as the operator...')
        test_command("debug", ["1"], rpcuser_authpair_operator, 200)


if __name__ == '__main__':
    HTTPBasicsTest().main()
