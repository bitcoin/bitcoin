#!/usr/bin/env python3
# Copyright (c) 2019-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test rpc calls can be forbidden for main chain

Test the following RPCs:
    - setmocktime
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error

class RpcForbiddenTest(BitcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [[
            '-allow_set_mocktime=0',
        ]]

    def run_test(self):
        assert_raises_rpc_error(-8, 'setmocktime is not allowed for chain %s' % self.chain, self.nodes[0].setmocktime, '1255227617')

if __name__ == '__main__':
    RpcForbiddenTest().main()
