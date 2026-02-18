#!/usr/bin/env python3
# Copyright (c) 2015-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Bearer token authentication for RPC."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

import http.client
import subprocess
import sys
import urllib.parse


def call_with_bearer_token(node, token, method="getbestblockhash"):
    """Make an RPC call with Bearer token authentication."""
    url = urllib.parse.urlparse(node.url)
    headers = {"Authorization": f"Bearer {token}"}

    conn = http.client.HTTPConnection(url.hostname, url.port)
    conn.connect()
    conn.request('POST', '/', f'{{"method": "{method}"}}', headers)
    resp = conn.getresponse()
    conn.close()
    return resp


class TokenAuthTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.supports_cli = False

    def setup_network(self):
        # Generate token credentials using rpctoken.py
        gen_rpctoken = self.config["environment"]["RPCAUTH"].replace("rpcauth.py", "rpctoken.py")
        
        # Generate token with specified value for testing
        self.test_token = "test_token_abc123xyz789"
        p = subprocess.Popen([sys.executable, gen_rpctoken, 'tokenuser', self.test_token], 
                           stdout=subprocess.PIPE, text=True)
        lines = p.stdout.read().splitlines()
        rpctoken = lines[1]
        
        # Generate token without specifying value (random)
        self.test_user2 = 'tokenuser2'
        p = subprocess.Popen([sys.executable, gen_rpctoken, self.test_user2], 
                           stdout=subprocess.PIPE, text=True)
        lines = p.stdout.read().splitlines()
        rpctoken2 = lines[1]
        self.test_token2 = lines[3]

        # Add token config to bitcoin.conf
        with open(self.nodes[0].datadir_path / "bitcoin.conf", "a") as f:
            f.write(f"{rpctoken}\n")
            f.write(f"{rpctoken2}\n")
        
        self.start_node(0)

    def run_test(self):
        self.log.info('Test Bearer token authentication')
        
        # Test successful authentication with token
        self.log.info('Testing correct token authentication...')
        assert_equal(200, call_with_bearer_token(self.nodes[0], self.test_token).status)
        assert_equal(200, call_with_bearer_token(self.nodes[0], self.test_token2).status)

        # Test failed authentication with wrong token
        self.log.info('Testing incorrect token authentication...')
        assert_equal(401, call_with_bearer_token(self.nodes[0], self.test_token + 'wrong').status)
        assert_equal(401, call_with_bearer_token(self.nodes[0], 'completely_wrong_token').status)

        self.log.info('Test malformed Bearer token')
        # Test with empty token
        assert_equal(401, call_with_bearer_token(self.nodes[0], '').status)

        self.log.info('Bearer token authentication test passed')


if __name__ == '__main__':
    TokenAuthTest(__file__).main()
