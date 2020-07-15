#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.messages import COIN

class AssetZDAGTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.rpc_timeout = 240
        self.extra_args = [['-assetindex=1'],['-assetindex=1'],['-assetindex=1']]

    def run_test(self):
        self.nodes[0].generate(200)
        self.sync_blocks()
        self.basic_zdag_doublespend()

    def basic_zdag_doublespend(self):
        self.basic_asset()
        self.nodes[0].generate(1)
        newaddress2 = self.nodes[1].getnewaddress()
        newaddress3 = self.nodes[1].getnewaddress()
        newaddress1 = self.nodes[0].getnewaddress()
        self.nodes[2].importprivkey(self.nodes[1].dumpprivkey(newaddress2))
        self.nodes[0].assetsend(self.asset, newaddress1, int(2*COIN))
        self.nodes[0].sendtoaddress(newaddress2, 1)
        self.nodes[0].generate(1)
        self.sync_blocks()
        out =  self.nodes[0].listunspent(query_options={'assetGuid': self.asset, 'minimumAmountAsset': 0.5})
        assert_equal(len(out), 1)
        self.nodes[0].assetallocationsend(self.asset, newaddress2, int(1.5*COIN))
        self.sync_mempool()
        self.nodes[1].assetallocationsend(self.asset, newaddress1, int(0.5*COIN))
        self.sync_mempool()
        self.nodes[0].assetallocationsend(self.asset, newaddress2, int(0.5*COIN))
        # dbl spend
        self.nodes[2].assetallocationsend(self.asset, newaddress3, int(1*COIN))
        self.sync_mempool()
        self.nodes[0].generate(1)
        self.sync_blocks()
        out =  self.nodes[0].listunspent(query_options={'assetGuid': self.asset})
        assert_equal(len(out), 0)

    def basic_asset(self):
        self.asset = self.nodes[0].assetnew('1', "TST", "asset description", "0x9f90b5093f35aeac5fbaeb591f9c9de8e2844a46", 8, 1000*COIN, 10000*COIN, 31, {})['asset_guid']


if __name__ == '__main__':
    AssetZDAGTest().main()
