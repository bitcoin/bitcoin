#!/usr/bin/env python3
from test_framework.test_framework import BitcoinTestFramework

class GetNetworkInfoTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        info = self.nodes[0].getnetworkinfo()
        assert 'version' in info
        assert 'subversion' in info
        assert 'connections' in info
        print("getnetworkinfo test passed!")

if __name__ == '__main__':
    GetNetworkInfoTest().main()
