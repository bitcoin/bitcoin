#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.messages import COIN

class AssetBurnTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.rpc_timeout = 240
        self.extra_args = [['-assetindex=1'],['-assetindex=1']]

    def run_test(self):
        self.nodes[0].generate(200)
        self.sync_blocks()
        self.basic_burn_asset()
        self.basic_burn_asset_multiple()
    
    def basic_burn_asset(self):
        self.basic_asset(None)
        self.nodes[0].generate(1)
        self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1)
        self.nodes[0].assetsend(self.asset, self.nodes[1].getnewaddress(), int(0.5*COIN))
        self.nodes[0].generate(1)
        self.sync_blocks()
        out =  self.nodes[1].listunspent(query_options={'assetGuid': self.asset})
        assert_equal(len(out), 1)
        # try to burn more than we own
        assert_raises_rpc_error(-4, "Insufficient funds", self.nodes[1].assetallocationburn, self.asset, int(0.6*COIN), "0x931d387731bbbc988b312206c74f77d004d6b84b")
        self.nodes[1].assetallocationburn(self.asset, int(0.5*COIN), "0x931d387731bbbc988b312206c74f77d004d6b84b")
        self.nodes[0].generate(1)
        self.sync_blocks()
        out =  self.nodes[1].listunspent(query_options={'assetGuid': self.asset})
        assert_equal(len(out), 0)

    def basic_burn_asset_multiple(self):
        # SYSX guid on regtest is 123456
        self.basic_asset(123456)
        self.nodes[0].generate(1)
        self.sync_blocks()
        self.nodes[0].assetsend(self.asset, self.nodes[1].getnewaddress(), COIN)
        self.nodes[0].generate(1)
        self.sync_blocks()
        out =  self.nodes[1].listunspent(query_options={'assetGuid': self.asset})
        assert_equal(len(out), 1)
        # burn 0.4 + 0.5 + 0.05
        prebalance = float(self.nodes[1].getbalance())
        self.nodes[1].assetallocationburn(self.asset, int(0.4*COIN), "")
        self.nodes[1].assetallocationburn(self.asset, int(0.5*COIN), "")
        self.nodes[1].assetallocationburn(self.asset, int(0.05*COIN), "")
        postbalance = float(self.nodes[1].getbalance())
        # ensure SYS balance goes up on burn to syscoin, use 0.94 because of fees
        assert(prebalance + 0.94 <= postbalance)
        out =  self.nodes[1].listunspent(minconf=0, query_options={'assetGuid': self.asset})
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], 123456)
        assert_equal(out[0]['asset_amount'], int(0.05*COIN))
        # in mempool, create more allocations and burn them all accumulating the coins
        self.nodes[1].assetallocationsend(self.asset, self.nodes[1].getnewaddress(), int(0.01*COIN))
        self.nodes[1].assetallocationsend(self.asset, self.nodes[1].getnewaddress(), int(0.02*COIN))
        self.nodes[1].assetallocationsend(self.asset, self.nodes[1].getnewaddress(), int(0.005*COIN))
        self.nodes[1].assetallocationsend(self.asset, self.nodes[1].getnewaddress(), int(0.005*COIN))
        self.nodes[1].assetallocationsend(self.asset, self.nodes[1].getnewaddress(), int(0.01*COIN))
        self.nodes[1].assetallocationburn(self.asset, int(0.05*COIN), "0x931d387731bbbc988b312206c74f77d004d6b84b")
        out =  self.nodes[1].listunspent(minconf=0, query_options={'assetGuid': self.asset})
        assert_equal(len(out), 0)
        self.nodes[0].generate(1)
        self.sync_blocks()
        # check over block too
        out =  self.nodes[1].listunspent(query_options={'assetGuid': self.asset})
        assert_equal(len(out), 0)

    def basic_asset(self, guid):
        if guid is None:
            self.asset = self.nodes[0].assetnew('1', "TST", "asset description", "0x9f90b5093f35aeac5fbaeb591f9c9de8e2844a46", 8, 1000*COIN, 10000*COIN, 31, '', '', {})['asset_guid']
        else:
            self.asset = self.nodes[0].assetnewtest(guid, '1', "TST", "asset description", "0x9f90b5093f35aeac5fbaeb591f9c9de8e2844a46", 8, 1000*COIN, 10000*COIN, 31, '', '', {})['asset_guid']


if __name__ == '__main__':
    AssetBurnTest().main()
