#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import time
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal
from decimal import Decimal

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

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

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
        useraddress2 = self.nodes[2].getnewaddress()
        useraddress3 = self.nodes[3].getnewaddress()
        self.nodes[0].sendtoaddress(useraddress2, 1)
        self.nodes[2].importprivkey(self.nodes[0].dumpprivkey(useraddress0))
        self.nodes[0].assetsend(self.asset, useraddress0, 1.5)
        self.nodes[0].generate(1)
        tx1 = self.nodes[0].assetallocationsend(self.asset, useraddress2, 0.00001, 0, False)['txid']
        tx2 = self.nodes[0].assetallocationsend(self.asset, useraddress3, 0.0001, 0, False)['txid']
        tx3 = self.nodes[0].assetallocationsend(self.asset, useraddress0, 1, 0, False)['txid']
        self.sync_mempools(self.nodes,timeout=30)
        assert_equal(self.nodes[3].assetallocationbalance(self.asset, [], 0)['asset_amount'], Decimal('0.0001'))
        tx4 = self.nodes[0].assetallocationsend(self.asset, useraddress0, 1, 0, False)['txid']
        # dbl spend outputs from tx3 (tx4 and tx5 should be flagged as conflict)
        tx4a = self.nodes[2].assetallocationsend(self.asset, useraddress0, 1, 0, False)['txid']
        time.sleep(0.25)
        tx5 = self.nodes[0].assetallocationsend(self.asset, useraddress2, 0.0001, 0, False)['txid']
        time.sleep(0.25)
        tx6 = self.nodes[0].assetallocationsend(self.asset, useraddress2, 1)['txid']
        time.sleep(0.25)
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
        self.nodes[0].assetsend(self.asset, useraddress0, 1.5)
        self.nodes[0].generate(1)
        tx1 = self.nodes[0].assetallocationsend(self.asset, useraddress2, 0.00001)['txid']
        tx2 = self.nodes[0].assetallocationsend(self.asset, useraddress3, 0.0001)['txid']
        tx3 = self.nodes[0].assetallocationburn(self.asset, 1, '0x9f90b5093f35aeac5fbaeb591f9c9de8e2844a46')['txid']
        tx4 = self.nodes[0].assetallocationsend(self.asset, useraddress1, 0.001)['txid']
        tx5 = self.nodes[0].assetallocationsend(self.asset, useraddress2, 0.002)['txid']
        self.sync_mempools(self.nodes[0:3],timeout=30)
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

        tx1 = self.nodes[0].assetallocationsend(self.asset, useraddress2, 0.00001)['txid']
        tx2 = self.nodes[0].assetallocationsend(self.asset, useraddress3, 0.0001)['txid']
        tx3 = self.nodes[0].assetupdate(self.asset, '', '', 127, '', {}, {})['txid']
        tx4 = self.nodes[0].assetallocationsend(self.asset, useraddress1, 0.001)['txid']
        tx5 = self.nodes[0].assetallocationsend(self.asset, useraddress2, 0.002)['txid']
        self.sync_mempools(self.nodes[0:3],timeout=30)
        for i in range(3):
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx1)['status'], ZDAG_STATUS_OK)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx2)['status'], ZDAG_STATUS_OK)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx3)['status'], ZDAG_WARNING_NOT_ZDAG_TX)
            # update won't be used as input, tx4 will use tx2 as input because asset update uses different UTXO for ownership which is
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
            self.asset = self.nodes[0].assetnew('1', "TST", "asset description", "0x9f90b5093f35aeac5fbaeb591f9c9de8e2844a46", 8, 10000, 127, '', {}, {})['asset_guid']
        else:
            self.asset = self.nodes[0].assetnewtest(guid, '1', "TST", "asset description", "0x9f90b5093f35aeac5fbaeb591f9c9de8e2844a46", 8, 10000, 127, '', {}, {})['asset_guid']

if __name__ == '__main__':
    AssetVerifyZDAGTest().main()
