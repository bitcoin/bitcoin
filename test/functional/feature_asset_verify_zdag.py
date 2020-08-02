#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error, set_node_times, disconnect_nodes, connect_nodes, bump_node_times
from test_framework.messages import COIN
import time
ZDAG_NOT_FOUND = -1
ZDAG_STATUS_OK = 0
ZDAG_WARNING_RBF = 1
ZDAG_WARNING_NOT_ZDAG_TX = 2
ZDAG_WARNING_SIZE_OVER_POLICY = 3
ZDAG_MAJOR_CONFLICT = 4
class AssetVerifyZDAGTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4
        self.rpc_timeout = 240
        self.extra_args = [['-assetindex=1'],['-assetindex=1'],['-assetindex=1'],['-assetindex=1']]

    def run_test(self):
        self.nodes[0].generate(200)
        self.sync_blocks()
        self.burn_zdag_ancestor_nonzdag()
        self.burn_zdag_ancestor_doublespend()

    # dbl spend verify zdag will flag any descendents but not ancestor txs
    def burn_zdag_ancestor_doublespend(self):
        self.basic_asset(guid=None)
        self.nodes[0].generate(1)
        useraddress0 = self.nodes[0].getnewaddress()
        useraddress1 = self.nodes[1].getnewaddress()
        useraddress2 = self.nodes[2].getnewaddress()
        useraddress3 = self.nodes[3].getnewaddress()
        self.nodes[0].sendtoaddress(useraddress3, 1)
        self.nodes[3].importprivkey(self.nodes[0].dumpprivkey(useraddress0))
        self.nodes[0].assetsend(self.asset, useraddress0, int(1.5*COIN))
        self.nodes[0].generate(1)
        tx1 = self.nodes[0].assetallocationsend(self.asset, useraddress2, int(0.00001*COIN))['txid']
        tx2 = self.nodes[0].assetallocationsend(self.asset, useraddress3, int(0.0001*COIN))['txid']
        tx3 = self.nodes[0].assetallocationsend(self.asset, useraddress0, int(1*COIN))['txid']
        self.sync_mempools(timeout=30)
        tx4 = self.nodes[0].assetallocationsend(self.asset, useraddress0, int(1*COIN))['txid']
        # dbl spend outputs from tx3 (tx4 and tx5 should be flagged as conflict)
        tx4a = self.nodes[3].assetallocationsend(self.asset, useraddress0, int(1*COIN))['txid']
        tx5 = self.nodes[0].assetallocationsend(self.asset, useraddress2, int(0.0001*COIN))['txid']
        tx6 = self.nodes[0].assetallocationsend(self.asset, useraddress2, int(1*COIN))['txid']
        self.sync_mempools(self.nodes[0:3], timeout=30)
        for i in range(2):
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx1)['status'], ZDAG_STATUS_OK)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx2)['status'], ZDAG_STATUS_OK)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx3)['status'], ZDAG_STATUS_OK)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx4a)['status'], ZDAG_MAJOR_CONFLICT)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx4)['status'], ZDAG_MAJOR_CONFLICT)
            # this one uses output from tx2 so its not involved in the conflict chain
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx5)['status'], ZDAG_STATUS_OK)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx6)['status'], ZDAG_MAJOR_CONFLICT)

        self.nodes[0].generate(1)
        self.sync_blocks()
        for i in range(2):
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx1)['status'], ZDAG_NOT_FOUND)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx2)['status'], ZDAG_NOT_FOUND)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx3)['status'], ZDAG_NOT_FOUND)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx4a)['status'], ZDAG_NOT_FOUND)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx4)['status'], ZDAG_NOT_FOUND)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx5)['status'], ZDAG_NOT_FOUND)

    # verify zdag will flag any descendents of non-zdag tx but not ancestors
    def burn_zdag_ancestor_nonzdag(self):
        self.basic_asset(guid=None)
        self.nodes[0].generate(1)
        useraddress0 = self.nodes[0].getnewaddress()
        useraddress1 = self.nodes[1].getnewaddress()
        useraddress2 = self.nodes[2].getnewaddress()
        useraddress3 = self.nodes[3].getnewaddress()
        self.nodes[0].assetsend(self.asset, useraddress0, int(1.5*COIN))
        self.nodes[0].generate(1)
        tx1 = self.nodes[0].assetallocationsend(self.asset, useraddress2, int(0.00001*COIN))['txid']
        tx2 = self.nodes[0].assetallocationsend(self.asset, useraddress3, int(0.0001*COIN))['txid']
        tx3 = self.nodes[0].assetallocationburn(self.asset, int(1*COIN), '0x9f90b5093f35aeac5fbaeb591f9c9de8e2844a46')['txid']
        tx4 = self.nodes[0].assetallocationsend(self.asset, useraddress1, int(0.001*COIN))['txid']
        tx5 = self.nodes[0].assetallocationsend(self.asset, useraddress2, int(0.002*COIN))['txid']
        self.sync_mempools(timeout=30)
        for i in range(3):
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx1)['status'], ZDAG_STATUS_OK)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx2)['status'], ZDAG_STATUS_OK)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx3)['status'], ZDAG_WARNING_NOT_ZDAG_TX)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx4)['status'], ZDAG_WARNING_NOT_ZDAG_TX)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx5)['status'], ZDAG_WARNING_NOT_ZDAG_TX)

        self.nodes[0].generate(1)
        self.sync_blocks()
        for i in range(3):
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx1)['status'], ZDAG_NOT_FOUND)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx2)['status'], ZDAG_NOT_FOUND)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx3)['status'], ZDAG_NOT_FOUND)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx4)['status'], ZDAG_NOT_FOUND)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx5)['status'], ZDAG_NOT_FOUND)

        tx1 = self.nodes[0].assetallocationsend(self.asset, useraddress2, int(0.00001*COIN))['txid']
        tx2 = self.nodes[0].assetallocationsend(self.asset, useraddress3, int(0.0001*COIN))['txid']
        tx3 = self.nodes[0].assetupdate(self.asset, '', '', 0, 31, '', '', {})['txid']
        tx4 = self.nodes[0].assetallocationsend(self.asset, useraddress1, int(0.001*COIN))['txid']
        tx5 = self.nodes[0].assetallocationsend(self.asset, useraddress2, int(0.002*COIN))['txid']
        self.sync_mempools(timeout=30)
        for i in range(3):
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx1)['status'], ZDAG_STATUS_OK)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx2)['status'], ZDAG_STATUS_OK)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx3)['status'], ZDAG_WARNING_NOT_ZDAG_TX)
            # update won't be used as input, tx4 will us tx2 as input because asset update uses different UTXO for ownership which is
            # not selected for zdag txs
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx4)['status'], ZDAG_STATUS_OK)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx5)['status'], ZDAG_STATUS_OK)
        
        self.nodes[0].generate(1)
        self.sync_blocks()
        for i in range(3):
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx1)['status'], ZDAG_NOT_FOUND)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx2)['status'], ZDAG_NOT_FOUND)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx3)['status'], ZDAG_NOT_FOUND)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx4)['status'], ZDAG_NOT_FOUND)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx5)['status'], ZDAG_NOT_FOUND)

    def basic_asset(self, guid):
        if guid is None:
            self.asset = self.nodes[0].assetnew('1', "TST", "asset description", "0x9f90b5093f35aeac5fbaeb591f9c9de8e2844a46", 8, 1000*COIN, 10000*COIN, 31, '', '', {})['asset_guid']
        else:
            self.asset = self.nodes[0].assetnewtest(guid, '1', "TST", "asset description", "0x9f90b5093f35aeac5fbaeb591f9c9de8e2844a46", 8, 1000*COIN, 10000*COIN, 31, '', '', {})['asset_guid']

if __name__ == '__main__':
    AssetVerifyZDAGTest().main()
