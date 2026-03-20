#!/usr/bin/env python3
# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.

"""Test wallet birthtime updates during wallet scanning"""

import time

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)

class WalletBirthTimeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    # Verify the wallet updates the birth time accordingly when it detects a transaction
    # with a time older than the oldest descriptor timestamp.
    # This could happen when the user blindly imports a descriptor with 'timestamp=now'.
    def birthtime_test(self, node):
        self.log.info("Test birth time update during wallet rescan")
        node.setmocktime(int(time.time()))

        BLOCK_TIME = 60 * 10 # 10 mins
        TX_CONFIRMATIONS = BLOCKS_TO_GENERATE = 20
        AMOUNT_SENT = AMOUNT_RECEIVED = 2 # BTC

        # Fund miner wallet
        node.createwallet(wallet_name='miner')
        miner_wallet = node.get_wallet_rpc('miner')
        # Generate (COINBASE_MATURITY + 1) blocks, 1 extra so that the wallet can readily spend
        self.generatetoaddress(node, COINBASE_MATURITY + 1, miner_wallet.getnewaddress())

        # Fund address to test
        wallet_addr = miner_wallet.getnewaddress()
        addr_desc = miner_wallet.getaddressinfo(wallet_addr)["desc"]
        tx_id = miner_wallet.sendtoaddress(wallet_addr, AMOUNT_SENT)

        # This wallet is not needed anymore, unload it here so that the following
        # block generations need not cause this wallet to process those notifications.
        miner_wallet.unloadwallet()

        # Generate enough blocks every 10 mins to surpass the 2 hours rescan window the wallet has.
        # 20 blocks every 10 mins equals 3hrs 20mins from now till last block - 20 mins more than
        # the start time of wallet to scan from while importing a descriptor with "now" timestamp.
        for _ in range(BLOCKS_TO_GENERATE):
            self.generate(node, 1)
            node.bumpmocktime(BLOCK_TIME)

        # Now create a new wallet to import the descriptor in
        node.createwallet(wallet_name='watch_only', disable_private_keys=True)
        wallet_watch_only = node.get_wallet_rpc('watch_only')
        # Blank wallets don't have a birth time
        assert 'birthtime' not in wallet_watch_only.getwalletinfo()

        # Import descriptor with timestamp=now
        wallet_watch_only.importdescriptors([{"desc": addr_desc, "timestamp": "now"}])
        assert_equal(len(wallet_watch_only.listtransactions()), 0)
        # As blocks were generated every 10 min, the chain MTP timestamp is (node.mocktime - 60 min).
        assert_equal(wallet_watch_only.getwalletinfo()['birthtime'], node.mocktime - BLOCK_TIME * 6)

        # Rescan the wallet to detect the missing transaction
        wallet_watch_only.rescanblockchain()

        assert_equal(len(wallet_watch_only.listtransactions()), 1)
        assert_equal(wallet_watch_only.getbalances()['mine']['trusted'], AMOUNT_RECEIVED)

        tx_info = wallet_watch_only.gettransaction(tx_id)
        assert_equal(tx_info['confirmations'], TX_CONFIRMATIONS)
        # Verify the wallet updated the birth time to the transaction time
        assert_equal(wallet_watch_only.getwalletinfo()['birthtime'], tx_info['time'])

        wallet_watch_only.unloadwallet()

    def run_test(self):
        node = self.nodes[0]

        self.birthtime_test(node)


if __name__ == '__main__':
    WalletBirthTimeTest(__file__).main()
