#!/usr/bin/env python3
# Copyright (c) 2017-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test getrpcwhitelist RPC call.
"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    get_datadir_path,
    str_to_b64str
)
import http.client
import json
import os
import urllib.parse

def call_rpc(node, settings, rpc):
    url = urllib.parse.urlparse(node.url)
    headers = {"Authorization": "Basic " + str_to_b64str('{}:{}'.format(settings[0], settings[2]))}
    conn = http.client.HTTPConnection(url.hostname, url.port)
    conn.connect()
    conn.request('POST', '/', '{"method": "' + rpc + '"}', headers)
    resp = conn.getresponse()
    code = resp.status
    if code == 200:
        json_ret = json.loads(resp.read().decode())
    else:
        json_ret = {"result": None}
    conn.close()
    return {"status": code, "json": json_ret['result']}

class RPCWhitelistTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def setup_chain(self):
        super().setup_chain()
        self.settings = ["dummy",
                         "4e799db4b65924f4468b1c9ff3a68109$5fcd282dcaf4ae74599934a543626c0a11e7e83ead30f07b182058ead8e85da9",
                         "dummypwd",
                         "getbalance,getrpcwhitelist,getwalletinfo"]
        self.settings_forbidden = ["dummy2",
                        "f3d319f64b076012f75626c9d895fced$7f55381a24fda02c5de7c18fc377f56fc573149b4d6f83daa9fd584210b51f99",
                        "dummy2pwd",
                        "getbalance,getwalletinfo"]

        # declare rpc-whitelisting entries
        with open(os.path.join(get_datadir_path(self.options.tmpdir, 0), "bitcoin.conf"), 'a', encoding='utf8') as f:
            f.write("\nrpcwhitelistdefault=0\n")
            f.write("rpcauth={}:{}\n".format(self.settings[0], self.settings[1]))
            f.write("rpcwhitelist={}:{}\n".format(self.settings[0], self.settings[3]))
            f.write("rpcauth={}:{}\n".format(self.settings_forbidden[0], self.settings_forbidden[1]))
            f.write("rpcwhitelist={}:{}\n".format(self.settings_forbidden[0], self.settings_forbidden[3]))

    def run_test(self):
        self.log.info("Test getrpcwhitelist")
        whitelisted = {method: None for method in self.settings[3].split(',')}

        # should return allowed rpcs
        result = call_rpc(self.nodes[0], self.settings, 'getrpcwhitelist')
        assert_equal(200, result['status'])
        assert_equal(result['json']['methods'], whitelisted)
        # should fail because user has no rpcwhitelist-rpc entry in bitcoin.conf
        result = call_rpc(self.nodes[0], self.settings_forbidden, 'getrpcwhitelist')
        assert_equal(result['status'], 403)

        # should return a long list of allowed RPC methods (ie, all of them)
        result = self.nodes[0].getrpcwhitelist()
        assert len(result['methods']) > 10

if __name__ == "__main__":
    RPCWhitelistTest(__file__).main()
