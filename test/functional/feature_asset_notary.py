#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.messages import COIN

class AssetNotaryTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.rpc_timeout = 240
        self.extra_args = [['-assetindex=1'],['-assetindex=1']]

    def run_test(self):
        self.nodes[0].generate(200)
        self.basic_asset()
        self.nodes[0].assetsend(self.asset, self.nodes[0].getnewaddress(), int(2*COIN))
        self.nodes[0].generate(1)
        # will give back hex because notarization doesn't happen in assetallocationsend
        hextx = self.nodes[0].assetallocationsend(self.asset, self.nodes[0].getnewaddress(), int(0.4*COIN))['hex']
        sighash = self.nodes[0].getnotarysighash(hextx)
        notarysig = self.nodes[0].signhash(self.notary_address, sighash)
        hextx_notarized = self.nodes[0].assettransactionnotarize(hextx, notarysig)['hex']
        tx_resigned = self.nodes[0].signrawtransactionwithwallet(hextx_notarized)['hex']
        assert_equal(len(hextx), len(hextx_notarized))
        assert(hextx != hextx_notarized)
        assert_equal(len(tx_resigned), len(hextx_notarized))
        assert(tx_resigned != hextx_notarized)
         # cannot send without notarization
        assert_raises_rpc_error(-26, 'assetallocation-notary-sig', self.nodes[0].sendrawtransaction, hextx)
        assert_raises_rpc_error(-26, 'non-mandatory-script-verify-flag', self.nodes[0].sendrawtransaction, hextx_notarized)
        self.nodes[0].sendrawtransaction(tx_resigned)
        self.nodes[0].generate(1)

    def basic_asset(self):
        self.notary_address = self.nodes[0].getnewaddress()
        notary = {'e': 'https://jsonplaceholder.typicode.com/posts/', 'it': True, 'rx': True}
        self.asset = self.nodes[0].assetnew('1', 'TST', 'asset description', '0x', 8, 1000*COIN, 10000*COIN, 31, self.notary_address, notary, {})['asset_guid']
        self.sync_mempools()
        self.nodes[0].generate(1)
        self.sync_blocks()
        assetInfo = self.nodes[0].assetinfo(self.asset)
        assert_equal(assetInfo['asset_guid'], self.asset)
        assetInfo = self.nodes[1].assetinfo(self.asset)
        assert_equal(assetInfo['asset_guid'], self.asset)

if __name__ == '__main__':
    AssetNotaryTest().main()
