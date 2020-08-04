#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.messages import COIN

class AssetAuxFeesTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.rpc_timeout = 240
        self.extra_args = [['-assetindex=1'],['-assetindex=1']]

    def run_test(self):
        self.nodes[0].generate(200)
        self.basic_asset()
        self.nodes[0].assetsend(self.asset, self.nodes[0].getnewaddress(), int(1000*COIN))
        self.nodes[0].assetallocationsend(self.asset, self.nodes[0].getnewaddress(), int(250*COIN))
        self.sync_mempools()
        out =  self.nodes[1].listunspent(minconf=0, query_options={'assetGuid': self.asset})
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], self.asset)
        assert_equal(out[0]['asset_amount'], 106000000)
        # remove aux fees
        self.nodes[0].assetupdate(self.asset, '', '', 0, 31, '', {}, {})
        self.nodes[0].assetallocationsend(self.asset, self.nodes[0].getnewaddress(), int(250*COIN))
        self.sync_mempools()
        out =  self.nodes[1].listunspent(minconf=0, query_options={'assetGuid': self.asset})
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], self.asset)
        assert_equal(out[0]['asset_amount'], 106000000)
        self.nodes[0].generate(3)
        self.sync_blocks()

    def basic_asset(self):
        newaddressfee = self.nodes[1].getnewaddress()
        auxfees = {'address': newaddressfee, 'fee_struct': [[0,'0.01'],[int(10*COIN),'0.004'],[int(250*COIN),'0.002'],[int(2500*COIN),'0.0007'],[int(25000*COIN),'0.00007'],[int(250000*COIN),'0']]}
        self.asset = self.nodes[0].assetnew('1', 'AGX', 'AGX silver backed token, licensed and operated by Interfix corporation', '0x', 8, 1000*COIN, 10000*COIN, 63, '', {}, auxfees)['asset_guid']
        self.sync_mempools()
        self.nodes[0].generate(3)
        self.sync_blocks()
        assetInfo = self.nodes[0].assetinfo(self.asset)
        assert_equal(assetInfo['asset_guid'], self.asset)
        assetInfo = self.nodes[1].assetinfo(self.asset)
        assert_equal(assetInfo['asset_guid'], self.asset)
  
if __name__ == '__main__':
    AssetAuxFeesTest().main()
