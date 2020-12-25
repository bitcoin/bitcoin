#!/usr/bin/env python3
# Copyright (c) 2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from test_framework.test_framework import DashTestFramework
'''
rpc_masternode.py

Test "masternode" rpc subcommands
'''

class RPCMasternodeTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(4, 3, fast_dip3_enforcement=True)

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("test that results from `winners` and `payments` RPCs match")


if __name__ == '__main__':
    RPCMasternodeTest().main()
