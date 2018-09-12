#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Copyright (c) 2017-2018 The Raven Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test raven-cli"""
from test_framework.test_framework import RavenTestFramework
from test_framework.util import assert_equal, assert_raises_process_error, get_auth_cookie

class TestRavenCli(RavenTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        """Main test logic"""

        self.log.info("Compare responses from getinfo RPC and `raven-cli getinfo`")
        cli_get_info = self.nodes[0].cli.getinfo()
        rpc_get_info = self.nodes[0].getinfo()

        assert_equal(cli_get_info, rpc_get_info)

        user, password = get_auth_cookie(self.nodes[0].datadir)

        self.log.info("Test -stdinrpcpass option")
        assert_equal(0, self.nodes[0].cli('-rpcuser=%s' % user, '-stdinrpcpass', input=password).getblockcount())
        assert_raises_process_error(1, "incorrect rpcuser or rpcpassword", self.nodes[0].cli('-rpcuser=%s' % user, '-stdinrpcpass', input="foo").echo)

        self.log.info("Test -stdin and -stdinrpcpass")
        assert_equal(["foo", "bar"], self.nodes[0].cli('-rpcuser=%s' % user, '-stdin', '-stdinrpcpass', input=password + "\nfoo\nbar").echo())
        assert_raises_process_error(1, "incorrect rpcuser or rpcpassword", self.nodes[0].cli('-rpcuser=%s' % user, '-stdin', '-stdinrpcpass', input="foo").echo)

        self.log.info("Compare responses from `raven-cli -getinfo` and the RPCs data is retrieved from.")
        cli_get_info = self.nodes[0].cli('-getinfo').help()
        wallet_info = self.nodes[0].getwalletinfo()
        network_info = self.nodes[0].getnetworkinfo()
        blockchain_info = self.nodes[0].getblockchaininfo()

        assert_equal(cli_get_info['version'], network_info['version'])
        assert_equal(cli_get_info['protocolversion'], network_info['protocolversion'])
        assert_equal(cli_get_info['walletversion'], wallet_info['walletversion'])
        assert_equal(cli_get_info['balance'], wallet_info['balance'])
        assert_equal(cli_get_info['blocks'], blockchain_info['blocks'])
        assert_equal(cli_get_info['timeoffset'], network_info['timeoffset'])
        assert_equal(cli_get_info['connections'], network_info['connections'])
        assert_equal(cli_get_info['proxy'], network_info['networks'][0]['proxy'])
        assert_equal(cli_get_info['difficulty'], blockchain_info['difficulty'])
        assert_equal(cli_get_info['testnet'], blockchain_info['chain'] == "test")
        assert_equal(cli_get_info['balance'], wallet_info['balance'])
        assert_equal(cli_get_info['keypoololdest'], wallet_info['keypoololdest'])
        assert_equal(cli_get_info['keypoolsize'], wallet_info['keypoolsize'])
        assert_equal(cli_get_info['paytxfee'], wallet_info['paytxfee'])
        assert_equal(cli_get_info['relayfee'], network_info['relayfee'])
        # unlocked_until is not tested because the wallet is not encrypted

if __name__ == '__main__':
    TestRavenCli().main()
