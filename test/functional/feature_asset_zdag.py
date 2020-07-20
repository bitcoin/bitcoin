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
MAX_INITIAL_BROADCAST_DELAY = 15 * 60 # 15 minutes in seconds
class AssetZDAGTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.rpc_timeout = 240
        self.extra_args = [['-assetindex=1'],['-assetindex=1'],['-assetindex=1']]

    def run_test(self):
        self.nodes[0].generate(200)
        self.sync_blocks()
        self.basic_zdag_doublespend()

    def basic_zdag_doublespend(self):
        self.basic_asset()
        self.nodes[0].generate(1)
        newaddress2 = self.nodes[1].getnewaddress()
        newaddress3 = self.nodes[1].getnewaddress()
        newaddress1 = self.nodes[0].getnewaddress()
        self.nodes[2].importprivkey(self.nodes[1].dumpprivkey(newaddress2))
        self.nodes[0].assetsend(self.asset, newaddress1, int(2*COIN))
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
        self.nodes[0].assetallocationsend(self.asset, newaddress2, int(1*COIN))
        self.nodes[0].assetallocationsend(self.asset, newaddress2, int(0.4*COIN))
        self.nodes[0].generate(1)
        self.sync_blocks()
        # should have 2 sys utxos and 2 asset utxos
        out =  self.nodes[2].listunspent()
        assert_equal(len(out), 4)
        # this will use 1 sys utxo and 1 asset utxo and send it to change address owned by node2
        self.nodes[1].assetallocationsend(self.asset, newaddress1, int(0.3*COIN))
        self.sync_mempools(timeout=30)
        # node3 should have 2 less utxos because they were sent to change on node2
        out =  self.nodes[2].listunspent(minconf=0)
        assert_equal(len(out), 2)
        # disconnect node 2 and 3 so they can double spend without seeing each others transaction
        disconnect_nodes(self.nodes[1], 2)
        tx1 = self.nodes[1].assetallocationsend(self.asset, newaddress1, int(1*COIN))['txid']
        time.sleep(1)
        # dbl spend
        tx2 = self.nodes[2].assetallocationsend(self.asset, newaddress1, int(0.9*COIN))['txid']
        # use tx2 to build tx3
        tx3 = self.nodes[2].assetallocationsend(self.asset, newaddress1, int(0.05*COIN))['txid']
        # use tx2 to build tx4
        tx4 = self.nodes[2].assetallocationsend(self.asset, newaddress1, int(0.025*COIN))['txid']
        connect_nodes(self.nodes[1], 2)
        # broadcast transactions
        bump_node_times(self.nodes, MAX_INITIAL_BROADCAST_DELAY+1)
        time.sleep(2)
        self.sync_mempools(timeout=30)
        for i in range(3):
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx1)['status'], ZDAG_MAJOR_CONFLICT)
            # ensure the tx2 made it to mempool, should propogate dbl-spend first time
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx2)['status'], ZDAG_MAJOR_CONFLICT)
            # will conflict because its using tx2 which is in conflict state
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx3)['status'], ZDAG_MAJOR_CONFLICT)
            # will conflict because its using tx3 which uses tx2 which is in conflict state
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx4)['status'], ZDAG_MAJOR_CONFLICT)
        self.nodes[0].generate(1)
        self.sync_blocks()
        for i in range(3):
            self.nodes[i].getrawtransaction(tx1)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx1)['status'], ZDAG_NOT_FOUND)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx2)['status'], ZDAG_NOT_FOUND)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx3)['status'], ZDAG_NOT_FOUND)
            assert_equal(self.nodes[i].assetallocationverifyzdag(tx4)['status'], ZDAG_NOT_FOUND)
            assert_raises_rpc_error(-5, 'No such mempool transaction', self.nodes[i].getrawtransaction, tx2)
            assert_raises_rpc_error(-5, 'No such mempool transaction', self.nodes[i].getrawtransaction, tx3)
            assert_raises_rpc_error(-5, 'No such mempool transaction', self.nodes[i].getrawtransaction, tx4)
        out =  self.nodes[0].listunspent(query_options={'assetGuid': self.asset, 'minimumAmountAsset':0,'maximumAmountAsset':0})
        assert_equal(len(out), 1)
        out =  self.nodes[0].listunspent(query_options={'assetGuid': self.asset, 'minimumAmountAsset':0.3,'maximumAmountAsset':0.3})
        assert_equal(len(out), 1)
        out =  self.nodes[0].listunspent(query_options={'assetGuid': self.asset, 'minimumAmountAsset':0.6,'maximumAmountAsset':0.6})
        assert_equal(len(out), 1)
        out =  self.nodes[0].listunspent(query_options={'assetGuid': self.asset, 'minimumAmountAsset':1.0,'maximumAmountAsset':1.0})
        assert_equal(len(out), 1)
        out =  self.nodes[0].listunspent(query_options={'assetGuid': self.asset})
        assert_equal(len(out), 4)

    def basic_asset(self):
        self.asset = self.nodes[0].assetnew('1', "TST", "asset description", "0x9f90b5093f35aeac5fbaeb591f9c9de8e2844a46", 8, 1000*COIN, 10000*COIN, 31, {})['asset_guid']


if __name__ == '__main__':
    AssetZDAGTest().main()
