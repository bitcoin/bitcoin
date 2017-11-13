#!/usr/bin/env python3
# Copyright (c) 2017 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.util import *

class ExtKeyTest(ParticlTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [ ['-debug',] for i in range(self.num_nodes)]

    def setup_network(self, split=False):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args)
        self.start_nodes()

        connect_nodes_bi(self.nodes, 0, 1)
        connect_nodes_bi(self.nodes, 0, 2)
        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        node = self.nodes[0]
        node1 = self.nodes[1]

        # stop staking
        ro = node.reservebalance(True, 10000000)

        ro = node.extkeyimportmaster("abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb")
        assert(ro['account_id'] == 'aaaZf2qnNr5T7PWRmqgmusuu5ACnBcX2ev')

        #ro = node.extkey("list", "true")
        #print(json.dumps(ro, indent=4))

        ro = node.getinfo()
        assert(ro['total_balance'] == 100000)

        # Start staking
        ro = node.walletsettings('stakelimit', {'height':1})
        ro = node.reservebalance(False)

        assert(self.wait_for_height(node, 1))

        # stop staking
        ro = node.reservebalance(True, 10000000)
        ro = node1.reservebalance(True, 10000000)

        ro = node1.extkeyimportmaster("drip fog service village program equip minute dentist series hawk crop sphere olympic lazy garbage segment fox library good alley steak jazz force inmate")
        #print(ro)

        extAddrTo = node1.getnewextaddress()
        #print(extAddrTo)

        txnHash = node.sendtoaddress(extAddrTo, 10)
        #print(txnHash)

        ro = node.getmempoolentry(txnHash)
        #print("getmempoolentry",ro)
        assert(ro['height'] == 1)

        #ro = node.listtransactions()
        #print("listtransactions",ro)
        #print(json.dumps(ro, indent=4))

        # start staking
        ro = node.walletsettings('stakelimit', {'height':2})
        ro = node.reservebalance(False)

        assert(self.wait_for_height(node, 2))

        # stop staking
        ro = node.reservebalance(True, 10000000)


        ro = node1.listtransactions()
        #print("listtransactions node1",ro)

        assert(len(ro) == 1)
        assert(ro[0]['address'] == 'pkGv5xgviEAEjwpRPeEt8c9cvraw2umKYo')
        assert(ro[0]['amount'] == 10)

        ro = node1.getinfo()
        assert(ro['total_balance'] == 10)


        block2_hash = node.getblockhash(2)

        ro = node.getblock(block2_hash)
        assert(txnHash in ro['tx'])


        txnHash2 = node.sendtoaddress(extAddrTo, 20, '', '', False, 'narration test')
        print(txnHash2)

        assert(self.wait_for_mempool(node1, txnHash2))

        ro = node1.listtransactions()
        #print("\nlisttransactions node1",ro)

        assert(len(ro) == 2)
        assert(ro[1]['address'] == 'pbo5e7tsLJBdUcCWteTTkGBxjW8Xy12o1V')
        assert(ro[1]['amount'] == 20)
        assert('narration test' in ro[1].values())


        ro = node.listtransactions()
        #print("\nlisttransactions node",ro)
        assert('narration test' in ro[-1].values())


        extAddrTo0 = node.getnewextaddress()

        txnHashes = []
        for k in range(24):
            v = round(0.01 * float(k+1), 5)
            #print(v)
            txnHash = node1.sendtoaddress(extAddrTo0, v, '', '', False)
            #assert(self.wait_for_mempool(node1, txnHash))
            txnHashes.append(txnHash)

        for txnHash in txnHashes:
            assert(self.wait_for_mempool(node, txnHash))

        ro = node.listtransactions()
        #print("\nlisttransactions node",ro)


        # start staking
        ro = node.walletsettings('stakelimit', {'height':3})
        ro = node.reservebalance(False)

        assert(self.wait_for_height(node, 3))

        block3_hash = node.getblockhash(3)
        ro = node.getblock(block3_hash)

        for txnHash in txnHashes:
            assert(txnHash in ro['tx'])


        #assert(False)
        #print(json.dumps(ro, indent=4))


if __name__ == '__main__':
    ExtKeyTest().main()
