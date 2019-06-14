#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet maxtxfee limiting in conjunction with settxfee or mintxfee."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error

class WalletMaxFeeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.nodes[0].generate(105)

        outputs = {}

        for i in range(10):
            outputs[self.nodes[0].getnewaddress()] = 0.001

        # test sending a tx with normal fee parameters (must work)
        self.nodes[0].sendmany("",outputs)

        # test sending a tx with very low max fee (must still work)
        self.restart_node(0, extra_args=["-maxtxfee=0.0001"])
        self.nodes[0].sendmany("",outputs)

        # test sending a tx with very low max fee and a high paytxfee (must fail)
        self.restart_node(0, extra_args=["-maxtxfee=0.0001","-paytxfee=0.0005"])
        assert_raises_rpc_error(-6, "Fee rate too low after limiting to -maxtxfee", lambda: self.nodes[0].sendmany("",outputs))

        # test sending a tx with very low max fee and a high mintxfee (must fail)
        self.restart_node(0, extra_args=["-maxtxfee=0.0001","-mintxfee=0.0005"])
        assert_raises_rpc_error(-6, "Fee rate too low after limiting to -maxtxfee", lambda: self.nodes[0].sendmany("",outputs))

if __name__ == '__main__':
    WalletMaxFeeTest().main()
