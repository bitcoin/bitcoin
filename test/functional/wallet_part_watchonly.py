#!/usr/bin/env python3
# Copyright (c) 2017 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.util import *
from test_framework.test_particl import isclose

class WalletParticlWatchOnlyTest(ParticlTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4
        self.extra_args = [ ['-debug',] for i in range(self.num_nodes)]

    def setup_network(self, split=False):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args)
        self.start_nodes()

    def run_test (self):
        nodes = self.nodes

        # Stop staking
        for i in range(len(nodes)):
            nodes[i].reservebalance(True, 10000000)

        nodes[0].extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')
        assert(nodes[0].getwalletinfo()['total_balance'] == 100000)

        addr = 'pcwP4hTtaMb7n4urszBTsgxWLdNLU4yNGz'
        nodes[1].importaddress(addr, addr, True)

        ro = nodes[1].getaddressinfo(addr)
        assert(ro['ismine'] == False)
        assert(ro['iswatchonly'] == True)

        ro = nodes[1].getwalletinfo()
        assert(isclose(ro['watchonly_balance'], 10000.0))

        ro = nodes[1].filtertransactions({'include_watchonly': True})
        assert(len(ro) == 1)


if __name__ == '__main__':
    WalletParticlWatchOnlyTest().main()
