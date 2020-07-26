#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.messages import COIN

class AssetTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.rpc_timeout = 240
        self.extra_args = [['-assetindex=1'],['-assetindex=1']]

    def run_test(self):
        self.nodes[0].generate(200)
        self.basic_asset()
        self.asset_description_too_big()
        self.asset_maxsupply()

    def basic_asset(self):
        asset = self.nodes[0].assetnew('1', 'TST', 'asset description', '0x', 8, 1000*COIN, 10000*COIN, 31, {})['asset_guid']
        self.sync_mempools()
        self.nodes[1].generate(3)
        self.sync_blocks()
        assetInfo = self.nodes[0].assetinfo(asset)
        assert_equal(assetInfo['asset_guid'], asset)
        assetInfo = self.nodes[1].assetinfo(asset)
        assert_equal(assetInfo['asset_guid'], asset)

    def asset_description_too_big(self):
        # 494 bytes long
        gooddata = "SfsddfdfsdsdffsdfdfsdsfDsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfddSfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdssfsddSfdddssfsddSfdddSfddas"
        # 495 bytes long (makes 513 with the pubdata overhead)
        baddata = gooddata + "a"
        asset = self.nodes[0].assetnew('1', 'TST', gooddata, '0x', 8, 1000*COIN, 10000*COIN, 31, {})['asset_guid']
        asset1 = self.nodes[0].assetnew('1', 'TST', gooddata, '0x', 8, 1000*COIN, 10000*COIN, 31, {})['asset_guid']
        self.nodes[0].generate(1)
        self.sync_blocks()
        assetInfo = self.nodes[0].assetinfo(asset)
        assert_equal(assetInfo['asset_guid'], asset)
        assetInfo = self.nodes[1].assetinfo(asset)
        assert_equal(assetInfo['asset_guid'], asset)
        assetInfo = self.nodes[0].assetinfo(asset1)
        assert_equal(assetInfo['asset_guid'], asset1)
        assetInfo = self.nodes[1].assetinfo(asset1)
        assert_equal(assetInfo['asset_guid'], asset1)
        # data too big
        assert_raises_rpc_error(-4, 'asset-pubdata-too-big', self.nodes[0].assetnew, '1', 'TST', baddata, '0x', 8, 1000*COIN, 10000*COIN, 31, {})
    
    def asset_maxsupply(self):
        # 494 bytes long (512 with overhead)
        gooddata = "SfsddfdfsdsdffsdfdfsdsfDsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfddSfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdssfsddSfdddssfsddSfdddSfddas"
        asset = self.nodes[0].assetnew('1', 'TST', gooddata, '0x', 8, 1*COIN, 1*COIN, 31, {})['asset_guid']
        self.nodes[0].generate(1)
        # cannot increase supply
        assert_raises_rpc_error(-4, 'asset-invalid-supply', self.nodes[0].assetupdate, asset, '', '', int(0.1*COIN), 31, {})
        asset = self.nodes[0].assetnew('1', 'TST', gooddata, '0x', 8, 1*COIN, 2*COIN, 31, {})['asset_guid']
        self.nodes[0].generate(1)
        self.nodes[0].assetupdate(asset, '', '', int(0.1*COIN), 31, {})
        self.nodes[0].generate(1)
        self.nodes[0].assetupdate(asset, '', '', int(0.9*COIN), 31, {})
        self.nodes[0].generate(1)
        # would go over 2 coins supply
        assert_raises_rpc_error(-4, 'asset-invalid-supply', self.nodes[0].assetupdate, asset, '', '', int(0.1*COIN), 31, {})
        self.nodes[0].generate(1)
        # balance > max supply
        assert_raises_rpc_error(-4, 'asset-invalid-supply', self.nodes[0].assetnew, '1', 'TST', gooddata, '0x', 8, 2*COIN, 1*COIN, 31, {})
        # uint64 limits
        asset = self.nodes[0].assetnew('1', 'TST', gooddata, '0x', 8, 18446744073709551614, 18446744073709551615, 31, {})['asset_guid']
        self.nodes[0].generate(1)
        assetInfo = self.nodes[0].assetinfo(asset)
        assert_equal(assetInfo['asset_guid'], asset)
        assert_equal(assetInfo['total_supply'], 18446744073709551614)
        assert_equal(assetInfo['balance'], 18446744073709551614)
        assert_equal(assetInfo['max_supply'], 18446744073709551615)
        self.nodes[0].assetupdate(asset, '', '', 1, 31, {})
        self.nodes[0].generate(1)
        assetInfo = self.nodes[0].assetinfo(asset)
        assert_equal(assetInfo['asset_guid'], asset)
        assert_equal(assetInfo['total_supply'], 18446744073709551615)
        assert_equal(assetInfo['balance'], 18446744073709551615)
        assert_equal(assetInfo['max_supply'], 18446744073709551615)
        assert_raises_rpc_error(-4, 'asset-invalid-supply', self.nodes[0].assetupdate, asset, '', '', 1, 31, {})
        asset = self.nodes[0].assetnew('1', 'TST', gooddata, '0x', 8, 18446744073709551615, 18446744073709551616, 31, {})
        asset = self.nodes[0].assetnew('1', 'TST', gooddata, '0x', 8, 18446744073709551616, 18446744073709551616, 31, {})
        
if __name__ == '__main__':
    AssetTest().main()
