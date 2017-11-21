#!/usr/bin/env python3
# Copyright (c) 2017 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.test_particl import isclose
from test_framework.util import *


class SmsgDevTest(ParticlTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [ ['-debug','-noacceptnonstdtxn'] for i in range(self.num_nodes) ]

    def setup_network(self, split=False):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args)
        self.start_nodes()
        connect_nodes(self.nodes[0], 1)

        self.is_network_split = False
        self.sync_all()


    def run_test (self):
        tmpdir = self.options.tmpdir
        nodes = self.nodes

        # Stop staking
        ro = nodes[0].reservebalance(True, 10000000)
        ro = nodes[1].reservebalance(True, 10000000)

        ro = nodes[0].mnemonic('new');
        roImport0 = nodes[0].extkeyimportmaster(ro['master'])

        roImport1 = nodes[1].extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')

        address0 = nodes[0].getnewaddress() # will be different each run
        address1 = nodes[1].getnewaddress()
        assert(address1 == 'pX9N6S76ZtA5BfsiJmqBbjaEgLMHpt58it')

        ro = nodes[0].smsglocalkeys()
        assert(len(ro['keys']) == 0)

        ro = nodes[0].smsgaddlocaladdress(address0)
        print(json.dumps(ro, indent=4, default=self.jsonDecimal))

        ro = nodes[0].smsglocalkeys()
        assert(len(ro['keys']) == 1)


        ro = nodes[1].smsgaddkey(address0, ro['keys'][0]['public_key'])
        assert(ro['result'] == 'Public key added to db.')

        ro = nodes[1].smsgsend(address1, address0, "['data':'test','value':1]", True, 4, True)
        print(json.dumps(ro, indent=4, default=self.jsonDecimal))
        assert(ro['result'] == 'Not Sent.')
        assert(isclose(ro['fee'], 0.00085800))



        ro = nodes[1].smsgsend(address1, address0, "['data':'test','value':1]", True, 4)
        print(json.dumps(ro, indent=4, default=self.jsonDecimal))

        self.stakeBlocks(1, nStakeNode=1)

        self.waitForSmsgExchange(1, 1, 0)

        ro = nodes[0].smsginbox()
        print(json.dumps(ro, indent=4, default=self.jsonDecimal))
        assert(len(ro['messages']) == 1)

        #assert(False)
        #print(json.dumps(ro, indent=4, default=self.jsonDecimal))

if __name__ == '__main__':
    SmsgDevTest().main()
