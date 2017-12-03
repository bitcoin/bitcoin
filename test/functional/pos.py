#!/usr/bin/env python3
# Copyright (c) 2017 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.test_particl import isclose
from test_framework.util import *

class PosTest(ParticlTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4
        self.extra_args = [ ['-debug','-noacceptnonstdtxn'] for i in range(self.num_nodes)]

    def setup_network(self, split=False):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args)
        self.start_nodes()

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
        ro = node1.reservebalance(True, 10000000)

        ro = node.extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')
        assert(ro['account_id'] == 'aaaZf2qnNr5T7PWRmqgmusuu5ACnBcX2ev')

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
        ro = node.reservebalance(True, 10000000)

        oRoot1 = node1.mnemonic("new")
        ro = node1.extkeyimportmaster(oRoot1['master'])

        addrTo = node1.getnewaddress()

        txnHash = node.sendtoaddress(addrTo, 10)
        ro = node.getmempoolentry(txnHash)
        assert(ro['height'] == 1)

        ro = node.listtransactions()
        fPass = False
        for txl in ro:
            if txl['address'] == addrTo and txl['amount'] == -10 and txl['category'] == 'send':
                fPass = True
                break
        assert(fPass), "node0, listtransactions failed."


        assert(self.wait_for_mempool(node1, txnHash))

        ro = node1.listtransactions()
        assert(len(ro) == 1)
        assert(ro[0]['address'] == addrTo)
        assert(ro[0]['amount'] == 10)
        assert(ro[0]['category'] == 'receive')

        self.stakeBlocks(1)
        block2_hash = node.getblockhash(2)
        ro = node.getblock(block2_hash)
        assert(txnHash in ro['tx'])


        addrReward = node.getnewaddress()
        print('addrReward', addrReward)

        ro = node.walletsettings('stakingoptions', {'rewardaddress':addrReward})
        assert(ro['stakingoptions']['rewardaddress'] == addrReward)

        self.stakeBlocks(1)
        block3_hash = node.getblockhash(3)
        ro = node.getblock(block3_hash)
        print(json.dumps(ro, indent=4, default=self.jsonDecimal))

        coinstakehash = ro['tx'][0]
        ro = node.getrawtransaction(coinstakehash, True)
        print(json.dumps(ro, indent=4, default=self.jsonDecimal))

        fFound = False

        for vout in ro["vout"]:
            try:
                addr0 = vout['scriptPubKey']['addresses'][0]
            except:
                continue
            if addr0 == addrReward:
                fFound = True
                assert(vout['valueSat'] == 39637)
                break
        assert(fFound)


        #assert(False)
        #print(json.dumps(ro, indent=4, default=self.jsonDecimal))


if __name__ == '__main__':
    PosTest().main()
