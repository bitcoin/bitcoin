#!/usr/bin/env python3
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test txindex generation and fetching
#

from test_framework.test_particl import ParticlTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *

class TxIndexTest(ParticlTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4
        self.extra_args = [
            # Nodes 0/1 are "wallet" nodes
            ['-debug',],
            ['-debug','-txindex'],
            # Nodes 2/3 are used for testing
            ['-debug','-txindex'],
            ['-debug','-txindex'],]

    def setup_network(self):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args)
        self.start_nodes()

        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[0], 2)
        connect_nodes(self.nodes[0], 3)

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


        print('Testing transaction index...')

        ro = nodes[1].extkeyimportmaster('graine article givre hublot encadrer admirer stipuler capsule acajou paisible soutirer organe')
        addr1 = nodes[1].getnewaddress()

        txid = nodes[0].sendtoaddress(addr1, 5)

        # Check verbose raw transaction results
        verbose = self.nodes[0].getrawtransaction(txid, 1)
        print(json.dumps(verbose, indent=4, default=self.jsonDecimal))
        assert(len(verbose['vout']) == 2)

        str0 = json.dumps(verbose['vout'][0], indent=4, default=self.jsonDecimal)
        str1 = json.dumps(verbose['vout'][1], indent=4, default=self.jsonDecimal)
        if addr1 in str0:
            assert_equal(verbose['vout'][0]['valueSat'], 500000000);
            assert_equal(verbose['vout'][0]['value'], 5);
        elif addr1 in str1:
            assert_equal(verbose['vout'][1]['valueSat'], 500000000);
            assert_equal(verbose['vout'][1]['value'], 5);
        else:
            assert(False)

        print('Passed\n')


if __name__ == '__main__':
    TxIndexTest().main()
