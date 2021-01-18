#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal

class AssetNFTTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.rpc_timeout = 240
        self.extra_args = [['-assetindex=1'],['-assetindex=1']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.nodes[0].generate(200)
        self.basic_assetnft()

    def GetBaseAssetID(self, nAsset):
        return (nAsset & 0xFFFFFFFF)

    def GetNFTID(self, nAsset):
        return nAsset >> 32

    def CreateAssetID(self, NFTID, nBaseAsset):
        return (NFTID << 32) | nBaseAsset

    def basic_assetnft(self):
        asset = self.nodes[0].assetnew('1', 'NFT', 'asset nft description', '0x', 8, 10000, 127, '', {}, {})['asset_guid']
        self.sync_mempools()
        self.nodes[1].generate(3)
        self.sync_blocks()
        self.nodes[0].assetsend(asset, self.nodes[1].getnewaddress(), 1.1, 1)
        self.nodes[0].generate(3)
        self.sync_blocks()
        nftGuid = self.CreateAssetID(1, asset)
        out = self.nodes[1].listunspent(query_options={'assetGuid': nftGuid, 'minimumAmountAsset': 1.1})
        assert_equal(len(out), 1)

if __name__ == '__main__':
    AssetNFTTest().main()
