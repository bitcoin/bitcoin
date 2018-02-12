#!/usr/bin/env python3
# Copyright (c) 2017 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.util import *

class ForkTest(ParticlTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 6
        self.extra_args = [ ['-debug',] for i in range(self.num_nodes)]

    def setup_network(self, split=False):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args)
        self.start_nodes()

        connect_nodes_bi(self.nodes, 0, 1)
        connect_nodes_bi(self.nodes, 0, 2)
        connect_nodes_bi(self.nodes, 1, 2)

        connect_nodes_bi(self.nodes, 3, 4)
        connect_nodes_bi(self.nodes, 3, 5)
        connect_nodes_bi(self.nodes, 4, 5)


        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        nodes = self.nodes

        # stop staking
        nodes[0].reservebalance(True, 10000000)
        nodes[3].reservebalance(True, 10000000)

        ro = nodes[0].extkeyimportmaster('pact mammal barrel matrix local final lecture chunk wasp survey bid various book strong spread fall ozone daring like topple door fatigue limb olympic', '', 'true')
        ro = nodes[0].getnewextaddress('lblExtTest')
        assert(ro == 'pparszNetDqyrvZksLHJkwJGwJ1r9JCcEyLeHatLjerxRuD3qhdTdrdo2mE6e1ewfd25EtiwzsECooU5YwhAzRN63iFid6v5AQn9N5oE9wfBYehn')

        ro = nodes[0].scanchain()
        assert(nodes[0].getwalletinfo()['total_balance'] == 25000)


        ro = nodes[3].extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')
        assert(ro['account_id'] == 'aaaZf2qnNr5T7PWRmqgmusuu5ACnBcX2ev')
        assert(nodes[3].getwalletinfo()['total_balance'] == 100000)


        # start staking
        nBlocksShorterChain = 2
        nBlocksLongerChain = 5

        ro = nodes[0].walletsettings('stakelimit', {'height':nBlocksShorterChain})
        ro = nodes[3].walletsettings('stakelimit', {'height':nBlocksLongerChain})
        ro = nodes[0].reservebalance(False)
        ro = nodes[3].reservebalance(False)


        self.wait_for_height(nodes[0], nBlocksShorterChain, 1000)

        # stop group1 from staking
        ro = nodes[0].reservebalance(True, 10000000)


        self.wait_for_height(nodes[3], nBlocksLongerChain, 2000)

        # stop group2 from staking
        ro = nodes[3].reservebalance(True, 10000000)

        node0_chain = []
        for k in range(1, nBlocksLongerChain+1):
            try:
                ro = nodes[0].getblockhash(k)
            except JSONRPCException as e:
                assert('Block height out of range' in e.error['message'])
                ro = ''
            node0_chain.append(ro)
            print('node0 ',k, " - ", ro)

        node3_chain = []
        for k in range(1, 6):
            ro = nodes[3].getblockhash(k)
            node3_chain.append(ro)
            print('node3 ',k, ' - ', ro)


        # connect groups
        connect_nodes_bi(self.nodes, 0, 3)

        fPass = False
        for i in range(15):
            time.sleep(2)

            fPass = True
            for k in range(1, nBlocksLongerChain+1):
                try:
                    ro = nodes[0].getblockhash(k)
                except JSONRPCException as e:
                    assert('Block height out of range' in e.error['message'])
                    ro = ''
                if not ro == node3_chain[k]:
                    fPass = False
                    break
            if fPass:
                break
        #assert(fPass)


        node0_chain = []
        for k in range(1, nBlocksLongerChain+1):
            try:
                ro = nodes[0].getblockhash(k)
            except JSONRPCException as e:
                assert('Block height out of range' in e.error['message'])
                ro = ''
            node0_chain.append(ro)
            print('node0 ',k, ' - ', ro)


        ro = nodes[0].getblockchaininfo()
        assert(ro['blocks'] == 5)

        ro = nodes[3].getblockchaininfo()
        assert(ro['blocks'] == 5)

        #assert(False)
        #print(json.dumps(ro, indent=4, default=self.jsonDecimal))


if __name__ == '__main__':
    ForkTest().main()
