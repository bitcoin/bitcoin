#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test transaction time during old block rescanning
"""

import threading
import time

from test_framework.authproxy import JSONRPCException
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    set_node_times,
)
from test_framework.wallet_util import (
    get_generate_key,
)


class TransactionTimeRescanTest(SyscoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 3
        self.extra_args = [["-keypool=400"],
                           ["-keypool=400"],
                           []
                          ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info('Prepare nodes and wallet')

        minernode = self.nodes[0]  # node used to mine SYS and create transactions
        usernode = self.nodes[1]  # user node with correct time
        restorenode = self.nodes[2]  # node used to restore user wallet and check time determination in ComputeSmartTime (wallet.cpp)

        # time constant
        cur_time = int(time.time())
        ten_days = 10 * 24 * 60 * 60

        # synchronize nodes and time
        self.sync_all()
        set_node_times(self.nodes, cur_time)

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

        # generate some sys to create transactions and check blockcount
        initial_mine = COINBASE_MATURITY + 1
        self.generatetoaddress(minernode, initial_mine, m1)
        assert_equal(minernode.getblockcount(), initial_mine + 200)

        # synchronize nodes and time
        self.sync_all()
        set_node_times(self.nodes, cur_time + ten_days)
        # send 10 sys to user's first watch-only address
        self.log.info('Send 10 sys to user')
        miner_wallet.sendtoaddress(wo1, 10)

        # generate blocks and check blockcount
        self.generatetoaddress(minernode, COINBASE_MATURITY, m1)
        assert_equal(minernode.getblockcount(), initial_mine + 300)

        # synchronize nodes and time
        self.sync_all()
        set_node_times(self.nodes, cur_time + ten_days + ten_days)
        # send 5 sys to our second watch-only address
        self.log.info('Send 5 sys to user')
        miner_wallet.sendtoaddress(wo2, 5)

        # generate blocks and check blockcount
        self.generatetoaddress(minernode, COINBASE_MATURITY, m1)
        assert_equal(minernode.getblockcount(), initial_mine + 400)

        # synchronize nodes and time
        self.sync_all()
        set_node_times(self.nodes, cur_time + ten_days + ten_days + ten_days)
        # send 1 sys to our third watch-only address
        self.log.info('Send 1 sys to user')
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

        # for descriptor wallets, the test framework maps the importaddress RPC to the
        # importdescriptors RPC (with argument 'timestamp'='now'), which always rescans
        # blocks of the past 2 hours, based on the current MTP timestamp; in order to avoid
        # importing the last address (wo3), we advance the time further and generate 10 blocks
        if self.options.descriptors:
            set_node_times(self.nodes, cur_time + ten_days + ten_days + ten_days + ten_days)
            self.generatetoaddress(minernode, 10, m1)

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


        self.log.info('Test handling of invalid parameters for rescanblockchain')
        assert_raises_rpc_error(-8, "Invalid start_height", restorewo_wallet.rescanblockchain, -1, 10)
        assert_raises_rpc_error(-8, "Invalid stop_height", restorewo_wallet.rescanblockchain, 1, -1)
        assert_raises_rpc_error(-8, "stop_height must be greater than start_height", restorewo_wallet.rescanblockchain, 20, 10)

        self.log.info("Test `rescanblockchain` fails when wallet is encrypted and locked")
        usernode.createwallet(wallet_name="enc_wallet", passphrase="passphrase")
        enc_wallet = usernode.get_wallet_rpc("enc_wallet")
        assert_raises_rpc_error(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.", enc_wallet.rescanblockchain)

        if not self.options.descriptors:
            self.log.info("Test rescanning an encrypted wallet")
            hd_seed = get_generate_key().privkey

            usernode.createwallet(wallet_name="temp_wallet", blank=True, descriptors=False)
            temp_wallet = usernode.get_wallet_rpc("temp_wallet")
            temp_wallet.sethdseed(seed=hd_seed)

            for i in range(399):
                temp_wallet.getnewaddress()

            self.generatetoaddress(usernode, COINBASE_MATURITY + 1, temp_wallet.getnewaddress())
            self.generatetoaddress(usernode, COINBASE_MATURITY + 1, temp_wallet.getnewaddress())

            minernode.createwallet("encrypted_wallet", blank=True, passphrase="passphrase", descriptors=False)
            encrypted_wallet = minernode.get_wallet_rpc("encrypted_wallet")

            encrypted_wallet.walletpassphrase("passphrase", 99999)
            encrypted_wallet.sethdseed(seed=hd_seed)

            t = threading.Thread(target=encrypted_wallet.rescanblockchain)

            with minernode.assert_debug_log(expected_msgs=[f'Rescan started from block 0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206... (slow variant inspecting all blocks)'], timeout=5):
                t.start()

            # set the passphrase timeout to 1 to test that the wallet remains unlocked during the rescan
            minernode.cli("-rpcwallet=encrypted_wallet").walletpassphrase("passphrase", 1)

            try:
                minernode.cli("-rpcwallet=encrypted_wallet").walletlock()
            except JSONRPCException as e:
                assert e.error['code'] == -4 and "Error: the wallet is currently being used to rescan the blockchain for related transactions. Please call `abortrescan` before locking the wallet." in e.error['message']

            try:
                minernode.cli("-rpcwallet=encrypted_wallet").walletpassphrasechange("passphrase", "newpassphrase")
            except JSONRPCException as e:
                assert e.error['code'] == -4 and "Error: the wallet is currently being used to rescan the blockchain for related transactions. Please call `abortrescan` before changing the passphrase." in e.error['message']

            t.join()

            assert_equal(encrypted_wallet.getbalance(), temp_wallet.getbalance())

if __name__ == '__main__':
    TransactionTimeRescanTest().main()
