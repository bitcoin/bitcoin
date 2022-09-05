#!/usr/bin/env python3
# Copyright (c) 2017-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import os
from test_framework.test_framework import BitcoinTestFramework

class FeatureChainArgCollision(BitcoinTestFramework):
    """
    Keep in mind Bitcoin Test Framework launches bitcoind with -regtest flag by default.
    All the tests are run with that in mind.
    This is for providing a more verbose error message of where chain arguments are colliding.
    It helps when you forget about a chain argument from a config file.
    """
    
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1
        self.supports_cli = False     
        
    def test_network_flag_chain_arg_collision(self):
        """
        Tests for bitcoind -regtest -chain=test
        """
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(
            extra_args=['-chain=test'],
            expected_msg="""************************
EXCEPTION: St13runtime_error       
Invalid combination of -regtest, -signet, -testnet and -chain. Can use at most one. Chain argument is being set in commandline with network argument.       
bitcoin in AppInit()       

Assertion failed: (globalChainBaseParams), function BaseParams, file chainparamsbase.cpp, line 35."""
        )
        
    def test_multiple_network_flag(self):
        """
        Tests for bitcoind -regtest -testnet
        """
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(
            extra_args=['-testnet'],
            expected_msg="""************************
EXCEPTION: St13runtime_error       
Invalid combination of -regtest, -signet, -testnet and -chain. Can use at most one. Too many network flags being set in the commandline.       
bitcoin in AppInit()       

Assertion failed: (globalChainBaseParams), function BaseParams, file chainparamsbase.cpp, line 35."""
        )
        
    def test_config_chain_arg(self):
        """
        Tests for chain=regtest in an include.conf file imported into a bitcoin.conf file.
        """

        self.stop_node(0)
        inc_conf_file_path = os.path.join(self.nodes[0].datadir, 'include.conf')
        with open(os.path.join(self.nodes[0].datadir, 'bitcoin.conf'), 'a', encoding='utf-8') as conf:
            conf.write(f'includeconf={inc_conf_file_path}\n')
            
        with open(inc_conf_file_path, 'w', encoding='utf-8') as conf:
            conf.write('chain=regtest\n')
            
        self.nodes[0].assert_start_raises_init_error(
            expected_msg="""************************
EXCEPTION: St13runtime_error       
Invalid combination of -regtest, -signet, -testnet and -chain. Can use at most one. Chain argument is being set in a config file and by network arguments in the commandline.       
bitcoin in AppInit()       

Assertion failed: (globalChainBaseParams), function BaseParams, file chainparamsbase.cpp, line 35."""
        )
        
    def run_test(self):
        self.test_network_flag_chain_arg_collision()
        self.test_multiple_network_flag()
        self.test_config_chain_arg()
        

            
    
if __name__ == '__main__':
    FeatureChainArgCollision().main()
    