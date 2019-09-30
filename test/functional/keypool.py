#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the wallet keypool and interaction with wallet encryption/locking."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class KeyPoolTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [['-usehd=0']]

    def run_test(self):
        nodes = self.nodes

        # Encrypt wallet and wait to terminate
        nodes[0].node_encrypt_wallet('test')
        # Restart node 0
        self.start_node(0)
        # Keep creating keys
        addr = nodes[0].getnewaddress()

        assert_raises_rpc_error(-12, "Error: Keypool ran out, please call keypoolrefill first", nodes[0].getnewaddress)

        # put three new keys in the keypool
        nodes[0].walletpassphrase('test', 12000)
        nodes[0].keypoolrefill(3)
        nodes[0].walletlock()

        # drain the keys
        addr = set()
        addr.add(nodes[0].getrawchangeaddress())
        addr.add(nodes[0].getrawchangeaddress())
        addr.add(nodes[0].getrawchangeaddress())
        # assert that three unique addresses were returned
        assert(len(addr) == 3)
        # the next one should fail
        assert_raises_rpc_error(-12, "Keypool ran out", nodes[0].getrawchangeaddress)

        # refill keypool with three new addresses
        nodes[0].walletpassphrase('test', 1)
        nodes[0].keypoolrefill(3)
        # test walletpassphrase timeout
        time.sleep(1.1)
        assert_equal(nodes[0].getwalletinfo()["unlocked_until"], 0)

        # drain them by mining
        nodes[0].generate(1)
        nodes[0].generate(1)
        nodes[0].generate(1)
        assert_raises_rpc_error(-12, "Keypool ran out", nodes[0].generate, 1)

if __name__ == '__main__':
    KeyPoolTest().main()
