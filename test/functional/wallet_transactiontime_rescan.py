#!/usr/bin/env python3
# Copyright (c) 2018-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test transaction time during old block rescanning
"""

import time

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal
)


class TransactionTimeRescanTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 3

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info('Prepare nodes and wallet')

        minernode = self.nodes[0]  # node used to mine BTC and create transactions
        usernode = self.nodes[1]  # user node with correct time
        restorenode = self.nodes[2]  # node used to restore user wallet and check time determination in ComputeSmartTime (wallet.cpp)

        # time constant
        cur_time = int(time.time())
        ten_days = 10 * 24 * 60 * 60

        # synchronize nodes and time
        self.sync_all()
        minernode.setmocktime(cur_time)
        usernode.setmocktime(cur_time)
        restorenode.setmocktime(cur_time)

        # prepare miner wallet
        minernode.createwallet(wallet_name='default')
        miner_wallet = minernode.get_wallet_rpc('default')
        m1 = miner_wallet.getnewaddress()

        # prepare the user wallet with 3 watch only addresses
        wo1 = usernode.getnewaddress()
        wo2 = usernode.getnewaddress()
        wo3 = usernode.getnewaddress()

        usernode.createwallet(wallet_name='wo', disable_private_keys=True)
        wo_wallet = usernode.get_wallet_rpc('wo')

        wo_wallet.importaddress(wo1)
        wo_wallet.importaddress(wo2)
        wo_wallet.importaddress(wo3)

        self.log.info('Start transactions')

        # check blockcount
        assert_equal(minernode.getblockcount(), 200)

        # generate some btc to create transactions and check blockcount
        initial_mine = COINBASE_MATURITY + 1
        self.generatetoaddress(minernode, initial_mine, m1)
        assert_equal(minernode.getblockcount(), initial_mine + 200)

        # synchronize nodes and time
        self.sync_all()
        minernode.setmocktime(cur_time + ten_days)
        usernode.setmocktime(cur_time + ten_days)
        restorenode.setmocktime(cur_time + ten_days)
        # send 10 btc to user's first watch-only address
        self.log.info('Send 10 btc to user')
        miner_wallet.sendtoaddress(wo1, 10)

        # generate blocks and check blockcount
        self.generatetoaddress(minernode, COINBASE_MATURITY, m1)
        assert_equal(minernode.getblockcount(), initial_mine + 300)

        # synchronize nodes and time
        self.sync_all()
        minernode.setmocktime(cur_time + ten_days + ten_days)
        usernode.setmocktime(cur_time + ten_days + ten_days)
        restorenode.setmocktime(cur_time + ten_days + ten_days)
        # send 5 btc to our second watch-only address
        self.log.info('Send 5 btc to user')
        miner_wallet.sendtoaddress(wo2, 5)

        # generate blocks and check blockcount
        self.generatetoaddress(minernode, COINBASE_MATURITY, m1)
        assert_equal(minernode.getblockcount(), initial_mine + 400)

        # synchronize nodes and time
        self.sync_all()
        minernode.setmocktime(cur_time + ten_days + ten_days + ten_days)
        usernode.setmocktime(cur_time + ten_days + ten_days + ten_days)
        restorenode.setmocktime(cur_time + ten_days + ten_days + ten_days)
        # send 1 btc to our third watch-only address
        self.log.info('Send 1 btc to user')
        miner_wallet.sendtoaddress(wo3, 1)

        # generate more blocks and check blockcount
        self.generatetoaddress(minernode, COINBASE_MATURITY, m1)
        assert_equal(minernode.getblockcount(), initial_mine + 500)

        self.log.info('Check user\'s final balance and transaction count')
        assert_equal(wo_wallet.getbalance(), 16)
        assert_equal(len(wo_wallet.listtransactions()), 3)

        self.log.info('Check transaction times')
        for tx in wo_wallet.listtransactions():
            if tx['address'] == wo1:
                assert_equal(tx['blocktime'], cur_time + ten_days)
                assert_equal(tx['time'], cur_time + ten_days)
            elif tx['address'] == wo2:
                assert_equal(tx['blocktime'], cur_time + ten_days + ten_days)
                assert_equal(tx['time'], cur_time + ten_days + ten_days)
            elif tx['address'] == wo3:
                assert_equal(tx['blocktime'], cur_time + ten_days + ten_days + ten_days)
                assert_equal(tx['time'], cur_time + ten_days + ten_days + ten_days)

        # restore user wallet without rescan
        self.log.info('Restore user wallet on another node without rescan')
        restorenode.createwallet(wallet_name='wo', disable_private_keys=True)
        restorewo_wallet = restorenode.get_wallet_rpc('wo')

        restorewo_wallet.importaddress(wo1, rescan=False)
        restorewo_wallet.importaddress(wo2, rescan=False)
        restorewo_wallet.importaddress(wo3, rescan=False)

        # check user has 0 balance and no transactions
        assert_equal(restorewo_wallet.getbalance(), 0)
        assert_equal(len(restorewo_wallet.listtransactions()), 0)

        # proceed to rescan, first with an incomplete one, then with a full rescan
        self.log.info('Rescan last history part')
        restorewo_wallet.rescanblockchain(initial_mine + 350)
        self.log.info('Rescan all history')
        restorewo_wallet.rescanblockchain()

        self.log.info('Check user\'s final balance and transaction count after restoration')
        assert_equal(restorewo_wallet.getbalance(), 16)
        assert_equal(len(restorewo_wallet.listtransactions()), 3)

        self.log.info('Check transaction times after restoration')
        for tx in restorewo_wallet.listtransactions():
            if tx['address'] == wo1:
                assert_equal(tx['blocktime'], cur_time + ten_days)
                assert_equal(tx['time'], cur_time + ten_days)
            elif tx['address'] == wo2:
                assert_equal(tx['blocktime'], cur_time + ten_days + ten_days)
                assert_equal(tx['time'], cur_time + ten_days + ten_days)
            elif tx['address'] == wo3:
                assert_equal(tx['blocktime'], cur_time + ten_days + ten_days + ten_days)
                assert_equal(tx['time'], cur_time + ten_days + ten_days + ten_days)


if __name__ == '__main__':
    TransactionTimeRescanTest().main()
