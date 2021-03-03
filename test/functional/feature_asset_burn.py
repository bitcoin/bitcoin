#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import time
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from decimal import Decimal
class AssetBurnTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.rpc_timeout = 240
        self.extra_args = [['-assetindex=1'],['-assetindex=1']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.nodes[0].generate(200)
        self.sync_blocks()
        self.basic_burn_asset()
        self.basic_burn_asset_multiple()
        self.two_way_burn()

    def basic_burn_asset(self):
        self.basic_asset(None)
        self.nodes[0].generate(1)
        self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1)
        self.nodes[0].assetsend(self.asset, self.nodes[1].getnewaddress(), 0.5)
        self.nodes[0].generate(1)
        self.sync_blocks()
        out =  self.nodes[1].listunspentasset(self.asset)
        assert_equal(len(out), 1)
        # try to burn more than we own
        assert_raises_rpc_error(-4, "Insufficient funds", self.nodes[1].assetallocationburn, self.asset, 0.6, "0x931d387731bbbc988b312206c74f77d004d6b84b")
        self.nodes[1].assetallocationburn(self.asset, 0.5, "0x931d387731bbbc988b312206c74f77d004d6b84b")
        self.nodes[0].generate(1)
        self.sync_blocks()
        out =  self.nodes[1].listunspentasset(self.asset)
        assert_equal(len(out), 0)

    def two_way_burn(self):
        # burn SYS to SYSX and back
        self.nodes[0].generate(1)
        self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 100)
        self.nodes[0].generate(1)
        self.sync_blocks()
        balancebefore = self.nodes[1].getbalance()
        txid = self.nodes[1].syscoinburntoassetallocation(self.asset, 50)['txid']
        self.sync_mempools()
        fee = self.nodes[1].gettransaction(txid)['fee']
        assert_equal(self.nodes[1].getbalance(), (balancebefore + fee) - 50)
        self.sync_mempools()
        self.nodes[0].generate(1)
        self.sync_blocks()
        out = self.nodes[1].listunspentasset(self.asset)
        assert_equal(len(out), 1)
        # try to burn more than we own
        assert_raises_rpc_error(-4, "Insufficient funds", self.nodes[1].assetallocationburn, self.asset, 55, "")
        # burn part of it
        balancebefore = self.nodes[1].getbalance()
        txid = self.nodes[1].assetallocationburn(self.asset, 22)['txid']
        self.sync_mempools()
        self.nodes[0].generate(1)
        self.sync_blocks()
        fee = self.nodes[1].gettransaction(txid)['fee']
        assert_equal(self.nodes[1].getbalance(), (balancebefore + fee) + 22)
        out = self.nodes[1].listunspentasset(self.asset)
        assert_equal(len(out), 1)
        # burn the rest
        balancebefore = self.nodes[1].getbalance()
        txid = self.nodes[1].assetallocationburn(self.asset, 28)['txid']
        self.sync_mempools()
        self.nodes[0].generate(1)
        self.sync_blocks()
        fee = self.nodes[1].gettransaction(txid)['fee']
        assert_equal(self.nodes[1].getbalance(), (balancebefore + fee) + 28)
        out = self.nodes[1].listunspentasset(self.asset)
        assert_equal(len(out), 0)

    def basic_burn_asset_multiple(self):
        # SYSX guid on regtest is 123456
        self.basic_asset('123456')
        self.nodes[0].generate(1)
        self.sync_blocks()
        self.nodes[0].assetsend(self.asset, self.nodes[1].getnewaddress(), 1)
        self.nodes[0].generate(1)
        self.sync_blocks()
        out =  self.nodes[1].listunspentasset(self.asset)
        assert_equal(len(out), 1)
        # burn 0.4 + 0.5 + 0.05
        prebalance = float(self.nodes[1].getbalance())
        self.nodes[1].assetallocationburn(self.asset, 0.4, "")
        time.sleep(0.25)
        self.nodes[1].assetallocationburn(self.asset, 0.5, "")
        time.sleep(0.25)
        self.nodes[1].assetallocationburn(self.asset, 0.05, "")
        postbalance = float(self.nodes[1].getbalance())
        # ensure SYS balance goes up on burn to syscoin, use 0.94 because of fees
        assert(prebalance + 0.94 <= postbalance)
        out =  self.nodes[1].listunspent(minconf=0, query_options={'assetGuid': self.asset})
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], '123456')
        assert_equal(out[0]['asset_amount'], Decimal('0.05'))
        # in mempool, create more allocations and burn them all accumulating the coins
        self.nodes[1].assetallocationsend(self.asset, self.nodes[1].getnewaddress(), 0.01)
        time.sleep(0.25)
        self.nodes[1].assetallocationsend(self.asset, self.nodes[1].getnewaddress(), 0.02)
        time.sleep(0.25)
        self.nodes[1].assetallocationsend(self.asset, self.nodes[1].getnewaddress(), 0.005)
        time.sleep(0.25)
        self.nodes[1].assetallocationsend(self.asset, self.nodes[1].getnewaddress(), 0.005)
        time.sleep(0.25)
        self.nodes[1].assetallocationsend(self.asset, self.nodes[1].getnewaddress(), 0.01)
        time.sleep(0.25)
        self.nodes[1].assetallocationburn(self.asset, 0.05, "0x931d387731bbbc988b312206c74f77d004d6b84b")
        out =  self.nodes[1].listunspent(minconf=0, query_options={'assetGuid': self.asset})
        assert_equal(len(out), 0)
        self.nodes[0].generate(1)
        self.sync_blocks()
        # check over block too
        out =  self.nodes[1].listunspentasset(self.asset)
        assert_equal(len(out), 0)

    def basic_asset(self, guid):
        if guid is None:
            self.asset = self.nodes[0].assetnew('1', "TST", "asset description", "0x9f90b5093f35aeac5fbaeb591f9c9de8e2844a46", 8, 10000, 127, '', {}, {})['asset_guid']
        else:
            self.asset = self.nodes[0].assetnewtest(guid, '1', "TST", "asset description", "0x9f90b5093f35aeac5fbaeb591f9c9de8e2844a46", 8, 10000, 127, '', {}, {})['asset_guid']


if __name__ == '__main__':
    AssetBurnTest().main()
