#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
import time


class WalletSPVTest(BitcoinTestFramework):
 
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 4
        self.extra_args = [["-debug=net"], ["-spv=1", "-spvonly=1", "-debug=net"], ["-debug=net"]]

    def setup_network(self, split=False):
        self.nodes = start_nodes(3, self.options.tmpdir, self.extra_args)
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[0], 2)
        connect_nodes(self.nodes[1], 2)
        self.is_network_split=False
        self.sync_all()

    def sync_spv(self, headerheight, wait=1, timeout=20):
        """
        Wait until everybody has the same tip
        """
        maxheight = 0
        while timeout > 0:
            insync = True
            for x in self.nodes:
                info = x.getwalletinfo()
                if info['spv']['enabled'] == True:
                    if not info['spv']['synced_up_to_height'] == headerheight:
                        insync = False
            if insync == True:
                return True
            timeout -= wait
            time.sleep(wait)
        raise AssertionError("SPV sync failed")

    def run_test(self):
        # Generate some old blocks
        addr = self.nodes[1].getnewaddress() #for 1 conf tx
        self.nodes[0].generate(130)
        self.nodes[0].sendtoaddress(addr, 1.1)
        self.nodes[0].generate(1)
        headerheight = self.nodes[0].getblockchaininfo()['headers']
        self.sync_spv(headerheight)

        time.sleep(5)
        
        node1info = self.nodes[1].getblockchaininfo()
        assert_equal(node1info['blocks'], 0)
        assert_equal(node1info['headers'], headerheight)
        spvinfo = self.nodes[1].getwalletinfo()['spv']
        assert_equal(headerheight, spvinfo['best_known_header_height'])
        lt = self.nodes[1].listtransactions()
        
        # node1 is in SPV only mode
        # txes should not be present in the wallet with spv
        assert_equal(lt[0]['address'], addr)
        assert_equal(lt[0]['spv'], True)
        assert_equal(lt[0]['confirmations'], 1)
        
        print("Restarting nodes without -spvonly (hybrid SPV)")
        self.stop_node(1)
        os.mkdir(self.options.tmpdir + "/node3/regtest")
        shutil.copyfile(self.options.tmpdir + "/node1/regtest/wallet.dat", self.options.tmpdir + "/node3/regtest/wallet.dat")
        self.nodes[1] = start_node(1, self.options.tmpdir, ["-spv=1", "-debug=net"])
        connect_nodes_bi(self.nodes,0, 1)
        connect_nodes_bi(self.nodes,1, 2)
    
        self.nodes[0].resendwallettransactions()
        self.sync_all()
        
        lt = self.nodes[1].listtransactions()

        # tx should now be fully validated
        assert_equal(lt[0]['address'], addr)
        assert_equal(lt[0]['spv'], False)
        assert_equal(lt[0]['confirmations'], 1)
        
        # start another node with initial hyprid SPV mode
        # use the wallet from node1
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug=net","-spv=1"]))
        connect_nodes_bi(self.nodes,0, 3)
        connect_nodes_bi(self.nodes,0, 2)
        headerheight = self.nodes[0].getblockchaininfo()['headers']
        self.sync_spv(headerheight)
        # make sure we have identical mempools
        self.nodes[0].resendwallettransactions()
        self.sync_all()
        
        # currently there is no efficient way to test for the "SPV first" wallet listing
        # but we test that the hybrid modes results with a standard full validated wtxns
        lt = self.nodes[3].listtransactions()
        assert_equal(lt[0]['address'], addr)
        assert_equal(lt[0]['spv'], False)
        assert_equal(lt[0]['confirmations'], 1)
    

if __name__ == '__main__':
    WalletSPVTest().main()
