#!/usr/bin/env python3
# Copyright (c) 2018-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import os
import shutil
import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.ltc_util import setup_mweb_chain

from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

class MWEBNodeCompatibilityTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.wallet_names = [self.default_wallet_name]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_previous_releases()

    def setup_nodes(self):
        versions = [None, None, 180100]
        self.add_nodes(len(versions), versions=versions)
        self.start_nodes()
        self.import_deterministic_coinbase_privkeys()

    def setup_network(self):
        self.setup_nodes()
        self.connect_nodes(0, 1)
        self.connect_nodes(0, 2)
        self.connect_nodes(1, 2)

    def run_test(self):
        node_a_master = self.nodes[0];
        node_b_master = self.nodes[1];
        node_c_v18 = self.nodes[2]
        
        setup_mweb_chain(node_a_master)
        self.sync_all()
        
        a_chain_info = node_a_master.getblockchaininfo()
        b_chain_info = node_b_master.getblockchaininfo()
        c_chain_info = node_c_v18.getblockchaininfo()
        assert int(b_chain_info['blocks']) == int(a_chain_info['blocks'])
        assert int(c_chain_info['blocks']) == int(a_chain_info['blocks'])
        
        self.disconnect_nodes(0, 1)

        mweb_addr = node_a_master.getnewaddress(address_type='mweb')
        pegin_txid = node_a_master.sendtoaddress(mweb_addr, 100)

        self.sync_mempools([node_a_master, node_c_v18], wait=1, timeout=5)
        assert pegin_txid in node_a_master.getrawmempool() and pegin_txid in node_c_v18.getrawmempool()

        # Wait a few seconds to ensure node_b_master did not accept a pegin tx from node_c_v18
        time.sleep(3)
        
        assert set(node_a_master.getrawmempool()) == set([pegin_txid])
        assert set(node_c_v18.getrawmempool()) == set([pegin_txid])
        assert len(node_b_master.getrawmempool()) == 0

if __name__ == '__main__':
    MWEBNodeCompatibilityTest().main()
