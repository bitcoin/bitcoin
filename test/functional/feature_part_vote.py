#!/usr/bin/env python3
# Copyright (c) 2017 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.util import *

class VoteTest(ParticlTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [ ['-debug','-noacceptnonstdtxn'] for i in range(self.num_nodes)]

    def setup_network(self, split=False):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args)
        self.start_nodes()

        connect_nodes_bi(self.nodes, 0, 1)
        connect_nodes_bi(self.nodes, 0, 2)
        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        nodes = self.nodes

        # Stop staking
        for i in range(len(nodes)):
            nodes[i].reservebalance(True, 10000000)

        ro = nodes[0].extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')
        assert(ro['account_id'] == 'aaaZf2qnNr5T7PWRmqgmusuu5ACnBcX2ev')
        assert(nodes[0].getwalletinfo()['total_balance'] == 100000)

        ro = nodes[0].setvote(1, 2, 0, 10)
        assert(ro['result'] == 'Voting for option 2 on proposal 1')
        assert(ro['from_height'] == 0)
        assert(ro['to_height'] == 10)

        ro = nodes[0].votehistory()
        assert(len(ro) == 1)
        assert(ro[0]['proposal'] == 1)
        assert(ro[0]['option'] == 2)

        self.stakeBlocks(1)

        ro = nodes[0].tallyvotes(1, 0, 10)
        assert(ro['blocks_counted'] == 1)
        assert(ro['Option 2'] == '1, 100.00%')


        ro = nodes[0].setvote(1, 3, 0, 10)
        assert(ro['result'] == 'Voting for option 3 on proposal 1')
        assert(ro['from_height'] == 0)
        assert(ro['to_height'] == 10)

        ro = nodes[0].votehistory()
        assert(len(ro) == 2)

        self.stakeBlocks(1)

        ro = nodes[0].tallyvotes(1, 0, 10)
        assert(ro['blocks_counted'] == 2)
        assert(ro['Option 3'] == '1, 50.00%')




        #assert(False)
        #print(json.dumps(ro, indent=4, default=self.jsonDecimal))

if __name__ == '__main__':
    VoteTest().main()
