#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.pop import mine_until_pop_active
from test_framework.util import (
    connect_nodes,
)
from test_framework.pop_const import NETWORK_ID

import time


class PopParams(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [[]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypoptools()

    def setup_network(self):
        self.setup_nodes()
        mine_until_pop_active(self.nodes[0])

        for i in range(self.num_nodes - 1):
            connect_nodes(self.nodes[i + 1], i)
        self.sync_all()

    def run_test(self):
        """Main test logic"""

        from pypoptools.pypopminer import MockMiner
        self.apm = MockMiner()
        self.node = self.nodes[0]

        self.bootstrap_block_exists()

    def bootstrap_block_exists(self):
        param = self.node.getpopparams()
        bootstrap = param['bootstrapBlock']
        bootstraphash = bootstrap['hash']

        try:
            block = self.node.getblock(bootstraphash)
            self.log.info("Test passed: bootstrap block={} exists!".format(bootstraphash))
            return
        except:
            pass

        hash = bytes.fromhex(bootstraphash)[::-1].hex()
        try:
            block = self.node.getblock(hash)
        except:
            raise Exception('''
            
            It appears that bootstrap block hash can not be found in local blockchain. 
            Make sure you specified EXISTING Bootstrap block.
            
            ''')

        self.log.info("Test passed: bootstrap block={} exists (direction B)!".format(hash))
        raise Exception('''
        
        It appears that bootstrapBlock hash is reversed in getpopparams rpc call.
        Please, try setting second parameter ("reversedAltHashes") to true/false in ToJSON call
        for AltChainParams inside getpopparams rpc call.
        
        ''')


if __name__ == '__main__':
    PopParams().main()
