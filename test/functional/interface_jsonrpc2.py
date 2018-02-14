#!/usr/bin/env python3
# Copyright (c) 2014-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the JSON-RPC 2.0 strict support."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

from test_framework.authproxy import JSONRPCException

import http.client
import urllib.parse
import json

class JSONRPC2Test(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-strictjsonrpcspec"]]
        self.skip_rpc_check = True

    def setup_network(self):
        self.setup_nodes()

    def run_test(self):
        url = urllib.parse.urlparse(self.nodes[0].url)
        authpair = url.username + ':' + url.password
        headers = {"Authorization": "Basic " + str_to_b64str(authpair)}

        conn = http.client.HTTPConnection(url.hostname, url.port)
        conn.connect()

        # jsonrpc field must exist
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        out1 = conn.getresponse().read()
        obj1 = json.loads(out1.decode("utf-8"))
        assert "error" in obj1
        assert "result" not in obj1
        assert_equal(obj1["error"]["code"], -32600) # The JSON sent is not a valid Request object.
        assert_equal(obj1["error"]["message"], "Missing jsonrpc field")

        # jsonrpc field MUST be exactly "2.0"
        conn.request('POST', '/', '{"method": "getbestblockhash", "jsonrpc": "dog"}', headers)
        out2 = conn.getresponse().read()
        obj2 = json.loads(out2.decode("utf-8"))
        assert "error" in obj2 # result OR error
        assert "result" not in obj2
        assert_equal(obj2["error"]["code"], -32600) # The JSON sent is not a valid Request object.
        assert_equal(obj2["error"]["message"], "jsonrpc field must be equal to '2.0'")

        # Notifications (no id) should be ignored (no response)
        conn.request('POST', '/', '{"method": "getbestblockhash", "jsonrpc": "2.0"}', headers)
        out3 = conn.getresponse().read()
        assert_equal(out3, b"Unhandled request")

        # Valid request - id SHOULD normally not be null, but can be.
        conn.request('POST', '/', '{"method": "getbestblockhash", "id": null, "jsonrpc": "2.0"}', headers)
        out4 = conn.getresponse().read()
        obj4 = json.loads(out4.decode("utf-8"))
        assert "result" in obj4
        assert "error" not in obj4 # result _or_ error
        assert obj4["result"] is not None
        assert_equal(obj4["id"], None)
        assert_equal(obj4["jsonrpc"], "2.0")

        # Notifications within batch requests should be ignored
        conn.request('POST', '/', json.dumps([
            {"method": "getbestblockhash", "id": "2", "jsonrpc": "2.0"},
            {"method": "getbestblockhash", "jsonrpc": "2.0"},
        ]), headers)
        out4 = conn.getresponse().read()
        obj4 = json.loads(out4.decode("utf-8"))
        assert len(obj4) == 1

        # Empty batch requests should be ignored (no response), not "[]"
        conn.request('POST', '/', json.dumps([
            {"method": "getbestblockhash", "jsonrpc": "2.0"},
            {"method": "getbestblockhash", "jsonrpc": "2.0"},
        ]), headers)
        out5 = conn.getresponse().read()
        assert_equal(out5, b"Unhandled request")

if __name__ == '__main__':
    JSONRPC2Test().main ()
