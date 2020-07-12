#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error, disconnect_nodes, connect_nodes


class AssetReOrgTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [['-assetindex=1'],['-assetindex=1'],['-assetindex=1']]

    def run_test(self):
        self.nodes[0].generate(1)
        self.sync_blocks()
        self.nodes[2].generate(200)
        self.sync_blocks()
        disconnect_nodes(self.nodes[0], 1)
        disconnect_nodes(self.nodes[0], 2)
        self.basic_asset()
        # create fork
        self.nodes[0].generate(11)
        # won't exist on node 0 because it was created on node 3 and we are disconnected
        assert_raises_rpc_error(-4, 'asset not found', self.nodes[0].assetinfo, self.asset)
        self.nodes[2].generate(10)
        assetInfo = self.nodes[2].assetinfo(self.asset)
        assert_equal(assetInfo['asset_guid'], self.asset)
        # still won't exist on node 0 yet
        assert_raises_rpc_error(-4, 'asset not found', self.nodes[0].assetinfo, self.asset)
        # connect and sync to longest chain now which does not include the asset
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[0], 2)
        self.sync_all()
        assert_raises_rpc_error(-4, 'asset not found', self.nodes[0].assetinfo, self.asset)
        assert_raises_rpc_error(-4, 'asset not found', self.nodes[1].assetinfo, self.asset)
        assert_raises_rpc_error(-4, 'asset not found', self.nodes[2].assetinfo, self.asset)
        self.nodes[0].generate(1)
        self.sync_blocks()
        # asset is there now
        assetInfo = self.nodes[0].assetinfo(self.asset)
        assert_equal(assetInfo['asset_guid'], self.asset)
        assetInfo = self.nodes[1].assetinfo(self.asset)
        assert_equal(assetInfo['asset_guid'], self.asset)
        assetInfo = self.nodes[2].assetinfo(self.asset)
        assert_equal(assetInfo['asset_guid'], self.asset)

    def basic_asset(self):
        self.asset = self.nodes[2].assetnew('1', 'TST', 'asset description', '0x', 8, '1000', '10000', 31, {})['asset_guid']

if __name__ == '__main__':
    AssetReOrgTest().main()
