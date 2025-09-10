#!/usr/bin/env python3
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class WalletColdStakeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]
        owner = node.getnewaddress()
        staker = node.getnewaddress()
        res = node.delegatestakeaddress(owner, staker)
        cold_addr = res['address']
        script = res['script']
        node.registercoldstakeaddress(cold_addr, script)
        info = node.getaddressinfo(cold_addr)
        assert info['iswatchonly']

if __name__ == '__main__':
    WalletColdStakeTest().main()
