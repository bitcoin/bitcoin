#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test transaction time during old block rescanning
"""

import time

import threading
from test_framework.authproxy import AuthServiceProxy
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    set_node_times,
)


class TransactionTimeRescanTest(BitcoinTestFramework):
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

        minernode = self.nodes[0]  # node used to mine BTC and create transactions
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
        wo1_desc = usernode.getaddressinfo(wo1)["desc"]
        wo2 = usernode.getnewaddress()
        wo2_desc = usernode.getaddressinfo(wo2)["desc"]
        wo3 = usernode.getnewaddress()
        wo3_desc = usernode.getaddressinfo(wo3)["desc"]

        usernode.createwallet(wallet_name='wo', disable_private_keys=True)
        wo_wallet = usernode.get_wallet_rpc('wo')

        import_res = wo_wallet.importdescriptors(
            [
                {"desc": wo1_desc, "timestamp": "now"},
                {"desc": wo2_desc, "timestamp": "now"},
                {"desc": wo3_desc, "timestamp": "now"},
            ]
        )
        assert_equal(all([r["success"] for r in import_res]), True)

        self.log.info('Start transactions')

        # check blockcount
        assert_equal(minernode.getblockcount(), 200)

        # generate some btc to create transactions and check blockcount
        initial_mine = COINBASE_MATURITY + 1
        self.generatetoaddress(minernode, initial_mine, m1)
        assert_equal(minernode.getblockcount(), initial_mine + 200)

        # synchronize nodes and time
        self.sync_all()
        set_node_times(self.nodes, cur_time + ten_days)
        # send 10 btc to user's first watch-only address
        self.log.info('Send 10 btc to user')
        miner_wallet.sendtoaddress(wo1, 10)

        # generate blocks and check blockcount
        self.generatetoaddress(minernode, COINBASE_MATURITY, m1)
        assert_equal(minernode.getblockcount(), initial_mine + 300)

        # synchronize nodes and time
        self.sync_all()
        set_node_times(self.nodes, cur_time + ten_days + ten_days)
        # send 5 btc to our second watch-only address
        self.log.info('Send 5 btc to user')
        miner_wallet.sendtoaddress(wo2, 5)

        # generate blocks and check blockcount
        self.generatetoaddress(minernode, COINBASE_MATURITY, m1)
        assert_equal(minernode.getblockcount(), initial_mine + 400)

        # synchronize nodes and time
        self.sync_all()
        set_node_times(self.nodes, cur_time + ten_days + ten_days + ten_days)
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

        # importdescriptors with "timestamp": "now" always rescans
        # blocks of the past 2 hours, based on the current MTP timestamp; in order to avoid
        # importing the last address (wo3), we advance the time further and generate 10 blocks
        set_node_times(self.nodes, cur_time + ten_days + ten_days + ten_days + ten_days)
        self.generatetoaddress(minernode, 10, m1)

        import_res = restorewo_wallet.importdescriptors(
            [
                {"desc": wo1_desc, "timestamp": "now"},
                {"desc": wo2_desc, "timestamp": "now"},
                {"desc": wo3_desc, "timestamp": "now"},
            ]
        )
        assert_equal(all([r["success"] for r in import_res]), True)

        self.log.info('Testing abortrescan when no rescan is in progress')
        assert_equal(restorewo_wallet.getwalletinfo()['scanning'], False)
        assert_equal(restorewo_wallet.abortrescan(), False)

        # check user has 0 balance and no transactions
        assert_equal(restorewo_wallet.getbalance(), 0)
        assert_equal(len(restorewo_wallet.listtransactions()), 0)

        # Test 'abortrescan' when a rescan is in progress:
        # We’ll retry on increasingly large tails until we catch 'scanning==True'.
        self.log.info('testing abortrescan when a rescan is in progress')
        # RPC handle dedicated to polling getwalletinfo() and abortrescan()
        poll_rpc = AuthServiceProxy(restorenode.url + '/wallet/wo')

        # Get current block height and start with up to 10‐block tail.
        height = restorewo_wallet.getblockcount()
        tail = min(10, height)

        # Flag for whether we ever observed scanning==True.
        saw = False
        # Cap to avoid infinite retries.
        max_attempts = 5

        for attempt in range(max_attempts):
            self.log.info(
                f'Attempt {attempt + 1}/{max_attempts}: '
                f'rescan last {tail} blocks'
            )

            def _rescan_ignore_abort():
                """Do the rescan and swallow any exception (including abort)."""
                try:
                    restorewo_wallet.rescanblockchain(height - tail)
                except Exception:
                    pass

            # Launch the rescan in a daemon thread.
            t = threading.Thread(target=_rescan_ignore_abort)
            t.daemon = True
            t.start()

            # Poll every 0.01s for up to 5s to see scanning==True.
            deadline = time.time() + 5
            while time.time() < deadline:
                if poll_rpc.getwalletinfo().get('scanning', False):
                    saw = True
                    break
                time.sleep(0.01)

            if saw:
                # We saw the scan start, exit loop.
                self.log.info(f'scanning observed for tail={tail}')
                break

            # Didn't see scanning, double tail, cleanup and retry.
            self.log.warning(
                f'No scanning for tail={tail}; doubling tail'
            )
            t.join(timeout=1)
            tail = min(height, tail * 2)
        else:
            # If we never saw scanning after all attempts, fail.
            raise AssertionError(
                f'Could not observe scanning after {max_attempts} attempts'
            )

        # Now abort the in-flight rescan and require a True return.
        result = poll_rpc.abortrescan()
        assert_equal(result, True)
        self.log.info(f'abortrescan() returned {result}')

        # Wait for the thread to finish cleanup.
        t.join(timeout=2)
        if t.is_alive():
            self.log.warning(
                'Background rescan thread still alive after join'
            )

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


if __name__ == '__main__':
    TransactionTimeRescanTest(__file__).main()
