#!/usr/bin/env python3
# Copyright (c) 2017 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.test_particl import isclose
from test_framework.util import *

class PosTest(ParticlTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 4
        self.extra_args = [ ['-debug','-noacceptnonstdtxn'] for i in range(self.num_nodes)]

    def setup_network(self, split=False):
        self.nodes = self.start_nodes(self.num_nodes, self.options.tmpdir, self.extra_args)

        connect_nodes_bi(self.nodes, 0, 1)
        connect_nodes_bi(self.nodes, 0, 2)
        connect_nodes_bi(self.nodes, 0, 3)
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
        ro = node.walletsettings('stakelimit', {'height':1})

        # test reserve balance
        ro = node.getwalletinfo()
        assert(isclose(ro['reserve'], 10000000.0))

        ro = node.reservebalance(True, 100)
        assert(ro['reserve'] == True)
        assert(isclose(ro['amount'], 100.0))

        ro = node.getwalletinfo()
        assert(ro['reserve'] == 100)

        ro = node.reservebalance(False)
        assert(ro['reserve'] == False)
        assert(ro['amount'] == 0)

        ro = node.getwalletinfo()
        assert(ro['reserve'] == 0)

        assert(self.wait_for_height(node, 1))

        # stop staking
        ro = node.reservebalance(True, 10000000)

        oRoot1 = node1.mnemonic("new")
        #print(oRoot1['master'])

        ro = node1.extkeyimportmaster(oRoot1['master'])
        #print(ro)

        addrTo = node1.getnewaddress()
        #print(addrTo)

        txnHash = node.sendtoaddress(addrTo, 10)
        print(txnHash)

        ro = node.getmempoolentry(txnHash)
        #print("getmempoolentry",ro)
        assert(ro['height'] == 1)

        ro = node.listtransactions()
        #print("listtransactions",ro)
        #print(json.dumps(ro, indent=4))

        fPass = False
        for txl in ro:
            #print(txl['address'], txl['amount'])
            if txl['address'] == addrTo and txl['amount'] == -10 and txl['category'] == 'send':
                fPass = True
        assert(fPass), "node0, listtransactions failed."


        assert(self.wait_for_mempool(node1, txnHash))

        ro = node1.listtransactions()
        #print("node1 listtransactions",ro)
        assert(len(ro) == 1)
        assert(ro[0]['address'] == addrTo)
        assert(ro[0]['amount'] == 10)
        assert(ro[0]['category'] == 'receive')


        # start staking
        ro = node.walletsettings('stakelimit', {'height':2})
        ro = node.reservebalance(False)

        assert(self.wait_for_height(node, 2))

        # stop staking
        ro = node.reservebalance(True, 10000000)

        block2_hash = node.getblockhash(2)

        ro = node.getblock(block2_hash)
        assert(txnHash in ro['tx'])

        #assert(False)
        #print(json.dumps(ro, indent=4))


if __name__ == '__main__':
    PosTest().main()
