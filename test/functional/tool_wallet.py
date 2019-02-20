#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bitcoin-wallet."""
import subprocess
import textwrap

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class ToolWalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def bitcoin_wallet_process(self, *args):
        binary = self.config["environment"]["BUILDDIR"] + '/src/bitcoin-wallet' + self.config["environment"]["EXEEXT"]
        args = ['-datadir={}'.format(self.nodes[0].datadir), '-regtest'] + list(args)
        return subprocess.Popen([binary] + args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)

    def assert_raises_tool_error(self, error, *args):
        p = self.bitcoin_wallet_process(*args)
        stdout, stderr = p.communicate()
        assert_equal(p.poll(), 1)
        assert_equal(stdout, '')
        assert_equal(stderr.strip(), error)

    def assert_tool_output(self, output, *args):
        p = self.bitcoin_wallet_process(*args)
        stdout, stderr = p.communicate()
        assert_equal(p.poll(), 0)
        assert_equal(stderr, '')
        assert_equal(stdout, output)

    def run_test(self):

        self.assert_raises_tool_error('Invalid method: foo', 'foo')
        # `bitcoin-wallet help` is an error. Use `bitcoin-wallet -help`
        self.assert_raises_tool_error('Invalid method: help', 'help')
        self.assert_raises_tool_error('Error: unexpected argument(s)', 'info', 'create')
        self.assert_raises_tool_error('Error parsing command line arguments: Invalid parameter -foo', '-foo')
        self.assert_raises_tool_error('Error loading wallet.dat. Is wallet being used by another process?', '-wallet=wallet.dat', 'info')
        self.assert_raises_tool_error('Error: no wallet file at nonexistent.dat', '-wallet=nonexistent.dat', 'info')

        # stop the node to close the wallet to call info command
        self.stop_node(0)

        self.log.info("Test method: info")

        out = textwrap.dedent('''\
            Wallet info
            ===========
            Encrypted: no
            HD (hd seed available): yes
            Keypool Size: 2
            Transactions: 0
            Address Book: 3
        ''')
        self.assert_tool_output(out, '-wallet=wallet.dat', 'info')

        # mutate the wallet to check the info command output changes accordingly
        self.start_node(0)
        self.nodes[0].generate(1)
        self.stop_node(0)

        out = textwrap.dedent('''\
            Wallet info
            ===========
            Encrypted: no
            HD (hd seed available): yes
            Keypool Size: 2
            Transactions: 1
            Address Book: 3
        ''')
        self.assert_tool_output(out, '-wallet=wallet.dat', 'info')

        self.log.info("Test method: create")

        out = textwrap.dedent('''\
            Topping up keypool...
            Wallet info
            ===========
            Encrypted: no
            HD (hd seed available): yes
            Keypool Size: 2000
            Transactions: 0
            Address Book: 0
        ''')
        self.assert_tool_output(out, '-wallet=foo', 'create')

        self.start_node(0, ['-wallet=foo'])
        out = self.nodes[0].getwalletinfo()
        self.stop_node(0)

        assert_equal(0, out['txcount'])
        assert_equal(1000, out['keypoolsize'])
        assert_equal(1000, out['keypoolsize_hd_internal'])
        assert_equal(True, 'hdseedid' in out)

        self.log.info("Test method: keymetadata")
        self.start_node(0, ['-wallet=foo'])
        address = self.nodes[0].getnewaddress()
        address_info_before = self.nodes[0].getaddressinfo(address)
        assert ("hdmasterfingerprint" in address_info_before)
        result = self.nodes[0].importmulti([{
            "desc":
            "wpkh([deadbeef/1/2'/3/4']03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd)#yk54u40t",
            "timestamp": "now",
            "watchonly": True
        }])
        assert (result[0]["success"])
        address_watch_only = "bcrt1qngw83fg8dz0k749cg7k3emc7v98wy0c7azaa6h"
        address_watch_only_info_before = self.nodes[0].getaddressinfo(address_watch_only)
        assert_equal(address_watch_only_info_before["hdmasterfingerprint"], "deadbeef")
        self.stop_node(0)
        out = textwrap.dedent('''\
            Modifying key metadata...
            =========================
            Operation: delete
            Key: all
        ''')
        self.assert_tool_output(out, '-wallet=foo', 'keymetadata', 'delete')
        self.start_node(0, ['-wallet=foo'])
        address_info_after = self.nodes[0].getaddressinfo(address)
        assert ("hdmasterfingerprint" not in address_info_after)
        address_watch_only_info_after = self.nodes[0].getaddressinfo(address)
        assert ("hdmasterfingerprint" not in address_watch_only_info_after)
        self.stop_node(0)


if __name__ == '__main__':
    ToolWalletTest().main()
