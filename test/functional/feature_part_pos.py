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
        nodes = self.nodes

        # stop staking
        for i in range(len(nodes)):
            nodes[i].reservebalance(True, 10000000)

        ro = nodes[0].extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')
        assert(ro['account_id'] == 'aaaZf2qnNr5T7PWRmqgmusuu5ACnBcX2ev')
        assert(nodes[0].getwalletinfo()['total_balance'] == 100000)

        nodes[1].extkeyimportmaster(nodes[1].mnemonic('new')['master'])
        nodes[2].extkeyimportmaster('sección grito médula hecho pauta posada nueve ebrio bruto buceo baúl mitad')


        addrTo256 = nodes[2].getnewaddress('256 test', 'True', 'False', 'True')
        assert(addrTo256 == 'tpl16a6gjrpfwkqrf8fveajkek07l6a0pxgaayk4y6gyq9zlkxxk2hqqmld6tr')
        [nodes[0].sendtoaddress(addrTo256, 1000) for i in range(4)]


        # test reserve balance
        ro = nodes[0].walletsettings('stakelimit', {'height':1})
        ro = nodes[0].getwalletinfo()
        assert(isclose(ro['reserve'], 10000000.0))

        ro = nodes[0].reservebalance(True, 100)
        assert(ro['reserve'] == True)
        assert(isclose(ro['amount'], 100.0))

        ro = nodes[0].getwalletinfo()
        assert(ro['reserve'] == 100)

        ro = nodes[0].reservebalance(False)
        assert(ro['reserve'] == False)
        assert(ro['amount'] == 0)

        ro = nodes[0].getwalletinfo()
        assert(ro['reserve'] == 0)

        assert(self.wait_for_height(nodes[0], 1))
        ro = nodes[0].reservebalance(True, 10000000)

        addrTo = nodes[1].getnewaddress()
        txnHash = nodes[0].sendtoaddress(addrTo, 10)
        ro = nodes[0].getmempoolentry(txnHash)
        assert(ro['height'] == 1)

        ro = nodes[0].listtransactions()
        fPass = False
        for txl in ro:
            if txl['address'] == addrTo and txl['amount'] == -10 and txl['category'] == 'send':
                fPass = True
                break
        assert(fPass), "node0, listtransactions failed."


        assert(self.wait_for_mempool(nodes[1], txnHash))

        ro = nodes[1].listtransactions()
        assert(len(ro) == 1)
        assert(ro[0]['address'] == addrTo)
        assert(ro[0]['amount'] == 10)
        assert(ro[0]['category'] == 'receive')

        self.stakeBlocks(1)
        block2_hash = nodes[0].getblockhash(2)
        ro = nodes[0].getblock(block2_hash)
        assert(txnHash in ro['tx'])


        addrReward = nodes[0].getnewaddress()
        ro = nodes[0].walletsettings('stakingoptions', {'rewardaddress':addrReward})
        assert(ro['stakingoptions']['rewardaddress'] == addrReward)

        self.stakeBlocks(1)
        block3_hash = nodes[0].getblockhash(3)
        coinstakehash = nodes[0].getblock(block3_hash)['tx'][0]
        ro = nodes[0].getrawtransaction(coinstakehash, True)

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

        # Test staking pkh256 outputs
        ro = nodes[2].walletsettings('stakelimit', {'height':1})
        ro = nodes[2].reservebalance(False)
        ro = nodes[2].getstakinginfo()
        assert(ro['weight'] == 400000000000)

        self.stakeBlocks(1, nStakeNode=2)


        addrRewardExt = nodes[0].getnewextaddress()
        ro = nodes[0].walletsettings('stakingoptions', {'rewardaddress':addrRewardExt})
        assert(ro['stakingoptions']['rewardaddress'] == addrRewardExt)
        self.stakeBlocks(1)
        block5_hash = nodes[0].getblockhash(5)
        coinstakehash = nodes[0].getblock(block5_hash)['tx'][0]
        ro = nodes[0].getrawtransaction(coinstakehash, True)

        fFound = False
        for vout in ro["vout"]:
            try:
                addr0 = vout['scriptPubKey']['addresses'][0]
                ro = nodes[0].validateaddress(addr0)
                if ro['from_ext_address_id'] == 'xXZRLYvJgbJyrqJhgNzMjEvVGViCdGmVAt':
                    assert(addr0 == 'pgaKYsNmHTuQB83FguN44WW4ADKmwJwV7e')
                    fFound = True
                    assert(vout['valueSat'] == 39637)
            except:
                continue
        assert(fFound)



        addrRewardSx = nodes[0].getnewstealthaddress()
        ro = nodes[0].walletsettings('stakingoptions', {'rewardaddress':addrRewardSx})
        assert(ro['stakingoptions']['rewardaddress'] == addrRewardSx)
        self.stakeBlocks(1)
        block6_hash = nodes[0].getblockhash(6)
        coinstakehash = nodes[0].getblock(block6_hash)['tx'][0]
        ro = nodes[0].getrawtransaction(coinstakehash, True)

        fFound = False
        for vout in ro["vout"]:
            try:
                addr0 = vout['scriptPubKey']['addresses'][0]
                ro = nodes[0].validateaddress(addr0)
                if ro['from_stealth_address'] == addrRewardSx:
                    fFound = True
                    assert(vout['valueSat'] == 39637)
            except:
                continue
        assert(fFound)


        #assert(False)
        #print(json.dumps(ro, indent=4, default=self.jsonDecimal))


if __name__ == '__main__':
    PosTest().main()
