#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import time
from decimal import Decimal
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
ZDAG_NOT_FOUND = -1
ZDAG_STATUS_OK = 0
ZDAG_WARNING_RBF = 1
ZDAG_WARNING_NOT_ZDAG_TX = 2
ZDAG_WARNING_SIZE_OVER_POLICY = 3
ZDAG_MAJOR_CONFLICT = 4
class AssetZDAGTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4
        self.rpc_timeout = 240
        self.extra_args = [['-assetindex=1'],['-assetindex=1'],['-assetindex=1'],['-assetindex=1']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.nodes[0].generate(200)
        self.sync_blocks()
        self.basic_zdag_doublespend()
        self.burn_zdag_doublespend()
        self.burn_zdag_doublespend_chain()

    def basic_zdag_doublespend(self):
        self.basic_asset(guid=None)
        self.nodes[0].generate(1)
        newaddress2 = self.nodes[1].getnewaddress()
        newaddress1 = self.nodes[0].getnewaddress()
        self.nodes[2].importprivkey(self.nodes[1].dumpprivkey(newaddress2))
        self.nodes[0].assetsend(self.asset, newaddress1, 2)
        # create 2 utxo's so below newaddress1 recipient of 0.5 COIN uses 1 and the newaddress3 recipient on node3 uses the other on dbl spend
        self.nodes[0].sendtoaddress(newaddress2, 1)
        self.nodes[0].sendtoaddress(newaddress2, 1)
        self.nodes[0].generate(1)
        self.sync_blocks()
        out =  self.nodes[0].listunspent(query_options={'assetGuid': self.asset, 'minimumAmountAsset': 0.5})
        assert_equal(len(out), 1)
        out =  self.nodes[2].listunspent()
        assert_equal(len(out), 2)
        # send 2 asset UTXOs to newaddress2 same logic as explained above about dbl spend
        self.nodes[0].assetallocationsend(self.asset, newaddress2, 0.4)
        time.sleep(0.25)
        self.nodes[0].assetallocationsend(self.asset, newaddress2, 1)
        time.sleep(0.25)
        self.nodes[0].generate(1)
        self.sync_blocks()
        # should have 2 sys utxos and 2 asset utxos
        out =  self.nodes[2].listunspent()
        assert_equal(len(out), 4)
        # this will use 1 sys utxo and 1 asset utxo and send it to change address owned by node2
        self.nodes[1].assetallocationsend(self.asset, newaddress1, 0.4, 0, False)
        time.sleep(0.25)
        self.sync_mempools(self.nodes[0:3], timeout=30)
        # node3 should have 2 less utxos because they were sent to change on node2
        out =  self.nodes[2].listunspent(minconf=0)
        assert_equal(len(out), 2)
        time.sleep(0.25)
        tx1 = self.nodes[1].assetallocationsend(self.asset, newaddress1, 1, 0, False)['txid']
        # dbl spend
        tx2 = self.nodes[2].assetallocationsend(self.asset, newaddress1, 0.9, 0, False)['txid']
        time.sleep(0.25)
        # use tx2 to build tx3
        tx3 = self.nodes[2].assetallocationsend(self.asset, newaddress1, 0.05, 0, False)['txid']
        time.sleep(0.25)
        # use tx3 to build tx4
        tx4 = self.nodes[2].assetallocationsend(self.asset, newaddress1, 0.025, 0, False)['txid']
        time.sleep(0.25)
        self.sync_mempools(self.nodes[0:3], timeout=30)
        for i in range(3):
            self.nodes[i].getrawtransaction(tx1)
            self.nodes[i].getrawtransaction(tx2)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx1)['status'], ZDAG_MAJOR_CONFLICT)
            # ensure the tx2 made it to mempool, should propagate dbl-spend first time
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx2)['status'], ZDAG_MAJOR_CONFLICT)
            # will conflict because its using tx2 which is in conflict state
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx3)['status'], ZDAG_MAJOR_CONFLICT)
            # will conflict because its using tx3 which uses tx2 which is in conflict state
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx4)['status'], ZDAG_MAJOR_CONFLICT)
        self.nodes[0].generate(1)
        self.sync_blocks()
        tx2inchain = False
        for i in range(3):
            try:
                self.nodes[i].getrawtransaction(tx1)
            except:
                tx2inchain = True
                continue
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx1)['status'], ZDAG_NOT_FOUND)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx2)['status'], ZDAG_NOT_FOUND)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx3)['status'], ZDAG_NOT_FOUND)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx4)['status'], ZDAG_NOT_FOUND)
            assert_raises_rpc_error(-5, 'No such mempool transaction', self.nodes[i].getrawtransaction, tx2)
            assert_raises_rpc_error(-5, 'No such mempool transaction', self.nodes[i].getrawtransaction, tx3)
            assert_raises_rpc_error(-5, 'No such mempool transaction', self.nodes[i].getrawtransaction, tx4)
        out =  self.nodes[0].listunspent(query_options={'assetGuid': self.asset, 'minimumAmountAsset':0,'maximumAmountAsset':0})
        assert_equal(len(out), 1)
        out =  self.nodes[0].listunspent(query_options={'assetGuid': self.asset, 'minimumAmountAsset':0.4,'maximumAmountAsset':0.4})
        assert_equal(len(out), 1)
        out =  self.nodes[0].listunspent(query_options={'assetGuid': self.asset, 'minimumAmountAsset':0.6,'maximumAmountAsset':0.6})
        assert_equal(len(out), 1)
        out =  self.nodes[0].listunspent(query_options={'assetGuid': self.asset, 'minimumAmountAsset':1.0,'maximumAmountAsset':1.0})
        if tx2inchain is True:
            assert_equal(len(out), 0)
            out =  self.nodes[0].listunspent(query_options={'assetGuid': self.asset})
            assert_equal(len(out), 6)
        else:
            assert_equal(len(out), 1)
            out =  self.nodes[0].listunspent(query_options={'assetGuid': self.asset})
            assert_equal(len(out), 4)

    def burn_zdag_doublespend(self):
        self.basic_asset(guid=None)
        self.nodes[0].generate(1)
        useraddress2 = self.nodes[1].getnewaddress()
        useraddress3 = self.nodes[2].getnewaddress()
        useraddress4 = self.nodes[3].getnewaddress()
        useraddress1 = self.nodes[0].getnewaddress()
        # needed by node4 when dbl-spending
        self.nodes[3].importprivkey(self.nodes[0].dumpprivkey(useraddress1))
        self.nodes[0].sendtoaddress(useraddress1, 1)
        self.nodes[0].sendtoaddress(useraddress1, 1)
        self.nodes[0].sendtoaddress(useraddress2, 1)
        self.nodes[0].sendtoaddress(useraddress3, 1)
        self.nodes[0].sendtoaddress(useraddress4, 1)
        self.nodes[0].assetsendmany(self.asset,[{'address': useraddress1,'amount':1.0},{'address': useraddress2,'amount':0.4},{'address': useraddress3,'amount':0.5}])
        self.nodes[0].generate(1)
        self.sync_blocks()
        # create separate output for dbl spend
        self.nodes[0].assetsend(self.asset, useraddress1, 0.5)
        time.sleep(0.25)
        # try to do multiple asset sends in one block
        assert_raises_rpc_error(-4, 'No inputs found for this asset', self.nodes[0].assetsend, self.asset, useraddress1, 2)
        self.nodes[0].generate(1)
        self.sync_blocks()
        self.nodes[0].assetallocationsend(self.asset, useraddress2, 0.2)
        time.sleep(0.25)
        self.nodes[1].assetallocationsend(self.asset, useraddress1, 0.2)
        time.sleep(0.25)
        self.nodes[0].assetallocationsend(self.asset, useraddress3, 0.2)
        time.sleep(0.25)
        self.nodes[2].assetallocationsend(self.asset, useraddress1, 0.2)
        time.sleep(0.25)
        self.sync_mempools(self.nodes[0:3],timeout=30)
        # put all in useraddress1 so node4 can access in dbl spend, its probably in change address prior to this on node0
        self.nodes[0].assetallocationsend(self.asset, useraddress1, 1.5)
        time.sleep(0.25)
        self.sync_mempools(timeout=30)
        txid = self.nodes[0].assetallocationsend(self.asset, useraddress1, 1.5)['txid']
        # dbl spend
        txdblspend = self.nodes[3].assetallocationburn(self.asset, 1.1, "0x931d387731bbbc988b312206c74f77d004d6b84b")["txid"]
        rawtx = self.nodes[0].getrawtransaction(txid)
        self.nodes[1].sendrawtransaction(rawtx)
        self.nodes[2].sendrawtransaction(rawtx)

        self.nodes[1].assetallocationsend(self.asset, useraddress3, 0.2)
        time.sleep(0.25)
        self.nodes[2].assetallocationburn(self.asset, 0.3, "0x931d387731bbbc988b312206c74f77d004d6b84b")
        time.sleep(0.25)
        self.sync_mempools(self.nodes[0:3], timeout=30)
        # node1/node2/node3 shouldn't have dbl spend tx because no RBF and not zdag tx
        assert_raises_rpc_error(-5, 'No such mempool transaction', self.nodes[0].getrawtransaction, txdblspend)
        assert_raises_rpc_error(-5, 'No such mempool transaction', self.nodes[1].getrawtransaction, txdblspend)
        assert_raises_rpc_error(-5, 'No such mempool transaction', self.nodes[2].getrawtransaction, txdblspend)
        self.nodes[3].getrawtransaction(txdblspend)
        self.nodes[0].generate(1)
        self.sync_blocks()
        # after block, even node4 should have removed conflicting tx
        assert_raises_rpc_error(-5, 'No such mempool transaction', self.nodes[3].getrawtransaction, txdblspend)
        out =  self.nodes[0].listunspent(query_options={'assetGuid': self.asset, 'minimumAmountAsset':0,'maximumAmountAsset':0})
        assert_equal(len(out), 1)
        out =  self.nodes[0].listunspent(query_options={'assetGuid': self.asset, 'minimumAmountAsset':1.5,'maximumAmountAsset':1.5})
        assert_equal(len(out), 1)
        out =  self.nodes[1].listunspent(query_options={'assetGuid': self.asset, 'minimumAmountAsset':0.2,'maximumAmountAsset':0.2})
        assert_equal(len(out), 1)
        out =  self.nodes[2].listunspent(query_options={'assetGuid': self.asset, 'minimumAmountAsset':0.2,'maximumAmountAsset':0.2})
        assert_equal(len(out), 2)
        out =  self.nodes[3].listunspent(query_options={'assetGuid': self.asset, 'minimumAmountAsset':1.5,'maximumAmountAsset':1.5})
        assert_equal(len(out), 1)

    def burn_zdag_doublespend_chain(self):
        # SYSX guid on regtest is 123456
        self.basic_asset(123456)
        self.nodes[0].generate(1)
        useraddress1 = self.nodes[1].getnewaddress()
        useraddress2 = self.nodes[2].getnewaddress()
        useraddress3 = self.nodes[3].getnewaddress()
        self.nodes[0].sendtoaddress(useraddress1, 1)
        self.nodes[0].sendtoaddress(useraddress2, 1)
        self.nodes[0].sendtoaddress(useraddress3, 1)
        self.nodes[0].generate(1)
        self.nodes[0].syscoinburntoassetallocation(self.asset, 1)
        time.sleep(0.25)
        self.nodes[0].syscoinburntoassetallocation(self.asset, 1)
        time.sleep(0.25)
        self.nodes[0].assetallocationsend(self.asset, useraddress1, 0.1)
        time.sleep(0.25)
        self.nodes[0].assetallocationsend(self.asset, useraddress2, 0.01)
        time.sleep(0.25)
        self.nodes[0].assetallocationsend(self.asset, useraddress2, 0.001)
        time.sleep(0.25)
        self.nodes[0].assetallocationsend(self.asset, useraddress3, 0.0001)
        time.sleep(0.25)
        self.nodes[0].assetallocationsend(self.asset, useraddress2, 0.00001)
        time.sleep(0.25)
        balanceBefore = self.nodes[0].getbalance(minconf=0)
        self.nodes[0].assetallocationburn(self.asset, 1, '')
        time.sleep(0.25)
        self.nodes[0].assetupdate(self.asset, '', '', 127, '', {}, {})
        time.sleep(0.25)
        self.nodes[0].assetallocationburn(self.asset, 0.88889, '')
        # subtract balance with 0.001 threshold to account for update fee
        assert(self.nodes[0].getbalance(minconf=0) - (balanceBefore+Decimal(1.88889)) < Decimal(0.001))
        # listunspent for node0 should be have just 1 (asset ownership) in mempool
        out =  self.nodes[0].listunspent(minconf=0, query_options={'assetGuid': self.asset})
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], 123456)
        assert_equal(out[0]['asset_amount'], 0)
        self.nodes[0].generate(1)
        out =  self.nodes[0].listunspent(query_options={'assetGuid': self.asset})
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], 123456)
        assert_equal(out[0]['asset_amount'], 0)
        self.sync_blocks()
        # listunspent for node0 should be have just 1 (asset ownership)
        # check that nodes have allocations in listunspent before burning
        self.nodes[1].assetallocationburn(self.asset, 0.1, '')
        time.sleep(0.25)
        self.nodes[2].assetallocationburn(self.asset, 0.01, '')
        time.sleep(0.25)
        self.nodes[2].assetallocationburn(self.asset, 0.001, '')
        time.sleep(0.25)
        self.nodes[3].assetallocationburn(self.asset, 0.0001, '')
        time.sleep(0.25)
        self.nodes[2].assetallocationburn(self.asset, 0.00001, '')
        # ensure burning sysx gives new sys balance
        # account for rounding errors in Decimal
        assert(self.nodes[1].getbalance(minconf=0) - (balanceBefore+Decimal(0.1)) < Decimal(0.001))
        assert(self.nodes[2].getbalance(minconf=0) - (balanceBefore+Decimal(0.01101)) < Decimal(0.001))
        assert(self.nodes[3].getbalance(minconf=0) - (balanceBefore+Decimal(0.0001)) < Decimal(0.0001))
        out =  self.nodes[1].listunspent(minconf=0, query_options={'assetGuid': self.asset})
        assert_equal(len(out), 0)
        out =  self.nodes[2].listunspent(minconf=0, query_options={'assetGuid': self.asset})
        assert_equal(len(out), 0)
        out =  self.nodes[3].listunspent(minconf=0, query_options={'assetGuid': self.asset})
        assert_equal(len(out), 0)
        # check listunspent is empty in mempool, all should be burned
        self.nodes[0].assetupdate(self.asset, '', '', 127, '', {}, {})
        self.nodes[0].generate(1)
        assert(self.nodes[1].getbalance() - (balanceBefore+Decimal(0.1)) < Decimal(0.001))
        assert(self.nodes[2].getbalance() - (balanceBefore+Decimal(0.01101)) < Decimal(0.001))
        assert(self.nodes[3].getbalance() - (balanceBefore+Decimal(0.0001)) < Decimal(0.0001))
        # check listunspent is empty, all should be burned
        out =  self.nodes[1].listunspent(query_options={'assetGuid': self.asset})
        assert_equal(len(out), 0)
        out =  self.nodes[2].listunspent(query_options={'assetGuid': self.asset})
        assert_equal(len(out), 0)
        out =  self.nodes[3].listunspent(query_options={'assetGuid': self.asset})
        assert_equal(len(out), 0)

    def basic_asset(self, guid):
        if guid is None:
            self.asset = self.nodes[0].assetnew('1', "TST", "asset description", "0x9f90b5093f35aeac5fbaeb591f9c9de8e2844a46", 8, 10000, 127, '', {}, {})['asset_guid']
        else:
            self.asset = self.nodes[0].assetnewtest(guid, '1', "TST", "asset description", "0x9f90b5093f35aeac5fbaeb591f9c9de8e2844a46", 8, 10000, 127, '', {}, {})['asset_guid']

if __name__ == '__main__':
    AssetZDAGTest().main()
