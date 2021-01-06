#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

class AssetReOrgTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [['-assetindex=1'],['-assetindex=1'],['-assetindex=1']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.nodes[0].generate(1)
        self.sync_blocks()
        self.nodes[2].generate(200)
        self.sync_blocks()
        self.disconnect_nodes(0, 1)
        self.disconnect_nodes(0, 2)
        self.basic_asset()
        # create fork
        self.nodes[0].generate(11)
        # won't exist on node 0 because it was created on node 2 and we are disconnected
        assert_raises_rpc_error(-20, 'Failed to read from asset DB', self.nodes[0].assetinfo, self.asset)
        self.nodes[2].generate(10)
        assetInfo = self.nodes[2].assetinfo(self.asset)
        assert_equal(assetInfo['asset_guid'], self.asset)
        # still won't exist on node 0 yet
        assert_raises_rpc_error(-20, 'Failed to read from asset DB', self.nodes[0].assetinfo, self.asset)
        # connect and sync to longest chain now which does not include the asset
        self.connect_nodes(0, 1)
        self.connect_nodes(0, 2)
        self.sync_blocks()
        assert_raises_rpc_error(-20, 'Failed to read from asset DB', self.nodes[0].assetinfo, self.asset)
        assert_raises_rpc_error(-20, 'Failed to read from asset DB', self.nodes[1].assetinfo, self.asset)
        assert_raises_rpc_error(-20, 'Failed to read from asset DB', self.nodes[2].assetinfo, self.asset)
        # node 2 should have the asset in mempool again
        self.nodes[2].generate(1)
        self.sync_blocks()
        # asset is there now
        assetInfo = self.nodes[0].assetinfo(self.asset)
        assert_equal(assetInfo['asset_guid'], self.asset)
        assetInfo = self.nodes[1].assetinfo(self.asset)
        assert_equal(assetInfo['asset_guid'], self.asset)
        assetInfo = self.nodes[2].assetinfo(self.asset)
        assert_equal(assetInfo['asset_guid'], self.asset)
        # increase total supply
        self.nodes[2].generate(1)
        self.sync_blocks()
        self.nodes[2].assetsend(self.asset, self.nodes[1].getnewaddress(), 100)
        blockhash = self.nodes[2].getbestblockhash()
        self.nodes[2].generate(1)
        self.sync_blocks()
        assetInfo = self.nodes[0].assetinfo(self.asset)
        assert_equal(assetInfo['asset_guid'], self.asset)
        assert_equal(assetInfo['total_supply'], 100.00000000)
        assetInfo = self.nodes[1].assetinfo(self.asset)
        assert_equal(assetInfo['total_supply'], 100.00000000)
        assert_equal(assetInfo['asset_guid'], self.asset)
        assetInfo = self.nodes[2].assetinfo(self.asset)
        assert_equal(assetInfo['total_supply'], 100.00000000)
        # revert back to before supply was created
        self.nodes[0].invalidateblock(blockhash)
        self.nodes[1].invalidateblock(blockhash)
        self.nodes[2].invalidateblock(blockhash)
        assetInfo = self.nodes[0].assetinfo(self.asset)
        assert_equal(assetInfo['asset_guid'], self.asset)
        assert_equal(assetInfo['total_supply'], 0)
        assetInfo = self.nodes[1].assetinfo(self.asset)
        assert_equal(assetInfo['total_supply'], 0)
        assert_equal(assetInfo['asset_guid'], self.asset)
        assetInfo = self.nodes[2].assetinfo(self.asset)
        assert_equal(assetInfo['total_supply'], 0)
        # back to tip
        # revert back to before supply was created
        self.nodes[0].reconsiderblock(blockhash)
        self.nodes[1].reconsiderblock(blockhash)
        self.nodes[2].reconsiderblock(blockhash)
        assetInfo = self.nodes[0].assetinfo(self.asset)
        assert_equal(assetInfo['asset_guid'], self.asset)
        assert_equal(assetInfo['total_supply'], 100.00000000)
        assetInfo = self.nodes[1].assetinfo(self.asset)
        assert_equal(assetInfo['total_supply'], 100.00000000)
        assert_equal(assetInfo['asset_guid'], self.asset)
        assetInfo = self.nodes[2].assetinfo(self.asset)
        assert_equal(assetInfo['total_supply'], 100.00000000)


    def basic_asset(self):
        self.asset = self.nodes[2].assetnew('1', 'TST', 'asset description', '0x', 8, 10000, 127, '', {}, {})['asset_guid']

if __name__ == '__main__':
    AssetReOrgTest().main()
