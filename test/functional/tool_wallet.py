#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bitcoint-wallet-tool.
"""

import os
import subprocess
import textwrap

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class ToolWalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def process(self, *args):
        binary = self.config["environment"]["BUILDDIR"] + '/src/bitcoin-wallet-tool' + self.config["environment"]["EXEEXT"]
        input = None
        return subprocess.Popen([binary] + list(args), stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)

    def assert_raises_tool_error(self, error, *args):
        process = self.process(*args)
        stdout, stderr = process.communicate()
        assert_equal(process.poll(), 1)
        assert_equal(stdout, '')
        assert_equal(stderr.strip(), error)

    def assert_tool_output(self, output, *args):
        process = self.process(*args)
        stdout, stderr = process.communicate()
        assert_equal(process.poll(), 0)
        assert_equal(stderr, '')
        assert_equal(stdout, output)

    def run_test(self):

        self.assert_raises_tool_error('Unknown command', 'foo')
        self.assert_raises_tool_error('Error parsing command line arguments: Invalid parameter -foo', '-foo')

        wallet_dir = lambda *p: data_dir('wallets', *p)
        data_dir = lambda *p: os.path.join(self.nodes[0].datadir, 'regtest', *p)

        # TODO calling info on a wallet already opened results in the segfault:
        #  `libc++abi.dylib: terminating with uncaught exception of type std::runtime_error: BerkeleyBatch: Failed to open database environment`
        #  the error must be fixed and tested here

        # stop the node to close the wallet to call info command
        self.stop_node(0)

        out = textwrap.dedent('''\
            Wallet info
            ===========
            Encrypted: no
            HD (hd seed available): yes
            Keypool Size: 2
            Transactions: 50
            Address Book: 0
        ''')
        self.assert_tool_output(out, '-name=' + wallet_dir(), 'info')

        # mutate the wallet to check the info command output changes accordingly
        self.start_node(0)
        self.nodes[0].generate(1)
        self.stop_node(0)

        out = textwrap.dedent('''\
            Wallet info
            ===========
            Encrypted: no
            HD (hd seed available): yes
            Keypool Size: 1
            Transactions: 51
            Address Book: 0
        ''')
        self.assert_tool_output(out, '-name=' + wallet_dir(), 'info')

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
        self.assert_tool_output(out, '-name=' + wallet_dir('foo'), 'create')

        self.start_node(0, ['-wallet=' + wallet_dir('foo')])
        out = self.nodes[0].getwalletinfo()
        self.stop_node(0)

        assert_equal(0, out['txcount'])
        assert_equal(1000, out['keypoolsize'])
        assert_equal(1000, out['keypoolsize_hd_internal'])
        assert_equal(True, 'hdseedid' in out)

if __name__ == '__main__':
    ToolWalletTest().main()
