#!/usr/bin/env python3
# Copyright (c) 2021 The Litecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Basic MWEB Wallet test"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.ltc_util import setup_mweb_chain
from test_framework.util import assert_equal

class MWEBWalletBasicTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [['-whitelist=noban@127.0.0.1'],[]]  # immediate tx relay

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node0 = self.nodes[0]
        node1 = self.nodes[1]

        self.log.info("Setting up MWEB chain")
        setup_mweb_chain(node0)
        self.sync_all()

        self.log.info("Send to node1 mweb address")
        n1_addr0 = node1.getnewaddress(address_type='mweb')
        tx1_id = node0.sendtoaddress(n1_addr0, 25)
        self.sync_mempools()

        self.log.info("Verify node0's wallet lists the transaction as spent")
        n0_tx1 = node0.gettransaction(txid=tx1_id)
        assert_equal(n0_tx1['confirmations'], 0)
        assert n0_tx1['amount'] == -25
        assert n0_tx1['fee'] < 0 and n0_tx1['fee'] > -0.1

        self.log.info("Verify node1's wallet receives the unconfirmed transaction")
        n1_addr0_coins = node1.listreceivedbyaddress(minconf=0, address_filter=n1_addr0)
        assert_equal(len(n1_addr0_coins), 1)
        assert n1_addr0_coins[0]['amount'] == 25
        assert n1_addr0_coins[0]['confirmations'] == 0
        assert node1.getbalances()['mine']['untrusted_pending'] == 25
        assert node1.getbalances()['mine']['trusted'] == 0
        
        self.log.info("Mine the next block")
        node0.generate(1)
        self.sync_blocks()
        
        self.log.info("Verify node1's wallet lists the transaction as confirmed")
        n1_addr0_coins = node1.listunspent(addresses=[n1_addr0])
        assert_equal(len(n1_addr0_coins), 1)
        assert n1_addr0_coins[0]['amount'] == 25
        assert n1_addr0_coins[0]['confirmations'] == 1
        assert node1.getbalances()['mine']['untrusted_pending'] == 0
        assert node1.getbalances()['mine']['trusted'] == 25

        self.log.info("Verify node0's wallet lists the transaction as confirmed")
        n0_tx1 = node0.gettransaction(txid=tx1_id)
        assert_equal(n0_tx1['confirmations'], 1)
        assert n0_tx1['amount'] == -25
        assert n0_tx1['fee'] < 0 and n0_tx1['fee'] > -0.1


        # TODO: Conflicting txs
        # TODO: Duplicate hash


if __name__ == '__main__':
    MWEBWalletBasicTest().main()
