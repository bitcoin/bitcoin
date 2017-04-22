#!/usr/bin/env python3
# Copyright (c) 2017 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.util import *

import decimal

def isclose(a, b, rel_tol=1e-09, abs_tol=0.0):
    a = decimal.Decimal(a)
    b = decimal.Decimal(b)
    return abs(a-b) <= max(decimal.Decimal(rel_tol) * decimal.Decimal(max(abs(a), abs(b))), abs_tol)

class BlindTest(ParticlTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [ ['-debug',] for i in range(self.num_nodes)]

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, self.extra_args, genfirstkey=False)
        
        connect_nodes_bi(self.nodes, 0, 1)
        connect_nodes_bi(self.nodes, 0, 2)
        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        nodes = self.nodes
        
        # stop staking
        ro = nodes[0].reservebalance(True, 10000000)
        ro = nodes[1].reservebalance(True, 10000000)
        
        ro = nodes[0].extkeyimportmaster("abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb")
        assert(ro['account_id'] == 'aaaZf2qnNr5T7PWRmqgmusuu5ACnBcX2ev')
        
        #ro = node.extkey("list", "true")
        #print(json.dumps(ro, indent=4))
        
        ro = nodes[0].getinfo()
        assert(ro['total_balance'] == 100000)
        
        txnHashes = []
        
        #assert(self.wait_for_height(node, 1))
        
        ro = nodes[1].extkeyimportmaster("drip fog service village program equip minute dentist series hawk crop sphere olympic lazy garbage segment fox library good alley steak jazz force inmate")
        
        sxAddrTo1_1 = nodes[1].getnewstealthaddress()
        assert(sxAddrTo1_1 == 'TetbYTGv5LiqyFiUD3a5HHbpSinQ9KiRYDGAMvRzPfz4RnHMbKGAwDr1fjLGJ5Eqg1XDwpeGyqWMiwdK3qM3zKWjzHNpaatdoHVzzA')
        
        txnHash = nodes[0].sendparttoblind(sxAddrTo1_1, 0.4, '', '', False, 'node0 -> node1 blind')
        print("txnHash ", txnHash)
        txnHashes.append(txnHash)
        
        
        
        
        assert(False)
        #print(json.dumps(ro, indent=4))
        

if __name__ == '__main__':
    BlindTest().main()
