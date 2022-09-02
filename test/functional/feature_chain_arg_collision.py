#!/usr/bin/env python3
# Copyright (c) 2017-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import os
from re import X
from test_framework.test_framework import BitcoinTestFramework

class FeatureChainArgCollision(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1

    def run_test(self):
        """
        - test terminating initialization after seeing a certain log line.
        - test removing certain essential files to test startup error paths.
        """
        
        self.stop_node(0)
        self.setup_config()
        with self.nodes[0].assert_start_raises_init_error(['-chain=test'], 'Invalid combination of -regtest, -signet, -testnet and -chain. Can use at most one.'):
            self.start_node(0)
            
        
    def setup_config(self):
        self.stop_node(0)
        
        inc_conf_file_path = os.path.join(self.nodes[0].datadir, 'include.conf')
        
        with open(os.path.join(self.nodes[0].datadir, 'bitcoin.conf'), 'a', encoding='utf-8') as conf:
            conf.write(f'includeconf={inc_conf_file_path}\n')
            
        with open(inc_conf_file_path, 'w', encoding='utf-8') as conf:
            conf.write('chain=test\n')
            
    
if __name__ == '__main__':
    FeatureChainArgCollision().main()